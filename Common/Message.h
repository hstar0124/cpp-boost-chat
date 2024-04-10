#pragma once
#include "Common.h"

// 메시지 타입을 나타내는 enum 클래스
// uint32_t 를 하게 되면 각각의 ID의 사이즈는 4바이트
enum class PacketType : uint32_t
{
	ServerAccept,	
	ServerPing,
	ServerMessage,
	AllMessage	
};


struct MessageHeader
{
	PacketType id{};
	uint32_t size = 0;
};

struct Message
{
	MessageHeader header{};
	std::vector<uint8_t> body;

	size_t size() const
	{
		// 헤더의 크기와 본문의 크기를 모두 더해서 반환
		return body.size();
	}

	// std::cout 호환성을 위한 재정의 - 메시지 관련된 정보 출력
	friend std::ostream& operator << (std::ostream& os, const Message& msg)
	{
		os << "ID: " << int(msg.header.id) << " Size: " << msg.header.size;
		return os;
	}

	// POD(Plain Old Data)와 같은 데이터를 메시지 버퍼에 추가
	template<typename DataType>
	friend Message& operator << (Message& msg, const DataType& data)
	{
		// 들어온 데이터가 복사 가능한지 확인한다.
		static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

		// 벡터의 현재 크기를 캐시합니다. 데이터가 삽입될 지점 체크
		size_t i = msg.body.size();

		// 추가되는 데이터의 크기로 벡터의 크기를 조정
		msg.body.resize(msg.body.size() + sizeof(DataType));

		// 새로 할당된 벡터 공간에 데이터를 물리적으로 복사
		std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

		// 메시지 크기를 다시 계산
		msg.header.size = msg.size();

		return msg;
	}


	template<typename DataType>
	friend Message& operator >> (Message& msg, DataType& data)
	{
		static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

		// 벡터 뒤에서부터 데이터를 가져오기 때문에 벡터의 끝 쪽 위치를 캐시
		size_t i = msg.body.size() - sizeof(DataType);

		// 벡터에서 데이터를 물리적으로 사용자 변수에 복사
		std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

		// 읽은 바이트를 제거하여 벡터를 축소하고 끝 위치를 초기화
		// 벡터를 원래 크기보다 작게 만드는 것은 성능 저하를 일으키지 않는다.
		// 만약 벡터의 앞 부분에서 데이터를 가져오는 경우, 
		// 해당 데이터는 벡터의 앞 부분에서 제거되어야 하며, 
		// 이는 데이터를 추출할 때마다 상당한 재할당을 유발한다.
		// Stack 과 같은 벡터 구현으로 우리는 불필요한 벡터 재할당을 무시할 수 있다.
		msg.body.resize(i);

		// 메시지 크기를 재계산
		msg.header.size = msg.size();

		// 체인되도록 대상 메시지를 반환
		return msg;
	}


};

class TcpSession;

struct OwnedMessage
{
	std::shared_ptr<TcpSession> remote = nullptr;
	Message msg;

	friend std::ostream& operator<<(std::ostream& os, const OwnedMessage& msg)
	{
		os << msg.msg;
		return os;
	}
};