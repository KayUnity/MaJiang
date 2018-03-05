#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include "DS_HashKeyer.h"
#include <assert.h>
#include <memory.h>

namespace DataStructures
{
	template <class KeyType, class DataType> 
	class HashTable 
	{
	public:
		class HashNode 
		{
		public:
			KeyType key;
			DataType data;
			//pointer to the next storage location in the hasnNode.a has node is essentially just a linked list.
			HashNode *prev;
			HashNode *next;
			//returns a newly instantiated object of type HashNode.
			HashNode(const KeyType &_key, const DataType &_data, HashNode *_prev = 0, HashNode *_next = 0 );
			HashNode() { key = 0; data = DataType(); prev = 0; next = 0; }
		};
		//java's built-in hash table uses Ko=89 as the default bucket size, as Kn = 2Kn+1 + 1 is a prime number after as many as 5 repetitions.
		const static int DEFAULT_BUCKETS = 89;
		const static double THRESHOLD;
	private:
		HashNode **hashNodes;
		int numberHashNodes;
		int numberHashEntries;

		int CalcuHash( const KeyType &key ) const;
		void ReHash(int newSize = 0);
		//grow() increases the size of the array based on the formula new_size = 2*old_size + 1
		void Grow();

		int defaultHashSize;
	public:
		class SimpleIterator
		{
		public:
			SimpleIterator(HashTable *hash)
			{
				hashTable = hash;
				for (currentIndex=0; currentIndex<hash->numberHashNodes; currentIndex++)
				{
					if (hash->hashNodes[currentIndex])
					{
						currentNode = hash->hashNodes[currentIndex];
						return;
					}
				}
				//currentIndex < 0 and currentNode = 0 means empty hash table or reach the end
				currentIndex = -1;
				currentNode = 0;
			}
			SimpleIterator(HashTable *hash, KeyType key)
			{
				hashTable = hash;
				currentNode = hashTable->FindHashNode(key);
				if (currentNode)
				{
					currentIndex = hashTable->CalcuHash(key);
				}
				else
					Reset();
			}
			
			void Reset()
			{
				SimpleIterator::SimpleIterator(hashTable);
			}

			DataType *GetData()
			{
				if (currentNode)
					return &(currentNode->data);
				else
					return 0;
			}

			HashNode *GetNode()
			{
				if (currentNode)
					return currentNode;
				else
					return 0;
			}

			void FindNext()
			{
				if (currentNode)
				{
					currentNode = currentNode->next;
					if (currentNode)
						return;
				}
				if (currentIndex<0)
					return;
				for (currentIndex=currentIndex+1; currentIndex<hashTable->numberHashNodes; currentIndex++ )
				{
					currentNode = hashTable->hashNodes[currentIndex];
					if (currentNode)
						return;
				}
				currentNode = 0;
				currentIndex = -1;
			}
		private:
			HashTable *hashTable;
			int currentIndex;
			HashNode *currentNode; 
		};

		HashTable(int startingHashNodes = DEFAULT_BUCKETS);
		~HashTable();

		HashTable &operator = (const HashTable &table);
		DataType &operator [] ( const KeyType &key );

		bool InHashTable(const KeyType &key);

		inline int GetHashTableSize() { return numberHashNodes; }
		inline int GetHashEntriesSize() { return numberHashEntries; }

		HashNode *Add(const KeyType &key, const DataType &data);
		bool Del(const KeyType &key);
		HashNode *FindHashNode(const KeyType &key);
		bool SetData(const KeyType &key, const DataType &data);

		HashTable &Clear();
	};

	template <class KeyType, class DataType>
	const double HashTable<KeyType, DataType>::THRESHOLD = 0.75;

	template <class KeyType, class DataType>
	HashTable<KeyType, DataType> &HashTable<KeyType, DataType>::operator = (const HashTable<KeyType, DataType> &table) 
	{
		HashNode *currentNode = 0;
		defaultHashSize = table.numberHashNodes;
		Clear();

		//Iterate over all the hashNodes in <table>, copying over each entry.
		for ( int nodeIndex = 0; nodeIndex < table.numberHashNodes; nodeIndex++ ) 
		{
			currentNode = table.hashNodes[nodeIndex];
			while( currentNode != 0 ) 
			{
				Add( currentNode->key, currentNode->data );
				currentNode = currentNode->next;
			}
		}
		return *this;
	}

	template <class KeyType, class DataType>
	int HashTable<KeyType, DataType>::CalcuHash(const KeyType &key) const 
	{
		HashKeyer<KeyType> calculater(key);
		return calculater.Get() % numberHashNodes;
	}

	template <class KeyType, class DataType>
	void HashTable<KeyType, DataType>::Grow() 
	{
		int currentPosition = numberHashNodes;
		numberHashNodes = 2*numberHashNodes + 1;
		hashNodes = (HashNode **)realloc( hashNodes, (sizeof( HashNode *) * numberHashNodes) );
		memset(&hashNodes[currentPosition], 0, sizeof(HashNode *) * (numberHashNodes - currentPosition));
		ReHash(numberHashNodes);
	}

	template <class KeyType, class DataType>
	void HashTable<KeyType, DataType>::ReHash(int newSize) 
	{
		//back old table
		HashNode **oldNodes = (HashNode **)calloc(sizeof(HashNode *), numberHashNodes);
		memcpy(oldNodes, hashNodes, numberHashNodes * sizeof(HashNode *));
		int oldSize = numberHashNodes;
		//allocate new hash table
		int actSize = numberHashNodes;
		if (newSize>numberHashNodes)
			actSize = newSize;
		numberHashNodes = actSize;
		numberHashEntries = 0;
		free(hashNodes);
		hashNodes = (HashNode **)calloc(sizeof(HashNode *), numberHashNodes);
		//a while loop to add backed data
		for (int i=0; i<oldSize; i++)
		{
			HashNode *currentNode = oldNodes[i];
			while (currentNode)
			{
				Add( currentNode->key, currentNode->data );
				currentNode = currentNode->next;
			}
		}
		//clear backed nodes
		for (int i=0; i<oldSize; i++)
		{
			HashNode *currentNode = oldNodes[i];
			while (currentNode)
			{
				HashNode *nextNode = currentNode->next;
				delete currentNode;
				currentNode = nextNode;
			}
		}
		//finally, free backed memory
		free(oldNodes);
	}

	template <class KeyType, class DataType>
	HashTable<KeyType, DataType>::~HashTable() 
	{
		int nodeIndex = 0;
		HashNode *currentNode = 0;
		HashNode *nextNode = 0;

		for( nodeIndex = 0; nodeIndex < numberHashNodes; nodeIndex++ ) 
		{
			currentNode = hashNodes[nodeIndex];
			while( currentNode != 0 ) 
			{
				nextNode = currentNode->next;
				delete currentNode;
				currentNode = nextNode;
			}
		}
		free(hashNodes);
	}

	template <class KeyType, class DataType>
	HashTable<KeyType, DataType>::HashTable(int startingHashNodes) :
		numberHashNodes(startingHashNodes), 
		numberHashEntries(0) 
	{
		defaultHashSize = startingHashNodes;
		hashNodes = (HashNode **)calloc(sizeof(HashNode *), numberHashNodes);
	}

	template <class KeyType, class DataType>
	typename HashTable<KeyType, DataType>::HashNode *HashTable<KeyType, DataType>::Add( const KeyType &key, const DataType &data ) 
	{
		if (InHashTable(key))
			return 0;

		if (((double)numberHashEntries / numberHashNodes) >= THRESHOLD)
			Grow();

		int hashValue = CalcuHash(key);
		numberHashEntries++;
		HashNode *newNode = new HashNode( key, data, 0, 0 );

		if( hashNodes[hashValue] != 0 )
		{
			newNode->prev = hashNodes[hashValue]->prev;		//should always be zero, i guess
			newNode->next = hashNodes[hashValue];
			hashNodes[hashValue]->prev = newNode;
		}
		hashNodes[hashValue] = newNode;

		return newNode;
	}

	template <class KeyType, class DataType>
	bool HashTable<KeyType, DataType>::InHashTable(const KeyType &key)  
	{
		return FindHashNode(key) != 0;
	}

	template <class KeyType, class DataType>
	typename HashTable<KeyType, DataType>::HashNode *HashTable<KeyType, DataType>::FindHashNode(const KeyType &key)
	{
		int hashValue = CalcuHash(key);
		HashNode *currentNode = hashNodes[hashValue];

		while( currentNode != 0 ) 
		{
			if (currentNode->key == key)
				return currentNode;
			currentNode = currentNode->next;
		}
		return 0;
	}

	template <class KeyType, class DataType>
	HashTable<KeyType, DataType> &HashTable<KeyType, DataType>::Clear() 
	{
		int nodeIndex;

		for (nodeIndex = 0; nodeIndex < numberHashNodes; nodeIndex++) 
		{
			HashNode *currentNode = hashNodes[nodeIndex];
			while( currentNode != 0 ) 
			{
				HashNode *nextNode = currentNode->next;
				delete currentNode;
				currentNode = nextNode;
			}
			hashNodes[nodeIndex] = 0;
		}

		numberHashEntries = 0;
		numberHashNodes = defaultHashSize;;
		hashNodes = (HashNode **)realloc(hashNodes, (sizeof( HashNode *) * numberHashNodes));

		return *this;
	}

	template <class KeyType, class DataType>
	bool HashTable<KeyType, DataType>::Del(const KeyType &key)
	{
		HashNode *node = FindHashNode( key );
		if (!node)
			return false;
		HashNode *prevNode = node->prev;
		HashNode *nextNode = node->next;
		delete node;
		if (prevNode)
			prevNode->next = nextNode;
		if (nextNode)
			nextNode->prev = prevNode;
		if (((long)prevNode | (long)nextNode) == 0)
		{
			int hashValue = CalcuHash(key);
			hashNodes[hashValue] = 0;
		}
		else
		{
			int hashValue = CalcuHash(key);
			if (prevNode)
				hashNodes[hashValue] = prevNode;
			else
				hashNodes[hashValue] = nextNode;
		}
		numberHashEntries --;
		return true;
	}

	template <class KeyType, class DataType>
	DataType &HashTable<KeyType, DataType>::operator[] (const KeyType &key) 
	{
		HashNode *foundNode = FindHashNode( key );
		assert(foundNode);
		return foundNode->data;
	}

	template <class KeyType, class DataType>
	bool HashTable<KeyType, DataType>::SetData(const KeyType &key, const DataType &data)
	{
		HashNode *foundNode = FindHashNode( key );
		if (!foundNode)
			return false;
		foundNode->data = data;
		return true;
	}

	template <class KeyType, class DataType>
	HashTable<KeyType, DataType>::HashNode::HashNode(const KeyType &_key, const DataType &_data, HashNode *_prev, HashNode *_next ) : 
		key(_key), 
		data(_data), 
		prev(_prev),
		next(_next) 
	{}

} // End namespace

#endif 
