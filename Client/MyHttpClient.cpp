#include "MyHttpClient.h"
#include "ProtobufHttpMessage.hpp"

template<typename RequestType, typename ResponseType>
MyHttpClient<RequestType, ResponseType>::MyHttpClient(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
    : ioContext(io_context), host(host), port(port) 
{}

template<typename RequestType, typename ResponseType>
bool MyHttpClient<RequestType, ResponseType>::Connect() 
{
    try 
    {
        boost::asio::ip::tcp::resolver resolver(ioContext);
        auto const results = resolver.resolve(host, port);
        boost::asio::connect(results.begin(), results.end());

        return true;
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Connection error: " << e.what() << std::endl;
        return false;
    }
}

template<typename RequestType, typename ResponseType>
bool MyHttpClient<RequestType, ResponseType>::SendRequest(RequestMethod method, const std::string& target, const RequestType& request_proto, ResponseType& response_proto) {
    if (!Connect()) 
    {
        return false;
    }

    try 
    {
        boost::beast::http::verb http_method = boost::beast::http::string_to_verb(GetRequestMethodString(method));
        boost::beast::http::request<boost::beast::http::string_body> req{ http_method, target, 11 };

        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(boost::beast::http::field::content_type, "application/x-protobuf");

        ProtobufHttpMessage<RequestType> protobufRequest(request_proto);
        req.body() = protobufRequest.SerializeToString();
        req.prepare_payload();

        boost::beast::http::write(boost::beast::tcp_stream(ioContext), req);

        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::dynamic_body> res;
        boost::beast::http::read(boost::beast::tcp_stream(ioContext), buffer, res);

        std::string response_body = boost::beast::buffers_to_string(res.body().data());

        ProtobufHttpMessage<ResponseType> protobufResponse(response_proto);
        if (!protobufResponse.ParseFromString(response_body)) 
        {
            std::cerr << "Failed to parse response body." << std::endl;
            return false;
        }

        return true;
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Request error: " << e.what() << std::endl;
        return false;
    }
}

template<typename RequestType, typename ResponseType>
std::string MyHttpClient<RequestType, ResponseType>::GetRequestMethodString(RequestMethod method) 
{
    switch (method) 
    {
    case RequestMethod::GET_USER:
        return "GET";
    case RequestMethod::CREATE_USER:
        return "POST";
    }
}
