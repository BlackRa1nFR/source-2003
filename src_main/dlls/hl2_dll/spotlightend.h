//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Dynamic light at the end of a spotlight 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef	SPOTLIGHTEND_H
#define	SPOTLIGHTEND_H

#ifdef _WIN32
#pragma once
#endif


#include "baseentity.h"

class CSpotlightEnd : public CBaseEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CSpotlightEnd, CBaseEntity );

	void				Spawn( void );

	DECLARE_SERVERCLASS();

public:
	CNetworkVar( float, m_flLightScale );
	CNetworkVar( float, m_Radius );
	CNetworkVector( m_vSpotlightDir );
	CNetworkVector( m_vSpotlightOrg );
};

#endif	//SPOTLIGHTEND_H


