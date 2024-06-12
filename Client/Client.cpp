#include "Client.h"
#include "ChatClient.h"


Client::Client() 
{
    Init();
}

Client::~Client() 
{
    m_IoContext.stop();
    if (m_Thread.joinable()) 
    {
        m_Thread.join();
    }
}

void Client::Init() 
{
    m_HttpClient = std::make_unique<HttpClient>(m_IoContext, m_Host, m_Port);
    m_ChatClient = std::make_unique<ChatClient>(m_IoContext);
}

void Client::Start()
{
    while (true)
    {
        DisplayCommand();

        int choice;
        std::cin >> choice;

        if (std::cin.fail())
        {
            std::cin.clear(); // 스트림 상태 플래그 초기화
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 잘못된 입력 무시
            std::cout << "Invalid input. Please enter a number between 1 and 5." << std::endl;
            continue;
        }

        if (choice == 5) break;

        HandleMenuChoice(choice);
    }
}

void Client::DisplayCommand() 
{
    std::cout << "====================================" << std::endl;
    std::cout << "Select Menu" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "1. Login" << std::endl;
    std::cout << "2. Get User Info By userId" << std::endl;
    std::cout << "3. Create User" << std::endl;
    std::cout << "4. Update User" << std::endl;
    std::cout << "5. Delete User" << std::endl;
    std::cout << "6. Exit" << std::endl;
    std::cout << "Select an option: ";
}

void Client::HandleMenuChoice(int choice) 
{
    switch (choice) 
    {
    case 1:
        if (ProcessLoginUser()) 
        {
            StartChatClient();
        }
        break;
    case 2:
        ProcessGetUser();
        break;
    case 3:
        ProcessCreateUser();
        break;
    case 4:
        ProcessUpdateUser();
        break;
    case 5:
        ProcessDeleteUser();
        break;
    default:
        std::cout << "Invalid choice. Please try again." << std::endl;
        break;
    }
}

void Client::StartChatClient() 
{
    m_ChatClient->Connect("127.0.0.1", 4242);
    m_Thread = std::thread([this]() { m_IoContext.run(); });

    std::string userInput;
    while (getline(std::cin, userInput)) 
    {
        m_ChatClient->Send(userInput);
    }

    m_Thread.join();
}

bool Client::ProcessLoginUser() 
{
    LoginRequest loginRequest;
    GetUserInput("Enter User ID: ", loginRequest.mutable_userid());
    GetUserInput("Enter Password: ", loginRequest.mutable_password());
    return ProcessUserPostRequest("/User/Login", loginRequest);
}

bool Client::ProcessGetUser()
{
    GetUserRequest getUserRequest;
    GetUserInput("Enter User ID: ", getUserRequest.mutable_userid());

    const std::string endpoint = "/User/GetUser?userId=" + getUserRequest.userid();

    return ProcessUserGetRequest(endpoint);
}

bool Client::ProcessCreateUser() 
{
    CreateUserRequest createUserRequest;
    GetUserInput("Enter User ID: ", createUserRequest.mutable_userid());
    GetUserInput("Enter Password: ", createUserRequest.mutable_password());
    GetUserInput("Enter Username: ", createUserRequest.mutable_username());
    GetUserInput("Enter Email: ", createUserRequest.mutable_email());
    return ProcessUserPostRequest("/User/Create", createUserRequest);
}

bool Client::ProcessUpdateUser() 
{
    UpdateUserRequest updateUserRequest;
    GetUserInput("Enter User ID: ", updateUserRequest.mutable_userid());
    GetUserInput("Enter Current Password: ", updateUserRequest.mutable_password());
    GetUserInput("Enter New Password: ", updateUserRequest.mutable_tobepassword());
    GetUserInput("Enter New Username: ", updateUserRequest.mutable_tobeusername());
    GetUserInput("Enter New Email: ", updateUserRequest.mutable_tobeemail());
    return ProcessUserPostRequest("/User/Update", updateUserRequest);
}

bool Client::ProcessDeleteUser() 
{
    DeleteUserRequest deleteUserRequest;
    GetUserInput("Enter User ID: ", deleteUserRequest.mutable_userid());
    GetUserInput("Enter Password: ", deleteUserRequest.mutable_password());
    return ProcessUserPostRequest("/User/Delete", deleteUserRequest);
}

template <typename RequestType>
bool Client::ProcessUserPostRequest(const std::string& endpoint, const RequestType& request) 
{
    UserResponse response = m_HttpClient->Post(endpoint, request);
    std::cout << "Response Status: " << response.status() << std::endl;
    std::cout << "Response Message: " << response.message() << std::endl;
    return response.status() == UserStatusCode::Success;
}

bool Client::ProcessUserGetRequest(const std::string& endpoint)
{
    UserResponse response = m_HttpClient->Get(endpoint);

    std::cout << "Response Status: " << response.status() << std::endl;
    std::cout << "Response Message: " << response.message() << std::endl;

    if (response.has_content()) 
    {
        GetUserResponse getUserResponse;
        if (response.content().UnpackTo(&getUserResponse)) 
        {
            std::cout << "User ID: " << getUserResponse.userid() << std::endl;
            std::cout << "Username: " << getUserResponse.username() << std::endl;
            std::cout << "Email: " << getUserResponse.email() << std::endl;
        }
        else 
        {
            std::cout << "Failed to unpack content." << std::endl;
        }
    }

    return response.status() == UserStatusCode::Success;
}


void Client::GetUserInput(const std::string& prompt, std::string* input) 
{
    std::cout << prompt;
    std::cin >> *input;
}