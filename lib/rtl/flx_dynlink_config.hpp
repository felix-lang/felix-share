#line 1274 "/Users/skaller/felix/src/packages/dynlink.fdoc"
#ifndef __FLX_DYNLINK_CONFIG_H__
#define __FLX_DYNLINK_CONFIG_H__
#include "flx_rtl_config.hpp"
#ifdef BUILD_DYNLINK
#define DYNLINK_EXTERN FLX_EXPORT
#else
#define DYNLINK_EXTERN FLX_IMPORT
#endif
#endif
