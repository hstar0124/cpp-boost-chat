#include "TcpServer.h"

// TcpServer 클래스 생성자 정의
TcpServer::TcpServer(boost::asio::io_context& io_context, int port, std::unique_ptr<CRedisClient> redisClient, std::unique_ptr<MySQLManager> mysqlManager, HSThreadPool& threadPool)
	: m_Acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
	, m_IoContext(io_context)
	, m_PartyManager(std::make_unique<PartyManager>())
	, m_RedisClient(std::move(redisClient))
	, m_MySQLConnector(std::move(mysqlManager))
	, m_ThreadPool(threadPool)
{
}

// TcpServer 클래스 소멸자 정의
TcpServer::~TcpServer()
{
	try
	{
		// Acceptor와 IoContext 정리
		m_Acceptor.close();
		m_IoContext.stop();

		// ContextThread가 join 가능한 상태일 경우 스레드를 join
		if (m_ContextThread.joinable())
		{
			m_ContextThread.join();
		}

		// 사용자 세션들을 정리
		RemoveUserSessions();
		RemoveNewUserSessions();
	}
	catch (const std::exception& e)
	{
		// 소멸자에서 예외가 발생하면 메시지 출력
		std::cerr << "[SERVER] Exception in destructor: " << e.what() << "\n";
	}

	// 서버 종료 메시지 출력
	std::cout << "[SERVER] Shutdown complete.\n";
}

// 서버 시작 함수
bool TcpServer::Start(uint32_t maxUser /* = 3 */)
{
	try
	{
		m_MaxUser = maxUser;
		// 클라이언트 연결을 기다리는 함수 호출
		WaitForClientConnection();
		// IoContext를 실행하는 스레드 시작
		m_ContextThread = std::thread([this]() { m_IoContext.run(); });
	}
	catch (std::exception& e)
	{
		// 예외 발생 시 에러 메시지 출력
		std::cerr << "[SERVER] Exception: " << e.what() << "\n";
		return false;
	}

	// 서버 시작 완료 메시지 출력
	std::cout << "[SERVER] Started!\n";
	return true;
}

// 작업을 스레드 풀에 추가하는 함수
void TcpServer::EnqueueJob(std::function<void()>&& task)
{
	// ThreadPool의 EnqueueJob 함수 호출
	m_ThreadPool.EnqueueJob(std::move(task));
}

// 클라이언트 연결을 기다리는 함수
void TcpServer::WaitForClientConnection()
{
	// UserSession 객체를 shared_ptr로 생성
	auto user = std::make_shared<UserSession>(m_IoContext);
	// Acceptor를 이용하여 비동기적으로 클라이언트 연결을 받음
	m_Acceptor.async_accept(user->GetSocket(),
		[this, user](const boost::system::error_code& err)
		{
			this->OnAccept(user, err);
		});
}

// 클라이언트 연결 처리 콜백 함수
void TcpServer::OnAccept(std::shared_ptr<UserSession> user, const boost::system::error_code& err)
{
	if (!err)
	{
		// 새로 연결된 사용자 세션을 큐에 추가하고 시작
		std::scoped_lock lock(m_NewUsersMutex);
		m_NewUsers.push(std::move(user));
		m_NewUsers.back()->Start();
	}
	else
	{
		// 에러 발생 시 에러 메시지 출력
		std::cout << "[SERVER] Error " << err.message() << std::endl;
	}

	// 다음 클라이언트 연결을 기다림
	WaitForClientConnection();
}

// 사용자 세션을 업데이트하는 함수
void TcpServer::UpdateUsers()
{
	std::scoped_lock lock(m_UsersMutex, m_NewUsersMutex);

	// 연결되지 않은 사용자 세션을 제거
	m_Users.erase(std::remove_if(m_Users.begin(), m_Users.end(), [](auto& user)
		{
			if (!user->IsConnected())
			{
				user->Close();
				return true;
			}
			return false;
		}), m_Users.end());

	// 새로운 사용자 세션을 기존 사용자 목록에 추가
	while (!m_NewUsers.empty() && m_Users.size() < m_MaxUser)
	{
		auto user = m_NewUsers.front();
		m_NewUsers.pop();

		auto msg = user->GetMessageInUserQueue();

		if (msg && VerifyUser(user, msg->content()))
		{
			auto userID = user->GetId();
			// 중복 로그인 체크 후 처리
			auto it = std::find_if(m_Users.begin(), m_Users.end(), [&](const auto& existingUser)
				{
					return existingUser->GetId() == userID;
				});

			if (it != m_Users.end())
			{
				// 중복 로그인 시 기존 사용자 처리
				SendServerMessage(*it, "Logged out due to duplicate login.");
				(*it)->Close();
				m_Users.erase(it);
			}

			// 새로운 사용자 세션을 목록에 추가하고 로그인 메시지 전송
			m_Users.push_back(std::move(user));
			SendLoginMessage(m_Users.back());
		}
		else
		{
			// 유효하지 않은 사용자 세션은 다시 큐에 추가
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
		// Redis에서 세션 정보 조회
		if (m_RedisClient->Get(sessionKey, &sessionValue) != RC_SUCCESS)
		{
			// 세션 ID가 존재하지 않는 경우 처리
			std::cout << "Not Found Session ID!!" << std::endl;
			return false;
		}

		// 세션 정보 로깅
		std::cout << sessionKey << " : " << sessionValue << std::endl;

		// UserSession 객체의 ID 설정 및 인증 상태 설정
		user->SetID(StringToUint32(sessionValue));
		user->SetVerified(true);

		// MySQL에서 해당 사용자 정보 조회
		std::shared_ptr<UserEntity> userEntity = m_MySQLConnector->GetUserById(sessionValue);
		user->SetUserEntity(userEntity);

		return true;
	}
	catch (const std::exception& e)
	{
		// 예외 발생 시 처리
		std::cout << "Exception occurred: " << e.what() << std::endl;
		return false;
	}

	return true;
}

void TcpServer::RemoveUserSessions()
{
	std::scoped_lock lock(m_UsersMutex);

	// 모든 연결된 사용자 세션을 닫고, m_Users 컨테이너를 비웁니다.
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

	// 새로 추가된 사용자 세션 큐를 비웁니다.
	while (!m_NewUsers.empty())
	{
		m_NewUsers.pop();
	}
}

void TcpServer::Update()
{
	while (1)
	{
		// 사용자 세션 업데이트
		UpdateUsers();

		if (m_Users.empty())
		{
			continue;
		}

		// 모든 연결된 사용자 세션에 대해 메시지 처리
		for (auto u : m_Users)
		{
			if (u == nullptr || !u->IsConnected())
			{
				continue;
			}

			// 사용자로부터 수신된 메시지 처리
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
	// 클라이언트가 보낸 시간을 millisecond로 변환
	auto sentTimeMs = std::stoll(msg->content());
	auto sentTime = std::chrono::milliseconds(sentTimeMs);

	// 현재 시간과의 차이 계산
	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);

	// 여기서 elapsedTime 변수에는 클라이언트와 서버 간의 시간 차이가 milliseconds로 들어있음
	// 이 값을 사용하여 네트워크 지연 등을 계산하거나 측정할 수 있음
}


void TcpServer::HandleAllMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// 메시지 내용이 비어 있으면 처리하지 않음
	if (msg->content().empty())
	{
		return;
	}

	// 서버에서 모든 클라이언트에게 메시지 전송
	std::cout << "[SERVER] Send message to all clients\n";

	// 메시지 타입 설정
	msg->set_messagetype(myChatMessage::ChatMessageType::ALL_MESSAGE);

	// 모든 사용자에게 메시지 전송
	SendAllUsers(msg);
}


void TcpServer::HandlePartyCreate(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// 파티 이름이 비어 있으면 에러 메시지 전송 후 종료
	if (msg->content().empty())
	{
		SendErrorMessage(user, "The party name is empty.");
		return;
	}

	// 이미 파티에 참여 중인 경우 에러 메시지 전송 후 종료
	if (user->GetPartyId() != 0)
	{
		SendErrorMessage(user, "Already in a party.");
		return;
	}

	std::cout << "[SERVER] Create party\n";

	// 파티 이름이 이미 사용 중인 경우 에러 메시지 전송 후 종료
	if (m_PartyManager->IsPartyNameTaken(msg->content()))
	{
		SendErrorMessage(user, "The party name already exists.");
		return;
	}

	// 파티 생성 및 사용자에게 파티 ID 설정
	auto createdParty = m_PartyManager->CreateParty(user, msg->content());
	if (!createdParty)
	{
		SendErrorMessage(user, "The party creation failed!");
		return;
	}
	user->SetPartyId(createdParty->GetId());

	// 파티 생성 성공 메시지 전송
	SendServerMessage(user, "Party creation successful");
}


void TcpServer::HandlePartyJoin(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// 파티 이름이 비어 있으면 에러 메시지 전송 후 종료
	if (msg->content().empty())
	{
		SendErrorMessage(user, "The party name is empty.");
		return;
	}

	// 이미 파티에 참여 중인 경우 에러 메시지 전송 후 종료
	if (user->GetPartyId() != 0)
	{
		SendErrorMessage(user, "Already in a party.");
		return;
	}

	// 파티에 참여하고, 성공적으로 참여한 경우 사용자의 파티 ID 설정 및 성공 메시지 전송
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
	// 파티 이름이 비어 있으면 에러 메시지 전송 후 종료
	if (msg->content().empty())
	{
		SendErrorMessage(user, "The party name is empty.");
		return;
	}

	std::cout << "[SERVER] Delete party\n";

	// 파티 삭제 시도 및 실패 시 에러 메시지 전송 후 종료
	auto partyId = m_PartyManager->DeleteParty(user, msg->content());
	if (!partyId)
	{
		SendErrorMessage(user, "Party delete Failed");
		return;
	}

	// 해당 파티에 속한 모든 사용자의 파티 ID를 초기화
	for (auto& u : m_Users)
	{
		if (u->GetPartyId() == partyId)
		{
			u->SetPartyId(0);
		}
	}

	// 파티 삭제 성공 메시지 전송
	SendServerMessage(user, "Party delete successful");
}


void TcpServer::HandlePartyLeave(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// 파티 이름이 비어 있으면 에러 메시지 전송 후 종료
	if (msg->content().empty())
	{
		SendErrorMessage(user, "The party name is empty.");
		return;
	}

	// 현재 사용자가 어떤 파티에도 속해 있지 않은 경우 에러 메시지 전송 후 종료
	if (user->GetPartyId() == 0)
	{
		SendErrorMessage(user, "Not in a party.");
		return;
	}

	std::cout << "[SERVER] Leave party\n";

	// 사용자가 파티를 탈퇴할 수 없는 경우(파티 리더인 경우) 에러 메시지 전송 후 종료
	if (!m_PartyManager->LeaveParty(user, msg->content()))
	{
		SendErrorMessage(user, "Sorry, as the party leader, you cannot leave the party. Deletion is the only option.");
		return;
	}

	// 사용자의 파티 ID를 초기화하고, 성공 메시지 전송
	user->SetPartyId(0);
	SendServerMessage(user, "Party leave successful");
}


void TcpServer::HandlePartyMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// 파티 메시지의 내용이 비어 있으면 에러 메시지 전송 후 종료
	if (msg->content().empty())
	{
		SendErrorMessage(user, "The content of the party message is empty.");
		return;
	}

	// 사용자가 참여 중인 파티를 찾아서 메시지 전송
	auto party = m_PartyManager->FindPartyById(user->GetPartyId());
	if (!party)
	{
		SendErrorMessage(user, "It's a party not joined.");
		return;
	}

	// 해당 파티에 메시지 전송
	SendPartyMessage(party, msg);
}


void TcpServer::HandleWhisperMessage(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// 수신자나 내용이 비어 있으면 에러 메시지를 전송하고 함수 종료
	if (msg->receiver().empty() || msg->content().empty())
	{
		SendErrorMessage(user, "Recipient or content is empty.");
		return;
	}

	// 수신자에게 귓속말 메시지를 전송
	SendWhisperMessage(user, msg->receiver(), msg);
}


void TcpServer::HandleFriendRequest(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg) {
	try {
		// 요청 유효성 검사를 비동기적으로 실행
		auto validateFuture = ValidateRequest(user, msg);
		validateFuture.get();

		// 사용자의 존재 여부를 비동기적으로 확인
		auto checkUserExistenceFuture = CheckUserExistence(msg->content());
		auto receiveUser = checkUserExistenceFuture.get();

		// 친구 요청 상태를 비동기적으로 확인
		auto checkFriendRequestStatusFuture = CheckFriendRequestStatus(user, receiveUser);
		checkFriendRequestStatusFuture.get();

		// 친구 요청을 추가하는 작업을 비동기적으로 실행
		auto addFriendRequestFuture = AddFriendRequest(user, receiveUser);
		addFriendRequestFuture.get();

		// 사용자들에게 알림을 보냄
		NotifyUsers(user, receiveUser);
	}
	catch (const std::exception& e) {
		// 예외 발생 시 에러 메시지를 전송
		SendErrorMessage(user, e.what());
	}
}


std::future<void> TcpServer::ValidateRequest(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg) {
	return m_ThreadPool.EnqueueJob([this, user, msg]()
		{
			// 요청 내용이 비어 있으면 예외를 발생시킴
			if (msg->content().empty())
			{
				throw std::runtime_error("The content of is empty.");
			}
			// 자기 자신에게 요청을 보내는 경우 예외를 발생시킴
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
			// 주어진 사용자 ID로 사용자를 검색하고, 존재하지 않으면 예외를 발생시킴
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
			// 요청자와 수신자 간에 이미 친구 요청이 있는지 확인하고, 있으면 예외를 발생시킴
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
			// 친구 요청을 데이터베이스에 추가하고, 실패한 경우 예외를 발생시킴
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
		// 요청 유효성 검사를 비동기적으로 실행
		auto validateFuture = ValidateRequest(user, msg);
		validateFuture.get();

		// 요청을 보낸 사용자의 존재 여부를 비동기적으로 확인
		auto checkUserExistenceFuture = CheckUserExistence(msg->content());
		auto sender = checkUserExistenceFuture.get();

		// 친구 요청 수락 작업을 비동기적으로 실행
		auto processFriendAcceptFuture = ProcessFriendAccept(user, sender);
		processFriendAcceptFuture.get();

		// 수락된 친구에게 알림을 보냄
		NotifyAcceptUsers(user, sender);
	}
	catch (const std::exception& e) {
		// 예외 발생 시 에러 메시지를 전송
		SendErrorMessage(user, e.what());
	}
}



std::future<void> TcpServer::ProcessFriendAccept(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender)
{
	return m_ThreadPool.EnqueueJob([this, user, sender]()
		{
			try
			{
				// 트랜잭션 시작
				m_MySQLConnector->BeginTransaction();
				// 친구 관계를 업데이트하고, 친구 목록에 추가
				m_MySQLConnector->UpdateFriend(std::to_string(sender->GetId()), std::to_string(user->GetId()), "A");
				m_MySQLConnector->AddFriendship(std::to_string(sender->GetId()), std::to_string(user->GetId()));
				// 트랜잭션 커밋
				m_MySQLConnector->CommitTransaction();
			}
			catch (const std::exception&)
			{
				// 오류 발생 시 트랜잭션 롤백하고 예외 발생
				m_MySQLConnector->RollbackTransaction();
				throw std::runtime_error("Failed to process friend accept.");
			}
		});
}




void TcpServer::HandleFriendReject(std::shared_ptr<UserSession> user, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	try {
		// 요청 유효성 검사를 비동기적으로 실행
		auto validateFuture = ValidateRequest(user, msg);
		validateFuture.get();

		// 요청을 보낸 사용자의 존재 여부를 비동기적으로 확인
		auto checkUserExistenceFuture = CheckUserExistence(msg->content());
		auto sender = checkUserExistenceFuture.get();

		// 친구 요청 거절 작업을 비동기적으로 실행
		auto processFriendRejectFuture = ProcessFriendReject(user, sender);
		processFriendRejectFuture.get();

		// 거절된 친구에게 알림을 보냄
		NotifyRejectUsers(user, sender);
	}
	catch (const std::exception& e) {
		// 예외 발생 시 에러 메시지를 전송
		SendErrorMessage(user, e.what());
	}
}


std::future<void> TcpServer::ProcessFriendReject(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender)
{
	return m_ThreadPool.EnqueueJob([this, user, sender]()
		{
			try
			{
				// 트랜잭션 시작
				m_MySQLConnector->BeginTransaction();
				// 친구 요청을 삭제하고, 트랜잭션 커밋
				m_MySQLConnector->DeleteFriendRequest(std::to_string(sender->GetId()), std::to_string(user->GetId()));
				m_MySQLConnector->CommitTransaction();
			}
			catch (const std::exception&)
			{
				// 오류 발생 시 트랜잭션 롤백하고 예외 발생
				m_MySQLConnector->RollbackTransaction();
				throw std::runtime_error("Failed to process friend reject.");
			}
		});
}


void TcpServer::NotifyUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> receiveUser) {
	// 요청한 사용자의 ID를 가져옴
	auto requestUserId = user->GetUserEntity()->GetUserId();
	// 친구 요청 메시지 생성
	std::string inviteMessage = "Friend Request Received From [" + requestUserId + "]. To accept, /fa " + requestUserId;

	// 수신자의 세션을 찾아 메시지를 전송
	auto receiveUserSession = GetUserByUserId(receiveUser->GetUserId());
	if (receiveUserSession)
	{
		SendServerMessage(receiveUserSession, inviteMessage);
	}

	// 요청 사용자에게 성공 메시지 전송
	SendServerMessage(user, "Success friend request message!");
}



void TcpServer::NotifyAcceptUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender)
{
	// 요청을 보낸 사용자의 세션을 찾아 알림 메시지를 전송
	auto senderUserSession = GetUserByUserId(sender->GetUserId());
	if (senderUserSession)
	{
		SendServerMessage(senderUserSession, "Your friend request to [" + user->GetUserEntity()->GetUserId() + "] has been accepted.");
	}

	// 수락된 사용자에게 성공 메시지를 전송
	SendServerMessage(user, "You have accepted the friend request from [" + sender->GetUserId() + "].");
}



void TcpServer::NotifyRejectUsers(std::shared_ptr<UserSession> user, std::shared_ptr<UserEntity> sender)
{
	// 요청을 보낸 사용자의 세션을 찾아 알림 메시지를 전송
	auto senderUserSession = GetUserByUserId(sender->GetUserId());
	if (senderUserSession)
	{
		SendServerMessage(senderUserSession, "Your friend request to [" + user->GetUserEntity()->GetUserId() + "] has been rejected.");
	}

	// 거절된 사용자에게 성공 메시지를 전송
	SendServerMessage(user, "You have rejected the friend request from [" + sender->GetUserId() + "].");
}



void TcpServer::SendAllUsers(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	bool hasDisconnectedClient = false; // 연결이 끊긴 클라이언트 여부를 표시

	// 모든 사용자에게 메시지를 전송
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

	// 연결이 끊긴 클라이언트를 벡터에서 제거
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
	// 자기 자신에게는 귓속말을 보낼 수 없음
	if (sender->GetUserEntity()->GetUserId() == receiver)
	{
		SendErrorMessage(sender, "You cannot whisper to yourself.");
		return;
	}

	// 수신자가 존재하고 연결되어 있는 경우 메시지를 전송
	for (auto& user : m_Users)
	{
		if (user->GetUserEntity()->GetUserId() == receiver && user->IsConnected())
		{
			msg->set_receiver(receiver);
			user->Send(msg);
			return;
		}
	}

	// 수신자를 찾지 못한 경우 에러 메시지 전송
	SendErrorMessage(sender, "Receiver not found.");
}


void TcpServer::SendPartyMessage(std::shared_ptr<Party>& party, std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	// 파티가 유효한 경우 파티 멤버에게 메시지를 전송
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
	// 에러 메시지를 서버 콘솔에 출력
	std::cout << "[SERVER] " << user->GetUserEntity()->GetUserId() << " : " << errorMessage << "\n";
	// 클라이언트에게 전송할 에러 메시지 생성
	auto errMsg = std::make_shared<myChatMessage::ChatMessage>();
	errMsg->set_messagetype(myChatMessage::ChatMessageType::ERROR_MESSAGE);
	errMsg->set_content(errorMessage);

	// 사용자에게 에러 메시지 전송
	user->Send(errMsg);
}

void TcpServer::SendServerMessage(std::shared_ptr<UserSession>& user, const std::string& serverMessage)
{
	// 서버 메시지를 서버 콘솔에 출력
	std::cout << "[SERVER] " << user->GetUserEntity()->GetUserId() << " : " << serverMessage << "\n";

	// 서버 메시지 생성
	auto serverMsg = std::make_shared<myChatMessage::ChatMessage>();
	serverMsg->set_messagetype(myChatMessage::ChatMessageType::SERVER_MESSAGE);
	serverMsg->set_content(serverMessage);

	// 사용자에게 서버 메시지 전송
	user->Send(serverMsg);
}


void TcpServer::SendLoginMessage(std::shared_ptr<UserSession>& user)
{
	// 로그인 성공 메시지를 서버 콘솔에 출력
	std::cout << "[SERVER] " << user->GetUserEntity()->GetUserId() << " : Login Success!!" << "\n";

	// 로그인 성공 메시지 생성
	auto serverMsg = std::make_shared<myChatMessage::ChatMessage>();
	serverMsg->set_messagetype(myChatMessage::ChatMessageType::LOGIN_MESSAGE);
	serverMsg->set_content("Login Success!!");

	// 사용자에게 로그인 성공 메시지 전송
	user->Send(serverMsg);
}


uint32_t TcpServer::StringToUint32(const std::string& str)
{
	try
	{
		// 문자열을 숫자로 변환
		unsigned long result = std::stoul(str);
		// 변환된 숫자가 uint32_t의 범위를 초과하면 예외 처리
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
	// 사용자 ID에 해당하는 세션을 찾아 반환
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
	// 사용자 ID에 해당하는 세션을 찾아 반환
	for (auto user : m_Users)
	{
		if (user->GetUserEntity()->GetUserId() == userId)
		{
			return user;
		}
	}

	return nullptr;
}