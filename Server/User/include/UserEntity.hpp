#pragma once
#include "Common.h"

class UserEntity
{
private:
    uint32_t        m_Id;       // ����� ID
    std::string     m_UserId;   // ����� ���� ID
    std::string     m_Password; // ����� ��й�ȣ
    std::string     m_Username; // ����� �̸�
    std::string     m_Email;    // ����� �̸���
    char            m_IsAlive;  // ����� Ȱ�� ����

public:

    // Getter methods
    uint32_t GetId() const { return m_Id; } // ID ��ȯ
    std::string GetUserId() const { return m_UserId; } // ���� ID ��ȯ
    std::string GetPassword() const { return m_Password; } // ��й�ȣ ��ȯ
    std::string GetUsername() const { return m_Username; } // �̸� ��ȯ
    std::string GetEmail() const { return m_Email; } // �̸��� ��ȯ
    char GetIsAlive() const { return m_IsAlive; } // Ȱ�� ���� ��ȯ

    // Setter methods
    void SetId(uint32_t id) { m_Id = id; } // ID ����
    void SetUserId(const std::string& userId) { m_UserId = userId; } // ���� ID ����
    void SetPassword(const std::string& password) { m_Password = password; } // ��й�ȣ ����
    void SetUsername(const std::string& username) { m_Username = username; } // �̸� ����
    void SetEmail(const std::string& email) { m_Email = email; } // �̸��� ����
    void SetIsAlive(const char& isAlive) { m_IsAlive = isAlive; } // Ȱ�� ���� ����
};