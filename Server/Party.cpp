#include "Party.h"

Party::Party(uint32_t partyId, uint32_t creator, const std::string& partyName) :
m_PartyId(partyId)
, m_PartyCreator(creator)
, m_PartyName(partyName)
{}

bool Party::AddMember(uint32_t sessionId)
{
    m_Members.push_back(sessionId);
    std::cout << "[SERVER] Add Member in Party {" << m_PartyName << ", " << sessionId << "}" << std::endl;
    PrintMembers();
    return true;
}

bool Party::RemoveMember(uint32_t sessionId)
{
    m_Members.erase(std::remove_if(m_Members.begin(), m_Members.end(),
        [&](uint32_t sid)
        {
            return sid == sessionId;
        }),
        m_Members.end());
    
    std::cout << "[SERVER] Remove Member in Party {" << m_PartyName << ", " << sessionId << "}" << std::endl;
    PrintMembers();
    return true;
}

void Party::PrintMembers() const
{
    std::cout << "Member List : ";
    for (auto it = m_Members.begin(); it != m_Members.end(); it++)
    {
        std::cout << *it << " ";
    }
    std::cout << "\n";
}


const std::vector<uint32_t>& Party::GetMembers() const
{
    return m_Members;
}

bool Party::HasMember(uint32_t sessionId) const
{
    return std::any_of(m_Members.begin(), m_Members.end(),
        [&](uint32_t sid)
        {
            return sid == sessionId;
        });
}
