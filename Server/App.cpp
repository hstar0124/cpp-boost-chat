#include "Common.h"
#include "TcpServer.h"


int main()
{
	boost::asio::io_context io_context;

	TcpServer tcpServer(io_context, 4242);
	if (!tcpServer.Start())
	{
		std::cerr << "[SERVER] Server Error!!" << "\n";
	}

	tcpServer.Update();

	return 0;
}
