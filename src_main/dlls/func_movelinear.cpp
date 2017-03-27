//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements a brush model entity that moves along a linear path.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "func_movelinear.h"
#include "entitylist.h"
#include "locksounds.h"
#include "ndebugoverlay.h"
#include "engine/IEngineSound.h"


// -------------------------------
//  SPAWN_FLAGS
// -------------------------------
#define SF_MOVELINEAR_NOTSOLID		8

LINK_ENTITY_TO_CLASS( func_movelinear, CFuncMoveLinear );
LINK_ENTITY_TO_CLASS( momentary_door, CFuncMoveLinear );	// For backward compatibility

BEGIN_DATADESC( CFuncMoveLinear )

	DEFINE_KEYFIELD( CFuncMoveLinear, m_vecMoveDir,		 FIELD_VECTOR, "movedir" ),
	DEFINE_KEYFIELD( CFuncMoveLinear, m_soundStart,		 FIELD_SOUNDNAME, "StartSound" ),
	DEFINE_KEYFIELD( CFuncMoveLinear, m_soundStop,		 FIELD_SOUNDNAME, "StopSound" ),
	DEFINE_FIELD( CFuncMoveLinear, m_currentSound, FIELD_SOUNDNAME ),
	DEFINE_KEYFIELD( CFuncMoveLinear, m_flBlockDamage,	 FIELD_FLOAT,	"BlockDamage"),
	DEFINE_KEYFIELD( CFuncMoveLinear, m_flStartPosition, FIELD_FLOAT,	"StartPosition"),
	DEFINE_KEYFIELD( CFuncMoveLinear, m_flMoveDistance,  FIELD_FLOAT,	"MoveDistance"),

	// Inputs
	DEFINE_INPUTFUNC( CFuncMoveLinear, FIELD_VOID,  "Open", InputOpen ),
	DEFINE_INPUTFUNC( CFuncMoveLinear, FIELD_VOID,  "Close", InputClose ),
	DEFINE_INPUTFUNC( CFuncMoveLinear, FIELD_FLOAT, "SetPosition", InputSetPosition ),

	// Outputs
	DEFINE_OUTPUT( CFuncMoveLinear, m_OutPosition, "Position" ),

	// Functions
	DEFINE_FUNCTION( CFuncMoveLinear, StopMoveSound ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose: Called before spawning, after keyvalues have been parsed.
//------------------------------------------------------------------------------
void CFuncMoveLinear::Spawn( void )
{
	// Convert movedir from angles to a vector
	QAngle angMoveDir = QAngle( m_vecMoveDir.x, m_vecMoveDir.y, m_vecMoveDir.z );
	AngleVectors( angMoveDir, &m_vecMoveDir );

	SetMoveType( MOVETYPE_PUSH );

	Relink();
	SetModel( STRING( GetModelName() ) );
	
	// Don't allow zero or negative speeds
	if (m_flSpeed <= 0)
	{
		m_flSpeed = 100;
	}
	
	// If move distance is set to zero, use with width of the 
	// brush to determine the size of the move distance
	if (m_flMoveDistance <= 0)
	{
		m_flMoveDistance = (fabs( m_vecMoveDir.x * (EntitySpaceSize().x-2) ) + 
							fabs( m_vecMoveDir.y * (EntitySpaceSize().y-2) ) + 
							fabs( m_vecMoveDir.z * (EntitySpaceSize().z-2) ));

		// For backwards compatibility subtract the lip (if it was set)
		m_flMoveDistance -= m_flLip;
	}

	m_vecPosition1 = GetLocalOrigin()    - (m_vecMoveDir * m_flMoveDistance * m_flStartPosition);
	m_vecPosition2 = m_vecPosition1 + (m_vecMoveDir * m_flMoveDistance);
	m_vecFinalDest = GetLocalOrigin();

	SetTouch( NULL );

	Precache();

	// It is solid?
	SetSolid( SOLID_BSP );
	if ( FBitSet (m_spawnflags, SF_MOVELINEAR_NOTSOLID) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
	}
	
	CreateVPhysics();
}

//------------------------------------------------------------------------------
bool CFuncMoveLinear::CreateVPhysics( void )
{
	if ( !IsSolidFlagSet( FSOLID_NOT_SOLID ) )
	{
		VPhysicsInitShadow( false, false );
	}

	return true;
}
	
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CFuncMoveLinear::Precache( void )
{
	if (m_soundStart != NULL_STRING)
	{
		enginesound->PrecacheSound( (char *) STRING(m_soundStart) );
	}
	if (m_soundStop != NULL_STRING)
	{
		enginesound->PrecacheSound( (char *) STRING(m_soundStop) );
	}
	m_currentSound = NULL_STRING;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CFuncMoveLinear::MoveTo(Vector vPosition, float flSpeed)
{
	if ( flSpeed != 0 )
	{
		if ( m_soundStart != NULL_STRING )
		{
			if (m_currentSound == m_soundStart)
			{
				StopSound(entindex(), CHAN_BODY, (char*)STRING(m_soundStop));
			}
			else
			{
				m_currentSound = m_soundStart;
				CPASAttenuationFilter filter( this );

				EmitSound( filter, entindex(), CHAN_BODY, (char*)STRING(m_soundStart), 1, ATTN_NORM );	
			}
		}

		LinearMove( vPosition, flSpeed );

		// Clear think (that stops sounds)
		SetThink(NULL);
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CFuncMoveLinear::StopMoveSound( void )
{
	if ( m_soundStart != NULL_STRING && ( m_currentSound == m_soundStart ) )
	{
		StopSound(entindex(), CHAN_BODY, (char*)STRING(m_soundStart) );
	}

	if ( m_soundStop != NULL_STRING && ( m_currentSound != m_soundStop ) )
	{
		m_currentSound = m_soundStop;
		CPASAttenuationFilter filter( this );
		EmitSound( filter, entindex(), CHAN_BODY, (char*)STRING(m_soundStop), 1, ATTN_NORM );
	}

	SetThink(NULL);
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CFuncMoveLinear::MoveDone( void )
{
	// Stop sounds at the next think, rather than here as another
	// SetPosition call might immediately follow the end of this move
	SetThink(&CFuncMoveLinear::StopMoveSound);
	SetNextThink( gpGlobals->curtime + 0.1f );
	BaseClass::MoveDone();
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CFuncMoveLinear::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType != USE_SET )		// Momentary buttons will pass down a float in here
		return;

	if ( value > 1.0 )
		value = 1.0;
	Vector move = m_vecPosition1 + (value * (m_vecPosition2 - m_vecPosition1));
	
	Vector delta = move - GetLocalOrigin();
	float speed = delta.Length() * 10;

	MoveTo(move, speed);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the position as a value from [0..1].
//-----------------------------------------------------------------------------
void CFuncMoveLinear::SetPosition( float flPosition )
{
	Vector vTargetPos = m_vecPosition1 + ( flPosition * (m_vecPosition2 - m_vecPosition1));
	if ((vTargetPos - GetLocalOrigin()).Length() > 0.001)
	{
		MoveTo(vTargetPos, m_flSpeed);
	}
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CFuncMoveLinear::InputOpen( inputdata_t &inputdata )
{
	if (GetLocalOrigin() != m_vecPosition2)
	{
		MoveTo(m_vecPosition2, m_flSpeed);
	}
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CFuncMoveLinear::InputClose( inputdata_t &inputdata )
{
	if (GetLocalOrigin() != m_vecPosition1)
	{
		MoveTo(m_vecPosition1, m_flSpeed);
	}
}


//------------------------------------------------------------------------------
// Purpose: Input handler for setting the position from [0..1].
// Input  : Float position.
//-----------------------------------------------------------------------------
void CFuncMoveLinear::InputSetPosition( inputdata_t &inputdata )
{
	SetPosition( inputdata.value.Float() );
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame when the bruch is blocked while moving
// Input  : pOther - The blocking entity.
// Output :
//-----------------------------------------------------------------------------
void CFuncMoveLinear::Blocked( CBaseEntity *pOther )
{
	// Hurt the blocker 
	if ( m_flBlockDamage )
	{
		pOther->TakeDamage( CTakeDamageInfo( this, this, m_flBlockDamage, DMG_CRUSH ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CFuncMoveLinear::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		float flTravelDist = (m_vecPosition1 - m_vecPosition2).Length();
		float flCurDist	   = (m_vecPosition1 - GetLocalOrigin()).Length();
		Q_snprintf(tempstr,sizeof(tempstr),"Current Pos: %3.3f",flCurDist/flTravelDist);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		float flTargetDist	   = (m_vecPosition1 - m_vecFinalDest).Length();
		Q_snprintf(tempstr,sizeof(tempstr),"Target Pos: %3.3f",flTargetDist/flTravelDist);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}
