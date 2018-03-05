#include "HexTimeCounter.h"
#include "HexString.h"
#include "formatString.h" 


HexTimeCounter::HexTimeCounter()
{
	currentTime = lastTime = DataStructures::GetTime();
	timerStopped = true;
}

HexTimeCounter::~HexTimeCounter()
{
}

HexTime HexTimeCounter::StartTimer()
{
	currentTime = lastTime = DataStructures::GetTime();
	timerStopped = false;
	return currentTime;
}

HexTime HexTimeCounter::StopTimer()
{
	timerStopped = true;
	currentTime = DataStructures::GetTime();
	return currentTime;
}

HexTime HexTimeCounter::GetTimeSlapped()
{
	if (timerStopped==false)
		currentTime = DataStructures::GetTime();
	return currentTime - lastTime;
}

DataStructures::HexString HexTimeCounter::GetTimeSlappedString()
{
	HexTime timeGap = GetTimeSlapped();
	DataStructures::HexString res = FormatString("%id %ih %im %is %ims", timeGap/86400000, (timeGap%86400000)/3600000, (timeGap%3600000)/60000, timeGap%60000/1000, timeGap%1000);
	return res;
}

bool HexTimeCounter::StopTimerAtThePoint(HexTime ms)
{
	if (timerStopped==true)
		return true;
	if (ms>GetTimeSlapped())
	{
		StopTimer();
		return true;
	}
	return false;
}

void HexTimeCounter::ForceDecreaseLastTime(HexTime newTime)
{
	lastTime -= newTime;
}

void HexTimeCounter::ForceIncreaseLastTime(HexTime newTime)
{
	lastTime += newTime;
}

bool HexTimeCounter::AcquireTimerState(HexTime ms)
{
	if (timerStopped==true)
		return true;
	if (ms>GetTimeSlapped())
		return true;
	return false;
}





HexScaledTimeCounter::HexScaledTimeCounter() 
	: timerStopped(true),
	mTimeScale(1.0f)
{
	currentTime = lastTime = DataStructures::GetTime();
}

HexScaledTimeCounter::~HexScaledTimeCounter()
{
}

HexTime HexScaledTimeCounter::StartTimer()
{
	currentTime = lastTime = DataStructures::GetTime();
	timerStopped = false;
	return currentTime;
}

HexTime HexScaledTimeCounter::StopTimer()
{
	timerStopped = true;
	currentTime = DataStructures::GetTime();
	return currentTime;
}

HexTime HexScaledTimeCounter::GetTimeSlapped()
{
	if (timerStopped==false)
		currentTime = DataStructures::GetTime();
	return (HexTime)(mTimeScale * (float)(currentTime - lastTime) + 0.5f);
}

DataStructures::HexString HexScaledTimeCounter::GetTimeSlappedString()
{
	HexTime timeGap = GetTimeSlapped();
	DataStructures::HexString res = FormatString("%id %ih %im %is %ims", timeGap/86400000, (timeGap%86400000)/3600000, (timeGap%3600000)/60000, timeGap%60000/1000, timeGap%1000);
	return res;
}

bool HexScaledTimeCounter::StopTimerAtThePoint(HexTime ms)
{
	if (timerStopped==true)
		return true;
	HexTime realMs = (HexTime)(mTimeScale * (float)ms + 0.5f);
	if (realMs>GetTimeSlapped())
	{
		StopTimer();
		return true;
	}
	return false;
}

void HexScaledTimeCounter::ForceDecreaseLastTime(HexTime newTime)
{
	//HexTime ms = (HexTime)(mTimeScale * (float)newTime + 0.5f);
	//if (lastTime >= ms)
	//	lastTime -= ms;
	//else
	//	lastTime = 0;
	if (lastTime >= newTime)
		lastTime -= newTime;
	else
		lastTime = 0;
}

void HexScaledTimeCounter::ForceIncreaseLastTime(HexTime newTime)
{
	//HexTime ms = (HexTime)(mTimeScale * (float)newTime + 0.5f);
	//lastTime += ms;
	lastTime += newTime;
}

bool HexScaledTimeCounter::AcquireTimerState(HexTime ms)
{
	if (timerStopped==true)
		return true;
	HexTime realMs = (HexTime)(mTimeScale * (float)ms + 0.5f);
	if (realMs>GetTimeSlapped())
		return true;
	return false;
}

void HexScaledTimeCounter::SetTimeScale(float scale)
{
	mTimeScale = scale;
}