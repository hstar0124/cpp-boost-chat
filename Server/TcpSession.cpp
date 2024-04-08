#include "TcpSession.h"

void TcpSession::Start()
{
    ReadHeader();
}

boost::asio::ip::tcp::socket& TcpSession::GetSocket()
{
    return m_Socket;
}

void TcpSession::Send(const Message& msg)
{
    boost::asio::post(m_IoContext,
        [this, msg]()
        {
            bool bWritingMessage = !m_QMessagesOutServer.Empty();
            m_QMessagesOutServer.PushBack(msg);
            if (!bWritingMessage)
                WriteHeader();
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



void TcpSession::WriteHeader()
{
    std::cout << "[SERVER] WriteHeader\n";
    boost::asio::async_write(m_Socket, boost::asio::buffer(&m_QMessagesOutServer.Front().header, sizeof(MessageHeader)), 
        [this](const boost::system::error_code& err, const size_t size)
        {
            // 에러 코드 없을 시
            if (!err)
            {
                // 오류가 없으므로 방금 보낸 메시지 헤더에
                // 메시지 본문도 있는지 확인
                if (m_QMessagesOutServer.Front().body.size() > 0)
                {
                    // 본문 바이트를 쓰는 작업을 실행
                    WriteBody();
                }
                else
                {
                    // 없으므로 이 메시지 처리가 완료되었습니다. 
                    // 보내는 메시지 큐에서 제거합니다.
                    m_QMessagesOutServer.Pop();

                    // 큐가 비어 있지 않으면 더 보낼 메시지가 있으므로
                    // 다음 헤더를 보내는 작업을 발행합니다.
                    if (!m_QMessagesOutServer.Empty())
                    {
                        WriteHeader();
                    }
                }
            }
            else
            {
                // 일단 단순히 연결이 종료되었다고 가정하고 소켓을 닫기
                std::cout << "[SERVER] Write Header Fail.\n";
                m_Socket.close();
            }
        });
}

void TcpSession::WriteBody()
{
    std::cout << "[SERVER] WriteBody\n";
    // 이 함수가 호출된 경우, 이미 헤더가 읽혔으며 해당 헤더가 본문을 읽으라고 요청
    // 해당 본문을 저장할 공간은 이미 임시 메시지 객체에 할당되어 있으므로 바이트가 도착할 때까지 대기
    boost::asio::async_write(m_Socket, boost::asio::buffer(m_QMessagesOutServer.Front().body.data(), m_QMessagesOutServer.Front().body.size()),
        [this](const boost::system::error_code& err, const size_t size)
        {
            if (!err)
            {
                m_QMessagesOutServer.Pop();

                // 큐에 여전히 메시지가 있는 경우, 다음 메시지의 헤더를 보내는 작업을 발행합니다.
                if (!m_QMessagesOutServer.Empty())
                {
                    WriteHeader();
                }
            }
            else
            {
                // 보내기 실패, WriteHeader()와 동일
                std::cout << "[SERVER] Write Body Fail.\n";
                m_Socket.close();
            }
        });
}

void TcpSession::ReadHeader()
{
    boost::asio::async_read(m_Socket, boost::asio::buffer(&m_TemporaryMessage.header, sizeof(MessageHeader)),
        [this](const boost::system::error_code& err, const size_t size)
        {
            if (!err)
            {
                // 메시지 헤더가 다 읽혔는지 확인
                if (m_TemporaryMessage.header.size > 0)
                {
                    // 메시지의 본문에 충분한 공간을 할당하고,
                    // asio에 본문을 읽을 작업을 실행
                    m_TemporaryMessage.body.resize(m_TemporaryMessage.header.size);
                    ReadBody();
                }
                else
                {
                    // 없으면, 이 본문 없는 메시지를 연결의 수신 메시지 큐에 추가합니다.
                    AddToIncomingMessageQueue();
                }
            }
            else
            {
                // 클라이언트로부터의 읽기가 실패하는 경우
                // 소켓을 닫고 시스템이 이후 정리하도록 합니다.
                std::cout << "[SERVER] Read Header Fail.\n";
                m_Socket.close();
            }
        });
}

void TcpSession::ReadBody()
{
    boost::asio::async_read(m_Socket,boost::asio::buffer(m_TemporaryMessage.body.data(), m_TemporaryMessage.body.size()),
        [this](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                // 메시지를 수신 큐에 추가합니다.
                AddToIncomingMessageQueue();
            }
            else
            {
                std::cout << "[SERVER] Read Body Fail.\n";
                m_Socket.close();
            }
        });
}

// 완전한 메시지를 받으면 수신 큐에 추가합니다.
void TcpSession::AddToIncomingMessageQueue()
{
    m_QMessagesInServer.PushBack({ this->shared_from_this(), m_TemporaryMessage });
    ReadHeader();
}