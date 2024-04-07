#include "Common.h"
#include "TcpServer.h"


int main()
{
	boost::asio::io_context io_context;

	TcpServer tcpServer(io_context, 4242);
	tcpServer.Start();

	while (1)
	{
		tcpServer.Update(-1, true);
	}

	return 0;
}
