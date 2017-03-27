//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of CTeamKillAward
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef TEAMKILLAWARD_H
#define TEAMKILLAWARD_H
#ifdef WIN32
#pragma once
#endif
#include "Award.h"
#include <map>


using namespace std;
//------------------------------------------------------------------------------------------------------
// Purpose: CTeamKillAward is an award given to the player who kills more of 
// their teammates than any other player.
//------------------------------------------------------------------------------------------------------
class CTeamKillAward: public CAward
{
protected:
	map<PID,int> numbetrayals;
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CTeamKillAward():CAward("Antisocial"){}
	void getWinner();
};
#endif // TEAMKILLAWARD_H
