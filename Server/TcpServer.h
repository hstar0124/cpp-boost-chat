#pragma once
#include "Common.h"
#include "Party.h"
#include "ThreadSafeQueue.h"
#include "ThreadSafeVector.h""
#include "PartyManager.h"
#include "User.h"

class TcpServer 
{
private:
    boost::asio::io_context& m_IoContext;
    std::thread m_ContextThread;
    boost::asio::ip::tcp::acceptor m_Acceptor;

    std::vector<std::shared_ptr<User>> m_Users;
    std::queue<std::shared_ptr<User>> m_NewUsers;
    std::mutex m_UsersMutex;
    std::mutex m_NewUsersMutex;

    uint32_t m_IdCounter = 10'000;

public:
    TcpServer(boost::asio::io_context& io_context, int port);
    bool Start();
    void Update();
    
    std::shared_ptr<User> GetUserById(uint32_t sessionId);

private:
    void WaitForClientConnection();
    void UpdateUsers();

    void OnAccept(std::shared_ptr<User> user, const boost::system::error_code& err);
    void OnMessage(std::shared_ptr<User> user, std::shared_ptr<myChatMessage::ChatMessage> msg);

    void SendAllUsers(std::shared_ptr<myChatMessage::ChatMessage> msg);
    void SendWhisperMessage(std::shared_ptr<User>& sender, const std::string& receiver, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void SendPartyMessage(std::shared_ptr<Party>& party, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void SendErrorMessage(std::shared_ptr<User>& user, const std::string& errorMessage);
    void SendServerMessage(std::shared_ptr<User>& user, const std::string& serverMessage);

    
};
