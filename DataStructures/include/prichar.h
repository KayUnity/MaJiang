#ifndef __PRICHAR_H_INCLUDED
#define __PRICHAR_H_INCLUDED

//#include "tchar.h"

#if defined __cplusplus
extern "C" {
#endif
    
    ////////////////////////////////////////
    // char
    ////////////////////////////////////////
    
#if defined(_MSC_VER)||defined(__BORLANDC__)
#define PRIcA    "hc"
#define PRIsA    "hs"
#elif defined(__GNUC__)||defined(_WIN32)||defined(_WIN64)
#if defined(_UNICODE)
#define PRIcA    "hc"
#define PRIsA    "hs"
#else
#define PRIcA    "c"
#define PRIsA    "s"
#endif
#else
#define PRIcA    "c"
#define PRIsA    "s"
#endif
    
    
    ////////////////////////////////////////
    // wchar_t
    ////////////////////////////////////////
    
#define PRIcW    "lc"
#define PRIsW    "ls"
    
    
    ////////////////////////////////////////
    // TCHAR
    ////////////////////////////////////////
    
#if defined(_WIN32)||defined(_WIN64)||defined(_MSC_VER)
 define PRIcT    "c"
#define PRIsT    "s"
#else
#if defined(_UNICODE)
#define PRIcT    PRIcW
#define PRIsT    PRIsW
#else
#define PRIcT    PRIcA
#define PRIsT    PRIsA
#endif
#endif
    
    
    ////////////////////////////////////////
    // SCN
    ////////////////////////////////////////
    
#define SCNcA    PRIcA
#define SCNsA    PRIsA
#define SCNcW    PRIcW
#define SCNsW    PRIsW
#define SCNcT    PRIcT
#define SCNsT    PRIsT
    
    
#if defined __cplusplus
};
#endif

#endif    // #ifndef __PRICHAR_H_INCLUDED