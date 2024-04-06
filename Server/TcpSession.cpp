#include "TcpSession.h"
#include "TcpSessionManager.h"

TcpSession::TcpSession(boost::asio::io_context& io_context)
    : m_Socket(io_context)
{
    memset(m_RecvBuffer, 0, m_RecvBufferSize);
    memset(m_SendBuffer, 0, m_SendBufferSize);
}

void TcpSession::Start()
{
    TcpSessionManager::getInstance().Connection(shared_from_this());
    AsyncRead();
}

boost::asio::ip::tcp::socket& TcpSession::GetSocket()
{
    return m_Socket;
}

void TcpSession::Send(const char* message)
{
    AsyncWrite(message);
}

void TcpSession::Close()
{
    m_Socket.close();
}

void TcpSession::AsyncRead()
{
    m_Socket.async_read_some(boost::asio::buffer(m_RecvBuffer, m_RecvBufferSize),
        [this](const boost::system::error_code& err, const size_t size)
        {
            this->OnRead(err, size);
        });
}

void TcpSession::OnRead(const boost::system::error_code& err, const size_t size)
{
    std::cout << "[TcpSession] OnRead : " << m_RecvBuffer << std::endl;

    if (!err)
    {
        AsyncRead();
        TcpSessionManager::getInstance().AllMessage(m_RecvBuffer);
    }
    else
    {
        std::cout << "Error " << err.message() << std::endl;
        TcpSessionManager::getInstance().Disconnection(shared_from_this());
    }
}

void TcpSession::AsyncWrite(const char* message)
{
    memcpy(m_SendBuffer, message, m_SendBufferSize);

    boost::asio::async_write(m_Socket, boost::asio::buffer(m_SendBuffer, m_SendBufferSize),
        [this](const boost::system::error_code& err, const size_t transferred)
        {
            this->OnWrite(err, transferred);
        });
}

void TcpSession::OnWrite(const boost::system::error_code& err, const size_t transferred)
{
    std::cout << "[TcpSession] OnWrite : " << m_SendBuffer << std::endl;
    if (!err)
    {
        // Success
    }
    else
    {
        std::cout << "Error " << err.message() << std::endl;
        TcpSessionManager::getInstance().Disconnection(shared_from_this());
    }
}
