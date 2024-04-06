#include "Common.h"
#include "Acceptor.h"


int main()
{
	boost::asio::io_context io_context;
	Acceptor tcpServer(io_context, 4242);
	tcpServer.StartAccept();
	std::cout << "Start Server" << std::endl;

	io_context.run();
	return 0;
}
