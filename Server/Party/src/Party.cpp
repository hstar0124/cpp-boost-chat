#include "Party/include/Party.h"
#include <Util/HsLogger.hpp>

Party::Party(uint32_t partyId, uint32_t creator, const std::string& partyName) :
    m_PartyId(partyId),
    m_PartyCreator(creator),
    m_PartyName(partyName)
{
    LOG_INFO("Party Created: ID=%d, Creator=%d, Name=%s", m_PartyId, m_PartyCreator, m_PartyName.c_str());
}

Party::~Party()
{
    LOG_INFO("Party %s is being destroyed. Cleaning up members.", m_PartyName.c_str());
    m_Members.clear(); // ��Ƽ ������� ��� �����Ͽ� ����
}

bool Party::AddMember(uint32_t userId)
{
    m_Members.push_back(userId); // ��Ƽ ��� �߰�
    LOG_INFO("Add Member in Party %s: %d", m_PartyName.c_str(), userId);
    PrintMembers(); // ���� ��Ƽ ��� ��� ���
    return true;
}

bool Party::RemoveMember(uint32_t userId)
{
    m_Members.erase(std::remove_if(m_Members.begin(), m_Members.end(),
        [&](uint32_t sid)
        {
            return sid == userId; // ���ǿ� �´� ��� ����
        }),
        m_Members.end());

    LOG_INFO("Remove Member in Party %s: %d", m_PartyName.c_str(), userId);
    PrintMembers(); // ���� ��Ƽ ��� ��� ���
    return true;
}

void Party::PrintMembers() const
{
    std::stringstream ss;
    ss << "Member List: ";
    for (auto it = m_Members.begin(); it != m_Members.end(); it++)
    {
        ss << *it << " "; // ��Ƽ ��� ��� ���
    }
    LOG_INFO("%s", ss.str().c_str());
}

const std::vector<uint32_t>& Party::GetMembers() const
{
    return m_Members; // ��Ƽ ������� const ���� ��ȯ
}

bool Party::HasMember(uint32_t sessionId) const
{
    return std::any_of(m_Members.begin(), m_Members.end(),
        [&](uint32_t sid)
        {
            return sid == sessionId; // Ư�� ����ڰ� ��Ƽ ������� Ȯ��
        });
}