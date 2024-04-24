#include "Common.h"
#include "Message.h"
#include "ThreadSafeQueue.h"
#include "Payload.pb.h"

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
		std::shared_ptr<myPayload::Payload> pl = std::make_shared<myPayload::Payload>();

		if (userInput.substr(0, 2) == "/w") {
			size_t spacePos = userInput.find(' ');
			size_t receiverEndPos = userInput.find(' ', spacePos + 1);

			if (spacePos == std::string::npos || userInput.size() <= spacePos + 1 || receiverEndPos == std::string::npos) {
				std::cout << "[CLIENT] 잘못된 메시지 포맷입니다. 형식: /w [수신자] [메시지]\n";
				return;
			}

			std::string receiver = userInput.substr(spacePos + 1, receiverEndPos - spacePos - 1);
			std::string message = userInput.substr(receiverEndPos + 1);

			pl->set_payloadtype(myPayload::PayloadType::WHISPER_MESSAGE);
			pl->set_receiver(receiver);
			pl->set_content(message);
		}
		else if (userInput.substr(0, 2) == "/p") 
		{
			if (userInput.size() > 2) 
			{
				pl->set_payloadtype(myPayload::PayloadType::PARTY_MESSAGE);
				pl->set_content(userInput.substr(2));
			}
			else 
			{
				std::cout << "[CLIENT] 잘못된 메시지 포맷입니다. 형식: /p [메시지]\n";
				return;
			}
		}
		else {
			pl->set_payloadtype(myPayload::PayloadType::ALL_MESSAGE);
			pl->set_content(userInput);
		}

		AsyncWrite(pl);
	}

	void SendPong()
	{
		// 현재 시간을 milliseconds로 얻어옴
		auto now = std::chrono::system_clock::now();
		auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
		auto value = now_ms.time_since_epoch().count();

		std::shared_ptr<myPayload::Payload> pl = std::make_shared<myPayload::Payload>();
		pl->set_payloadtype(myPayload::PayloadType::SERVER_PING);
		pl->set_content(std::to_string(value));

		AsyncWrite(pl);
	}

private:
	void AsyncWrite(std::shared_ptr<myPayload::Payload> payload)
	{
		int size = static_cast<int>(payload->ByteSizeLong());

		std::vector<uint8_t> buffer;
		buffer.resize(HEADER_SIZE + size);

		payload->SerializePartialToArray(buffer.data() + HEADER_SIZE, size);

		int bodySize = static_cast<int>(buffer.size()) - HEADER_SIZE;
		buffer[0] = static_cast<uint8_t>((bodySize >> 24) & 0xFF);
		buffer[1] = static_cast<uint8_t>((bodySize >> 16) & 0xFF);
		buffer[2] = static_cast<uint8_t>((bodySize >> 8) & 0xFF);
		buffer[3] = static_cast<uint8_t>(bodySize & 0xFF);

		boost::asio::async_write(m_Socket, boost::asio::buffer(buffer.data(), buffer.size()),
			[this](const boost::system::error_code& err, const size_t size)
			{
				this->OnWrite(err, size);
			});
	}

	void OnWrite(const boost::system::error_code& err, const size_t size)
	{
	}

	void ReadHeader()
	{
		m_Readbuf.clear();
		m_Readbuf.resize(HEADER_SIZE);

		boost::asio::async_read(m_Socket, boost::asio::buffer(m_Readbuf, HEADER_SIZE),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					size_t bodySize = 0;
					for (int j = 0; j < m_Readbuf.size(); j++) {
						bodySize = static_cast<size_t>(m_Readbuf[j]);
					}
					if (bodySize > 0)
					{
						ReadBody(bodySize);
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

	void ReadBody(size_t bodySize)
	{
		m_Readbuf.clear();
		m_Readbuf.resize(bodySize);

		boost::asio::async_read(m_Socket, boost::asio::buffer(m_Readbuf),
			[this](std::error_code ec, std::size_t size)
			{
				if (!ec)
				{
					// 디시리얼라이즈할 Payload 메시지 객체 생성
					std::shared_ptr<myPayload::Payload> payload = std::make_shared<myPayload::Payload>();

					// m_readbuf를 Payload 메시지로 디시리얼라이즈
					if (payload->ParseFromArray(m_Readbuf.data(), size))
					{
						MessageHandler(payload);
						ReadHeader();
					}
					else
					{
						// 디시리얼라이즈 실패
						std::cerr << "Failed to parse Payload message" << std::endl;
					}

				}
				else
				{
					std::cout << "[CLIENT] Read Body Fail.\n";
					m_Socket.close();
				}
			});
	}

	void MessageHandler(std::shared_ptr<myPayload::Payload>& payload)
	{
		//std::cout << "Payload Type: " << payload->payloadtype() << std::endl;
		if (payload->payloadtype() == myPayload::PayloadType::ALL_MESSAGE)
		{
			std::cout << "[" << payload->sender() << "] " << payload->content() << std::endl;
			return;
		}

		if (payload->payloadtype() == myPayload::PayloadType::SERVER_PING)
		{
			auto sentTimeMs = std::stoll(payload->content());
			auto sentTime = std::chrono::milliseconds(sentTimeMs);
			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);
			//std::cout << "Response time: " << elapsedTime.count() << "ms" << std::endl;
			SendPong();

			return;
		}

		if (payload->payloadtype() == myPayload::PayloadType::WHISPER_MESSAGE)
		{
			std::cout << "<<" << payload->sender() << ">> " << payload->content() << std::endl;
			return;
		}

		if (payload->payloadtype() == myPayload::PayloadType::ERROR_MESSAGE)
		{
			std::cout << payload->content() << std::endl;
			return;
		}
	}

private:

	boost::asio::ip::tcp::endpoint m_Endpoint;
	boost::asio::io_context& m_IoContext;
	boost::asio::ip::tcp::socket m_Socket;
	std::string m_Meaasge;

	Message m_TemporaryMessage;
	std::vector<uint8_t> m_Readbuf;
};

int main()
{
	boost::asio::io_context io_context;
	TcpClient client(io_context);
	client.Connect(std::string("127.0.0.1"), 4242);

	// 비동기 I/O 작업이 처리된 후에 호출되는 함수를 실행시켜주는 함수
	// 이 함수는 블록 함수 새로운 비동기 작업이 등록되거나, 기존 작업이 완료될 때까지 대기합니다.

	std::thread thread([&io_context]() { io_context.run(); });

	std::string userInput;
	while (getline(std::cin, userInput))
	{
		client.Send(userInput);
	}

}
