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
#include "cbase.h"
#include "c_te_basebeam.h"

//-----------------------------------------------------------------------------
// Purpose: Contains common variables for all beam TEs
//-----------------------------------------------------------------------------
C_TEBaseBeam::C_TEBaseBeam( void )
{
	m_nModelIndex	= 0;
	m_nHaloIndex	= 0;
	m_nStartFrame	= 0;
	m_nFrameRate	= 0;
	m_fLife			= 0.0;
	m_fWidth		= 0;
	m_fEndWidth		= 0;
	m_nFadeLength	= 0;
	m_fAmplitude	= 0;
	r = g = b = a = 0;
	m_nSpeed		= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBaseBeam::~C_TEBaseBeam( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBaseBeam::PreDataUpdate( DataUpdateType_t updateType ) 
{ 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBaseBeam::PostDataUpdate( DataUpdateType_t updateType )
{
	Assert( 0 );
}


IMPLEMENT_CLIENTCLASS(C_TEBaseBeam, DT_BaseBeam, CTEBaseBeam);

BEGIN_RECV_TABLE_NOBASE( C_TEBaseBeam, DT_BaseBeam )
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropInt( RECVINFO(m_nHaloIndex)),
	RecvPropInt( RECVINFO(m_nStartFrame)),
	RecvPropInt( RECVINFO(m_nFrameRate)),
	RecvPropFloat( RECVINFO(m_fLife)),
	RecvPropFloat( RECVINFO(m_fWidth)),
	RecvPropFloat( RECVINFO(m_fEndWidth)),
	RecvPropInt( RECVINFO(m_nFadeLength)),
	RecvPropFloat( RECVINFO(m_fAmplitude)),
	RecvPropInt( RECVINFO(m_nSpeed)),
	RecvPropInt( RECVINFO(r)),
	RecvPropInt( RECVINFO(g)),
	RecvPropInt( RECVINFO(b)),
	RecvPropInt( RECVINFO(a)),
END_RECV_TABLE()

