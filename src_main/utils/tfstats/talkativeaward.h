//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of CTalkativeAward
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef TALKATIVEAWARD_H
#define TALKATIVEAWARD_H
#ifdef WIN32
#pragma once
#endif
#include "Award.h"
#include <map>
using namespace std;

//------------------------------------------------------------------------------------------------------
// Purpose: CTalkativeAward is an award given to the player who speaks the most
// words. 
//------------------------------------------------------------------------------------------------------
class CTalkativeAward: public CAward
{
protected:
	map<PID,int> numtalks;
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
	
	map<int,string> fulltalktext;
public:
	explicit CTalkativeAward():CAward("Bigmouth"){}
	void getWinner();
};
#endif // TALKATIVEAWARD_H
