//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of CSurvivalistAward
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef SURVIVALISTAWARD_H
#define SURVIVALISTAWARD_H
#ifdef WIN32
#pragma once
#endif
#include "Award.h"

//------------------------------------------------------------------------------------------------------
// Purpose: CSurvivalistAward is an award given to the player who died the least
// while playing as a scout
//------------------------------------------------------------------------------------------------------
class CSurvivalistAward: public CAward
{
protected:
	map <PID,int> numdeaths;
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CSurvivalistAward():CAward("Survivalist"){}
	void getWinner();
};
#endif // SURVIVALISTAWARD_H
