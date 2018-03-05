#ifndef __DS_QUICKLZ_CORE_H__
#define __DS_QUICKLZ_CORE_H__

#include "CommonTypeDefines.h"

namespace DataStructures
{
	/// This generates special cases of the huffman encoding tree using 8 bit keys with the additional condition that unused combinations of 8 bits are treated as a frequency of 1
	class QuickLZCoreLevel1
	{
	public:
		QuickLZCoreLevel1();
		~QuickLZCoreLevel1();

		__u32 EncodeArray( unsigned char *input, __u32 sizeInBytes, unsigned char **output );
		__u32 DecodeArray( unsigned char *input, unsigned char **output );
	private:
#define QLZ_COMPRESSION_LEVEL	1
#define QLZ_STREAMING_BUFFER	0
#define QLZ_POINTERS			1
#define QLZ_HASH_VALUES			4096
		typedef struct 
		{
			unsigned int cache[QLZ_POINTERS];
			const unsigned char *offset[QLZ_POINTERS];
		} qlz_hash_compress;

		typedef struct 
		{
			const unsigned char *offset[QLZ_POINTERS];
		} qlz_hash_decompress;


#define QLZ_ALIGNMENT_PADD 8
#define QLZ_BUFFER_COUNTER 8

#define QLZ_SCRATCH_COMPRESS (QLZ_ALIGNMENT_PADD + QLZ_BUFFER_COUNTER + QLZ_STREAMING_BUFFER + sizeof(qlz_hash_compress[QLZ_HASH_VALUES]) + QLZ_HASH_VALUES)

#define QLZ_SCRATCH_DECOMPRESS (QLZ_ALIGNMENT_PADD + QLZ_BUFFER_COUNTER + QLZ_STREAMING_BUFFER + sizeof(qlz_hash_decompress[QLZ_HASH_VALUES]) + QLZ_HASH_VALUES)

		int QLZ_Get_Setting(int seeting);
		static inline void Update_Hash(qlz_hash_decompress h[QLZ_HASH_VALUES], unsigned char counter[QLZ_HASH_VALUES], const unsigned char *s);
		static void Update_Hash_Upto(qlz_hash_decompress h[QLZ_HASH_VALUES], unsigned char counter[QLZ_HASH_VALUES], unsigned char **lh, const unsigned char *max);
		static __u32 QLZ_Compress_Core(const unsigned char *source, unsigned char *destination, __u32 size, qlz_hash_compress hashtable[QLZ_HASH_VALUES], unsigned char hash_counter[QLZ_HASH_VALUES]);
		static __u32 QLZ_Decompress_Core(const unsigned char *source, unsigned char *destination, __u32 size, qlz_hash_decompress hashtable[QLZ_HASH_VALUES], unsigned char hash_counter[QLZ_HASH_VALUES], unsigned char *history, const char *source_2);
	};

}

#endif
