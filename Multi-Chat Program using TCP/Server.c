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

typedef struct {
	char nickname[20]; // 닉네임
	SOCKET socket; // 소켓
	SOCKADDR_IN clientIPAddr; // 클라이언트 IP 주소
	int port; // 클라이언트 포트
	bool connected; // 연결 상태
	bool saveInfo; // 닉네임 정보 저장 여부
}CLIENT;

CLIENT clientInfo[MAX_CLIENTS]; 
CRITICAL_SECTION cs; 
int clientCount;
int messageCount = 0; 
int isThreadRunning = 1; 

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
	char receivedMessage[BUFSIZE] = ""; 
	char nickname[20] = "";
	char message[BUFSIZE] = ""; 

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

		select(maxSocket + 1, &readfds, NULL, NULL, NULL);

		if (FD_ISSET(listenSocket, &readfds)) {
			clientAddrLength = sizeof(clientAddr);
			clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrLength);
			if (clientSocket == INVALID_SOCKET) {
				if (isThreadRunning != 1) {
					break;
				}
				fprintf(stderr, "accept() failed\n");
				continue;
			}

			EnterCriticalSection(&cs);
			if (clientCount < MAX_CLIENTS) {
				clientInfo[clientCount].socket = clientSocket;
				clientInfo[clientCount].clientIPAddr = clientAddr;
				clientInfo[clientCount].port = ntohs(clientAddr.sin_port);
				clientInfo[clientCount].connected = true; 
				clientInfo[clientCount].saveInfo = false; 
				clientCount++;
			}
			else {
				printf("최대 클라이언트 수를 초과했습니다.\n");
				closesocket(clientSocket);
			}
			LeaveCriticalSection(&cs);
		}

		for (int i = 0; i < clientCount; i++) {
			if (FD_ISSET(clientInfo[i].socket, &readfds)) {
				memset(receivedMessage, 0, sizeof(receivedMessage));
				memset(message, 0, sizeof(message));

				retval = recv(clientInfo[i].socket, receivedMessage, BUFSIZE, 0);

				if (clientInfo[i].saveInfo == false) {
					nickname[0] = '\0';
					parseNickname(receivedMessage, nickname);

					strcpy(clientInfo[i].nickname, nickname); 
					clientInfo[i].saveInfo = true; 
					continue; 
				}

				parseMessage(receivedMessage, message);
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

				else {
					EnterCriticalSection(&cs);
					messageCount++;
					LeaveCriticalSection(&cs);

					for (int j = 0; j < clientCount; j++) {
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

void printCommandMenu() {
	printf("=========================================\n");
	printf("=================Command=================\n");
	printf("1. 클라이언트 정보 출력\n");
	printf("2. 분당 평균 메시지 수 출력\n");
	printf("3. 서버 종료\n");
	printf("=========================================\n");
	printf("Input Command (숫자로 입력): ");
}

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

void sendTerminateMessage(SOCKET s) {
	char terminateMessage[BUFSIZE] = "[Server] 서버가 종료되었습니다.";
	for (int i = 0; i < clientCount; i++) {
		if (clientInfo[i].connected == true) {
			send(clientInfo[i].socket, terminateMessage, strlen(terminateMessage), 0);

		}
	}
}

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

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup() failed\n");
		return 1;
	}

	int PORT = 0;
	printf("Input Server Port: ");
	fscanf(stdin, "%d", &PORT);
	getchar();

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

	retval = listen(listenSocket, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "listen() failed\n");
		return 1;
	}

	InitializeCriticalSection(&cs);
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
