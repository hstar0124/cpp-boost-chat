#include "Client.h"

Client::Client()
{
    Init(); // 클라이언트 초기화
}

Client::~Client()
{
    m_IoContext.stop(); // IO 컨텍스트 정지

    if (m_Thread.joinable())
    {
        m_Thread.join(); // 스레드 종료 대기
    }
}

void Client::Init()
{
    std::cout << "Initializing Start......" << std::endl;
    m_ApiUrls = LoadConfig("apiurl.txt"); // 설정 파일로부터 API URL 로드

    try
    {
        m_HttpClient = std::make_unique<HttpClient>(m_IoContext, m_ApiUrls["host"], m_ApiUrls["port"]); // HTTP 클라이언트 초기화        
        m_ChatClient = std::make_unique<ChatClient>(m_IoContext); // 채팅 클라이언트 초기화
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error initializing clients: " << ex.what() << std::endl; // 클라이언트 초기화 오류 메시지 출력
        exit(1);  // 프로그램 종료
    }
}

void Client::Start()
{
    std::cout << "Initializing Success!!!" << std::endl;
    while (true)
    {
        DisplayCommand(); // 메뉴 표시

        int choice;
        std::cin >> choice; // 사용자 입력 받기

        if (std::cin.fail())
        {
            std::cin.clear(); // 입력 오류 초기화
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 입력 버퍼 비우기
            std::cout << "Invalid input. Please enter a number between 1 and 5." << std::endl; // 잘못된 입력 메시지 출력
            continue;
        }

        if (choice == 6) break; // 종료 옵션 선택 시 반복문 종료

        HandleMenuChoice(choice); // 메뉴 선택 처리
    }
}

void Client::DisplayCommand()
{
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "메뉴를 선택하세요" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "1. 로그인" << std::endl;
    std::cout << "2. 사용자 정보 가져오기" << std::endl;
    std::cout << "3. 사용자 생성" << std::endl;
    std::cout << "4. 사용자 업데이트" << std::endl;
    std::cout << "5. 사용자 삭제" << std::endl;
    std::cout << "6. 종료" << std::endl;
    std::cout << "옵션을 선택하세요: ";
}

void Client::HandleMenuChoice(int choice)
{
    switch (choice)
    {
    case 1:
    {
        auto response = ProcessLoginUser(); // 로그인 처리
        if (response.status() != UserStatusCode::Success)
        {
            std::cout << "Login Failed!!" << std::endl; // 로그인 실패 메시지 출력
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
                    , loginResponse.sessionid()); // 채팅 클라이언트 시작
            }
        }

        break;
    }
    case 2:
        ProcessGetUser(); // 사용자 정보 조회
        break;
    case 3:
        ProcessCreateUser(); // 사용자 생성
        break;
    case 4:
        ProcessUpdateUser(); // 사용자 업데이트
        break;
    case 5:
        ProcessDeleteUser(); // 사용자 삭제
        break;
    default:
        std::cout << "Invalid choice. Please try again." << std::endl; // 잘못된 선택 메시지 출력
        break;
    }
}

void Client::StartChatClient(const std::string& ip, const std::string& port, const std::string& sessionId)
{
    std::cout << "IP : " << ip << std::endl;
    std::cout << "PORT : " << port << std::endl;
    std::cout << "SessionID : " << sessionId << std::endl;

    m_ChatClient->Connect(ip, std::stoi(port)); // 채팅 서버에 연결
    m_ChatClient->SendSessionId(sessionId); // 세션 ID 전송

    m_Thread = std::thread([ this ] ()
        {
            m_IoContext.run(); // IO 컨텍스트 실행 
        });

    // 클라이언트 확인 콜백 설정
    m_ChatClient->SetVerificationCallback([ this ] ()
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_IsVerified = true; // 확인 완료 상태 설정
            m_Condition.notify_one(); // 조건 변수 신호 전송
        });

    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_Condition.wait(lock, [ this ]
            {
                return m_IsVerified; // 확인 완료 시까지 대기
            });
    }

    // 채팅 루프 시작
    ChatLoop();

    m_Thread.join(); // 스레드 종료 대기
}

UserResponse Client::ProcessLoginUser()
{
    LoginRequest loginRequest;
    GetUserInput("Enter User ID: ", loginRequest.mutable_userid()); // 사용자 ID 입력 요청
    GetUserInput("Enter Password: ", loginRequest.mutable_password()); // 비밀번호 입력 요청

    return ProcessUserPostRequest("login", loginRequest); // 로그인 요청 처리
}

UserResponse Client::ProcessGetUser()
{
    GetUserRequest getUserRequest;
    GetUserInput("Enter User ID: ", getUserRequest.mutable_userid()); // 사용자 ID 입력 요청

    const std::string endpoint = m_ApiUrls["get_user_by_user_id"] + getUserRequest.userid(); // 엔드포인트 설정

    std::cout << endpoint << "\n"; // 엔드포인트 출력

    return ProcessUserGetRequest(endpoint); // 사용자 정보 조회 요청 처리
}

UserResponse Client::ProcessCreateUser()
{
    CreateUserRequest createUserRequest;
    GetUserInput("Enter User ID: ", createUserRequest.mutable_userid()); // 사용자 ID 입력 요청
    GetUserInput("Enter Password: ", createUserRequest.mutable_password()); // 비밀번호 입력 요청
    GetUserInput("Enter Username: ", createUserRequest.mutable_username()); // 사용자 이름 입력 요청
    GetUserInput("Enter Email: ", createUserRequest.mutable_email()); // 이메일 입력 요청
    return ProcessUserPostRequest("create_user", createUserRequest); // 사용자 생성 요청 처리
}

UserResponse Client::ProcessUpdateUser()
{
    UpdateUserRequest updateUserRequest;
    GetUserInput("Enter User ID: ", updateUserRequest.mutable_userid()); // 사용자 ID 입력 요청
    GetUserInput("Enter Current Password: ", updateUserRequest.mutable_password()); // 현재 비밀번호 입력 요청
    GetUserInput("Enter New Password: ", updateUserRequest.mutable_tobepassword()); // 새로운 비밀번호 입력 요청
    GetUserInput("Enter New Username: ", updateUserRequest.mutable_tobeusername()); // 새로운 사용자 이름 입력 요청
    GetUserInput("Enter New Email: ", updateUserRequest.mutable_tobeemail()); // 새로운 이메일 입력 요청
    return ProcessUserPostRequest("update_user", updateUserRequest); // 사용자 업데이트 요청 처리
}

UserResponse Client::ProcessDeleteUser()
{
    DeleteUserRequest deleteUserRequest;
    GetUserInput("Enter User ID: ", deleteUserRequest.mutable_userid()); // 사용자 ID 입력 요청
    GetUserInput("Enter Password: ", deleteUserRequest.mutable_password()); // 비밀번호 입력 요청
    return ProcessUserPostRequest("delete_user", deleteUserRequest); // 사용자 삭제 요청 처리
}

template <typename RequestType>
UserResponse Client::ProcessUserPostRequest(const std::string& endpoint, const RequestType& request)
{
    UserResponse response = m_HttpClient->Post(m_ApiUrls[endpoint], request); // POST 요청 전송
    std::cout << "Response Status: " << response.status() << std::endl; // 응답 상태 출력
    std::cout << "Response Message: " << response.message() << std::endl; // 응답 메시지 출력

    return response; // 응답 반환
}

UserResponse Client::ProcessUserGetRequest(const std::string& endpoint)
{
    UserResponse response = m_HttpClient->Get(endpoint); // GET 요청 전송

    std::cout << "Response Status: " << response.status() << std::endl; // 응답 상태 출력
    std::cout << "Response Message: " << response.message() << std::endl; // 응답 메시지 출력

    return response; // 응답 반환
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
        std::getline(iss, key, '='); // '=' 문자 기준으로 키 추출
        std::getline(iss, value); // 값 추출
        config[key] = value; // 설정 맵에 추가
    }
    return config; // 설정 맵 반환
}


void Client::GetUserInput(const std::string& prompt, std::string* input)
{
    std::cout << prompt; // 프롬프트 출력
    std::cin >> *input; // 사용자 입력 받기
}


void Client::ChatLoop()
{
    std::string userInput;

    if (m_IsVerified)
    {

        while (getline(std::cin, userInput))
        {
            m_ChatClient->Send(userInput); // 채팅 메시지 전송
        }
    }
    else
    {
        std::cout << "Waiting for verification. Please wait..." << std::endl; // 확인 대기 메시지 출력
    }
}
