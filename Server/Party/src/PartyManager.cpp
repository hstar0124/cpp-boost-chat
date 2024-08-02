#pragma once
#include "Party/include/PartyManager.h"
#include <Util/HsLogger.hpp>

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
        LOG_ERROR("Invalid Party Name");
        return nullptr;
    }

    auto party = std::make_shared<Party>(m_PartyIdCounter++, user->GetId(), partyName); // 파티 생성
    party->AddMember(user->GetId());                                                    // 파티 생성자를 파티 멤버로 추가
    m_MapParties[party->GetId()] = party;                                               // 파티를 맵에 추가

    LOG_INFO("Party Count : %zu", m_MapParties.size());
    return party;
}

std::shared_ptr<Party> PartyManager::JoinParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    auto party = FindPartyByName(partyName);
    if (!party)
    {
        LOG_INFO("Not found party: %s", partyName.c_str());
        return nullptr;
    }

    party->AddMember(user->GetId()); // 파티에 사용자 추가
    LOG_INFO("User %u joined party %s", user->GetId(), partyName.c_str());
    return party;
}

uint32_t PartyManager::DeleteParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    // 파티 이름으로 파티를 찾음
    auto party = FindPartyByName(partyName);
    if (!party)
    {
        std::cout << "[SERVER] Not found party: " << partyName << std::endl;
        LOG_INFO("Not found party: %s", partyName.c_str());
        return 0;
    }

    auto partyId = party->GetId();

    // 파티 생성자인 경우에만 파티 삭제 가능
    if (party->GetPartyCreator() == user->GetId())
    {
        LOG_INFO("Attempting to delete party with ID: %u", partyId);

        size_t erasedCount = m_MapParties.erase(partyId);
        LOG_INFO("Number of parties erased: %zu", erasedCount);

        if (erasedCount == 0)
        {
            LOG_ERROR("Failed to delete party with ID: %u. ID not found in m_MapParties.", partyId);
            return 0;
        }

        LOG_INFO("[SERVER] Deleted party: %s", partyName.c_str());
    }
    else
    {
        LOG_WARN("[SERVER] Fail Deleted Party : %s", partyName.c_str());
        return 0;
    }

    LOG_INFO("Party Count : %zu", m_MapParties.size());
    return partyId;
}

bool PartyManager::LeaveParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    auto party = FindPartyByName(partyName);
    if (!party)
    {
        LOG_INFO("[SERVER] Not found party: %s", partyName.c_str());
        return false;
    }

    // 파티 생성자는 파티를 떠날 수 없음
    if (party->GetPartyCreator() == user->GetId())
    {
        LOG_WARN("[SERVER] Sorry, as the party leader, you cannot leave the party. Deletion is the only option.");
        return false;
    }

    party->RemoveMember(user->GetId()); // 파티 멤버에서 사용자 제거
    LOG_INFO("User %u left the party %s", user->GetId(), partyName.c_str());
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