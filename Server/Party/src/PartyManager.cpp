#pragma once
#include "Party/include/PartyManager.h"
#include <Util/HsLogger.hpp>

PartyManager::PartyManager()
{
}

PartyManager::~PartyManager()
{
    m_MapParties.clear(); // ��� ��Ƽ �� �ʱ�ȭ
}

std::shared_ptr<Party> PartyManager::CreateParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    if (user == nullptr || partyName == "")
    {
        std::cout << "[SERVER] Invalid Party Name" << std::endl;
        LOG_ERROR("Invalid Party Name");
        return nullptr;
    }

    auto party = std::make_shared<Party>(m_PartyIdCounter++, user->GetId(), partyName); // ��Ƽ ����
    party->AddMember(user->GetId());                                                    // ��Ƽ �����ڸ� ��Ƽ ����� �߰�
    m_MapParties[party->GetId()] = party;                                               // ��Ƽ�� �ʿ� �߰�

    std::cout << "Party Count : " << m_MapParties.size() << "\n";
    LOG_INFO("Party Count : %zu", m_MapParties.size());
    return party;
}

std::shared_ptr<Party> PartyManager::JoinParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    auto party = FindPartyByName(partyName);
    if (!party)
    {
        std::cout << "[SERVER] Not found party: " << partyName << std::endl;
        LOG_INFO("Not found party: %s", partyName.c_str());
        return nullptr;
    }

    party->AddMember(user->GetId()); // ��Ƽ�� ����� �߰�
    LOG_INFO("User %u joined party %s", user->GetId(), partyName.c_str());
    return party;
}

uint32_t PartyManager::DeleteParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    // ��Ƽ �̸����� ��Ƽ�� ã��
    auto party = FindPartyByName(partyName);
    if (!party)
    {
        std::cout << "[SERVER] Not found party: " << partyName << std::endl;
        LOG_INFO("Not found party: %s", partyName.c_str());
        return 0;
    }

    auto partyId = party->GetId();

    // ��Ƽ �������� ��쿡�� ��Ƽ ���� ����
    if (party->GetPartyCreator() == user->GetId())
    {
        std::cout << "Attempting to delete party with ID: " << partyId << std::endl;
        LOG_INFO("Attempting to delete party with ID: %u", partyId);

        size_t erasedCount = m_MapParties.erase(partyId);
        std::cout << "Number of parties erased: " << erasedCount << std::endl;
        LOG_INFO("Number of parties erased: %zu", erasedCount);

        if (erasedCount == 0)
        {
            std::cout << "[SERVER] Failed to delete party with ID: " << partyId << ". ID not found in m_MapParties." << std::endl;
            LOG_ERROR("Failed to delete party with ID: %u. ID not found in m_MapParties.", partyId);
            return 0;
        }

        std::cout << "[SERVER] Deleted party: " << partyName << std::endl;
        LOG_INFO("[SERVER] Deleted party: %s", partyName.c_str());
    }
    else
    {
        std::cout << "[SERVER] Fail Deleted Party : " << partyName << std::endl;
        LOG_WARN("[SERVER] Fail Deleted Party : %s", partyName.c_str());
        return 0;
    }

    std::cout << "Party Count : " << m_MapParties.size() << "\n";
    LOG_INFO("Party Count : %zu", m_MapParties.size());
    return partyId;
}

bool PartyManager::LeaveParty(std::shared_ptr<UserSession> user, const std::string& partyName)
{
    auto party = FindPartyByName(partyName);
    if (!party)
    {
        std::cout << "[SERVER] Not found party: " << partyName << std::endl;
        LOG_INFO("[SERVER] Not found party: %s", partyName.c_str());
        return false;
    }

    // ��Ƽ �����ڴ� ��Ƽ�� ���� �� ����
    if (party->GetPartyCreator() == user->GetId())
    {
        std::cout << "[SERVER] Sorry, as the party leader, you cannot leave the party. Deletion is the only option." << std::endl;
        LOG_WARN("[SERVER] Sorry, as the party leader, you cannot leave the party. Deletion is the only option.");
        return false;
    }

    party->RemoveMember(user->GetId()); // ��Ƽ ������� ����� ����
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