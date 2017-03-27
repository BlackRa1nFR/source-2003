//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:  Interface of CDialogueReadout
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef DIALOGUEREADOUT_H
#define DIALOGUEREADOUT_H
#ifdef WIN32
#pragma once
#endif
#include "Report.h"

//------------------------------------------------------------------------------------------------------
// Purpose: CDialogueReadout is a full page report element that outputs a listing
// of all the dialogue in the match.  It also reports deaths, and suicides since those
// usually beget lots of smack talking.
//------------------------------------------------------------------------------------------------------
class CDialogueReadout : public CReport
{
private:
public:
	explicit CDialogueReadout(){}
	void writeHTML(CHTMLFile& html);
};
#endif // DIALOGUEREADOUT_H
