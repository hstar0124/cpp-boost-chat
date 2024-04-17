#pragma once
#include <vector>
#include "Payload.pb.h"

class PayloadMessage
{
public:
    static constexpr size_t HEADER_SIZE = 4;  // 헤더의 크기

public:
    PayloadMessage() {}

    // 페이로드를 설정
    void SetPayloadMessage(const myPayload::Payload& payload) {
        std::cout << "SetPayloadMessage : " << payload.ByteSizeLong() << "\n";
        m_Payload.CopyFrom(payload);
    }

    // 메시지를 시리얼라이즈하여 버퍼에 패킹
    std::vector<char> PackPayload()
    {
        // 버퍼를 클리어
        m_Buffer.clear();
        // 페이로드를 시리얼라이즈하여 버퍼에 추가
        m_Payload.SerializeToArray(m_Buffer.data(), m_Payload.ByteSizeLong());

        return m_Buffer;
    }


    // 헤더를 언패킹
    bool UnpackHeader(const char* data)
    {
        // 헤더가 충분한지 확인
        if (data == nullptr) {
            std::cerr << "Invalid header data." << std::endl;
            return false;
        }

        // 헤더를 디시리얼라이즈
        if (!m_PayloadHeader.ParseFromArray(data, HEADER_SIZE)) {
            std::cerr << "Failed to parse header." << std::endl;
            return false;
        }

        // 페이로드 사이즈 저장
        m_PayloadSize = m_PayloadHeader.size();        

        return true;
    }

    // 바디를 언패킹합니다.
    bool UnpackBody(const char* data, size_t size)
    {
        // 데이터가 유효한지 확인
        if (data == nullptr) {
            std::cerr << "Invalid body data." << std::endl;
            return false;
        }

        // 바디를 디시리얼라이즈
        if (!m_PayloadBody.ParseFromArray(data, size)) {
            std::cerr << "Failed to parse body." << std::endl;
            return false;
        }

        return true;
    }

    myPayload::PayloadType GetPayloadType()
    {
        return m_PayloadHeader.type();
    }

    size_t GetPayloadSize() const 
    {
        return m_PayloadSize;
    }

    size_t GetBodySize() const 
    {
        return m_PayloadSize - HEADER_SIZE;
    }

    // 헤더의 크기를 반환합니다.
    size_t GetHeaderSize() const 
    {
        return HEADER_SIZE;
    }

    

private:
    myPayload::Payload m_Payload;             // 프로토콜 버퍼 페이로드
    myPayload::Header m_PayloadHeader;        // 페이로드 Header
    myPayload::Body m_PayloadBody;            // 페이로드 body
    
    size_t m_PayloadSize = 0;                 // 언패킹된 페이로드의 크기를 저장하는 변수
    std::vector<char> m_Buffer;               // 패킹된 데이터를 저장하는 버퍼
    
};