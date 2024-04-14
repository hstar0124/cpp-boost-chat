#include "TcpSession.h"

void TcpSession::Start()
{
    ReadHeader();
}

boost::asio::ip::tcp::socket &TcpSession::GetSocket()
{
    return m_Socket;
}

void TcpSession::Send(std::shared_ptr<myPayload::Payload> msg)
{
    boost::asio::post(*m_IoContext,
        [this, msg]()
        {
            bool bWritingMessage = !m_QMessageOutServer.Empty();
            m_QMessageOutServer.PushBack(msg);
            if (!bWritingMessage)
                AsyncWrite();
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

void TcpSession::AsyncWrite()
{
    auto payload = m_QMessageOutServer.Front();

    int size = static_cast<int>(payload->ByteSizeLong());

    std::cout << "Message Size : " << payload->ByteSizeLong() << "\n";

    m_Writebuf.clear();
    m_Writebuf.resize(HEADER_SIZE + size);

    payload->SerializePartialToArray(m_Writebuf.data() + HEADER_SIZE, size);

    int bodySize = static_cast<int>(m_Writebuf.size()) - HEADER_SIZE;
    m_Writebuf[0] = static_cast<uint8_t>((bodySize >> 24) & 0xFF);
    m_Writebuf[1] = static_cast<uint8_t>((bodySize >> 16) & 0xFF);
    m_Writebuf[2] = static_cast<uint8_t>((bodySize >> 8) & 0xFF);
    m_Writebuf[3] = static_cast<uint8_t>(bodySize & 0xFF);

    std::cout << "buffer Size : " << m_Writebuf.size() << "\n";

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
    }
}


void TcpSession::ReadHeader()
{
    m_Readbuf.clear();
    m_Readbuf.resize(HEADER_SIZE);
    std::cout << "ReadHeader : " << m_Readbuf.size() << "\n";

    boost::asio::async_read(m_Socket, 
        boost::asio::buffer(m_Readbuf),
        [this](const boost::system::error_code& err, const size_t size)
        {
            if (!err)
            {
                size_t body_size = 0;
                for (int j = 0; j < m_Readbuf.size(); j++) {
                    body_size = static_cast<size_t>(m_Readbuf[j]);
                }

                ReadBody(body_size);

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

void TcpSession::ReadBody(size_t body_size)
{
    m_Readbuf.clear();
    m_Readbuf.resize(body_size);
    std::cout << "ReadBody : " << m_Readbuf.size() << "\n";

    boost::asio::async_read(m_Socket,
        boost::asio::buffer(m_Readbuf),
        [this](std::error_code ec, std::size_t size)
        {
            if (!ec)
            {

                std::shared_ptr<myPayload::Payload> payload = std::make_shared<myPayload::Payload>();

                // m_readbuf를 Payload 메시지로 디시리얼라이즈
                if (payload->ParseFromArray(m_Readbuf.data(), m_Readbuf.size()))
                {
                    // Payload 메시지에서 필요한 데이터를 출력
                    std::cout << "Payload Type: " << payload->payloadtype() << std::endl;
                    std::cout << "Content: " << payload->content() << std::endl;

                    // 메시지를 수신 큐에 추가합니다.
                    AddToIncomingMessageQueue(payload);
                }
            }
            else
            {
                std::cout << "[SERVER] Read Body Fail.\n";
                m_Socket.close();
            }
        });
}

// 완전한 메시지를 받으면 수신 큐에 추가합니다.
void TcpSession::AddToIncomingMessageQueue(std::shared_ptr<myPayload::Payload> payload)
{
    m_QMessagesInServer.PushBack({ this->shared_from_this(), payload });
    ReadHeader();
}