#include "HexCommonThread.h"
#ifdef _WIN32
#else
#include <unistd.h>
//#include <malloc/malloc.h>
#include <stdlib.h>
#endif

using namespace DataStructures;

#if defined _WIN32 || defined _WIN64
const int HexCommonThread::P_ABOVE_NORMAL = THREAD_PRIORITY_ABOVE_NORMAL;
const int HexCommonThread::P_BELOW_NORMAL = THREAD_PRIORITY_BELOW_NORMAL;
const int HexCommonThread::P_HIGHEST = THREAD_PRIORITY_HIGHEST;
const int HexCommonThread::P_IDLE = THREAD_PRIORITY_IDLE;
const int HexCommonThread::P_LOWEST = THREAD_PRIORITY_LOWEST;
const int HexCommonThread::P_NORMAL = THREAD_PRIORITY_NORMAL;
const int HexCommonThread::P_CRITICAL = THREAD_PRIORITY_TIME_CRITICAL;
#else
//roc todo
const int HexCommonThread::P_ABOVE_NORMAL = 10;
const int HexCommonThread::P_BELOW_NORMAL = 7;
const int HexCommonThread::P_HIGHEST = 11;
const int HexCommonThread::P_IDLE = 1;
const int HexCommonThread::P_LOWEST = 6;
const int HexCommonThread::P_NORMAL = 9;
const int HexCommonThread::P_CRITICAL = 11;
#endif

unsigned int HexCommonThreadProc( void* param )
{
	HexCommonThread* tp = (HexCommonThread*)param;
	tp->Run();
	return 0;
}

void HexCommonThread::ThreadSleep(long ms) 
{
#ifdef _WIN32
	Sleep(ms);
#else
	usleep((unsigned int)(ms*1000));
#endif
}

void HexCommonThread::Start() 
{
	if (ThreadStarted())
		return;

#ifdef _WIN32
	DWORD tid = 0;	
	m_hThread = (unsigned long*)CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)HexCommonThreadProc,(HexCommonThread*)this,0,&tid);
#else
	if (token==NULL)
		token = (sem_private) malloc(sizeof(sem_private_struct)+1);

	int rc0 = pthread_mutex_init(&(token->mutex), NULL);
	if(rc0)
	{
		free(token);
		//printf("Mutex Error  :%s\n",strerror(errno)); 
		// exit();
	}
	int rc1=pthread_cond_init(&(token->condition), NULL);
	if (rc1)
	{
		pthread_mutex_destroy( &(token->mutex) );
		free(token);
		//printf("Cond Error  :%s\n",strerror(errno)); 
	}

	token->semCount = 0;
	// Create thread linux
	pthread_attr_t attr;
	//struct sched_param param;
	pthread_attr_init( &attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	int rc2= pthread_create( &token->p_hThread, &attr, (void*(*)(void*))HexCommonThreadProc, (void *)this);
	if (rc2)
	{
		pthread_mutex_destroy( &(token->mutex) );
		pthread_cond_destroy(&(token->condition));
		free(token);
		//printf("Pthread Error  :%s\n",strerror(errno)); 
	}
#endif
#ifdef _WIN32
	EnterCriticalSection(&m_CS_TerminationMask);
#else 
	pthread_mutex_lock(&m_CS_TerminationMask);
#endif
	m_ternimationMask = false;
#ifdef _WIN32
	LeaveCriticalSection(&m_CS_TerminationMask);
#else 
	pthread_mutex_unlock(&m_CS_TerminationMask);
#endif
	SetPriority( HexCommonThread::P_NORMAL );
}

void HexCommonThread::Kill()
{
#ifdef _WIN32
	TerminateThread( m_hThread, 0 );
#else
	pthread_cancel(token->p_hThread);
#endif
}

void HexCommonThread::Stop() 
{
	TryTerminateThread();
#ifdef _WIN32
	if (m_hThread == NULL) 
		return;
	bool started = ThreadStarted();
	if (started)
		WaitForSingleObject( m_hThread, INFINITE );
	CloseHandle( m_hThread );
	m_hThread = NULL;
#else
	if (token==(sem_private)NULL)
		return;
	// pthread_join(token->p_hThread,NULL);
	pthread_mutex_destroy(&(token->mutex));

	pthread_cond_destroy(&(token->condition));
	free(token);
	token=NULL;
#endif

}
#ifdef _WIN32
void HexCommonThread::SetPriority( int tp ) 
{
	SetThreadPriority( m_hThread, tp );
}
#else
void HexCommonThread::SetPriority(int tp )
{
    //roc todo
	//pthread_setschedprio(pthread_self(),tp);
    struct sched_param sched;
    sched.sched_priority = tp;
    pthread_setschedparam(pthread_self(), SCHED_RR, &sched);
}
#endif

void HexCommonThread::Suspend() 
{
#ifdef _WIN32
	SuspendThread( m_hThread );
#else
	pthread_mutex_lock (&token->mutex);
	while(token->semCount==0)
		pthread_cond_wait( &token->condition, &token->mutex);
	token->semCount--;
	pthread_mutex_unlock (&token->mutex);

#endif
}

void HexCommonThread::Resume() 
{
#ifdef _WIN32
	ResumeThread( m_hThread );
#else
	pthread_mutex_lock (&token->mutex);
	if(token->semCount==0)
		pthread_cond_signal( &token->condition);
	token->semCount++;
	pthread_mutex_unlock (&token->mutex);
#endif
}
