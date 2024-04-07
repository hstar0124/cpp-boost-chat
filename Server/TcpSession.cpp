#include "TcpSession.h"

void TcpSession::Start()
{
    AsyncRead();
}

boost::asio::ip::tcp::socket& TcpSession::GetSocket()
{
    return m_Socket;
}

void TcpSession::Send(char* message)
{
    bool bWritingMessage = !m_QMessagesOutServer.Empty();
    m_QMessagesOutServer.PushBack(message);
    if(!bWritingMessage)
        AsyncWrite();
}

void TcpSession::Close()
{
    m_Socket.close();
}

bool TcpSession::IsConnected()
{
    return m_Socket.is_open();
}


void TcpSession::AsyncRead()
{
    boost::asio::async_read(m_Socket, boost::asio::buffer(m_RecvBuffer, 16),
        [this](const boost::system::error_code& err, const size_t size)
        {   
            this->OnRead(err, size);
        });
}

void TcpSession::OnRead(const boost::system::error_code& err, const size_t size)
{

    if (!err)
    {
        m_QMessagesInServer.PushBack(m_RecvBuffer);
        AsyncRead();
    }
    else
    {
        std::cout << "[TcpSession] Error " << err.message() << std::endl;
    }
}

void TcpSession::AsyncWrite()
{
    boost::asio::async_write(m_Socket, boost::asio::buffer(m_QMessagesOutServer.Front(), 16),
        [this](const boost::system::error_code& err, const size_t transferred)
        {
            this->OnWrite(err, transferred);
        });
}

void TcpSession::OnWrite(const boost::system::error_code& err, const size_t transferred)
{
    std::cout << "[TcpSession] OnWrite : " << m_QMessagesOutServer.Front() << std::endl;
    if (!err)
    {
        m_QMessagesOutServer.Pop();
        if (!m_QMessagesOutServer.Empty())
            AsyncWrite();
    }
    else
    {
        std::cout << "[TcpSession] Error " << err.message() << std::endl;
    }
}
