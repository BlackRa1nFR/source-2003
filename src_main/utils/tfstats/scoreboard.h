//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of CScoreboard
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef SCOREBOARD_H
#define SCOREBOARD_H
#ifdef WIN32
#pragma once
#endif
#pragma warning (disable:  4786)

#include "report.h"
#include <map>
#include <vector>
#include <string>
using namespace std;


//------------------------------------------------------------------------------------------------------
// Purpose: CScoreboard is a report element that outputs a scoreboard that tallies
// up all the kills and deaths for each player. it also displays (and sorts by) a 
// player's rank.  Rank is defined by how many kills a player got minus how many 
// times he died divided by the time he was in the game.
//------------------------------------------------------------------------------------------------------
class CScoreboard : public CReport
{
private:

	void init();
	
	public:
		explicit CScoreboard(){init();}
		
		void generate();
		void writeHTML(CHTMLFile& html);
};





#endif // SCOREBOARD_H
