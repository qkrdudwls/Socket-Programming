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

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup() failed\n");
        exit(1);
    }

    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed\n");
        exit(1);
    }

    char serverIP[20] = "";
    int PORT = 0;
    printf("Input Server IP : ");
    scanf("%s", serverIP);
    printf("Input Server Port : ");
    scanf("%d", &PORT);
    getchar();

    SOCKADDR_IN serverAddr = { 0 };
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "inet_addr() failed\n");
        exit(1);
    }
    serverAddr.sin_port = htons(PORT);

    SOCKADDR_IN clientAddr;
    int clientAddrLength = 0;
    char command[5] = "";
    unsigned short command_code = 0;
    char stat_command[BUFSIZE] = "";
    char message[BUFSIZE + 1] = "";
    int msgLength = 0;
    int totalLength = 0;

    while (1) { 
        command[0] = '\0';
        message[0] = '\0';

        printf("-Command List-\n");
        printf("1. echo\n2. chat\n3. stat\n4. quit\n");
        printf("Command: ");
        scanf("%s", command);
        getchar();

        if (strcmp(command, "echo") == 0) { 
            command_code = htons(0x01);
            memcpy(message, &command_code, sizeof(command_code));

            printf("Input Message: ");
            fgets(message + 2, BUFSIZE - 2, stdin);
            msgLength = strlen(message + 2);
            if (message[msgLength - 1] == '\n') {
                message[msgLength - 1] = '\0';
            }

            totalLength = strlen(message + 2) + 2;
            retval = sendto(s, message, totalLength, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "sendto() failed\n");
                continue;
            }
            else {
                printf("Send Message!\n\n");
            }

            clientAddrLength = sizeof(clientAddr);
            retval = recvfrom(s, message, BUFSIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrLength);
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "recvfrom() failed\n");
                continue;
            }
            if (retval < BUFSIZE) {
                message[retval] = '\0';
            }

            if (memcmp(&clientAddr, &serverAddr, sizeof(clientAddr)) != 0) {
                fprintf(stderr, "Invalid IP Address\n");
                continue;
            }

            message[retval] = '\0';
            printf("%s\n", message + 2);
        }
        else if (strcmp(command, "chat") == 0) {
            command_code = htons(0x02);
            memcpy(message, &command_code, sizeof(command_code));

            printf("Input Message: ");
            fgets(message + 2, BUFSIZE - 2, stdin);
            msgLength = strlen(message + 2);
            if (message[msgLength - 1] == '\n') {
                message[msgLength - 1] = '\0';
            }

            totalLength = strlen(message + 2) + 2;
            retval = sendto(s, message, totalLength, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "sendto() failed\n");
                continue;
            }
            else {
                printf("Send Message!\n\n");
            }

            clientAddrLength = sizeof(clientAddr);
            retval = recvfrom(s, message, BUFSIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrLength);
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "recvfrom() failed\n");
                continue;
            }
            if (retval < BUFSIZE) {
                message[retval] = '\0';
            }

            if (memcmp(&clientAddr, &serverAddr, sizeof(clientAddr)) != 0) {
                fprintf(stderr, "Invalid IP Address\n");
                continue;
            }

            message[retval] = '\0';
            printf("%s\n", message + 2);
            continue;
        }
        else if (strcmp(command, "stat") == 0) { 
            printf("-Stat Command List-\n");
            printf("1. bytes\n2. number\n3. both\n");
            printf("Command: ");
            scanf("%s", stat_command);
            getchar();
            printf("\n");

            command_code = htons(0x03);
            memcpy(message, &command_code, sizeof(command_code));
            strcpy(message + 2, stat_command);

            totalLength = strlen(message + 2) + 2;
            retval = sendto(s, message, totalLength, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "sendto() failed\n");
                continue;
            }
            else {
                printf("Send Message!\n\n");
            }

            clientAddrLength = sizeof(clientAddr);
            retval = recvfrom(s, message, BUFSIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrLength);
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "recvfrom() failed\n");
                continue;
            }
            if (retval < BUFSIZE) {
                message[retval] = '\0';
            }

            if (memcmp(&clientAddr, &serverAddr, sizeof(clientAddr)) != 0) {
                fprintf(stderr, "Invalid IP Address\n");
                continue;
            }

            message[retval] = '\0';
            printf("%s\n", message + 2);

            stat_command[0] = '\0';
            continue;
        }
        else if (strcmp(command, "quit") == 0) {
            command_code = htons(0x04);
            memcpy(message, &command_code, 2);

            totalLength = strlen(message + 2) + 2;
            retval = sendto(s, message, totalLength, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "sendto() failed\n");
                continue;
            }

            Sleep(1000);

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
