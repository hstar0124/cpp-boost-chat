#include "TcpServer.h"

TcpServer::TcpServer(boost::asio::io_context& io_context, int port)
    : m_Acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    m_IoContext(io_context)
{    
}


bool TcpServer::Start()
{
    try
    {
        // 클라이언트 연결 대기 및 연결
        WaitForClientConnection();
        m_ThreadContext = std::thread([this]() { m_IoContext.run(); });
    }
    catch (std::exception& e)
    {
        // 서버가 리스닝을 할 수 없는 경우
        std::cerr << "[SERVER] Exception: " << e.what() << "\n";
        return false;
    }

    std::cout << "[SERVER] Started!\n";
    return true;
}


void TcpServer::WaitForClientConnection()
{
    // 클라이언트 연결 정보 shared_ptr로 관리
    auto tcpSession = std::make_shared<TcpSession>(m_IoContext, m_QMessagesInServer);
    m_Acceptor.async_accept(tcpSession->GetSocket(),
        [this, tcpSession](const boost::system::error_code& err)
        {
            this->OnAccept(tcpSession, err);
        });
}

void TcpServer::Update(size_t nMaxMessages = -1, bool bWait = false)
{
    // 큐가 비어 있는 경우 대기
    if (bWait && m_QMessagesInServer.Empty()) {        
        m_QMessagesInServer.Wait();
    }

    // 큐에 메시지가 있는 경우 처리 시작
    if (!m_QMessagesInServer.Empty()) {
        size_t nMessageCount = 0;
        while (nMessageCount < nMaxMessages && !m_QMessagesInServer.Empty())
        {
            auto msg = m_QMessagesInServer.Pop();
            OnMessage(msg.remote, msg.payload);
            nMessageCount++;
        }
    }
}

void TcpServer::OnMessage(std::shared_ptr<TcpSession> session, std::shared_ptr<myPayload::Payload> msg)
{
    switch (msg->payloadtype())
    {
    case myPayload::PayloadType::SERVER_PING:
    {
        std::cout << "[SERVER] Server Ping\n";
        auto sentTimeMs = std::stoll(msg->content());
        auto sentTime = std::chrono::milliseconds(sentTimeMs);
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);
        std::cout << "Response time: " << elapsedTime.count() << "ms" << std::endl;
    }
    break;    
    case myPayload::PayloadType::ALL_MESSAGE:
    {
        std::cout << "[SERVER] Send Message for All Clients\n";
        msg->set_payloadtype(myPayload::PayloadType::SERVER_MESSAGE);
        SendAllClients(msg);
    }
    }
}

void TcpServer::SendAllClients(std::shared_ptr<myPayload::Payload> msg)
{
    bool hasDisconnectedClient = false; // 연결이 끊어진 클라이언트 여부를 추적

    for (auto session : m_VecTcpSessions)
    {
        if (session && session->IsConnected())
        {
            std::cout << "[SERVER] Send Message Process\n";
            session->Send(msg);
        }
        else
        {
            hasDisconnectedClient = true;
            std::cout << "session : " << session << "\n";
        }
    }

    if (hasDisconnectedClient)
    {
        m_VecTcpSessions.erase(std::remove_if(m_VecTcpSessions.begin(), m_VecTcpSessions.end(),
        [](const std::shared_ptr<TcpSession>& session) {
            return !session || !session->IsConnected();
        }), m_VecTcpSessions.end());
    }
}


void TcpServer::OnAccept(std::shared_ptr<TcpSession> tcpSession, const boost::system::error_code& err)
{
    if (!err)
    {
        // TODO : 클라이언트 유효성 체크        
        m_VecTcpSessions.push_back(std::move(tcpSession));
        m_VecTcpSessions.back()->Start();
        std::cout << "[SERVER] New Connection Approved!!\n";    
    }
    else
    {
        std::cout << "[SERVER] Error " << err.message() << std::endl;
    }

    WaitForClientConnection();
}

