//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Implementation of CWhoKilledWho
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef WHOKILLEDWHO_H
#define WHOKILLEDWHO_H
#ifdef WIN32
#pragma once
#endif

#include "report.h"

//------------------------------------------------------------------------------------------------------
// Purpose: CWhoKilledWho is a report element that outputs a detailed scoreboard
// showing, in a 2x2 matrix who killed who how many times. On the edges of the 
// matrix, total kills and total deaths for each player are also tallied up
//------------------------------------------------------------------------------------------------------
class CWhoKilledWho  :public CReport
{
private:

	int* kills;
	int size;
	int* deaths;

	//bids map from a player ID to a "board ID" because
	//player IDs are not necessarily contiguous
	//and rarely start at 0
	map<PID,int> bids;
	int nextbid;

	//this maps from bids back to pids
	map<int,PID> pids;

	map<pair<int,PID>,int> bidMap;
	map<int,pair<int,PID> > bidMap2;

	//returns the style class for a cell in the html table
	const char* getCellClass(int u,int v);
	void init();

	void makeBidMap();
public:
	explicit CWhoKilledWho(){kills=deaths=NULL;init();}
	void generate();
	void writeHTML(CHTMLFile& html);
	~CWhoKilledWho();
};



#endif // WHOKILLEDWHO_H
