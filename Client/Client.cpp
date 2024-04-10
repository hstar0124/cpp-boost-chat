#include "Common.h"
#include "Message.h"
#include "ThreadSafeQueue.h"

class TcpClient
{
public:

	TcpClient(boost::asio::io_context& io_context)
		: m_IoContext(io_context), m_Socket(io_context)
	{
	}

	void Connect(std::string host, int port)
	{
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(host), port);
		m_Endpoint = endpoint;

		m_Socket.async_connect(endpoint, [this](const boost::system::error_code& err)
			{
				this->OnConnect(err);
			});


	}

	void OnConnect(const boost::system::error_code& err)
	{
		std::cout << "OnConnect" << std::endl;
		if (!err)
		{
			ReadHeader();
		}
		else
		{
			std::cout << "[CLIENT] ERROR : " << err.message() << "\n";
		}
	}

	void Send(const std::string& userInput)
	{
		// TODO : 메시지 분류별로 전송
		Message msg;
		msg.header.id = PacketType::ServerPing;
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
		msg << timeNow;



		boost::asio::post(m_IoContext,
			[this, msg]()
			{
				// 큐에 메시지가 있으면 현재 비동기적으로 쓰여지고 있다고 가정해야 합니다.
				// 아무튼 메시지를 출력할 큐에 메시지를 추가합니다. 
				// 쓰여질 메시지가 없으면 큐의 처음에 있는 
				// 메시지의 쓰기 프로세스를 시작합니다.
				bool bWritingMessage = !m_MessagesOut.Empty();
				m_MessagesOut.PushBack(msg);
				if (!bWritingMessage)
				{
					WriteHeader();
				}
			});
	}

private:
	void WriteHeader()
	{
		boost::asio::async_write(m_Socket, boost::asio::buffer(&m_MessagesOut.Front().header, sizeof(MessageHeader)),
			[this](const boost::system::error_code& err, const size_t size)
			{
				// 에러 코드 없을 시
				if (!err)
				{
					// 오류가 없으므로 방금 보낸 메시지 헤더에
					// 메시지 본문도 있는지 확인
					if (m_MessagesOut.Front().body.size() > 0)
					{
						// 본문 바이트를 쓰는 작업을 실행
						WriteBody();
					}
					else
					{
						m_MessagesOut.Pop();

						// 큐가 비어 있지 않으면 더 보낼 메시지가 있으므로
						// 다음 헤더를 보내는 작업을 발행합니다.
						if (!m_MessagesOut.Empty())
						{
							WriteHeader();
						}
					}
				}
				else
				{
					// 일단 단순히 연결이 종료되었다고 가정하고 소켓을 닫기
					std::cout << "[CLIENT] Write Header Fail.\n";
					m_Socket.close();
				}
			});
	}

	void WriteBody()
	{
		boost::asio::async_write(m_Socket, boost::asio::buffer(m_MessagesOut.Front().body.data(), m_MessagesOut.Front().body.size()),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					m_MessagesOut.Pop();

					if (!m_MessagesOut.Empty())
					{
						WriteHeader();
					}
				}
				else
				{
					// 보내기 실패, WriteHeader()와 동일
					std::cout << "[CLIENT] Write Body Fail.\n";
					m_Socket.close();
				}
			});
	}

	void ReadHeader()
	{
		boost::asio::async_read(m_Socket, boost::asio::buffer(&m_TemporaryMessage.header, sizeof(MessageHeader)),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					// 메시지 헤더가 다 읽혔는지 확인
					if (m_TemporaryMessage.header.size > 0)
					{
						// 메시지의 본문에 충분한 공간을 할당하고,
						// asio에 본문을 읽을 작업을 실행
						m_TemporaryMessage.body.resize(m_TemporaryMessage.header.size);
						ReadBody();
					}
					else
					{
						ReadHeader();
					}
				}
				else
				{
					// 클라이언트로부터의 읽기가 실패하는 경우
					// 소켓을 닫고 시스템이 이후 정리하도록 합니다.
					std::cout << "[CLIENT] Read Header Fail.\n";
					m_Socket.close();
				}
			});
	}

	void ReadBody()
	{
		boost::asio::async_read(m_Socket, boost::asio::buffer(m_TemporaryMessage.body.data(), m_TemporaryMessage.body.size()),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
					std::chrono::system_clock::time_point timeThen;
					m_TemporaryMessage >> timeThen;
					std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
					ReadHeader();
				}
				else
				{
					std::cout << "[CLIENT] Read Body Fail.\n";
					m_Socket.close();
				}
			});
	}

	

private:
	
	boost::asio::ip::tcp::endpoint m_Endpoint;	
	boost::asio::io_context& m_IoContext;
	boost::asio::ip::tcp::socket m_Socket;
	std::string m_Meaasge;
		
	Message m_TemporaryMessage;
	ThreadSafeQueue<Message> m_MessagesOut;
};

int main()
{
	boost::asio::io_context io_context;
	TcpClient client(io_context);
	client.Connect(std::string("127.0.0.1"), 4242);

	// 비동기 I/O 작업이 처리된 후에 호출되는 함수를 실행시켜주는 함수
	// 이 함수는 블록 함수 새로운 비동기 작업이 등록되거나, 기존 작업이 완료될 때까지 대기합니다.

	std::thread thread([&io_context]() { io_context.run(); });

	char userInput[100];
	while (std::cin.getline(userInput, 100))
	{	
		client.Send(userInput);
	}

}
