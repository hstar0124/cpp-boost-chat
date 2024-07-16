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
        auto const results = m_Resolver.resolve(m_Host, m_Port); // 호스트와 포트를 해결합니다.
        boost::asio::connect(m_Socket, results.begin(), results.end()); // 소켓을 호스트에 연결합니다.
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Connection failed: " << ex.what() << std::endl; // 연결 실패 시 예외 메시지를 출력합니다.
        exit(1);
    }
}

void HttpClient::Shutdown()
{
    boost::beast::error_code ec;
    m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec); // 소켓을 종료합니다.
}

UserResponse HttpClient::Post(const std::string& target, const google::protobuf::Message& message)
{
    Connect(); // 연결을 시도합니다.

    std::cout << "target : " << target << "\n"; // 목표 URL을 출력합니다.
    std::string body;
    if (!message.SerializeToString(&body)) // 메시지를 직렬화하여 바디로 설정합니다.
    {
        throw std::runtime_error("Failed to serialize message."); // 직렬화 실패 시 예외를 던집니다.
    }

    boost::beast::http::request<boost::beast::http::string_body> req{ boost::beast::http::verb::post, target, 11 };

    req.set(boost::beast::http::field::host, m_Host); // 요청 헤더에 호스트 정보를 설정합니다.
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING); // 사용자 에이전트 정보를 설정합니다.
    req.set(boost::beast::http::field::content_type, "application/x-protobuf"); // 컨텐츠 타입을 설정합니다.

    req.body() = body; // 요청 바디를 설정합니다.

    req.prepare_payload(); // 페이로드를 준비합니다.

    boost::beast::http::write(m_Socket, req); // 소켓을 통해 요청을 보냅니다.

    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::dynamic_body> res;
    boost::beast::http::read(m_Socket, buffer, res); // 소켓을 통해 응답을 읽어옵니다.

    return ParseResponse(res); // 응답을 파싱하여 반환합니다.
}

UserResponse HttpClient::Get(const std::string& target)
{
    Connect(); // 연결을 시도합니다.

    std::cout << "target : " << target << "\n"; // 목표 URL을 출력합니다.
    boost::beast::http::request<boost::beast::http::empty_body> req{ boost::beast::http::verb::get, target, 11 };
    req.set(boost::beast::http::field::host, m_Host); // 요청 헤더에 호스트 정보를 설정합니다.
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING); // 사용자 에이전트 정보를 설정합니다.

    boost::beast::http::write(m_Socket, req); // 소켓을 통해 요청을 보냅니다.

    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::dynamic_body> res;
    boost::beast::http::read(m_Socket, buffer, res); // 소켓을 통해 응답을 읽어옵니다.

    return ParseResponse(res); // 응답을 파싱하여 반환합니다.
}

UserResponse HttpClient::ParseResponse(boost::beast::http::response<boost::beast::http::dynamic_body>& res)
{
    std::cout << res << std::endl; // 응답을 출력합니다.

    std::string response_body = boost::beast::buffers_to_string(res.body().data());
    UserResponse response_message;

    if (!response_message.ParseFromString(response_body)) // 응답 메시지를 파싱합니다.
    {
        throw std::runtime_error("Failed to parse response message."); // 파싱 실패 시 예외를 던집니다.
    }

    return response_message; // 파싱된 응답 메시지를 반환합니다.
}