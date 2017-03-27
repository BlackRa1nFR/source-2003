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
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_te_particlesystem.h"

//-----------------------------------------------------------------------------
// Purpose: Large Funnel TE
//-----------------------------------------------------------------------------
class C_TELargeFunnel : public C_TEParticleSystem
{
public:
	DECLARE_CLASS( C_TELargeFunnel, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

					C_TELargeFunnel( void );
	virtual			~C_TELargeFunnel( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );


public:
	void			CreateFunnel( void );

	int				m_nModelIndex;
	int				m_nReversed;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TELargeFunnel::C_TELargeFunnel( void )
{
	m_nModelIndex = 0;
	m_nReversed = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TELargeFunnel::~C_TELargeFunnel( void )
{
}


void C_TELargeFunnel::CreateFunnel( void )
{
	int			i, j;
	StandardParticle_t	*p;
	float		vel;
	Vector		dir;
	Vector		dest;
	float		flDist;

	CSmartPtr<CTEParticleRenderer> pRen = CTEParticleRenderer::Create( "TELargeFunnel", m_vecOrigin );
	if( !pRen )
		return;

	for ( i = -256 ; i <= 256 ; i += 32 )
	{
		for ( j = -256 ; j <= 256 ; j += 32 )
		{
			p = pRen->AddParticle();
			if( p )
			{
				if ( m_nReversed )
				{
					p->m_Pos = m_vecOrigin;

					dest[0] = m_vecOrigin[0] + i;
					dest[1] = m_vecOrigin[1] + j;
					dest[2] = m_vecOrigin[2] + random->RandomFloat(100, 800);

					// send particle heading to dest at a random speed
					dir = dest - p->m_Pos;

					vel = dest[ 2 ] / 8;// velocity based on how far particle has to travel away from org
				}
				else
				{
					p->m_Pos[0] = m_vecOrigin[0] + i;
					p->m_Pos[1] = m_vecOrigin[1] + j;
					p->m_Pos[2] = m_vecOrigin[2] + random->RandomFloat(100, 800);

					// send particle heading to org at a random speed
					dir = m_vecOrigin - p->m_Pos;

					vel = p->m_Pos[ 2 ] / 8;// velocity based on how far particle starts from org
				}

				p->SetColor(0, 1, 0);
				p->SetAlpha(1);
				pRen->SetParticleType(p, pt_static);

				flDist = VectorNormalize (dir);	// save the distance

				if ( vel < 64 )
				{
					vel = 64;
				}
				
				vel += random->RandomFloat( 64, 128  );
				VectorScale ( dir, vel, p->m_Velocity );

				// die right when you get there
				pRen->SetParticleLifetime(p, flDist / vel);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TELargeFunnel::PostDataUpdate( DataUpdateType_t updateType )
{
	CreateFunnel();
	DevWarning( 1, "funnel sprite function needs to be rewritten\n" );
	//effects->FunnelSprite( m_vecOrigin, m_nModelIndex, m_nReversed );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TELargeFunnel, DT_TELargeFunnel, CTELargeFunnel)
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropInt( RECVINFO(m_nReversed)),
END_RECV_TABLE()

void TE_LargeFunnel( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, int reversed )
{
	// Major hack to simulate receiving network message
	__g_C_TELargeFunnel.m_vecOrigin = *pos;
	__g_C_TELargeFunnel.m_nModelIndex = modelindex;
	__g_C_TELargeFunnel.m_nReversed = reversed;

	__g_C_TELargeFunnel.PostDataUpdate( DATA_UPDATE_CREATED );
}


