#include "Acceptor.h"

Acceptor::Acceptor(boost::asio::io_context& io_context, int port)
    : m_Acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    m_IoContext(io_context)
{    
}

void Acceptor::StartAccept()
{
    auto tcpSession = std::make_shared<TcpSession>(m_IoContext);
    m_Acceptor.async_accept(tcpSession->GetSocket(),
        [this, tcpSession](const boost::system::error_code& err)
        {
            this->OnAccept(tcpSession, err);
        });
}

void Acceptor::OnAccept(std::shared_ptr<TcpSession> tcpSession, const boost::system::error_code& err)
{
    if (!err)
    {
        std::cout << "[TcpServer] Accept" << std::endl;
        tcpSession->Start();
    }
    else
    {
        std::cout << "Error " << err.message() << std::endl;
    }

    StartAccept();
}
