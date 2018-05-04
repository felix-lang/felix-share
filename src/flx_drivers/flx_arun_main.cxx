#line 469 "/Users/skaller/felix/src/packages/driver.fdoc"
#include "flx_run.hpp"

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
  return felix_arun(argc, argv);
}
