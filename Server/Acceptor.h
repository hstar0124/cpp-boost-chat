#pragma once
#include "Common.h"
#include "TcpSession.h"
#include "TcpSessionManager.h"

class Acceptor
{
public:
    Acceptor(boost::asio::io_context& io_context, int port);
    void StartAccept();

private:
    void OnAccept(std::shared_ptr<TcpSession> tcpSession, const boost::system::error_code& err);

    boost::asio::io_context& m_IoContext;
    boost::asio::ip::tcp::acceptor m_Acceptor;
};
