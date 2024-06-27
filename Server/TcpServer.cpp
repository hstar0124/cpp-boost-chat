#include "TcpServer.h"

TcpServer::TcpServer(boost::asio::io_context& io_context, int port)
    : m_Acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    , m_IoContext(io_context)
    , m_PartyManager(std::make_unique<PartyManager>())
    , m_RedisClient(std::make_unique<CRedisClient>())
    , m_MySQLConnector(std::make_unique<MySQLConnector>("127.0.0.1", "root", "root", "hstar"))
{    
    if (!m_RedisClient->Initialize("127.0.0.1", 6379, 2, 10))
    {
        std::cout << "connect to redis failed" << std::endl;        
    }
    std::cout << "connect to redis Success!" << std::endl;
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


bool TcpServer::Start(uint32_t maxUser = 3)
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
    auto user = std::make_shared<UserSession>(m_IoContext);
    m_Acceptor.async_accept(user->GetSocket(),
        [this, user](const boost::system::error_code& err)
        {
            this->OnAccept(user, err);
        });
}



void TcpServer::OnAccept(std::shared_ptr<UserSession> user, const boost::system::error_code& err)
{
    if (!err)
    {   
        std::scoped_lock lock(m_NewUsersMutex);
        m_NewUsers.push(std::move(user));
        m_NewUsers.back()->Start();
    }
    else
    {
        std::cout << "[SERVER] Error " << err.message() << std::endl;
    }

    WaitForClientConnection();
}


void TcpServer::UpdateUsers()
{
    std::scoped_lock lock(m_UsersMutex, m_NewUsersMutex);

    // 연결이 종료된 User 정리
    m_Users.erase(std::remove_if(m_Users.begin(), m_Users.end(), [](auto& user) 
        {
        if (!user->IsConnected())
        {
            user->Close();
            return true;
        }
        return false;
        }), m_Users.end());

    // 접속한 유저 Session 키 값으로 인증 진행 및 대기열에 있는 유저 User Vector로 이동
    while (!m_NewUsers.empty() && m_Users.size() < m_MaxUser)
    {
        auto user = m_NewUsers.front();
        m_NewUsers.pop();

        auto msg = user->GetMessageInUserQueue();

        if (msg && VerifyUser(user, msg->content()))
        {
            auto userID = user->GetID();
            // 이미 접속 중인 유저가 있는지 확인하고 기존 유저를 해제
            auto it = std::find_if(m_Users.begin(), m_Users.end(), [&](const auto& existingUser) 
                {
                    return existingUser->GetID() == userID;
                });

            if (it != m_Users.end())
            {
                SendServerMessage(*it, "Logged out due to duplicate login.");
                (*it)->Close();
                m_Users.erase(it);
            }


            m_Users.push_back(std::move(user));
            SendLoginMessage(m_Users.back());
        }
        else
        {
            m_NewUsers.push(std::move(user)); // 인증에 실패한 경우 다시 큐에 넣기
        }
    }
}

bool TcpServer::VerifyUser(std::shared_ptr<UserSession>& user, const std::string& sessionId)
{
    std::string sessionKey = "Session:" + sessionId;
    std::string sessionValue;

    if (m_RedisClient->Get(sessionKey, &sessionValue) != RC_SUCCESS)
    {
        std::cout << "Not Found Session ID!!" << std::endl;
        return false;
    }

    std::cout << sessionKey << " : " << sessionValue << std::endl;
    user->SetID(StringToUint32(sessionValue));
    user->SetVerified(true);

    std::shared_ptr<UserEntity> userEntity = m_MySQLConnector->GetUserById(sessionValue);
    user->SetUserEntity(userEntity);

    return true;
}

void TcpServer::Update()
{
    while (1)
    {
        // 유저들 상태 업데이트
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

void TcpServer::OnMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
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
    case myChatMessage::ChatMessageType::FRIEND_REQUEST:
        HandleFriendRequestMessage(user, msg);
        break;
    case myChatMessage::ChatMessageType::FRIEND_ACCEPT:
        HandleFriendAcceptMessage(user, msg);
        break;
    default:
        SendErrorMessage(user, "Unknown message type received.");
        break;
    }
}



void TcpServer::HandleServerPing(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    auto sentTimeMs = std::stoll(msg->content());
    auto sentTime = std::chrono::milliseconds(sentTimeMs);
    auto currentTime = std::chrono::system_clock::now().time_since_epoch();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);

}

void TcpServer::HandleAllMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    std::cout << "[SERVER] Send message to all clients\n";
    msg->set_messagetype(myChatMessage::ChatMessageType::ALL_MESSAGE);
    SendAllUsers(msg);
}

void TcpServer::HandlePartyCreate(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
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

    if (m_PartyManager->IsPartyNameTaken(msg->content()))
    {
        SendErrorMessage(user, "The party name already exists.");
        return;
    }

    auto createdParty = m_PartyManager->CreateParty(user, msg->content());
    if (!createdParty)
    {
        SendErrorMessage(user, "The party creation failed!");
        return;
    }
    user->SetPartyId(createdParty->GetId());
    SendServerMessage(user, "Party creation successful");
}

void TcpServer::HandlePartyJoin(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
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

    auto joinedParty = m_PartyManager->JoinParty(user, msg->content());
    if (!joinedParty)
    {
        SendErrorMessage(user, "Party join Failed");
        return;
    }

    user->SetPartyId(joinedParty->GetId());
    SendServerMessage(user, "Party join successful");
}


void TcpServer::HandlePartyDelete(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->content().empty())
    {
        SendErrorMessage(user, "The party name is empty.");
        return;
    }
    std::cout << "[SERVER] Delete party\n";

    auto partyId = m_PartyManager->DeleteParty(user, msg->content());
    if (!partyId)
    {
        SendErrorMessage(user, "Party delete Failed");
        return;
    }
    
    for (auto& user : m_Users)
    {
        if (user->GetPartyId() == partyId)
        {
            user->SetPartyId(0);
        }
    }
    
    SendServerMessage(user, "Party delete successful");
}


void TcpServer::HandlePartyLeave(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
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

    if (!m_PartyManager->LeaveParty(user, msg->content()))
    {
        SendErrorMessage(user, "Sorry, as the party leader, you cannot leave the party. Deletion is the only option.");
        return;
    }

    user->SetPartyId(0);
    SendServerMessage(user, "Party leave successful");
}

void TcpServer::HandlePartyMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->content().empty())
    {
        SendErrorMessage(user, "The content of the party message is empty.");
        return;
    }

    auto party = m_PartyManager->FindPartyById(user->GetPartyId());
    if (!party)
    {
        SendErrorMessage(user, "It's a party not joined.");
        return;
    }

    SendPartyMessage(party, msg);
}

void TcpServer::HandleWhisperMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->receiver().empty() || msg->content().empty())
    {
        SendErrorMessage(user, "Recipient or content is empty.");
        return;
    }
    SendWhisperMessage(user, msg->receiver(), msg);
}

void TcpServer::HandleFriendRequestMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->content().empty())
    {
        SendErrorMessage(user, "The content of the friend id is empty.");
        return;
    }

    // 현재는 접속한 유저만 친구 추가 가능
    auto receiveUser = GetUserByUserId(msg->content());
    if (!receiveUser)
    {
        SendErrorMessage(user, "The user with the provided ID does not exist.");
        return;
    }

    bool isSuccess = m_MySQLConnector->CreateFriendRequest(std::to_string(user->GetID()), std::to_string(receiveUser->GetID()));
    if (!isSuccess)
    {
        SendErrorMessage(user, "Failed to create friend request.");
        return;
    }
    

    std::string inviteMessage = "Friend Request Received From [" + user->GetUserEntity()->GetUserId() + "]. "
        "To accept, /fa " + user->GetUserEntity()->GetUserId();

    SendServerMessage(receiveUser, inviteMessage);
}

void TcpServer::HandleFriendAcceptMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (msg->content().empty())
    {
        SendErrorMessage(user, "The content of the friend id is empty.");
        return;
    }

    // TODO : Mysql Process

    auto sender = GetUserById(StringToUint32(msg->content()));
    if (!sender)
    {
        SendErrorMessage(user, "Sender not found!");
        return;
    }

    SendServerMessage(sender, "Your friend request to [" + user->GetUserEntity()->GetUserId() + "] has been accepted.");
    SendServerMessage(user, "You have accepted the friend request from [" + sender->GetUserEntity()->GetUserId() + "].");

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
        [](const std::shared_ptr<UserSession>& u) 
        {
            return !u || !u->IsConnected();
        }), m_Users.end());
    }
}


void TcpServer::SendWhisperMessage(std::shared_ptr<UserSession>& sender, const std::string& receiver, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    if (sender->GetUserEntity()->GetUserId() == receiver)
    {
        SendErrorMessage(sender, "You cannot whisper to yourself.");
        return;
    }

    for (auto& user : m_Users) 
    {
        if (user->GetUserEntity()->GetUserId() == receiver && user->IsConnected())
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


void TcpServer::SendErrorMessage(std::shared_ptr<UserSession>& user, const std::string& errorMessage)
{
    std::cout << "[SERVER] " << user->GetUserEntity()->GetUserId() << " : " << errorMessage << "\n";
    // 에러 메시지 생성
    auto errMsg = std::make_shared<myChatMessage::ChatMessage>();
    errMsg->set_messagetype(myChatMessage::ChatMessageType::ERROR_MESSAGE);
    errMsg->set_content(errorMessage);

    // 해당 세션에게 에러 메시지 전송
    user->Send(errMsg);
}

void TcpServer::SendServerMessage(std::shared_ptr<UserSession>& user, const std::string& serverMessage)
{
    std::cout << "[SERVER] " << user->GetUserEntity()->GetUserId() << " : " << serverMessage << "\n";
    
    auto serverMsg = std::make_shared<myChatMessage::ChatMessage>();
    serverMsg->set_messagetype(myChatMessage::ChatMessageType::SERVER_MESSAGE);
    serverMsg->set_content(serverMessage);
        
    user->Send(serverMsg);
}

void TcpServer::SendLoginMessage(std::shared_ptr<UserSession>& user)
{
    std::cout << "[SERVER] " << user->GetUserEntity()->GetUserId() << " : Login Success!!" << "\n";
    auto serverMsg = std::make_shared<myChatMessage::ChatMessage>();
    serverMsg->set_messagetype(myChatMessage::ChatMessageType::LOGIN_MESSAGE);
    serverMsg->set_content("Login Success!!");

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

std::shared_ptr<UserSession> TcpServer::GetUserById(uint32_t userId)
{
    for (auto user : m_Users)
    {
        if (user->GetID() == userId)
        {
            return user;
        }
    }

    return nullptr;
}

std::shared_ptr<UserSession> TcpServer::GetUserByUserId(const std::string& userId)
{
    for (auto user : m_Users)
    {
        if (user->GetUserEntity()->GetUserId() == userId)
        {
            return user;
        }
    }

    return nullptr;
}
