#include "PartyManager.h"


bool PartyManager::CreateParty(std::shared_ptr<TcpSession> creatorSession, const std::string& partyName)
{
    if (creatorSession == nullptr || partyName == "")
    {
        std::cout << "[SERVER] Invalid Party Name" << std::endl;
        return false;
    }

    auto party = std::make_shared<Party>(m_PartyIdCounter++, creatorSession->GetID(), partyName);  // 파티 생성
    party->AddMember(creatorSession->GetID());                                                   // 파티 생성자를 파티 멤버로 추가
    m_VecParties.PushBack(party);                                                       // 파티를 파티 리스트에 추가

    std::cout << "Party Count : " << m_VecParties.Count() << "\n";
    return true;
}

bool PartyManager::DeleteParty(std::shared_ptr<TcpSession> session, const std::string& partyName)
{
    // 파티 이름에 해당하는 파티를 찾음|
    auto it = FindPartyByName(partyName);
    if (it == nullptr)
    {
        std::cout << "[SERVER] Not found party: " << partyName << std::endl;
        return false;
    }

    // 파티가 존재하고 파티 창설자인 경우
    if (it != nullptr && it->GetPartyCreator() == session->GetID())
    {
        // 파티를 파티 컨테이너에서 삭제
        m_VecParties.Erase(it);
        std::cout << "[SERVER] Deleted party: " << partyName << std::endl;
    }
    else
    {
        // 해당 이름을 가진 파티가 없거나 창시자가 아닌 경우
        std::cout << "[SERVER] Fail Deleted Party : " << partyName << std::endl;
        return false;
        //SendErrorMessage(session, "Failed to delete the party.");
    }

    std::cout << "Party Count : " << m_VecParties.Count() << "\n";
    return true;
}

bool PartyManager::HasParty(const std::string& partyName)
{
    // 파티 이름에 해당하는 파티를 찾음
    auto it = std::find_if(m_VecParties.Begin(), m_VecParties.End(),
        [&partyName](std::shared_ptr<Party>& party)
        {
            return party->GetName() == partyName;
        });

    return it != m_VecParties.End();
}


std::shared_ptr<Party> PartyManager::FindPartyByName(const std::string& partyName)
{
    // 파티 이름에 해당하는 파티를 찾음
    auto it = std::find_if(m_VecParties.Begin(), m_VecParties.End(),
        [&partyName](std::shared_ptr<Party>& party)
        {
            return party->GetName() == partyName;
        });

    // 파티가 존재하는 경우 해당 파티를 반환
    if (it != m_VecParties.End())
        return *it;
    else
        return nullptr;
}

std::shared_ptr<Party> PartyManager::FindPartyBySessionId(uint32_t creatorId)
{
    auto it = std::find_if(m_VecParties.Begin(), m_VecParties.End(),
        [&creatorId](std::shared_ptr<Party>& party)
        {
            return party->HasMember(creatorId);
        });

    // 파티가 존재하는 경우 해당 파티를 반환
    if (it != m_VecParties.End())
        return *it;
    else
        return nullptr;
}
