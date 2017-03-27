//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "ai_link.h"

//-----------------------------------------------------------------------------
// Purpose:	Given the source node ID, returns the destination ID
// Input  :
// Output :
//-----------------------------------------------------------------------------	
int	CAI_Link::DestNodeID(int srcID)
{
	if (srcID == m_iSrcID)
	{
		return m_iDestID;
	}
	else
	{
		return m_iSrcID;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Link::CAI_Link(void)
{
	m_iSrcID			= -1;
	m_iDestID			= -1;
	m_LinkInfo			= 0;
	m_timeStaleExpires = 0;
	m_pDynamicLink = NULL;

	for (int hull=0;hull<NUM_HULLS;hull++)
	{
		m_iAcceptedMoveTypes[hull] = 0;
	}

};
