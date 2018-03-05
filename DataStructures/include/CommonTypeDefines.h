#ifndef __COMMON_TYPE_DEFINES__
#define __COMMON_TYPE_DEFINES__

#if defined _WIN32 || defined _WIN64
#define __SSE__
#endif

#ifdef __ARM_NEON__
#include <arm_neon.h>
#else
#include <xmmintrin.h>
#if defined _WIN32 || defined _WIN64
#include <emmintrin.h>
#endif
#ifdef __APPLE__
#include "TargetConditionals.h"
#endif
#endif

#if !defined _WIN32 && !defined _WIN64
typedef int __s32;
typedef unsigned int __u32;
typedef __s32 __i128 __attribute__((__vector_size__(16)));
typedef unsigned long long __u64;
typedef long long __s64;
typedef __s64 __m128i __attribute__((__vector_size__(16)));
typedef unsigned short __u16;
typedef short __s16;
typedef unsigned char __u8;
typedef char __s8;
#else
typedef __int32 __s32;
typedef unsigned __int32 __u32;
typedef _CRT_ALIGN(16) __s32 __i128[4];
typedef __int64 __s64;
typedef unsigned __int64 __u64;
typedef unsigned __int16 __u16;
typedef __int16 __s16;
typedef unsigned __int8 __u8;
typedef __int8 __s8;
#endif

#if !defined _WIN32 && !defined _WIN64
#include <math.h>
#ifdef __ARM_NEON__
typedef float32x4_t __m128;
#endif
#endif

#include <time.h>
#if !defined _WIN32 && !defined _WIN64
typedef time_t  __time_t;
typedef time_t __time_t64;
#define __time(t) time(t)
#define __mktime(t) mktime(t)
#define __mktime64(t) mktime(t)
#define __localtime(t) localtime(t)
#define __localtime64(t) localtime(t)
#else
typedef __time32_t  __time_t;
typedef __time64_t  __time_t64;
#define __time(t) _time32(t)
#define __mktime(t) _mktime32(t)
#define __mktime64(t) _mktime64(t)
#define __localtime(t) _localtime32(t)
#define __localtime64(t) _localtime64(t)
#endif

#ifdef __GET_TIME_64BIT
typedef __u64 HexTime;
#else
typedef __u32 HexTime;
#endif
typedef __u64 HexTimeNS;
#include <string.h>
#include <sys/stat.h>
#if !defined _WIN32 && !defined _WIN64
#include <unistd.h>
#define __mkdir(t)  mkdir(t, 0755)
#define __chsize	ftruncate
#define __snprintf snprintf
#else
#include <io.h>
#include <direct.h>
#define __mkdir(t)	_mkdir(t)
#define __chsize	_chsize
#define getcwd		_getcwd
#define __snprintf _snprintf
#endif


#define MATH_FLOAT_SMALL            1.0e-37f
#define MATH_TOLERANCE              2e-37f
#define MATH_DEG_TO_RAD(x)          ((x) * 0.0174532925f)
#define MATH_RAD_TO_DEG(x)          ((x)* 57.29577951f)
#define MATH_RANDOM_MINUS1_1()      ((2.0f*((float)rand()/RAND_MAX))-1.0f)      // Returns a random float between -1 and 1.
#define MATH_RANDOM_0_1()           ((float)rand()/RAND_MAX)                    // Returns a random float between 0 and 1.
#define MATH_E                      2.71828182845904523536f
#define MATH_LOG10E                 0.4342944819032518f
#define MATH_LOG2E                  1.442695040888963387f
#define MATH_PI                     3.14159265358979323846f
#define MATH_PIOVER2                1.57079632679489661923f
#define MATH_PIOVER4                0.785398163397448309616f
#define MATH_PIX2                   6.28318530717958647693f
#define MATH_EPSILON                0.000001f
#define MATH_CLAMP(x, lo, hi)       ((x < lo) ? lo : ((x > hi) ? hi : x))
#ifndef M_1_PI
#define M_1_PI                      0.31830988618379067154
#endif


#endif

