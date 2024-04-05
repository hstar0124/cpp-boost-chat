#include "Common.h"

using boost::asio::ip::tcp;
using namespace boost;

class User 
{
public:
	virtual ~User() {}
	virtual void Send(const char* message) = 0;
};

class TcpSessionManager
{
public:
	void Connection(std::shared_ptr<User> session)
	{
		sessions.push_back(session);
	}
	void Disconnection(std::shared_ptr<User> session)
	{
		sessions.erase(std::remove(sessions.begin(), sessions.end(), session), sessions.end());
	}
	void AllMessage(const char* message)
	{
		std::cout << "[TcpSessionManager] msg : " << message << std::endl;
		for (auto s : sessions)
		{
			s->Send(message);
		}
	}
	void DirectMessage();

private:
	std::vector<std::shared_ptr<User>> sessions;
};

// 서버가 클라이언트랑 연결 후 해당 클라이언트와 통신을 하기 위한 클래스
class TcpSession : public User, public std::enable_shared_from_this<TcpSession>
{
public:
	TcpSession(asio::io_context& io_context, TcpSessionManager& tcpSessionMgr)
		: m_Socket(io_context), m_TcpSessionMgr(tcpSessionMgr)
	{
		memset(m_RecvBuffer, 0, m_RecvBufferSize);
		memset(m_SendBuffer, 0, m_SendBufferSize);
	}

	// 외부에서 TcpSession 객체를 생성할 수 없게 하기 위해 정적 팩토리 메서드를 제공
	static std::shared_ptr<TcpSession> Create(boost::asio::io_context& io_context, TcpSessionManager& tcpSessionMgr)
	{
		// shared_ptr로 생성된 객체를 반환
		return std::shared_ptr<TcpSession>(new TcpSession(io_context, tcpSessionMgr));
	}

	// Start 메서드를 통해 객체를 초기화하고 TcpSessionManager에 등록
	void Start()
	{
		m_TcpSessionMgr.Connection(shared_from_this());
		AsyncRead();
	}

	tcp::socket& GetSocket()
	{
		return m_Socket;
	}

	void Send(const char* message)
	{
		AsyncWrite(message);
	}

	void Close()
	{
		m_Socket.close(); // 소켓을 닫습니다.
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
		std::cout << "[TcpSession] OnRead : " << m_RecvBuffer << std::endl;

		if (!err)
		{
			AsyncRead();
			//AsyncWrite(m_RecvBuffer, size);
			m_TcpSessionMgr.AllMessage(m_RecvBuffer);

		}
		else
		{
			std::cout << "Error " << err.message() << std::endl;
			m_TcpSessionMgr.Disconnection(shared_from_this());
		}
	}

	void AsyncWrite(const char* message)
	{
		memcpy(m_SendBuffer, message, m_SendBufferSize);

		boost::asio::async_write(m_Socket, boost::asio::buffer(m_SendBuffer, m_SendBufferSize),
			[this](const boost::system::error_code& err, const size_t transferred)
			{
				this->OnWrite(err, transferred);
			});
	}

	void OnWrite(const boost::system::error_code& err, const size_t transferred)
	{
		std::cout << "[TcpSession] OnWrite : " << m_SendBuffer << std::endl;
		if (!err)
		{
			// 전송이 성공적으로 완료됐을 때 처리할 내용
		}
		else
		{
			std::cout << "Error " << err.message() << std::endl;
			m_TcpSessionMgr.Disconnection(shared_from_this());
		}
	}


private:
	tcp::socket m_Socket;
	TcpSessionManager& m_TcpSessionMgr;
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

	// 클라이언트와 통신을 위한 세션을 생성한 후 비동기 accept함수인 async_accept를 호출
	// 첫 번째 인자로는 연결 후 할당될 소켓을 전달하며, 두 번째 인자로는 async_accept 함수가 성공적으로 수행되고 호출될 함수 포인터를 전달합니다.
	void StartAccept()
	{
		auto tcpSession = TcpSession::Create(m_IoContext, m_TcpSessionMgr);
		m_Acceptor.async_accept(tcpSession->GetSocket(),
			[this, tcpSession](const boost::system::error_code& err)
			{
				this->OnAccept(tcpSession, err);
			});
	}

	// 전달한 세션의 Start를 호출해서 통신을 시작하고, StartAccept를 호출하여 다시 클라이언의 접속을 비동기적으로 대기합니다.
	void OnAccept(std::shared_ptr<TcpSession> tcpSession, const boost::system::error_code& err)
	{
		if (!err)
		{
			std::cout << "[TcpServer] Accept" << std::endl;
			tcpSession->Start();

		}
		else
		{
			std::cout << "Error " << err.message() << std::endl;
		}

		StartAccept();
	}



private:
	asio::io_context& m_IoContext;
	tcp::acceptor m_Acceptor;
	TcpSessionManager m_TcpSessionMgr;
};






int main()
{
	boost::asio::io_context io_context;
	TcpServer tcpServer(io_context, 4242);
	tcpServer.StartAccept();
	std::cout << "Start Server" << std::endl;

	io_context.run();
	return 0;
}
