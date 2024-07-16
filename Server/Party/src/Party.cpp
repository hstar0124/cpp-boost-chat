#include "Party/include/Party.h"

Party::Party(uint32_t partyId, uint32_t creator, const std::string& partyName) :
    m_PartyId(partyId),
    m_PartyCreator(creator),
    m_PartyName(partyName)
{}

Party::~Party()
{
    std::cout << "[SERVER] Party {" << m_PartyName << "} is being destroyed. Cleaning up members." << std::endl;
    m_Members.clear(); // ��Ƽ ������� ��� �����Ͽ� ����
}

bool Party::AddMember(uint32_t userId)
{
    m_Members.push_back(userId); // ��Ƽ ��� �߰�
    std::cout << "[SERVER] Add Member in Party {" << m_PartyName << ", " << userId << "}" << std::endl;
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

    std::cout << "[SERVER] Remove Member in Party {" << m_PartyName << ", " << userId << "}" << std::endl;
    PrintMembers(); // ���� ��Ƽ ��� ��� ���
    return true;
}

void Party::PrintMembers() const
{
    std::cout << "Member List : ";
    for (auto it = m_Members.begin(); it != m_Members.end(); it++)
    {
        std::cout << *it << " "; // ��Ƽ ��� ��� ���
    }
    std::cout << "\n";
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