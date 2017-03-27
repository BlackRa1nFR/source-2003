//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of CKamikazeAward
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef KAMIKAZEAWARD_H
#define KAMIKAZEAWARD_H
#ifdef WIN32
#pragma once
#endif
#include "Award.h"
#include <map>

using namespace std;
//------------------------------------------------------------------------------------------------------
// Purpose: CKamikazeAward is an award given to the player who kills him/herself
// the most often.
//------------------------------------------------------------------------------------------------------
class CKamikazeAward: public CAward
{
protected:
	map<PID,int> numdeaths;
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CKamikazeAward():CAward("Kamikaze"){}
	void getWinner();
};
#endif // KAMIKAZEAWARD_H
