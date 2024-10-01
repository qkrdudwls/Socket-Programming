#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 512
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
    int retval = 0;

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

    // 서버의 IP 주소 및 포트 번호 입력 
    char serverIP[20] = "";
    int PORT = 0;
    printf("Input Server IP : ");
    scanf("%s", serverIP);
    printf("Input Server Port : ");
    scanf("%d", &PORT);
    getchar();

    // 소켓 주소 구조체 초기화
    SOCKADDR_IN serverAddr = { 0 };
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "inet_addr() failed\n");
        exit(1);
    }
    serverAddr.sin_port = htons(PORT);

    // 변수
    SOCKADDR_IN clientAddr;
    int clientAddrLength = 0;
    char command[5] = "";
    unsigned short command_code = 0;
    char stat_command[BUFSIZE] = "";
    char message[BUFSIZE + 1] = "";
    int msgLength = 0;
    int totalLength = 0;

    while (1) {
        // 명령어 및 메시지 초기화 
        command[0] = '\0';
        message[0] = '\0';

        // 명령어 입력
        printf("-Command List-\n");
        printf("1. echo\n2. chat\n3. stat\n4. quit\n");
        printf("Command: ");
        scanf("%s", command);
        getchar();

        if (strcmp(command, "echo") == 0) { // echo
            // echo 0x01
            command_code = htons(0x01);
            // Command_code를 메시지에 복사
            memcpy(message, &command_code, sizeof(command_code));

            // 메시지 입력
            printf("Input Message: ");
            fgets(message + 2, BUFSIZE - 2, stdin);
            // '\n' 문자 제거 
            msgLength = strlen(message + 2);
            if (message[msgLength - 1] == '\n') {
                message[msgLength - 1] = '\0';
            }

            // 메시지 전송
            totalLength = strlen(message + 2) + 2;
            retval = sendto(s, message, totalLength, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "sendto() failed\n");
                continue;
            }
            else {
                printf("Send Message!\n\n");
            }

            // 메시지 받기
            clientAddrLength = sizeof(clientAddr);
            retval = recvfrom(s, message, BUFSIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrLength);
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "recvfrom() failed\n");
                continue;
            }
            if (retval < BUFSIZE) {
                message[retval] = '\0';
            }

            // 송신자의 IP 주소 체크
            if (memcmp(&clientAddr, &serverAddr, sizeof(clientAddr)) != 0) {
                fprintf(stderr, "Invalid IP Address\n");
                continue;
            }

            // 받은 메시지 출력
            message[retval] = '\0';
            printf("%s\n", message + 2);
        }
        else if (strcmp(command, "chat") == 0) { // chat
            // chat 0x02
            command_code = htons(0x02);
            // Command code를 메시지에 복사 
            memcpy(message, &command_code, sizeof(command_code));

            // 메시지 입력
            printf("Input Message: ");
            fgets(message + 2, BUFSIZE - 2, stdin);
            msgLength = strlen(message + 2);
            if (message[msgLength - 1] == '\n') {
                message[msgLength - 1] = '\0';
            }

            // 메시지 전송
            totalLength = strlen(message + 2) + 2;
            retval = sendto(s, message, totalLength, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "sendto() failed\n");
                continue;
            }
            else {
                printf("Send Message!\n\n");
            }

            // 메시지 받기
            clientAddrLength = sizeof(clientAddr);
            retval = recvfrom(s, message, BUFSIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrLength);
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "recvfrom() failed\n");
                continue;
            }
            if (retval < BUFSIZE) {
                message[retval] = '\0';
            }

            // 송신자의 IP 주소 체크
            if (memcmp(&clientAddr, &serverAddr, sizeof(clientAddr)) != 0) {
                fprintf(stderr, "Invalid IP Address\n");
                continue;
            }

            // 받은 메시지 출력
            message[retval] = '\0';
            printf("%s\n", message + 2);
            continue;
        }
        else if (strcmp(command, "stat") == 0) { // stat
            // stat 명령어 입력
            printf("-Stat Command List-\n");
            printf("1. bytes\n2. number\n3. both\n");
            printf("Command: ");
            scanf("%s", stat_command);
            getchar();
            printf("\n");

            // stat 0x03
            command_code = htons(0x03);
            // Command code를 메시지에 복사
            memcpy(message, &command_code, sizeof(command_code));
            // stat 명령어 (bytes, number, both or etc.)를 커맨드 코드 뒤에 복사
            strcpy(message + 2, stat_command);

            // 메시지 전송
            totalLength = strlen(message + 2) + 2;
            retval = sendto(s, message, totalLength, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "sendto() failed\n");
                continue;
            }
            else {
                printf("Send Message!\n\n");
            }

            // 메시지 받기
            clientAddrLength = sizeof(clientAddr);
            retval = recvfrom(s, message, BUFSIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrLength);
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "recvfrom() failed\n");
                continue;
            }
            if (retval < BUFSIZE) {
                message[retval] = '\0';
            }

            // 송신자의 IP 주소 체크
            if (memcmp(&clientAddr, &serverAddr, sizeof(clientAddr)) != 0) {
                fprintf(stderr, "Invalid IP Address\n");
                continue;
            }

            // 받은 메시지 출력
            message[retval] = '\0';
            printf("%s\n", message + 2);

            // stat 명령어 초기화
            stat_command[0] = '\0';
            continue;
        }
        else if (strcmp(command, "quit") == 0) { // quit
            // quit 0x04
            command_code = htons(0x04);
            // Command code를 메시지에 복사 
            memcpy(message, &command_code, 2);

            // 종료 메시지 전송
            totalLength = strlen(message + 2) + 2;
            retval = sendto(s, message, totalLength, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "sendto() failed\n");
                continue;
            }

            // 종료 메시지 전송 시간 확보, 1초 대기 
            Sleep(1000);

            // 종료
            printf("Terminated\n");
            closesocket(s);
            WSACleanup();
            return 0;
        }
        else {
            printf("Invalid Command!\n");
            continue;
        }
    }
}
