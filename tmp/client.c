#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>

#define REQUETE 14
#define RESPOND 18

int get_socket_server_addr(char *sname, int *s, struct sockaddr *saddr) {
  int rc = 0;
  struct addrinfo hints;
  struct addrinfo *r;
  memset(&hints,0,sizeof(saddr));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  //hints.ai_flags = AI_V4MAPPED | AI_ALL;
  rc = getaddrinfo(sname, "1212",&hints,&r);
  if (rc < 0) return rc;
  struct addrinfo *p = r;
  while (p != NULL) {
    if (p->ai_family != AF_INET6)
	   continue; 
    *s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (s >= 0) {
      *saddr = *((struct sockaddr*)p->ai_addr); 
      freeaddrinfo(r);
      return 0;
    }
  }
  printf("Can't find addr");
  freeaddrinfo(r);
  return -1;
}


void set_request(uint8_t *request) {
  request[0] = 93;
  request[1] = 2;
  request[2] = 0;
  request[3] = 10;
  request[4] = 2;
  request[5] = 8;
  request[13] = 122;
}

int sntp_req(int s, struct sockaddr *saddr) {
  uint32_t peer_size;
  uint8_t req[REQUETE];
  uint8_t resp[RESPOND];
  struct sockaddr_in6 peer;
  memset(req,0,REQUETE);
  memset(resp,0,RESPOND);
  set_request(req);
  for(int i = 0; i < REQUETE; i++) printf("%.2x ", req[i]);
  if(sendto(s,req,REQUETE, 0,saddr, sizeof(struct sockaddr_in6)) < 0) {
    printf("Echec 1\n");
    return -1;
  }
  printf("Request send ! \n Wait respond...\n");
  if(recvfrom(s,resp,RESPOND,0,(struct sockaddr *)&peer,&peer_size)){
    printf("Echec\n");
    return -1;
  }
  printf("Respond : %s\n", resp);
  return 0;
}

int main(int argc, char *argv[]) {
  if(argc != 2) {
    fprintf(stderr, "Wrong arg name! %s\n", argv[1]);
    exit(1);
  }
  struct sockaddr saddr; /* Serveur */
  int s = 0;
  memset(&saddr,0,sizeof(saddr));
  get_socket_server_addr(argv[1], &s, &saddr);
  sntp_req(s, &saddr);
  return 0;
}
