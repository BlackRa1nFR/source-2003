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
#include "iviewrender_beams.h"


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class C_TEBeamFollow : public C_TEBaseBeam
{
public:
	DECLARE_CLASS( C_TEBeamFollow, C_TEBaseBeam );
	DECLARE_CLIENTCLASS();

					C_TEBeamFollow( void );
	virtual			~C_TEBeamFollow( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	
public:

	int m_iEntIndex;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamFollow::C_TEBeamFollow( void )
{
	m_iEntIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamFollow::~C_TEBeamFollow( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBeamFollow::PostDataUpdate( DataUpdateType_t updateType )
{
	beams->CreateBeamFollow( m_iEntIndex, m_nModelIndex, m_nHaloIndex, 0, m_fLife,
		m_fWidth, m_fEndWidth, m_nFadeLength, r, g, b, a );
}

// Expose the TE to the engine.
IMPLEMENT_CLIENTCLASS_EVENT( C_TEBeamFollow, DT_TEBeamFollow, CTEBeamFollow );

BEGIN_RECV_TABLE(C_TEBeamFollow, DT_TEBeamFollow)
	RecvPropInt( RECVINFO(m_iEntIndex)),
END_RECV_TABLE()
