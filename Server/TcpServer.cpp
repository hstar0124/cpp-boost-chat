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
        m_ContextThread = std::thread([this]() { m_IoContext.run(); });
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
    auto user = std::make_shared<User>(m_IoContext);
    m_Acceptor.async_accept(user->GetSocket(),
        [this, user](const boost::system::error_code& err)
        {
            this->OnAccept(user, err);
        });
}

void TcpServer::OnAccept(std::shared_ptr<User> user, const boost::system::error_code& err)
{
    if (!err)
    {   
        std::lock_guard<std::mutex> lock(m_NewUsersMutex);
        m_NewUsers.push(std::move(user));
        //m_Users.push_back(std::move(user));
        //m_Users.back()->Start(m_IdCounter++);
        //std::cout << "[SERVER] " << m_Users.back()->GetID() << " Connection Approved\n";
    }
    else
    {
        std::cout << "[SERVER] Error " << err.message() << std::endl;
    }

    WaitForClientConnection();
}


void TcpServer::Update()
{
    while (1)
    {
        {
            std::lock_guard<std::mutex> lock(m_NewUsersMutex);
            while (!m_NewUsers.empty()) 
            {
                auto user = m_NewUsers.front();
                m_NewUsers.pop();
                {
                    std::lock_guard<std::mutex> users_lock(m_UsersMutex);
                    m_Users.push_back(user);
                    user->Start(m_IdCounter++);
                }
                std::cout << "[SERVER] " << user->GetID() << " Connection Approved\n";
            }
        }

        if (m_Users.empty())
        {
            continue;
        }

        //User 벡터 순회하면서 메시지 처리
        for (auto u : m_Users)
        {
            if (u == nullptr || !u->IsConnected())
            {
                continue;
            }

            while (true)
            {
                auto msg = u->GetMessageInUserQueue();
                if (!msg)
                {
                    break;
                }

                OnMessage(u, msg);
            }
        } 
    }
}

void TcpServer::OnMessage(std::shared_ptr<User> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    switch (msg->messagetype())
    {
    case myChatMessage::ChatMessageType::SERVER_PING:
    {
        auto sentTimeMs = std::stoll(msg->content());
        auto sentTime = std::chrono::milliseconds(sentTimeMs);
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);
    }
    break;
    case myChatMessage::ChatMessageType::ALL_MESSAGE:
    {
        std::cout << "[SERVER] Send message to all clients\n";
        msg->set_messagetype(myChatMessage::ChatMessageType::ALL_MESSAGE);
        SendAllUsers(msg);
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_CREATE:
    {
        if (msg->content().empty())
        {
            SendErrorMessage(user, "The party name is empty.");
            return;
        }

        std::cout << "[SERVER] Create party\n";

        if (PartyManager::getInstance().HasParty(msg->content()))
        {
            SendErrorMessage(user, "The party name already exists.");
            return;
        }

        if (PartyManager::getInstance().CreateParty(user, msg->content()))
        {
            SendServerMessage(user, "Party creation successful");
            return;
        }

    }
    break;
    case myChatMessage::ChatMessageType::PARTY_DELETE:
    {
        if (msg->content().empty())
        {
            SendErrorMessage(user, "The party name is empty.");
            return;
        }
        std::cout << "[SERVER] Delete party\n";

        if (PartyManager::getInstance().DeleteParty(user, msg->content()))
        {
            SendServerMessage(user, "Party Delete successful");
            return;
        }
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_JOIN:
    {
        if (msg->content().empty())
        {
            SendErrorMessage(user, "The party name is empty.");
            return;
        }


        if (!PartyManager::getInstance().HasParty(msg->content()))
        {
            SendErrorMessage(user, "The party does not exist.");
            return;
        }

        auto party = PartyManager::getInstance().FindPartyByName(msg->content());
        if (party->HasMember(user->GetID()))
        {
            SendErrorMessage(user, "Already joined.");
            return;
        }

        party->AddMember(user->GetID());
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_LEAVE:
    {
        if (msg->content().empty())
        {
            SendErrorMessage(user, "The party name is empty.");
            return;
        }

        if (!PartyManager::getInstance().HasParty(msg->content()))
        {
            SendErrorMessage(user, "The party does not exist.");
            return;
        }

        auto party = PartyManager::getInstance().FindPartyByName(msg->content());
        if (!party->HasMember(user->GetID()))
        {
            SendErrorMessage(user, "It's a party not joined.");
            return;
        }

        if (party->GetPartyCreator() == user->GetID())
        {
            SendErrorMessage(user, "Sorry, as the party leader, you cannot leave the party. Deletion is the only option.");
            return;
        }

        party->RemoveMember(user->GetID());
    }
    break;
    case myChatMessage::ChatMessageType::PARTY_MESSAGE:
    {
        if (msg->content().empty())
        {
            SendErrorMessage(user, "The content of the party message is empty.");
            return;
        }

        auto party = PartyManager::getInstance().FindPartyBySessionId(user->GetID());
        if (!party)
        {
            SendErrorMessage(user, "It's a party not joined.");
            return;
        }

        SendPartyMessage(party, msg);
    }
    break;
    case myChatMessage::ChatMessageType::WHISPER_MESSAGE:
    {
        if (msg->receiver().empty() || msg->content().empty())
        {
            SendErrorMessage(user, "Recipient or content is empty.");
            return;
        }
        SendWhisperMessage(user, msg->receiver(), msg);
    }
    break;
    default:
        SendErrorMessage(user, "Unknown message type received.");
        break;
    }
}



void TcpServer::SendAllUsers(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    bool hasDisconnectedClient = false; // 연결이 끊어진 클라이언트 여부를 추적

    for (auto user : m_Users)
    {
        if (user && user->IsConnected())
        {
            user->Send(msg);
        }
        else
        {
            hasDisconnectedClient = true;
        }
    }

    // 끊어진 연결은 Vector에서 제거
    if (hasDisconnectedClient)
    {
        m_Users.erase(std::remove_if(m_Users.begin(), m_Users.end(),
        [](const std::shared_ptr<User>& u) 
        {
            return !u || !u->IsConnected();
        }), m_Users.end());
    }
}


void TcpServer::SendWhisperMessage(std::shared_ptr<User>& sender, const std::string& receiver, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (std::to_string(sender->GetID()) == receiver)
    {
        SendErrorMessage(sender, "You cannot whisper to yourself.");
        return;
    }

    for (auto& user : m_Users) 
    {
        if (std::to_string(user->GetID()) == receiver && user->IsConnected()) 
        {
            msg->set_receiver(receiver);
            user->Send(msg);
            return;
        }
    }
    SendErrorMessage(sender, "Receiver not found.");
}

void TcpServer::SendPartyMessage(std::shared_ptr<Party>& party, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (party != nullptr)
    {
        auto partyMembers = party->GetMembers();
        for (auto member : partyMembers)
        {
            auto session = GetUserById(member);
            if (session != nullptr)
            {
                session->Send(msg);
            }
        }
    }

}


void TcpServer::SendErrorMessage(std::shared_ptr<User>& user, const std::string& errorMessage)
{
    std::cout << "[SERVER] " << user->GetID() << " : " << errorMessage << "\n";
    // 에러 메시지 생성
    auto errMsg = std::make_shared<myChatMessage::ChatMessage>();
    errMsg->set_messagetype(myChatMessage::ChatMessageType::ERROR_MESSAGE);
    errMsg->set_content(errorMessage);

    // 해당 세션에게 에러 메시지 전송
    user->Send(errMsg);
}

void TcpServer::SendServerMessage(std::shared_ptr<User>& user, const std::string& serverMessage)
{
    std::cout << "[SERVER] " << user->GetID() << " : " << serverMessage << "\n";
    // 에러 메시지 생성
    auto serverMsg = std::make_shared<myChatMessage::ChatMessage>();
    serverMsg->set_messagetype(myChatMessage::ChatMessageType::SERVER_MESSAGE);
    serverMsg->set_content(serverMessage);

    // 해당 세션에게 에러 메시지 전송
    user->Send(serverMsg);
}

std::shared_ptr<User> TcpServer::GetUserById(uint32_t userId)
{
    for (auto user : m_Users)
    {
        if (user->GetID() == userId)
        {
            // 해당 세션 ID를 가진 세션을 찾았을 때
            return user;
        }
    }

    // 해당 세션 ID를 가진 세션이 없을 때
    return nullptr;
}
