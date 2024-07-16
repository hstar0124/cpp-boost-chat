#include "User/include/UserSession.h"

UserSession::UserSession(boost::asio::io_context& io_context)
	: m_IoContext(io_context)
	, m_Socket(io_context)
	, m_PingTimer(io_context, std::chrono::seconds(5))
	, m_IsActive(true)
	, m_UserEntity(std::make_shared<UserEntity>())
{
	m_MessageQueue1 = std::make_shared<std::queue<std::shared_ptr<myChatMessage::ChatMessage>>>();
	m_MessageQueue2 = std::make_shared<std::queue<std::shared_ptr<myChatMessage::ChatMessage>>>();

	m_InputQueue = m_MessageQueue1;
	m_OutputQueue = m_MessageQueue2;
}

UserSession::~UserSession()
{
	Close();
	m_PingTimer.cancel();
	
	std::cout << "[SERVER] User {" << m_Id << "} is being destroyed. Cleaning up resources." << std::endl;
	m_Socket.close();
}


void UserSession::Start()
{
	m_IsActive = true;
	ReadHeader();
}

void UserSession::Close()
{
	m_IsActive = false;
}

bool UserSession::IsConnected()
{
	return m_IsActive;
}

void UserSession::SetID(uint32_t id)
{
	m_Id = id;
}

uint32_t UserSession::GetId() const
{
	return m_Id;
}

uint32_t UserSession::GetPartyId() const
{
	return m_PartyId;
}

std::shared_ptr<UserEntity> UserSession::GetUserEntity() const
{
	return m_UserEntity;
}

bool UserSession::GetVerified()
{
	return m_Verified;
}

void UserSession::SetPartyId(uint32_t partyId)
{
	m_PartyId = partyId;
}

void UserSession::SetVerified(bool isVerified)
{
	m_Verified = isVerified;
}

void UserSession::SetUserEntity(std::shared_ptr<UserEntity> userEntity)
{
	m_UserEntity = std::move(userEntity);
}

std::shared_ptr<myChatMessage::ChatMessage> UserSession::GetMessageInUserQueue()
{
	if (m_OutputQueue->empty() && !SwapQueues()) // 출력 큐가 비어 있고, 입력 큐와 교체할 수 없는 경우
	{
		return nullptr; // 메시지 없음
	}

	auto msg = m_OutputQueue->front(); // 출력 큐의 맨 앞 메시지 가져오기
	if (!msg)
	{
		return nullptr; // 메시지 없음
	}

	m_OutputQueue->pop(); // 큐에서 메시지 제거
	return msg; // 메시지 반환
}

boost::asio::ip::tcp::socket& UserSession::GetSocket()
{
	return m_Socket; // 소켓 객체 반환
}

bool UserSession::SwapQueues()
{
	std::lock_guard<std::mutex> lock(m_QueueMutex); // 큐 뮤텍스로 잠금
	if (m_OutputQueue->empty() && !m_InputQueue->empty()) // 출력 큐가 비어 있고, 입력 큐가 비어 있지 않은 경우
	{
		std::swap(m_InputQueue, m_OutputQueue); // 입력 큐와 출력 큐 교체
		return true; // 교체 성공
	}

	return false; // 교체 실패
}

void UserSession::StartPingTimer()
{
	m_PingTimer.expires_after(std::chrono::seconds(5)); // 5초 후에 ping 타이머 만료
	m_PingTimer.async_wait([ this ] (const boost::system::error_code& ec)
	{
		if (!ec)
		{
			if (IsConnected()) // 연결 상태 확인
			{
				SendPing(); // ping 메시지 전송
				StartPingTimer(); // 타이머 재시작
			}
		}
		else
		{
			HandleError("[SERVER] DisConnected Client"); // 오류 처리: 클라이언트 연결 끊김
		}
	});
}

void UserSession::SendPing()
{
	auto pingMsg = std::make_shared<myChatMessage::ChatMessage>(); // ping 메시지 생성
	pingMsg->set_messagetype(myChatMessage::ChatMessageType::SERVER_PING); // 메시지 타입 설정
	pingMsg->set_content(GetCurrentTimeMilliseconds()); // 현재 시간 밀리초로 설정
	Send(pingMsg); // 메시지 전송
}

std::string UserSession::GetCurrentTimeMilliseconds()
{
	auto now = std::chrono::system_clock::now(); // 현재 시간 가져오기
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now); // 밀리초 단위로 변환
	return std::to_string(now_ms.time_since_epoch().count()); // 현재 시간을 문자열로 반환
}

void UserSession::Send(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	boost::asio::post(m_IoContext,
	[ this, msg ] ()
	{
		AsyncWrite(msg); // 비동기로 메시지 쓰기 호출
	});
}

void UserSession::AsyncWrite(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	MessageConverter<myChatMessage::ChatMessage>::SerializeMessage(msg, m_Writebuf); // 메시지 직렬화
	MessageConverter<myChatMessage::ChatMessage>::SetSizeToBufferHeader(m_Writebuf); // 버퍼 헤더에 사이즈 설정

	boost::asio::async_write(m_Socket, boost::asio::buffer(m_Writebuf.data(), m_Writebuf.size()),
		[ this ] (const boost::system::error_code& err, const size_t transferred)
		{
			if (err)
			{
				HandleError("[SERVER] Write Error!!"); // 오류 처리: 쓰기 오류
			}
		});
}

void UserSession::HandleError(const std::string& errorMessage)
{
	std::cout << errorMessage << "\n"; // 오류 메시지 출력
	Close(); // 세션 종료
}

void UserSession::ReadHeader()
{
	m_Readbuf.clear(); // 버퍼 초기화
	m_Readbuf.resize(HEADER_SIZE); // 헤더 크기로 리사이즈

	boost::asio::async_read(m_Socket,
		boost::asio::buffer(m_Readbuf),
		[ this ] (const boost::system::error_code& err, const size_t size) {
			if (!err) {
				size_t bodySize = 0;
				for (int i = 0; i < 4; ++i) {
					bodySize += (m_Readbuf[3 - i] << (8 * i)); // 바디 사이즈 계산
				}
				ReadBody(bodySize); // 바디 읽기 호출
			}
			else {
				HandleError("[SERVER] Read Header Error!!\n" + err.message()); // 오류 처리: 헤더 읽기 오류
			}
		});
}

void UserSession::ReadBody(size_t bodySize)
{
	m_Readbuf.clear(); // 버퍼 초기화
	m_Readbuf.resize(bodySize); // 바디 크기로 리사이즈
	std::cout << "Server Body Size : " << bodySize << "\n"; // 서버 바디 사이즈 출력
	boost::asio::async_read(m_Socket,
		boost::asio::buffer(m_Readbuf),
		[ this ] (std::error_code ec, std::size_t size) {
			if (!ec) {
				std::shared_ptr<myChatMessage::ChatMessage> chatMessage = std::make_shared<myChatMessage::ChatMessage>(); // 채팅 메시지 생성
				if (chatMessage->ParseFromArray(m_Readbuf.data(), static_cast<int>(size))) { // 배열에서 파싱
					chatMessage->set_sender(m_UserEntity->GetUserId()); // 발신자 설정
					m_InputQueue->push(chatMessage); // 입력 큐에 삽입
				}
				ReadHeader(); // 헤더 읽기 호출
			}
			else {
				HandleError("[SERVER] Read Body Error!!"); // 오류 처리: 바디 읽기 오류
			}
		});
}