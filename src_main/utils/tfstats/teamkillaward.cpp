//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:  Implemenation of CTeamKillAward
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "TeamKillAward.h"

//------------------------------------------------------------------------------------------------------
// Function:	CTeamKillAward::getWinner
// Purpose:	 generates the winner of the award
//------------------------------------------------------------------------------------------------------
void CTeamKillAward::getWinner()
{
	CEventListIterator it;
	
	for (it=g_pMatchInfo->eventList()->begin(); it != g_pMatchInfo->eventList()->end(); ++it)
	{
		if ((*it)->getType()==CLogEvent::TEAM_FRAG)
		{
			PID traitor=(*it)->getArgument(0)->asPlayerGetPID();
			numbetrayals[traitor]++;
			winnerID=traitor;
			fNoWinner=false;
		}
	}
	
	map<PID,int>::iterator traitoriter;

	for (traitoriter=numbetrayals.begin();traitoriter!=numbetrayals.end();++traitoriter)
	{
		int currID=(*traitoriter).first;
		if (numbetrayals[currID]>numbetrayals[winnerID])
			winnerID=currID;
	}
}

//------------------------------------------------------------------------------------------------------
// Function:	CTeamKillAward::noWinner
// Purpose:	 writes html to indicate that there was no winner of this award
// Input:	html - the html file to write to
//------------------------------------------------------------------------------------------------------
void CTeamKillAward::noWinner(CHTMLFile& html)
{
	html.write("No one killed any teammates! Good Work!");
}

//------------------------------------------------------------------------------------------------------
// Function:	CTeamKillAward::extendedinfo
// Purpose:	writes out how many teammates were killed by the traitor
// Input:	html - the html file to write to
//------------------------------------------------------------------------------------------------------
void CTeamKillAward::extendedinfo(CHTMLFile& html)
{
	html.write("The traitor %s murdered %li teammates.",winnerName.c_str(),numbetrayals[winnerID]);
}
