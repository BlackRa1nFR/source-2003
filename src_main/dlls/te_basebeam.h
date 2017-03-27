//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
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

//-----------------------------------------------------------------------------
// Purpose: Dispatches a beam ring between two entities
//-----------------------------------------------------------------------------
#if !defined( TE_BASEBEAM_H )
#define TE_BASEBEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "basetempentity.h"

class CTEBaseBeam : public CBaseTempEntity
{
public:

	DECLARE_CLASS( CTEBaseBeam, CBaseTempEntity );
	DECLARE_SERVERCLASS();


public:
					CTEBaseBeam( const char *name );
	virtual			~CTEBaseBeam( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles ) = 0;
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f ) = 0;

public:
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( int, m_nHaloIndex );
	CNetworkVar( int, m_nStartFrame );
	CNetworkVar( int, m_nFrameRate );
	CNetworkVar( float, m_fLife );
	CNetworkVar( float, m_fWidth );
	CNetworkVar( float, m_fEndWidth );
	CNetworkVar( int, m_nFadeLength );
	CNetworkVar( float, m_fAmplitude );
	CNetworkVar( int, r );
	CNetworkVar( int, g );
	CNetworkVar( int, b );
	CNetworkVar( int, a );
	CNetworkVar( int, m_nSpeed );
};

EXTERN_SEND_TABLE(DT_BaseBeam);

#endif // TE_BASEBEAM_H