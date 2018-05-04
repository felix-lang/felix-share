#line 600 "/Users/skaller/felix/src/packages/regex.fdoc"
#ifndef __FLX_RE2_CONFIG_H__
#define __FLX_RE2_CONFIG_H__
#include "flx_rtl_config.hpp"
#ifdef BUILD_RE2
#define RE2_EXTERN FLX_EXPORT
#else
#define RE2_EXTERN FLX_IMPORT
#endif
#endif
