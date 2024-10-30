#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <WinSock2.h>

#define BUFSIZE 512
#define PORT 12345
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

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);
    retval = bind(s, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (retval == SOCKET_ERROR) {
        fprintf(stderr, "bind() failed\n");
        exit(1);
    }

    // 변수
    SOCKADDR_IN clientAddr;
    int clientAddrLength = sizeof(clientAddr);
    double p = 0.5; // 0과 1 사이의 확률값 (기본값 0.5)
    double random = 0; // 0과 1 사이의 random number
    double max = 32767; // rand()의 최대값
    char message[BUFSIZE + 3] = "";
    unsigned short seqNum = 0; // sequence number 저장
    char buf[BUFSIZE + 1] = ""; // 메시지 저장
    size_t bufLength = 0;
    bool isRunning = true;

    int messageCount = 0; // client가 전송한 메시지의 개수 (재전송 메시지 제외)
    int retransmissionCount = 0; // client가 재전송한 메시지의 개수
    int total = 0; // 전송한 메시지의 총 개수
    double rate = 0; // 재전송률

    // p값 설정
    fprintf(stdout, "0과 1 사이의 확률값 입력 (default= 0.5): ");
    fscanf(stdin, "%lf", &p);

    while (isRunning == true) {
        // 변수 초기화
        memset(message, 0, sizeof(message));
        memset(buf, 0, sizeof(buf));

        // 클라이언트로부터 메시지 수신
        retval = recvfrom(s, message, BUFSIZE + 3, 0, (SOCKADDR*)&clientAddr, &clientAddrLength);
        if (retval == SOCKET_ERROR) {
            fprintf(stderr, "recvfrom() failed.\n");
            exit(1);
        }

        // seqNum과 메시지 내용을 수신한 message로부터 복사 
        memcpy(&seqNum, message, 2);
        seqNum = ntohs(seqNum);
        bufLength = retval - 2;
        if (bufLength > BUFSIZE) {
            bufLength = BUFSIZE;
        }
        memcpy(buf, message + 2, bufLength);
        buf[bufLength] = '\0';

        fprintf(stdout, "Client( %s : %d )로부터 메시지:[%d][%s]\n\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), seqNum, buf);

        // 0과 1 사이의 확률값 생성
        random = (double)rand() / max;

        if (random <= p) {
            // 메시지 echo
            retval = sendto(s, message, strlen(message), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
            if (retval == SOCKET_ERROR) {
                fprintf(stderr, "sendto() failed\n");
                exit(1);
            }

            messageCount++;

            // 종료 메시지 처리
            if (strcmp(buf, "quit") == 0 || strcmp(buf, "QUIT") == 0) {
                fprintf(stdout, "Client( %s : %d ) 종료\n\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                isRunning = false;
                break;
            }

            fprintf(stdout, "메시지 echo 완료\n\n");
        }
        else {
            // Packet loss로 판단. 메시지 손실 처리
            fprintf(stdout, "메시지 손실 처리\n\n");
            retransmissionCount++;
        }
    }
    total = messageCount + retransmissionCount;
    rate = (double)retransmissionCount / total;

    // 통계 정보 출력
    fprintf(stdout, "통계 정보: p= %lf, N1= %d, N2= %d, N3= %d, R= %lf\n", p, messageCount, retransmissionCount, total, rate);

    closesocket(s);
    WSACleanup();
    return 0;
}