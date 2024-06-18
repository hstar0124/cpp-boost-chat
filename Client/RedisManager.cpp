#include "RedisManager.h"

RedisManager::RedisManager(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
    : m_IoContext(io_context), m_Connection(std::make_shared<boost::redis::connection>(io_context))
{
    m_Config.addr.host = host;
    m_Config.addr.port = port;

    m_Connection->async_run(m_Config, {}, boost::asio::detached);
}

RedisManager::~RedisManager()
{
}

void RedisManager::Set(const std::string& key, const std::string& value)
{
    boost::redis::request req;
    req.push("SET", key, value);
    boost::redis::response<std::string> resp;

    m_Connection->async_exec(req, resp, [&](auto ec, auto)
        {
            if (!ec)
            {
                std::cout << "Result: " << std::get<0>(resp).value() << std::endl;
            }
            else
            {
                std::cerr << "Set operation failed: " << ec.message() << std::endl;
            }
        });
}

void RedisManager::Get(const std::string& key)
{
    boost::redis::request req;
    req.push("GET", key);
    boost::redis::response<std::string> resp;

    m_Connection->async_exec(req, resp, [&](auto ec, auto)
        {
            if (!ec)
            {
                std::cout << "Value for key " << key << ": " << std::get<0>(resp).value() << std::endl;
            }
            else
            {
                std::cerr << "Get operation failed: " << ec.message() << std::endl;
            }
        });
}