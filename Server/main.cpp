#include "Common.h"
#include "TcpServer.h"
#include "DB/include/MySQLManager.h"
#include "DB/include/RedisClient.hpp"
#include "Util/HSThreadPool.hpp"
#include "Util/ConfigParser.hpp"
#include "Util/HsLogger.hpp"

int main()
{
	ConfigParser parser("config.txt");

	if (!parser.readConfig())
	{
		std::cerr << "Error: Failed to read configuration from file" << std::endl;
		LOG(LogLevel::ERR, "Error: Failed to read configuration from file");
		return 1;
	}

	const auto& config = parser.getConfig();


	HSThreadPool threadPool(10);

	// Redis 초기화
	std::unique_ptr<CRedisClient> redisClient = std::make_unique<CRedisClient>();
	if (!redisClient->Initialize(config.at("redis_host"), stoi(config.at("redis_port")), 2, 10))
	{
		LOG(LogLevel::ERR, "connect to redis failed");
		std::cout << "connect to redis failed" << std::endl;
		exit(1);
	}
	else
	{
		LOG(LogLevel::INFO, "connect to redis Success!");
		std::cout << "connect to redis Success!" << std::endl;
	}

	// MySQL 초기화
	std::unique_ptr<MySQLManager> mysqlManager = std::make_unique<MySQLManager>(config.at("mysql_host"), config.at("mysql_id"), config.at("mysql_pw"), config.at("mysql_db"));

	// Socket Server 초기화
	boost::asio::io_context io_context;
	TcpServer tcpServer(io_context, 4242, std::move(redisClient), std::move(mysqlManager), threadPool);

	if (!tcpServer.Start(2))
	{
		LOG(LogLevel::ERR, "Server Error!!");
		std::cerr << "[SERVER] Server Error!!" << "\n";
	}

	tcpServer.Update();

	return 0;

}