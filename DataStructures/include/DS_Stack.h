#ifndef DS_STACK_H
#define DS_STACK_H

#include "assert.h"

namespace DataStructures
{

	template<class T>
	class Stack
	{
	public:
		Stack();
		void Push(T);  //push an item onto the stack
		T Pop();       //pop an item from stack
		T Peek();

		bool IsEmpty(); 
		void Clear();
		~Stack();
	private:
		struct StackNode 
		{
			T _current;
			StackNode *_below;
			StackNode(T C, StackNode *B = 0) : _current(C), _below(B) {}
		};
		int _size;
		StackNode *_top;
	};

	//constructor
	template<class T>
	Stack<T>::Stack(void)
	{
		_size = 0;
		_top = 0;
	}

	//adds an item to the stack
	template<class T>
	void Stack<T>::Push(T s)
	{
		StackNode *P = new StackNode(s,_top);
		_size++;
		_top = P;
	}

	//returns top of the stack
	template<class T>
	T Stack<T>::Pop(void)
	{
		assert(!IsEmpty());
		T t;
		StackNode temp(t);
		temp = *_top;
		delete _top;
		_size--;
		if (_size)
			_top = temp._below;
		else
			_top = 0;
		return temp._current;
	}

	//returns top of the stack
	template<class T>
	T Stack<T>::Peek(void)
	{
		assert(!IsEmpty());
		return _top->_current;
	}

	//returns true if stack is empty
	//returns false if stack is not empty
	template<class T>
	bool Stack<T>::IsEmpty(void)
	{
		return (!_size);
	}

	//Removes all items from stack
	template <class T>
	void Stack<T>::Clear()
	{
		while (!IsEmpty())
		{
			Pop();
		}
		delete _top;
		_top = 0;
	}

	//Destructor
	template<class T>
	Stack<T>::~Stack(void)
	{
		while (!IsEmpty())
		{
			Pop();
		}
	}

}
#endif