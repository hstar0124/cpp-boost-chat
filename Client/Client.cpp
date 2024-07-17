#include "Client.h"
#include <Util/HsLogger.hpp>

Client::Client()
{
    Init(); // Ŭ���̾�Ʈ �ʱ�ȭ
}

Client::~Client()
{
    m_IoContext.stop(); // IO ���ؽ�Ʈ ����

    if (m_Thread.joinable())
    {
        m_Thread.join(); // ������ ���� ���
    }
}

void Client::Init()
{
    std::cout << "Initializing Start......" << std::endl;
    m_ApiUrls = LoadConfig("apiurl.txt"); // ���� ���Ϸκ��� API URL �ε�

    try
    {
        m_HttpClient = std::make_unique<HttpClient>(m_IoContext, m_ApiUrls["host"], m_ApiUrls["port"]); // HTTP Ŭ���̾�Ʈ �ʱ�ȭ        
        m_ChatClient = std::make_unique<ChatClient>(m_IoContext); // ä�� Ŭ���̾�Ʈ �ʱ�ȭ
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error initializing clients: " << ex.what() << std::endl; // Ŭ���̾�Ʈ �ʱ�ȭ ���� �޽��� ���
        LOG_ERROR("Error initializing clients: %s", ex.what()); // Ŭ���̾�Ʈ �ʱ�ȭ ���� �α� ���
        exit(1);  // ���α׷� ����
    }
}

void Client::Start()
{

    std::cout << "Initializing Success!!!" << std::endl;
    LOG_INFO("Initializing Success!!!");
    while (true)
    {
        DisplayCommand(); // �޴� ǥ��

        int choice;
        std::cin >> choice; // ����� �Է� �ޱ�

        if (std::cin.fail())
        {
            std::cin.clear(); // �Է� ���� �ʱ�ȭ
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // �Է� ���� ����
            std::cout << "Invalid input. Please enter a number between 1 and 5." << std::endl; // �߸��� �Է� �޽��� ���
            LOG_WARN("Invalid input. Please enter a number between 1 and 5."); // �߸��� �Է� ��� �α� ���
            continue;
        }

        if (choice == 6) break; // ���� �ɼ� ���� �� �ݺ��� ����

        HandleMenuChoice(choice); // �޴� ���� ó��
    }
}

void Client::DisplayCommand()
{
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "�޴��� �����ϼ���" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "1. �α���" << std::endl;
    std::cout << "2. ����� ���� ��������" << std::endl;
    std::cout << "3. ����� ����" << std::endl;
    std::cout << "4. ����� ������Ʈ" << std::endl;
    std::cout << "5. ����� ����" << std::endl;
    std::cout << "6. ����" << std::endl;
    std::cout << "�ɼ��� �����ϼ���: ";
}

void Client::HandleMenuChoice(int choice)
{
    switch (choice)
    {
    case 1:
    {
        auto response = ProcessLoginUser(); // �α��� ó��
        if (response.status() != UserStatusCode::Success)
        {
            std::cout << "Login Failed!!" << std::endl; // �α��� ���� �޽��� ���
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
                    , loginResponse.sessionid()); // ä�� Ŭ���̾�Ʈ ����
            }
        }

        break;
    }
    case 2:
        ProcessGetUser(); // ����� ���� ��ȸ
        break;
    case 3:
        ProcessCreateUser(); // ����� ����
        break;
    case 4:
        ProcessUpdateUser(); // ����� ������Ʈ
        break;
    case 5:
        ProcessDeleteUser(); // ����� ����
        break;
    default:
        std::cout << "Invalid choice. Please try again." << std::endl; // �߸��� ���� �޽��� ���
        LOG_WARN("Invalid choice. Please try again."); 
        break;
    }
}

void Client::StartChatClient(const std::string& ip, const std::string& port, const std::string& sessionId)
{
    std::cout << "IP : " << ip << std::endl;
    std::cout << "PORT : " << port << std::endl;
    std::cout << "SessionID : " << sessionId << std::endl;
    LOG_INFO("IP : %s", ip.c_str());
    LOG_INFO("PORT : %s", port.c_str());
    LOG_INFO("SessionID : %s", sessionId.c_str());

    m_ChatClient->Connect(ip, std::stoi(port)); // ä�� ������ ����
    m_ChatClient->SendSessionId(sessionId); // ���� ID ����

    m_Thread = std::thread([this]()
        {
            m_IoContext.run(); // IO ���ؽ�Ʈ ���� 
        });

    // Ŭ���̾�Ʈ Ȯ�� �ݹ� ����
    m_ChatClient->SetVerificationCallback([this]()
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_IsVerified = true; // Ȯ�� �Ϸ� ���� ����
            m_Condition.notify_one(); // ���� ���� ��ȣ ����
        });

    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_Condition.wait(lock, [this]
            {
                return m_IsVerified; // Ȯ�� �Ϸ� �ñ��� ���
            });
    }

    // ä�� ���� ����
    ChatLoop();

    m_Thread.join(); // ������ ���� ���
}

UserResponse Client::ProcessLoginUser()
{
    LoginRequest loginRequest;
    GetUserInput("Enter User ID: ", loginRequest.mutable_userid()); // ����� ID �Է� ��û
    GetUserInput("Enter Password: ", loginRequest.mutable_password()); // ��й�ȣ �Է� ��û

    return ProcessUserPostRequest("login", loginRequest); // �α��� ��û ó��
}

UserResponse Client::ProcessGetUser()
{
    GetUserRequest getUserRequest;
    GetUserInput("Enter User ID: ", getUserRequest.mutable_userid()); // ����� ID �Է� ��û

    const std::string endpoint = m_ApiUrls["get_user_by_user_id"] + getUserRequest.userid(); // ��������Ʈ ����

    std::cout << endpoint << "\n"; // ��������Ʈ ���

    return ProcessUserGetRequest(endpoint); // ����� ���� ��ȸ ��û ó��
}

UserResponse Client::ProcessCreateUser()
{
    CreateUserRequest createUserRequest;
    GetUserInput("Enter User ID: ", createUserRequest.mutable_userid()); // ����� ID �Է� ��û
    GetUserInput("Enter Password: ", createUserRequest.mutable_password()); // ��й�ȣ �Է� ��û
    GetUserInput("Enter Username: ", createUserRequest.mutable_username()); // ����� �̸� �Է� ��û
    GetUserInput("Enter Email: ", createUserRequest.mutable_email()); // �̸��� �Է� ��û
    return ProcessUserPostRequest("create_user", createUserRequest); // ����� ���� ��û ó��
}

UserResponse Client::ProcessUpdateUser()
{
    UpdateUserRequest updateUserRequest;
    GetUserInput("Enter User ID: ", updateUserRequest.mutable_userid()); // ����� ID �Է� ��û
    GetUserInput("Enter Current Password: ", updateUserRequest.mutable_password()); // ���� ��й�ȣ �Է� ��û
    GetUserInput("Enter New Password: ", updateUserRequest.mutable_tobepassword()); // ���ο� ��й�ȣ �Է� ��û
    GetUserInput("Enter New Username: ", updateUserRequest.mutable_tobeusername()); // ���ο� ����� �̸� �Է� ��û
    GetUserInput("Enter New Email: ", updateUserRequest.mutable_tobeemail()); // ���ο� �̸��� �Է� ��û
    return ProcessUserPostRequest("update_user", updateUserRequest); // ����� ������Ʈ ��û ó��
}

UserResponse Client::ProcessDeleteUser()
{
    DeleteUserRequest deleteUserRequest;
    GetUserInput("Enter User ID: ", deleteUserRequest.mutable_userid()); // ����� ID �Է� ��û
    GetUserInput("Enter Password: ", deleteUserRequest.mutable_password()); // ��й�ȣ �Է� ��û
    return ProcessUserPostRequest("delete_user", deleteUserRequest); // ����� ���� ��û ó��
}

template <typename RequestType>
UserResponse Client::ProcessUserPostRequest(const std::string& endpoint, const RequestType& request)
{
    UserResponse response = m_HttpClient->Post(m_ApiUrls[endpoint], request); // POST ��û ����
    std::cout << "Response Status: " << response.status() << std::endl; // ���� ���� ���
    std::cout << "Response Message: " << response.message() << std::endl; // ���� �޽��� ���

    return response; // ���� ��ȯ
}

UserResponse Client::ProcessUserGetRequest(const std::string& endpoint)
{
    UserResponse response = m_HttpClient->Get(endpoint); // GET ��û ����

    std::cout << "Response Status: " << response.status() << std::endl; // ���� ���� ���
    std::cout << "Response Message: " << response.message() << std::endl; // ���� �޽��� ���

    return response; // ���� ��ȯ
}

std::unordered_map<std::string, std::string> Client::LoadConfig(const std::string& filename)
{
    std::ifstream m_File(filename);
    std::unordered_map<std::string, std::string> config;
    std::string line;
    while (std::getline(m_File, line))
    {
        std::istringstream iss(line);
        std::string key, value;
        std::getline(iss, key, '='); // '=' ���� �������� Ű ����
        std::getline(iss, value); // �� ����
        config[key] = value; // ���� �ʿ� �߰�
    }
    return config; // ���� �� ��ȯ
}


void Client::GetUserInput(const std::string& prompt, std::string* input)
{
    std::cout << prompt; // ������Ʈ ���
    std::cin >> *input; // ����� �Է� �ޱ�
}


void Client::ChatLoop()
{
    std::string userInput;

    if (m_IsVerified)
    {

        while (getline(std::cin, userInput))
        {
            m_ChatClient->Send(userInput); // ä�� �޽��� ����
        }
    }
    else
    {
        std::cout << "Waiting for verification. Please wait..." << std::endl; // Ȯ�� ��� �޽��� ���
    }
}