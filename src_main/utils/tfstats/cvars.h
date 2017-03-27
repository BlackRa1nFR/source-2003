//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:  Interface of CCVarList
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef CVARS_H
#define CVARS_H
#ifdef WIN32
#pragma once
#endif
#include "report.h"

//------------------------------------------------------------------------------------------------------
// Purpose: CCVarList is a report element that outputs a two column table with 
// the cvars that were in effect while the match was running.  Cvars that contain
// various passwords are omitted from the listing.
//------------------------------------------------------------------------------------------------------
class CCVarList :public CReport
{
private:
	enum Consts
	{
		HTML_TABLE_WIDTH=500,
		HTML_TABLE_NUM_COLS=2,
	};
public:
	explicit CCVarList(){}
	
	void writeHTML(CHTMLFile& html);

};
#endif // CVARS_H
