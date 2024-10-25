#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <WinSock2.h>
#include <Windows.h>

#define BUFSIZE 512
#define MAX_CLIENTS 200
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

// 클라이언트 정보 구조체
typedef struct {
	char nickname[20]; // 닉네임
	SOCKET socket; // 소켓
	SOCKADDR_IN clientIPAddr; // 클라이언트 IP 주소
	int port; // 클라이언트 포트
	bool connected; // 연결 상태
	bool saveInfo; // 닉네임 정보 저장 여부
}CLIENT;

CLIENT clientInfo[MAX_CLIENTS]; // 클라이언트 정보 구조체 배열
CRITICAL_SECTION cs; // 임계 영역
int clientCount; // 클라이언트 수
int messageCount = 0; // 메시지 수
int isThreadRunning = 1; // 스레드 실행 여부

// 닉네임 파싱
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

// 메시지 파싱
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

// Echo 스레드 
DWORD WINAPI echoThread(LPVOID arg) {
	int retval;
	clientCount = 0;

	SOCKET listenSocket = *(SOCKET*)arg;
	fd_set readfds;
	SOCKET maxSocket = listenSocket;
	SOCKET clientSocket;
	SOCKADDR_IN clientAddr;
	int clientAddrLength = 0;
	char receivedMessage[BUFSIZE] = ""; // 수신 메시지
	char nickname[20] = ""; // 닉네임
	char message[BUFSIZE] = ""; // 메시지


	while (isThreadRunning) {
		FD_ZERO(&readfds);
		FD_SET(listenSocket, &readfds);

		EnterCriticalSection(&cs);
		for (int i = 0; i < clientCount; i++) {
			if (clientInfo[i].connected == true) {
				FD_SET(clientInfo[i].socket, &readfds);
				if (clientInfo[i].socket > maxSocket) {
					maxSocket = clientInfo[i].socket;
				}
			}
		}
		LeaveCriticalSection(&cs);

		// select() 함수를 이용하여 소켓의 상태 변화를 감지
		select(maxSocket + 1, &readfds, NULL, NULL, NULL);

		if (FD_ISSET(listenSocket, &readfds)) {
			clientAddrLength = sizeof(clientAddr);
			// 클라이언트의 접속 요청을 수락
			clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrLength);
			if (clientSocket == INVALID_SOCKET) {
				if (isThreadRunning != 1) {
					break;
				}
				fprintf(stderr, "accept() failed\n");
				continue;
			}

			// 클라이언트 정보 저장
			EnterCriticalSection(&cs);
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
			LeaveCriticalSection(&cs);
		}

		// 클라이언트로부터 메시지를 수신하고 다른 클라이언트에게 전송
		for (int i = 0; i < clientCount; i++) {
			if (FD_ISSET(clientInfo[i].socket, &readfds)) {
				// 메시지 초기화
				memset(receivedMessage, 0, sizeof(receivedMessage));
				memset(message, 0, sizeof(message));

				// 클라이언트로부터 메시지 수신
				retval = recv(clientInfo[i].socket, receivedMessage, BUFSIZE, 0);

				if (clientInfo[i].saveInfo == false) {
					// 클라이언트의 닉네임 파싱
					nickname[0] = '\0';
					parseNickname(receivedMessage, nickname);

					strcpy(clientInfo[i].nickname, nickname); // 닉네임 정보 저장
					clientInfo[i].saveInfo = true; // 닉네임 정보 저장 여부 변경
					continue; // 닉네임 정보 저장용 메시지는 다른 클라이언트들에게 전송하지 않음 
				}

				// 클라이언트의 메시지 파싱
				parseMessage(receivedMessage, message);
				// 클라이언트의 연결 종료 여부 확인
				if (strcmp(message, "quit") == 0 || retval == SOCKET_ERROR || retval == 0) {
					// 클라이언트 소켓을 닫기 전 FD_SET에서 제거
					FD_CLR(clientInfo[i].socket, &readfds);
					closesocket(clientInfo[i].socket);

					EnterCriticalSection(&cs);
					clientInfo[i].connected = false; // 연결 상태 변경
					LeaveCriticalSection(&cs);

					// maxSocket 재설정
					maxSocket = listenSocket;
					for (int j = 0; j < clientCount; j++) {
						if (clientInfo[j].connected == true && clientInfo[j].socket > maxSocket) {
							maxSocket = clientInfo[j].socket; // maxSocket 갱신
						}
					}
					continue;
				}

				else {
					// 메시지 수 증가
					EnterCriticalSection(&cs);
					messageCount++;
					LeaveCriticalSection(&cs);

					for (int j = 0; j < clientCount; j++) {
						// 접속 중인 다른 클라이언트에게 메시지 전송
						if (i != j && clientInfo[j].connected == true) {
							retval = send(clientInfo[j].socket, receivedMessage, strlen(receivedMessage), 0);
							if (retval == SOCKET_ERROR) {
								fprintf(stderr, "send() failed\n");
								break;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

// 명령 메뉴 출력
void printCommandMenu() {
	printf("=========================================\n");
	printf("=================Command=================\n");
	printf("1. 클라이언트 정보 출력\n");
	printf("2. 분당 평균 메시지 수 출력\n");
	printf("3. 서버 종료\n");
	printf("=========================================\n");
	printf("Input Command (숫자로 입력): ");
}

// 클라이언트 정보 출력
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

// 메시지 통계 출력
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

// 접속 중인 클라이언트들에게 서버 종료 메시지 전송
void sendTerminateMessage(SOCKET s) {
	char terminateMessage[BUFSIZE] = "[Server] 서버가 종료되었습니다.";
	for (int i = 0; i < clientCount; i++) {
		if (clientInfo[i].connected == true) {
			send(clientInfo[i].socket, terminateMessage, strlen(terminateMessage), 0);

		}
	}
}

// 서버 종료
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

int main(int argc, char* argv[]) {
	int retval;

	// Winsock 초기화
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup() failed\n");
		return 1;
	}

	// 서버 포트 입력
	int PORT = 0;
	printf("Input Server Port: ");
	fscanf(stdin, "%d", &PORT);
	getchar();

	// 소켓 생성 및 바인딩
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

	// 클라이언트 접속 대기
	retval = listen(listenSocket, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "listen() failed\n");
		return 1;
	}

	InitializeCriticalSection(&cs);
	// Echo 스레드 생성
	HANDLE hThread = CreateThread(NULL, 0, echoThread, (LPVOID)&listenSocket, 0, NULL);
	if (hThread == NULL) {
		fprintf(stderr, "CreateThread() failed\n");
		exit(1);
	}

	time_t startTime = time(NULL);
	int command = 0;
	while (1) {
		printCommandMenu();
		fscanf(stdin, "%d", &command);

		switch (command) {
		case 1:
			printClientInfo();
			break;

		case 2:
			printMessageStat(startTime);
			break;

		case 3:
			terminateServer(listenSocket);
			return 0;

		default:
			printf("잘못된 명령입니다.\n");
			continue;
		}
	}
}