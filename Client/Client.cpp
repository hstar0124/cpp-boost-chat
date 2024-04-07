#include "Common.h"

class TcpClient
{
public:

	TcpClient(boost::asio::io_context& io_context)
		: m_Socket(io_context)
	{
		memset(m_RecvBuffer, 0, m_RecvBufferSize);
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
			AsyncRead();
		}
	}

	void AsyncWrite(std::string message)
	{
		m_Meaasge = message;
		boost::asio::async_write(m_Socket, boost::asio::buffer(m_Meaasge), 
			[this](const boost::system::error_code& err, const size_t transferred)
			{
				this->OnWrite(err, transferred);
			});
	}

	void OnWrite(const boost::system::error_code& err, const size_t transferred)
	{
		std::cout << "OnWrite" << std::endl;
	}

	void AsyncRead()
	{
		boost::asio::async_read(m_Socket, boost::asio::buffer(m_RecvBuffer, 16),
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
			AsyncRead();
		}
	}


private:
	static const int m_RecvBufferSize = 1024;
	boost::asio::ip::tcp::endpoint m_Endpoint;
	char m_RecvBuffer[m_RecvBufferSize];
	boost::asio::ip::tcp::socket m_Socket;
	std::string m_Meaasge;
};

int main()
{
	boost::asio::io_context io_context;
	TcpClient client(io_context);
	client.Connect(std::string("127.0.0.1"), 4242);

	// 비동기 I/O 작업이 처리된 후에 호출되는 함수를 실행시켜주는 함수
	// 이 함수는 블록 함수 새로운 비동기 작업이 등록되거나, 기존 작업이 완료될 때까지 대기합니다.
	std::thread thread([&io_context]() { io_context.run(); });

	while (true)
	{
		std::string message;
		std::cin >> message;
		client.AsyncWrite(message);
	}

	thread.join();
}
