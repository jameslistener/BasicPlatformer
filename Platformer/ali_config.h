#ifndef _ALIALO_CONFIG_H
#define _ALIALO_CONFIG_H


#if defined(WIN32)

/* we use the std C APIs, not the secure Microsoft versions */
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

typedef char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef long int32_t;
typedef unsigned long uint32_t;
typedef unsigned int uint_t;

#else

#include <stdint.h>
#define strnicmp strncasecmp

#endif

#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdarg.h>

#ifndef __cplusplus
typedef unsigned char bool;
#define true (!0)
#define false (!1)
#endif


#endif /* _ALIALO_CONFIG_H */
