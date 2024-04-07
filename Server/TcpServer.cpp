#include "TcpServer.h"

TcpServer::TcpServer(boost::asio::io_context& io_context, int port)
    : m_Acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    m_IoContext(io_context)
{    
}


bool TcpServer::Start()
{
    try
    {
        WaitForClientConnection();
        m_ThreadContext = std::thread([this]() { m_IoContext.run(); });
    }
    catch (std::exception& e)
    {
        // 서버가 리스닝을 할 수 없는 경우
        std::cerr << "[SERVER] Exception: " << e.what() << "\n";
        return false;
    }

    std::cout << "[SERVER] Started!\n";
    return true;
}


void TcpServer::WaitForClientConnection()
{
    auto tcpSession = std::make_shared<TcpSession>(m_IoContext, m_QMessagesInServer);
    m_Acceptor.async_accept(tcpSession->GetSocket(),
        [this, tcpSession](const boost::system::error_code& err)
        {
            this->OnAccept(tcpSession, err);
        });
}

void TcpServer::Update(size_t nMaxMessages = -1, bool bWait = false)
{
    // 큐가 비어 있는 경우 대기
    if (bWait && m_QMessagesInServer.Empty()) {        
        m_QMessagesInServer.Wait();
    }

    // 메시지를 처리하기 전에 큐가 비어 있는지 확인
    if (!m_QMessagesInServer.Empty()) {
        size_t nMessageCount = 0;
        while (nMessageCount < nMaxMessages && !m_QMessagesInServer.Empty())
        {
            auto msg = m_QMessagesInServer.Pop();
            SendAllClients(msg);
            nMessageCount++;
        }
    }
}


void TcpServer::OnAccept(std::shared_ptr<TcpSession> tcpSession, const boost::system::error_code& err)
{
    if (!err)
    {
        // TODO : 클라이언트 유효성 체크        
        m_VecTcpSessions.push_back(std::move(tcpSession));
        m_VecTcpSessions.back()->Start();
        std::cout << "[SERVER] New Connection Approved!!\n";    
    }
    else
    {
        std::cout << "[SERVER] Error " << err.message() << std::endl;
    }

    WaitForClientConnection();
}


void TcpServer::SendAllClients(char* message)
{
    std::cout << "[SERVER] Send Msg for All Clients : " << message << std::endl;
    for (auto session : m_VecTcpSessions)
    {
        if (session && session->IsConnected())
        {
            session->Send(message);
        }
        else
        {
            // TODO : 세션 연결 해제 처리
            // 이더레이터가 꼬일수 있으니 for문 바깥에서 처리
        }      
    }
}

