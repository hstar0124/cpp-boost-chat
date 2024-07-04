# HStar Project 
Asp.net Core 와 Protobuf를 활용한 API Server
Boost.Asio 와 Protobuf를 활용한 Socket Server

## 프로젝트 소개

게임 서버에서 중요한 것은 **빠른 반응속도**와 **다양한 환경과 연결**이라고 생각합니다.

로그인 및 유저 정보 관리는 ASP.net Core API 서버로 구현하였으며, **MVC 패턴**을 적용하여, 가독성과 유지보수성을 생각했습니다.
또한 EntityFrameWorkCore를 활용하여 **생산성**을 높였습니다.

**빠른 반응속도**를 생각하여 C++로 소켓서버를 구성하였습니다.
**비동기 I/O에 특화**된 Boost.Asio를 활용하여 소켓 통신 기능을 구현 하였으며,
Protobuf를 통해 메시지 직렬화/역직렬화를 진행하여 JSON 대비 **패킷 사이즈를 2/3 절감** 했습니다.



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



## 주요 특장점

**[API 서버]**
- 로그인/유저 CRUD 기능 제공
- **Protobuf** 를 활용한 Http Body 직렬화/역직렬화 하여 **JSON 대비 패킷 2/3** 절감
- Password 를 **Hash + Salt** 하여 개인정보 보호
- Service를 **Read / Write 부분을 분리**하여 구현함으로 **복잡성 관리**
- MVC 패턴을 적용하여 **유지보수성, 확장성, 유연성 확보**
- EntityFrameworkCore 를 적용하여 **생산성 향상**

**[소켓 서버]**
- Redis 에서 **Session Key** 를 활용하여 로그인 처리함으로 **API Server 와 결합도** 낮춤
- **비동기 TCP 소켓** 통신 기능
- **Protobuf** 를 활용한 Http Body 직렬화/역직렬화 하여 **JSON 대비 패킷 2/3** 절감
- 각 유저 메시지를 Swap Queue 구조로 구현하여 **Lock 최소화**
- 최대 동접자를 관리 할 수 있도록 **유저 대기열 구현**
- **중복 로그인 체크** 하여 기존 로그인 Close 구현



## 프로젝트 아키텍처

<img src="https://github.com/hstar0124/hstar-project/assets/57317290/cdd2701d-590f-49ac-83a5-f8d310cf3005" width="750" height="560"/>



## Socket 서버 구조도

<img src="https://github.com/hstar0124/hstar-project/assets/57317290/ca66e1de-bfc0-4d00-ae74-64ec5dfac3a9" width="850" height="400"/>


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



## 기술 스택

- C++ 17
- Boost 1.84.0
- Protobuf 3.14.0
- Redis
- Mysql
- Visual Studio 2022
- Windows 10
- Github
