# Chat 
Boost.Asio 와 Protobuf를 활용한 채팅 서버

## 프로젝트 소개
게임 서버에서 중요한 것은 빠른 반응 속도와 다양한 환경과 연결이라고 생각합니다.
비동기 I/O에 특화된 Boost.Asio를 활용하여 소켓 통신 기능을 구현 하였으며,
Protobuf를 통해 메시지 직렬화/역직렬화를 진행하였습니다. 

[Boost.Asio를 사용한 이유]
- Boost 라이브러리는 풍부한 기능과 유연성을 제공
- Asio 라이브러리는 이 Boost 생태계 일부로 표준화될 가능성이 높음
- Boost.Asio는 다양한 플랫폼에서 동작하기 때문에 다양한 환경에서 배포할 때 큰 장점
- Boost.Asio는 사용사례가 많기 때문에 이슈가 있을 때 해결방법을 찾기 쉬움

[Protobuf를 사용한 이유]
- 빠르고 효율적인 데이터 직렬화로 네트워크 트래픽을 최적화
- Protobuf는 여러 언어와 플랫폼에서 사용 가능
- Google에서 개발하고 오랜 기간 동안 사용하는 데에서 비롯된 안정성이 보장
- 간단하고 명확한 인터페이스로 직관적으로 개발 가능


## 주요 기능
- 비동기 TCP 소켓 통신 기능
- Protobuf 를 활용한 직렬화/역직렬화 기능
- 메시지를 Queue에 담아 처리하는 기능
- 클라이언트 간 전체 채팅 기능
- 클라이언트 간 귓속말 채팅 기능
- 파티 생성/삭제/가입/탈퇴 기능
- 파티채팅 기능

## 구조도

![구조도](https://github.com/hstar0124/cpp-boost-chat/assets/57317290/b44e8007-abdb-4813-83af-1efc1982ba02)


## 흐름도

[연결 및 일반 메시지 발송]

![연결 및 일반 메시지 발송](https://github.com/hstar0124/cpp-boost-chat/assets/57317290/d92238d0-b330-4cc6-a77a-b2511d2ac2aa)




[파티 생성 및 파티 메시지 발송]

![파티 생성 및 파티 메시지 발송](https://github.com/hstar0124/cpp-boost-chat/assets/57317290/4daf5315-d7b2-4974-b262-a25b9f603e4c)



## 기술 스택

- C++ 17
- Boost 1.84.0
- Protobuf 3.14.0
- Visual Studio 2022
- Windows 10
- Github
