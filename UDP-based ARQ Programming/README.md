# UDP-based ARQ Programming
- **운영체제**: Windows 11
- **개발 도구**: Visual Studio 또는 GCC를 지원하는 Windows용 C 컴파일러
- **네트워크 라이브러리**: WinSock2
## Contents
User Datagram Protocol(UDP) 기반의 Stop-and-Wait ARQ를 적용한 클라이언트-서버 모델을 구현한다. Packet loss가 발생하지 않을 때, 즉 정상적으로 작동하는 상황에서 서버는 클라이언트로부터 수신한 메시지를 echo하고, packet loss가 발생할 때는 메시지를 echo하지 않고, 메시지 수신을 대기한다. 클라이언트는 timeout을 설정하여 일정 시간 동안 서버로부터 응답을 받지 못하면 메시지를 재전송한다. 클라이언트와 통신 종료 시, 서버는 클라이언트가 전송한 메시지 수(N1), 클라이언트가 재전송한 메시지 수(N2), 전체 메시지 수(N3), 그리고 재전송 빙율 값(R)에 대한 통계 데이터를 출력한다. 

## Syntax
### UDP Header
![image](https://github.com/user-attachments/assets/078b6a51-c403-46b4-a1fb-5197a680e48f)

8 바이트 크기의 UDP 헤더는 Source Port, Destination Port, Length, 그리고 Checksum으로 이루어진다. 
- Source Port: 송신자의 포트 번호이다.
- Destination Port: 수신자의 포트 번호로, 외부 입력으로 결정된다.
- Length: 전체 UDP 패킷의 길이를 나타낸다.
- Checksum: 데이터의 무결성을 검증하기 위한 값이다.

### UDP Data
![image](https://github.com/user-attachments/assets/59de04ad-31b3-44b5-97b0-ff2a4521a066)

UDP 데이터는 2 바이트 크기의 sequence number와 가변 길이의 문자열인 메시지로 이루어진다. 클라이언트가 전송하는 packet의 초기 sequence number는 0으로 설정하며, 그 후의 sequence number는 이전 sequence number와 이전 메시지의 입력 문자열의 문자 개수의 합과 같다. 이때, 입력 문자열은 프로그램에서 자동으로 생성되거나, 또는 외부로부터 입력을 받아 생성된다. 가변 길이인 문자열의 최대 크기는 512 바이트로 제한하며, 전송 및 수신할 수 있는 packet의 최대 크기는 2바이트의 sequence number과 512 바이트의 문자열, 그리고 문자열의 끝을 나타내는 ‘\0’을 합하여 515 바이트이다. 서버가 클라이언트의 메시지를 수신 후 전송하는 echo packet의 acknowledgement number는 수신한 packet의 sequence number와 같고, 전송 및 수신할 수 있는 packet의 최대 크기는 클라이언트와 같다. 
## Semantics
### Client
클라이언트가 메시지 구성 후 서버로 전송하고, 서버로부터 echo 메시지 수신을 대기한다. Timeout 전에 echo 메시지를 수신하면, 이를 터미널에 출력하고 다음 메시지 전송 및 수신 대기를 반복한다. Timeout이 발생한다면, 해당 메시지를 서버로 재전송하고, echo 메시지 수신을 대기한다.

![image](https://github.com/user-attachments/assets/3a2e557c-4332-460a-b517-504b6aefb1b0)
![image](https://github.com/user-attachments/assets/9860e06a-2995-4aa5-8893-e2224e84a63f)

클라이언트가 종료를 원할 경우, 메시지에 “QUIT” 또는 “quit”을 입력함으로써 종료할 수 있다. 이때 클라이언트는 서버에게 종료 메시지를 전송하고, 위의 상황과 마찬가지로 echo 메시지 수신을 대기한다. Timeout mechanism 또한 동일하게 적용되고, echo 메시지 수신 시 클라이언트는 프로그램을 종료한다.

![image](https://github.com/user-attachments/assets/60fea807-4c06-4356-9d83-a758325d9050)

### Server
서버는 클라이언트로부터 메시지 수신을 대기하고, 메시지를 수신하면 해당 메시지를 터미널에 출력하고, echo 상황을 결정한다. 프로그램을 시작할 때 설정한 확률 p와 매번 메시지 수신 시 생성한 0과 1 사이의 임의의 수 random number에 대하여, random number가 p보다 작거나 같으면 수신한 메시지를 echo한다. 이와 반대로 random number가 p보다 크다면, 해당 상황을 packet loss가 발생한 상황으로 판단하고, 계속해서 클라이언트로부터 메시지 수신을 대기한다. 

![image](https://github.com/user-attachments/assets/e1f0330b-a269-4805-98cd-2b04d280b428)

서버가 클라이언트로부터 수신한 메시지가 “QUIT” 또는 “quit”인 경우, 위와 마찬가지로 random number을 생성하고 확률 p와 비교하여 echo 메시지 전송 또는 손실 처리를 결정한다. 종료 메시지를 echo하게 되는 경우, echo 메시지 전송 후, 통계 데이터를 터미널에 출력한다.

![image](https://github.com/user-attachments/assets/85bd4052-1929-4f03-a383-614dbbc11fdf)

## 주요 코드
### Client
- 제한 시간 설정
- 소켓 생성
- 서버 IP 주소 입력
- 소켓 주소 구조체 초기화
- 난수 생성 및 랜덤 메시지 생성 (난수 범위: 1 ~ BUFSIZE) (Random Input)
- 메시지 입력 (Keyboard Input)
- 메시지 전송
- 타임아웃 설정
- 메시지 수신
- 타임 아웃 또는 packet loss 발생 시 메시지 재전송
- 종료 메시지 전송 (Random Input) 
### Server
- 소켓 생성
- 소켓 주소 구조체 초기화 및 바인딩
- p값 설정
- 메시지 수신
- 확률값 생성
- 메시지 echo 처리
- 메시지 손실 처리 
## Key Points
