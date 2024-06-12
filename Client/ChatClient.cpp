#include "ChatClient.h"

ChatClient::ChatClient(boost::asio::io_context& io_context)
	: m_IoContext(io_context), m_Socket(io_context)
{
}

ChatClient::~ChatClient()
{
	m_Socket.close();
}

void ChatClient::Connect(std::string host, int port)
{
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(host), port);
	m_Endpoint = endpoint;

	std::cout << "[CLIENT] Try Connect!! Waiting......" << std::endl;

	m_Socket.async_connect(endpoint, [this](const boost::system::error_code& err)
		{
			this->OnConnect(err);
		});

}

void ChatClient::Send(const std::string& userInput)
{
	std::shared_ptr<myChatMessage::ChatMessage> chatMessage = std::make_shared<myChatMessage::ChatMessage>();

	// 메시지 성공 플래그 값
	bool isSuccess = false;

	if (IsWhisperMessage(userInput))
	{
		isSuccess = CreateWhisperMessage(chatMessage, userInput);
	}
	else if (IsPartyMessage(userInput))
	{
		isSuccess = CreatePartyMessage(chatMessage, userInput);
	}
	else
	{
		isSuccess = CreateNormalMessage(chatMessage, userInput);
	}

	if (isSuccess)
	{
		AsyncWrite(chatMessage);
	}
}

void ChatClient::OnConnect(const boost::system::error_code& err)
{
	if (!err)
	{
		ReadHeader();
	}
	else
	{
		std::cout << "[CLIENT] ERROR : " << err.message() << "\n";
	}
}

bool ChatClient::IsWhisperMessage(const std::string& userInput)
{
	return userInput.substr(0, 2) == "/w";
}

bool ChatClient::CreateWhisperMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput)
{
	size_t spacePos = userInput.find(' ');
	size_t receiverEndPos = userInput.find(' ', spacePos + 1);

	if (spacePos == std::string::npos || userInput.size() <= spacePos + 1 || receiverEndPos == std::string::npos)
	{
		std::cout << "[CLIENT] Invalid message format. Format: /w [recipient] [message]\n";
		return false;
	}

	std::string receiver = userInput.substr(spacePos + 1, receiverEndPos - spacePos - 1);
	std::string message = userInput.substr(receiverEndPos + 1);

	chatMessage->set_messagetype(myChatMessage::ChatMessageType::WHISPER_MESSAGE);
	chatMessage->set_receiver(receiver);
	chatMessage->set_content(message);

	return true;
}

bool ChatClient::IsPartyMessage(const std::string& userInput)
{
	return userInput.substr(0, 3) == "/p ";
}

std::pair<std::string, std::string> ChatClient::ExtractOptionAndPartyName(const std::string& userInput)
{

	if (userInput.size() < 3)
	{
		return std::make_pair("Error", "");
	}

	size_t spacePos = userInput.find(' ');
	if (spacePos == std::string::npos)
	{
		return std::make_pair("", "");
	}

	std::string option;
	std::string partyName;

	if (userInput.substr(0, 3) == "/p ")
	{
		// 3번째 위치부터 검색
		size_t optionPos = userInput.find('-', 3);
		if (optionPos != std::string::npos)
		{
			size_t optionEndPos = userInput.find(' ', optionPos + 1);
			if (optionEndPos != std::string::npos)
			{
				option = userInput.substr(optionPos, optionEndPos - optionPos);
				partyName = userInput.substr(optionEndPos + 1);
			}
			else
			{
				return std::make_pair("Error", "");
			}
		}
	}
	else
	{
		partyName = userInput.substr(spacePos + 1);
	}

	return std::make_pair(option, partyName);
}

bool ChatClient::CreatePartyMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput)
{
	std::pair<std::string, std::string> optionAndPartyName = ExtractOptionAndPartyName(userInput);
	std::string& option = optionAndPartyName.first;
	std::string& partyName = optionAndPartyName.second;

	//std::cout << "option : " << option << "\n";
	//std::cout << "partyName : " << partyName << "\n";

	if (option.empty())
	{
		// 파티 메시지의 경우
		chatMessage->set_messagetype(myChatMessage::ChatMessageType::PARTY_MESSAGE);
		chatMessage->set_content(userInput.substr(3)); // Remove "/p "
		return true;
	}


	if (option == "-create" || option == "-delete" || option == "-join" || option == "-leave")
	{
		if (partyName.empty() || partyName == "")
		{
			std::cout << "[CLIENT] Invalid message format. Format: /p -create [party name]\n";
			return false;
		}

		if (option == "-create")
		{
			chatMessage->set_messagetype(myChatMessage::ChatMessageType::PARTY_CREATE);
		}
		else if (option == "-delete")
		{
			chatMessage->set_messagetype(myChatMessage::ChatMessageType::PARTY_DELETE);
		}
		else if (option == "-join")
		{
			chatMessage->set_messagetype(myChatMessage::ChatMessageType::PARTY_JOIN);
		}
		else if (option == "-leave")
		{
			chatMessage->set_messagetype(myChatMessage::ChatMessageType::PARTY_LEAVE);
		}

		chatMessage->set_content(partyName);
	}
	else
	{
		std::cout << "[CLIENT] Invalid message format. Format: /p [-create | -delete | -join | -leave] [party name]\n";
		return false;
	}

	//std::cout << "MSG : " << chatMessage->messagetype() << "\n";
	//std::cout << "Content : " << chatMessage->content() << "\n";
	return true;
}

bool ChatClient::CreateNormalMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput)
{

	chatMessage->set_messagetype(myChatMessage::ChatMessageType::ALL_MESSAGE);
	chatMessage->set_content(userInput);

	return true;
}

void ChatClient::SendPong()
{
	auto now = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	auto value = now_ms.time_since_epoch().count();

	std::shared_ptr<myChatMessage::ChatMessage> message = std::make_shared<myChatMessage::ChatMessage>();
	message->set_messagetype(myChatMessage::ChatMessageType::SERVER_PING);
	message->set_content(std::to_string(value));

	AsyncWrite(message);
}

void ChatClient::AsyncWrite(std::shared_ptr<myChatMessage::ChatMessage> message)
{
	int size = static_cast<int>(message->ByteSizeLong());

	std::vector<uint8_t> buffer;
	buffer.resize(HEADER_SIZE + size);

	message->SerializePartialToArray(buffer.data() + HEADER_SIZE, size);

	int bodySize = static_cast<int>(buffer.size()) - HEADER_SIZE;
	buffer[0] = static_cast<uint8_t>((bodySize >> 24) & 0xFF);
	buffer[1] = static_cast<uint8_t>((bodySize >> 16) & 0xFF);
	buffer[2] = static_cast<uint8_t>((bodySize >> 8) & 0xFF);
	buffer[3] = static_cast<uint8_t>(bodySize & 0xFF);

	boost::asio::async_write(m_Socket, boost::asio::buffer(buffer.data(), buffer.size()),
		[this](const boost::system::error_code& err, const size_t size)
		{
			this->OnWrite(err, size);
		});
}

void ChatClient::OnWrite(const boost::system::error_code& err, const size_t size)
{
}

void ChatClient::ReadHeader()
{
	m_Readbuf.clear();
	m_Readbuf.resize(HEADER_SIZE);

	boost::asio::async_read(m_Socket, boost::asio::buffer(m_Readbuf, HEADER_SIZE),
		[this](std::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				size_t bodySize = 0;
				for (int j = 0; j < m_Readbuf.size(); j++)
				{
					bodySize = static_cast<size_t>(m_Readbuf[j]);
				}
				if (bodySize > 0)
				{
					ReadBody(bodySize);
				}
			}
			else
			{
				// 소켓으로부터 읽어오는 것을 실패한다면
				// 소켓을 닫는다.
				std::cout << "[CLIENT] Read Header Fail.\n";
				m_Socket.close();
			}
		});
}

void ChatClient::ReadBody(size_t bodySize)
{
	m_Readbuf.clear();
	m_Readbuf.resize(bodySize);

	boost::asio::async_read(m_Socket, boost::asio::buffer(m_Readbuf),
		[this](std::error_code ec, std::size_t size)
		{
			if (!ec)
			{
				// 메시지 객체를 만듬
				std::shared_ptr<myChatMessage::ChatMessage> chatMessage = std::make_shared<myChatMessage::ChatMessage>();

				// 메시지 버퍼로 디시리얼라이즈
				if (chatMessage->ParseFromArray(m_Readbuf.data(), size))
				{
					OnMessage(chatMessage);
					ReadHeader();
				}
				else
				{
					// 디시리얼라이즈 실패
					std::cerr << "[CLIENT] Failed to parse message" << std::endl;
				}

			}
			else
			{
				std::cout << "[CLIENT] Read Body Fail.\n";
				m_Socket.close();
			}
		});
}

void ChatClient::OnMessage(std::shared_ptr<myChatMessage::ChatMessage>& message)
{
	//std::cout << "Payload Type: " << payload->payloadtype() << std::endl;
	if (message->messagetype() == myChatMessage::ChatMessageType::ALL_MESSAGE)
	{
		std::cout << "[" << message->sender() << "] " << message->content() << std::endl;
		return;
	}
	else if (message->messagetype() == myChatMessage::ChatMessageType::SERVER_PING)
	{
		auto sentTimeMs = std::stoll(message->content());
		auto sentTime = std::chrono::milliseconds(sentTimeMs);
		auto currentTime = std::chrono::system_clock::now().time_since_epoch();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);
		//std::cout << "Response time: " << elapsedTime.count() << "ms" << std::endl;
		SendPong();

		return;
	}
	else if (message->messagetype() == myChatMessage::ChatMessageType::SERVER_MESSAGE)
	{
		std::cout << "[SERVER] " << message->content() << std::endl;
		return;
	}
	else if (message->messagetype() == myChatMessage::ChatMessageType::WHISPER_MESSAGE)
	{
		std::cout << "<<" << message->sender() << ">> " << message->content() << std::endl;
		return;
	}
	else if (message->messagetype() == myChatMessage::ChatMessageType::PARTY_MESSAGE)
	{
		std::cout << "##" << message->sender() << "## " << message->content() << std::endl;
		return;
	}
	else if (message->messagetype() == myChatMessage::ChatMessageType::ERROR_MESSAGE)
	{
		std::cout << message->content() << std::endl;
		return;
	}
}