#include "send_thread.h"

static void make_package_hello(struct msghdr *msg, uint64_t id, uint16_t size) {
  uint8_t *header = malloc(sizeof(uint8_t)*4); 
  header[0] = 93;
  header[1] = 2;
  *((uint16_t*)header+2) = size;
  struct iovec h_iov = {0};
  h_iov.iov_len = (sizeof(uint8_t) * 4);
  h_iov.iov_base = header;
  msg->msg_iov[0] = h_iov;
  msg->msg_iov[1] = *(hello_long(myid, id));
}

static short send_hello_to(int sock, node_t *list) {
  struct msghdr msg = {0};
  struct sockaddr_in6 sin6 = {0};
  int rc = 0;
  node_t *current = list;
  while(current != NULL) {
    ip_port_t *addr_intel = (ip_port_t*)current->key;
    neighbor_t *n_intel = (neighbor_t*)current->value;
    memset(&msg, 0 ,sizeof(msg));
    memset(&sin6, 0 ,sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    inet_pton(AF_INET6, (char *)addr_intel->ipv6, &sin6.sin6_addr);
    sin6.sin6_port = htons(addr_intel->port);
    msg.msg_name = &sin6;
    msg.msg_namelen = sizeof(sin6);
    msg.msg_iov = malloc(sizeof(struct iovec)*2);
    make_package_hello(&msg,n_intel->id,htons(18));
    msg.msg_iovlen = 2;
    rc = sendmsg(sock, &msg, 0);
    if(rc < 0)
      return rc;
    current = current->next;
  }
  return 0;
}

// Thread sender
void *hello_sender(void *sock) {
  while(1) {
    sleep(5);
    printf("READING => neighbors\n");
    lock(&lock_n);
    node_t *n_list = map_to_list(neighbors);
    unlock(&lock_n);
    send_hello_to(*((int*)sock), n_list);
    if (n_list != NULL)
      freedeepnode(n_list);
    printf("READING Done => neighbors\n");
  }
}

// Thread init give s
// Partage des descripteurs de fichiers
short init_sender(int *s) {
  pthread_t th;
  if(pthread_create(&th, NULL, hello_sender, s)) {
    fprintf(stderr, "Can't initialise thread sender\n");
    return 0;
  }
  return 1;
}
 


