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

int main(int argc, char **argv)
{
  int buffer_size = 2048;
  int packet_size = sizeof (struct iphdr) + sizeof (struct icmphdr) + buffer_size;
  int data_size;
  int ihl, i, n;
  char *data;

  FILE *file = fopen("out", "w+");
  char *packet;
  int sockfd;
  if ( (packet = malloc(packet_size)) == NULL)
  {
  	fprintf(stderr, "Could not allocate memory for packet\n");
	exit(-1);
  }
  struct iphdr *ip = (struct iphdr *) packet;
  struct icmphdr *icmp = (struct icmphdr *) (packet + sizeof (struct iphdr));
  if ( (sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
  {
    fprintf(stderr, "Failed to initialize socket\n");
    free(packet);
    exit(-1);
  }

  while(1) {
    if ((i = recvfrom(sockfd, packet, packet_size, 0, NULL, NULL)) == -1)
    {
      fprintf(stderr, "Failed to receive packets\n");
      free(packet);
      exit(-1);
    }
    ihl = (int)ip->ihl << 2;
    data = (char *)(packet + 28);
    //icmp = (struct icmphdr *)(packet + ihl);
    data_size = i - ihl - 8;
    printf("id: %x, all: %d, length %d \n", ntohs(ip->id), i, data_size);
    if( ntohs(ip->id) == 0x3713)
    {
      printf("write_file id: %x, length %d \n", ntohs(ip->id), data_size);
      //for(n = 0; n < data_size; n++)
        //printf("%x", *(data + n));
      //printf("\n");
      fwrite(data, data_size, 1, file);
      fflush(file);
    }
  }

}

