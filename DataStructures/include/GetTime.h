#ifndef __GET_TIME_H
#define __GET_TIME_H

#include "CommonTypeDefines.h"

/// The namespace HexNet is not consistently used.  It's only purpose is to avoid compiler errors for classes whose names are very common.
/// For the most part I've tried to avoid this simply by using names very likely to be unique for my classes.
namespace DataStructures
{
	/// Returns the value from QueryPerformanceCounter.  This is the function HexNet uses to represent time.
	HexTime GetTime( void );
	HexTimeNS GetTimeNS( void );
	HexTime GetDuration( HexTime lastTime );
	HexTime GetDuration( HexTime lastTime, HexTime currentTime );
	HexTimeNS GetDurationNS( HexTimeNS lastTime );

}

#endif
