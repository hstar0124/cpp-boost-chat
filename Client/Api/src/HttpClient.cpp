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
        std::cout << "Http Client Connect! " << "\n";
        auto const results = m_Resolver.resolve(m_Host, m_Port); // ȣ��Ʈ�� ��Ʈ�� �ذ��մϴ�.
        boost::asio::connect(m_Socket, results.begin(), results.end()); // ������ ȣ��Ʈ�� �����մϴ�.
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Connection failed: " << ex.what() << std::endl; // ���� ���� �� ���� �޽����� ����մϴ�.
        exit(1);
    }
}

void HttpClient::Shutdown()
{
    boost::beast::error_code ec;
    m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec); // ������ �����մϴ�.
}

UserResponse HttpClient::Post(const std::string& target, const google::protobuf::Message& message)
{
    Connect(); // ������ �õ��մϴ�.

    std::cout << "target : " << target << "\n"; // ��ǥ URL�� ����մϴ�.
    std::string body;
    if (!message.SerializeToString(&body)) // �޽����� ����ȭ�Ͽ� �ٵ�� �����մϴ�.
    {
        throw std::runtime_error("Failed to serialize message."); // ����ȭ ���� �� ���ܸ� �����ϴ�.
    }

    boost::beast::http::request<boost::beast::http::string_body> req{ boost::beast::http::verb::post, target, 11 };

    req.set(boost::beast::http::field::host, m_Host); // ��û ����� ȣ��Ʈ ������ �����մϴ�.
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING); // ����� ������Ʈ ������ �����մϴ�.
    req.set(boost::beast::http::field::content_type, "application/x-protobuf"); // ������ Ÿ���� �����մϴ�.

    req.body() = body; // ��û �ٵ� �����մϴ�.

    req.prepare_payload(); // ���̷ε带 �غ��մϴ�.

    boost::beast::http::write(m_Socket, req); // ������ ���� ��û�� �����ϴ�.

    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::dynamic_body> res;
    boost::beast::http::read(m_Socket, buffer, res); // ������ ���� ������ �о�ɴϴ�.

    return ParseResponse(res); // ������ �Ľ��Ͽ� ��ȯ�մϴ�.
}

UserResponse HttpClient::Get(const std::string& target)
{
    Connect(); // ������ �õ��մϴ�.

    std::cout << "target : " << target << "\n"; // ��ǥ URL�� ����մϴ�.
    boost::beast::http::request<boost::beast::http::empty_body> req{ boost::beast::http::verb::get, target, 11 };
    req.set(boost::beast::http::field::host, m_Host); // ��û ����� ȣ��Ʈ ������ �����մϴ�.
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING); // ����� ������Ʈ ������ �����մϴ�.

    boost::beast::http::write(m_Socket, req); // ������ ���� ��û�� �����ϴ�.

    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::dynamic_body> res;
    boost::beast::http::read(m_Socket, buffer, res); // ������ ���� ������ �о�ɴϴ�.

    return ParseResponse(res); // ������ �Ľ��Ͽ� ��ȯ�մϴ�.
}

UserResponse HttpClient::ParseResponse(boost::beast::http::response<boost::beast::http::dynamic_body>& res)
{
    std::cout << res << std::endl; // ������ ����մϴ�.

    std::string response_body = boost::beast::buffers_to_string(res.body().data());
    UserResponse response_message;

    if (!response_message.ParseFromString(response_body)) // ���� �޽����� �Ľ��մϴ�.
    {
        throw std::runtime_error("Failed to parse response message."); // �Ľ� ���� �� ���ܸ� �����ϴ�.
    }

    return response_message; // �Ľ̵� ���� �޽����� ��ȯ�մϴ�.
}