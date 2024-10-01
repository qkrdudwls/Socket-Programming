#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 512
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
    int retval;

    // Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup() failed\n");
        exit(1);
    }

    // 포트 번호 설정
    int PORT = 0;
    printf("Input Server Port : ");
    scanf("%d", &PORT);
    while (getchar() != '\n');

    // socket()
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed\n");
        exit(1);
    }

    // bind()
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

    // 변수
    SOCKADDR_IN clientAddr;
    int total_bytes = 0;
    int total_messages = 0;
    unsigned short command_code = 0;
    char message[BUFSIZE + 1] = "";
    int clientAddrLength = 0;
    char response[BUFSIZE + 1] = "";
    char ServerIP[INET_ADDRSTRLEN]; // 서버 IP 주소
    char ClientIP[INET_ADDRSTRLEN]; // 클라이언트 IP 주소

    // 호스트 이름 추출
    char hostname[256];
    retval = gethostname(hostname, sizeof(hostname));
    if (retval == SOCKET_ERROR) {
        fprintf(stderr, "gethostname() failed\n");
        exit(1);
    }

    // 호스트 이름을 기반으로 IP 주소 구하기 
    struct addrinfo hints, * res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    retval = getaddrinfo(hostname, NULL, &hints, &res);
    if (retval != 0) {
        fprintf(stderr, "getaddrinfo() failed\n");
        exit(1);
    }

    // IP 주소 변환
    inet_ntop(AF_INET, &((struct sockaddr_in*)res->ai_addr)->sin_addr, ServerIP, sizeof(ServerIP));

    // 메모리 해제 
    freeaddrinfo(res);

    while (1) {
        // 메시지 받기 
        clientAddrLength = sizeof(clientAddr);
        retval = recvfrom(s, message, BUFSIZE, 0, (SOCKADDR*)&clientAddr, &clientAddrLength);
        if (retval == SOCKET_ERROR) {
            fprintf(stderr, "recvfrom() failed\n");
            continue;
        }
        if (retval < BUFSIZE) {
            message[retval] = '\0';
        }

        // 통계 정보 업데이트
        total_messages++;

        // Command_code 추출
        memcpy(&command_code, message, sizeof(command_code));
        command_code = ntohs(*(unsigned short*)message);

        // 통계 정보 업데이트
        total_bytes += retval;

        // 클라이언트 IP 주소 추출
        inet_ntop(AF_INET, &clientAddr.sin_addr, ClientIP, sizeof(ClientIP));

        switch (command_code) {
        case 0x01: // echo
            printf("Client Message : %s\n", message + 2);
            break;

        case 0x02: // chat
            // 메시지 입력
            printf("Client Message : %s\n", message + 2);
            printf("Input Message : ");
            fgets(message + 2, BUFSIZE - 2, stdin);
            if (message[strlen(message + 2) - 1] == '\n') {
                message[strlen(message + 2) - 1] = '\0';
            }
            break;

        case 0x03: // stat 
            if (strcmp(message + 2, "bytes") == 0) {
                // 정수형 데이터를 문자열로 변환 
                itoa(total_bytes, message + 2, 10);
                strcat(message + 2, " bytes");
            }
            else if (strcmp(message + 2, "number") == 0) {
                // 정수형 데이터를 문자열로 변환
                itoa(total_messages, message + 2, 10);
                strcat(message + 2, " messages");
            }
            else {
                // 정수형 데이터를 문자열로 변환
                itoa(total_bytes, message + 2, 10);
                strcat(message + 2, " bytes, ");
                itoa(total_messages, message + 2 + strlen(message + 2), 10);
                strcat(message + 2, " messages");
            }
            break;

        case 0x04: // quit 
            // 종료 
            printf("Terminated\n");
            closesocket(s);
            WSACleanup();
            return 0;

        default:
            printf("Unknown Command\n");
            continue;
        }

        // 응답 메시지 전송
        snprintf(response, BUFSIZE, "%s:%d가 %s:%d로부터 %d 바이트 메시지 수신: %s", ServerIP - 2, PORT, ClientIP, ntohs(clientAddr.sin_port), retval, message + 2);
        retval = sendto(s, response, strlen(response), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
        if (retval == SOCKET_ERROR) {
            fprintf(stderr, "sendto() failed\n");
            continue;
        }
    }
}