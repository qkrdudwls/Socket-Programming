# Multi-Chat Program using UDP
* **운영체제**: Windows 11
* **개발 도구**: Visual Studio 또는 GCC를 지원하는 Windows용 C 컴파일러
* **네트워크 라이브러리**: WinSock2
## Contents
### Client
* 클라이언트 프로그램에서는 메시지 전송과 수신이 동시에 이루어진다.
* **메시지 전송**
  * 클라이언트는 접속 후 자신의 닉네임을 설정한다.
  * 그 후, 메시지를 입력하고 서버로 전송한다. 이때 메시지 형식은 '[닉네임]채팅 메시지'이다.
* **메시지 수신**
  * 서버로부터 수신한 메시지를 그대로 출력한다.
### Server
* 서버 프로그램은 클라이언트로부터 수신한 메시지를 다른 클라이언트들에게 그대로 전송한다. (Echo)
  * 이때, 해당 메시지를 전송한 클라이언트에게는 보내지 않는다.
* 메시지 전송과 동시에 서버는 다음과 같은 정보를 제공한다.
  1. 접속 중인 클라이언트 정보
     * 접속 클라이언트 수와 함께 각 클라이언트의 닉네임, IP 주소, 포트 번호를 표시한다.   
  2. 채팅 통계
     * 1분 당 평균 메시지 수에 대한 통계 데이터를 제공한다. 
  4. 채팅 종료
## Syntax

## Semantics

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
- 논블로킹 모드 설정
```c
	u_long mode = 1; // 논블로킹 모드 설정
	retval = ioctlsocket(s, FIONBIO, &mode);
	if (retval != 0) {
		fprintf(stderr, "ioctlsocket() failed\n");
		exit(1);
	}
```
- 서버 IP 주소 및 포트 번호 설정
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
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);
	if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
		fprintf(stderr, "inet_addr() failed\n");
		exit(1);
	}
	serverAddr.sin_port = htons(PORT);
```
- 닉네임 입력 및 전송
```c
	printf("닉네임 입력 (최대 20자) : ");
	scanf("%s", nickname);
	getchar();
	nicknameLegth = strlen(nickname);
	if (nicknameLegth > 20) {
		fprintf(stderr, "닉네임은 최대 20자까지 가능합니다.\n");
		exit(1);
	}
```
```c
	message[0] = '\0';
	snprintf(totalMessage, sizeof(totalMessage), "[%s] ", nickname);
	strncat(totalMessage, message, BUFSIZE - strlen(totalMessage) - 1);
	retval = sendto(s, totalMessage, strlen(totalMessage), 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "sendto() failed\n");
		exit(1);
	}
```
- 수신 스레드 생성
```c
	HANDLE hThread = CreateThread(NULL, 0, recvThread, (LPVOID)s, 0, NULL);
	if (hThread == NULL) {
		printf("CreateThread() failed\n");
		exit(1);
	}
```
- 메시지 생성 및 전송
```c
	snprintf(totalMessage, sizeof(totalMessage), "[%s] ", nickname); 
	memset(message, 0, BUFSIZE);

	printf("메시지 입력 (종료 시 quit 입력) : ");
	fgets(message, BUFSIZE, stdin);
	msgLength = strlen(message);
	if (message[msgLength - 1] == '\n') {
		message[msgLength - 1] = '\0';
	}

	strncat(totalMessage, message, BUFSIZE - strlen(totalMessage) - 1);
```
```c
	retval = sendto(s, totalMessage, msgLength, 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (strcmp(message, "quit") == 0) {
		break;
	}
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "sendto() failed\n");
		continue;
	}
	else {
		printf("Send Message!\n");
		printf("\n");
	}
```
- 메시지 수신 및 서버 종료 메시지 처리
```c
	retval = recvfrom(s, message, BUFSIZE, 0, (SOCKADDR*)&clientAddr, &clientAddrLength);
	if (retval == SOCKET_ERROR) {
		error_code = WSAGetLastError();
		if (error_code == WSAEINVAL || error_code == WSAEWOULDBLOCK) {
			Sleep(100); // 100ms 대기
			continue;
		}
		else {
			fprintf(stderr, "recvfrom() failed: %d\n", error_code);
			break;
		}
			
	}
	else {
		message[retval] = '\0';
		printf("\nReceived Message : %s\n", message);
		if (strcmp(message, "[SERVER]Server is terminated.") == 0) {
			printf("서버가 종료되었습니다.\n");
			exit(0);
		}
	}
```
### Server
- 클라이언트 정보 구조체
```c
	typedef struct {
		char nickname[20]; 
		SOCKADDR_IN clientIPAddr;
		int port;
	}CLIENT;
```
- 포트 번호 설정
```c
	int PORT = 0;
	printf("Input Server Port : ");
	scanf("%d", &PORT);
	getchar();
```
- 소켓 생성
```c
	SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		fprintf(stderr, "socket() failed\n");
		exit(1);
	}
```
- 소켓 정보 구조체 초기화 및 바인딩
```c
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(PORT);
	retval = bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "bind() failed\n");
		exit(1);
	}
```
- Echo 스레드 생성
```c
	hThread = CreateThread(NULL, 0, echoThread, (LPVOID)serverSocket, 0, &ThreadID);
	if (hThread == NULL) {
		fprintf(stderr, "CreateThread() failed\n");
		exit(1);
	}
```
- 논블로킹 모드 설정
```c
	u_long mode = 1;
	ioctlsocket(s, FIONBIO, &mode);
```
- 메시지 수신
```c
	retval = recvfrom(s, receivedMessage, BUFSIZE, 0, (SOCKADDR*)&clientAddr, &clientAddrLength);
	if (retval == SOCKET_ERROR) {
		int error = WSAGetLastError();
		if (error == WSAEWOULDBLOCK) {
			Sleep(10);
			continue;
		}
		else {
			fprintf(stderr, "recvfrom() failed with error: %d\n", error);
			break;
		}
	}
```
- 클라이언트 정보 확인 및 저장
```c
	int clientExist = 0;
	for (int i = 0; i < clientCount; i++) {
		if (memcmp(&clientInfo[i].clientIPAddr, &clientAddr, sizeof(SOCKADDR_IN)) == 0) {
			clientExist = 1;
			break;
		}
	}
	if (clientExist == 0 && clientCount < MAX_CLIENT) {
		parseNickname(receivedMessage, clientNickname);
		strcpy(clientInfo[clientCount].nickname, clientNickname);
		clientInfo[clientCount].clientIPAddr = clientAddr;
		clientInfo[clientCount].port = ntohs(clientAddr.sin_port);
		clientCount++;
	}
	else if (clientExist == 0 && clientCount >= MAX_CLIENT) {
		printf("최대 접속 가능한 클라이언트 수를 초과했습니다.\n");
	}
```
- 클라이언트 정보 삭제
```c
	if (clientCount == 1) {
		memset(&clientInfo[0], 0, sizeof(clientInfo));
		clientCount = 0;
	}
	else {
		for (int i = 0; i < clientCount; i++) {
			if (memcmp(&clientInfo[i].clientIPAddr, &clientAddr, sizeof(SOCKADDR_IN)) == 0) {
				if (i < clientCount - 1) {
					strcpy(clientInfo[i].nickname, clientInfo[clientCount - 1].nickname);
					clientInfo[i].clientIPAddr = clientInfo[clientCount - 1].clientIPAddr;
					clientInfo[i].port = clientInfo[clientCount - 1].port;
				}
			clientCount--;
			break;
		}
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

		if (end != NULL) {
			strcpy(message, end + 1);
			message[BUFSIZE - (end - receivedMessage - 1)] = '\0';

			char* trimedMessage = message;
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
- 채팅 서버 종료 메시지 전송
```c
	void sendTerminateMessage(SOCKET s) {
		int retval;

		SOCKADDR_IN clientAddr;
		int clientAddrLength = sizeof(clientAddr);
		char terminateMessage[BUFSIZE] = "[SERVER]Server is terminated.";

		for (int i = 0; i < clientCount; i++) {
			retval = sendto(s, terminateMessage, strlen(terminateMessage), 0, (SOCKADDR*)&clientInfo[i].clientIPAddr, sizeof(SOCKADDR_IN));
			if (retval == SOCKET_ERROR) {
				fprintf(stderr, "sendto() failed\n");
				continue;
			}
		}
	}
```
  
