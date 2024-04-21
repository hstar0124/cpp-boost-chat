#pragma once
#include "Common.h"
#include "ThreadSafeQueue.h"
#include "Message.h"
#include "PacketConverter.hpp"
#include "Payload.pb.h"

class TcpSession : public std::enable_shared_from_this<TcpSession>
{
public:
    TcpSession(boost::asio::io_context& io_context, ThreadSafeQueue<OwnedMessage>& qToServer)
        : m_IoContext(io_context)
        , m_Socket(io_context)
        , m_QMessagesInServer(qToServer)
        , m_PingTimer(io_context, std::chrono::seconds(5))
        , m_IsActive(true)
    {
    }

    void Start();
    void StartPingTimer();
    void Close();
    void Send(std::shared_ptr<myPayload::Payload> msg);
    void SendPing();
    bool IsConnected();
    boost::asio::ip::tcp::socket& GetSocket();

private:
    void AsyncWrite();
    void OnWrite(const boost::system::error_code& err, const size_t size);
    void ReadHeader();
    void ReadBody(size_t body_size);
    void AddToIncomingMessageQueue(std::shared_ptr<myPayload::Payload> payload);

private:
    boost::asio::ip::tcp::socket m_Socket;
    boost::asio::io_context& m_IoContext;

    std::vector<uint8_t> m_Writebuf;
    std::vector<uint8_t> m_Readbuf;
    
    ThreadSafeQueue<OwnedMessage>& m_QMessagesInServer;     
    ThreadSafeQueue<std::shared_ptr<myPayload::Payload>> m_QMessageOutServer;

    boost::asio::steady_timer m_PingTimer;
    bool m_IsActive;
};
