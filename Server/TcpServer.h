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
    boost::asio::io_context&                    m_IoContext;        // Boost ASIO의 I/O 컨텍스트
    std::thread                                 m_ContextThread;    // 컨텍스트 실행을 위한 스레드

    boost::asio::ip::tcp::acceptor              m_Acceptor;         // TCP 연결을 수락하는 객체
    
    std::vector<std::shared_ptr<UserSession>>   m_Users;            // 연결된 사용자 세션들
    std::queue<std::shared_ptr<UserSession>>    m_NewUsers;         // 새로운 사용자 대기열
    
    std::mutex                                  m_UsersMutex;       // 사용자 세션 접근을 위한 뮤텍스
    std::mutex                                  m_NewUsersMutex;    // 새로운 사용자 대기열 접근을 위한 뮤텍스

    std::unique_ptr<PartyManager>               m_PartyManager;     // 파티 관리자 객체
    std::unique_ptr<CRedisClient>               m_RedisClient;      // Redis 클라이언트 객체    
    std::unique_ptr<MySQLManager>               m_MySQLConnector;   // MySQL 관리자 객체
    
    uint32_t                                    m_MaxUser = 5;      // 최대 사용자 수
    
    HSThreadPool&                               m_ThreadPool;       // DB 작업을 처리하는 스레드 풀 객체


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

    void RemoveUserSessions();
    void RemoveNewUserSessions();

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
