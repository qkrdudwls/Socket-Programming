#define CRT_SECRURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <WinSock2.h>

#define BUFSIZE 512
#define PORT 12345
#define TIMEOUT 10 // 타임아웃 시간 (10초)
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
	int retval = 0;
	srand((unsigned int)time(NULL));

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

	// 서버 IP 주소 입력
	char serverIP[20] = "";
	fprintf(stdout, "Input Server IP : ");
	fscanf(stdin, "%s", serverIP);
	getchar();

	// 소켓 주소 구조체 초기화
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);
	serverAddr.sin_port = htons(PORT);
	if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
		fprintf(stderr, "inet_addr() failed\n");
		exit(1);
	}
	else {
		fprintf(stdout, "Server Connection Success\n");
	}

	// 변수
	SOCKADDR_IN clientAddr;
	int clientAddrLength = sizeof(clientAddr);
	unsigned short seqNum = 0;
	unsigned short sequence = 0;
	unsigned short ackNum = 0;
	char message[BUFSIZE + 1] = "";
	char totalMessage[BUFSIZE + 3] = "";
	char command[5] = "";
	int messageLength = 0;
	int totalLength = 0;
	int K = 1;
	bool ackReceived = false;
	struct timeval timeout;

	while (1) {
		// 변수 초기화
		ackReceived = false;
		memset(message, 0, BUFSIZE + 1);
		memset(totalMessage, 0, BUFSIZE + 3);

		// seqNum 복사 
		seqNum = htons(sequence);
		memcpy(totalMessage, &seqNum, 2);

		// 메시지 입력
		fprintf(stdout, "[%d] Input Message (종료 시 quit 또는 QUIT 입력) : ", K);
		fgets(message, BUFSIZE + 1, stdin);
		messageLength = strlen(message);
		if (message[messageLength - 1] == '\n') {
			message[messageLength - 1] = '\0';
		}

		// message를 totalMessage에 복사
		memcpy(totalMessage + 2, message, messageLength);
		totalLength = messageLength + 2;

		while (ackReceived == false) {
			// 메시지 전송
			retval = sendto(s, totalMessage, totalLength, 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
			if (retval == SOCKET_ERROR) {
				fprintf(stderr, "sendto() failed\n");
				exit(1);
			}
			else {
				fprintf(stdout, "Send Message: %s\n\n", message);
			}

			// select() 함수를 이용한 타임아웃 설정
			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(s, &readfds);

			// 타임아웃 설정
			timeout.tv_sec = TIMEOUT;
			timeout.tv_usec = 0;

			// select()
			retval = select(0, &readfds, NULL, NULL, &timeout);
			if (retval == SOCKET_ERROR) {
				fprintf(stderr, "select() failed\n");
				exit(1);
			}
			// 타임아웃 발생 시 메시지 재전송 
			else if (retval == 0) {
				fprintf(stderr, "Timeout. Resending Message...\n\n");
				continue;
			}
			else if (FD_ISSET(s, &readfds)) {
				// 메시지 수신
				retval = recvfrom(s, totalMessage, totalLength, 0, (SOCKADDR*)&clientAddr, &clientAddrLength);
				if (retval == SOCKET_ERROR) {
					fprintf(stderr, "recvfrom() failed\n");
					exit(1);
				}
				else {
					// ackNum 추출 및 비교 
					memcpy(&ackNum, totalMessage, 2);
					if (ackNum == seqNum) {
						// 수신한 메시지 출력 
						fprintf(stdout, "Received Message: %s\n\n", totalMessage + 2);
						sequence += messageLength; // seqNum 증가
						ackReceived = true;
					}
					// Ack Number가 일치하지 않을 경우 packet loss로 판단. 메시지 재전송
					else {
						fprintf(stdout, "Packet loss. Resending Message...\n\n");
					}
				}
			}
		}
		// 종료	명령어 입력 시 종료
		if (strcmp(message, "quit") == 0 || strcmp(message, "QUIT") == 0) {
			fprintf(stdout, "Terminated\n");
			break;
		}
		K++;
	}

	closesocket(s);
	WSACleanup();
	return 0;
}