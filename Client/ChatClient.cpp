#include "ChatClient.h"

ChatClient::ChatClient(boost::asio::io_context& io_context)
    : m_IoContext(io_context), m_Socket(io_context)
{}

ChatClient::~ChatClient() 
{
    m_Socket.close();
}

void ChatClient::Connect(const std::string& host, int port) 
{
    m_Endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(host), port);
    std::cout << "[CLIENT] Try Connect!! Waiting......" << std::endl;

    m_Socket.async_connect(m_Endpoint, [this](const boost::system::error_code& err) 
        {
            OnConnect(err);
        });
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
        exit(1);
    }
}

void ChatClient::Send(const std::string& userInput) 
{
    auto chatMessage = std::make_shared<myChatMessage::ChatMessage>();

    auto createMessageStrategy = GetCreateMessageStrategy(userInput);
    if (createMessageStrategy && createMessageStrategy(chatMessage, userInput)) 
    {
        AsyncWrite(chatMessage);
    }
}

bool ChatClient::SendSessionId(const std::string& sessionId)
{
    auto sessionMessage = std::make_shared<myChatMessage::ChatMessage>();

    sessionMessage->set_messagetype(myChatMessage::ChatMessageType::LOGIN_MESSAGE);
    sessionMessage->set_content(sessionId);
    
    AsyncWrite(sessionMessage);
    
    return true;
}

std::function<bool(std::shared_ptr<myChatMessage::ChatMessage>&, const std::string&)> ChatClient::GetCreateMessageStrategy(const std::string& userInput) 
{
    if (IsWhisperMessage(userInput)) 
    {
        return [this](std::shared_ptr<myChatMessage::ChatMessage>& msg, const std::string& input) 
            {
                return CreateWhisperMessage(msg, input);
            };
    }
    if (IsPartyMessage(userInput)) 
    {
        return [this](std::shared_ptr<myChatMessage::ChatMessage>& msg, const std::string& input) 
            {
                return CreatePartyMessage(msg, input);
            };
    }
    if (IsFriendRequestMessage(userInput)) 
    {
        return [this](std::shared_ptr<myChatMessage::ChatMessage>& msg, const std::string& input) 
            {
                return CreateFriendRequestMessage(msg, input);
            };
    }
    if (IsFriendAcceptMessage(userInput)) 
    {
        return [this](std::shared_ptr<myChatMessage::ChatMessage>& msg, const std::string& input) 
            {
                return CreateFriendAcceptMessage(msg, input);
            };
    }
    if (IsFriendRejectMessage(userInput))
    {
        return [this](std::shared_ptr<myChatMessage::ChatMessage>& msg, const std::string& input)
            {
                return CreateFriendRejectMessage(msg, input);
            };
    }
    return [this](std::shared_ptr<myChatMessage::ChatMessage>& msg, const std::string& input) 
        {
            return CreateNormalMessage(msg, input);
        };
}



bool ChatClient::IsWhisperMessage(const std::string& userInput) 
{
    return userInput.substr(0, 2) == "/w";
}

bool ChatClient::CreateWhisperMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput) 
{
    size_t spacePos = userInput.find(' ');
    size_t receiverEndPos = userInput.find(' ', spacePos + 1);

    if (spacePos == std::string::npos || receiverEndPos == std::string::npos) 
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

bool ChatClient::IsFriendRequestMessage(const std::string& userInput)
{
    return userInput.substr(0, 4) == "/fr ";
}

bool ChatClient::IsFriendAcceptMessage(const std::string& userInput)
{
    return userInput.substr(0, 4) == "/fa ";
}

bool ChatClient::IsFriendRejectMessage(const std::string& userInput)
{
    return userInput.substr(0, 4) == "/fj ";
}




std::pair<std::string, std::string> ChatClient::ExtractOptionAndPartyName(const std::string& userInput) 
{
    if (userInput.size() < 3) 
    {
        return { "Error", "" };
    }

    size_t spacePos = userInput.find(' ');
    if (spacePos == std::string::npos) 
    {
        return { "", "" };
    }

    std::string option;
    std::string partyName;

    if (userInput.substr(0, 3) == "/p ") 
    {
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
                return { "Error", "" };
            }
        }
    }
    else 
    {
        partyName = userInput.substr(spacePos + 1);
    }

    return { option, partyName };
}

bool ChatClient::CreatePartyMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput) 
{
    auto [option, partyName] = ExtractOptionAndPartyName(userInput);

    if (option.empty()) 
    {
        chatMessage->set_messagetype(myChatMessage::ChatMessageType::PARTY_MESSAGE);
        chatMessage->set_content(userInput.substr(3)); // Remove "/p "
        return true;
    }

    if (option == "-create" || option == "-delete" || option == "-join" || option == "-leave") 
    {
        if (partyName.empty()) 
        {
            std::cout << "[CLIENT] Invalid message format. Format: /p -create [party name]\n";
            return false;
        }

        chatMessage->set_messagetype(GetPartyMessageType(option));
        chatMessage->set_content(partyName);
        return true;
    }
    else 
    {
        std::cout << "[CLIENT] Invalid message format. Format: /p [-create | -delete | -join | -leave] [party name]\n";
        return false;
    }
}

myChatMessage::ChatMessageType ChatClient::GetPartyMessageType(const std::string& option) 
{
    if (option == "-create") return myChatMessage::ChatMessageType::PARTY_CREATE;
    if (option == "-delete") return myChatMessage::ChatMessageType::PARTY_DELETE;
    if (option == "-join") return myChatMessage::ChatMessageType::PARTY_JOIN;
    if (option == "-leave") return myChatMessage::ChatMessageType::PARTY_LEAVE;
    return myChatMessage::ChatMessageType::ERROR_MESSAGE;
}

bool ChatClient::CreateNormalMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput) 
{
    chatMessage->set_messagetype(myChatMessage::ChatMessageType::ALL_MESSAGE);
    chatMessage->set_content(userInput);
    return true;
}

bool ChatClient::CreateFriendRequestMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput)
{
    std::string friendId = userInput.substr(4); // Remove "/fr "
    chatMessage->set_messagetype(myChatMessage::ChatMessageType::FRIEND_REQUEST);
    chatMessage->set_content(friendId);

    return true;
}
bool ChatClient::CreateFriendAcceptMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput)
{
    std::string friendId = userInput.substr(4); // Remove "/fa "
    chatMessage->set_messagetype(myChatMessage::ChatMessageType::FRIEND_ACCEPT);
    chatMessage->set_content(friendId);

    return true;
}

bool ChatClient::CreateFriendRejectMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput)
{
    std::string friendId = userInput.substr(4); // Remove "/fj "
    chatMessage->set_messagetype(myChatMessage::ChatMessageType::FRIEND_REJECT);
    chatMessage->set_content(friendId);

    return true;
}


bool ChatClient::SendPong() 
{
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch().count();

    auto message = std::make_shared<myChatMessage::ChatMessage>();
    message->set_messagetype(myChatMessage::ChatMessageType::SERVER_PING);
    message->set_content(std::to_string(value));

    AsyncWrite(message);
    return true;
}

bool ChatClient::SendFriendRequest(const std::string& friendId)
{
    auto friendRequestMessage = std::make_shared<myChatMessage::ChatMessage>();
    friendRequestMessage->set_messagetype(myChatMessage::ChatMessageType::FRIEND_REQUEST);
    friendRequestMessage->set_content(friendId);

    AsyncWrite(friendRequestMessage);
    return true;
}

bool ChatClient::SendFriendAccept(const std::string& friendId)
{
    auto friendAcceptMessage = std::make_shared<myChatMessage::ChatMessage>();
    friendAcceptMessage->set_messagetype(myChatMessage::ChatMessageType::FRIEND_ACCEPT);
    friendAcceptMessage->set_content(friendId);

    AsyncWrite(friendAcceptMessage);
    return true;
}

bool ChatClient::GetVerified()
{
    return m_IsVerified.load();
}

void ChatClient::SetVerified(bool isVerified)
{
    m_IsVerified.store(isVerified);
    if (isVerified && m_VerificationCallback)
    {
        m_VerificationCallback();
    }
}

void ChatClient::SetVerificationCallback(const std::function<void()>& callback) 
{
    m_VerificationCallback = callback;
}

void ChatClient::AsyncWrite(std::shared_ptr<myChatMessage::ChatMessage> message) 
{
    int size = static_cast<int>(message->ByteSizeLong());
    m_Writebuf.resize(HEADER_SIZE + size);
    message->SerializePartialToArray(m_Writebuf.data() + HEADER_SIZE, size);

    int bodySize = static_cast<int>(m_Writebuf.size()) - HEADER_SIZE;
    for (int i = 0; i < 4; ++i) 
    {
        m_Writebuf[3 - i] = static_cast<uint8_t>((bodySize >> (8 * i)) & 0xFF);
    }

    boost::asio::async_write(m_Socket, boost::asio::buffer(m_Writebuf.data(), m_Writebuf.size()),
        [this](const boost::system::error_code& err, size_t size) 
        {
            OnWrite(err, size);
        });
}

void ChatClient::OnWrite(const boost::system::error_code& err, size_t size) {
    if (err) 
    {
        std::cerr << "Error sending message: " << err.message() << std::endl;
    }
}

void ChatClient::ReadHeader() 
{
    m_Readbuf.resize(HEADER_SIZE);

    boost::asio::async_read(m_Socket, boost::asio::buffer(m_Readbuf, HEADER_SIZE),
        [this](std::error_code ec, std::size_t length) 
        {
            if (!ec) 
            {
                size_t bodySize = 0;
                for (int j = 0; j < m_Readbuf.size(); ++j) 
                {
                    bodySize = (bodySize << 8) | static_cast<size_t>(m_Readbuf[j]);
                }
                if (bodySize > 0) 
                {
                    ReadBody(bodySize);
                }
            }
            else 
            {
                std::cout << "[CLIENT] Read Header Fail.\n" + ec.message();
                m_Socket.close();
            }
        });
}

void ChatClient::ReadBody(size_t bodySize) 
{
    m_Readbuf.resize(bodySize);

    boost::asio::async_read(m_Socket, boost::asio::buffer(m_Readbuf),
        [this](std::error_code ec, std::size_t size) 
        {
            if (!ec) 
            {
                auto chatMessage = std::make_shared<myChatMessage::ChatMessage>();
                if (chatMessage->ParseFromArray(m_Readbuf.data(), size)) 
                {
                    OnMessage(chatMessage);
                    ReadHeader();
                }
                else 
                {
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
    using MsgType = myChatMessage::ChatMessageType;
    switch (message->messagetype()) 
    {
    case MsgType::ALL_MESSAGE:
        std::cout << message->sender() << " >> " << message->content() << std::endl;
        break;
    case MsgType::SERVER_PING:
        HandleServerPing(message);
        break;
    case MsgType::LOGIN_MESSAGE:
        SetVerified(true);
        ClearScreen();
        std::cout << "[SERVER] Connect Success!!!!" << std::endl;
        break;
    case MsgType::SERVER_MESSAGE:
    case MsgType::ERROR_MESSAGE:
        std::cout << "[SERVER] " << message->content() << std::endl;
        break;
    case MsgType::WHISPER_MESSAGE:
        std::cout << "[WHISPER MESSAGE] " << message->sender() << " >> " << message->content() << std::endl;
        break;
    case MsgType::PARTY_MESSAGE:
        std::cout << "[PARTY MESSAGE] " << message->sender() << " >> " << message->content() << std::endl;
        break;
    case MsgType::FRIEND_REQUEST:
        std::cout << "[FRIEND REQUEST] " << message->sender() << " has sent you a friend request." << std::endl;
        break;
    case MsgType::FRIEND_ACCEPT:
        std::cout << "[FRIEND ACCEPT] " << message->sender() << " has accepted your friend request." << std::endl;
        break;
    default:
        std::cerr << "[CLIENT] Unknown message type received" << std::endl;
        break;
    }
}

void ChatClient::HandleServerPing(const std::shared_ptr<myChatMessage::ChatMessage>& message) 
{
    auto sentTimeMs = std::stoll(message->content());
    auto sentTime = std::chrono::milliseconds(sentTimeMs);
    auto currentTime = std::chrono::system_clock::now().time_since_epoch();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sentTime);
    //std::cout << "Response time: " << elapsedTime.count() << "ms" << std::endl;
    SendPong();
}

void ChatClient::ClearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}