#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <Windows.h>

#define BUFSIZE 512
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

volatile int isThreadRunning = 1;

// 수신 스레드
DWORD WINAPI recvThread(LPVOID arg) {
	int retval;

	SOCKET s = (SOCKET)arg;
	SOCKADDR_IN clientAddr;
	int clientAddrLength = sizeof(clientAddr);
	char message[BUFSIZE] = "";
	int error = 0;

	while (isThreadRunning) {
		// 서버로부터 메시지 수신
		retval = recv(s, message, BUFSIZE - 1, 0); // BUFSIZE - 1 : 마지막 문자를 NULL로 채우기 위해 1을 뺌
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
		else if(retval>0) {
			message[retval] = '\0';
			printf("\nRecieved Message: %s\n", message);
			if (strcmp(message, "[Server] 서버가 종료되었습니다.") == 0) {
				printf("서버와의 연결이 종료되었습니다.\n");
				exit(0);
			}
		}
	}

	return 0;
}

int main(int argc, char* argv[]) {
	int retval;

	// Winsock 초기화	
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup()(= failed\n");
		exit(1);
	}

	// 소켓 생성
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		fprintf(stderr, "socket() failed\n");
		exit(1);
	}

	// 논블로킹 모드 설정 
	u_long mode = 1;
	retval = ioctlsocket(s, FIONBIO, &mode);
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "ioctlsocket() failed\n");
		exit(1);
	}

	// 서버 IP 주소 및 포트 번호 설정
	char serverIP[20];
	int PORT = 0;
	printf("Input Server IP: ");
	scanf("%s", serverIP);
	printf("Input Server Port: ");
	scanf("%d", &PORT);
	getchar();

	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);
	if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
		fprintf(stderr, "inet_addr() failed\n");
		exit(1);
	}
	serverAddr.sin_port = htons(PORT);

	// 서버에 연결
	retval = connect(s, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

	char nickname[20] = "";
	char message[BUFSIZE - 20] = "";
	char totalMessage[BUFSIZE] = "";
	int msgLength = 0;
	int nicknameLength = 0;

	// 닉네임 설정
	printf("닉네임 입력: ");
	scanf("%s", nickname);
	getchar();
	nicknameLength = strlen(nickname);
	
	// 서버가 클라이언트 정보를 저장하기 위해 닉네임 전송 
	message[0] = '\0';
	snprintf(totalMessage, sizeof(totalMessage), "[%s]", nickname);
	strncat(totalMessage, message, BUFSIZE - strlen(totalMessage) - 1);
	retval = send(s, totalMessage, strlen(totalMessage), 0);
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "send() failed\n");
		exit(1);
	}

	// 수신 스레드 생성
	HANDLE hThread = CreateThread(NULL, 0, recvThread, (LPVOID)s, 0, NULL);
	if (hThread == NULL) {
		fprintf(stderr, "CreateThread() failed\n");
		exit(1);
	}

	while (1) {
		snprintf(totalMessage, sizeof(totalMessage), "[%s]", nickname);
		memset(message, 0, BUFSIZE - 20);

		// 메시지 입력
		printf("메시지 입력 (종료 시 quit 입력) : ");
		fgets(message, BUFSIZE - 20, stdin);
		msgLength = strlen(message);
		if (message[msgLength - 1] == '\n') {
			message[msgLength - 1] = '\0';
		}

		strncat(totalMessage, message, BUFSIZE - strlen(totalMessage) - 1);

		msgLength = strlen(totalMessage);
		if (totalMessage[msgLength - 1] == '\n') {
			totalMessage[msgLength - 1] = '\0';
		}

		// 서버로 메시지 전송 
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
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	closesocket(s);
	WSACleanup();
	return 0;
}
