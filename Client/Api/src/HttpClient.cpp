#include "Api/include/HttpClient.h"
#include <iostream>
#include <stdexcept>


HttpClient::HttpClient(boost::asio::io_context& ioc, const std::string& host, const std::string& port)
    : m_Host(host), m_Port(port), io_context(ioc), m_Resolver(ioc), m_Socket(ioc) 
{
}

HttpClient::~HttpClient() 
{
    Shutdown();
}

void HttpClient::Connect()
{
    try
    {
        auto const results = m_Resolver.resolve(m_Host, m_Port);
        boost::asio::connect(m_Socket, results.begin(), results.end());
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Connection failed: " << ex.what() << std::endl;
        exit(1);
    }
}

void HttpClient::Shutdown() 
{
    boost::beast::error_code ec;
    m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
}

UserResponse HttpClient::Post(const std::string& target, const google::protobuf::Message& message) 
{

    Connect();

    std::cout << "target : " << target << "\n";
    std::string body;
    if (!message.SerializeToString(&body)) 
    {
        throw std::runtime_error("Failed to serialize message.");
    }

    boost::beast::http::request<boost::beast::http::string_body> req{ boost::beast::http::verb::post, target, 11 };

    req.set(boost::beast::http::field::host, m_Host);
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(boost::beast::http::field::content_type, "application/x-protobuf");

    req.body() = body;

    req.prepare_payload();

    boost::beast::http::write(m_Socket, req);

    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::dynamic_body> res;
    boost::beast::http::read(m_Socket, buffer, res);

    return ParseResponse(res);
}

UserResponse HttpClient::Get(const std::string& target) 
{
    Connect();

    std::cout << "target : " << target << "\n";
    boost::beast::http::request<boost::beast::http::empty_body> req{ boost::beast::http::verb::get, target, 11 };
    req.set(boost::beast::http::field::host, m_Host);
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    boost::beast::http::write(m_Socket, req);

    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::dynamic_body> res;
    boost::beast::http::read(m_Socket, buffer, res);

    return ParseResponse(res);
}

UserResponse HttpClient::ParseResponse(boost::beast::http::response<boost::beast::http::dynamic_body>& res) 
{
    std::cout << res << std::endl;

    std::string response_body = boost::beast::buffers_to_string(res.body().data());
    UserResponse response_message;

    if (!response_message.ParseFromString(response_body)) 
    {
        throw std::runtime_error("Failed to parse response message.");
    }

    return response_message;
}