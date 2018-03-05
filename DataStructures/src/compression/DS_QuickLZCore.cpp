#include "compression/DS_QuickLZCore.h"
#include <assert.h> 
#include <string.h>
#include <memory.h>

namespace DataStructures
{

#define MINOFFSET 2
#define UNCONDITIONAL_MATCHLEN 6
#define UNCOMPRESSED_END 4
#define CWORD_LEN 4

	QuickLZCoreLevel1::QuickLZCoreLevel1()
	{
	}

	QuickLZCoreLevel1::~QuickLZCoreLevel1()
	{
	}

	__s32 QuickLZCoreLevel1::QLZ_Get_Setting(__s32 setting)
	{
		switch (setting)
		{
		case 0: return QLZ_COMPRESSION_LEVEL;
		case 1: return QLZ_SCRATCH_COMPRESS;
		case 2: return QLZ_SCRATCH_DECOMPRESS;
		case 3: return QLZ_STREAMING_BUFFER;
		case 6: return 0;
		}
		return -1;
	}

	static void reset_state(unsigned char hash_counter[QLZ_HASH_VALUES])
	{
		memset(hash_counter, 0, QLZ_HASH_VALUES);
	}

	static inline __u32 hash_func(__u32 i)
	{
		return ((i >> 12) ^ i) & (QLZ_HASH_VALUES - 1);
	}

	static inline __u32 fast_read(void const *src, __u32 bytes)
	{
		unsigned char *p = (unsigned char*)src;
		switch (bytes)
		{
		case 4:
			return(*p | *(p + 1) << 8 | *(p + 2) << 16 | *(p + 3) << 24);
		case 3: 
			return(*p | *(p + 1) << 8 | *(p + 2) << 16);
		case 2:
			return(*p | *(p + 1) << 8);
		case 1: 
			return(*p);
		}
		return 0;
	}

	static inline void fast_write(__u32 f, void *dst, size_t bytes)
	{
		unsigned char *p = (unsigned char*)dst;

		switch (bytes)
		{
		case 4: 
			*p = (unsigned char)f;
			*(p + 1) = (unsigned char)(f >> 8);
			*(p + 2) = (unsigned char)(f >> 16);
			*(p + 3) = (unsigned char)(f >> 24);
			return;
		case 3:
			*p = (unsigned char)f;
			*(p + 1) = (unsigned char)(f >> 8);
			*(p + 2) = (unsigned char)(f >> 16);
			return;
		case 2:
			*p = (unsigned char)f;
			*(p + 1) = (unsigned char)(f >> 8);
			return;
		case 1:
			*p = (unsigned char)f;
			return;
		}
	}

	static inline void memcpy_up(unsigned char *dst, const unsigned char *src, __u32 n)
	{
		// Caution if modifying memcpy_up! Overlap of dst and src must be special handled.
		unsigned char *end = dst + n; // todo, optimize
		while(dst < end)
		{
			*dst = *src;
			dst++;
			src++;
		}
	}

	// Public functions of QuickLZ
	size_t qlz_size_decompressed(const char *source)
	{
		__u32 n, r;
		n = (((*source) & 2) == 2) ? 4 : 1;
		r = fast_read(source + 1 + n, n);
		r = r & (0xffffffff >> ((4 - n)*8));
		return r;
	}

	size_t qlz_size_compressed(const char *source)
	{
		__u32 n, r;
		n = (((*source) & 2) == 2) ? 4 : 1;
		r = fast_read(source + 1, n);
		r = r & (0xffffffff >> ((4 - n)*8));
		return r;
	}

	size_t qlz_size_decompressed(const char *source, size_t *len)
	{
		__u32 n, r;
		n = (((*source) & 2) == 2) ? 4 : 1;
		*len = n;
		r = fast_read(source + 1 + n, n);
		r = r & (0xffffffff >> ((4 - n)*8));
		return r;
	}

	size_t qlz_size_compressed(const char *source, size_t *len)
	{
		__u32 n, r;
		n = (((*source) & 2) == 2) ? 4 : 1;
		*len = n;
		r = fast_read(source + 1, n);
		r = r & (0xffffffff >> ((4 - n)*8));
		return r;
	}

	inline void QuickLZCoreLevel1::Update_Hash(qlz_hash_decompress h[QLZ_HASH_VALUES], unsigned char counter[QLZ_HASH_VALUES], const unsigned char *s)
	{
		__u32 hash, fetch;
		fetch = fast_read(s, 3);
		hash = hash_func(fetch);
		h[hash].offset[0] = s;
		counter[hash] = 1; // todo, updating counter
	}

	void QuickLZCoreLevel1::Update_Hash_Upto(qlz_hash_decompress h[QLZ_HASH_VALUES], unsigned char counter[QLZ_HASH_VALUES], unsigned char **lh, const unsigned char *max)
	{
		while(*lh < max)
		{
			(*lh)++;
			Update_Hash(h, counter, *lh);
		}
	}

	__u32 QuickLZCoreLevel1::QLZ_Compress_Core(const unsigned char *source, unsigned char *destination, __u32 size, qlz_hash_compress hashtable[QLZ_HASH_VALUES], unsigned char hash_counter[QLZ_HASH_VALUES])
	{
		const unsigned char *last_byte = source + size - 1;
		const unsigned char *src = source;
		unsigned char *cword_ptr = destination;
		unsigned char *dst = destination + CWORD_LEN;
		__u32 cword_val = 1U << 31;
		const unsigned char *last_matchstart = last_byte - UNCONDITIONAL_MATCHLEN - UNCOMPRESSED_END; 
		__u32 fetch = 0;
		__u32 lits = 0;

		(void) lits;

		if(src <= last_matchstart)
			fetch = fast_read(src, 3);

		while(src <= last_matchstart)
		{
			if ((cword_val & 1) == 1)
			{
				// store uncompressed if compression ratio is too low
				if (src > source + 3*(size >> 2) && dst - destination > src - source - ((src - source) >> 5))
					return 0;

				fast_write((cword_val >> 1) | (1U << 31), cword_ptr, CWORD_LEN);

				cword_ptr = dst;
				dst += CWORD_LEN;
				cword_val = 1U << 31;
				fetch = fast_read(src, 3);
			}
			{
				const unsigned char *o;
				__u32 hash, cached;

				hash = hash_func(fetch);

				cached = fetch ^ hashtable[hash].cache[0];
				hashtable[hash].cache[0] = fetch;

				o = hashtable[hash].offset[0];
				hashtable[hash].offset[0] = src;
				if (cached == 0 && hash_counter[hash] != 0 && (src - o > MINOFFSET || (src == o + 1 && lits >= 3 && src > source + 3 && *src == *(src - 3) && *src == *(src - 2) && *src == *(src - 1) && *src == *(src + 1) && *src == *(src + 2))))
				{
					if (*(o + 3) != *(src + 3))
					{
						cword_val = (cword_val >> 1) | (1U << 31);
						fast_write((3 - 2) | (hash << 4), dst, 2);
						src += 3;
						dst += 2;
					}
					else
					{
						const unsigned char *old_src = src;
						size_t matchlen;

						cword_val = (cword_val >> 1) | (1U << 31);
						src += 4;

						if(*(o + (src - old_src)) == *src)
						{
							src++;
							if(*(o + (src - old_src)) == *src)
							{
								size_t q = last_byte - UNCOMPRESSED_END - (src - 5) + 1;
								size_t remaining = q > 255 ? 255 : q;
								src++;	
								while(*(o + (src - old_src)) == *src && (size_t)(src - old_src) < remaining)
									src++;
							}
						}

						matchlen = src - old_src;
						hash <<= 4;
						if (matchlen < 18)
						{
							fast_write((__u32)(matchlen - 2) | hash, dst, 2);
							dst += 2;
						} 
						else
						{
							fast_write((__u32)(matchlen << 16) | hash, dst, 3);
							dst += 3;
						}
					}
					fetch = fast_read(src, 3);
					lits = 0;
				}
				else
				{
					lits++;
					hash_counter[hash] = 1; 
					*dst = *src;
					src++;
					dst++;
					cword_val = (cword_val >> 1);
					fetch = (fetch >> 8 & 0x0000ffff) | (* (src + 2) << 16);
				}
			}
		}

		while (src <= last_byte)
		{
			if ((cword_val & 1) == 1)
			{
				fast_write((cword_val >> 1) | (1U << 31), cword_ptr, CWORD_LEN);
				cword_ptr = dst;
				dst += CWORD_LEN;
				cword_val = 1U << 31;
			}
			if (src <= last_byte - 3)
			{
				__u32 hash;
				fetch = fast_read(src, 3);
				hash = hash_func(fetch);
				hashtable[hash].offset[0] = src;
				hashtable[hash].cache[0] = fetch;
				hash_counter[hash] = 1;
			}
			*dst = *src;
			src++;
			dst++;

			cword_val = (cword_val >> 1);
		}

		while((cword_val & 1) != 1)
			cword_val = (cword_val >> 1);

		fast_write((cword_val >> 1) | (1U << 31), cword_ptr, CWORD_LEN);

		// min. size must be 9 bytes so that the qlz_size functions can take 9 bytes as argument
		return (__u32)(dst - destination < 9 ? 9 : dst - destination);
	}

	__u32 QuickLZCoreLevel1::QLZ_Decompress_Core(const unsigned char *source, unsigned char *destination, __u32 size, qlz_hash_decompress hashtable[QLZ_HASH_VALUES], unsigned char hash_counter[QLZ_HASH_VALUES], unsigned char *history, const char *source_2)
	{
		const unsigned char *src = source;
		unsigned char *dst = destination;
		const unsigned char *last_destination_byte = destination + size - 1;
		__u32 cword_val = 1;
		const __u32 bitlut[16] = {4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
		const unsigned char *last_matchstart = last_destination_byte - UNCONDITIONAL_MATCHLEN - UNCOMPRESSED_END;
		unsigned char *last_hashed = destination - 1;
		const unsigned char *last_source_byte = (const unsigned char *)source_2 + qlz_size_compressed(source_2) - 1;

		(void) last_source_byte;
		(void) history;
		(void) last_hashed;
		(void) hash_counter;
		(void) hashtable;

		for(;;) 
		{
			__u32 fetch;

			if (cword_val == 1)
			{
				cword_val = fast_read(src, CWORD_LEN);
				src += CWORD_LEN;
			}

			fetch = fast_read(src, 4);

			if ((cword_val & 1) == 1)

			{
				__u32 matchlen;
				const unsigned char *offset2;

				__u32 hash;
				cword_val = cword_val >> 1;
				hash = (fetch >> 4) & 0xfff;
				offset2 = hashtable[hash].offset[0];

				if((fetch & 0xf) != 0)
				{
					matchlen = (fetch & 0xf) + 2;
					src += 2;
				}
				else
				{
					matchlen = *(src + 2);
					src += 3;							
				}	
				memcpy_up(dst, offset2, matchlen);
				dst += matchlen;

				Update_Hash_Upto(hashtable, hash_counter, &last_hashed, dst - matchlen);
				last_hashed = dst - 1;
			}

			else
			{
				if (dst < last_matchstart)
				{
					memcpy_up(dst, src, 4);
					dst += bitlut[cword_val & 0xf];
					src += bitlut[cword_val & 0xf];
					cword_val = cword_val >> (bitlut[cword_val & 0xf]);
					Update_Hash_Upto(hashtable, hash_counter, &last_hashed, dst - 3);		
				}
				else
				{			
					while(dst <= last_destination_byte)
					{
						if (cword_val == 1)
						{
							src += CWORD_LEN;
							cword_val = 1U << 31;
						}
						*dst = *src;
						dst++;
						src++;
						cword_val = cword_val >> 1;
					}

					Update_Hash_Upto(hashtable, hash_counter, &last_hashed, last_destination_byte - 3); // todo, use constant
					return size;
				}

			}
		}
	}

	__u32 QuickLZCoreLevel1::EncodeArray( unsigned char *input, __u32 sizeInBytes, unsigned char **output )
	{
		if(sizeInBytes == 0 || sizeInBytes > 0xffffffff - 400)
			return 0;

		char *scratch_compress = (char *)malloc(QLZ_SCRATCH_COMPRESS);
		//allocate source length + 400 as the destination buffer
		char *destination = (char *)malloc( sizeInBytes + 400 );
		unsigned char *scratch_aligned = (unsigned char *)scratch_compress + QLZ_ALIGNMENT_PADD - (((size_t)scratch_compress) % QLZ_ALIGNMENT_PADD);
		size_t *buffersize = (size_t *)scratch_aligned;
		qlz_hash_compress *hashtable = (qlz_hash_compress *)(scratch_aligned + QLZ_BUFFER_COUNTER);
		unsigned char *hash_counter = (unsigned char*)hashtable + sizeof(qlz_hash_compress[QLZ_HASH_VALUES]);
		size_t r;
		__u32 compressed;
		size_t base;

		if(sizeInBytes < 216)  // todo
			base = 3;
		else
			base = 9;

		{
			reset_state(hash_counter);
			r = base + QLZ_Compress_Core((const unsigned char *)input, (unsigned char*)destination + base, sizeInBytes, hashtable, hash_counter);
			if(r == base)
			{
				memcpy(destination + base, input, sizeInBytes);
				r = sizeInBytes + base;
				compressed = 0;
			}
			else
			{
				compressed = 1;
			}
			*buffersize = 0;
		}
		if(base == 3)
		{
			*destination = (unsigned char)(0 | compressed);
			*(destination + 1) = (unsigned char)r;
			*(destination + 2) = (unsigned char)sizeInBytes;
		}
		else
		{
			*destination = (unsigned char)(2 | compressed);
			fast_write((__u32)r, destination + 1, 4);
			fast_write((__u32)sizeInBytes, destination + 5, 4);
		}

		*destination |= (QLZ_COMPRESSION_LEVEL << 2);
		*destination |= (1 << 6);
		*destination |= ((QLZ_STREAMING_BUFFER == 0 ? 0 : (QLZ_STREAMING_BUFFER == 100000 ? 1 : (QLZ_STREAMING_BUFFER == 1000000 ? 2 : 3))) << 4);

		*output = (unsigned char *)destination;
		free( scratch_compress );
		
		return (__u32)r;
	}

	__u32 QuickLZCoreLevel1::DecodeArray( unsigned char *input, unsigned char **output )
	{
		char * scratch_compress = (char*) malloc(QLZ_SCRATCH_COMPRESS);
		//allocate source length + 400 as the destination buffer
		size_t len = qlz_size_decompressed((const char *)input);
		char * destination = (char *)malloc( len );

		unsigned char *scratch_aligned = (unsigned char *)scratch_compress + QLZ_ALIGNMENT_PADD - (((size_t)scratch_compress) % QLZ_ALIGNMENT_PADD);
		size_t *buffersize = (size_t *)scratch_aligned;
		qlz_hash_decompress *hashtable = (qlz_hash_decompress *)(scratch_aligned + QLZ_BUFFER_COUNTER); 
		unsigned char *hash_counter = (unsigned char*)hashtable + sizeof(qlz_hash_decompress[QLZ_HASH_VALUES]);
		__u32 headerlen = 2*((((*input) & 2) == 2) ? 4 : 1) + 1;
		size_t dsiz = qlz_size_decompressed((char *)input);

		if((*input & 1) == 1)
		{
			reset_state(hash_counter);
			dsiz = QLZ_Decompress_Core((unsigned char *)input + headerlen, (unsigned char *)destination, (__u32)dsiz, hashtable, hash_counter, (unsigned char *)destination, (const char *)input);
		}
		else
		{
			memcpy(destination, input + headerlen, dsiz);
		}
		*buffersize = 0;
		reset_state(hash_counter);
		free( scratch_compress );

		*output = (unsigned char *)destination;

		return (__u32)dsiz;
	}


}