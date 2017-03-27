//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Shield used by the shield scanner.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "scanner_shield.h"
#include "beam_shared.h"
#include "Sprite.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"

LINK_ENTITY_TO_CLASS(scanner_shield, CScannerShield);

BEGIN_DATADESC( CScannerShield )

	DEFINE_ARRAY( CScannerShield, m_pCageBeam, FIELD_CLASSPTR, NUM_CAGE_BEAMS ),

	// Function Pointers
	DEFINE_FUNCTION( CScannerShield, FadeIn ),
	DEFINE_FUNCTION( CScannerShield, FadeOut ),
	DEFINE_FUNCTION( CScannerShield, Spark ),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CScannerShield::Precache( void )
{
	engine->PrecacheModel( "models/Scanner_Shield.mdl" );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CScannerShield::Spawn( void )
{
	Precache();
	SetModel( "models/Scanner_Shield.mdl" );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_NONE );
	UTIL_SetSize(this, Vector(-30,-30,0), Vector(30,30,72));
	Relink();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CScannerShield::TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType)
{
	EmitSound( "ScannerShield.MetalDebris" );

	// -------------
	//  Impact Glow
	// -------------
	CSprite* pImpact = CSprite::SpriteCreate( "sprites/glow01.vmt", ptr->endpos, true );
	pImpact->SetTransparency( kRenderGlow, 255, 200, 200, 0, kRenderFxNoDissipation );
	pImpact->SetBrightness( 255 );
	pImpact->SetScale( 0.2 );
	pImpact->Expand( 1.0, 100 );
	pImpact->SetParent( this );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CScannerShield::IsShieldOn(void)
{
	return IsFollowingEntity();
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CScannerShield::SetTarget( CBaseEntity *pTarget )
{
	if (pTarget == NULL)
	{
		SetThink(FadeOut);
		SetNextThink( gpGlobals->curtime );
	}

	else if ( pTarget == GetFollowedEntity() )
	{
		return;
	}
	else
	{
		m_fEffects &= ~EF_NODRAW;
		FollowEntity( pTarget );
		SetOwnerEntity( pTarget );
		SetThink(FadeIn);
		SetRenderColorA( 0 );
		SetNextThink( gpGlobals->curtime );
		Relink();
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CScannerShield::FadeIn(void)
{
	SetRenderColorA( m_clrRender->a + 0.02 );
	if (m_clrRender->a >= 1.0)
	{
		SetRenderColorA( 1 );
		SetThink(Spark);
	}

	// Go collidable 1/3 of the way into the fade
	else if (m_clrRender->a >= 0.3)
	{
		if (IsSolidFlagSet(FSOLID_NOT_SOLID))
		{
			RemoveSolidFlags( FSOLID_NOT_SOLID );
			Relink();
		}
	}

	Spark();
	for (int i=0;i<NUM_CAGE_BEAMS;i++)
	{ 
		m_pCageBeam[i]->SetBrightness(255*m_clrRender->a);
	}
	SetNextThink( gpGlobals->curtime );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CScannerShield::FadeOut(void)
{
	SetRenderColorA( m_clrRender->a - 0.05 );

	if (m_clrRender->a <= 0.0)
	{
		SetOwnerEntity( NULL );
		AddSolidFlags( FSOLID_NOT_SOLID );
		StopFollowingEntity();
		SetMoveType( MOVETYPE_NONE );
		m_fEffects	|= EF_NODRAW;
		SetRenderColorA( 0 );
		Relink();

		// -----------------------------
		//	Stop the shield cage beams
		// -----------------------------
		for (int i=0;i<3;i++)
		{
			UTIL_RemoveImmediate(m_pCageBeam[i]);
			m_pCageBeam[i] = NULL;
		}
		SetThink(NULL);
		return;
	}
	// Go non-collidable 1/3 of the way out of the fade
	else if (m_clrRender->a <= 0.3)
	{
		if (!IsSolidFlagSet(FSOLID_NOT_SOLID))
		{
			AddSolidFlags( FSOLID_NOT_SOLID );
			Relink();
		}
	}

	Spark();
	for (int i=0;i<NUM_CAGE_BEAMS;i++)
	{ 
		m_pCageBeam[i]->SetBrightness(255*m_clrRender->a);
	}

	SetNextThink( gpGlobals->curtime );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CScannerShield::Spark(void)
{
	for (int i=0;i<NUM_CAGE_BEAMS;i++)
	{
		if (!m_pCageBeam[i])
		{
			m_pCageBeam[i] = CBeam::BeamCreate( "sprites/physbeam.vmt", 1 );//<<TEMP2>>temp art
			m_pCageBeam[i]->SetColor( 255, 255, 255 );
			m_pCageBeam[i]->SetBrightness( 0 );
			m_pCageBeam[i]->SetNoise( 100 );
			m_pCageBeam[i]->SetWidth(4.0);
			m_pCageBeam[i]->SetEndWidth(1.0);
		}

		CBaseEntity* entArray[3];
		int attArray[3];

		entArray[0] = this;
		entArray[1] = this;
		entArray[2] = this;
		attArray[0] = 2;
		attArray[2] = random->RandomInt(3,9);
		attArray[1] = attArray[2]+7;

		m_pCageBeam[i]->SplineInit(3, entArray, attArray);

		Vector vOrigin;
		QAngle vAngles;
		GetAttachment(attArray[2],vOrigin,vAngles);
		g_pEffects->Sparks(vOrigin);
	}

	SetNextThink( gpGlobals->curtime );
}
