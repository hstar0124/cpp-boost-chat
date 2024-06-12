#pragma once
#include <string>
#include <iostream>
#include "UserMessage.pb.h"
#include "HttpClient.h"
#include "ChatClient.h"

class Client
{
private:
	const std::string m_Host = "localhost";
	const std::string m_Port = "5174";

	std::unique_ptr<HttpClient> m_HttpClient;
	std::unique_ptr<ChatClient> m_ChatClient;
	boost::asio::io_context m_IoContext;

	std::thread m_Thread;

public:
	Client();
	~Client();

	void Start();

private:
	void Init();
	void DisplayCommand();
	void StartChatClient();
	void HandleMenuChoice(int choice);
	void GetUserInput(const std::string& prompt, std::string* input);
	
	bool ProcessLoginUser();
	bool ProcessGetUser();
	bool ProcessCreateUser();
	bool ProcessUpdateUser();
	bool ProcessDeleteUser();
	
	template <typename RequestType>
	bool ProcessUserPostRequest(const std::string& endpoint, const RequestType& request);

	bool ProcessUserGetRequest(const std::string& endpoint);
};