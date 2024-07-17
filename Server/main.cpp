#include "Common.h"
#include "TcpServer.h"
#include "DB/include/MySQLManager.h"
#include "DB/include/RedisClient.hpp"
#include "Util/HSThreadPool.hpp"
#include "Util/ConfigParser.hpp"
#include "Util/HsLogger.hpp"

int main()
{
	Logger::instance().init(LogLevel::LOG_DEBUG, LogPeriod::DAY, true, false); // 파일에만 로그 출력
	LOG_INFO("Connected to Redis successfully.");
	ConfigParser parser("config.txt");

	if (!parser.readConfig())
	{
		std::cerr << "Error: Failed to read configuration from file" << std::endl;
		LOG_ERROR("Error: Failed to read configuration from file");
		return 1;
	}

	const auto& config = parser.getConfig();


	HSThreadPool threadPool(10);

	// Redis 초기화
	std::unique_ptr<CRedisClient> redisClient = std::make_unique<CRedisClient>();
	try
	{
		if (redisClient->Initialize(config.at("redis_host"), stoi(config.at("redis_port")), 2, 10))
		{
			LOG_INFO("Connected to Redis successfully.");
			std::cout << "Connected to Redis successfully." << std::endl;
		}
		else
		{
			throw std::runtime_error("Failed to connect to Redis.");
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		LOG_ERROR("Redis connection error: {}", e.what());
	}


	// MySQL 초기화
	std::unique_ptr<MySQLManager> mysqlManager = std::make_unique<MySQLManager>(config.at("mysql_host"), config.at("mysql_id"), config.at("mysql_pw"), config.at("mysql_db"));

	// Socket Server 초기화
	boost::asio::io_context io_context;
	TcpServer tcpServer(io_context, 4242, std::move(redisClient), std::move(mysqlManager), threadPool);

	int maxUser = 2;
	if (!tcpServer.Start(maxUser))
	{
		std::cerr << "[SERVER] Server Error!!" << "\n";
		LOG_ERROR("Tcp Server Start Error!!");
	}

	tcpServer.Update();

	return 0;

}