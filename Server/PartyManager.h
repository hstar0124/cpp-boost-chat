#pragma once
#include "ThreadSafeVector.h""
#include "Party.h"
#include "User.h"

class PartyManager
{
private:
	uint32_t m_PartyIdCounter = 100'000;
	ThreadSafeVector<std::shared_ptr<Party>> m_VecParties;

public:
	static PartyManager& getInstance()
	{
		// staitc 변수로 선언함으로서, instance 뱐수는 한번만 초기화되고, 프로그램 수명 내내 지속됨.
		// 특히 C++11부터 thread-safe 변수 초기화가 보장됨.
		static PartyManager instance;
		return instance;		
	}

	bool CreateParty(std::shared_ptr<User> creatorSession, const std::string& partyName);
	bool DeleteParty(std::shared_ptr<User> session, const std::string& partyName);
	bool HasParty(const std::string& partyName);
	std::shared_ptr<Party> FindPartyByName(const std::string& partyName);
	std::shared_ptr<Party> FindPartyBySessionId(uint32_t creatorId);

private:
	// Default 생성자 사용 (필요시 생성자를 원하는데로 수정해서 사용해도 됨)
	PartyManager() = default;
	// 객체는 유일하게 하나만 생성되어야 하기에 복사(대입), 이동(대입) 생성자 비활성화
	// 복사, 이동 생성자를 delete로 선언
	// 프로그래머 실수에 의한 복사, 이동 생성자 호출을 원천에 방지
	PartyManager(const PartyManager&) = delete;
	PartyManager& operator=(const PartyManager&) = delete;
	PartyManager(PartyManager&&) = delete;
	PartyManager& operator=(PartyManager&&) = delete;

};

