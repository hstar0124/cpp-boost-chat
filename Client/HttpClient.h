#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <google/protobuf/message.h>
#include "UserMessage.pb.h"  // 여기에 실제 Proto 파일의 경로를 사용하세요
#include <memory>
#include <string>


class HttpClient {
public:
    HttpClient(boost::asio::io_context& ioc, const std::string& host, const std::string& port);
    ~HttpClient();

    UserResponse Post(const std::string& target, const google::protobuf::Message& message);
    UserResponse Get(const std::string& target);

private:
    void Connect();
    void Shutdown();
    UserResponse ParseResponse(boost::beast::http::response<boost::beast::http::dynamic_body>& res);

    std::string m_Host;
    std::string m_Port;
    boost::asio::io_context& io_context;
    boost::asio::ip::tcp::resolver m_Resolver;
    boost::asio::ip::tcp::socket m_Socket;
};
