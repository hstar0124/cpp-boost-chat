#pragma once
#include "Common.h"
#include "MyMessage.pb.h"
#include "PacketConverter.hpp"

class User : public std::enable_shared_from_this<User>
{
private:
	boost::asio::ip::tcp::socket												m_Socket;
	boost::asio::io_context&													m_IoContext;
	uint32_t																	m_Id = 0;
	bool																		m_IsActive;

	std::vector<uint8_t>														m_Writebuf;
	std::vector<uint8_t>														m_Readbuf;

	std::shared_ptr<std::queue<std::shared_ptr<myChatMessage::ChatMessage>>>	m_MessageQueue1;
	std::shared_ptr<std::queue<std::shared_ptr<myChatMessage::ChatMessage>>>	m_MessageQueue2;

	std::shared_ptr<std::queue<std::shared_ptr<myChatMessage::ChatMessage>>>	m_InputQueue;
	std::shared_ptr<std::queue<std::shared_ptr<myChatMessage::ChatMessage>>>	m_OutputQueue;
	std::mutex																	m_QueueMutex;

	boost::asio::steady_timer													m_PingTimer;

	uint32_t																	m_PartyId = 0;

public:
	User(boost::asio::io_context& io_context);
	~User();

	void Start(uint32_t uid);
	void Close();
	bool IsConnected();

	uint32_t GetID() const;
	uint32_t GetPartyId() const;
	void SetPartyId(uint32_t partyId);


	std::shared_ptr<myChatMessage::ChatMessage> GetMessageInUserQueue();
	void Send(std::shared_ptr<myChatMessage::ChatMessage> msg);

	boost::asio::ip::tcp::socket& GetSocket();

private:
	void SendPing();
	void StartPingTimer();
	std::string GetCurrentTimeMilliseconds();

	bool SwapQueues();

	void AsyncWrite(std::shared_ptr<myChatMessage::ChatMessage> msg);
	void ReadHeader();
	void ReadBody(size_t bodySize);

	void HandleError(const std::string& errorMessage);
};

