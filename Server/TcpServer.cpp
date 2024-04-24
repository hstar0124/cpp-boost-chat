#include "TcpServer.h"

TcpServer::TcpServer(boost::asio::io_context& io_context, int port)
    : m_Acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    , m_IoContext(io_context)
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

    //std::cout << "MSG : " << msg->payloadtype() << "\n";
    //std::cout << "MSG : " << msg->receiver() << "\n";
    //std::cout << "MSG : " << msg->content() << "\n";

    switch (msg->payloadtype())
    {
    case myPayload::PayloadType::SERVER_PING:
    {
        //std::cout << "[SERVER] Server Ping\n";
        auto sentTimeMs = std::stoll(msg->content());
        auto sentTime = std::chrono::milliseconds(sentTimeMs);
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);
        //std::cout << "Response time: " << elapsedTime.count() << "ms" << std::endl;
    }
    break;
    case myPayload::PayloadType::ALL_MESSAGE:
    {
        std::cout << "[SERVER] 모든 클라이언트에게 메시지 전송\n";
        msg->set_payloadtype(myPayload::PayloadType::ALL_MESSAGE);
        SendAllClients(msg);
    }
    break;
    case myPayload::PayloadType::PARTY_MESSAGE:
    {
        if (msg->content().empty()) {
            SendErrorMessage(session, "파티 메시지의 내용이 비어 있습니다.");
            return;
        }
        std::cout << "[SERVER] 파티 메시지 전송\n";
        //SendPartyMessage(session, msg);
    }
    break;
    case myPayload::PayloadType::WHISPER_MESSAGE:
    {
        if (msg->receiver().empty() || msg->content().empty()) {
            SendErrorMessage(session, "[SERVER] 수신자 또는 내용이 비어 있습니다.");
            return;
        }
        SendWhisperMessage(session, msg->receiver(), msg);
    }
    break;
    default:
        SendErrorMessage(session, "[SERVER] 알 수 없는 메시지 유형이 수신되었습니다.");
        break;
    }
}

void TcpServer::SendAllClients(std::shared_ptr<myPayload::Payload> msg)
{
    bool hasDisconnectedClient = false; // 연결이 끊어진 클라이언트 여부를 추적

    for (auto session : m_VecTcpSessions)
    {
        if (session && session->IsConnected())
        {
            session->Send(msg);
        }
        else
        {
            hasDisconnectedClient = true;
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


void TcpServer::SendWhisperMessage(std::shared_ptr<TcpSession>& senderSession, const std::string& receiver, std::shared_ptr<myPayload::Payload> msg)
{
    if (std::to_string(senderSession->GetID()) == receiver)
    {
        SendErrorMessage(senderSession, "[SERVER] 자신에게 귓속말 할 수 없습니다.");
        return;
    }

    for (auto& session : m_VecTcpSessions) {
        if (std::to_string(session->GetID()) == receiver && session->IsConnected()) 
        {
            msg->set_receiver(receiver);
            session->Send(msg);
            return;
        }
    }
    SendErrorMessage(senderSession, "[SERVER] 수신자를 찾을 수 없습니다.");
}

void TcpServer::SendErrorMessage(std::shared_ptr<TcpSession>& session, const std::string& errorMessage)
{
    std::cout << "[" << session->GetID() << "] " << errorMessage << "\n";
    // 에러 메시지 생성
    auto errorPayload = std::make_shared<myPayload::Payload>();
    errorPayload->set_payloadtype(myPayload::PayloadType::ERROR_MESSAGE);
    errorPayload->set_content(errorMessage);

    // 해당 세션에게 에러 메시지 전송
    session->Send(errorPayload);
}


void TcpServer::OnAccept(std::shared_ptr<TcpSession> tcpSession, const boost::system::error_code& err)
{
    if (!err)
    {
        // TODO : 클라이언트 유효성 체크        
        m_VecTcpSessions.push_back(std::move(tcpSession));
        m_VecTcpSessions.back()->Start(m_IdCounter++);
        std::cout << "[SERVER] " << m_VecTcpSessions.back()->GetID() << " Connection Approved\n";
    }
    else
    {
        std::cout << "[SERVER] Error " << err.message() << std::endl;
    }

    WaitForClientConnection();
}

