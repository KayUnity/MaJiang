#ifndef __HEX_TIMECOUNTER_H
#define __HEX_TIMECOUNTER_H

#include "GetTime.h"
#include "HexString.h"

class HexTimeCounter
{
public:
	HexTimeCounter();
	~HexTimeCounter();

	HexTime StartTimer();
	HexTime StopTimer();

	HexTime GetTimeSlapped();
	DataStructures::HexString GetTimeSlappedString();
	inline bool IsTimerStopped() { return timerStopped; }
	bool StopTimerAtThePoint(HexTime ms);
	bool AcquireTimerState(HexTime ms);
	void ForceDecreaseLastTime(HexTime newTime);
	void ForceIncreaseLastTime(HexTime newTime);
private:
	HexTime currentTime, lastTime;
	bool timerStopped;
};

class HexScaledTimeCounter
{
public:
	HexScaledTimeCounter();
	~HexScaledTimeCounter();

	HexTime StartTimer();
	HexTime StopTimer();

	HexTime GetTimeSlapped();
	DataStructures::HexString GetTimeSlappedString();
	inline bool IsTimerStopped() { return timerStopped; }
	bool StopTimerAtThePoint(HexTime ms);
	bool AcquireTimerState(HexTime ms);
	void ForceDecreaseLastTime(HexTime newTime);
	void ForceIncreaseLastTime(HexTime newTime);
	void SetTimeScale(float scale);
	float GetTimeScale() { return mTimeScale; }
private:
	HexTime currentTime, lastTime;
	bool timerStopped;
	float mTimeScale;
};

//------------------------------ time utility functions -----------------------------------
static __time_t UNINITIALIZED_TIME_T = 0;
static struct tm UNINITIALIZED_TIME_TM = *__localtime(&UNINITIALIZED_TIME_T);

inline bool IsUninitializedTime(struct tm &time)
{
	return __mktime(&time) == UNINITIALIZED_TIME_T;
}

inline bool TimeEquals(struct tm &t1, struct tm &t2)
{
	return memcmp(&t1, &t2, sizeof(struct tm)) == 0;
}

inline struct tm *GetUninitializedTime()
{
	return &UNINITIALIZED_TIME_TM;
}

inline struct tm *GetCurrentTimeInTM()
{
	__time_t long_time;
	__time( &long_time );
	return __localtime( &long_time ); // C4996
}

inline struct tm *GetExpireTimeInTM(unsigned int afterSecond)
{
	__time_t nowtime;
	__time(&nowtime);
	nowtime += afterSecond;
	return __localtime(&nowtime);
}

inline __time_t CompareTimeTM(struct tm *baseTM,struct tm *compareTM)
{
	__time_t baseTMsecond = __mktime(baseTM);
	__time_t compareTMsecond = __mktime(compareTM);
	return compareTMsecond - baseTMsecond;
}

inline bool IsTimeExpired(struct tm *baseTime, int expireSeconds)
{
	__time_t long_time;
	__time( &long_time );
	__time_t baseTime32 = __mktime(baseTime);
	return long_time - baseTime32 >= expireSeconds;
}

inline __time_t GetDurationInSecond(__time_t from)
{
	__time_t nowtime = __time(&nowtime);
	nowtime -= from;
	return nowtime;
}

inline __time_t GetDurationInSecond(__time_t from, __time_t to)
{
	return to - from;
}

inline __time_t GetDurationInSecond(struct tm *from)
{
	__time_t baseTMsecond = __mktime(from);
	__time_t nowtime;
	__time(&nowtime);
	nowtime -= baseTMsecond;
	return nowtime;
}

inline __time_t GetDurationInSecond(struct tm *from, struct tm *to)
{
	__time_t baseTMsecond = __mktime(from);
	__time_t toTime = __mktime(to);
	toTime -= baseTMsecond;
	return toTime;
}

inline __time_t64 GetDurationInMS(struct tm *from)
{
	return GetDurationInSecond(from) * 1000;
}

inline __time_t64 GetDurationInMS(struct tm *from, struct tm *to)
{
	return GetDurationInSecond(from, to) * 1000;
}


#endif
