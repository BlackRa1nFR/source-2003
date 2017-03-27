//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:  Implementation of CEventList
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include <stdlib.h>
#include "TFStatsApplication.h"
#include "EventList.h"
#include "memdbg.h"

#pragma warning (disable : 4786)
//------------------------------------------------------------------------------------------------------
// Function:	CEventList::readEventList
// Purpose:	 reads and returns a CEventList from a logfile
// Input:	filename -  the logfile to read from
// Output:	CEventList*
//------------------------------------------------------------------------------------------------------
CEventList* CEventList::readEventList(const char* filename)
{
	
	//	ifstream ifs(filename);
	CEventList* plogfile=new TRACKED CEventList();
	if (!plogfile)
	{
		printf("TFStats ran out of memory!\n");
		return NULL;
	}
	
	
	FILE* f=fopen(filename,"rt");
	if (!f)
		g_pApp->fatalError("Error opening %s, please make sure that the file exists and is not being accessed by other processes",filename);

	while (!feof(f))
	{
		CLogEvent* curr=NULL;
		curr=new TRACKED CLogEvent(f);
		
		if (!curr->isValid())
		{
			delete curr;
			break;//eof reached
		}
		plogfile->insert(plogfile->end(),curr);
	}		
	fclose(f);
#ifndef WIN32
		chmod(filename,PERMIT);
#endif

	return plogfile;
	
}

