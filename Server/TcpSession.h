#pragma once
#include "Common.h"
#include "ThreadSafeQueue.h"

class TcpSession : public std::enable_shared_from_this<TcpSession>
{
public:
    TcpSession(boost::asio::io_context& io_context, ThreadSafeQueue<char*>& qToServer)
        : m_Socket(io_context), m_QMessagesInServer(qToServer)
    {
        memset(m_RecvBuffer, 0, m_RecvBufferSize);
        memset(m_SendBuffer, 0, m_SendBufferSize);
    }
    void Start();
    void Close();
    void Send(char* message);
    bool IsConnected();
    boost::asio::ip::tcp::socket& GetSocket();

private:
    void AsyncRead();
    void OnRead(const boost::system::error_code& err, const size_t size);
    void AsyncWrite();
    void OnWrite(const boost::system::error_code& err, const size_t transferred);

    boost::asio::ip::tcp::socket m_Socket;
    ThreadSafeQueue<char*>& m_QMessagesInServer;    // 서버로 보내는 메시지
    ThreadSafeQueue<char*> m_QMessagesOutServer;    // 클라이언트로 보내는 메시지

    static const int m_RecvBufferSize = 1024;
    char m_RecvBuffer[m_RecvBufferSize];
    static const int m_SendBufferSize = 1024;
    char m_SendBuffer[m_SendBufferSize];
};
