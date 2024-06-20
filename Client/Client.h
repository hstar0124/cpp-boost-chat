#pragma once
#include "Common.h"
#include "UserMessage.pb.h"
#include "HttpClient.h"
#include "ChatClient.h"

class Client
{
private:

	std::unique_ptr<HttpClient>		m_HttpClient;
	std::unique_ptr<ChatClient>		m_ChatClient;
	boost::asio::io_context			m_IoContext;

	std::thread						m_Thread;

	std::unordered_map<std::string, std::string> m_ApiUrls;


public:
	Client();
	~Client();

	void Start();

private:
	void Init();
	void DisplayCommand();
	void StartChatClient(const std::string& ip, const std::string& port, const std::string& sessionId);
	void HandleMenuChoice(int choice);
	void GetUserInput(const std::string& prompt, std::string* input);
	
	UserResponse ProcessLoginUser();
	UserResponse ProcessGetUser();
	UserResponse ProcessCreateUser();
	UserResponse ProcessUpdateUser();
	UserResponse ProcessDeleteUser();
	
	template <typename RequestType>
	UserResponse ProcessUserPostRequest(const std::string& endpoint, const RequestType& request);

	UserResponse ProcessUserGetRequest(const std::string& endpoint);


	std::unordered_map<std::string, std::string> LoadConfig(const std::string& filename);
};