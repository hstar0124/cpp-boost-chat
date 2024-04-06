#pragma once
#include "Common.h"

class TcpSession : public std::enable_shared_from_this<TcpSession>
{
public:
    TcpSession(boost::asio::io_context& io_context);
    void Start();
    boost::asio::ip::tcp::socket& GetSocket();
    void Close();
    void Send(const char* message);

private:
    void AsyncRead();
    void OnRead(const boost::system::error_code& err, const size_t size);
    void AsyncWrite(const char* message);
    void OnWrite(const boost::system::error_code& err, const size_t transferred);

    boost::asio::ip::tcp::socket m_Socket;
    static const int m_RecvBufferSize = 1024;
    char m_RecvBuffer[m_RecvBufferSize];
    static const int m_SendBufferSize = 1024;
    char m_SendBuffer[m_SendBufferSize];
};
