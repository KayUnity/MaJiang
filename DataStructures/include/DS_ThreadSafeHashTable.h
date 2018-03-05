#ifndef __THREAD_SAFE_HASH_TABLE_H__
#define __THREAD_SAFE_HASH_TABLE_H__

#include "SimpleMutex.h"
#include "DS_HashTable.h"

namespace DataStructures
{
	template <class KeyType, class DataType> 
	class ThreadSafeHashTable 
	{
	private:
        typedef HashTable<KeyType, DataType> __hashTable;
        
		HashTable<KeyType, DataType> mTable;
		SimpleMutex mutex;
	public:
		ThreadSafeHashTable(int startingHashNodes = __hashTable::DEFAULT_BUCKETS);
		virtual ~ThreadSafeHashTable() {}

		ThreadSafeHashTable &operator = (const HashTable<KeyType, DataType> &table);
		ThreadSafeHashTable &operator = (const ThreadSafeHashTable<KeyType, DataType> &table);
		DataType &operator [] ( const KeyType &key );

		bool InHashTable(const KeyType &key);

		int GetHashTableSize();
		int GetHashEntriesSize();

		typename HashTable<KeyType, DataType>::HashNode *Add(const KeyType &key, const DataType &data);
		bool Del(const KeyType &key);
		typename HashTable<KeyType, DataType>::HashNode *FindHashNode(const KeyType &key);
		bool SetData(const KeyType &key, const DataType &data);

		ThreadSafeHashTable &Clear();

		HashTable<KeyType, DataType> *LockTable();
		void ReleaseTable();
	};

	template <class KeyType, class DataType>
	HashTable<KeyType, DataType> *ThreadSafeHashTable<KeyType, DataType>::LockTable()
	{
		mutex.Lock();
		return &mTable;
	}

	template <class KeyType, class DataType>
	void ThreadSafeHashTable<KeyType, DataType>::ReleaseTable()
	{
		mutex.Unlock();
	}

	template <class KeyType, class DataType>
	ThreadSafeHashTable<KeyType, DataType> &ThreadSafeHashTable<KeyType, DataType>::operator = (const HashTable<KeyType, DataType> &table) 
	{
		HashTable<KeyType, DataType> *self = LockTable();
		*self = table;
		ReleaseTable();
		return *this;
	}

	template <class KeyType, class DataType>
	ThreadSafeHashTable<KeyType, DataType> &ThreadSafeHashTable<KeyType, DataType>::operator = (const ThreadSafeHashTable<KeyType, DataType> &table) 
	{
		HashTable<KeyType, DataType> *self = LockTable();
		*self = *table.LockTable();
		table.ReleaseTable();
		ReleaseTable();
		return *this;
	}

	template <class KeyType, class DataType>
	ThreadSafeHashTable<KeyType, DataType>::ThreadSafeHashTable(int startingHashNodes) : mTable(startingHashNodes)
	{}

	template <class KeyType, class DataType>
	typename HashTable<KeyType, DataType>::HashNode *ThreadSafeHashTable<KeyType, DataType>::Add( const KeyType &key, const DataType &data ) 
	{
		class HashTable<KeyType, DataType>::HashNode *node = LockTable()->Add(key, data);
		ReleaseTable();
		return node;
	}

	template <class KeyType, class DataType>
	bool ThreadSafeHashTable<KeyType, DataType>::InHashTable(const KeyType &key)  
	{
		bool res = LockTable()->InHashTable(key);
		ReleaseTable();
		return res;
	}

	template <class KeyType, class DataType>
	typename HashTable<KeyType, DataType>::HashNode *ThreadSafeHashTable<KeyType, DataType>::FindHashNode(const KeyType &key)
	{
		class HashTable<KeyType, DataType>::HashNode *node = LockTable()->FindHashNode(key);
		ReleaseTable();
		return node;
	}

	template <class KeyType, class DataType>
	ThreadSafeHashTable<KeyType, DataType> &ThreadSafeHashTable<KeyType, DataType>::Clear() 
	{
		LockTable()->Clear();
		ReleaseTable();
		return *this;
	}

	template <class KeyType, class DataType>
	bool ThreadSafeHashTable<KeyType, DataType>::Del(const KeyType &key)
	{
		bool res = LockTable()->Del(key);
		ReleaseTable();
		return res;
	}

	template <class KeyType, class DataType>
	DataType &ThreadSafeHashTable<KeyType, DataType>::operator[] (const KeyType &key) 
	{
		DataType &res = (*LockTable())[key];
		ReleaseTable();
		return res;
	}

	template <class KeyType, class DataType>
	bool ThreadSafeHashTable<KeyType, DataType>::SetData(const KeyType &key, const DataType &data)
	{
		bool res = LockTable()->SetData(key, data);
		ReleaseTable();
		return res;
	}

	template <class KeyType, class DataType>
	int ThreadSafeHashTable<KeyType, DataType>::GetHashTableSize()
	{
		int res = LockTable()->GetHashTableSize();
		ReleaseTable();
		return res;
	}
	
	template <class KeyType, class DataType>
	int ThreadSafeHashTable<KeyType, DataType>::GetHashEntriesSize()
	{
		int res = LockTable()->GetHashEntriesSize();
		ReleaseTable();
		return res;
	}

} // End namespace

#endif 
