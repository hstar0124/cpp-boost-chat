#include "TcpServer.h"

TcpServer::TcpServer(boost::asio::io_context& io_context, int port, std::unique_ptr<CRedisClient> redisClient, std::unique_ptr<MySQLManager> mysqlManager, HSThreadPool& threadPool)
	: m_Acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
	, m_IoContext(io_context)
	, m_PartyManager(std::make_unique<PartyManager>())
	, m_RedisClient(std::move(redisClient))
	, m_MySQLConnector(std::move(mysqlManager))
	, m_ThreadPool(threadPool)
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


void TcpServer::EnqueueJob(std::function<void()>&& task)
{
	// Multi Thread에 작업 등록
	m_ThreadPool.EnqueueJob(std::move(task));
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
			auto userID = user->GetId();
			// 이미 접속 중인 유저가 있는지 확인하고 기존 유저를 해제
			auto it = std::find_if(m_Users.begin(), m_Users.end(), [&](const auto& existingUser)
				{
					return existingUser->GetId() == userID;
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

	try
	{
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
	catch (const std::exception& e)
	{
		std::cout << "Exception occurred: " << e.what() << std::endl;
		return false;
	}
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
		//m_ThreadPool.EnqueueJob([=](){ HandleFriendRequestMessage(user, msg); });
		HandleFriendRequest(user, msg);
		break;
	case myChatMessage::ChatMessageType::FRIEND_ACCEPT:
		HandleFriendAccept(user, msg);
		break;
	case myChatMessage::ChatMessageType::FRIEND_REJECT:
		HandleFriendReject(user, msg);
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
	// 메시지가 비어 있는지 확인
	if (msg->content().empty())
	{
		return;
	}

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

void TcpServer::HandleFriendRequest(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg) {
	try {
		auto validateFuture = ValidateRequest(user, msg);
		validateFuture.get();

		auto checkUserExistenceFuture = CheckUserExistence(msg->content());
		auto receiveUser = checkUserExistenceFuture.get();

		auto checkFriendRequestStatusFuture = CheckFriendRequestStatus(user, receiveUser);
		checkFriendRequestStatusFuture.get();

		auto addFriendRequestFuture = AddFriendRequest(user, receiveUser);
		addFriendRequestFuture.get();

		NotifyUsers(user, receiveUser);
	}
	catch (const std::exception& e) {
		SendErrorMessage(user, e.what());
	}
}

std::future<void> TcpServer::ValidateRequest(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg) {
	return m_ThreadPool.EnqueueJob([this, user, msg]() 
	{
		if (msg->content().empty()) 
		{
			throw std::runtime_error("The content of is empty.");
		}
		if (user->GetUserEntity()->GetUserId() == msg->content()) 
		{
			throw std::runtime_error("You cannot send request to yourself.");
		}
	});
}

std::future<std::shared_ptr<UserEntity>> TcpServer::CheckUserExistence(const std::string& userId) 
{
	return m_ThreadPool.EnqueueJob([this, userId]() -> std::shared_ptr<UserEntity> 
	{
		std::vector<MySQLManager::Condition> conditions = { {"user_id", userId} };
		auto receiveUser = m_MySQLConnector->GetUserByConditions(conditions);
		if (!receiveUser) 
		{
			throw std::runtime_error("User not found.");
		}
		
		return receiveUser;
	});
}

std::future<void> TcpServer::CheckFriendRequestStatus(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> receiveUser) {
	return m_ThreadPool.EnqueueJob([this, user, receiveUser]() 
	{
		auto requestId = std::to_string(user->GetId());
		auto receiveId = std::to_string(receiveUser->GetId());

		if (m_MySQLConnector->HasFriendRequest(requestId, receiveId)) 
		{
			throw std::runtime_error("You have already sent a friend request to this user.");
		}
		if (m_MySQLConnector->HasFriendRequest(receiveId, requestId)) 
		{
			throw std::runtime_error("This user has already sent you a friend request.");
		}
	});
}

std::future<void> TcpServer::AddFriendRequest(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> receiveUser) {
	return m_ThreadPool.EnqueueJob([this, user, receiveUser]() 
	{
		if (!m_MySQLConnector->AddFriendRequest(std::to_string(user->GetId()), std::to_string(receiveUser->GetId()))) 
		{
			throw std::runtime_error("Failed to create friend request.");
		}
	});
}




void TcpServer::HandleFriendAccept(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg) 
{
	try 
	{
		auto validateFuture = ValidateRequest(user, msg);
		validateFuture.get();

		auto checkUserExistenceFuture = CheckUserExistence(msg->content());
		auto sender = checkUserExistenceFuture.get();

		auto processFriendAcceptFuture = ProcessFriendAccept(user, sender);
		processFriendAcceptFuture.get();

		NotifyAcceptUsers(user, sender);
	}
	catch (const std::exception& e) {
		SendErrorMessage(user, e.what());
	}
}


std::future<void> TcpServer::ProcessFriendAccept(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender) 
{
	return m_ThreadPool.EnqueueJob([this, user, sender]() 
	{
		try 		
		{
			m_MySQLConnector->BeginTransaction();
			m_MySQLConnector->UpdateFriend(std::to_string(sender->GetId()), std::to_string(user->GetId()), "A");
			m_MySQLConnector->AddFriendship(std::to_string(sender->GetId()), std::to_string(user->GetId()));
			m_MySQLConnector->CommitTransaction();
		}
		catch (const std::exception&) 
		{
			m_MySQLConnector->RollbackTransaction();
			throw std::runtime_error("Failed to process friend accept.");
		}
	});
}



void TcpServer::HandleFriendReject(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg) 
{
	try {
		auto validateFuture = ValidateRequest(user, msg);
		validateFuture.get();

		auto checkUserExistenceFuture = CheckUserExistence(msg->content());
		auto sender = checkUserExistenceFuture.get();

		auto processFriendRejectFuture = ProcessFriendReject(user, sender);
		processFriendRejectFuture.get();

		NotifyRejectUsers(user, sender);
	}
	catch (const std::exception& e) {
		SendErrorMessage(user, e.what());
	}
}


std::future<void> TcpServer::ProcessFriendReject(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender) 
{
	return m_ThreadPool.EnqueueJob([this, user, sender]() 
	{
		try 
		{
			m_MySQLConnector->BeginTransaction();
			m_MySQLConnector->DeleteFriendRequest(std::to_string(sender->GetId()), std::to_string(user->GetId()));
			m_MySQLConnector->CommitTransaction();
		}
		catch (const std::exception&) 
		{
			m_MySQLConnector->RollbackTransaction();
			throw std::runtime_error("Failed to process friend reject.");
		}
	});
}

void TcpServer::NotifyUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> receiveUser) {
	auto requestUserId = user->GetUserEntity()->GetUserId();
	std::string inviteMessage = "Friend Request Received From [" + requestUserId + "]. To accept, /fa " + requestUserId;

	auto receiveUserSession = GetUserByUserId(receiveUser->GetUserId());
	if (receiveUserSession)
	{
		SendServerMessage(receiveUserSession, inviteMessage);
	}

	SendServerMessage(user, "Success friend request message!");
}


void TcpServer::NotifyAcceptUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender)
{
	auto senderUserSession = GetUserByUserId(sender->GetUserId());
	if (senderUserSession)
	{
		SendServerMessage(senderUserSession, "Your friend request to [" + user->GetUserEntity()->GetUserId() + "] has been accepted.");
	}

	SendServerMessage(user, "You have accepted the friend request from [" + sender->GetUserId() + "].");
}


void TcpServer::NotifyRejectUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender) 
{
	auto senderUserSession = GetUserByUserId(sender->GetUserId());
	if (senderUserSession) 
	{
		SendServerMessage(senderUserSession, "Your friend request to [" + user->GetUserEntity()->GetUserId() + "] has been rejected.");
	}

	SendServerMessage(user, "You have rejected the friend request from [" + sender->GetUserId() + "].");
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
		if (user->GetId() == userId)
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
