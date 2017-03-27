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
#include "c_basetempentity.h"

#define MAX_SPLINE_POINTS 16
//-----------------------------------------------------------------------------
// Purpose: BeamSpline TE
//-----------------------------------------------------------------------------
class C_TEBeamSpline : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEBeamSpline, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEBeamSpline( void );
	virtual			~C_TEBeamSpline( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecPoints[ MAX_SPLINE_POINTS ];
	int				m_nPoints;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamSpline::C_TEBeamSpline( void )
{
	int i;
	for ( i = 0; i < MAX_SPLINE_POINTS; i++ )
	{
		m_vecPoints[ i ].Init();
	}
	m_nPoints = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBeamSpline::~C_TEBeamSpline( void )
{
}

void TE_BeamSpline( IRecipientFilter& filter, float delay,
	int points, Vector* rgPoints )
{
	DevMsg( 1, "Beam spline with %i points invoked\n", points );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBeamSpline::PostDataUpdate( DataUpdateType_t updateType )
{
	//
	DevMsg( 1, "Beam spline with %i points received\n", m_nPoints );
}

// Expose the TE to the engine.
IMPLEMENT_CLIENTCLASS_EVENT( C_TEBeamSpline, DT_TEBeamSpline, CTEBeamSpline );

BEGIN_RECV_TABLE_NOBASE(C_TEBeamSpline, DT_TEBeamSpline)
	RecvPropInt( RECVINFO( m_nPoints )),
	RecvPropArray(
		RecvPropVector( RECVINFO(m_vecPoints[0])),
		m_vecPoints)
END_RECV_TABLE()

