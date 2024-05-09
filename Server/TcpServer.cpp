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
    if (bWait && m_QMessagesInServer.Empty()) 
    {        
        m_QMessagesInServer.Wait();
    }

    // 큐에 메시지가 있는 경우 처리 시작
    if (!m_QMessagesInServer.Empty()) 
    {
        size_t nMessageCount = 0;
        while (nMessageCount < nMaxMessages && !m_QMessagesInServer.Empty())
        {
            auto msg = m_QMessagesInServer.Pop();
            OnMessage(msg.remote, msg.chatMessage);
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
        std::cout << "[SERVER] Send message to all clients\n";
        msg->set_messagetype(myChatMessage::ChatMessageType::ALL_MESSAGE);
        SendAllClients(msg);
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_CREATE:
    {
        if (msg->content().empty()) 
        {
            SendErrorMessage(session, "The party name is empty.");
            return;
        }

        std::cout << "[SERVER] Create party\n";

        if (PartyManager::getInstance().HasParty(msg->content()))
        {
            SendErrorMessage(session, "The party name already exists.");
            return;
        }

        if (PartyManager::getInstance().CreateParty(session, msg->content()))
        {
            SendServerMessage(session, "Party creation successful");
            return;
        }

    }
    break;
    case myChatMessage::ChatMessageType::PARTY_DELETE:
    {
        if (msg->content().empty())
        {
            SendErrorMessage(session, "The party name is empty.");
            return;
        }
        std::cout << "[SERVER] Delete party\n";

        if (PartyManager::getInstance().DeleteParty(session, msg->content()))
        {
            SendServerMessage(session, "Party Delete successful");
            return;
        }
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_JOIN:
    {
        if (msg->content().empty()) 
        {
            SendErrorMessage(session, "The party name is empty.");
            return;
        }
        

        if (!PartyManager::getInstance().HasParty(msg->content()))
        {
            SendErrorMessage(session, "The party does not exist.");
            return;
        }

        auto party = PartyManager::getInstance().FindPartyByName(msg->content());
        if (party->HasMember(session->GetID()))
        {
            SendErrorMessage(session, "Already joined.");
            return;
        }

        party->AddMember(session->GetID());
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_LEAVE:
    {
        if (msg->content().empty()) 
        {
            SendErrorMessage(session, "The party name is empty.");
            return;
        }
        
        if (!PartyManager::getInstance().HasParty(msg->content()))
        {
            SendErrorMessage(session, "The party does not exist.");
            return;
        }

        auto party = PartyManager::getInstance().FindPartyByName(msg->content());
        if (!party->HasMember(session->GetID()))
        {
            SendErrorMessage(session, "It's a party not joined.");
            return;
        }

        if (party->GetPartyCreator() == session->GetID())
        {
            SendErrorMessage(session, "Sorry, as the party leader, you cannot leave the party. Deletion is the only option.");
            return;
        }

        party->RemoveMember(session->GetID());
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_MESSAGE:
    {
        if (msg->content().empty()) 
        {
            SendErrorMessage(session, "The content of the party message is empty.");
            return;
        }

        auto party = PartyManager::getInstance().FindPartyBySessionId(session->GetID());
        if (!party)
        {
            SendErrorMessage(session, "It's a party not joined.");
            return;
        }

        SendPartyMessage(party, msg);
    }
    break;
    case myChatMessage::ChatMessageType::WHISPER_MESSAGE:
    {
        if (msg->receiver().empty() || msg->content().empty()) 
        {
            SendErrorMessage(session, "Recipient or content is empty.");
            return;
        }
        SendWhisperMessage(session, msg->receiver(), msg);
    }
    break;
    default:
        SendErrorMessage(session, "Unknown message type received.");
        break;
    }    
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
        [](const std::shared_ptr<TcpSession>& session) 
        {
            return !session || !session->IsConnected();
        }), m_VecTcpSessions.end());
    }
}


void TcpServer::SendWhisperMessage(std::shared_ptr<TcpSession>& senderSession, const std::string& receiver, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (std::to_string(senderSession->GetID()) == receiver)
    {
        SendErrorMessage(senderSession, "You cannot whisper to yourself.");
        return;
    }

    for (auto& session : m_VecTcpSessions) 
    {
        if (std::to_string(session->GetID()) == receiver && session->IsConnected()) 
        {
            msg->set_receiver(receiver);
            session->Send(msg);
            return;
        }
    }
    SendErrorMessage(senderSession, "Receiver not found.");
}

void TcpServer::SendPartyMessage(std::shared_ptr<Party>& party, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (party != nullptr)
    {
        auto partyMembers = party->GetMembers();
        for (auto member : partyMembers)
        {
            auto session = GetSessionById(member);
            if (session != nullptr)
            {
                session->Send(msg);
            }
        }
    }

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

std::shared_ptr<TcpSession> TcpServer::GetSessionById(uint32_t sessionId)
{
    for (auto session : m_VecTcpSessions)
    {
        if (session->GetID() == sessionId)
        {
            // 해당 세션 ID를 가진 세션을 찾았을 때
            return session;
        }
    }

    // 해당 세션 ID를 가진 세션이 없을 때
    return nullptr;
}
