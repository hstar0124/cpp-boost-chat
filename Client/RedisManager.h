#pragma once
#include <boost/redis/src.hpp>
#include <boost/redis/connection.hpp>
#include <boost/redis/connection.hpp>
#include <boost/asio/detached.hpp>

class RedisManager
{
private:
    boost::redis::config m_Config;
    boost::asio::io_context& m_IoContext;
    std::shared_ptr<boost::redis::connection> m_Connection;

public:
    RedisManager(boost::asio::io_context& io_context, const std::string& host, const std::string& port);
    ~RedisManager();

    void Set(const std::string& key, const std::string& value);
    void Get(const std::string& key);
};
