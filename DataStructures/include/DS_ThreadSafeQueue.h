#ifndef __THREAD_SAFE_QUEUE_H
#define __THREAD_SAFE_QUEUE_H 

#include "DS_Queue.h"
#include "SimpleMutex.h"

namespace DataStructures
{

	/// \brief A queue implemented as an array with a read and write index.
	template <class queue_type>
	class ThreadSafeQueue
	{
	private:
		Queue<queue_type> m_queue;
		SimpleMutex mutex;
	public:
		ThreadSafeQueue() {}
		ThreadSafeQueue( unsigned int allocation_size );
		ThreadSafeQueue( ThreadSafeQueue<queue_type>& original_copy );
		ThreadSafeQueue( Queue<queue_type>& original_copy );

		Queue<queue_type> *LockQueue();
		void ReleaseQueue();

		bool operator= ( const Queue<queue_type> &original_copy );
		bool operator= ( const ThreadSafeQueue<queue_type> &original_copy );
		void Push( const queue_type &input );
		void PushAtHead( const queue_type &input, unsigned index=0 );
		queue_type& operator[] ( unsigned int position ) const; // Not a normal thing you do with a queue but can be used for efficiency, NEVER this when queue_type is a reference one 
		void RemoveAtIndex( unsigned int position ); // Not a normal thing you do with a queue but can be used for efficiency
		inline queue_type Peek( void ) const;
		inline queue_type PeekTail( void ) const;
		inline queue_type Pop( void );
		inline unsigned int Size( void ) const;
		inline bool IsEmpty(void) const;
		inline unsigned int AllocationSize( void ) const;
		inline void Clear( void );
		void Compress( void );
		bool Find ( queue_type q );
		unsigned int GetItemIndex ( queue_type q );
		void ClearAndForceAllocation( int size ); // Force a memory allocation to a certain larger size

		unsigned int GetHead(){ unsigned int h = LockQueue()->GetHead(); ReleaseQueue(); return h; };
		unsigned int GetTail(){ unsigned int t = LockQueue()->GetTail(); ReleaseQueue(); return t;};
	};

	template <class queue_type>
	Queue<queue_type> *ThreadSafeQueue<queue_type>::LockQueue()
	{
		mutex.Lock();
		return &m_queue;
	}

	template <class queue_type>
	void ThreadSafeQueue<queue_type>::ReleaseQueue()
	{
		mutex.Unlock();
	}

	template <class queue_type>
	ThreadSafeQueue<queue_type>::ThreadSafeQueue( unsigned int allocation_size )
	{
		LockQueue()->ClearAndForceAllocation(allocation_size);
		ReleaseQueue();
	}

	template <class queue_type>
	ThreadSafeQueue<queue_type>::ThreadSafeQueue( Queue<queue_type>& original_copy )
	{
		Queue<queue_type> *queue = LockQueue();
		*queue = original_copy;
		ReleaseQueue();
	}

	template <class queue_type>
	ThreadSafeQueue<queue_type>::ThreadSafeQueue( ThreadSafeQueue<queue_type>& original_copy )
	{
		Queue<queue_type> *dest = LockQueue();
		Queue<queue_type> *source = original_copy.LockQueue();
		*dest = *source;
		original_copy.ReleaseQueue();
		ReleaseQueue();
	}

	template <class queue_type>
	bool ThreadSafeQueue<queue_type>::operator= ( const Queue<queue_type>& original_copy )
	{
		Queue<queue_type> *queue = LockQueue();
		bool res = (*queue = original_copy);
		ReleaseQueue();
		return res;
	}

	template <class queue_type>
	bool ThreadSafeQueue<queue_type>::operator= ( const ThreadSafeQueue<queue_type>& original_copy )
	{
		Queue<queue_type> *dest = LockQueue();
		Queue<queue_type> *source = original_copy.LockQueue();
		bool res = (*dest = *source);
		original_copy.ReleaseQueue();
		ReleaseQueue();
		return res;
	}

	template <class queue_type>
	inline unsigned int ThreadSafeQueue<queue_type>::Size( void ) const
	{
		Queue<queue_type> *queue = ((ThreadSafeQueue<queue_type> *)(this))->LockQueue();
		unsigned int size = queue->Size();
		((ThreadSafeQueue<queue_type> *)(this))->ReleaseQueue();
		return size;
	}

	template <class queue_type>
	inline bool ThreadSafeQueue<queue_type>::IsEmpty(void) const
	{
		Queue<queue_type> *queue = ((ThreadSafeQueue<queue_type> *)(this))->LockQueue();
		bool isEmpty = queue->IsEmpty();
		((ThreadSafeQueue<queue_type> *)(this))->ReleaseQueue();
		return isEmpty;
	}

	template <class queue_type>
	inline unsigned int ThreadSafeQueue<queue_type>::AllocationSize( void ) const
	{
		Queue<queue_type> *queue = ((ThreadSafeQueue<queue_type> *)(this))->LockQueue();
		unsigned int size = queue->AllocationSize();
		((ThreadSafeQueue<queue_type> *)(this))->ReleaseQueue();
		return size;
	}

	template <class queue_type>
	inline queue_type ThreadSafeQueue<queue_type>::Pop( void )
	{
		Queue<queue_type> *queue = LockQueue();
		queue_type result = queue->Pop();
		ReleaseQueue();
		return result;
	}

	template <class queue_type>
	void ThreadSafeQueue<queue_type>::PushAtHead( const queue_type &input, unsigned index )
	{
		Queue<queue_type> *queue = LockQueue();
		queue->PushAtHead(input, index);
		ReleaseQueue();
	}


	template <class queue_type>
	inline queue_type ThreadSafeQueue<queue_type>::Peek( void ) const
	{
		Queue<queue_type> *queue = ((ThreadSafeQueue<queue_type> *)(this))->LockQueue();
		queue_type result = queue->Peek();
		((ThreadSafeQueue<queue_type> *)(this))->ReleaseQueue();
		return result;
	}

	template <class queue_type>
	inline queue_type ThreadSafeQueue<queue_type>::PeekTail( void ) const
	{
		Queue<queue_type> *queue = ((ThreadSafeQueue<queue_type> *)(this))->LockQueue();
		queue_type result = queue->PeekTail();
		((ThreadSafeQueue<queue_type> *)(this))->ReleaseQueue();
		return result;
	}

	template <class queue_type>
	void ThreadSafeQueue<queue_type>::Push( const queue_type &input )
	{
		Queue<queue_type> *queue = LockQueue();
		queue->Push(input);
		ReleaseQueue();
	}

	template <class queue_type>
	inline void ThreadSafeQueue<queue_type>::Clear ( void )
	{
		Queue<queue_type> *queue = LockQueue();
		queue->Clear();
		ReleaseQueue();
	}

	template <class queue_type>
	void ThreadSafeQueue<queue_type>::Compress ( void )
	{
		Queue<queue_type> *queue = LockQueue();
		queue->Compress();
		ReleaseQueue();
	}

	template <class queue_type>
	bool ThreadSafeQueue<queue_type>::Find ( queue_type q )
	{
		Queue<queue_type> *queue = LockQueue();
		bool result = queue->Find(q);
		ReleaseQueue();
		return result;
	}

	template <class queue_type>
	unsigned int ThreadSafeQueue<queue_type>::GetItemIndex ( queue_type q )
	{
		Queue<queue_type> *queue = LockQueue();
		unsigned int result = queue->GetItemIndex(q);
		ReleaseQueue();
		return result;
	}

	template <class queue_type>
	void ThreadSafeQueue<queue_type>::ClearAndForceAllocation( int size )
	{
		Queue<queue_type> *queue = LockQueue();
		queue->ClearAndForceAllocation(size);
		ReleaseQueue();
	}

	template <class queue_type>
	inline queue_type& ThreadSafeQueue<queue_type>::operator[] ( unsigned int position ) const
	{
		Queue<queue_type> *queue = ((ThreadSafeQueue<queue_type> *)(this))->LockQueue();
		queue_type &result = (*queue)[position];
		((ThreadSafeQueue<queue_type> *)(this))->ReleaseQueue();
		return result;
	}

	template <class queue_type>
	void ThreadSafeQueue<queue_type>::RemoveAtIndex( unsigned int position )
	{
		Queue<queue_type> *queue = LockQueue();
		queue->RemoveAtIndex(position);
		ReleaseQueue();
	}

} // End namespace

#endif

