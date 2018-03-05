#ifndef _SINGLETON_H__
#define _SINGLETON_H__

#include <assert.h>

namespace DataStructures
{
    template <typename T> 
	class Singleton
    {
	private:
		Singleton(const Singleton<T> &);

		Singleton &operator = (const Singleton<T> &);
	protected:
        static T *mSingleton;
    public:
        Singleton( void )
        {
            assert(!mSingleton);
#if defined( _MSC_VER ) && _MSC_VER < 1200	 
            int offset = (int)(T*)1 - (int)(Singleton <T>*)(T*)1;
            mSingleton = (T*)((int)this + offset);
#else
	    mSingleton = static_cast< T* >(this);
#endif
        }

        virtual ~Singleton( void )
        {
            assert(mSingleton);
            mSingleton = 0;
        }
        static T &GetSingleton()
		{
            assert(mSingleton);
            return (*mSingleton);
        }
        static T *GetSingletonPtr()
		{
            return mSingleton;
        }
    };
}

#endif
