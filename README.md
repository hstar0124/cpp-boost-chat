# HStar Project 
Asp.net Core 와 Protobuf를 활용한 API Server
Boost.Asio 와 Protobuf를 활용한 Socket Server

## 프로젝트 소개

현대 게임 산업에서 실시간 상호작용과 빠른 반응 속도는 필수적인 요소로 자리 잡았습니다. 
특히, 멀티플레이어 게임과 실시간 스트리밍 게임의 인기가 높아짐에 따라, 게임 서버의 성능과 안정성은 사용자 경험에 직접적인 영향을 미치고 있습니다. 
이러한 트렌드에 맞춰, 본 프로젝트는 고성능 게임 서버를 구축하는 것을 목표로 하였으며, 로그인 및 유저 관리는 C# API 서버로, 실시간 소켓 통신은 C++로 구현하였습니다.

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
**Protobuf**를 사용한 메시지 직렬화/역직렬화는 **JSON 대비 패킷 사이즈를 약 2/3 절감**시켜 네트워크 대역폭을 효율적으로 활용할 수 있게 해주었습니다.



**[Boost.Asio를 사용한 이유]**
- Boost 라이브러리는 풍부한 기능과 유연성을 제공
- Asio 라이브러리는 이 Boost 생태계 일부로 표준화될 가능성이 높음
- Boost.Asio는 다양한 플랫폼에서 동작하기 때문에 다양한 환경에서 배포할 때 큰 장점
- Boost.Asio는 사용사례가 많기 때문에 이슈가 있을 때 해결방법을 찾기 쉬움



**[Protobuf를 사용한 이유]**
- 빠르고 효율적인 데이터 직렬화로 네트워크 트래픽을 최적화
- Protobuf는 여러 언어와 플랫폼에서 사용 가능
- Google에서 개발하고 오랜 기간 동안 사용하는 데에서 비롯된 안정성이 보장
- 간단하고 명확한 인터페이스로 직관적으로 개발 가능


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
  

## 주요 특장점

**[API 서버]**

- 로그인/유저 CRUD 기능 제공
- Protobuf 를 활용한 Http Body 직렬화/역직렬화 하여 **JSON 대비 패킷 2/3 절감**
- Password 를 **Hash + Salt 하여 개인정보 보호**
- Service를 Read / Write 부분을 분리하여 구현함으로 복잡성 관리
- **MVC 패턴**을 적용하여 **유지보수성, 확장성, 유연성** 확보
- **EntityFrameworkCore** 를 적용하여 **생산성** 향상

**[소켓 서버]**

- **Redis** 에서 Session Key 를 활용하여 로그인 처리함으로** API Server 와 결합도 낮춤**
- **비동기 TCP 소켓 통신** 기능
- Protobuf 를 활용한 Http Body 직렬화/역직렬화 하여 **JSON 대비 패킷 2/3 절감**
- 각 유저 메시지를 **Swap Queue 구조**로 구현하여 Lock 최소화
- 최대 동접자를 관리 할 수 있도록 **유저 대기열 구현**
- **중복 로그인 체크** 하여 기존 로그인 Close 구현
- DB 작업은 Queue에 담아 **Multi thread** 로 처리



## 프로젝트 아키텍처

<img src="https://github.com/hstar0124/hstar-project/assets/57317290/cdd2701d-590f-49ac-83a5-f8d310cf3005" width="650" height="460"/>



## Socket 서버 구조도

<img src="https://github.com/hstar0124/hstar-project/assets/57317290/ca66e1de-bfc0-4d00-ae74-64ec5dfac3a9" width="850" height="350"/>


## 주요 기능

**[API 서버]**
- 로그인
- 회원가입
- 회원 정보 조회
- 회원 정보 변경
- 회원 탈퇴

**[소켓 서버]**
- 전체 채팅
- 귓속말 채팅
- 파티 생성/가입/탈퇴/삭제
- 친구 신청/수락
  

## 주요 흐름도

**[접속_흐름도]**

![접속_흐름도](https://github.com/hstar0124/hstar-project/assets/57317290/c510c2d1-6e41-4748-89fd-976ba140731b)


**[메시지 발송]**

![메시지_발송](https://github.com/hstar0124/hstar-project/assets/57317290/bb31bdf8-0c1e-4793-94b2-b349337bba86)


**[친구추가 흐름도]**

![친구추가_흐름도](https://github.com/hstar0124/hstar-project/assets/57317290/b5f72f61-0adc-49ac-b38b-d482020714a1)

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

로컬에 Mysql, Redis 가 기동(두 DB 모두 기본 포트로 기동)
MySQL DB 명은 hstar로 기본적으로 접속 시도

[DB Script](https://github.com/hstar0124/hstar-project/wiki/MySQL-DB-Script)

### Solution 세팅 및 기동

root 폴더에 있는 HStarProject.sln 을 실행시키면 Visual Studio가 열리게 된다. 
열리면 Debug 모드로 빌드 후 테스트 진행하면 된다.

![image](https://github.com/hstar0124/hstar-project/assets/57317290/d012dfd5-3af6-44f1-b5e3-effb098db525)

Multiple startup projects 세팅은 아래와 같이 세팅 후 진행하면 오류 없이 잘 실행된다.

![image](https://github.com/hstar0124/hstar-project/assets/57317290/0bee494a-c4f7-44f9-a024-6a56deb30271)
