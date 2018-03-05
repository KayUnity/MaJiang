#define __UNUS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encription/AES.h"

#include "Rand.h"

#include "encription/AES-Boxes.h"


// AES key expansion, error if invalid key size
void aes_Init(AESContext* aesContext, word8 *key, word16 keySize)
{
	int *pk = (int *)aesContext->keyData[0];
	memcpy(pk, key, keySize << 2);
	aesContext->rounds = keySize + 6;
	switch (keySize) 
	{
		case 4:
			{
				for (int i=0; i<10; i++) 
				{
					unsigned int k = pk[3];
					pk[4] = (SBox[(k >> 8) & 0xff] | SBox[(k >> 16) & 0xff] << 8 | SBox[(k >> 24)] << 16 | SBox[k & 0xff] << 24) ^ pk[0] ^ RCon[i];
					pk[5] = pk[1] ^ pk[4];
					pk[6] = pk[2] ^ pk[5];
					pk[7] = pk[3] ^ pk[6];
					pk += 4;
				}
				break;
			}
		case 6:
			{
				for (int i=0; i<8; i++)
				{
					unsigned int k = pk[5];
					pk[6] = (SBox[(k >> 8) & 0x0ff] | SBox[(k >> 16) & 0x0ff] << 8 | SBox[(k >> 24)] << 16 | SBox[k & 0x0ff] << 24) ^pk[0] ^ RCon[i];
					pk[7] = pk[1] ^ pk[6];
					pk[8] = pk[2] ^ pk[7];
					pk[9] = pk[3] ^ pk[8];
					if (i==7)
						return;
					pk[10] = pk[4] ^ pk[9];
					pk[11] = pk[5] ^ pk[10];
					pk += 6;
				}
				break;
			};
		default:
			{
				for (int i=0; i<7; i++)
				{
					unsigned int k = pk[7];
					pk[8] = (SBox[(k >> 8) & 0x0ff] | SBox[(k >> 16) & 0x0ff] << 8 | SBox[(k >> 24)] << 16 | SBox[k & 0x0ff] << 24) ^ pk[0] ^ RCon[i];
					pk[9] = pk[1] ^ pk[8];
					pk[10] = pk[2] ^ pk[9];
					pk[11] = pk[3] ^ pk[10];
					if (i==6)
						return;
					k = pk[11];
					pk[12] = (SBox[k & 0x0ff] | SBox[(k >> 8) & 0x0ff] << 8 | SBox[(k >> 16) & 0x0ff] << 16 | SBox[k >> 24] << 24) ^ pk[4];
					pk[13] = pk[5] ^ pk[12];
					pk[14] = pk[6] ^ pk[13];
					pk[15] = pk[7] ^ pk[14];
					pk += 8;
				}
				break;
			}
	}
}

void aes_Encrypt(AESContext* aesContext, word32 B0[4])
{
	int *pk = (int *)aesContext->keyData[0];
	unsigned int s0 = B0[0] ^ pk[0];
	unsigned int s1 = B0[1] ^ pk[1];
	unsigned int s2 = B0[2] ^ pk[2];
	unsigned int s3 = B0[3] ^ pk[3];
	pk += 4;
	for (int i=1; i<aesContext->rounds; i++)
	{
		unsigned int b0 = Te0[s0 & 0x0ff] ^ Te1[(s1 >> 8) & 0x0ff] ^ Te2[(s2 >> 16) & 0x0ff] ^ Te3[s3 >> 24];
		unsigned int b1 = Te0[s1 & 0x0ff] ^ Te1[(s2 >> 8) & 0x0ff] ^ Te2[(s3 >> 16) & 0x0ff] ^ Te3[s0 >> 24];
		unsigned int b2 = Te0[s2 & 0x0ff] ^ Te1[(s3 >> 8) & 0x0ff] ^ Te2[(s0 >> 16) & 0x0ff] ^ Te3[s1 >> 24];
		unsigned int b3 = Te0[s3 & 0x0ff] ^ Te1[(s0 >> 8) & 0x0ff] ^ Te2[(s1 >> 16) & 0x0ff] ^ Te3[s2 >> 24];
		s0 = b0 ^ pk[0];
		s1 = b1 ^ pk[1];
		s2 = b2 ^ pk[2];
		s3 = b3 ^ pk[3];
		pk += 4;
	}
	B0[0] = (SBox[s0 & 0x0ff] | (SBox[(s1 >> 8) & 0x0ff] << 8) | (SBox[(s2 >> 16) & 0x0ff] << 16) | (SBox[s3 >> 24] << 24)) ^ pk[0];
	B0[1] = (SBox[s1 & 0x0ff] | (SBox[(s2 >> 8) & 0x0ff] << 8) | (SBox[(s3 >> 16) & 0x0ff] << 16) | (SBox[s0 >> 24] << 24)) ^ pk[1];
	B0[2] = (SBox[s2 & 0x0ff] | (SBox[(s3 >> 8) & 0x0ff] << 8) | (SBox[(s0 >> 16) & 0x0ff] << 16) | (SBox[s1 >> 24] << 24)) ^ pk[2];
	B0[3] = (SBox[s3 & 0x0ff] | (SBox[(s0 >> 8) & 0x0ff] << 8) | (SBox[(s1 >> 16) & 0x0ff] << 16) | (SBox[s2 >> 24] << 24)) ^ pk[3];
}

void aes_Decrypt(AESContext *aesContext, word32 BO[4])
{
	int *pk = (int *)aesContext->keyData[aesContext->rounds];
	unsigned int s0 = BO[0] ^ pk[0];
	unsigned int s1 = BO[1] ^ pk[1];
	unsigned int s2 = BO[2] ^ pk[2];
	unsigned int s3 = BO[3] ^ pk[3];
	pk -= 4;
	for (int i=1; i<aesContext->rounds; i++)
	{
		unsigned int b0 = Td0[s0 & 0x0ff] ^ Td1[s3 >> 8 & 0x0ff] ^ Td2[s2 >> 16 & 0x0ff] ^ Td3[s1 >> 24];
		unsigned int b1 = Td0[s1 & 0x0ff] ^ Td1[s0 >> 8 & 0x0ff] ^ Td2[s3 >> 16 & 0x0ff] ^ Td3[s2 >> 24];
		unsigned int b2 = Td0[s2 & 0x0ff] ^ Td1[s1 >> 8 & 0x0ff] ^ Td2[s0 >> 16 & 0x0ff] ^ Td3[s3 >> 24];
		unsigned int b3 = Td0[s3 & 0x0ff] ^ Td1[s2 >> 8 & 0x0ff] ^ Td2[s1 >> 16 & 0x0ff] ^ Td3[s0 >> 24];
		s0 = b0 ^ pk[0];
		s1 = b1 ^ pk[1];
		s2 = b2 ^ pk[2];
		s3 = b3 ^ pk[3];
		pk -= 4;
	}
	BO[0] = (InvSBox[s0 & 0x0ff] | InvSBox[s3 >>  8 & 0x0ff] <<  8 | InvSBox[s2 >> 16 & 0x0ff] << 16 | InvSBox[s1 >> 24] << 24) ^ pk[0];
	BO[1] = (InvSBox[s1 & 0x0ff] | InvSBox[s0 >>  8 & 0x0ff] <<  8 | InvSBox[s3 >> 16 & 0x0ff] << 16 | InvSBox[s2 >> 24] << 24) ^ pk[1];
	BO[2] = (InvSBox[s2 & 0x0ff] | InvSBox[s1 >>  8 & 0x0ff] <<  8 | InvSBox[s0 >> 16 & 0x0ff] << 16 | InvSBox[s3 >> 24] << 24) ^ pk[2];
	BO[3] = (InvSBox[s3 & 0x0ff] | InvSBox[s2 >>  8 & 0x0ff] <<  8 | InvSBox[s1 >> 16 & 0x0ff] << 16 | InvSBox[s0 >> 24] << 24) ^ pk[3];
}

int init_AES(word8 *key, word16 keySize, AESContext *encContext, AESContext *decContext)
{
	memset(encContext->keyData, 0, sizeof(encContext->keyData));
	aes_Init(encContext, key, keySize);
	*decContext = *encContext;
	int *p = (int *)decContext->keyData[1];
	for (int i=1; i<(decContext->rounds-1) * 4 + 1; i++)
	{
		*p = Td0[SBox[(unsigned int)*p & 0x0ff]] ^ Td1[SBox[((unsigned int)*p >> 8) & 0x0ff]] ^ Td2[SBox[((unsigned int)*p >> 16) & 0x0ff]] ^ Td3[SBox[(unsigned int)*p >> 24]];
		p++;
	}
	return 0;
}


int blockEncrypt(AESContext *encContext, word8 *input, int inputLen, word8 *outBuffer)
{
	if (!input || !outBuffer || inputLen<=0)
		return 0;
	int n = inputLen/16;
	if (!(input==outBuffer))
		memcpy(outBuffer, input, (unsigned int)inputLen);
	word8 *p = outBuffer;
	for (int i=0; i<n; i++)
	{
		aes_Encrypt(encContext, (word32 *)p);
		p += 16;
	}
	return inputLen;
}


int blockDecrypt(AESContext *decContext, word8 *input, int inputLen, word8 *outBuffer)
{
	if (!input || !outBuffer || inputLen<=0)
		return 0;
	int n = inputLen/16;
	if (!(input==outBuffer))
		memcpy(outBuffer, input, (unsigned int)inputLen);
	word8 *p = outBuffer;
	for (int i=0; i<n; i++)
	{
		aes_Decrypt(decContext, (word32 *)p);
		p += 16;
	}
	return inputLen;
}


