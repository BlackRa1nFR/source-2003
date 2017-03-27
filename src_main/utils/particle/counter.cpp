
#include <windows.h>
#include "counter.h"


#define LOCK_AT_MICROSECONDS


static unsigned __int64 g_CountDiv = 1;


// Win2K has a VERY high counter frequency but it's more useful if it's in 
// microseconds.
class CountDivSetter
{
public:
	CountDivSetter()
	{
		LARGE_INTEGER perSec;
		
		QueryPerformanceFrequency(&perSec);
#ifdef LOCK_AT_MICROSECONDS
		g_CountDiv = perSec.QuadPart / 1000000;
#else
		g_CountDiv = 1;
#endif
	}
};
static CountDivSetter __g_CountDivSetter;



unsigned long cnt_NumTicksPerSecond()
{
	LARGE_INTEGER perSec;

	QueryPerformanceFrequency(&perSec);
	return (unsigned long)(perSec.QuadPart / g_CountDiv);	
}



Counter::Counter(unsigned long startMode)
{
	if(startMode == CSTART_MICRO)
		StartMicro();
	else if(startMode == CSTART_MILLI)
		StartMS();
}


void Counter::StartMS()
{
	LARGE_INTEGER *pInt;

	pInt = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(pInt);
}

unsigned long Counter::EndMS()
{
	LARGE_INTEGER curCount, *pInCount, perSec;
	LONGLONG timeElapsed;

	pInCount = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(&curCount);

	QueryPerformanceFrequency(&perSec);
	timeElapsed = curCount.QuadPart - pInCount->QuadPart;
	return (unsigned long)((timeElapsed*(LONGLONG)1000) / perSec.QuadPart);
}

unsigned long Counter::CountMS()
{
	return EndMS();
}


void Counter::StartMicro()
{
	LARGE_INTEGER *pInt;

	pInt = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(pInt);
}

unsigned long Counter::EndMicro()
{
	LARGE_INTEGER curCount, *pInCount;

	pInCount = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(&curCount);

	return (unsigned long)((curCount.QuadPart - pInCount->QuadPart) / g_CountDiv);
}

unsigned long Counter::CountMicro()
{
	return EndMicro();
}



void cnt_StartCounter(Counter *pCounter)
{
	pCounter->StartMicro();
}


unsigned long cnt_EndCounter(Counter *pCounter)
{
	return pCounter->EndMicro();
}




