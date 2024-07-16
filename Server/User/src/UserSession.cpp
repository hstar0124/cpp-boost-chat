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
	if (m_OutputQueue->empty() && !SwapQueues()) // ��� ť�� ��� �ְ�, �Է� ť�� ��ü�� �� ���� ���
	{
		return nullptr; // �޽��� ����
	}

	auto msg = m_OutputQueue->front(); // ��� ť�� �� �� �޽��� ��������
	if (!msg)
	{
		return nullptr; // �޽��� ����
	}

	m_OutputQueue->pop(); // ť���� �޽��� ����
	return msg; // �޽��� ��ȯ
}

boost::asio::ip::tcp::socket& UserSession::GetSocket()
{
	return m_Socket; // ���� ��ü ��ȯ
}

bool UserSession::SwapQueues()
{
	std::lock_guard<std::mutex> lock(m_QueueMutex); // ť ���ؽ��� ���
	if (m_OutputQueue->empty() && !m_InputQueue->empty()) // ��� ť�� ��� �ְ�, �Է� ť�� ��� ���� ���� ���
	{
		std::swap(m_InputQueue, m_OutputQueue); // �Է� ť�� ��� ť ��ü
		return true; // ��ü ����
	}

	return false; // ��ü ����
}

void UserSession::StartPingTimer()
{
	m_PingTimer.expires_after(std::chrono::seconds(5)); // 5�� �Ŀ� ping Ÿ�̸� ����
	m_PingTimer.async_wait([this](const boost::system::error_code& ec)
		{
			if (!ec)
			{
				if (IsConnected()) // ���� ���� Ȯ��
				{
					SendPing(); // ping �޽��� ����
					StartPingTimer(); // Ÿ�̸� �����
				}
			}
			else
			{
				HandleError("[SERVER] DisConnected Client"); // ���� ó��: Ŭ���̾�Ʈ ���� ����
			}
		});
}

void UserSession::SendPing()
{
	auto pingMsg = std::make_shared<myChatMessage::ChatMessage>(); // ping �޽��� ����
	pingMsg->set_messagetype(myChatMessage::ChatMessageType::SERVER_PING); // �޽��� Ÿ�� ����
	pingMsg->set_content(GetCurrentTimeMilliseconds()); // ���� �ð� �и��ʷ� ����
	Send(pingMsg); // �޽��� ����
}

std::string UserSession::GetCurrentTimeMilliseconds()
{
	auto now = std::chrono::system_clock::now(); // ���� �ð� ��������
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now); // �и��� ������ ��ȯ
	return std::to_string(now_ms.time_since_epoch().count()); // ���� �ð��� ���ڿ��� ��ȯ
}

void UserSession::Send(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	boost::asio::post(m_IoContext,
		[this, msg]()
		{
			AsyncWrite(msg); // �񵿱�� �޽��� ���� ȣ��
		});
}

void UserSession::AsyncWrite(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	MessageConverter<myChatMessage::ChatMessage>::SerializeMessage(msg, m_Writebuf); // �޽��� ����ȭ
	MessageConverter<myChatMessage::ChatMessage>::SetSizeToBufferHeader(m_Writebuf); // ���� ����� ������ ����

	boost::asio::async_write(m_Socket, boost::asio::buffer(m_Writebuf.data(), m_Writebuf.size()),
		[this](const boost::system::error_code& err, const size_t transferred)
		{
			if (err)
			{
				HandleError("[SERVER] Write Error!!"); // ���� ó��: ���� ����
			}
		});
}

void UserSession::HandleError(const std::string& errorMessage)
{
	std::cout << errorMessage << "\n"; // ���� �޽��� ���
	Close(); // ���� ����
}

void UserSession::ReadHeader()
{
	m_Readbuf.clear(); // ���� �ʱ�ȭ
	m_Readbuf.resize(HEADER_SIZE); // ��� ũ��� ��������

	boost::asio::async_read(m_Socket,
		boost::asio::buffer(m_Readbuf),
		[this](const boost::system::error_code& err, const size_t size) {
			if (!err) {
				size_t bodySize = 0;
				for (int i = 0; i < 4; ++i) {
					bodySize += (m_Readbuf[3 - i] << (8 * i)); // �ٵ� ������ ���
				}
				ReadBody(bodySize); // �ٵ� �б� ȣ��
			}
			else {
				HandleError("[SERVER] Read Header Error!!\n" + err.message()); // ���� ó��: ��� �б� ����
			}
		});
}

void UserSession::ReadBody(size_t bodySize)
{
	m_Readbuf.clear(); // ���� �ʱ�ȭ
	m_Readbuf.resize(bodySize); // �ٵ� ũ��� ��������

	boost::asio::async_read(m_Socket,
		boost::asio::buffer(m_Readbuf),
		[this](std::error_code ec, std::size_t size) {
			if (!ec) {
				std::shared_ptr<myChatMessage::ChatMessage> chatMessage = std::make_shared<myChatMessage::ChatMessage>(); // ä�� �޽��� ����
				if (chatMessage->ParseFromArray(m_Readbuf.data(), static_cast<int>(size))) { // �迭���� �Ľ�
					chatMessage->set_sender(m_UserEntity->GetUserId()); // �߽��� ����
					m_InputQueue->push(chatMessage); // �Է� ť�� ����
				}
				ReadHeader(); // ��� �б� ȣ��
			}
			else {
				HandleError("[SERVER] Read Body Error!!"); // ���� ó��: �ٵ� �б� ����
			}
		});
}