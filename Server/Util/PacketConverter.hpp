#pragma once
#include "Message/MyMessage.pb.h"
#include "Common.h"

template <typename T>
class MessageConverter
{
public:
    // ����Ʈ ���ۿ��� �������� ���� �޽����� ������ȭ�ϴ� �Լ�
    static bool DeserializeMessage(std::vector<uint8_t>& buffer, std::shared_ptr<T>& message)
    {
        return message->ParseFromArray(buffer.data(), buffer.size());
    }

    // �������� ���� �޽����� ����Ʈ ���۷� ����ȭ�ϴ� �Լ�
    static bool SerializeMessage(std::shared_ptr<T>& message, std::vector<uint8_t>& buffer)
    {
        size_t size = GetMessageSize(message);
        buffer.clear();
        buffer.resize(HEADER_SIZE + size);
        return message->SerializePartialToArray(buffer.data() + HEADER_SIZE, size);
    }

    // ����Ʈ ������ ����� �޽��� ũ�⸦ �����ϴ� �Լ�
    static bool SetSizeToBufferHeader(std::vector<uint8_t>& buffer)
    {
        if (buffer.size() < HEADER_SIZE)
        {
            return false; // ��� ũ�Ⱑ �����Ͽ� ������ �� �����ϴ�
        }

        // ���̷ε� ũ�⸦ ����Ͽ� ������ ����� �����մϴ�
        size_t size = static_cast<size_t>(buffer.size()) - HEADER_SIZE;
        buffer[0] = static_cast<uint8_t>((size >> 24) & 0xFF);
        buffer[1] = static_cast<uint8_t>((size >> 16) & 0xFF);
        buffer[2] = static_cast<uint8_t>((size >> 8) & 0xFF);
        buffer[3] = static_cast<uint8_t>(size & 0xFF);

        return true;
    }

    // �������� ���� �޽����� ����ȭ�� ũ�⸦ ��ȯ�ϴ� �Լ�
    static size_t GetMessageSize(const std::shared_ptr<T>& message)
    {
        return message->ByteSizeLong();
    }

    // ����Ʈ ���ۿ��� ������ �޽��� �ٵ��� ũ�⸦ ��ȯ�ϴ� �Լ�
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