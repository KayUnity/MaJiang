#ifndef __THREAD_SAFE_ORDERED_LIST_H
#define __THREAD_SAFE_ORDERED_LIST_H

#include "DS_OrderedList.h"
#include "SimpleMutex.h"

namespace DataStructures
{
	template <class key_type, class data_type, int (*default_comparison_function)(const key_type&, const data_type&)=defaultOrderedListComparison<key_type, data_type> >
	class ThreadSafeOrderedList
	{
	private:
		OrderedList<key_type, data_type, default_comparison_function> mOrderedList;
		SimpleMutex mutex;
	public:
		ThreadSafeOrderedList() {}
		virtual ~ThreadSafeOrderedList() {}

		ThreadSafeOrderedList( const OrderedList<key_type, data_type, default_comparison_function>& original_copy )
		{
			*LockList() = original_copy;
			ReleaseList();
		}
		ThreadSafeOrderedList( const ThreadSafeOrderedList<key_type, data_type, default_comparison_function>& original_copy )
		{
			*LockList() = *original_copy.LockList();
			ReleaseList();
			original_copy.ReleaseList();
		}
		ThreadSafeOrderedList& operator= ( const OrderedList<key_type, data_type, default_comparison_function>& original_copy )
		{
			*LockList() = original_copy;
			ReleaseList();
			return *this;
		}
		ThreadSafeOrderedList& operator= ( const ThreadSafeOrderedList<key_type, data_type, default_comparison_function>& original_copy )
		{
			*LockList() = *original_copy.LockList();
			ReleaseList();
			original_copy.ReleaseList();
			return *this;
		}

		OrderedList<key_type, data_type, default_comparison_function> *LockList() 
		{ 
			mutex.Lock(); 
			return &mOrderedList; 
		}
		void ReleaseList() 
		{ 
			mutex.Unlock(); 
		}

		bool HasData(const key_type &key, int (*cf)(const key_type&, const data_type&)=default_comparison_function)
		{
			bool res = LockList()->HasData(key, cf);
			ReleaseList();
			return res;
		}
		unsigned GetIndexFromKey(const key_type &key, bool *objectExists, int (*cf)(const key_type&, const data_type&)=default_comparison_function)
		{
			unsigned int index = LockList()->GetIndexFromKey(key, objectExists, cf);
			ReleaseList();
			return index;
		}
		data_type GetElementFromKey(const key_type &key, int (*cf)(const key_type&, const data_type&)=default_comparison_function)
		{
			data_type data = LockList()->GetElementFromKey(key, cf);
			ReleaseList();
			return data;
		}
		bool GetElementFromKey(const key_type &key, data_type &element, int (*cf)(const key_type&, const data_type&)=default_comparison_function)
		{
			bool res = LockList()->GetElementFromKey(key, element, cf);
			ReleaseList();
			return res;
		}
		unsigned Insert(const key_type &key, const data_type &data, bool assertOnDuplicate, int (*cf)(const key_type&, const data_type&)=default_comparison_function)
		{
			unsigned int index = LockList()->Insert(key, data, assertOnDuplicate, cf);
			ReleaseList();
			return index;
		}
		unsigned Remove(const key_type &key, int (*cf)(const key_type&, const data_type&)=default_comparison_function)
		{
			unsigned int index = LockList()->Remove(key, cf);
			ReleaseList();
			return index;
		}
		unsigned RemoveIfExists(const key_type &key, int (*cf)(const key_type&, const data_type&)=default_comparison_function)
		{
			unsigned int index = LockList()->RemoveIfExists(key, cf);
			ReleaseList();
			return index;
		}
		data_type& operator[] ( const unsigned int position )
		{
			data_type &data = (*LockList())[position];
			ReleaseList();
			return data;
		}
		void RemoveAtIndex(const unsigned index)
		{
			LockList()->RemoveAtIndex(index);
			ReleaseList();
		}
		void InsertAtIndex(const data_type &data, const unsigned index)
		{
			LockList()->InsertAtIndex(data, index);
			ReleaseList();
		}
		void InsertAtEnd(const data_type &data)
		{
			LockList()->InsertAtEnd();
			ReleaseList();
		}
		void RemoveFromEnd(const unsigned num=1)
		{
			LockList()->RemoveFromEnd(num);
			ReleaseList();
		}
		void Clear(bool doNotDeallocate=false)
		{
			LockList()->Clear(doNotDeallocate);
			ReleaseList();
		}
		unsigned int Size(void)
		{
			unsigned int size = LockList()->Size();
			ReleaseList();
			return size;
		}
	};
}

#endif
