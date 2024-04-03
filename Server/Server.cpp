#include <iostream>
#include "Common.h"


using boost::asio::ip::tcp;
using namespace boost;


// 서버가 클라이언트랑 연결 후 해당 클라이언트와 통신을 하기 위한 클래스
class TcpSession
{
public:
	TcpSession(asio::io_context& io_context, std::deque<std::string>& deq_msg)
		: m_Socket(io_context), m_IoContext(io_context), m_DeqMessages(deq_msg)
	{
		memset(m_RecvBuffer, 0, m_RecvBufferSize);
		memset(m_SendBuffer, 0, m_SendBufferSize);
	}

	tcp::socket& GetSocket()
	{
		return m_Socket;
	}

	void Start()
	{
		AsyncRead();
	}

	void Send(const std::string msg)
	{
		asio::post(m_IoContext,
			[this, msg]()
			{
				bool bWritingMessage = !m_DeqMessages.empty();
				m_DeqMessages.push_back(msg);
				if (!bWritingMessage)
				{
					AsyncWrite();
				}
			});
	}

private:

	void AsyncRead()
	{
		m_Socket.async_read_some(boost::asio::buffer(m_RecvBuffer, m_RecvBufferSize),
			[this](const boost::system::error_code& err, const size_t size)
			{
				this->OnRead(err, size);
			});
	}

	void OnRead(const boost::system::error_code& err, const size_t size)
	{
		std::cout << "OnRead" << size << ", " << m_RecvBuffer << std::endl;

		if (!err)
		{
			m_DeqMessages.push_back(m_RecvBuffer);
			AsyncRead();
		}
	}

	void AsyncWrite()
	{
		boost::asio::async_write(m_Socket, boost::asio::buffer(&m_DeqMessages.front(), m_SendBufferSize),
			[this](const boost::system::error_code& err, const size_t transferred)
			{
				this->OnWrite(err, transferred);
			});
	}

	void OnWrite(const boost::system::error_code& err, const size_t transferred)
	{
		std::cout << "OnWrite" << transferred << std::endl;

		if (!err)
		{
			if (!m_DeqMessages.empty())
			{
				m_DeqMessages.pop_front();
				AsyncWrite();
			}
		}
		else
		{
			std::cout << "Error " << err.message() << std::endl;
		}
	}


private:
	tcp::socket m_Socket;
	asio::io_context& m_IoContext;

	std::deque<std::string>& m_DeqMessages;

	static const int m_RecvBufferSize = 1024;
	char m_RecvBuffer[m_RecvBufferSize];

	static const int m_SendBufferSize = 1024;
	char m_SendBuffer[m_SendBufferSize];

};


// 비동기 서버 클래스
class TcpServer
{
public:
	TcpServer(asio::io_context& io_context, int port)
		: m_Acceptor(io_context, tcp::endpoint(tcp::v4(), port)), m_IoContext(io_context)

	{

	}

	bool Start()
	{
		try
		{
			StartAccept();
			m_ThreadContext = std::thread([this]() { m_IoContext.run(); });
		}
		catch (std::exception& e)
		{
			// 서버가 리스닝을 할 수 없는 경우
			std::cerr << "Exception: " << e.what() << "\n";
			return false;
		}

		std::cout << "Started!\n";
		return true;
	}

	// 클라이언트와 통신을 위한 세션을 생성한 후 비동기 accept함수인 async_accept를 호출
	// 첫 번째 인자로는 연결 후 할당될 소켓을 전달하며, 두 번째 인자로는 async_accept 함수가 성공적으로 수행되고 호출될 함수 포인터를 전달합니다.
	void StartAccept()
	{
		TcpSession* tcpSession = new TcpSession(m_IoContext, m_DeqMessages);
		m_Acceptor.async_accept(tcpSession->GetSocket(),
			[this, tcpSession](const boost::system::error_code& err)
			{
				this->OnAccept(tcpSession, err);
			});
	}

	// 전달한 세션의 Start를 호출해서 통신을 시작하고, StartAccept를 호출하여 다시 클라이언의 접속을 비동기적으로 대기합니다.
	void OnAccept(TcpSession* tcpSession, const boost::system::error_code& err)
	{
		if (!err)
		{
			tcpSession->Start();
			m_DeqSessions.push_back(tcpSession);
			std::cout << "Accept Session : " << tcpSession->GetSocket().remote_endpoint() << std::endl;
		}
		else
		{
			std::cout << "Error " << err.message() << std::endl;
		}

		StartAccept();
	}

	void OnMessage()
	{
		if (!m_DeqMessages.empty())
		{
			for (auto& session : m_DeqSessions)
			{
				std::string msg = m_DeqMessages.front();
				session->Send(msg);
			}
		}
	}



private:
	tcp::acceptor m_Acceptor;
	asio::io_context& m_IoContext;
	std::thread m_ThreadContext;

	std::deque<TcpSession*> m_DeqSessions;
	std::deque<std::string> m_DeqMessages;
};






int main()
{
	boost::asio::io_context io_context;
	TcpServer tcpServer(io_context, 4242);
	tcpServer.Start();

	while (1)
	{
		tcpServer.OnMessage();
	}

	return 0;
}
