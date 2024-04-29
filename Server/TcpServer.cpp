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

void TcpServer::OnMessage(std::shared_ptr<TcpSession> session, std::shared_ptr<myChatMessage::ChatMessage> msg)
{

    //std::cout << "MSG : " << msg->payloadtype() << "\n";
    //std::cout << "MSG : " << msg->receiver() << "\n";
    //std::cout << "MSG : " << msg->content() << "\n";

    switch (msg->messagetype())
    {
    case myChatMessage::ChatMessageType::SERVER_PING:
    {
        //std::cout << "[SERVER] Server Ping\n";
        auto sentTimeMs = std::stoll(msg->content());
        auto sentTime = std::chrono::milliseconds(sentTimeMs);
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);
        //std::cout << "Response time: " << elapsedTime.count() << "ms" << std::endl;
    }
    break;
    case myChatMessage::ChatMessageType::ALL_MESSAGE:
    {
        std::cout << "[SERVER] 모든 클라이언트에게 메시지 전송\n";
        msg->set_messagetype(myChatMessage::ChatMessageType::ALL_MESSAGE);
        SendAllClients(msg);
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_CREATE:
    {
        if (msg->content().empty()) {
            SendErrorMessage(session, "파티명이 비어 있습니다.");
            return;
        }
        if (FindPartyByName(msg->content())) 
        {
            SendErrorMessage(session, "해당 파티명은 이미 존재합니다.");
            return;
        }

        if (CreateParty(session, msg->content()))
            SendServerMessage(session, "파티 생성 성공");
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_DELETE:
    {
        if (msg->content().empty()) {
            SendErrorMessage(session, "파티명이 비어 있습니다.");
            return;
        }
        std::cout << "[SERVER] 파티 삭제\n";
        DeleteParty(session, msg->content());
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_JOIN:
    {
        if (msg->content().empty()) {
            SendErrorMessage(session, "파티명이 비어 있습니다.");
            return;
        }
        
        auto it = FindPartyByName(msg->content());

        std::cout << "[SERVER] 파티 가입\n";
        // 파티가 존재하는 경우 파티 내 멤버 추가
        if (it == nullptr || it->HasMember(session))
        {
            SendErrorMessage(session, "파티가 없거나, 이미 가입된 멤버입니다.");
            return;
        }

        it->AddMember(session);
        std::cout << "[SERVER] Add Member in Party : " << it->GetName() << " : " << session->GetID() << std::endl;
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_LEAVE:
    {
        if (msg->content().empty()) {
            SendErrorMessage(session, "파티명이 비어 있습니다.");
            return;
        }
        auto it = FindPartyByName(msg->content());

        std::cout << "[SERVER] 파티 탈퇴\n";
        if (it == nullptr || !it->HasMember(session))
        {
            SendErrorMessage(session, "파티가 없거나, 가입하지 않은 파티입니다.");
            return;
        }

        it->RemoveMember(session);
        std::cout << "[SERVER] Remove Member in Party : " << it->GetName() << " : " << session->GetID() << std::endl;

    }
    break;
    case myChatMessage::ChatMessageType::PARTY_MESSAGE:
    {
        if (msg->content().empty()) {
            SendErrorMessage(session, "파티 메시지의 내용이 비어 있습니다.");
            return;
        }
        std::cout << "[SERVER] 파티 메시지 전송\n";
        SendPartyMessage(session, msg);
    }
    break;
    case myChatMessage::ChatMessageType::WHISPER_MESSAGE:
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

    std::cout << "PP : " << m_VecParties.Count() << "\n";
}

void TcpServer::SendAllClients(std::shared_ptr<myChatMessage::ChatMessage> msg)
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

    // 끊어진 연결은 Vector에서 제거
    if (hasDisconnectedClient)
    {
        m_VecTcpSessions.erase(std::remove_if(m_VecTcpSessions.begin(), m_VecTcpSessions.end(),
        [](const std::shared_ptr<TcpSession>& session) {
            return !session || !session->IsConnected();
        }), m_VecTcpSessions.end());
    }
}


void TcpServer::SendWhisperMessage(std::shared_ptr<TcpSession>& senderSession, const std::string& receiver, std::shared_ptr<myChatMessage::ChatMessage> msg)
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

void TcpServer::SendPartyMessage(std::shared_ptr<TcpSession>& senderSession, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    for (auto it = m_VecParties.Begin(); it != m_VecParties.End(); ++it)
    {
        if ((*it)->HasMember(senderSession)) 
        {
            std::cout << (*it)->GetName() << "\n";
            (*it)->Send(msg); 
            return;
        }
    }

    SendErrorMessage(senderSession, "[SERVER] 가입된 파티가 없습니다.");
}


void TcpServer::SendErrorMessage(std::shared_ptr<TcpSession>& session, const std::string& errorMessage)
{
    std::cout << "[SERVER] " << session->GetID() << " : " << errorMessage << "\n";
    // 에러 메시지 생성
    auto errMsg = std::make_shared<myChatMessage::ChatMessage>();
    errMsg->set_messagetype(myChatMessage::ChatMessageType::ERROR_MESSAGE);
    errMsg->set_content(errorMessage);

    // 해당 세션에게 에러 메시지 전송
    session->Send(errMsg);
}

void TcpServer::SendServerMessage(std::shared_ptr<TcpSession>& session, const std::string& serverMessage)
{
    std::cout << "[SERVER] " << session->GetID() << " : " << serverMessage << "\n";
    // 에러 메시지 생성
    auto serverMsg = std::make_shared<myChatMessage::ChatMessage>();
    serverMsg->set_messagetype(myChatMessage::ChatMessageType::SERVER_MESSAGE);
    serverMsg->set_content(serverMessage);

    // 해당 세션에게 에러 메시지 전송
    session->Send(serverMsg);
}

std::shared_ptr<Party> TcpServer::CreateParty(std::shared_ptr<TcpSession> creatorSession, const std::string& partyName)
{
    auto party = std::make_shared<Party>(m_PartyIdCounter++, creatorSession->GetID(), partyName);  // 파티 생성
    party->AddMember(creatorSession);                                                   // 파티 생성자를 파티 멤버로 추가
    m_VecParties.PushBack(party);                                                       // 파티를 파티 리스트에 추가
    return party;
}

void TcpServer::DeleteParty(std::shared_ptr<TcpSession> session, const std::string& partyName)
{
    // 파티 이름에 해당하는 파티를 찾음|
    auto it = FindPartyByName(partyName);

    // 파티가 존재하고 파티 창설자인 경우
    if (it != nullptr && it->GetPartyCreator() == session->GetID())
    {
        // 파티를 파티 컨테이너에서 삭제
        m_VecParties.Erase(it);
        std::cout << "[SERVER] Deleted party: " << partyName << std::endl;
    }
    else
    {
        // 해당 이름을 가진 파티가 없거나 창시자가 아닌 경우
        std::cout << "[SERVER] Fail Deleted Party : " << partyName << std::endl;
        SendErrorMessage(session, "[SERVER] 파티 삭제를 실패하였습니다.");
    }
}   

std::shared_ptr<Party> TcpServer::FindPartyByName(const std::string& partyName)
{
    // 파티 이름에 해당하는 파티를 찾음
    auto it = std::find_if(m_VecParties.Begin(), m_VecParties.End(), 
        [&partyName](std::shared_ptr<Party>& party) 
        {
            return party->GetName() == partyName;
        });

    // 파티가 존재하는 경우 해당 파티를 반환
    if (it != m_VecParties.End())
        return *it;
    else
        return nullptr;
}
