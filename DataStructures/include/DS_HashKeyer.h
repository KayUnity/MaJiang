#ifndef __HASH_KEYER_H__
#define __HASH_KEYER_H__

#include "HexString.h"
#include "SuperFastHash.h"

namespace DataStructures
{
	template <class KeyType> 
	class HashKeyer 
	{
	protected:
		KeyType key;
		unsigned int hashKey;
	public:
		HashKeyer()
		{
			hashKey = 0;
		}

		HashKeyer(KeyType _key)
		{
			key = _key;
			CalculateHashKey();
		}
		
		void SetKey(KeyType _key) 
		{ 
			key = _key; 
			CalculateHashKey();
		}

		void CalculateHashKey();
		inline unsigned int Get() { return hashKey; }
	};

    template <>
    inline void HashKeyer<HexString>::CalculateHashKey()
    {
        hashKey = SuperFastHash(key.C_String(), (__u32)key.GetLength());
    }

    template <>
    inline void HashKeyer<__u64>::CalculateHashKey()
    {
        hashKey = SuperFastHash((char *)&key, sizeof(key));
    }
    
    template <>
    inline void HashKeyer<__s64>::CalculateHashKey()
    {
        hashKey = SuperFastHash((char *)&key, sizeof(key));
    }
    
    template <>
    inline void HashKeyer<__u32>::CalculateHashKey()
    {
        hashKey = SuperFastHash((char *)&key, sizeof(key));
    }
    
    template <>
    inline void HashKeyer<__s32>::CalculateHashKey()
    {
        hashKey = SuperFastHash((char *)&key, sizeof(key));
    }
    
} // End namespace

#endif 
