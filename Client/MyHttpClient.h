#pragma once
#include <string>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <google/protobuf/util/json_util.h>


enum class RequestMethod 
{
    GET_USER,
    CREATE_USER,
};

template<typename RequestType, typename ResponseType>
class MyHttpClient 
{
private:
    boost::asio::io_context& ioContext;
    std::string host;
    std::string port;

public:
    MyHttpClient(boost::asio::io_context& io_context, const std::string& host, const std::string& port);

    bool SendRequest(RequestMethod method, const std::string& target, const RequestType& request_proto, ResponseType& response_proto);

private:
    bool Connect();
    std::string GetRequestMethodString(RequestMethod method);
};