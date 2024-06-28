#pragma once
#include "Common.h"
#include "MyMessage.pb.h"
#include <functional>

class ChatClient
{
private:
    boost::asio::ip::tcp::endpoint	m_Endpoint;
    boost::asio::io_context&        m_IoContext;
    boost::asio::ip::tcp::socket	m_Socket;
    std::string						m_Message;

    std::atomic<bool>               m_IsVerified{ false };
    std::function<void()>           m_VerificationCallback;

    std::vector<uint8_t>			m_Readbuf;
    std::vector<uint8_t>			m_Writebuf;

public:
    ChatClient(boost::asio::io_context& io_context);
    ~ChatClient();

    void Connect(const std::string& host, int port);
    void Send(const std::string& userInput);
    
    bool GetVerified(); 
    void SetVerificationCallback(const std::function<void()>& callback);

    bool SendSessionId(const std::string& sessionId);

private:
    void OnConnect(const boost::system::error_code& err);
    void OnMessage(std::shared_ptr<myChatMessage::ChatMessage>& message);
    void OnWrite(const boost::system::error_code& err, const size_t size);

    void AsyncWrite(std::shared_ptr<myChatMessage::ChatMessage> message);
    void ReadHeader();
    void ReadBody(size_t bodySize);

    bool SendPong();
    bool SendFriendRequest(const std::string& friendId);
    bool SendFriendAccept(const std::string& friendId);

    void SetVerified(bool isVerified);
    

    bool IsWhisperMessage(const std::string& userInput);
    bool IsPartyMessage(const std::string& userInput);
    bool IsFriendRequestMessage(const std::string& userInput);
    bool IsFriendAcceptMessage(const std::string& userInput);

    bool CreateWhisperMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput);
    bool CreatePartyMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput);
    bool CreateNormalMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput);
    bool CreateFriendRequestMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput);
    bool CreateFriendAcceptMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput);

    std::pair<std::string, std::string> ExtractOptionAndPartyName(const std::string& userInput);
    myChatMessage::ChatMessageType GetPartyMessageType(const std::string& option);

    std::function<bool(std::shared_ptr<myChatMessage::ChatMessage>&, const std::string&)> GetCreateMessageStrategy(const std::string& userInput);
    void HandleServerPing(const std::shared_ptr<myChatMessage::ChatMessage>& message);
};
