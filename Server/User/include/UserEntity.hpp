#pragma once
#include "Common.h"

class UserEntity
{
private:
    uint32_t        m_Id;       // 사용자 ID
    std::string     m_UserId;   // 사용자 고유 ID
    std::string     m_Password; // 사용자 비밀번호
    std::string     m_Username; // 사용자 이름
    std::string     m_Email;    // 사용자 이메일
    char            m_IsAlive;  // 사용자 활성 상태

public:

    // Getter methods
    uint32_t GetId() const { return m_Id; } // ID 반환
    std::string GetUserId() const { return m_UserId; } // 고유 ID 반환
    std::string GetPassword() const { return m_Password; } // 비밀번호 반환
    std::string GetUsername() const { return m_Username; } // 이름 반환
    std::string GetEmail() const { return m_Email; } // 이메일 반환
    char GetIsAlive() const { return m_IsAlive; } // 활성 상태 반환

    // Setter methods
    void SetId(uint32_t id) { m_Id = id; } // ID 설정
    void SetUserId(const std::string& userId) { m_UserId = userId; } // 고유 ID 설정
    void SetPassword(const std::string& password) { m_Password = password; } // 비밀번호 설정
    void SetUsername(const std::string& username) { m_Username = username; } // 이름 설정
    void SetEmail(const std::string& email) { m_Email = email; } // 이메일 설정
    void SetIsAlive(const char& isAlive) { m_IsAlive = isAlive; } // 활성 상태 설정
};
