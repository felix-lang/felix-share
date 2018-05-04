#line 153 "/Users/skaller/felix/src/packages/debug.fdoc"
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#define BUFLEN 2000
#define PORT 1153
int main()
{
  char buffer[BUFLEN];

  int sock = socket(PF_INET,SOCK_DGRAM,0); // 17=UDP
  struct sockaddr_in addr;
  memset((char *)&addr, 0, sizeof(sockaddr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(PORT);
  int result = bind (sock, (struct sockaddr*)&addr, sizeof(addr));
  if (result != 0)
    printf("UDP Trace Monitor: bind on port %d failed\n",PORT);
  printf("UDP Trace Monitor Listening on port %d\n",PORT);

  struct sockaddr_in writer;
  socklen_t addrlen = sizeof(writer);
  for(;;){
    memset(buffer,0,BUFLEN);
    result = recvfrom (sock, buffer, BUFLEN,0,(struct sockaddr*)&writer, &addrlen);
    printf("Received = %d\n",result);
    printf("Buffer = %s\n",buffer);
  }
}
