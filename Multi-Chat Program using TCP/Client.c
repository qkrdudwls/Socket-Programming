#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <Windows.h>

#define BUFSIZE 512 // 메시지 버퍼 크기
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

// 수신 스레드
DWORD WINAPI recvThread(LPVOID arg) {
	int retval;

	SOCKET s = (SOCKET)arg;
	SOCKADDR_IN clientAddr;
	int clientAddrLength = sizeof(clientAddr);
	char message[BUFSIZE] = "";
	int error_code = 0;

	while (1) {
		// 메시지 수신
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
	}

	return 0;
}

int main(int argc, char* argv[]) {
	int retval;

	// Winsock 초기화
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup() failed\n");
		exit(1);
	}

	// socket()
	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == INVALID_SOCKET) {
		fprintf(stderr, "socket() failed\n");
		exit(1);
	}

	u_long mode = 1; // 논블로킹 모드 설정
	retval = ioctlsocket(s, FIONBIO, &mode);
	if (retval != 0) {
		fprintf(stderr, "ioctlsocket() failed\n");
		exit(1);
	}

	// 서버 주소 및 포트 설정
	char serverIP[20] = "";
	int PORT = 0;
	printf("Input Server IP : ");
	scanf("%s", serverIP);
	printf("Input Server Port : ");
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

	char nickname[20] = ""; // 사용자의 닉네임
	char message[BUFSIZE - 20] = ""; // 사용자가 입력한 메시지
	char totalMessage[BUFSIZE] = ""; // 서버에게 전송하는 메시지 
	int msgLength = 0; // totalMessage의 길이
	int nicknameLegth = 0;

	// 닉네임 입력
	printf("닉네임 입력 (최대 20자) : ");
	scanf("%s", nickname);
	getchar();
	nicknameLegth = strlen(nickname);
	if (nicknameLegth > 20) {
		fprintf(stderr, "닉네임은 최대 20자까지 가능합니다.\n");
		exit(1);
	}

	// 서버에게 클라이언트가 접속했음을 알리기 위해 닉네임 전송
	message[0] = '\0';
	snprintf(totalMessage, sizeof(totalMessage), "[%s] ", nickname);
	strncat(totalMessage, message, BUFSIZE - strlen(totalMessage) - 1);
	retval = sendto(s, totalMessage, strlen(totalMessage), 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == SOCKET_ERROR) {
		fprintf(stderr, "sendto() failed\n");
		exit(1);
	}

	// 수신	스레드 생성
	HANDLE hThread = CreateThread(NULL, 0, recvThread, (LPVOID)s, 0, NULL);
	if (hThread == NULL) {
		printf("CreateThread() failed\n");
		exit(1);
	}

	while (1) {
		// [닉네임] 메시지 형태로 생성
		snprintf(totalMessage, sizeof(totalMessage), "[%s] ", nickname);
		// 사용자가 입력할 메시지 버퍼 초기화 
		memset(message, 0, BUFSIZE);

		// 메시지 입력
		printf("메시지 입력 (종료 시 quit 입력) : ");
		fgets(message, BUFSIZE, stdin);
		msgLength = strlen(message);
		if (message[msgLength - 1] == '\n') {
			message[msgLength - 1] = '\0';
		}

		// [닉네임] 뒤에 메시지를 붙여서 전송
		strncat(totalMessage, message, BUFSIZE - strlen(totalMessage) - 1);

		msgLength = strlen(totalMessage);
		if (totalMessage[msgLength - 1] == '\n') {
			totalMessage[msgLength - 1] = '\0';
		}

		// 메시지 전송
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
	}

	TerminateThread(hThread, 0);
	CloseHandle(hThread);
	closesocket(s);
	WSACleanup();

	return 0;
}