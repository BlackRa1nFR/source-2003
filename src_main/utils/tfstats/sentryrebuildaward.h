//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of CSentryRebuildAward
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef SENTRYREBUILDAWARD_H
#define SENTRYREBUILDAWARD_H
#ifdef WIN32
#pragma once
#endif
#include "Award.h"
#include <map>


using namespace std;
//------------------------------------------------------------------------------------------------------
// Purpose:  CSentryRebuildAward is given to the engineers who built a sentry
// gun the most times
//------------------------------------------------------------------------------------------------------
class CSentryRebuildAward: public CAward
{
protected:
	map<PID,int> numbuilds;
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CSentryRebuildAward():CAward("Worst Sentry Placement"){}
	void getWinner();
};
#endif // SENTRYREBUILDAWARD_H
