#ifndef __SIMPLE_MUTEX_H
#define __SIMPLE_MUTEX_H

#ifdef _XBOX360
#include "Console1Includes.h"
#elif defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#include <sys/types.h>
#endif

#include <assert.h>

/// \brief An easy to use mutex.
/// 
/// I wrote this because the version that comes with Windows is too complicated and requires too much code to use.
/// @remark Previously I used this everywhere, and in fact for a year or two HexNet was totally threadsafe.  While doing profiling, I saw that this function was incredibly slow compared to the blazing performance of everything else, so switched to single producer / consumer everywhere.  Now the user thread of HexNet is not threadsafe, but it's 100X faster than before.
class SimpleMutex
{
public:

	/// Constructor
	inline SimpleMutex();
	
	// Destructor
	virtual inline ~SimpleMutex();
	
	// Locks the mutex.  Slow!
	inline void Lock(void);
	
	// Unlocks the mutex.
	inline void Unlock(void);
private:
	#ifdef _WIN32
	CRITICAL_SECTION criticalSection; /// Docs say this is faster than a mutex for single process access
	#else
	pthread_mutex_t hMutex;
	#endif
};

// ---------------------------- inline functions -------------------------------------
SimpleMutex::SimpleMutex()
{
#ifdef _WIN32
	//	hMutex = CreateMutex(NULL, FALSE, 0);
	//	assert(hMutex);
	InitializeCriticalSection(&criticalSection);
#else
	int error = pthread_mutex_init(&hMutex, 0);
	(void) error;
	assert(error==0);
#endif
}

SimpleMutex::~SimpleMutex()
{
#ifdef _WIN32
	//	CloseHandle(hMutex);
	DeleteCriticalSection(&criticalSection);
#else
	pthread_mutex_destroy(&hMutex);
#endif
}

#ifdef _WIN32
#ifdef _DEBUG
#include <stdio.h>
#endif
#endif

void SimpleMutex::Lock(void)
{
#ifdef _WIN32
	if (&criticalSection)
		EnterCriticalSection(&criticalSection);
#else
	int error = pthread_mutex_lock(&hMutex);
	(void) error;
	assert(error==0);
#endif
}

void SimpleMutex::Unlock(void)
{
#ifdef _WIN32
	//	ReleaseMutex(hMutex);
	LeaveCriticalSection(&criticalSection);
#else
	int error = pthread_mutex_unlock(&hMutex);
	(void) error;
	assert(error==0);
#endif
}



#endif

