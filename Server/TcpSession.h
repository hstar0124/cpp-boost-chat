#pragma once
#include "Common.h"
#include "ThreadSafeQueue.h"
#include "Message.h"

class TcpSession : public std::enable_shared_from_this<TcpSession>
{
public:

public:
    TcpSession(std::shared_ptr<boost::asio::io_context> io_context, ThreadSafeQueue<OwnedMessage>& qToServer)
        : m_IoContext(io_context), m_Socket(*io_context), m_QMessagesInServer(qToServer)
    {
    }
    void Start();
    void Close();
    void Send(const Message& msg);
    bool IsConnected();
    boost::asio::ip::tcp::socket& GetSocket();

private:
    void WriteHeader();
    void WriteBody();
    void ReadHeader();
    void ReadBody();
    void AddToIncomingMessageQueue();

private:
    boost::asio::ip::tcp::socket m_Socket;
    std::shared_ptr<boost::asio::io_context> m_IoContext;

    Message m_TemporaryMessage;

    ThreadSafeQueue<OwnedMessage>& m_QMessagesInServer;     // 서버로 보내는 메시지
    ThreadSafeQueue<Message> m_QMessagesOutServer;          // 클라이언트로 보내는 메시지

};
