//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of CSharpshooterAward
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef SHARPSHOOTERAWARD_H
#define SHARPSHOOTERAWARD_H
#ifdef WIN32
#pragma once
#endif
#include "Award.h"
#include <map>


using namespace std;
//------------------------------------------------------------------------------------------------------
// Purpose: CSharpshooterAward is the award for best sniper.
// It is calculated by finding all of a sniper's kills with his rifle, then totaling them
// up, with headshots being worth three regular shots.
//------------------------------------------------------------------------------------------------------
class CSharpshooterAward: public CAward
{
protected:
	static double HS_VALUE;
	static double SHOT_VALUE;


	map<PID,int> numhs;
	map<PID,int> numshots;
	map<PID,int> sharpshooterscore;
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CSharpshooterAward():CAward("Sharpshooter"){}
	void getWinner();
};
#endif // SHARPSHOOTERAWARD_H
