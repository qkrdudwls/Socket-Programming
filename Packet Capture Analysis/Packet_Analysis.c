#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#pragma comment("wpcap.lib")
#pragma comment("ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>

#define LINE_LEN 16

#define ETHERTYPE_IP		0x0800
#define ETH_II_HSIZE		14		
#define IP_HSIZE		20		
#define IP_PROTO_TCP		6		
#define IP_PROTO_UDP		17		

struct ethernet_header {
	uint8_t dest_mac[6];   
	uint8_t src_mac[6];   
	uint16_t eth_type;     
};

// IPv4 Header
struct ip_header {
	uint8_t  ihl : 4;      
	uint8_t  version : 4;  
	uint8_t  tos;         
	uint16_t tot_len;      
	uint16_t id;           
	uint16_t frag_off;     
	uint8_t  ttl;          
	uint8_t  protocol;     
	uint16_t checksum;     
	uint32_t saddr;       
	uint32_t daddr;        
};

struct tcp_header {
	uint16_t src_port;       
	uint16_t dst_port;       
	uint32_t seq_num;        
	uint32_t ack_num;       
	uint16_t hlen_flags;     
	uint16_t window;        
	uint16_t checksum;       
	uint16_t urg_ptr;      
};

struct udp_header {
	uint16_t src_port;       
	uint16_t dst_port;       
	uint16_t length;        
	uint16_t checksum;       
};

struct ethernet_header	eth_hdr;
struct ip_header		ip_hdr;
struct tcp_header		tcp_hdr;
struct udp_header		udp_hdr;

#define pntoh16(p)  ((unsigned short)                       \
                    ((unsigned short)*((unsigned char *)(p)+0)<<8|  \
                     (unsigned short)*((unsigned char *)(p)+1)<<0))

#define pntoh32(p)  ((unsigned short)*((unsigned char *)(p)+0)<<24|  \
                    (unsigned short)*((unsigned char *)(p)+1)<<16|  \
                    (unsigned short)*((unsigned char *)(p)+2)<<8|   \
                    (unsigned short)*((unsigned char *)(p)+3)<<0)

int parse_ip_header(unsigned char* data, struct ip_header* ip_hdr)
{
	ip_hdr->version = data[0] >> 4;		
	ip_hdr->ihl = data[0] & 0x0f;
	ip_hdr->protocol = data[9];		 
	ip_hdr->tot_len = pntoh16(&data[2]);
	ip_hdr->id = pntoh16(&data[4]);
	ip_hdr->frag_off = pntoh16(&data[6]);
	ip_hdr->ttl = data[8];
	ip_hdr->checksum = pntoh16(&data[10]);
	ip_hdr->saddr = pntoh32(&data[12]);
	ip_hdr->daddr = pntoh32(&data[16]);

	return 0;
}

int parse_ethernet_header(unsigned char* data, struct ethernet_header* eth_hdr)
{
	int i;

	for (i = 0; i < 6; i++) {
		eth_hdr->dest_mac[i] = data[i];
	}

	for (i = 0; i < 6; i++) {
		eth_hdr->src_mac[i] = data[i + 6];
	}

	eth_hdr->eth_type = pntoh16(&data[12]);

	return 0;
}

int parse_tcp_header(unsigned char* data, struct tcp_header* tcp_hdr)
{
	tcp_hdr->src_port = pntoh16(&data[0]);
	tcp_hdr->dst_port = pntoh16(&data[2]);
	tcp_hdr->seq_num = pntoh32(&data[4]);
	tcp_hdr->ack_num = pntoh32(&data[8]);
	tcp_hdr->hlen_flags = pntoh16(&data[12]);
	tcp_hdr->window = pntoh16(&data[14]);
	tcp_hdr->checksum = pntoh16(&data[16]);
	tcp_hdr->urg_ptr = pntoh16(&data[18]);

	return 0;
}

int parse_udp_header(unsigned char* data, struct udp_header* udp_hdr)
{
	udp_hdr->src_port = pntoh16(&data[0]);
	udp_hdr->dst_port = pntoh16(&data[2]);
	udp_hdr->length = pntoh16(&data[4]);
	udp_hdr->checksum = pntoh16(&data[6]);

	return 0;
}

void print_packet_info(int pk_no, double curr_time, struct ip_header* ip_hdr, struct tcp_header* tcp_hdr, struct udp_header* udp_hdr, int caplen) {
	char src_ip[16], dst_ip[16];
	snprintf(src_ip, sizeof(src_ip), "%d.%d.%d.%d",
		(ip_hdr->saddr & 0xFF), (ip_hdr->saddr >> 8) & 0xFF,
		(ip_hdr->saddr >> 16) & 0xFF, (ip_hdr->saddr >> 24) & 0xFF);
	snprintf(dst_ip, sizeof(dst_ip), "%d.%d.%d.%d",
		(ip_hdr->daddr & 0xFF), (ip_hdr->daddr >> 8) & 0xFF,
		(ip_hdr->daddr >> 16) & 0xFF, (ip_hdr->daddr >> 24) & 0xFF);

	printf("%-4d | %-8.6f | %-15s | %-15s | %-6s | %-5d | ",
		pk_no, curr_time, src_ip, dst_ip,
		(ip_hdr->protocol == IP_PROTO_TCP ? "TCP" : (ip_hdr->protocol == IP_PROTO_UDP ? "UDP" : "Other")),
		caplen);

	if (ip_hdr->protocol == IP_PROTO_TCP) {
		printf("%u -> %u : [Flags: %u]\n", tcp_hdr->src_port, tcp_hdr->dst_port, pntoh16(&tcp_hdr->hlen_flags) & 0x3F);
	}
	else if (ip_hdr->protocol == IP_PROTO_UDP) {
		printf("%u -> %u\n", udp_hdr->src_port, udp_hdr->dst_port);
	}
	else {
		printf("protocol: %u\n", ip_hdr->protocol);
	}
}

void print_header_details(struct ethernet_header* eth_hdr, struct ip_header* ip_hdr, struct tcp_header* tcp_hdr, struct udp_header* udp_hdr) {
	printf("Ethernet Header:\n");
	printf("  Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
		eth_hdr->src_mac[0], eth_hdr->src_mac[1], eth_hdr->src_mac[2],
		eth_hdr->src_mac[3], eth_hdr->src_mac[4], eth_hdr->src_mac[5]);
	printf("  Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
		eth_hdr->dest_mac[0], eth_hdr->dest_mac[1], eth_hdr->dest_mac[2],
		eth_hdr->dest_mac[3], eth_hdr->dest_mac[4], eth_hdr->dest_mac[5]);

	printf("IP Header:\n");
	printf("  Source IP: %d.%d.%d.%d\n",
		(ip_hdr->saddr & 0xFF), (ip_hdr->saddr >> 8) & 0xFF,
		(ip_hdr->saddr >> 16) & 0xFF, (ip_hdr->saddr >> 24) & 0xFF);
	printf("  Destination IP: %d.%d.%d.%d\n",
		(ip_hdr->daddr & 0xFF), (ip_hdr->daddr >> 8) & 0xFF,
		(ip_hdr->daddr >> 16) & 0xFF, (ip_hdr->daddr >> 24) & 0xFF);

	if (ip_hdr->protocol == IP_PROTO_TCP) {
		printf("TCP Header:\n");
		printf("  Source Port: %u\n", tcp_hdr->src_port);
		printf("  Destination Port: %u\n", tcp_hdr->dst_port);
		printf("  Sequence Number: %u\n", tcp_hdr->seq_num);
		printf("  Acknowledgment Number: %u\n", tcp_hdr->ack_num);
	}
	else if (ip_hdr->protocol == IP_PROTO_UDP) {
		printf("UDP Header:\n");
		printf("  Source Port: %u\n", udp_hdr->src_port);
		printf("  Destination Port: %u\n", udp_hdr->dst_port);
		printf("  Length: %u\n", udp_hdr->length);
	}
}

void main(int argc, char** argv)
{
	struct pcap_file_header	pcap_global_hdr;	
	struct pcap_pkthdr pcap_pk_hdr;			
	unsigned char pcap_pk_data[2000];		
	FILE* fin;
	int pk_no, res, offset = 0;
	double init_time, curr_time;	
	unsigned long net_ip_count = 0, net_etc_count = 0;
	unsigned long trans_tcp_count = 0, trans_udp_count = 0, trans_etc_count = 0;

	fin = fopen("ccc.pcap", "rb");
	if (fin == NULL) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}
	fread((char*)&pcap_global_hdr, sizeof(pcap_global_hdr), 1, fin);
	if (pcap_global_hdr.magic != 0xA1B2C3D4) {
		printf("파일 오류: 지원되지 않는 PCAP 파일 형식 (0x%x)\n", pcap_global_hdr.magic);
		exit(0);
	}

	pk_no = 0;
	while (1) {
		if (fread((char*)&pcap_pk_hdr, sizeof(pcap_pk_hdr), 1, fin) == 0)
			break;

		curr_time = pcap_pk_hdr.ts.tv_sec + pcap_pk_hdr.ts.tv_usec * 0.000001;
		if (pk_no == 0)
			init_time = curr_time;

		fread(pcap_pk_data, sizeof(unsigned char), pcap_pk_hdr.caplen, fin);

		offset = 0;
		res = parse_ethernet_header(&pcap_pk_data[offset], &eth_hdr);

		if (eth_hdr.eth_type == ETHERTYPE_IP) {
			offset += ETH_II_HSIZE;
			res = parse_ip_header(&pcap_pk_data[offset], &ip_hdr);
			net_ip_count++;

			offset += IP_HSIZE;
			if (ip_hdr.protocol == IP_PROTO_TCP) {
				res = parse_tcp_header(&pcap_pk_data[offset], &tcp_hdr);
				trans_tcp_count++;
			}
			else if (ip_hdr.protocol == IP_PROTO_UDP) {
				res = parse_udp_header(&pcap_pk_data[offset], &udp_hdr);
				trans_udp_count++;
			}
			else
				trans_etc_count++;
		}
		else
			net_etc_count++;

		pk_no++;

		static int print_summary_only = 0;

		if (!print_summary_only) {
			printf("Enter 1 to proceed to the next packet, or 0 to display summary only: ");
			int user_input = 1; scanf("%d", &user_input);
			if (user_input == 1) {
				print_packet_info(pk_no, curr_time, &ip_hdr, &tcp_hdr, &udp_hdr, pcap_pk_hdr.caplen);
				print_header_details(&eth_hdr, &ip_hdr, &tcp_hdr, &udp_hdr);
			}
			else if (user_input == 0) {
				print_summary_only = 1;
			}
		}
		if (print_summary_only) {
			print_packet_info(pk_no, curr_time, &ip_hdr, &tcp_hdr, &udp_hdr, pcap_pk_hdr.caplen);
		}

	}
	fclose(fin);

	printf("#total number of packets : %d\n", pk_no);
	printf("IP packets: %d\n", net_ip_count);
	printf("non-IP packets: %d\n", net_etc_count);
	printf("TCP packets: %d\n", trans_tcp_count);
	printf("UDP packets: %d\n", trans_udp_count);
}
