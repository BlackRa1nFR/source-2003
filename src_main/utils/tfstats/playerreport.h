//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:  Interface of CPlayerReport
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef PLAYERREPORT_H
#define PLAYERREPORT_H
#ifdef WIN32
#pragma once
#endif
#include "Player.h"
#include "Report.h"
#include "PlrPersist.h"

//------------------------------------------------------------------------------------------------------
// Purpose:  Reports a specific player's stats. 
//------------------------------------------------------------------------------------------------------
class CPlayerReport: public CReport
{
private:
	CPlayer* pPlayer;	
	CPlrPersist* pPersist;
	int iWhichTeam;
	
	static map<unsigned long,bool> alreadyPersisted;
	static map<unsigned long,bool> alreadyWroteCombStats;
	bool reportingPersistedPlayer;

	void writePersistHTML(CHTMLFile& html);
public:
	CPlayerReport(CPlayer* pP,int t):pPlayer(pP),iWhichTeam(t){reportingPersistedPlayer=false;}
	CPlayerReport(CPlrPersist* pPP):pPersist(pPP) {iWhichTeam=-1;reportingPersistedPlayer=true;}

	virtual void writeHTML(CHTMLFile& html);
};


#endif // PLAYERREPORT_H
