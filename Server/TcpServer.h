#pragma once
#include "Common.h"
#include "Party/include/Party.h"
#include "Party/include/PartyManager.h"
#include "User/include/UserSession.h"
#include "DB/include/RedisClient.hpp"
#include "DB/include/MySQLManager.h"
#include "Util/HSThreadPool.hpp"

class TcpServer 
{
private:
    boost::asio::io_context&                    m_IoContext;
    std::thread                                 m_ContextThread;

    boost::asio::ip::tcp::acceptor              m_Acceptor;
    std::vector<std::shared_ptr<UserSession>>   m_Users;
    std::queue<std::shared_ptr<UserSession>>    m_NewUsers;
    std::mutex                                  m_UsersMutex;    
    std::mutex                                  m_NewUsersMutex;

    std::unique_ptr<PartyManager>               m_PartyManager;

    // Redis 관리 클래스
    std::unique_ptr<CRedisClient>               m_RedisClient;
    // MySQL 관리 클래스
    std::unique_ptr<MySQLManager>               m_MySQLConnector;
    // 동접자 수
    uint32_t                                    m_MaxUser = 5;

    //DB 작업은 Multi Thread로 진행
    HSThreadPool&                               m_ThreadPool;

public:
    TcpServer(boost::asio::io_context& io_context, int port, std::unique_ptr<CRedisClient> redisClient, std::unique_ptr<MySQLManager> mysqlManager, HSThreadPool& threadPool);
    ~TcpServer();
    bool Start(uint32_t maxUser);
    void Update();
    
    std::shared_ptr<UserSession> GetUserById(uint32_t userId);
    std::shared_ptr<UserSession> GetUserByUserId(const std::string& userId);

private:

    void EnqueueJob(std::function<void()>&& task);

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
    void HandleFriendRequest(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandleFriendAccept(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    void HandleFriendReject(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);


    std::future<void> ValidateRequest(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg);
    std::future<std::shared_ptr<UserEntity>> CheckUserExistence(const std::string& userId);
    std::future<void> CheckFriendRequestStatus(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> receiveUser);
    std::future<void> AddFriendRequest(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> receiveUser);
    void NotifyUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> receiveUser);

    std::future<void> ProcessFriendAccept(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender);
    void NotifyAcceptUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender);

    std::future<void> ProcessFriendReject(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender);
    void NotifyRejectUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender);

    uint32_t StringToUint32(const std::string& str);
};
