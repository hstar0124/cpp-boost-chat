#include "TcpServer.h"

// TcpServer Ŭ���� ������ ����
TcpServer::TcpServer(boost::asio::io_context& io_context, int port, std::unique_ptr<CRedisClient> redisClient, std::unique_ptr<MySQLManager> mysqlManager, HSThreadPool& threadPool)
	: m_Acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
	, m_IoContext(io_context)
	, m_PartyManager(std::make_unique<PartyManager>())
	, m_RedisClient(std::move(redisClient))
	, m_MySQLConnector(std::move(mysqlManager))
	, m_ThreadPool(threadPool)
{
}

// TcpServer Ŭ���� �Ҹ��� ����
TcpServer::~TcpServer()
{
	try
	{
		// Acceptor�� IoContext ����
		m_Acceptor.close();
		m_IoContext.stop();

		// ContextThread�� join ������ ������ ��� �����带 join
		if (m_ContextThread.joinable())
		{
			m_ContextThread.join();
		}

		// ����� ���ǵ��� ����
		RemoveUserSessions();
		RemoveNewUserSessions();
	}
	catch (const std::exception& e)
	{
		// �Ҹ��ڿ��� ���ܰ� �߻��ϸ� �޽��� ���
		std::cerr << "[SERVER] Exception in destructor: " << e.what() << "\n";
	}

	// ���� ���� �޽��� ���
	std::cout << "[SERVER] Shutdown complete.\n";
}

// ���� ���� �Լ�
bool TcpServer::Start(uint32_t maxUser /* = 3 */)
{
	try
	{
		m_MaxUser = maxUser;
		// Ŭ���̾�Ʈ ������ ��ٸ��� �Լ� ȣ��
		WaitForClientConnection();
		// IoContext�� �����ϴ� ������ ����
		m_ContextThread = std::thread([this]() { m_IoContext.run(); });
	}
	catch (std::exception& e)
	{
		// ���� �߻� �� ���� �޽��� ���
		std::cerr << "[SERVER] Exception: " << e.what() << "\n";
		return false;
	}

	// ���� ���� �Ϸ� �޽��� ���
	std::cout << "[SERVER] Started!\n";
	return true;
}

// �۾��� ������ Ǯ�� �߰��ϴ� �Լ�
void TcpServer::EnqueueJob(std::function<void()>&& task)
{
	// ThreadPool�� EnqueueJob �Լ� ȣ��
	m_ThreadPool.EnqueueJob(std::move(task));
}

// Ŭ���̾�Ʈ ������ ��ٸ��� �Լ�
void TcpServer::WaitForClientConnection()
{
	// UserSession ��ü�� shared_ptr�� ����
	auto user = std::make_shared<UserSession>(m_IoContext);
	// Acceptor�� �̿��Ͽ� �񵿱������� Ŭ���̾�Ʈ ������ ����
	m_Acceptor.async_accept(user->GetSocket(),
		[this, user](const boost::system::error_code& err)
		{
			this->OnAccept(user, err);
		});
}

// Ŭ���̾�Ʈ ���� ó�� �ݹ� �Լ�
void TcpServer::OnAccept(std::shared_ptr<UserSession> user, const boost::system::error_code& err)
{
	if (!err)
	{
		// ���� ����� ����� ������ ť�� �߰��ϰ� ����
		std::scoped_lock lock(m_NewUsersMutex);
		m_NewUsers.push(std::move(user));
		m_NewUsers.back()->Start();
	}
	else
	{
		// ���� �߻� �� ���� �޽��� ���
		std::cout << "[SERVER] Error " << err.message() << std::endl;
	}

	// ���� Ŭ���̾�Ʈ ������ ��ٸ�
	WaitForClientConnection();
}

// ����� ������ ������Ʈ�ϴ� �Լ�
void TcpServer::UpdateUsers()
{
	std::scoped_lock lock(m_UsersMutex, m_NewUsersMutex);

	// ������� ���� ����� ������ ����
	m_Users.erase(std::remove_if(m_Users.begin(), m_Users.end(), [](auto& user)
		{
			if (!user->IsConnected())
			{
				user->Close();
				return true;
			}
			return false;
		}), m_Users.end());

	// ���ο� ����� ������ ���� ����� ��Ͽ� �߰�
	while (!m_NewUsers.empty() && m_Users.size() < m_MaxUser)
	{
		auto user = m_NewUsers.front();
		m_NewUsers.pop();

		auto msg = user->GetMessageInUserQueue();

		if (msg && VerifyUser(user, msg->content()))
		{
			auto userID = user->GetId();
			// �ߺ� �α��� üũ �� ó��
			auto it = std::find_if(m_Users.begin(), m_Users.end(), [&](const auto& existingUser)
				{
					return existingUser->GetId() == userID;
				});

			if (it != m_Users.end())
			{
				// �ߺ� �α��� �� ���� ����� ó��
				SendServerMessage(*it, "Logged out due to duplicate login.");
				(*it)->Close();
				m_Users.erase(it);
			}

			// ���ο� ����� ������ ��Ͽ� �߰��ϰ� �α��� �޽��� ����
			m_Users.push_back(std::move(user));
			SendLoginMessage(m_Users.back());
		}
		else
		{
			// ��ȿ���� ���� ����� ������ �ٽ� ť�� �߰�
			m_NewUsers.push(std::move(user));
		}
	}
}

bool TcpServer::VerifyUser(std::shared_ptr<UserSession>& user, const std::string& sessionId)
{
	std::string sessionKey = "Session:" + sessionId;
	std::string sessionValue;

	try
	{
		// Redis���� ���� ���� ��ȸ
		if (m_RedisClient->Get(sessionKey, &sessionValue) != RC_SUCCESS)
		{
			// ���� ID�� �������� �ʴ� ��� ó��
			std::cout << "Not Found Session ID!!" << std::endl;
			return false;
		}

		// ���� ���� �α�
		std::cout << sessionKey << " : " << sessionValue << std::endl;

		// UserSession ��ü�� ID ���� �� ���� ���� ����
		user->SetID(StringToUint32(sessionValue));
		user->SetVerified(true);

		// MySQL���� �ش� ����� ���� ��ȸ
		std::shared_ptr<UserEntity> userEntity = m_MySQLConnector->GetUserById(sessionValue);
		user->SetUserEntity(userEntity);

		return true;
	}
	catch (const std::exception& e)
	{
		// ���� �߻� �� ó��
		std::cout << "Exception occurred: " << e.what() << std::endl;
		return false;
	}

	return true;
}

void TcpServer::RemoveUserSessions()
{
	std::scoped_lock lock(m_UsersMutex);

	// ��� ����� ����� ������ �ݰ�, m_Users �����̳ʸ� ���ϴ�.
	for (auto& user : m_Users)
	{
		if (user && user->IsConnected())
		{
			user->Close();
		}
	}

	m_Users.clear();
}


void TcpServer::RemoveNewUserSessions()
{
	std::scoped_lock lock(m_NewUsersMutex);

	// ���� �߰��� ����� ���� ť�� ���ϴ�.
	while (!m_NewUsers.empty())
	{
		m_NewUsers.pop();
	}
}

void TcpServer::Update()
{
	while (1)
	{
		// ����� ���� ������Ʈ
		UpdateUsers();

		if (m_Users.empty())
		{
			continue;
		}

		// ��� ����� ����� ���ǿ� ���� �޽��� ó��
		for (auto u : m_Users)
		{
			if (u == nullptr || !u->IsConnected())
			{
				continue;
			}

			// ����ڷκ��� ���ŵ� �޽��� ó��
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
	std::cout << "On Message : " << msg->content() << "\n";

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
	// Ŭ���̾�Ʈ�� ���� �ð��� millisecond�� ��ȯ
	auto sentTimeMs = std::stoll(msg->content());
	auto sentTime = std::chrono::milliseconds(sentTimeMs);

	// ���� �ð����� ���� ���
	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);

	// ���⼭ elapsedTime �������� Ŭ���̾�Ʈ�� ���� ���� �ð� ���̰� milliseconds�� �������
	// �� ���� ����Ͽ� ��Ʈ��ũ ���� ���� ����ϰų� ������ �� ����
}


void TcpServer::HandleAllMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// �޽��� ������ ��� ������ ó������ ����
	if (msg->content().empty())
	{
		return;
	}

	// �������� ��� Ŭ���̾�Ʈ���� �޽��� ����
	std::cout << "[SERVER] Send message to all clients\n";

	// �޽��� Ÿ�� ����
	msg->set_messagetype(myChatMessage::ChatMessageType::ALL_MESSAGE);

	// ��� ����ڿ��� �޽��� ����
	SendAllUsers(msg);
}


void TcpServer::HandlePartyCreate(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// ��Ƽ �̸��� ��� ������ ���� �޽��� ���� �� ����
	if (msg->content().empty())
	{
		SendErrorMessage(user, "The party name is empty.");
		return;
	}

	// �̹� ��Ƽ�� ���� ���� ��� ���� �޽��� ���� �� ����
	if (user->GetPartyId() != 0)
	{
		SendErrorMessage(user, "Already in a party.");
		return;
	}

	std::cout << "[SERVER] Create party\n";

	// ��Ƽ �̸��� �̹� ��� ���� ��� ���� �޽��� ���� �� ����
	if (m_PartyManager->IsPartyNameTaken(msg->content()))
	{
		SendErrorMessage(user, "The party name already exists.");
		return;
	}

	// ��Ƽ ���� �� ����ڿ��� ��Ƽ ID ����
	auto createdParty = m_PartyManager->CreateParty(user, msg->content());
	if (!createdParty)
	{
		SendErrorMessage(user, "The party creation failed!");
		return;
	}
	user->SetPartyId(createdParty->GetId());

	// ��Ƽ ���� ���� �޽��� ����
	SendServerMessage(user, "Party creation successful");
}


void TcpServer::HandlePartyJoin(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// ��Ƽ �̸��� ��� ������ ���� �޽��� ���� �� ����
	if (msg->content().empty())
	{
		SendErrorMessage(user, "The party name is empty.");
		return;
	}

	// �̹� ��Ƽ�� ���� ���� ��� ���� �޽��� ���� �� ����
	if (user->GetPartyId() != 0)
	{
		SendErrorMessage(user, "Already in a party.");
		return;
	}

	// ��Ƽ�� �����ϰ�, ���������� ������ ��� ������� ��Ƽ ID ���� �� ���� �޽��� ����
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
	// ��Ƽ �̸��� ��� ������ ���� �޽��� ���� �� ����
	if (msg->content().empty())
	{
		SendErrorMessage(user, "The party name is empty.");
		return;
	}

	std::cout << "[SERVER] Delete party\n";

	// ��Ƽ ���� �õ� �� ���� �� ���� �޽��� ���� �� ����
	auto partyId = m_PartyManager->DeleteParty(user, msg->content());
	if (!partyId)
	{
		SendErrorMessage(user, "Party delete Failed");
		return;
	}

	// �ش� ��Ƽ�� ���� ��� ������� ��Ƽ ID�� �ʱ�ȭ
	for (auto& u : m_Users)
	{
		if (u->GetPartyId() == partyId)
		{
			u->SetPartyId(0);
		}
	}

	// ��Ƽ ���� ���� �޽��� ����
	SendServerMessage(user, "Party delete successful");
}


void TcpServer::HandlePartyLeave(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// ��Ƽ �̸��� ��� ������ ���� �޽��� ���� �� ����
	if (msg->content().empty())
	{
		SendErrorMessage(user, "The party name is empty.");
		return;
	}

	// ���� ����ڰ� � ��Ƽ���� ���� ���� ���� ��� ���� �޽��� ���� �� ����
	if (user->GetPartyId() == 0)
	{
		SendErrorMessage(user, "Not in a party.");
		return;
	}

	std::cout << "[SERVER] Leave party\n";

	// ����ڰ� ��Ƽ�� Ż���� �� ���� ���(��Ƽ ������ ���) ���� �޽��� ���� �� ����
	if (!m_PartyManager->LeaveParty(user, msg->content()))
	{
		SendErrorMessage(user, "Sorry, as the party leader, you cannot leave the party. Deletion is the only option.");
		return;
	}

	// ������� ��Ƽ ID�� �ʱ�ȭ�ϰ�, ���� �޽��� ����
	user->SetPartyId(0);
	SendServerMessage(user, "Party leave successful");
}


void TcpServer::HandlePartyMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// ��Ƽ �޽����� ������ ��� ������ ���� �޽��� ���� �� ����
	if (msg->content().empty())
	{
		SendErrorMessage(user, "The content of the party message is empty.");
		return;
	}

	// ����ڰ� ���� ���� ��Ƽ�� ã�Ƽ� �޽��� ����
	auto party = m_PartyManager->FindPartyById(user->GetPartyId());
	if (!party)
	{
		SendErrorMessage(user, "It's a party not joined.");
		return;
	}

	// �ش� ��Ƽ�� �޽��� ����
	SendPartyMessage(party, msg);
}


void TcpServer::HandleWhisperMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// �����ڳ� ������ ��� ������ ���� �޽����� �����ϰ� �Լ� ����
	if (msg->receiver().empty() || msg->content().empty())
	{
		SendErrorMessage(user, "Recipient or content is empty.");
		return;
	}

	// �����ڿ��� �ӼӸ� �޽����� ����
	SendWhisperMessage(user, msg->receiver(), msg);
}


void TcpServer::HandleFriendRequest(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg) {
	try {
		// ��û ��ȿ�� �˻縦 �񵿱������� ����
		auto validateFuture = ValidateRequest(user, msg);
		validateFuture.get();

		// ������� ���� ���θ� �񵿱������� Ȯ��
		auto checkUserExistenceFuture = CheckUserExistence(msg->content());
		auto receiveUser = checkUserExistenceFuture.get();

		// ģ�� ��û ���¸� �񵿱������� Ȯ��
		auto checkFriendRequestStatusFuture = CheckFriendRequestStatus(user, receiveUser);
		checkFriendRequestStatusFuture.get();

		// ģ�� ��û�� �߰��ϴ� �۾��� �񵿱������� ����
		auto addFriendRequestFuture = AddFriendRequest(user, receiveUser);
		addFriendRequestFuture.get();

		// ����ڵ鿡�� �˸��� ����
		NotifyUsers(user, receiveUser);
	}
	catch (const std::exception& e) {
		// ���� �߻� �� ���� �޽����� ����
		SendErrorMessage(user, e.what());
	}
}


std::future<void> TcpServer::ValidateRequest(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg) {
	return m_ThreadPool.EnqueueJob([this, user, msg]()
		{
			// ��û ������ ��� ������ ���ܸ� �߻���Ŵ
			if (msg->content().empty())
			{
				throw std::runtime_error("The content of is empty.");
			}
			// �ڱ� �ڽſ��� ��û�� ������ ��� ���ܸ� �߻���Ŵ
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
			// �־��� ����� ID�� ����ڸ� �˻��ϰ�, �������� ������ ���ܸ� �߻���Ŵ
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
			// ��û�ڿ� ������ ���� �̹� ģ�� ��û�� �ִ��� Ȯ���ϰ�, ������ ���ܸ� �߻���Ŵ
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
			// ģ�� ��û�� �����ͺ��̽��� �߰��ϰ�, ������ ��� ���ܸ� �߻���Ŵ
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
		// ��û ��ȿ�� �˻縦 �񵿱������� ����
		auto validateFuture = ValidateRequest(user, msg);
		validateFuture.get();

		// ��û�� ���� ������� ���� ���θ� �񵿱������� Ȯ��
		auto checkUserExistenceFuture = CheckUserExistence(msg->content());
		auto sender = checkUserExistenceFuture.get();

		// ģ�� ��û ���� �۾��� �񵿱������� ����
		auto processFriendAcceptFuture = ProcessFriendAccept(user, sender);
		processFriendAcceptFuture.get();

		// ������ ģ������ �˸��� ����
		NotifyAcceptUsers(user, sender);
	}
	catch (const std::exception& e) {
		// ���� �߻� �� ���� �޽����� ����
		SendErrorMessage(user, e.what());
	}
}



std::future<void> TcpServer::ProcessFriendAccept(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender)
{
	return m_ThreadPool.EnqueueJob([this, user, sender]()
		{
			try
			{
				// Ʈ����� ����
				m_MySQLConnector->BeginTransaction();
				// ģ�� ���踦 ������Ʈ�ϰ�, ģ�� ��Ͽ� �߰�
				m_MySQLConnector->UpdateFriend(std::to_string(sender->GetId()), std::to_string(user->GetId()), "A");
				m_MySQLConnector->AddFriendship(std::to_string(sender->GetId()), std::to_string(user->GetId()));
				// Ʈ����� Ŀ��
				m_MySQLConnector->CommitTransaction();
			}
			catch (const std::exception&)
			{
				// ���� �߻� �� Ʈ����� �ѹ��ϰ� ���� �߻�
				m_MySQLConnector->RollbackTransaction();
				throw std::runtime_error("Failed to process friend accept.");
			}
		});
}




void TcpServer::HandleFriendReject(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	try {
		// ��û ��ȿ�� �˻縦 �񵿱������� ����
		auto validateFuture = ValidateRequest(user, msg);
		validateFuture.get();

		// ��û�� ���� ������� ���� ���θ� �񵿱������� Ȯ��
		auto checkUserExistenceFuture = CheckUserExistence(msg->content());
		auto sender = checkUserExistenceFuture.get();

		// ģ�� ��û ���� �۾��� �񵿱������� ����
		auto processFriendRejectFuture = ProcessFriendReject(user, sender);
		processFriendRejectFuture.get();

		// ������ ģ������ �˸��� ����
		NotifyRejectUsers(user, sender);
	}
	catch (const std::exception& e) {
		// ���� �߻� �� ���� �޽����� ����
		SendErrorMessage(user, e.what());
	}
}


std::future<void> TcpServer::ProcessFriendReject(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender)
{
	return m_ThreadPool.EnqueueJob([this, user, sender]()
		{
			try
			{
				// Ʈ����� ����
				m_MySQLConnector->BeginTransaction();
				// ģ�� ��û�� �����ϰ�, Ʈ����� Ŀ��
				m_MySQLConnector->DeleteFriendRequest(std::to_string(sender->GetId()), std::to_string(user->GetId()));
				m_MySQLConnector->CommitTransaction();
			}
			catch (const std::exception&)
			{
				// ���� �߻� �� Ʈ����� �ѹ��ϰ� ���� �߻�
				m_MySQLConnector->RollbackTransaction();
				throw std::runtime_error("Failed to process friend reject.");
			}
		});
}


void TcpServer::NotifyUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> receiveUser) {
	// ��û�� ������� ID�� ������
	auto requestUserId = user->GetUserEntity()->GetUserId();
	// ģ�� ��û �޽��� ����
	std::string inviteMessage = "Friend Request Received From [" + requestUserId + "]. To accept, /fa " + requestUserId;

	// �������� ������ ã�� �޽����� ����
	auto receiveUserSession = GetUserByUserId(receiveUser->GetUserId());
	if (receiveUserSession)
	{
		SendServerMessage(receiveUserSession, inviteMessage);
	}

	// ��û ����ڿ��� ���� �޽��� ����
	SendServerMessage(user, "Success friend request message!");
}



void TcpServer::NotifyAcceptUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender)
{
	// ��û�� ���� ������� ������ ã�� �˸� �޽����� ����
	auto senderUserSession = GetUserByUserId(sender->GetUserId());
	if (senderUserSession)
	{
		SendServerMessage(senderUserSession, "Your friend request to [" + user->GetUserEntity()->GetUserId() + "] has been accepted.");
	}

	// ������ ����ڿ��� ���� �޽����� ����
	SendServerMessage(user, "You have accepted the friend request from [" + sender->GetUserId() + "].");
}



void TcpServer::NotifyRejectUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender)
{
	// ��û�� ���� ������� ������ ã�� �˸� �޽����� ����
	auto senderUserSession = GetUserByUserId(sender->GetUserId());
	if (senderUserSession)
	{
		SendServerMessage(senderUserSession, "Your friend request to [" + user->GetUserEntity()->GetUserId() + "] has been rejected.");
	}

	// ������ ����ڿ��� ���� �޽����� ����
	SendServerMessage(user, "You have rejected the friend request from [" + sender->GetUserId() + "].");
}



void TcpServer::SendAllUsers(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	bool hasDisconnectedClient = false; // ������ ���� Ŭ���̾�Ʈ ���θ� ǥ��

	// ��� ����ڿ��� �޽����� ����
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

	// ������ ���� Ŭ���̾�Ʈ�� ���Ϳ��� ����
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
	// �ڱ� �ڽſ��Դ� �ӼӸ��� ���� �� ����
	if (sender->GetUserEntity()->GetUserId() == receiver)
	{
		SendErrorMessage(sender, "You cannot whisper to yourself.");
		return;
	}

	// �����ڰ� �����ϰ� ����Ǿ� �ִ� ��� �޽����� ����
	for (auto& user : m_Users)
	{
		if (user->GetUserEntity()->GetUserId() == receiver && user->IsConnected())
		{
			msg->set_receiver(receiver);
			user->Send(msg);
			return;
		}
	}

	// �����ڸ� ã�� ���� ��� ���� �޽��� ����
	SendErrorMessage(sender, "Receiver not found.");
}


void TcpServer::SendPartyMessage(std::shared_ptr<Party>& party, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// ��Ƽ�� ��ȿ�� ��� ��Ƽ ������� �޽����� ����
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
	// ���� �޽����� ���� �ֿܼ� ���
	std::cout << "[SERVER] " << user->GetUserEntity()->GetUserId() << " : " << errorMessage << "\n";
	// Ŭ���̾�Ʈ���� ������ ���� �޽��� ����
	auto errMsg = std::make_shared<myChatMessage::ChatMessage>();
	errMsg->set_messagetype(myChatMessage::ChatMessageType::ERROR_MESSAGE);
	errMsg->set_content(errorMessage);

	// ����ڿ��� ���� �޽��� ����
	user->Send(errMsg);
}

void TcpServer::SendServerMessage(std::shared_ptr<UserSession>& user, const std::string& serverMessage)
{
	// ���� �޽����� ���� �ֿܼ� ���
	std::cout << "[SERVER] " << user->GetUserEntity()->GetUserId() << " : " << serverMessage << "\n";

	// ���� �޽��� ����
	auto serverMsg = std::make_shared<myChatMessage::ChatMessage>();
	serverMsg->set_messagetype(myChatMessage::ChatMessageType::SERVER_MESSAGE);
	serverMsg->set_content(serverMessage);

	// ����ڿ��� ���� �޽��� ����
	user->Send(serverMsg);
}


void TcpServer::SendLoginMessage(std::shared_ptr<UserSession>& user)
{
	// �α��� ���� �޽����� ���� �ֿܼ� ���
	std::cout << "[SERVER] " << user->GetUserEntity()->GetUserId() << " : Login Success!!" << "\n";

	// �α��� ���� �޽��� ����
	auto serverMsg = std::make_shared<myChatMessage::ChatMessage>();
	serverMsg->set_messagetype(myChatMessage::ChatMessageType::LOGIN_MESSAGE);
	serverMsg->set_content("Login Success!!");

	// ����ڿ��� �α��� ���� �޽��� ����
	user->Send(serverMsg);
}


uint32_t TcpServer::StringToUint32(const std::string& str)
{
	try
	{
		// ���ڿ��� ���ڷ� ��ȯ
		unsigned long result = std::stoul(str);
		// ��ȯ�� ���ڰ� uint32_t�� ������ �ʰ��ϸ� ���� ó��
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
	// ����� ID�� �ش��ϴ� ������ ã�� ��ȯ
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
	// ����� ID�� �ش��ϴ� ������ ã�� ��ȯ
	for (auto user : m_Users)
	{
		if (user->GetUserEntity()->GetUserId() == userId)
		{
			return user;
		}
	}

	return nullptr;
}