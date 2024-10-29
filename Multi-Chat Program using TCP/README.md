# Multi-Chat Program using TCP
* **운영체제**: Windows 11
* **개발 도구**: Visual Studio 또는 GCC를 지원하는 Windows용 C 컴파일러
* **네트워크 라이브러리**: WinSock2
## Contents
* UDP의 가장 큰 특징 중 하나는 connectionless라는 점이기 때문에, 기존 환경에서는 클라이언트가 빠져나가나는 것을 확인하기 어렵다는 문제점이 있다.
* TCP의 경우 connection-oriented라는 특징을 가지므로, 클라이언트가 접속하고 종료하는 것을 확인하는 것이 가능하다.
### Clients
* 클라이언트 프로그램에서는 메시지 전송과 수신이 동시에 이루어진다.
* 메시지 전송
  * 클라이언트는 접속 후 자신의 닉네임을 설정한다.
  * 그 후, 메시지를 입력하고 서버로 전송한다. 이때 메시지 형식은 '[닉네임]채팅 메시지'이다.
* 메시지 수신
  * 서버로부터 수신한 메시지를 그대로 출력한다.
### Server
* 서버 프로그램은 클라이언트로부터 수신한 메시지를 다른 클라이언트들에게 그대로 전송한다. (Echo)
  * 이때, 해당 메시지를 전송한 클라이언트에게는 보내지 않는다.
* 메시지 전송과 동시에 서버는 다음과 같은 정보를 제공한다.
  1. 접속 중인 클라이언트 정보
     * 접속 클라이언트 수와 함께 각 클라이언트의 닉네임, IP 주소, 포트 번호, 그리고 클라이언트의 접속 여부를 표시한다.
  2. 채팅 통계
     * 1분 당 평균 메시지 수에 대한 통계 데이터를 제공한다.
  3. 채팅 종료
## Syntax
### TCP Header
![image](https://github.com/user-attachments/assets/06e73876-ce54-4b60-b4de-5f7567626cc7)

-	Source port number: 데이터를 전송하는 컴퓨터의 포트 번호이다.
-	Destination port number: 데이터를 받는 컴퓨터의 포트 번호이다.
-	Sequence number: 세그먼트의 순서를 정의하는 번호이다.
-	Acknowledgement number: 수신된 데이터에 대해 송신자가 성공적으로 받았음을 알리는 응답 번호이다.
-	Header length: TCP 헤더의 길이를 나타낸다.
-	Flags: SYN, ACK, FIN등과 같이 여러 상태 정보를 나타내며, 데이터 전송의 다양한 과정을 제어한다.
-	Window size: 수신 측이 한 번에 수신할 수 있는 데이터의 양을 나타낸다.
-	Checksum: 오류 검출을 위한 필드이다.
-	Urgent pointer: 긴급 데이터가 있는 경우 이를 표시한다.
-	Option: 추가 기능을 제공할 수 있는 확장 필드이다. 

### TCP Data
![image](https://github.com/user-attachments/assets/25c9f9b3-2039-4565-9f94-4ae6b50bff24)

클라이언트는 메시지를 전송할 때, ‘[닉네임]메시지’ 형식으로 메시지를 전송한다. 닉네임의 최대 크기는 20 바이트이고, 가변 길이의 문자열인 메시지를 포함하여 보낼 수 있는 데이터의 최대 크기는 512 바이트로 제한한다.
## Semantics
클라이언트는 서버에 접속하기 위해 서버에 연결 요청을 보내면 서버는 3-way handshake를 시작한다. 클라이언트가 SYN 패킷을 보내면 서버는 SYN-ACK 패킷으로 응답한다. 이 패킷을 받은 클라이언트는 ACK 패킷을 서버로 보낸다. 

![image](https://github.com/user-attachments/assets/f0ebf939-f530-472e-825f-daef50062d38)

3-way handshake 과정이 끝난 후, 클라이언트가 서버로 메시지를 전송하면, 서버는 접속 중인 다른 클라이언트들에게 해당 메시지를 echo한다. 클라이언트가 서버에 메시지를 전송할 때, 클라이언트가 보낼 데이터는 세그먼트로 나뉘게 되고, 각 세그먼트에 순서 번호가 부여된다. 데이터 세그먼트를 전송할 때, 클라이언트는 세그먼트가 서버에 도착했는지 확인하기 위해 ACK을 요구한다. 서버는 받은 데이터 세그먼트의 순서 번호를 확인하고, 올바른 순서로 데이터를 재조합한 후, 클라이언트에게 데이터를 잘 받았다는 것을 알리기 위해 ACK 패킷을 보낸다. 

![image](https://github.com/user-attachments/assets/ed413de5-b2d1-4146-a77e-1f1ed5cc7be1)

그 후, 서버는 해당 메시지를 여러 세그먼트로 나누어 다른 클라이언트들에게 전송하고, 클라이언트가 데이터를 수신했는지 확인하기 위해 ACK 패킷을 기다린다. 클라이언트들은 서버가 보낸 데이터 세그먼트를 조합하여 원래의 메시지로 복원 후, 데이터를 잘 받았다는 것을 서버에게 알리기 위해 ACK 패킷을 서버로 전송한다. 

![image](https://github.com/user-attachments/assets/91462795-7cc1-48c9-b576-bca3f2f35562)

클라이언트가 quit 명령을 보내거나, 서버가 채팅 서버를 종료할 때 4-way handshake 과정이 일어난다. 연결을 종료하기 위해, 연결을 종료하려는 측은 TCP 헤더에 FIN 플래그를 설정하여 상대방에게 보낸다. 연결 종료 요청을 받은 측은 ACK 플래그를 설정한 패킷을 보내 연결 종료 요청을 확인하는데, 보내야 할 데이터를 모두 전송하고 나면 FIN 플래그를 설정해 다시 연결을 종료하겠다고 알린다. 마자막으로 연결 종료를 요청했던 측에서 ACK 패킷을 보냄으로써 연결을 완전히 종료한다.

![image](https://github.com/user-attachments/assets/6a906ec0-1d2d-41b4-a102-b7602daa5d46)

## 주요 코드
### Client
- 소켓 생성
```c
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		fprintf(stderr, "socket() failed\n");
		exit(1);
	}
```
- 논블로킹 모드 설정
```c
	u_long mode = 1;
	retval = ioctlsocket(s, FIONBIO, &mode);
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "ioctlsocket() failed\n");
		exit(1);
	}
```
- 서버 IP 주소 및 포트 번호 설정
```c
	char serverIP[20];
	int PORT = 0;
	printf("Input Server IP: ");
	scanf("%s", serverIP);
	printf("Input Server Port: ");
	scanf("%d", &PORT);
	getchar();
```
- 서버에 연결
```c
	retval = connect(s, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
```
- 닉네임 설정 및 전송
```c
	printf("닉네임 입력: ");
	scanf("%s", nickname);
	getchar();
	nicknameLength = strlen(nickname);
 
	message[0] = '\0';
	snprintf(totalMessage, sizeof(totalMessage), "[%s]", nickname);
	strncat(totalMessage, message, BUFSIZE - strlen(totalMessage) - 1);
	retval = send(s, totalMessage, strlen(totalMessage), 0);
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "send() failed\n");
		exit(1);
	}
```
- 수신 스레드 생성
```c
	HANDLE hThread = CreateThread(NULL, 0, recvThread, (LPVOID)s, 0, NULL);
	if (hThread == NULL) {
		fprintf(stderr, "CreateThread() failed\n");
		exit(1);
	}
```
- 메시지 전송
```c
  retval = send(s, totalMessage, strlen(totalMessage), 0);
		if (strcmp(message, "quit") == 0) {
			printf("서버와의 연결을 종료합니다.\n");
			isThreadRunning = 0;
			break;
		}
		if (retval == SOCKET_ERROR) {
			fprintf(stderr, "send() failed\n");
			break;
		}
		else {
			printf("Send Message!\n");
			printf("\n");
		}
```
- 메시지 수신
```c
retval = recv(s, message, BUFSIZE - 1, 0); 
		if (retval == SOCKET_ERROR) {
			error = WSAGetLastError();
			if (error == WSAEINVAL || error == WSAEWOULDBLOCK) {
				Sleep(100);
				continue;
			}
			else {
				fprintf(stderr, "recv() failed\n");
				break;
			}
		}
```
- 프로그램 종료
```c
message[retval] = '\0';
			printf("\nRecieved Message: %s\n", message);
			if (strcmp(message, "[Server] 서버가 종료되었습니다.") == 0) {
				printf("서버와의 연결이 종료되었습니다.\n");
				exit(0);
			}
```
### Server
- 클라이언트 정보 구조체
```c
typedef struct {
	char nickname[20]; // 닉네임
	SOCKET socket; // 소켓
	SOCKADDR_IN clientIPAddr; // 클라이언트 IP 주소
	int port; // 클라이언트 포트
	bool connected; // 연결 상태
	bool saveInfo; // 닉네임 정보 저장 여부
}CLIENT;
```
- 서버 포트 번호 설정
```c
	int PORT = 0;
	printf("Input Server Port: ");
	fscanf(stdin, "%d", &PORT);
	getchar();
```
- 소켓 생성 및 바인딩
```c
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) {
		fprintf(stderr, "socket() failed\n");
		return 1;
	}
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(PORT);
	retval = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "bind() failed\n");
		return 1;
	}
```
- 클라이언트 접속 대기
```c
	retval = listen(listenSocket, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "listen() failed\n");
		return 1;
	}
```
- Echo 스레드 생성
```c
	HANDLE hThread = CreateThread(NULL, 0, echoThread, (LPVOID)&listenSocket, 0, NULL);
	if (hThread == NULL) {
		fprintf(stderr, "CreateThread() failed\n");
		exit(1);
	}
```
- 닉네임 파싱
```c
void parseNickname(char* receivedMessage, char* nickname) {
	char* start = strchr(receivedMessage, '[');
	char* end = strchr(receivedMessage, ']');

	if (start != NULL && end != NULL && end > start) {
		strncpy(nickname, start + 1, end - start - 1);
		nickname[end - start - 1] = '\0';
	}
	else {
		strcpy(nickname, "Unknown");
	}
}
```
- 메시지 파싱
```c
void parseMessage(char* receivedMessage, char* message) {
	char* end = strchr(receivedMessage, ']');
	char* trimedMessage = NULL;

	if (end != NULL) {
		strcpy(message, end + 1);
		message[BUFSIZE - (end - receivedMessage + 1)] = '\0';

		trimedMessage = message;
		while (*trimedMessage == ' ') {
			trimedMessage++;
		}
		strcpy(message, trimedMessage);
	}
	else {
		strcpy(message, receivedMessage);
	}
}
```
- select() 함수를 이용하여 소켓의 상태 변화 감지
```c
select(maxSocket + 1, &readfds, NULL, NULL, NULL);
```
- 클라이언트 접속 요청 수락
```c
	clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrLength);
			if (clientSocket == INVALID_SOCKET) {
				if (isThreadRunning != 1) {
					break;
				}
				fprintf(stderr, "accept() failed\n");
				continue;
			}
```
- 클라이언트 정보 저장
```c
if (clientCount < MAX_CLIENTS) {
				clientInfo[clientCount].socket = clientSocket;
				clientInfo[clientCount].clientIPAddr = clientAddr;
				clientInfo[clientCount].port = ntohs(clientAddr.sin_port);
				clientInfo[clientCount].connected = true; // 연결 상태 변경
				clientInfo[clientCount].saveInfo = false; // 닉네임 정보 저장 여부 초기화
				clientCount++;
			}
			else {
				printf("최대 클라이언트 수를 초과했습니다.\n");
				closesocket(clientSocket);
			}
```
- 메시지 수신
```c
retval = recv(clientInfo[i].socket, receivedMessage, BUFSIZE, 0);
```
- 닉네임 저장
```c
if (clientInfo[i].saveInfo == false) {
					nickname[0] = '\0';
					parseNickname(receivedMessage, nickname);

					strcpy(clientInfo[i].nickname, nickname);
					clientInfo[i].saveInfo = true; 
					continue; 
				}
```
- 클라이언트 연결 종료 여부 확인
```c
if (strcmp(message, "quit") == 0 || retval == SOCKET_ERROR || retval == 0) {
					FD_CLR(clientInfo[i].socket, &readfds);
					closesocket(clientInfo[i].socket);

					EnterCriticalSection(&cs);
					clientInfo[i].connected = false; 
					LeaveCriticalSection(&cs);

					maxSocket = listenSocket;
					for (int j = 0; j < clientCount; j++) {
						if (clientInfo[j].connected == true && clientInfo[j].socket > maxSocket) {
							maxSocket = clientInfo[j].socket;
						}
					}
					continue;
				}
```
- 접속 중인 다른 클라이언트들에게 메시지 echo
```c
for (int j = 0; j < clientCount; j++) {
						if (i != j && clientInfo[j].connected == true) {
							retval = send(clientInfo[j].socket, receivedMessage, strlen(receivedMessage), 0);
							if (retval == SOCKET_ERROR) {
								fprintf(stderr, "send() failed\n");
								break;
							}
						}
					}
```
- 명령 메뉴 출력
```c
void printCommandMenu() {
	printf("=========================================\n");
	printf("=================Command=================\n");
	printf("1. 클라이언트 정보 출력\n");
	printf("2. 분당 평균 메시지 수 출력\n");
	printf("3. 서버 종료\n");
	printf("=========================================\n");
	printf("Input Command (숫자로 입력): ");
}
```
- 클라이언트 정보 출력
```c
void printClientInfo() {
	EnterCriticalSection(&cs);
	printf("=========================================\n");
	printf("전체 클라이언트 수: %d\n", clientCount);
	if (clientCount != 0) {
		for (int i = 0; i < clientCount; i++) {
			printf("%d. 닉네임: %s , IP: %s , Port: %d , 연결 상태: %s\n", i + 1, clientInfo[i].nickname, inet_ntoa(clientInfo[i].clientIPAddr.sin_addr), clientInfo[i].port, clientInfo[i].connected ? "접속 중" : "종료");
		}
	}
	LeaveCriticalSection(&cs);
}
```
- 메시지 통계 출력
```c
void printMessageStat(time_t startTime) {
	EnterCriticalSection(&cs);
	time_t currentTime = time(NULL);
	double elapsedTime = difftime(currentTime, startTime) / 60.0;
	int averageMessageCount = (int)(messageCount / elapsedTime);
	if (averageMessageCount > 0) {
		printf("=========================================\n");
		printf("분당 평균 메시지 수: %d\n", averageMessageCount);
	}
	else {
		printf("=========================================\n");
		printf("전체 메시지 수: %d\n", messageCount);
	}
	LeaveCriticalSection(&cs);
}
```
- 서버 종료 메시지 전송
```c
void sendTerminateMessage(SOCKET s) {
	char terminateMessage[BUFSIZE] = "[Server] 서버가 종료되었습니다.";
	for (int i = 0; i < clientCount; i++) {
		if (clientInfo[i].connected == true) {
			send(clientInfo[i].socket, terminateMessage, strlen(terminateMessage), 0);

		}
	}
}
```
- 서버 종료
```c
void terminateServer(SOCKET s) {
	printf("서버를 종료합니다.\n");
	EnterCriticalSection(&cs);
	sendTerminateMessage(s);
	isThreadRunning = 0;
	for (int i = 0; i < clientCount; i++) {
		closesocket(clientInfo[i].socket);
	}
	LeaveCriticalSection(&cs);
	closesocket(s);
	DeleteCriticalSection(&cs);
	WSACleanup();
}
```

## Key Points
