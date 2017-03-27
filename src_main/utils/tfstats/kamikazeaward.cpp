//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Implementation of CKamikazeAward
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "KamikazeAward.h"

//------------------------------------------------------------------------------------------------------
// Function:	CKamikazeAward::getWinner
// Purpose:	determines the winner of the award
//------------------------------------------------------------------------------------------------------
void CKamikazeAward::getWinner()
{
	CEventListIterator it;
	
	for (it=g_pMatchInfo->eventList()->begin(); it != g_pMatchInfo->eventList()->end(); ++it)
	{
		if ((*it)->getType()==CLogEvent::SUICIDE || (*it)->getType()==CLogEvent::KILLED_BY_WORLD)
		{
			PID kami=(*it)->getArgument(0)->asPlayerGetPID();
			numdeaths[kami]++;
			winnerID=kami;
			fNoWinner=false;
		}
	}
	
	map<PID,int>::iterator kamiter;

	for (kamiter=numdeaths.begin();kamiter!=numdeaths.end();++kamiter)
	{
		int currID=(*kamiter).first;
		if (numdeaths[currID]>numdeaths[winnerID])
			winnerID=currID;
	}
}

//------------------------------------------------------------------------------------------------------
// Function:	CKamikazeAward::noWinner
// Purpose:	writes html indicating that no one won this award
// Input:	html - the html file to write to
//------------------------------------------------------------------------------------------------------
void CKamikazeAward::noWinner(CHTMLFile& html)
{
	html.write("No one killed themselves during this match! Good work!");
}

//------------------------------------------------------------------------------------------------------
// Function:	CKamikazeAward::extendedinfo
// Purpose:	 reports how many times the winner killed him/herself
// Input:	html - the html file to write to
//------------------------------------------------------------------------------------------------------
void CKamikazeAward::extendedinfo(CHTMLFile& html)
{
	if (numdeaths[winnerID]==1)
		html.write("%s suicided once.",winnerName.c_str());
	else
		html.write("%s was self-victimized %li times.",winnerName.c_str(),numdeaths[winnerID]);
}

