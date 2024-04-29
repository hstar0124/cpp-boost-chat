#include "Party.h"

void Party::AddMember(std::weak_ptr<TcpSession> session)
{
    m_Members.push_back(session);
}

void Party::RemoveMember(std::weak_ptr<TcpSession> session)
{
    m_Members.erase(std::remove_if(m_Members.begin(), m_Members.end(),
        [&](const auto& weakSession) {
            return weakSession.expired() || weakSession.lock() == session.lock();
        }),
        m_Members.end());
}

const std::vector<std::weak_ptr<TcpSession>>& Party::GetMembers() const
{
    return m_Members;
}

bool Party::HasMember(std::weak_ptr<TcpSession> session) const
{
    return std::any_of(m_Members.begin(), m_Members.end(),
        [&](const auto& weakSession) {
            return !weakSession.expired() && weakSession.lock() == session.lock();
        });
}

void Party::Send(std::shared_ptr<myChatMessage::ChatMessage> msg)
{
    for (auto& weakMember : m_Members)
    {
        if (auto member = weakMember.lock())
        {
            std::cout << "Member : " << member->GetID() << "\n";
            member->Send(msg);
        }
    }
}