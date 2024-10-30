# UDP-based ARQ Programming
- **운영체제**: Windows 11
- **개발 도구**: Visual Studio 또는 GCC를 지원하는 Windows용 C 컴파일러
- **네트워크 라이브러리**: WinSock2
## Contents
User Datagram Protocol(UDP) 기반의 Stop-and-Wait ARQ를 적용한 클라이언트-서버 모델을 구현한다. Packet loss가 발생하지 않을 때, 즉 정상적으로 작동하는 상황에서 서버는 클라이언트로부터 수신한 메시지를 echo하고, packet loss가 발생할 때는 메시지를 echo하지 않고, 메시지 수신을 대기한다. 클라이언트는 timeout을 설정하여 일정 시간 동안 서버로부터 응답을 받지 못하면 메시지를 재전송한다.

## Syntax
### UDP Header

### UDP Data

## Semantics

## 주요 코드

## Key Points
