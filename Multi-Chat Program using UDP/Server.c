#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <WinSock2.h>
#include <Windows.h>

#define BUFSIZE 512 
#define MAX_CLIENT 200 
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

typedef struct {
	char nickname[20]; // 닉네임
	SOCKADDR_IN clientIPAddr; // 클라이언트 IP 주소
	int port; // 클라이언트 포트 번호
}CLIENT;

CLIENT clientInfo[MAX_CLIENT]; 
CRITICAL_SECTION cs; 
int clientCount = 0; 
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

DWORD WINAPI echoThread(LPVOID arg) {
	int retval;

	SOCKET s = (SOCKET)arg;
	SOCKADDR_IN clientAddr;
	int clientAddrLength = sizeof(clientAddr);
	char receivedMessage[BUFSIZE] = "";
	char clientNickname[20] = "";
	char message[BUFSIZE - 20] = "";
	time_t startTime = time(NULL);

	u_long mode = 1;
	ioctlsocket(s, FIONBIO, &mode);

	while (isThreadRunning) {
		clientNickname[0] = '\0';
		message[0] = '\0';

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

		EnterCriticalSection(&cs);
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
		LeaveCriticalSection(&cs);

		parseMessage(receivedMessage, message);

		if (strcmp(message, "quit") == 0) {
			EnterCriticalSection(&cs);
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
			}
			LeaveCriticalSection(&cs);
			continue;
		}

		EnterCriticalSection(&cs);
		messageCount++;
		LeaveCriticalSection(&cs);

		EnterCriticalSection(&cs);
		for (int i = 0; i < clientCount; i++) {
			if (memcmp(&clientInfo[i].clientIPAddr, &clientAddr, sizeof(SOCKADDR_IN)) != 0) {
				retval = sendto(s, receivedMessage, retval, 0, (SOCKADDR*)&clientInfo[i].clientIPAddr, sizeof(SOCKADDR_IN));
				if (retval == SOCKET_ERROR) {
					fprintf(stderr, "sendto() failed\n");
					continue;
				}
			}
		}
		LeaveCriticalSection(&cs);
	}

	closesocket(s);
	return 0;
}

int main(int argc, char* argv[]) {
	int retval;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup() failed\n");
		exit(1);
	}

	int PORT = 0;
	printf("Input Server Port : ");
	scanf("%d", &PORT);
	getchar();

	SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		fprintf(stderr, "socket() failed\n");
		exit(1);
	}

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

	int command = 0;
	int averageMessageCountperMinute = 0;
	time_t startTime = time(NULL);
	time_t currentTime;
	double elapsedTime = 0;

	InitializeCriticalSection(&cs);
	HANDLE hThread;
	DWORD ThreadID;

	hThread = CreateThread(NULL, 0, echoThread, (LPVOID)serverSocket, 0, &ThreadID);
	if (hThread == NULL) {
		fprintf(stderr, "CreateThread() failed\n");
		exit(1);
	}

	while (1) {
		printf("---Menu---\n");
		printf("1. 클라이언트 정보\n2. 채팅 통계\n3. 채팅 종료\n");
		printf("Input Command (해당하는 숫자 입력) : ");
		scanf("%d", &command);

		switch (command) {
		case 1:
			EnterCriticalSection(&cs);
			printf("접속한 클라이언트 수: %d\n", clientCount);
			if (clientCount == 0) {
				printf("접속한 클라이언트가 없습니다.\n");
				LeaveCriticalSection(&cs);
				break;
			}
			for (int i = 0; i < clientCount; i++) {
				printf("닉네임: %s, IP: %s, Port: %d\n", clientInfo[i].nickname, inet_ntoa(clientInfo[i].clientIPAddr.sin_addr), clientInfo[i].port);
			}
			LeaveCriticalSection(&cs);
			break;

		case 2:
			currentTime = time(NULL); // 현재 시간
			elapsedTime = difftime(currentTime, startTime) / 60.0; // 분 단위로 변환
			if (elapsedTime > 0) {
				averageMessageCountperMinute = (int)(messageCount / elapsedTime); // 분당 평균 메시지 수 계산
				printf("분당 평균 메시지 수: %d\n", averageMessageCountperMinute);
			}
			else {
				printf("전체 메시지 수: %d\n", messageCount);
			}
			break;

		case 3:
			printf("채팅 서버를 종료합니다.\n");
			EnterCriticalSection(&cs);
			isThreadRunning = 0;
			sendTerminateMessage(serverSocket);
			LeaveCriticalSection(&cs);
			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);
			DeleteCriticalSection(&cs);
			closesocket(serverSocket);
			WSACleanup();
			return 0;

		default:
			printf("잘못된 명령어입니다.\n");
			continue;
		}
	}
}
