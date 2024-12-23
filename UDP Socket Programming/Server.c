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

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup() failed\n");
        exit(1);
    }

    int PORT = 0;
    printf("Input Server Port : ");
    scanf("%d", &PORT);
    while (getchar() != '\n');

    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed\n");
        exit(1);
    }

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

    SOCKADDR_IN clientAddr;
    int total_bytes = 0;
    int total_messages = 0;
    unsigned short command_code = 0;
    char message[BUFSIZE + 1] = "";
    int clientAddrLength = 0;
    char response[BUFSIZE + 1] = "";
    char ServerIP[INET_ADDRSTRLEN]; 
    char ClientIP[INET_ADDRSTRLEN]; 

    char hostname[256];
    retval = gethostname(hostname, sizeof(hostname));
    if (retval == SOCKET_ERROR) {
        fprintf(stderr, "gethostname() failed\n");
        exit(1);
    }

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

    inet_ntop(AF_INET, &((struct sockaddr_in*)res->ai_addr)->sin_addr, ServerIP, sizeof(ServerIP));

    freeaddrinfo(res);

    while (1) {
        clientAddrLength = sizeof(clientAddr);
        retval = recvfrom(s, message, BUFSIZE, 0, (SOCKADDR*)&clientAddr, &clientAddrLength);
        if (retval == SOCKET_ERROR) {
            fprintf(stderr, "recvfrom() failed\n");
            continue;
        }
        if (retval < BUFSIZE) {
            message[retval] = '\0';
        }

        total_messages++;

        memcpy(&command_code, message, sizeof(command_code));
        command_code = ntohs(*(unsigned short*)message);

        total_bytes += retval;

        inet_ntop(AF_INET, &clientAddr.sin_addr, ClientIP, sizeof(ClientIP));

        switch (command_code) {
        case 0x01: 
            printf("Client Message : %s\n", message + 2);
            break;

        case 0x02:
            printf("Client Message : %s\n", message + 2);
            printf("Input Message : ");
            fgets(message + 2, BUFSIZE - 2, stdin);
            if (message[strlen(message + 2) - 1] == '\n') {
                message[strlen(message + 2) - 1] = '\0';
            }
            break;

        case 0x03: 
            if (strcmp(message + 2, "bytes") == 0) {
                itoa(total_bytes, message + 2, 10);
                strcat(message + 2, " bytes");
            }
            else if (strcmp(message + 2, "number") == 0) {
                itoa(total_messages, message + 2, 10);
                strcat(message + 2, " messages");
            }
            else {
                itoa(total_bytes, message + 2, 10);
                strcat(message + 2, " bytes, ");
                itoa(total_messages, message + 2 + strlen(message + 2), 10);
                strcat(message + 2, " messages");
            }
            break;

        case 0x04: 
            printf("Terminated\n");
            closesocket(s);
            WSACleanup();
            return 0;

        default:
            printf("Unknown Command\n");
            continue;
        }

        snprintf(response, BUFSIZE, "%s:%d가 %s:%d로부터 %d 바이트 메시지 수신: %s", ServerIP - 2, PORT, ClientIP, ntohs(clientAddr.sin_port), retval, message + 2);
        retval = sendto(s, response, strlen(response), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
        if (retval == SOCKET_ERROR) {
            fprintf(stderr, "sendto() failed\n");
            continue;
        }
    }
}
