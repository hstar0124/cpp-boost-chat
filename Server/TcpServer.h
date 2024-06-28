#pragma once
#include "Common.h"
#include "Party.h"
#include "PartyManager.h"
#include "UserSession.h"
#include "RedisClient.hpp"
#include "MySQLConnector.h"

class TcpServer 
{
private:
    boost::asio::io_context&            m_IoContext;
    std::thread                         m_ContextThread;
    boost::asio::ip::tcp::acceptor      m_Acceptor;

    std::vector<std::shared_ptr<UserSession>>  m_Users;
    std::mutex                          m_UsersMutex;
    
    std::queue<std::shared_ptr<UserSession>>   m_NewUsers;
    std::mutex                          m_NewUsersMutex;

    std::unique_ptr<PartyManager>       m_PartyManager;
    std::unique_ptr<CRedisClient>       m_RedisClient;
    std::unique_ptr<MySQLConnector>     m_MySQLConnector;

    uint32_t                            m_MaxUser = 5;

public:
    TcpServer(boost::asio::io_context& io_context, int port);
    ~TcpServer();
    bool Start(uint32_t maxUser);
    void Update();
    
    std::shared_ptr<UserSession> GetUserById(uint32_t userId);
    std::shared_ptr<UserSession> GetUserByUserId(const std::string& userId);

private:

    void WaitForClientConnection();
    void UpdateUsers();
    bool VerifyUser(std::shared_ptr<UserSession>& user, const std::string& sessionId);

    void OnAccept(std::shared_ptr<UserSession> user, const boost::system::error_code& err);
    void OnMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);


    void SendAllUsers(std::shared_ptr<myChatMessage::ChatMessage> msg);
    void SendWhisperMessage(std::shared_ptr<UserSession>& sender, const std::string& receiver, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void SendPartyMessage(std::shared_ptr<Party>& party, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void SendErrorMessage(std::shared_ptr<UserSession>& user, const std::string& errorMessage);
    void SendServerMessage(std::shared_ptr<UserSession>& user, const std::string& serverMessage);
    void SendLoginMessage(std::shared_ptr<UserSession>& user);

    void HandleServerPing(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandleAllMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandlePartyCreate(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandlePartyDelete(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandlePartyJoin(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandlePartyLeave(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandlePartyMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandleWhisperMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandleFriendRequestMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandleFriendAcceptMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandleFriendRejectMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);


    uint32_t StringToUint32(const std::string& str);
};
