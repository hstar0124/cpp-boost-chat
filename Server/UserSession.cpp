#include "UserSession.h"

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
	if (m_OutputQueue->empty() && !SwapQueues())
	{
		return nullptr;
	}

	auto msg = m_OutputQueue->front();
	if (!msg)
	{
		return nullptr;
	}

	m_OutputQueue->pop();
	return msg;
}

boost::asio::ip::tcp::socket& UserSession::GetSocket()
{
	return m_Socket;
}

bool UserSession::SwapQueues()
{
	std::lock_guard<std::mutex> lock(m_QueueMutex);
	if (m_OutputQueue->empty() && !m_InputQueue->empty())
	{
		std::swap(m_InputQueue, m_OutputQueue);
		return true;
	}

	return false;
}

void UserSession::StartPingTimer()
{
	m_PingTimer.expires_after(std::chrono::seconds(5)); // 5초마다 ping 전송
	m_PingTimer.async_wait([this](const boost::system::error_code& ec) 
	{
		if (!ec)
		{
			if (IsConnected())
			{
				SendPing();
				StartPingTimer(); // 타이머 재시작
			}
		}
		else
		{
			HandleError("[SERVER] DisConnected Client");
		}
	});
}

void UserSession::SendPing()
{
	auto pingMsg = std::make_shared<myChatMessage::ChatMessage>();
	pingMsg->set_messagetype(myChatMessage::ChatMessageType::SERVER_PING);
	pingMsg->set_content(GetCurrentTimeMilliseconds());
	Send(pingMsg);
}

std::string UserSession::GetCurrentTimeMilliseconds()
{
	auto now = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	return std::to_string(now_ms.time_since_epoch().count());
}

void UserSession::Send(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	boost::asio::post(m_IoContext,
	[this, msg]()
	{
		AsyncWrite(msg);
	});
}

void UserSession::AsyncWrite(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
	MessageConverter<myChatMessage::ChatMessage>::SerializeMessage(msg, m_Writebuf);
	MessageConverter<myChatMessage::ChatMessage>::SetSizeToBufferHeader(m_Writebuf);

	boost::asio::async_write(m_Socket, boost::asio::buffer(m_Writebuf.data(), m_Writebuf.size()),
		[this](const boost::system::error_code& err, const size_t transferred)
		{
			if (err)
			{
				HandleError("[SERVER] Write Error!!");
			}
		});
}

void UserSession::HandleError(const std::string& errorMessage)
{
	std::cout << errorMessage << "\n";
	Close();
}

void UserSession::ReadHeader()
{
	m_Readbuf.clear();
	m_Readbuf.resize(HEADER_SIZE);

	boost::asio::async_read(m_Socket,
		boost::asio::buffer(m_Readbuf),
		[this](const boost::system::error_code& err, const size_t size)
		{
			if (!err)
			{
				size_t bodySize = MessageConverter<myChatMessage::ChatMessage>::GetMessageBodySize(m_Readbuf);
				ReadBody(bodySize);
			}
			else
			{
				HandleError("[SERVER] Read Header Error!!\n" + err.message());
			}
		});
}

void UserSession::ReadBody(size_t bodySize)
{
	m_Readbuf.clear();
	m_Readbuf.resize(bodySize);

	boost::asio::async_read(m_Socket,
		boost::asio::buffer(m_Readbuf),
		[this](std::error_code ec, std::size_t size)
		{
			if (!ec)
			{

				std::shared_ptr<myChatMessage::ChatMessage> chatMessage = std::make_shared<myChatMessage::ChatMessage>();
				if (MessageConverter<myChatMessage::ChatMessage>::DeserializeMessage(m_Readbuf, chatMessage))
				{
					chatMessage->set_sender(m_UserEntity->GetUserId());
					//chatMessage->set_sender(std::to_string(m_Id));
					m_InputQueue->push(chatMessage);
				}

				ReadHeader();
			}
			else
			{
				HandleError("[SERVER] Read Body Error!!");
			}
		});
}
