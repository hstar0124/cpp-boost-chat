#include "TcpServer.h"

TcpServer::TcpServer(boost::asio::io_context& io_context, int port)
    : m_Acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    , m_IoContext(io_context)
    , m_PartyManager()
{    

}

TcpServer::~TcpServer()
{
    try
    {
        m_Acceptor.close();

        m_IoContext.stop();

        if (m_ContextThread.joinable())
        {
            m_ContextThread.join();
        }

        {
            std::scoped_lock lock(m_UsersMutex);

            for (auto& user : m_Users)
            {
                if (user && user->IsConnected())
                {
                    user->Close();
                }
            }

            m_Users.clear();
        }

        {
            std::scoped_lock lock(m_NewUsersMutex);

            while (!m_NewUsers.empty())
            {
                m_NewUsers.pop();
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[SERVER] Exception in destructor: " << e.what() << "\n";
    }

    std::cout << "[SERVER] Shutdown complete.\n";
}


bool TcpServer::Start(uint32_t maxUser = 2)
{
    try
    {
        m_MaxUser = maxUser;
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
        std::scoped_lock lock(m_NewUsersMutex);
        m_NewUsers.push(std::move(user));
        m_NewUsers.back()->Start(m_IdCounter++);
    }
    else
    {
        std::cout << "[SERVER] Error " << err.message() << std::endl;
    }

    WaitForClientConnection();
}


void TcpServer::UpdateUsers()
{
    {
        std::scoped_lock lock(m_UsersMutex);
        m_Users.erase(std::remove_if(m_Users.begin(), m_Users.end(), [](auto& user)
            {
            if (!user->IsConnected()) 
            {
                user->Close();
                return true;
            }
            return false;
            }), m_Users.end());
    }

    {
        std::scoped_lock lock(m_NewUsersMutex);

        while (!m_NewUsers.empty() && m_Users.size() < m_MaxUser) 
        {
            auto user = std::move(m_NewUsers.front());
            m_NewUsers.pop();
            m_Users.push_back(std::move(user));
            SendServerMessage(m_Users.back(), "Connection successful");
        }
    }
}

void TcpServer::Update()
{
    while (1)
    {
        // 새로 접속한 유저를 유저 vector에 추가
        UpdateUsers();

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
        HandleServerPing(user, msg);
        break;
    case myChatMessage::ChatMessageType::ALL_MESSAGE:
        HandleAllMessage(user, msg);
        break;
    case myChatMessage::ChatMessageType::PARTY_CREATE:
        HandlePartyCreate(user, msg);
        break;
    case myChatMessage::ChatMessageType::PARTY_DELETE:
        HandlePartyDelete(user, msg);
        break;
    case myChatMessage::ChatMessageType::PARTY_JOIN:
        HandlePartyJoin(user, msg);
        break;
    case myChatMessage::ChatMessageType::PARTY_LEAVE:
        HandlePartyLeave(user, msg);
        break;
    case myChatMessage::ChatMessageType::PARTY_MESSAGE:
        HandlePartyMessage(user, msg);
        break;
    case myChatMessage::ChatMessageType::WHISPER_MESSAGE:
        HandleWhisperMessage(user, msg);
        break;
    default:
        SendErrorMessage(user, "Unknown message type received.");
        break;
    }
}

void TcpServer::HandleServerPing(std::shared_ptr<User> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    auto sentTimeMs = std::stoll(msg->content());
    auto sentTime = std::chrono::milliseconds(sentTimeMs);
    auto currentTime = std::chrono::system_clock::now().time_since_epoch();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);
    // Further handling
}

void TcpServer::HandleAllMessage(std::shared_ptr<User> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    std::cout << "[SERVER] Send message to all clients\n";
    msg->set_messagetype(myChatMessage::ChatMessageType::ALL_MESSAGE);
    SendAllUsers(msg);
}

void TcpServer::HandlePartyCreate(std::shared_ptr<User> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->content().empty())
    {
        SendErrorMessage(user, "The party name is empty.");
        return;
    }

    if (user->GetPartyId() != 0)
    {
        SendErrorMessage(user, "Already in a party.");
        return;
    }

    std::cout << "[SERVER] Create party\n";

    if (m_PartyManager.IsPartyNameTaken(msg->content()))
    {
        SendErrorMessage(user, "The party name already exists.");
        return;
    }

    auto createdParty = m_PartyManager.CreateParty(user, msg->content());
    if (!createdParty)
    {
        SendErrorMessage(user, "The party creation failed!");
        return;
    }
    user->SetPartyId(createdParty->GetId());
    SendServerMessage(user, "Party creation successful");
}

void TcpServer::HandlePartyJoin(std::shared_ptr<User> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->content().empty())
    {
        SendErrorMessage(user, "The party name is empty.");
        return;
    }

    if (user->GetPartyId() != 0)
    {
        SendErrorMessage(user, "Already in a party.");
        return;
    }

    uint32_t partyId = StringToUint32(msg->content());
    if (!m_PartyManager.HasParty(partyId))
    {
        SendErrorMessage(user, "The party does not exist.");
        return;
    }

    auto party = m_PartyManager.FindPartyById(partyId);
    if (party->HasMember(user->GetID()))
    {
        SendErrorMessage(user, "Already joined.");
        return;
    }

    party->AddMember(user->GetID());
    user->SetPartyId(partyId);
}


void TcpServer::HandlePartyDelete(std::shared_ptr<User> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->content().empty())
    {
        SendErrorMessage(user, "The party name is empty.");
        return;
    }
    std::cout << "[SERVER] Delete party\n";

    if (!m_PartyManager.DeleteParty(user, msg->content()))
    {
        SendErrorMessage(user, "Party delete Failed");
        return;
    }
    
    user->SetPartyId(0);
    SendServerMessage(user, "Party delete successful");
}


void TcpServer::HandlePartyLeave(std::shared_ptr<User> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->content().empty())
    {
        SendErrorMessage(user, "The party name is empty.");
        return;
    }

    if (user->GetPartyId() == 0)
    {
        SendErrorMessage(user, "Not in a party.");
        return;
    }

    std::cout << "[SERVER] Leave party\n";

    if (!m_PartyManager.LeaveParty(user, msg->content()))
    {
        SendErrorMessage(user, "Sorry, as the party leader, you cannot leave the party. Deletion is the only option.");
        return;
    }

    user->SetPartyId(0);
    SendServerMessage(user, "Party leave successful");
}

void TcpServer::HandlePartyMessage(std::shared_ptr<User> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->content().empty())
    {
        SendErrorMessage(user, "The content of the party message is empty.");
        return;
    }

    auto party = m_PartyManager.FindPartyById(user->GetPartyId());
    if (!party)
    {
        SendErrorMessage(user, "It's a party not joined.");
        return;
    }

    SendPartyMessage(party, msg);
}

void TcpServer::HandleWhisperMessage(std::shared_ptr<User> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->receiver().empty() || msg->content().empty())
    {
        SendErrorMessage(user, "Recipient or content is empty.");
        return;
    }
    SendWhisperMessage(user, msg->receiver(), msg);
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

uint32_t TcpServer::StringToUint32(const std::string& str)
{
    try
    {
        unsigned long result = std::stoul(str);
        if (result > std::numeric_limits<uint32_t>::max())
        {
            throw std::out_of_range("Converted number is out of range for uint32_t");
        }
        return static_cast<uint32_t>(result);
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Invalid argument: " << e.what() << std::endl;
        throw;
    }
    catch (const std::out_of_range& e)
    {
        std::cerr << "Out of range: " << e.what() << std::endl;
        throw;
    }
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
