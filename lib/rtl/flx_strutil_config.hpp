#line 274 "/Users/skaller/felix/src/packages/strutil.fdoc"
#ifndef __FLX_STRUTIL_CONFIG_H__
#define __FLX_STRUTIL_CONFIG_H__
#include "flx_rtl_config.hpp"
#ifdef BUILD_STRUTIL
#define STRUTIL_EXTERN FLX_EXPORT
#else
#define STRUTIL_EXTERN FLX_IMPORT
#endif
#endif
