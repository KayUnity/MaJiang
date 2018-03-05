#ifndef __HEX_COMMON_THREAD__
#define __HEX_COMMON_THREAD__

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#else
#include <pthread.h>
//#include "gnuclib.h"
#include <string.h>
#include <pthread.h>
#endif

namespace DataStructures 
{
	class HexCommonThread 
	{
	private:
#ifdef _WIN32
		CRITICAL_SECTION m_CS_TerminationMask;
		unsigned long* m_hThread;
#else
		pthread_mutex_t m_CS_TerminationMask;
		typedef struct 
		{
			pthread_mutex_t	    mutex;
			pthread_cond_t		condition;
			pthread_t           p_hThread;
			int			        semCount;	
		}sem_private_struct, *sem_private;
		sem_private    token;
#endif
		bool m_ternimationMask;

	protected:
		inline bool ShouldTerminated();
		inline void TryTerminateThread();
	public:
		inline HexCommonThread();
		inline virtual ~HexCommonThread();

		inline bool ThreadStarted();

#ifdef _WIN32 
		inline unsigned long* Handle() { return m_hThread; }
#else
		inline sem_private Handle() { return token; }   
#endif		

		void Start();

		virtual void Run() {};
		void Suspend();
		void Resume();
		void Kill();
		void Stop();
		
		static void ThreadSleep(long ms);
		
		void SetPriority(int p);

		static const int P_ABOVE_NORMAL;
		static const int P_BELOW_NORMAL;
		static const int P_HIGHEST;
		static const int P_IDLE;
		static const int P_LOWEST;
		static const int P_NORMAL;
		static const int P_CRITICAL;
	};

	//---------------------------------------------------- inline functions --------------------------------------------------
	HexCommonThread::HexCommonThread() : m_ternimationMask(false) 
	{
#ifdef _WIN32
		m_hThread = NULL;
		InitializeCriticalSection( &m_CS_TerminationMask );
#else
		token=(sem_private)NULL;
		pthread_mutex_init(&m_CS_TerminationMask, NULL); 
#endif
	}

	HexCommonThread::~HexCommonThread() 
	{ 
		Stop();
#ifdef _WIN32
	DeleteCriticalSection( &m_CS_TerminationMask );
#else
	pthread_mutex_destroy(&m_CS_TerminationMask);
#endif
	}

	bool HexCommonThread::ThreadStarted()
	{
		bool shouldBe = ShouldTerminated();
#ifdef _WIN32
		return !shouldBe && (m_hThread != NULL); 
#else
		return !shouldBe && (token!=(sem_private)NULL);
#endif 
	}

	bool HexCommonThread::ShouldTerminated()
	{
#ifdef _WIN32
		EnterCriticalSection(&m_CS_TerminationMask);
#else 
		pthread_mutex_lock(&m_CS_TerminationMask);
#endif
		bool shouldBe = m_ternimationMask;
#ifdef _WIN32
		LeaveCriticalSection(&m_CS_TerminationMask);
#else 
		pthread_mutex_unlock(&m_CS_TerminationMask);
#endif
		return shouldBe;
	}

	void HexCommonThread::TryTerminateThread()
	{
#ifdef _WIN32
		EnterCriticalSection(&m_CS_TerminationMask);
#else 
		pthread_mutex_lock(&m_CS_TerminationMask);
#endif
		m_ternimationMask = true;
#ifdef _WIN32
		LeaveCriticalSection(&m_CS_TerminationMask);
#else 
		pthread_mutex_unlock(&m_CS_TerminationMask);
#endif
	}

}

extern "C" { unsigned int HexCommonThreadProc(void* param); }				

#endif
