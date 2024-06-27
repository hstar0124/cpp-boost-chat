#pragma once
#include "Common.h"

class UserEntity
{
private:
    uint64_t        m_Id;       // 유저 식별키
    std::string     m_UserId;   // 유저 아이디
    std::string     m_Password; // 유저 패스워드
    std::string     m_Username; // 유저 이름
    std::string     m_Email;    // 유저 이메일
    char     m_IsAlive;  // 유저 삭제 유무

public:

    // Getter methods
    uint64_t GetId() const { return m_Id; }
    std::string GetUserId() const { return m_UserId; }
    std::string GetPassword() const { return m_Password; }
    std::string GetUsername() const { return m_Username; }
    std::string GetEmail() const { return m_Email; }
    char GetIsAlive() const { return m_IsAlive; }

    // Setter methods
    void SetId(uint64_t id) { m_Id = id; }
    void SetUserId(const std::string& userId) { m_UserId = userId; }
    void SetPassword(const std::string& password) { m_Password = password; }
    void SetUsername(const std::string& username) { m_Username = username; }
    void SetEmail(const std::string& email) { m_Email = email; }
    void SetIsAlive(const char& isAlive) { m_IsAlive = isAlive; }
};