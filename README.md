# HStar Project 

- Asp.net Core 와 Protobuf를 활용한 API Server
- Boost.Asio 와 Protobuf를 활용한 Socket Server

<br/>

## 프로젝트 소개

로그인 및 유저 정보 관리는 ASP.NET Core API 서버를 통해 안정적이고 확장 가능한 백엔드 서비스를 구축했습니다.
이를 통해 **안정적**이고 **확장 가능**한 백엔드 서비스를 구축할 수 있었습니다. 
**MVC 패턴**을 적용함으로써 코드의 **가독성과 유지보수성**을 높였으며, 
**Entity Framework Core**를 활용하여 데이터베이스 작업의 **생산성**을 크게 향상시켰습니다. 
이러한 접근 방식은 유저 데이터의 효율적인 관리와 안정성을 보장합니다.

빠른 반응 속도를 달성하기 위해 C++로 소켓 서버를 구성하였습니다. 
C++는 **높은 성능과 자원 효율성**을 제공하며, 
이를 통해 게임 서버의 핵심인 **빠른 데이터 처리를 구현**할 수 있었습니다. 
특히, **비동기 I/O**에 특화된 Boost.Asio 라이브러리를 활용하여 소켓 통신 기능을 구현하였고, 
이를 통해 높은 동시성을 처리할 수 있는 서버 환경을 구축하였습니다. 
**Protobuf**를 사용한 메시지 직렬화/역직렬화는 **JSON 대비 패킷 사이즈를 약 2/3 절감**시켜 네트워크 대역폭을 효율적으로 활용할 수 있게 해주었습니다

<br/>

## 기술 스택

- C++ 17
- .NET 8
- ASP.Net Core
- EntityFreamworkCore-MySql 8.0.2
- Boost 1.84.0
- Protobuf 3.14.0
- hiredis 1.2.0
- Redis
- Mysql 8.0
- openssl 3.3.1
- Visual Studio 2022
- Windows 10
- Github
  
<br/>

## 주요 특장점

### [API 서버]

- 로그인/유저 CRUD 기능 제공
- Protobuf 를 활용한 Http Body 직렬화/역직렬화 하여 **JSON 대비 패킷 2/3 절감**
- Password 를 **Hash + Salt 하여 개인정보 보호**
- Service를 Read / Write 부분을 분리하여 구현함으로 복잡성 관리
- **MVC 패턴**을 적용하여 **유지보수성, 확장성, 유연성** 확보
- **EntityFrameworkCore** 를 적용하여 **생산성** 향상

### [소켓 서버]

- **Redis** 에서 Session Key 를 활용하여 로그인 처리함으로 **API Server 와 결합도 낮춤**
- **비동기 TCP 소켓 통신** 기능
- Protobuf 를 활용한 Http Body 직렬화/역직렬화 하여 **JSON 대비 패킷 2/3 절감**
- 각 유저 메시지를 **Swap Queue 구조**로 구현하여 Lock 최소화
- 최대 동접자를 관리 할 수 있도록 **유저 대기열 구현**
- **중복 로그인 체크** 하여 기존 로그인 Close 구현
- DB 작업은 Queue에 담아 **Multi thread** 로 처리

<br/>


<br/>

## 프로젝트 아키텍처

<img src="https://github.com/hstar0124/hstar-project/assets/57317290/cdd2701d-590f-49ac-83a5-f8d310cf3005" width="650" height="460"/>


**1. Boost.Asio를 사용한 이유**
- Boost 라이브러리는 풍부한 기능과 유연성을 제공
- Asio 라이브러리는 이 Boost 생태계 일부로 표준화될 가능성이 높음
- Boost.Asio는 다양한 플랫폼에서 동작하기 때문에 다양한 환경에서 배포할 때 큰 장점
- Boost.Asio는 사용사례가 많기 때문에 이슈가 있을 때 해결방법을 찾기 쉬움

**2. Protobuf를 사용한 이유**
- 빠르고 효율적인 데이터 직렬화로 네트워크 트래픽을 최적화
- Protobuf는 여러 언어와 플랫폼에서 사용 가능
- Google에서 개발하고 오랜 기간 동안 사용하는 데에서 비롯된 안정성이 보장
- 간단하고 명확한 인터페이스로 직관적으로 개발 가능

**3. Redis 사용 이유**
- API 서버와 Socket 서버의 결합도를 낮출수 있음
- Single Thread 이지만 Key-Value 방식으로 빠른 조회가 가능한 것이 큰 장점
- 트래픽이 많이 몰렸을 때 Scale Out을 빠르게 대처 가능

**4. MySQL 사용 이유**
- 오픈 소스 라이센스로 제공
- 수십 년간 개발과 개선이 되어 안정적이고 신뢰도가 높음
- 광범위한 사용자 커뮤니티가 있어 이슈 발생시 해결방법 찾기 쉬움


<br/>

## Socket 서버 구조도

<img src="https://github.com/hstar0124/hstar-project/assets/57317290/ca66e1de-bfc0-4d00-ae74-64ec5dfac3a9" width="850" height="350"/>

<br/>

## 주요 기능

### [API 서버]
- 로그인
- 회원가입
- 회원 정보 조회
- 회원 정보 변경
- 회원 탈퇴
  
### [소켓 서버]
- 전체 채팅
- 귓속말 채팅
- 파티 생성/가입/탈퇴/삭제
- 친구 신청/수락
  
<br/>

## 주요 흐름도

### [접속_흐름도]

![접속_흐름도](https://github.com/hstar0124/hstar-project/assets/57317290/c510c2d1-6e41-4748-89fd-976ba140731b)


### [메시지 발송]

![메시지_발송](https://github.com/hstar0124/hstar-project/assets/57317290/bb31bdf8-0c1e-4793-94b2-b349337bba86)


### [친구추가 흐름도]

![친구추가_흐름도](https://github.com/hstar0124/hstar-project/assets/57317290/b5f72f61-0adc-49ac-b38b-d482020714a1)

<br/>

## 코드 설명

### Socket Server

**TcpServer**

- Boost Asio io_context: io_context 객체를 사용하여 비동기 I/O 처리
- **Acceptor**: 클라이언트의 연결 요청을 수락하기 위해 acceptor를 관리
- **비동기 읽기/쓰기**: 클라이언트와의 데이터 송수신을 비동기적으로 처리
- **DB 연동**: MySQL 및 Redis 데이터베이스와의 연결 관리
  - 각 DB 객체는 std::unique_ptr로 관리
- **UserSession 관리**: 클라이언트와의 세션을 생성하고 처리


**UserSession**

- Boost Asio를 사용하여 TCP 소켓을 통해 클라이언트와 통신
- **세션 상태**: 활성화 여부 및 클라이언트 연결 상태 관리
- **큐 관리**: 입력 및 출력 메시지 큐 관리 및 교체
- **Ping**: 주기적인 ping 메시지 전송 및 타이머 관리
- **메시지 송수신**:
  - 비동기적으로 메시지 읽기 및 쓰기 처리
  - 메시지 직렬화 및 역직렬화
- **에러 처리**: 오류 발생 시 세션 종료 및 로그 기록


**PartyManager**

- **파티 관리**: unordered_map 을 사용하여 파티 생성/가입/탈퇴/삭제 관리
  - 파티 이름 중복 체크
  - 파티 가입 중복 체크
- User Session 은 Shared_ptr 로 받아서 처리


**Party**

- 파티에 대한 정보 저장 및 파티 멤버 관리
- **파티 멤버 관리**: 파티 멤버는 Vector 로 관리


**PacketConverter**

- 프로토콜 버퍼 메시지와 바이트 버퍼 간의 직렬화 및 역직렬화
- 메시지 크기를 바이트 버퍼 헤더에 설정


**HSThreadPool**

- 스레드 풀의 스레드 개수 관리
- 워커 스레드 생성 및 관리
- 작업 큐에 작업 추가 및 실행
  - 작업 추가 시 `std::future`로 결과 반환
- 스레드 안전을 위한 조건 변수와 뮤텍스 사용
- 스레드 풀 종료 시 모든 스레드 종료 대기


**Logger**

- 다양한 로그 레벨 및 로그 출력 옵션 지원
- **로그 레벨**: DEBUG, INFO, WARN, ERROR
- **로그 출력**:
  - **ConsoleOutput**: 콘솔에 로그 메시지 출력
  - **FileOutput**: 파일에 로그 메시지 저장, 파일명은 기간에 따라 변경 (년, 월, 일 단위)
- **싱글턴 패턴**: `Logger::instance()`로 전역적으로 접근 가능
- **로그 포맷**: 날짜/시간, 로그 레벨, 메시지, 소스 파일 및 라인 정보 포함


**ConfigParser**

- 설정 파일을 읽어와 설정 항목을 맵에 저장
- 파일에서 키-값 쌍을 추출하여 `std::map<std::string, std::string>`에 저장
- 설정 값을 반환하는 함수 제공


### Client

**Client**

- Api URL 로드 및 관리
- Http Client, Socket Client 초기화 관리
- User Interface

**ChatClient**

- Boost Asio io_context: io_context 객체를 사용하여 비동기 I/O 처리
- **비동기 읽기/쓰기**: 클라이언트와의 데이터 송수신을 비동기적으로 처리
- **Message 처리**: 유저 입력에 따른 메시지 타입 분류 및 처리
  - 일반 메시지 / 귓속말 메시지 / 파티 메시지


**HttpClient**

- **HTTP 요청 및 응답처리**: 유저 입력에 따라 API 요청이 필요한 경우 API Server 로 요청 및 응답 처리
  - 로그인 요청의 경우 POST로 요청을 보내며 메시지 protobuf로 직렬화하여 진행
  - 간단한 정보 요청은 GET으로 처리



### Api Server

**UserController**

- 생성자에서 ILogger<UserController>, IUserWriteService, IUserReadService를 주입받아 사용
- **주요 메서드**:
  - GetUser: userId를 쿼리 파라미터로 받아 사용자 정보를 조회
  - Create: 요청 본문으로 받은 CreateUserRequest를 통해 새로운 사용자를 생성
  - Login: 요청 본문으로 받은 LoginRequest를 통해 사용자의 로그인을 처리
  - Update: 요청 본문으로 받은 UpdateUserRequest를 통해 사용자의 정보를 업데이트
  - Delete: 요청 본문으로 받은 DeleteUserRequest를 통해 사용자를 삭제
- **CQRS 패턴 적용**: 명령 처리와 조회 처리를 분리하여 각 역할에 맞는 서비스를 사용
  - IUserWriteService: 사용자 관련 명령 처리 (생성, 로그인, 업데이트, 삭제)
  - IUserReadService: 사용자 조회 처리

**UserWriteService**

- IUserWriteService 인터페이스를 구현
- 프로세스에 따라 Redis 또는 MySQL DB 나눠서 처리 진행
- **주요 메서드**:
  - LoginUser: 사용자 자격 증명을 검증하고 세션을 생성하여 로그인 성공 응답을 반환
  - CreateUser: 새 사용자 정보를 받아 비밀번호를 해시하여 사용자 생성 결과를 반환
  - UpdateUser: 사용자 정보를 검증한 후, 요청된 필드를 업데이트
  - DeleteUser: 사용자 정보를 검증하고 사용자를 소프트 삭제하여 결과를 반환
 
**UserReadService**

- IUserReadService 인터페이스 구현
- **주요 메서드**:
  - GetUserFromUserid: 사용자 ID를 기반으로 사용자 정보를 조회하고, 성공 시 사용자 정보를 포함한 응답을 반환. 실패 시 로그를 기록하고 UserNotFoundException 발생.

**UserRepositoryFromMysql**

- Async-await로 DB 비동기 처리 
- **Entity Framework**: DbContext인 HStarContext를 사용하여 데이터베이스 작업을 수행
- **주요 메서드**:
  - CreateUser: 사용자를 데이터베이스에 생성. 사용자 ID가 이미 존재하면 실패를 반환.
  - GetUserFromUserid: 사용자 ID로 사용자 정보를 조회. 사용자 존재 여부와 활성 상태를 확인.
  - ValidateUserCredentials: 사용자 자격 증명을 검증. 비밀번호와 사용자 상태를 확인.
  - UpdateUser: 사용자의 정보를 업데이트. 사용자가 존재하지 않으면 실패를 반환.
  - DeleteUser: 사용자의 상태를 '삭제됨'으로 변경. 사용자가 존재하지 않으면 실패를 반환.

**CacheRepositoryFromRedis**

- Async-await로 DB 비동기 처리 
- **Redis 연결**: ConnectionMultiplexer를 사용하여 Redis에 연결하고, 세션 데이터베이스를 초기화
- **주요 메서드**:
  - CreateSession: 새로운 세션을 생성하고 Redis에 저장합니다. 기존 세션이 있으면 삭제 후 새 세션을 저장합니다.
  - KeepAliveSessionFromAccountId: 세션을 갱신하여 유효 기간을 연장합니다.
  - DeleteSessionFromAccountId: 사용자 계정과 연결된 세션을 삭제합니다.



**PasswordHelper**

- 사용자 비밀번호를 안전하게 해시하고 검증하는 기능을 제공
- **주요 메서드**:
  - HashPassword: 주어진 비밀번호를 해시하여 안전한 문자열로 변환
  - VerifyPassword : 사용자로부터 입력된 비밀번호와 해시화된 비밀번호 검증


<br/>

## Getting Started

### 소스 다운로드

github develop 브런치에서 소스 [다운로드](https://github.com/hstar0124/hstar-project/archive/refs/heads/develop.zip) 이후 압축해제

https://github.com/hstar0124/hstar-project

![image](https://github.com/hstar0124/hstar-project/assets/57317290/e07f0cc9-88f1-42c0-b284-d0509463f9d9)

![image](https://github.com/hstar0124/hstar-project/assets/57317290/c4bc0df2-a77f-44dd-b465-ba1c1c5d7ffe)

### 라이브러리 다운로드

dropbox 에서 관련 라이브러리 [다운로드](https://www.dropbox.com/scl/fi/d40xvoxuj9pbzt6njkkyo/Libraries.zip?rlkey=pz0fj9rviodcz6jby4etphf0x&st=ojmfume1&dl=0) 이후 압축해제

압축 해제할 때 폴더 구조는 아래와 같이 진행
```
- ApiServer
- Client
- Libraries
  └ include
  └ libs
- Proto
- Server
- ...
```

### DB 세팅 

로컬에 Mysql, Redis 가 기본포트로 기동되어 있어야 한다.
MySQL DB 명은 hstar로 기본적으로 접속 시도

[DB Script](https://github.com/hstar0124/hstar-project/wiki/MySQL-DB-Script)

### Solution 세팅 및 기동

root 폴더에 있는 HStarProject.sln 을 실행시키면 Visual Studio가 열리게 된다. 
열리면 Debug 모드로 빌드 후 테스트 진행하면 된다.

![image](https://github.com/hstar0124/hstar-project/assets/57317290/d012dfd5-3af6-44f1-b5e3-effb098db525)

Multiple startup projects 세팅은 아래와 같이 세팅 후 진행하면 오류 없이 잘 실행된다.

![image](https://github.com/hstar0124/hstar-project/assets/57317290/0bee494a-c4f7-44f9-a024-6a56deb30271)
