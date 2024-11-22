#define _CRT_SECURE_NO_WARNINGS
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netdb.h>
#include <time.h>

#define ADDR_STR_MAX 128

void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);
void ifprint(pcap_if_t* d);
const char* iptos(struct sockaddr* sockaddr);
void analyze_packets(const char* filename);
void display_packet_details(const u_char *data, struct pcap_pkthdr *header);
void print_statistics(int packet_count, int byte_count, int tcp_count, int udp_count);

int main(int argc, char **argv) {
    pcap_if_t *alldevs;
    pcap_if_t *d;
    int inum, i = 0;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *adhandle;
    char filename[256] = "capture.pcap";
    int maxpknum = 1000, promiscuous_mode = 0;
    pcap_dumper_t *dumper;
    int num=0;

    printf("Calling pcap_findalldevs...\n");
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
        return -1;
    }

    printf("pcap_findalldevs called successfully.\n");
    printf("\n--- Device list ---\n");
    for (d = alldevs; d; d = d->next) {
        num = i + 1;
        printf("#%d: ", num);
        ifprint(d);
        i++;
    }

    if (i == 0) {
        printf("\nNo interfaces found! Make sure libpcap is installed.\n");
        pcap_freealldevs(alldevs);
        return -1;
    }

    printf("\nEnter the interface number (1-%d): ", i);
    while (scanf("%d", &inum) != 1 || inum < 1 || inum > i) {
        printf("Invalid input. Please enter a number between 1 and %d: ", i);
        while (getchar() != '\n'); 
    }

    printf("Enable Promiscuous mode? (1 for Yes, 0 for No): ");
    scanf("%d", &promiscuous_mode);
    printf("Enter maximum number of packets to be captured (default: 1000): ");
    scanf("%d", &maxpknum);
    printf("Enter filename to save capture (default: capture.pcap): ");
    scanf("%s", filename);

    for (d = alldevs, i = 0; i < inum - 1; d = d->next, i++);
    if ((adhandle = pcap_open_live(d->name, 65536, promiscuous_mode, 10000, errbuf)) == NULL) {
        fprintf(stderr, "\nUnable to open the adapter. %s is not supported by libpcap\n", d->name);
        pcap_freealldevs(alldevs);
        return -1;
    }

    printf("\nListening on %s... Press Ctrl+C to stop...\n", d->description);

    dumper = pcap_dump_open(adhandle, filename);
    if (dumper == NULL) {
        fprintf(stderr, "Error opening dump file: %s\n", pcap_geterr(adhandle));
        pcap_freealldevs(alldevs);
        pcap_close(adhandle);
        return -1;
    }

    pcap_freealldevs(alldevs);

    pcap_loop(adhandle, maxpknum, packet_handler, (u_char *)dumper);

    pcap_dump_close(dumper);
    pcap_close(adhandle);

    printf("\nCapture complete. Saved to file: %s\n", filename);

    analyze_packets(filename);

    return 0;
}

void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data) {
    static int packet_number=0;
    packet_number++;

    size_t custom_data_len = sizeof(int) + header->caplen;
    u_char* custom_data = (u_char*)malloc(custom_data_len);
    if (custom_data == NULL) {
        fprintf(stderr, "Error allocating memory for custom data\n");
        return;
    }

    printf("--- Captured Packet ---\n");
    printf("Packet number: %d\n", packet_number);
    printf("Capture Time: %s", ctime((const time_t *)&header->ts.tv_sec));
    printf("Captured packet of length %d bytes\n", header->len);
    printf("Number of bytes captured: %d\n", header->caplen);
    
    struct ether_header *eth_header = (struct ether_header *)pkt_data;
    printf("Ethernet Header:\n");
    printf("\tSource MAC: %s\n", ether_ntoa((const struct ether_addr *)&eth_header->ether_shost));
    printf("\tDestination MAC: %s\n", ether_ntoa((const struct ether_addr *)&eth_header->ether_dhost));
    printf("\tType: 0x%04x\n", ntohs(eth_header->ether_type));

    if (ntohs(eth_header->ether_type) == ETHERTYPE_IP) {
        struct ip *ip_header = (struct ip *)(pkt_data + sizeof(struct ether_header));

        printf("IP Header:\n");
        printf("\tSource IP: %s\n", inet_ntoa(ip_header->ip_src));
        printf("\tDestination IP: %s\n", inet_ntoa(ip_header->ip_dst));
        
        const char *protocol = NULL;
        switch(ip_header->ip_p) {
            case IPPROTO_TCP:
                protocol = "TCP";
                break;
            case IPPROTO_UDP:
                protocol = "UDP";
                break;
            case IPPROTO_ICMP:
                protocol = "ICMP";
                break;
            case IPPROTO_IGMP:
                protocol = "IGMP";
                break;
            case IPPROTO_IP:
                protocol = "IP";
                break; 
            default:
                protocol = "OTHER";
                break;
        }
        printf("\tProtocol: %s\n", protocol);

        if (ip_header->ip_p == IPPROTO_TCP) {
            struct tcphdr *tcp_header = (struct tcphdr *)(pkt_data + sizeof(struct ether_header) + (ip_header->ip_hl * 4));
            printf("TCP Header:\n");
            printf("\tSource Port: %d\n", ntohs(tcp_header->th_sport));
            printf("\tDestination Port: %d\n", ntohs(tcp_header->th_dport));
        } else if (ip_header->ip_p == IPPROTO_UDP) {
            struct udphdr *udp_header = (struct udphdr *)(pkt_data + sizeof(struct ether_header) + (ip_header->ip_hl * 4));
            printf("UDP Header:\n");
            printf("\tSource Port: %d\n", ntohs(udp_header->uh_sport));
            printf("\tDestination Port: %d\n", ntohs(udp_header->uh_dport));
        }
    } else {
        printf("Non-IP Packet.\n");
    }

    memcpy(custom_data, &packet_number, sizeof(int));
    memcpy(custom_data + sizeof(int), pkt_data, header->caplen);

    struct pcap_pkthdr custom_header = *header;
    custom_header.caplen = custom_data_len;
    custom_header.len = custom_data_len;

    pcap_dump(param, header, pkt_data); // Save packet to file
    free(custom_data);
}

void ifprint(pcap_if_t* d) {
    pcap_addr_t* a;
    printf("%s\n", d->name);
    if (d->description) printf("\tDescription: %s\n", d->description);
    printf("\tLoopback: %s\n", (d->flags & PCAP_IF_LOOPBACK) ? "yes" : "no");

    for (a = d->addresses; a; a = a->next) {
        if (a->addr && a->addr->sa_family == AF_INET) {
            printf("\tIPv4 Address: %s\n", iptos(a->addr));
        }
        if(a->netmask && a->netmask->sa_family == AF_INET) {
            printf("\tSubnet Mask: %s\n", iptos(a->netmask));
        }
        if(a->broadaddr && a->broadaddr->sa_family == AF_INET) {
            printf("\tBroadcast Address: %s\n", iptos(a->broadaddr));
        }
    }
    printf("\n");
}

const char* iptos(struct sockaddr* sockaddr) {
    static char address[ADDR_STR_MAX] = { 0 };
    if (getnameinfo(sockaddr, sizeof(struct sockaddr_storage), address, ADDR_STR_MAX, NULL, 0, NI_NUMERICHOST) != 0) {
        return "ERROR!";
    }
    return address;
}

void analyze_packets(const char* filename) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_offline(filename, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Error opening file: %s\n", errbuf);
        return;
    }

    struct pcap_pkthdr *header;
    const u_char *data;
    int packet_count = 0, byte_count = 0, tcp_count = 0, udp_count = 0;
    int result;
    int command = 0;

    printf("Analyzing packets...\n");

    while(1){
        printf("--- Enter Command ---\n");
        printf("0: Display all packets details\n");
        printf("1: Display next packet details\n");
        printf("Number to anlayze specific packet\n");

        scanf("%d", &command);
        if(command == 0){
            while (pcap_next_ex(handle, &header, &data) >= 0) {
                packet_count++;
                byte_count += header->len;
                if(ntohs(((struct ether_header *)data)->ether_type) == ETHERTYPE_IP){
                    struct ip *ip_header = (struct ip *)(data + sizeof(struct ether_header));
                if(ip_header->ip_p == IPPROTO_TCP){
                    tcp_count++;
                } else if(ip_header->ip_p == IPPROTO_UDP){
                    udp_count++;
                }
            }
                display_packet_details(data, header);
            }

            print_statistics(packet_count, byte_count, tcp_count, udp_count);
            break;
        } else if(command == 1){
            result = pcap_next_ex(handle, &header, &data);
            if(result >= 0){
                display_packet_details(data, header);
            } else {
                printf("No more packets to display\n");
            }
        } else if(command > 1){
            int packet_number = command;
            for(int i = 0; i < packet_number; i++){
                result = pcap_next_ex(handle, &header, &data);
                if(result < 0){
                    printf("No more packets to display\n");
                    break;
                }
            }
            if(result >= 0){
                display_packet_details(data, header);
            }
        }
    }

    pcap_close(handle);
}

void display_packet_details(const u_char *data, struct pcap_pkthdr *header) {
    printf("\nPacket Length: %d bytes\n", header->len);
    printf("Number of bytes captured: %d\n", header->caplen);
    printf("Capture Time: %s", ctime((const time_t *)&header->ts.tv_sec));

    struct ether_header *eth_header = (struct ether_header *)data;

    printf("Ethernet Header\n");
    printf("\tSource MAC: %s\n", ether_ntoa((const struct ether_addr *)&eth_header->ether_shost));
    printf("\tDestination MAC: %s\n", ether_ntoa((const struct ether_addr *)&eth_header->ether_dhost));
    printf("\tType: %u\n", ntohs(eth_header->ether_type));

    if (ntohs(eth_header->ether_type) == ETHERTYPE_IP) {
        struct ip *ip_header = (struct ip *)(data + sizeof(struct ether_header));
        printf("IP Header\n");
        printf("\tSource IP: %s\n", inet_ntoa(ip_header->ip_src));
        printf("\tDestination IP: %s\n", inet_ntoa(ip_header->ip_dst));
        printf("\tProtocol: %d\n", ip_header->ip_p);
        
        if(ip_header->ip_p == IPPROTO_TCP) {
            struct tcphdr *tcp_header = (struct tcphdr *)(data + sizeof(struct ether_header) + sizeof(struct ip));
            printf("TCP Header\n");
            printf("\tSource Port: %d\n", ntohs(tcp_header->th_sport));
            printf("\tDestination Port: %d\n", ntohs(tcp_header->th_dport));
        } else if(ip_header->ip_p == IPPROTO_UDP) {
            struct udphdr *udp_header = (struct udphdr *)(data + sizeof(struct ether_header) + sizeof(struct ip));
            printf("UDP Header\n");
            printf("\tSource Port: %d\n", ntohs(udp_header->uh_sport));
            printf("\tDestination Port: %d\n", ntohs(udp_header->uh_dport));
        }
    } else {
        printf("Non-IP Packet\n");
    }
}

void print_statistics(int packet_count, int byte_count, int tcp_count, int udp_count) {
    printf("\n--- Packet Statistics ---\n");
    printf("Total packets captured: %d\n", packet_count);
    printf("Total bytes captured: %d\n", byte_count);
    printf("Total TCP packets: %d\n", tcp_count);
    printf("Total UDP packets: %d\n", udp_count);
}