#ifndef __AES_H
#define __AES_H

#include <stdio.h>
#include "CommonTypeDefines.h"

#define AESMaxRounds  14

//#define AES_Err_Invalid_Key_Size        -1;  //Key size <> 128, 192, or 256 Bits
//#define AES_Err_Invalid_Mode            -2;  //Encr/Decr with Init for Decr/Encr
//#define AES_Err_Invalid_Length          -3;  //No full block for cipher stealing
//#define AES_Err_Data_After_Short_Block  -4;  //Short block must be last
//#define AES_Err_MultipleIncProcs        -5;  //More than one IncProc Setting    
//#define AES_Err_NIL_Pointer             -6;  //nil pointer to block with nonzero length

typedef unsigned char		word8;	
typedef __u16               word16;
typedef __u32               word32;

typedef struct
{
	int		keyData[AESMaxRounds+1][4];
	int		rounds;
}AESContext;

int init_AES(word8 *key, word16 keySize, AESContext *encContext, AESContext *decContext);
int blockEncrypt(AESContext *encContext, word8 *input, int inputLen, word8 *outBuffer);
int blockDecrypt(AESContext *decContext, word8 *input, int inputLen, word8 *outBuffer);


#endif // __AES_H  
