#pragma once
#include "Common.h"
#include "MyMessage.pb.h"


class ChatClient
{
private:

	boost::asio::ip::tcp::endpoint	m_Endpoint;
	boost::asio::io_context&		m_IoContext;
	boost::asio::ip::tcp::socket	m_Socket;
	std::string						m_Message;

	std::vector<uint8_t>			m_Readbuf;

public:

	ChatClient(boost::asio::io_context& io_context);
	~ChatClient();

	void Connect(std::string host, int port);
	void Send(const std::string& userInput);


private:
	void OnConnect(const boost::system::error_code& err);
	void OnMessage(std::shared_ptr<myChatMessage::ChatMessage>& message);
	void OnWrite(const boost::system::error_code& err, const size_t size);

	void AsyncWrite(std::shared_ptr<myChatMessage::ChatMessage> message);
	void ReadHeader();
	void ReadBody(size_t bodySize);
	void SendPong();
	

	bool IsWhisperMessage(const std::string& userInput);
	bool IsPartyMessage(const std::string& userInput);

	bool CreateWhisperMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput);
	bool CreatePartyMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput);
	bool CreateNormalMessage(std::shared_ptr<myChatMessage::ChatMessage>& chatMessage, const std::string& userInput);
	
	std::pair<std::string, std::string> ExtractOptionAndPartyName(const std::string& userInput);
};
