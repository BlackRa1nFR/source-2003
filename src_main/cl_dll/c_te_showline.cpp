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
#include "c_te_particlesystem.h"

//-----------------------------------------------------------------------------
// Purpose: Show Line TE
//-----------------------------------------------------------------------------
class C_TEShowLine : public C_TEParticleSystem
{
public:
	DECLARE_CLASS( C_TEShowLine, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

					C_TEShowLine( void );
	virtual			~C_TEShowLine( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecEnd;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEShowLine::C_TEShowLine( void )
{
	m_vecEnd.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEShowLine::~C_TEShowLine( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEShowLine::PostDataUpdate( DataUpdateType_t updateType )
{
	Vector		vec;
	float		len;
	StandardParticle_t	*p;
	int			dec;
	static int	tracercount;

	VectorSubtract (m_vecEnd, m_vecOrigin, vec);
	len = VectorNormalize (vec);

	dec = 3;

	VectorScale(vec, dec, vec);

	CSmartPtr<CTEParticleRenderer> pRen = CTEParticleRenderer::Create( "TEShowLine", m_vecOrigin );
	if( !pRen )
		return;
	
	while (len > 0)
	{
		len -= dec;

		p = pRen->AddParticle();
		if ( p )
		{
			p->m_Velocity.Init();

			pRen->SetParticleLifetime(p, 30);
			
			p->SetColor(0, 1, 1);
			p->SetAlpha(1);
			pRen->SetParticleType(p, pt_static);

			p->m_Pos = m_vecOrigin;
			
			m_vecOrigin += vec;
		}
	}
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEShowLine, DT_TEShowLine, CTEShowLine)
	RecvPropVector( RECVINFO(m_vecEnd)),
END_RECV_TABLE()

void TE_ShowLine( IRecipientFilter& filter, float delay,
	const Vector* start, const Vector* end )
{
	// Major hack to simulate receiving network message
	__g_C_TEShowLine.m_vecOrigin = *start;
	__g_C_TEShowLine.m_vecEnd = *end;

	__g_C_TEShowLine.PostDataUpdate( DATA_UPDATE_CREATED );
}