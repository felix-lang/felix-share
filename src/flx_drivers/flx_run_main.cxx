#line 490 "/Users/skaller/felix/src/packages/driver.fdoc"
#include "flx_run.hpp"
#include "stdio.h"

// to set the critical error handler
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#endif

int main(int argc, char **argv)
{
  #ifdef _WIN32
  SetErrorMode (SEM_FAILCRITICALERRORS);
  #endif
  //fprintf(stderr,"Felix mainline flx_run_main starts!\n");
  return felix_run(argc, argv);
}
