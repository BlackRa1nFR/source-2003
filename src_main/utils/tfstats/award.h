//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of CAward, the base class for all awards
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef AWARD_H
#define AWARD_H
#ifdef WIN32
#pragma once
#endif
#pragma warning(disable :4786)
#include "Report.h"
#include <string>


//------------------------------------------------------------------------------------------------------
// Purpose: CAward is the base class for all awards, it is in turn a subclass of
// CReport. This class handles all the boring details of writing out the awards
// and things, so that the subclasses need only specify the name of the award
// and who won it, then this class will take care of the rest
//------------------------------------------------------------------------------------------------------
class CAward: public CReport
{
//from CReport:
protected:
	virtual void generate();
protected:
	std::string awardName;
	std::string winnerName;
	PID winnerID;
	bool fNoWinner;

	CAward(char* name);
	
	
	virtual void extendedinfo(CHTMLFile& html){};
	virtual void noWinner(CHTMLFile& html){};

	
public:
	virtual void getWinner(){}
	virtual void writeHTML(CHTMLFile& html);
	
	virtual ~CAward();

};


#endif // AWARD_H
