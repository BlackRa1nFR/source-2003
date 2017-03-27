//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basecombatweapon.h"
#include "explode.h"
#include "eventqueue.h"
#include "gamerules.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "game.h"

#include "player.h"
#include "entitylist.h"

class CMessageEntity : public CPointEntity
{
	DECLARE_CLASS( CMessageEntity, CPointEntity );

public:
	void	Spawn( void );
	void	Activate( void );
	void	Think( void );
	void	DrawOverlays(void);

	virtual void UpdateOnRemove();

	DECLARE_DATADESC();

protected:
	int				m_radius;
	string_t		m_messageText;
	bool			m_drawText;
	bool			m_bDeveloperOnly;

};

LINK_ENTITY_TO_CLASS( point_message, CMessageEntity );

BEGIN_DATADESC( CMessageEntity )

	DEFINE_KEYFIELD( CMessageEntity, m_radius, FIELD_INTEGER, "radius" ),
	DEFINE_KEYFIELD( CMessageEntity, m_messageText, FIELD_STRING, "message" ),
	DEFINE_KEYFIELD( CMessageEntity, m_bDeveloperOnly, FIELD_BOOLEAN, "developeronly" ),
	DEFINE_FIELD( CMessageEntity, m_drawText, FIELD_BOOLEAN ),

END_DATADESC()

static CUtlVector< CHandle< CMessageEntity > >	g_MessageEntities;

//-----------------------------------------
// Spawn
//-----------------------------------------
void CMessageEntity::Spawn( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );
	m_drawText = false;
	m_bDeveloperOnly = false;
	//m_debugOverlays |= OVERLAY_TEXT_BIT;		// make sure we always show the text
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageEntity::Activate( void )
{
	BaseClass::Activate();

	CHandle< CMessageEntity > h;
	h = this;
	g_MessageEntities.AddToTail( h );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessageEntity::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	CHandle< CMessageEntity > h;
	h = this;
	g_MessageEntities.FindAndRemove( h );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------
// Think
//-----------------------------------------
void CMessageEntity::Think( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	// check for player distance
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );

	if ( !pPlayer || ( pPlayer->GetFlags() & FL_NOTARGET ) )
		return;

	Vector worldTargetPosition = pPlayer->EyePosition();

	// bail if player is too far away
	if ( (worldTargetPosition - GetAbsOrigin()).Length() > m_radius )
	{
		m_drawText = false;
		return;
	}

	// turn on text
	m_drawText = true;
}
	
//-------------------------------------------
//-------------------------------------------
void CMessageEntity::DrawOverlays(void) 
{
	if ( !m_drawText )
	{
		return;
	}

	if ( m_bDeveloperOnly && !g_pDeveloper->GetInt() )
	{
		return;
	}

	// display text if they are within range
	char tempstr[512];
	Q_snprintf( tempstr, sizeof(tempstr), "%s", STRING(m_messageText) );
	NDebugOverlay::EntityText(entindex(), 0, tempstr, 0);
}

// This is a hack to make point_message stuff appear in developer 0 release builds
//  for now
void DrawMessageEntities()
{
	int c = g_MessageEntities.Count();
	for ( int i = c - 1; i >= 0; i-- )
	{
		CMessageEntity *me = g_MessageEntities[ i ];
		if ( !me )
		{
			g_MessageEntities.Remove( i );
			continue;
		}

		me->DrawOverlays();
	}
}
