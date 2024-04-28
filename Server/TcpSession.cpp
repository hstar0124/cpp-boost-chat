#include "TcpSession.h"

void TcpSession::Start(uint32_t uid = 0)
{
    m_Id = uid;
    StartPingTimer();
    ReadHeader();
}

boost::asio::ip::tcp::socket &TcpSession::GetSocket()
{
    return m_Socket;
}

void TcpSession::Send(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    boost::asio::post(m_IoContext,
        [this, msg]()
        {
            bool bWritingMessage = !m_QMessageOutServer.Empty();
            m_QMessageOutServer.PushBack(msg);
            if (!bWritingMessage)
                AsyncWrite();
        });
}

void TcpSession::SendPing()
{
    // 현재 시간을 milliseconds로 얻어옴
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch().count();

    // Payload 메시지 생성 및 시간 정보 설정
    auto pingMsg = std::make_shared<myChatMessage::ChatMessage>();
    pingMsg->set_messagetype(myChatMessage::ChatMessageType::SERVER_PING);
    pingMsg->set_content(std::to_string(value)); // 밀리초로 변환하여 content에 설정
    Send(pingMsg);
    m_IsActive = false;
}


void TcpSession::StartPingTimer()
{
    
    m_PingTimer.expires_after(std::chrono::seconds(5)); // 5초마다 ping 전송
    m_PingTimer.async_wait([this](const boost::system::error_code& ec) {

        if (!ec)
        {
            if (IsConnected())
            {
                // 클라이언트가 활성 상태인 경우에만 ping 전송
                SendPing();
                StartPingTimer(); // 타이머 재시작
            }
        }
        else
        {
            // 에러 처리
            std::cout << "[SERVER] DisConnected Client.\n";
            m_IsActive = false;
            Close();
        }

    });
}


void TcpSession::Close()
{
    m_Socket.close();
}

bool TcpSession::IsConnected()
{
    return m_Socket.is_open();
}

uint32_t TcpSession::GetID() const
{
    return m_Id;
}


void TcpSession::AsyncWrite()
{
    auto payload = m_QMessageOutServer.Front();
    PacketConverter<myChatMessage::ChatMessage>::SerializePayload(payload, m_Writebuf);
    PacketConverter<myChatMessage::ChatMessage>::SetSizeToBufferHeader(m_Writebuf);

    boost::asio::async_write(m_Socket, boost::asio::buffer(m_Writebuf.data(), m_Writebuf.size()),
        [this](const boost::system::error_code& err, const size_t transferred)
        {
            this->OnWrite(err, transferred);
        });
}

void TcpSession::OnWrite(const boost::system::error_code& err, const size_t size)
{
    if (!err)
    {
        m_QMessageOutServer.Pop();
        if (!m_QMessageOutServer.Empty())
        {
            AsyncWrite();
        }
    }
    else
    {
        std::cout << err.message() << std::endl;
        Close();
    }
}


void TcpSession::ReadHeader()
{
    m_Readbuf.clear();
    m_Readbuf.resize(HEADER_SIZE);

    boost::asio::async_read(m_Socket, 
        boost::asio::buffer(m_Readbuf),
        [this](const boost::system::error_code& err, const size_t size)
        {
            if (!err)
            {
                size_t bodySize = PacketConverter<myChatMessage::ChatMessage>::GetPayloadBodySize(m_Readbuf);
                ReadBody(bodySize);
            }
            else
            {
                // 클라이언트로부터의 읽기가 실패하는 경우
                // 소켓을 닫고 시스템이 이후 정리하도록 합니다.
                std::cout << "[SERVER] DisConnected Client.\n";
                Close();
            }
        });
}

void TcpSession::ReadBody(size_t bodySize)
{
    m_Readbuf.clear();
    m_Readbuf.resize(bodySize);

    boost::asio::async_read(m_Socket,
        boost::asio::buffer(m_Readbuf),
        [this](std::error_code ec, std::size_t size)
        {
            if (!ec)
            {

                std::shared_ptr<myChatMessage::ChatMessage> payload = std::make_shared<myChatMessage::ChatMessage>();

                // m_readbuf를 Payload 메시지로 디시리얼라이즈
                if (PacketConverter<myChatMessage::ChatMessage>::DeserializePayload(m_Readbuf, payload))
                {
                    // Payload 메시지에서 필요한 데이터를 출력
                    //std::cout << "Payload Type: " << payload->payloadtype() << std::endl;
                    //std::cout << "Content: " << payload->content() << std::endl;
                    payload->set_sender(std::to_string(m_Id));
                    // 메시지를 수신 큐에 추가합니다.
                    AddToIncomingMessageQueue(payload);
                }
            }
            else
            {
                std::cout << "[SERVER] DisConnected Client.\n";
                Close();
            }
        });
}

// 완전한 메시지를 받으면 수신 큐에 추가합니다.
void TcpSession::AddToIncomingMessageQueue(std::shared_ptr<myChatMessage::ChatMessage> message)
{
    m_QMessagesInServer.PushBack({ this->shared_from_this(), message });
    ReadHeader();
}