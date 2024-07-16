#pragma once
#include "Party/include/PartyManager.h"

PartyManager::PartyManager()
{
}

PartyManager::~PartyManager()
{
    m_MapParties.clear(); // 모든 파티 맵 초기화
}

std::shared_ptr<Party> PartyManager::CreateParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    if (user == nullptr || partyName == "")
    {
        std::cout << "[SERVER] Invalid Party Name" << std::endl;
        return nullptr;
    }

    auto party = std::make_shared<Party>(m_PartyIdCounter++, user->GetId(), partyName); // 파티 생성
    party->AddMember(user->GetId());                                                    // 파티 생성자를 파티 멤버로 추가
    m_MapParties[party->GetId()] = party;                                               // 파티를 맵에 추가

    std::cout << "Party Count : " << m_MapParties.size() << "\n";
    return party;
}

std::shared_ptr<Party> PartyManager::JoinParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    auto party = FindPartyByName(partyName);
    if (!party)
    {
        std::cout << "[SERVER] Not found party: " << partyName << std::endl;
        return nullptr;
    }

    party->AddMember(user->GetId()); // 파티에 사용자 추가
    return party;
}

uint32_t PartyManager::DeleteParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    // 파티 이름으로 파티를 찾음
    auto party = FindPartyByName(partyName);
    if (!party)
    {
        std::cout << "[SERVER] Not found party: " << partyName << std::endl;
        return 0;
    }

    auto partyId = party->GetId();

    // 파티 생성자인 경우에만 파티 삭제 가능
    if (party->GetPartyCreator() == user->GetId())
    {
        std::cout << "Attempting to delete party with ID: " << partyId << std::endl;

        size_t erasedCount = m_MapParties.erase(partyId);
        std::cout << "Number of parties erased: " << erasedCount << std::endl;

        if (erasedCount == 0)
        {
            std::cout << "[SERVER] Failed to delete party with ID: " << partyId << ". ID not found in m_MapParties." << std::endl;
            return 0;
        }

        std::cout << "[SERVER] Deleted party: " << partyName << std::endl;
    }
    else
    {
        std::cout << "[SERVER] Fail Deleted Party : " << partyName << std::endl;
        return 0;
    }

    std::cout << "Party Count : " << m_MapParties.size() << "\n";
    return partyId;
}

bool PartyManager::LeaveParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    auto party = FindPartyByName(partyName);
    if (!party)
    {
        std::cout << "[SERVER] Not found party: " << partyName << std::endl;
        return false;
    }

    // 파티 생성자는 파티를 떠날 수 없음
    if (party->GetPartyCreator() == user->GetId())
    {
        std::cout << "[SERVER] Sorry, as the party leader, you cannot leave the party. Deletion is the only option." << std::endl;
        return false;
    }

    party->RemoveMember(user->GetId()); // 파티 멤버에서 사용자 제거
    return true;
}

bool PartyManager::HasParty(uint32_t partyId)
{
    return m_MapParties.find(partyId) != m_MapParties.end();
}

std::shared_ptr<Party> PartyManager::FindPartyById(uint32_t partyId)
{
    auto it = m_MapParties.find(partyId);
    if (it != m_MapParties.end())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}

std::shared_ptr<Party> PartyManager::FindPartyByName(const std::string& partyName)
{
    for (const auto& entry : m_MapParties)
    {
        if (entry.second->GetName() == partyName)
        {
            return entry.second;
        }
    }
    return nullptr;
}

bool PartyManager::IsPartyNameTaken(const std::string& partyName)
{
    for (const auto& pair : m_MapParties)
    {
        if (pair.second->GetName() == partyName)
        {
            return true;
        }
    }
    return false;
}
