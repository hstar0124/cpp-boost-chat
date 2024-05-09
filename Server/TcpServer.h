#pragma once
#include "Common.h"
#include "Party.h"
#include "TcpSession.h"
#include "ThreadSafeQueue.h"
#include "ThreadSafeVector.h""
#include "PartyManager.h"

class TcpServer
{
private:
    boost::asio::io_context& m_IoContext;
    std::thread m_ThreadContext;
    boost::asio::ip::tcp::acceptor m_Acceptor;

    ThreadSafeQueue<OwnedMessage> m_QMessagesInServer;

    std::vector<std::shared_ptr<TcpSession>> m_VecTcpSessions;
    uint32_t m_IdCounter = 10'000;

public:
    TcpServer(boost::asio::io_context& io_context, int port);
    bool Start();
    void Update(size_t nMaxMessages, bool bWait);
    
    std::shared_ptr<TcpSession> GetSessionById(uint32_t sessionId);

private:
    void WaitForClientConnection();

    void OnAccept(std::shared_ptr<TcpSession> tcpSession, const boost::system::error_code& err);
    void OnMessage(std::shared_ptr<TcpSession> session, std::shared_ptr<myChatMessage::ChatMessage> msg);

    void SendAllClients(std::shared_ptr<myChatMessage::ChatMessage> msg);
    void SendWhisperMessage(std::shared_ptr<TcpSession>& senderSession, const std::string& receiver, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void SendPartyMessage(std::shared_ptr<Party>& party, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void SendErrorMessage(std::shared_ptr<TcpSession>& session, const std::string& errorMessage);
    void SendServerMessage(std::shared_ptr<TcpSession>& session, const std::string& serverMessage);

    
};
