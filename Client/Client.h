#pragma once
#include "Common.h"
#include "UserMessage.pb.h"
#include "HttpClient.h"
#include "ChatClient.h"

class Client
{
private:

	std::unique_ptr<HttpClient> m_HttpClient;
	std::unique_ptr<ChatClient> m_ChatClient;

	boost::asio::io_context		m_IoContext;

	std::thread					m_Thread;

	std::unordered_map<std::string, std::string> m_ApiUrls;

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

	std::unordered_map<std::string, std::string> LoadConfig(const std::string& filename);
};