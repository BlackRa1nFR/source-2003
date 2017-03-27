//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:  Implementation of CReport
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef REPORT_H
#define REPORT_H
#ifdef WIN32
#pragma once
#pragma warning(disable:4786)
#endif

#include "MatchInfo.h"
#include "HTML.h"


//------------------------------------------------------------------------------------------------------
// Purpose: CReport is the base class for all elements of a report. This includes
// things like scoreboards and awards.
//------------------------------------------------------------------------------------------------------
class CReport
{
protected:
	//every element must have some info about the match to go off of.
	//moved into global pointer.  g_pMatchInfo
	//CMatchInfo* pMatchInfo;
	virtual void init(){}
public:
	//explicit CReport(CMatchInfo* pMInfo):pMatchInfo(pMInfo){}
	explicit CReport(){}
	virtual void writeHTML(CHTMLFile& html){}
	virtual void generate(){}
	
	virtual void makeHTMLPage(char* pageName,char* pageTitle);
	virtual void report(CHTMLFile& html);
	virtual ~CReport(){}
};	


#endif // REPORT_H
