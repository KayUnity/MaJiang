#ifndef __THREAD_SAFE_LIST_H
#define __THREAD_SAFE_LIST_H 

#include "DS_List.h"
#include "SimpleMutex.h"

namespace DataStructures
{
	/// \brief Array based implementation of a list.
	/// \note ONLY USE THIS FOR SHALLOW COPIES.  I don't bother with operator= to improve performance.
	template <class list_type>
	class ThreadSafeList
	{	
	private:
		List<list_type> m_list;
		SimpleMutex mutex;
	public:
		/// Copy constructor
		ThreadSafeList() {}
		/// \param[in]  original_copy The list to duplicate 
		ThreadSafeList( const List<list_type>& original_copy );
		ThreadSafeList( const ThreadSafeList<list_type>& original_copy );

		List<list_type> *LockList();
		void ReleaseList();

		/// Assign one list to another
		ThreadSafeList& operator = ( const List<list_type>& original_copy );
		ThreadSafeList& operator = ( const ThreadSafeList<list_type>& original_copy );

		/// Access an element by its index in the array 
		/// \param[in]  position The index into the array. 
		/// \return The element at position \a position. 
		list_type& operator[] ( const unsigned int position ) const;

		/// Push an element at the end of the stack
		/// \param[in] input The new element. 
		void Push(const list_type &input);

		/// Pop an element from the end of the stack
		/// \pre Size()>0
		/// \return The element at the end. 
		list_type& Pop(void);

		/// Insert an element at position \a position in the list 
		/// \param[in] input The new element. 
		/// \param[in] position The position of the new element. 		
		void Insert( const list_type &input, const unsigned int position );

		/// Insert at the end of the list.
		/// \param[in] input The new element. 
		void Insert( const list_type &input );

		/// Replace the value at \a position by \a input.  If the size of
		/// the list is less than @em position, it increase the capacity of
		/// the list and fill slot with @em filler.
		/// \param[in] input The element to replace at position @em position. 
		/// \param[in] filler The element use to fill new allocated capacity. 
		/// \param[in] position The position of input in the list. 		
		void Replace( const list_type &input, const list_type &filler, const unsigned int position );

		/// Replace the last element of the list by \a input .
		/// \param[in] input The element used to replace the last element. 
		void Replace( const list_type &input );

		/// Delete the element at position \a position. 
		/// \param[in] position The index of the element to delete 
		void RemoveAtIndex( const unsigned int position );

		/// Delete the element at position \a position. 
		/// \note - swaps middle with end of list, only use if list order does not matter
		/// \param[in] position The index of the element to delete 
		void RemoveAtIndexFast( const unsigned int position );

		/// Delete the element at the end of the list 
		void RemoveFromEnd(const unsigned num=1);

		/// Returns the index of the specified item or MAX_UNSIGNED_LONG if not found
		/// \param[in] input The element to check for 
		/// \return The index or position of @em input in the list. 
		/// \retval MAX_UNSIGNED_LONG The object is not in the list
		/// \retval [Integer] The index of the element in the list
		unsigned int GetIndexOf( const list_type &input );

		/// \return The number of elements in the list
		unsigned int Size( void ) const;

		/// Clear the list		
		void Clear( bool doNotDeallocateSmallBlocks=false );

		// Preallocate the list, so it needs fewer reallocations at runtime
		void Preallocate( unsigned countNeeded );

		/// Frees overallocated members, to use the minimum memory necessary
		/// \attention 
		/// This is a slow operation		
		void Compress( void );
	};

	template <class list_type>
	List<list_type> *ThreadSafeList<list_type>::LockList()
	{
		mutex.Lock();
		return &m_list;
	}

	template <class list_type>
	void ThreadSafeList<list_type>::ReleaseList()
	{
		mutex.Unlock();
	}

	template <class list_type>
	ThreadSafeList<list_type>::ThreadSafeList( const List<list_type>& original_copy )
	{
		List<list_type> *list = LockList();
		*list = original_copy;
		ReleaseList();
	}

	template <class list_type>
	ThreadSafeList<list_type>::ThreadSafeList( const ThreadSafeList<list_type>& original_copy )
	{
		List<list_type> *dest = LockList();
		List<list_type> *source = original_copy.LockList();
		*dest = *source;
		original_copy.ReleaseList();
		ReleaseList();
	}

	template <class list_type>
	ThreadSafeList<list_type>& ThreadSafeList<list_type>::operator= ( const List<list_type>& original_copy )
	{
		List<list_type> *list = LockList();
		*list = original_copy;
		ReleaseList();
		return *this;
	}

	template <class list_type>
	ThreadSafeList<list_type>& ThreadSafeList<list_type>::operator= ( const ThreadSafeList<list_type>& original_copy )
	{
		List<list_type> *dest = LockList();
		List<list_type> *source = original_copy.LockList();
		*dest = *source;
		original_copy.ReleaseList();
		ReleaseList();
		return *this;
	}

	template <class list_type>
	inline list_type& ThreadSafeList<list_type>::operator[] ( const unsigned int position ) const
	{
		List<list_type> *list = ((ThreadSafeList<list_type> *)(this))->LockList();
		list_type &result = (*list)[position];
		((ThreadSafeList<list_type> *)(this))->ReleaseList();
		return result;
	}

	template <class list_type>
	void ThreadSafeList<list_type>::Push(const list_type &input)
	{
		List<list_type> *list = LockList();
		list->Push(input);
		ReleaseList();
	}

	template <class list_type>
	inline list_type& ThreadSafeList<list_type>::Pop(void)
	{
		List<list_type> *list = LockList();
		list_type &result = list->Pop();
		ReleaseList();
		return result;
	}

	template <class list_type>
	void ThreadSafeList<list_type>::Insert( const list_type &input, const unsigned int position )
	{
		List<list_type> *list = LockList();
		list->Insert(input, position);
		ReleaseList();
	}


	template <class list_type>
	void ThreadSafeList<list_type>::Insert( const list_type &input )
	{
		List<list_type> *list = LockList();
		list->Insert(input);
		ReleaseList();
	}

	template <class list_type>
	inline void ThreadSafeList<list_type>::Replace( const list_type &input, const list_type &filler, const unsigned int position )
	{
		List<list_type> *list = LockList();
		list->Replace(input, filler, position);
		ReleaseList();
	}

	template <class list_type>
	inline void ThreadSafeList<list_type>::Replace( const list_type &input )
	{
		List<list_type> *list = LockList();
		list->Replace(input);
		ReleaseList();
	}

	template <class list_type>
	void ThreadSafeList<list_type>::RemoveAtIndex( const unsigned int position )
	{
		List<list_type> *list = LockList();
		list->RemoveAtIndex(position);
		ReleaseList();
	}

	template <class list_type>
	void ThreadSafeList<list_type>::RemoveAtIndexFast( const unsigned int position )
	{
		List<list_type> *list = LockList();
		list->RemoveAtIndexFast(position);
		ReleaseList();
	}

	template <class list_type>
	inline void ThreadSafeList<list_type>::RemoveFromEnd( const unsigned num )
	{
		List<list_type> *list = LockList();
		list->RemoveFromEnd(num);
		ReleaseList();
	}

	template <class list_type>
	inline unsigned int ThreadSafeList<list_type>::Size( void ) const
	{
		List<list_type> *list = ((ThreadSafeList<list_type> *)(this))->LockList();
		unsigned int size = list->Size();
		((ThreadSafeList<list_type> *)(this))->ReleaseList();
		return size;
	}

	template <class list_type>
	void ThreadSafeList<list_type>::Clear( bool doNotDeallocateSmallBlocks )
	{
		List<list_type> *list = LockList();
		list->Clear();
		ReleaseList();
	}

	template <class list_type>
	void ThreadSafeList<list_type>::Compress( void )
	{
		List<list_type> *list = LockList();
		list->Compress();
		ReleaseList();
	}

	template <class list_type>
	void ThreadSafeList<list_type>::Preallocate( unsigned countNeeded )
	{
		List<list_type> *list = LockList();
		list->Preallocate(countNeeded);
		ReleaseList();
	}

	template <class list_type>
	unsigned int ThreadSafeList<list_type>::GetIndexOf( const list_type &input )
	{
		List<list_type> *list = LockList();
		unsigned int result = list->GetIndexOf(input);
		ReleaseList();
		return result;
	}

} // End namespace

#endif
