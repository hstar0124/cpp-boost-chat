#pragma once
#include "Message/MyMessage.pb.h"
#include "Common.h"

template <typename T>
class MessageConverter
{
public:
    // 바이트 버퍼에서 프로토콜 버퍼 메시지로 역직렬화하는 함수
    static bool DeserializeMessage(std::vector<uint8_t>& buffer, std::shared_ptr<T>& message)
    {
        return message->ParseFromArray(buffer.data(), buffer.size());
    }

    // 프로토콜 버퍼 메시지를 바이트 버퍼로 직렬화하는 함수
    static bool SerializeMessage(std::shared_ptr<T>& message, std::vector<uint8_t>& buffer)
    {
        size_t size = GetMessageSize(message);
        buffer.clear();
        buffer.resize(HEADER_SIZE + size);
        return message->SerializePartialToArray(buffer.data() + HEADER_SIZE, size);
    }

    // 바이트 버퍼의 헤더에 메시지 크기를 설정하는 함수
    static bool SetSizeToBufferHeader(std::vector<uint8_t>& buffer)
    {
        if (buffer.size() < HEADER_SIZE)
        {
            return false; // 헤더 크기가 부족하여 설정할 수 없습니다
        }

        // 페이로드 크기를 계산하여 버퍼의 헤더에 설정합니다
        size_t size = static_cast<size_t>(buffer.size()) - HEADER_SIZE;
        buffer[0] = static_cast<uint8_t>((size >> 24) & 0xFF);
        buffer[1] = static_cast<uint8_t>((size >> 16) & 0xFF);
        buffer[2] = static_cast<uint8_t>((size >> 8) & 0xFF);
        buffer[3] = static_cast<uint8_t>(size & 0xFF);

        return true;
    }

    // 프로토콜 버퍼 메시지의 직렬화된 크기를 반환하는 함수
    static size_t GetMessageSize(const std::shared_ptr<T>& message)
    {
        return message->ByteSizeLong();
    }

    // 바이트 버퍼에서 추출한 메시지 바디의 크기를 반환하는 함수
    static size_t GetMessageBodySize(const std::vector<uint8_t>& buffer)
    {
        size_t size = 0;
        for (int i = 0; i < HEADER_SIZE; i++)
        {
            size = static_cast<size_t>(buffer[i]);
        }

        return size;
    }
};
