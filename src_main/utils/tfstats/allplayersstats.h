//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface to CAllPlayersStats
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef ALLPLAYERSSTATS_H
#define ALLPLAYERSSTATS_H
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
// Purpose: CAllPlayersStats is a whole page report element that reports specific
// data about each player that has played on the server.  Data such as favourite
// weapon, rank, classes played, favourite class, and kills vs deaths.
//------------------------------------------------------------------------------------------------------
class CAllPlayersStats :public CReport
{
private:
	
	void init();
	
	
	public:
		explicit CAllPlayersStats(){init();}
		
		void generate();
		void writeHTML(CHTMLFile& html);
};

#endif // ALLPLAYERSSTATS_H
