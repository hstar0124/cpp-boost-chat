#include "Common.h"
#include "TcpServer.h"
#include "MySQLManager.h"




int main()
{
	HSThreadPool threadPool(10);

	// Redis 초기화
	std::unique_ptr<CRedisClient> redisClient = std::make_unique<CRedisClient>();
	if (!redisClient->Initialize("127.0.0.1", 6379, 2, 10))
	{
		std::cout << "connect to redis failed" << std::endl;
	}
	std::cout << "connect to redis Success!" << std::endl;

	// MySQL 초기화
	std::unique_ptr<MySQLManager> mysqlManager = std::make_unique<MySQLManager>("127.0.0.1", "root", "root", "hstar");

	// Socket Server 초기화
	boost::asio::io_context io_context;
	TcpServer tcpServer(io_context, 4242, std::move(redisClient), std::move(mysqlManager), threadPool);

	if (!tcpServer.Start(2))
	{
	  std::cerr << "[SERVER] Server Error!!" << "\n";
	}

	tcpServer.Update();

	return 0;

}