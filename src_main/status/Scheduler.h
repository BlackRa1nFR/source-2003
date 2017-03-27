//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SCHEDULER_H
#define SCHEDULER_H
#ifdef _WIN32
#pragma once
#endif

#include <afx.h>
#include <UtlVector.h>

class CScheduler
{
public:
	CScheduler() {}
	~CScheduler() {}

	void AddTime( CString str ) { SchedulerTimes.AddToTail( str ); }
	CString GetTime( int index ) { return SchedulerTimes[index]; }
	void RemoveAllTimes() { SchedulerTimes.RemoveAll();}
	void RemoveItemByIndex( int item ) { SchedulerTimes.Remove(item);}

	int GetItemCount( void ) { return SchedulerTimes.Count(); }
private:

	CUtlVector<CString> SchedulerTimes;
};

extern CScheduler *GetScheduler();

#endif // SCHEDULER_H
