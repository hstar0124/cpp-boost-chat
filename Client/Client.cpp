#include "Client.h"

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
    
    std::cout << "Initializing Start......" << std::endl;
    m_ApiUrls = LoadConfig("apiurl.txt");

    try
    {
        m_HttpClient = std::make_unique<HttpClient>(m_IoContext, m_ApiUrls["host"], m_ApiUrls["port"]);        
        m_ChatClient = std::make_unique<ChatClient>(m_IoContext);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error initializing clients: " << ex.what() << std::endl;
        exit(1);  
    }
}

void Client::Start()
{
    std::cout << "Initializing Success!!!" << std::endl;
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
    std::cout << std::endl;
    std::cout << std::endl;
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
    {
        auto response = ProcessLoginUser();
        if (response.status() != UserStatusCode::Success) 
        {
            std::cout << "Login Failed!!" << std::endl;
            break;
        }

        if (response.has_content())
        {
            LoginResponse loginResponse;
            if (response.content().UnpackTo(&loginResponse))
            {
                StartChatClient(
                      loginResponse.serverip()
                    , loginResponse.serverport()
                    , loginResponse.sessionid());
            }            
        }
        
        break;
    }
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

void Client::StartChatClient(const std::string& ip, const std::string& port, const std::string& sessionId)
{
    std::cout << "IP : " << ip << std::endl;
    std::cout << "PORT : " << port << std::endl;
    std::cout << "SessionID : " << sessionId << std::endl;

    m_ChatClient->Connect(ip, std::stoi(port));
    m_ChatClient->SendSessionId(sessionId);

    m_Thread = std::thread([this]() 
        { 
            m_IoContext.run(); 
        });

    // Client가 로그인 성공 이후 Verify 되기 전까지 대기
    m_ChatClient->SetVerificationCallback([this]() 
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_IsVerified = true;
            m_Condition.notify_one();
        });

    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_Condition.wait(lock, [this] 
            {
                return m_IsVerified; 
            });
    }

    // 인증이 완료된 후 채팅 시작
    ChatLoop();

    m_Thread.join();
}

UserResponse Client::ProcessLoginUser()
{
    LoginRequest loginRequest;
    GetUserInput("Enter User ID: ", loginRequest.mutable_userid());
    GetUserInput("Enter Password: ", loginRequest.mutable_password());

    return ProcessUserPostRequest("login", loginRequest);
}

UserResponse Client::ProcessGetUser()
{
    GetUserRequest getUserRequest;
    GetUserInput("Enter User ID: ", getUserRequest.mutable_userid());
    
    const std::string endpoint = m_ApiUrls["get_user_by_user_id"] + getUserRequest.userid();

    std::cout << endpoint << "\n";

    return ProcessUserGetRequest(endpoint);
}

UserResponse Client::ProcessCreateUser()
{
    CreateUserRequest createUserRequest;
    GetUserInput("Enter User ID: ", createUserRequest.mutable_userid());
    GetUserInput("Enter Password: ", createUserRequest.mutable_password());
    GetUserInput("Enter Username: ", createUserRequest.mutable_username());
    GetUserInput("Enter Email: ", createUserRequest.mutable_email());
    return ProcessUserPostRequest("create_user", createUserRequest);
}

UserResponse Client::ProcessUpdateUser()
{
    UpdateUserRequest updateUserRequest;
    GetUserInput("Enter User ID: ", updateUserRequest.mutable_userid());
    GetUserInput("Enter Current Password: ", updateUserRequest.mutable_password());
    GetUserInput("Enter New Password: ", updateUserRequest.mutable_tobepassword());
    GetUserInput("Enter New Username: ", updateUserRequest.mutable_tobeusername());
    GetUserInput("Enter New Email: ", updateUserRequest.mutable_tobeemail());
    return ProcessUserPostRequest("update_user", updateUserRequest);
}

UserResponse Client::ProcessDeleteUser()
{
    DeleteUserRequest deleteUserRequest;
    GetUserInput("Enter User ID: ", deleteUserRequest.mutable_userid());
    GetUserInput("Enter Password: ", deleteUserRequest.mutable_password());
    return ProcessUserPostRequest("delete_user", deleteUserRequest);
}

template <typename RequestType>
UserResponse Client::ProcessUserPostRequest(const std::string& endpoint, const RequestType& request) 
{
    UserResponse response = m_HttpClient->Post(m_ApiUrls[endpoint], request);
    std::cout << "Response Status: " << response.status() << std::endl;
    std::cout << "Response Message: " << response.message() << std::endl;

    return response;
}

UserResponse Client::ProcessUserGetRequest(const std::string& endpoint)
{
    UserResponse response = m_HttpClient->Get(endpoint);

    std::cout << "Response Status: " << response.status() << std::endl;
    std::cout << "Response Message: " << response.message() << std::endl;

    return response;
}

std::unordered_map<std::string, std::string> Client::LoadConfig(const std::string& filename)
{
    std::ifstream file(filename);
    std::unordered_map<std::string, std::string> config;
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key, value;
        std::getline(iss, key, '=');
        std::getline(iss, value);
        config[key] = value;
    }
    return config;
}


void Client::GetUserInput(const std::string& prompt, std::string* input) 
{
    std::cout << prompt;
    std::cin >> *input;
}

void Client::ChatLoop() 
{
    std::string userInput;
    while (getline(std::cin, userInput)) 
    {
        // 인증이 완료된 후에만 입력을 처리한다
        if (m_IsVerified) {
            m_ChatClient->Send(userInput);
        }
        else {
            std::cout << "Waiting for verification. Please wait..." << std::endl;
        }
    }
}