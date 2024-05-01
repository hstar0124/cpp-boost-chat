#pragma once
#include "Common.h"
#include "TcpSession.h"

class Party : public std::enable_shared_from_this<Party>
{
public:
    Party(uint32_t partyId, uint32_t creator, const std::string& partyName) : 
          m_PartyId(partyId)
        , m_PartyCreator(creator)
        , m_PartyName(partyName)
    {}

    uint32_t GetId() const { return m_PartyId; }
    uint32_t GetPartyCreator() const { return m_PartyCreator; }
    std::string GetName() const { return m_PartyName; }
    bool SetPartyName(const std::string& partyName) { m_PartyName = partyName; }

    const std::vector<std::weak_ptr<TcpSession>>& GetMembers() const;
    void AddMember(std::weak_ptr<TcpSession> session);
    bool HasMember(std::weak_ptr<TcpSession> session) const;
    void RemoveMember(std::weak_ptr<TcpSession> session);
    void Send(std::shared_ptr<myChatMessage::ChatMessage> msg);

private:
    uint32_t m_PartyId;
    std::string m_PartyName;
    uint32_t m_PartyCreator;
    std::vector<std::weak_ptr<TcpSession>> m_Members;
};