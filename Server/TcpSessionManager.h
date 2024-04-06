#pragma once
#include "Common.h"

class TcpSessionManager
{
public:
    static TcpSessionManager& getInstance() {
        static TcpSessionManager instance;
        return instance;
    }

    void Connection(std::shared_ptr<TcpSession> session)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        sessions.push_back(session);
    }

    void Disconnection(std::shared_ptr<TcpSession> session)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        sessions.erase(std::remove(sessions.begin(), sessions.end(), session), sessions.end());
    }

    void AllMessage(const char* message)
    {
        std::cout << "[TcpSessionManager] msg : " << message << std::endl;
        for (auto s : sessions)
        {
            s->Send(message);
        }
    }
    void DirectMessage();

private:
    TcpSessionManager() = default;
    // 객체는 유일하게 하나만 생성되어야 하기에 복사(대입), 이동(대입) 생성자 비활성화
    // 복사, 이동 생성자를 delete로 선언함으로서, 
    // 실수에 의한 복사, 이동 생성자 호출을 원천에 방지
    TcpSessionManager(const TcpSessionManager&) = delete;
    TcpSessionManager& operator=(const TcpSessionManager&) = delete;
    TcpSessionManager(TcpSessionManager&&) = delete;
    TcpSessionManager& operator=(TcpSessionManager&&) = delete;
    
    std::mutex m_Mutex;

    std::vector<std::shared_ptr<TcpSession>> sessions;
};