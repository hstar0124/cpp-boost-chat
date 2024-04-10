#pragma once
#include "Common.h"
#include "TcpSession.h"
#include "ThreadSafeQueue.h"

class TcpServer
{
public:
    TcpServer(boost::asio::io_context& io_context, int port);
    bool Start();
    void WaitForClientConnection();
    void Update(size_t nMaxMessages, bool bWait);
    void OnMessage(std::shared_ptr<TcpSession> session, Message& msg);
    void SendAllClients(const Message& msg, std::shared_ptr<TcpSession> session);

private:
    void OnAccept(std::shared_ptr<TcpSession> tcpSession, const boost::system::error_code& err);

    boost::asio::io_context& m_IoContext;
    std::thread m_ThreadContext;
    boost::asio::ip::tcp::acceptor m_Acceptor;

    ThreadSafeQueue<OwnedMessage> m_QMessagesInServer;
    std::vector<std::shared_ptr<TcpSession>> m_VecTcpSessions;
};
