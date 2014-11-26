/**
    Transfer file by ICMP
    By: liu feiran
*/
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <getopt.h>

typedef unsigned char u8;
typedef unsigned short int u16;
 
unsigned long daddr;
unsigned long saddr;
int payload_size = 0, sent, sent_size, sockfd;
char *packet;

int max_buffer_size = 1400; 
 
void usage(char *progname)
{
        printf("Usage: %s <dest-ip> [options]\n"
                        "\tOptions:\n"
                        "\t-s, --source-ip\t\tsource ip\n"
                        "\t-f, --file-name\t\tfile name\n"
                        "\t-m, --max-buffer-size\t\tmax buffer size\n"
                        "\t-h, --help\n"
                        "\n"
                        , progname);
}

/*
    Function calculate checksum
*/
unsigned short in_cksum(unsigned short *ptr, int nbytes)
{
    register long sum;
    u_short oddbyte;
    register u_short answer;
 
    sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
 
    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char *) & oddbyte) = *(u_char *) ptr;
        sum += oddbyte;
    }
 
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
 
    return (answer);
}

int create_send_socket() 
{
    //Raw socket - if you use IPPROTO_ICMP, then kernel will fill in the correct ICMP header checksum, if IPPROTO_RAW, then it wont
    sockfd = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);
    
    if (sockfd < 0)
    {
        perror("could not create socket");
        return (1);
    }
     
    int on = 1;
     
    // We shall provide IP headers
    if (setsockopt (sockfd, IPPROTO_IP, IP_HDRINCL, (const char*)&on, sizeof (on)) == -1)
    {
        perror("setsockopt");
        return (2);
    }
     
    //allow socket to send datagrams to broadcast addresses
    if (setsockopt (sockfd, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof (on)) == -1)
    {
        perror("setsockopt");
        return (3);
    }  
     
    return(0);
}

int read_file(char *buffer, FILE *file, int read_size)
{
    //char c;
    //int size = 0;
    //while( --read_size > 0 && ( c = getc(file)) != EOF ) {
    //    *buffer++ = c;
    //    size++;
    //}
    //return(size);
    return(0);
}

int send_file(FILE *file, int max_buffer_size)
{
    char *buffer_hdr;
    int buffer_size = 0;
    //Calculate total packet size
    payload_size = max_buffer_size;
    int packet_size = sizeof (struct iphdr) + sizeof (struct icmphdr) + payload_size;
    char *packet = (char *) malloc (packet_size);
                    
    if (!packet)
    {
        perror("out of memory");
        close(sockfd);
        return (1);
    }
    //ip header
    struct iphdr *ip = (struct iphdr *) packet;
    struct icmphdr *icmp = (struct icmphdr *) (packet + sizeof (struct iphdr));
     
    //zero out the packet buffer
    memset (packet, 0, packet_size);
 
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons (packet_size);
    ip->id = 0x1337;
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_ICMP;
    ip->saddr = saddr;
    ip->daddr = daddr;
    //ip->check = in_cksum ((u16 *) ip, sizeof (struct iphdr));
 
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.sequence = rand();
    icmp->un.echo.id = rand();
    //checksum
    icmp->checksum = 0;
     
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = daddr;
    memset(&servaddr.sin_zero, 0, sizeof (servaddr.sin_zero));
    
    buffer_hdr = packet + sizeof(struct iphdr) + sizeof(struct icmphdr);

    //while((buffer_size = read_file(buffer_hdr, file, max_buffer_size)) != 0) {
    while((buffer_size = fread(buffer_hdr, 1, max_buffer_size, file)) != 0) {
        icmp->checksum = 0;
        icmp->checksum = in_cksum((unsigned short *)icmp, sizeof(struct icmphdr) + buffer_size);
        packet_size = sizeof (struct iphdr) + sizeof (struct icmphdr) + buffer_size;
	printf("read %d ", buffer_size);
        if ( (sent_size = sendto(sockfd, packet, packet_size, 0, (struct sockaddr*) &servaddr, sizeof (servaddr))) < 1)
        {
            perror("send failed\n");
    	    return (2);
        }

        ++sent;
        printf("%d packets sent\r", sent);
        fflush(stdout);

        buffer_hdr = packet + sizeof(struct iphdr) + sizeof(struct icmphdr);
	usleep(200);
    }

    //memset(packet + sizeof(struct iphdr) + sizeof(struct icmphdr), rand() % 255, payload_size);
    //recalculate the icmp header checksum since we are filling the payload with random characters everytime
    //icmp->checksum = 0;
    //icmp->checksum = in_cksum((unsigned short *)icmp, sizeof(struct icmphdr) + payload_size);
       
    return (0);
}
 
int main(int argc, char **argv)
{
    int arg_options;
    int quit = 0;

    FILE *file;

    const char *short_options = "s:f:m:h";

    const struct option long_options[] = {
        {"source-ip", required_argument, NULL, 's'},
        {"file-name", required_argument, NULL, 'f'},
        {"max-buffer-size", required_argument, NULL, 'm'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    while ((arg_options =
        getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {

        switch (arg_options) {

        case 's':
            saddr = inet_addr(optarg);
            break;
        case 'f':
	    file= fopen(optarg, "r");
            break;
        case 'm':
            max_buffer_size = atoi(optarg);
            break;
        case 'h':
            usage(argv[0]);
            return 0;
            break;
        default:
            printf("CMD line Options Error\n\n");
            break;
        }
    }

    /* target IP */
    if (optind < argc) {
        daddr = inet_addr(argv[optind]);
    } else {
        quit = 1;
    }

    if (quit ||(! file)) {
        usage(argv[0]);
        exit(0);
    }

    create_send_socket();

    
    send_file(file, max_buffer_size);

    free(packet);
    close(sockfd);
}
