# UDP Socket Programming
* **운영체제**: Windows 11
* **개발 도구**: Visual Studio 또는 GCC를 지원하는 Windows용 C 컴파일러
* **네트워크 라이브러리**: WinSock2
## Contents
### Both Client and Server
- 서버 IP 주소와 포트 번호를 외부 입력으로 받아들여서 처리한다.
- 수신한 메시지를 전송 시, "(IP 주소: 포트 번호)가 (IP 주소: 포트 번호)로부터 (N)바이트 메시지 수신: 수신한 메시지 내용" 형식으로 만든다.
### Client
- 메시지 전송 시 다음의 4가지 형식을 지정할 수 있다.
  1. **echo**: 서버는 클라이언트로부터의 메시지를 echo 한다.
  2. **chat**: 서버는 클라이언트에게 답장 메시지를 보낸다.
  3. **stat**: 서버는 클라이언트가 요구하는 다음 메시지의 대한 통계 정보를 보낸다.
     - **bytes**: 이때까지 서버가 수신한 메시지의 바이트 수의 총 합계.
     - **number**: 클라이언트가 보낸 메시지의 총 개수.
     - **both**: bytes와 number 모두 포함.
  4. **quit**: 서버와 클라이언트는 모두 소켓을 close 하고 종료한다.
- 매번 메시지 전송 전에 클라이언트는 위의 4가지 동작 중 하나를 선택한다.
### Server
- 클라이언트가 요구하는 형식에 맞추어 동작한다.
## Syntax
![image](https://github.com/user-attachments/assets/e26a4dce-78fd-47cc-b97e-189da8d35b8e)

8 바이트 크기의 UDP 헤더는 Source Port, Destination Port, Length, 그리고 Checksum으로 이루어진다.
- Source Port: 송신자의 포트 번호이다.
- Destination Port: 수신자의 포트 번호로, 외부 입력으로 결정된다.
- Length: 전체 UDP 패킷의 길이를 나타낸다.
- Checksum: 데이터의 무결성을 검증하기 위한 값이다.

![image](https://github.com/user-attachments/assets/33ead910-568f-4ee8-a667-c784f60246cf)

UDP 데이터는 2 바이트 크기의 hexadecimal 값이 Command Code와 가변 길이의 문자열인 메시지로 이루어진다. 클라이언트와 서버 모두 전송하고 수신할 수 있는 데이터의 최대 크기는 512 바이트로 제한한다.
## Semantics
클라이언트가 서버에게 데이터를 전송할 때, 데이터의 Command Code는 클라이언트가 선택한 메시지 형식에 따라 달라진다. 
#### echo
Command Code는 0x01이다. 클라이언트는 서버에게 보낼 메시지 내용을 입력 받고, 서버는 클라이언트로부터 수신한 메시지 그대로 클라이언트에게 전송한다.

![image](https://github.com/user-attachments/assets/2d1dc692-d48f-43f2-89e5-6811354d33cd)

#### chat
Command Code는 0x02이다. 클라이언트는 서버에게 보낼 메시지 내용을 외부로부터 입력 받는다. 서버는 메시지 수신 후, 보낼 메시지 내용을 외부 입력으로 받아들이고, 클라이언트에게 전송한다.

![image](https://github.com/user-attachments/assets/49356d7c-54db-46d3-bf3b-04f2dba2a9d8)

#### stat
Command Code는 0x03이다. 클라이언트는 외부 입력으로 받아들인 stat 명령어와 일치하는 메시지를 전송하고, 서버는 수신한 메시지에 해당하는 현재까지의 통계 데이터를 클라이언트에게 전송한다. 

![image](https://github.com/user-attachments/assets/c9c3a5b7-82af-4ddf-af20-840a10491fda)

#### quit
Command Code는 0x04이다. 클라이언트는 메시지 내용 없이 서버에게 데이터를 전송한다.

![image](https://github.com/user-attachments/assets/7f508c6f-ca0c-49bb-b8e9-d3296084f98b)

## 주요 코드
### Client
- 소켓 생성
```c
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed\n");
        exit(1);
    }
```
- 서버의 IP 주소 및 포트 번호 입력
```c
    char serverIP[20] = "";
    int PORT = 0;
    printf("Input Server IP : ");
    scanf("%s", serverIP);
    printf("Input Server Port : ");
    scanf("%d", &PORT);
    getchar();
```
- 소켓 주소 구조체 초기화
```c
    SOCKADDR_IN serverAddr = { 0 };
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "inet_addr() failed\n");
        exit(1);
    }
    serverAddr.sin_port = htons(PORT);
```
- Command Code 복사
```c
            command_code = htons(0x01);
            memcpy(message, &command_code, sizeof(command_code));
```
- 메시지 전송
```c
            retval = sendto(s, message, totalLength, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "sendto() failed\n");
                continue;
            }
            else {
                printf("Send Message!\n\n");
            }
```
- 메시지 수신
```c
            clientAddrLength = sizeof(clientAddr);
            retval = recvfrom(s, message, BUFSIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrLength);
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "recvfrom() failed\n");
                continue;
            }
            if (retval < BUFSIZE) {
                message[retval] = '\0';
            }
```
### Server
- 포트 번호 설정
```c
    int PORT = 0;
    printf("Input Server Port : ");
    scanf("%d", &PORT);
    while (getchar() != '\n');
```
- 소켓 생성
```c
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed\n");
        exit(1);
    }
```
- 소켓 주소 구조체 초기화 및 바인딩
```c
    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);
    retval = bind(s, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (retval == SOCKET_ERROR) {
        fprintf(stderr, "bind() failed\n");
        exit(1);
    }
```
- 호스트 이름 추출
```c
    char hostname[256];
    retval = gethostname(hostname, sizeof(hostname));
    if (retval == SOCKET_ERROR) {
        fprintf(stderr, "gethostname() failed\n");
        exit(1);
    }
```
- 호스트 이름을 기반으로 IP 주소 구하기
```c
    struct addrinfo hints, * res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

```
- 메시지 수신
```c
        clientAddrLength = sizeof(clientAddr);
        retval = recvfrom(s, message, BUFSIZE, 0, (SOCKADDR*)&clientAddr, &clientAddrLength);
        if (retval == SOCKET_ERROR) {
            fprintf(stderr, "recvfrom() failed\n");
            continue;
        }
        if (retval < BUFSIZE) {
            message[retval] = '\0';
        }
```
- Command Code 추출
```c
        memcpy(&command_code, message, sizeof(command_code));
        command_code = ntohs(*(unsigned short*)message);
```
- 메시지 전송
```c
        retval = sendto(s, response, strlen(response), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
        if (retval == SOCKET_ERROR) {
            fprintf(stderr, "sendto() failed\n");
            continue;
        }
```
## 동작 과정
### Client
1. 클라이언트는 Winsock을 초기화하고 UDP 소켓을 생성한다.
2. 서버의 IP 주소와 포트 번호를 외부 입력으로 받아들인다.
3. 명령어를 입력 받아 처리하는 루프에 진입한다.
4. 각 명령어에 대한 동작
    * **echo**: 클라이언트가 입력한 메시지를 서버에 전송하고, 서버로부터 전송한 메시지와 동일한 메시지를 수신한다.
    * **chat**: 클라이언트가 입력한 메시지를 보내면, 서버는 응답 메시지를 작성해 클라이언트로 전송한다.
    * **stat**: 클라이언트가 bytes, number, both(또는 기타) 중 하나의 명령을 보내면 서버는 해당 통계 데이터를 반환한다.
    * **quit**: 클라이언트는 종료 명령을 보내고 통신을 종료한다.
### Server
1. 서버는 Winsock을 초기화하고 UDP 소켓을 생성한다.
2. 서버는 지정된 포트 번호로 소켓을 바인딩하여 클라이언트의 메시지를 받을 준비를 한다.
3. 서버는 클라이언트로부터 메시지를 수신하며, 수신된 메시지의 명령어 코드를 분석하여 동작을 수행한다.
4. 각 명령어에 대한 동작
     * **echo**: 클라이이언트가 전송한 메시지를 그대로 클라이언트에게 전송한다.
     * **chat**: 서버가 클라이언트로부터 받은 메시지를 출력한 후, 사용자로부터 응답 메시지를 입력 받아 클라이언트로 전송한다.
     * **stat**: 서버는 누적된 통계 데이터를 클라이언트에게 전송한다. 클라이언트는 bytes, number, both(또는 기타)에 따라 서버로부터 총 바이트 수, 메시지 수, 또는 총 바이트 수와 메시지 수 모두를 받을 수 있다.
     * **quit**: 클라이언트가 종료 명령을 보내면 서버는 소켓을 닫고 프로그램을 종료한다.
