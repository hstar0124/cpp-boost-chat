#pragma once
#include "Party.h"
#include "User/include/UserSession.h"

class PartyManager
{
private:
	uint32_t									            m_PartyIdCounter = 100'000;
    std::unordered_map<uint32_t, std::shared_ptr<Party>>    m_MapParties;

public:
    PartyManager();
    ~PartyManager();

    std::shared_ptr<Party> CreateParty(std::shared_ptr<UserSession> user, const std::string& partyName);
    std::shared_ptr<Party> JoinParty(std::shared_ptr<UserSession> user, const std::string& partyName);
    uint32_t DeleteParty(std::shared_ptr<UserSession> user, const std::string& partyName);
    bool LeaveParty(std::shared_ptr<UserSession> user, const std::string& partyName);
    bool HasParty(uint32_t partyId);
    std::shared_ptr<Party> FindPartyById(uint32_t partyId);
    std::shared_ptr<Party> FindPartyByName(const std::string& partyName); 
    bool IsPartyNameTaken(const std::string& partyName);
};

