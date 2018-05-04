#line 84 "/Users/skaller/felix/src/packages/debug.fdoc"
#ifdef _WIN32
#include <stdio.h>
#include <string>
namespace flx { namespace debug {
void enable_local_udp_trace () {}
void send_udp_trace_message (::std::string msg) {
  fprintf(stderr,"[WIN32: udp_trace not available, using stderr] %s\n",msg.c_str());
}
}}
#else
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <string>
#include "flx_udp_trace.hpp"

namespace flx { namespace debug {
static int trace_socket = 0;
static struct sockaddr_in dst;
static int notify_first_send = 0;

void enable_local_udp_trace ()
{
  trace_socket = socket(PF_INET,SOCK_DGRAM,0); // 17=UDP
  struct sockaddr_in addr;
  memset((char *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = 0;
  int result = bind (trace_socket, (struct sockaddr*)&addr, sizeof(addr));
  if (result != 0) {
    fprintf(stderr,"FAILED to bind Trace Output Socket!\n");
    return;
  }
  fprintf(stderr,"Bound Trace Output Socket OK!\n");

  memset((char*)&dst,0,sizeof(dst));
  dst.sin_family=AF_INET;
  dst.sin_port = htons(1153);
  inet_aton("127.0.0.1",&dst.sin_addr);
}

// Add locks later
void send_udp_trace_message (::std::string msg)
{
  if (trace_socket != 0)
  {
    char const * cp = msg.c_str();
    size_t n = msg.size();
    int result = sendto (trace_socket, cp, n,0,(struct sockaddr*)&dst, sizeof(dst));
    if (notify_first_send == 0)
    {
      notify_first_send = 1;
      if (result == n)
        fprintf(stderr, "First UDP Trace message sent OK! %d bytes = '%s'\n", result,cp);
      else
        fprintf(stderr, "First UDP Trace message send FAILED ****! Sent: %d bytes\n",result);
    }
  }
}
}} // namespaces
#endif
