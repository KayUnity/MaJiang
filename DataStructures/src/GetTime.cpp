#include "GetTime.h"
#ifdef _XBOX360
#include "Console1Includes.h" // Developers of a certain platform will know what to do here.
#elif defined(_WIN32)
#include <windows.h>
DWORD mProcMask;
DWORD mSysMask;
HANDLE mThread;
static LARGE_INTEGER yo;
#elif defined(_PS3)
#include "Console2Includes.h"
#include <sys/sys_time.h> // GetTime.cpp
#include <stdint.h> // GetTime.cpp
#include <sys/time_util.h> // GetTime.cpp
uint64_t ticksPerSecond;
uint64_t initialTime;
#else
#include <sys/time.h>
#include <unistd.h>
static timeval tp;
HexTimeNS initialTime;
#endif

static bool initialized=false;
int queryCount=0;

HexTime DataStructures::GetTime( void )
{
	return (HexTime)(GetTimeNS()/1000);
}

HexTimeNS DataStructures::GetTimeNS( void )
{
#if defined(_PS3)
	uint64_t curTime;
	if ( initialized == false)
	{
		ticksPerSecond = _PS3_GetTicksPerSecond();
		// Use the function to get elapsed ticks, this is a macro.
		_PS3_GetElapsedTicks(curTime);
		uint64_t quotient, remainder;
		quotient=(curTime / ticksPerSecond);
		remainder=(curTime % ticksPerSecond);
		initialTime = (HexTimeNS) quotient*(HexTimeNS)1000000 + (remainder*(HexTimeNS)1000000 / ticksPerSecond);
		initialized = true;
	}	
#elif defined(_WIN32)
	// Win32
	if ( initialized == false)
	{
		initialized = true;

#if !defined(_WIN32_WCE)
		// Save the current process
		HANDLE mProc = GetCurrentProcess();

		// Get the current Affinity
#if _MSC_VER >= 1400 && defined (_M_X64)
		GetProcessAffinityMask(mProc, (PDWORD_PTR)&mProcMask, (PDWORD_PTR)&mSysMask);
#else
		GetProcessAffinityMask(mProc, &mProcMask, &mSysMask);
#endif

		mThread = GetCurrentThread();

#endif // !defined(_WIN32_WCE)

		QueryPerformanceFrequency( &yo );
	}
	// 01/12/08 - According to the docs "The frequency cannot change while the system is running." so this shouldn't be necessary
	/*
	if (++queryCount==200)
	{
		// Set affinity to the first core
		SetThreadAffinityMask(mThread, 1);

		QueryPerformanceFrequency( &yo );

		// Reset affinity
		SetThreadAffinityMask(mThread, mProcMask);

		queryCount=0;
	}
	*/

#elif (defined(__GNUC__)  || defined(__GCCXML__))
	if ( initialized == false)
	{
		gettimeofday( &tp, 0 );
		initialized=true;
		// I do this because otherwise HexTime in milliseconds won't work as it will underflow when dividing by 1000 to do the conversion
		initialTime = ( tp.tv_sec ) * (HexTimeNS) 1000000 + ( tp.tv_usec );
	}	
#endif

#if defined(_PS3)
	// Use the function to get elapsed ticks, this is a macro.
	_PS3_GetElapsedTicks(curTime);
	uint64_t quotient, remainder;
	quotient=(curTime / ticksPerSecond);
	remainder=(curTime % ticksPerSecond);
	curTime = (HexTimeNS) quotient*(HexTimeNS)1000000 + (remainder*(HexTimeNS)1000000 / ticksPerSecond);
	// Subtract from initialTime so the millisecond conversion does not underflow
	return curTime - initialTime;
#elif defined(_WIN32)

	HexTimeNS curTime;
	static HexTimeNS lastQueryVal=(HexTimeNS)0;
	static unsigned long lastTickCountVal = GetTickCount();

	LARGE_INTEGER PerfVal;

#if !defined(_WIN32_WCE)
	// Set affinity to the first core
	SetThreadAffinityMask(mThread, 1);
#endif // !defined(_WIN32_WCE)

	// Docs: On a multiprocessor computer, it should not matter which processor is called.
	// However, you can get different results on different processors due to bugs in the basic input/output system (BIOS) or the hardware abstraction layer (HAL). To specify processor affinity for a thread, use the SetThreadAffinityMask function. 
	// Query the timer
	QueryPerformanceCounter( &PerfVal );

#if !defined(_WIN32_WCE)
	// Reset affinity
	SetThreadAffinityMask(mThread, mProcMask);
#endif // !defined(_WIN32_WCE)

	__int64 quotient, remainder;
	quotient=((PerfVal.QuadPart) / yo.QuadPart);
	remainder=((PerfVal.QuadPart) % yo.QuadPart);
	curTime = (HexTimeNS) quotient*(HexTimeNS)1000000 + (remainder*(HexTimeNS)1000000 / yo.QuadPart);

#if !defined(_WIN32_WCE)
	if (lastQueryVal==0)
	{
		// First call
		lastQueryVal=curTime;
		return curTime;
	}

	// To workaround http://support.microsoft.com/kb/274323 where the timer can sometimes jump forward by hours or days
	unsigned long curTickCount = GetTickCount();
	unsigned elapsedTickCount = curTickCount - lastTickCountVal;
	HexTimeNS elapsedQueryVal = curTime - lastQueryVal;
	if (elapsedQueryVal/1000 > elapsedTickCount+100)
	{
		curTime=lastQueryVal+elapsedTickCount*1000;
	}

	lastTickCountVal=curTickCount;
	lastQueryVal=curTime;
#endif
	return curTime;

#elif (defined(__GNUC__)  || defined(__GCCXML__))
	// GCC
	HexTimeNS curTime;
	gettimeofday( &tp, 0 );

	curTime = ( tp.tv_sec ) * (HexTimeNS) 1000000 + ( tp.tv_usec );
	// Subtract from initialTime so the millisecond conversion does not underflow
	return curTime - initialTime;
#endif
}

HexTime DataStructures::GetDuration( HexTime lastTime )
{
	HexTime currentTime = GetTime();
	if (currentTime > lastTime)
		return currentTime - lastTime;
	else
		return ~0 - currentTime + lastTime;
}

HexTimeNS DataStructures::GetDurationNS( HexTimeNS lastTime )
{
	HexTimeNS currentTime = GetTimeNS();
	if (currentTime > lastTime)
		return currentTime - lastTime;
	else
		return ~0 - currentTime + lastTime;
}

HexTime DataStructures::GetDuration( HexTime lastTime, HexTime currentTime )
{
	if (currentTime > lastTime)
		return currentTime - lastTime;
	else
		return ~0 - currentTime + lastTime;
}
