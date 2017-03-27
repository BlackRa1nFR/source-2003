//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: Used to create a path that can be followed by NPCs and trains.
//
//=============================================================================

#include "cbase.h"
#include "trains.h"
#include "entitylist.h"
#include "ndebugoverlay.h"


class CPathCorner : public CPointEntity
{
	DECLARE_CLASS( CPathCorner, CPointEntity );
public:

	void	Spawn( );
	float	GetDelay( void ) { return m_flWait; }
	int		DrawDebugTextOverlays(void);
	void	DrawDebugGeometryOverlays(void);

	// Input handlers	
	void InputSetNextPathCorner( inputdata_t &inputdata );
	void InputInPass( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	float			m_flWait;
	COutputEvent	m_OnPass;
};

LINK_ENTITY_TO_CLASS( path_corner, CPathCorner );


class CPathCornerCrash : public CPathCorner
{
	DECLARE_CLASS( CPathCornerCrash, CPathCorner );
};

LINK_ENTITY_TO_CLASS( path_corner_crash, CPathCornerCrash );


BEGIN_DATADESC( CPathCorner )

	DEFINE_KEYFIELD( CPathCorner, m_flWait, FIELD_FLOAT, "wait" ),

	// Inputs
	DEFINE_INPUTFUNC( CPathCorner,	FIELD_STRING, "SetNextPathCorner", InputSetNextPathCorner),

	// Internal inputs - not exposed in the FGD
	DEFINE_INPUTFUNC( CPathCorner, FIELD_VOID, "InPass", InputInPass ),

	// Outputs
	DEFINE_OUTPUT( CPathCorner, m_OnPass, "OnPass"),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathCorner::Spawn( void )
{
	ASSERTSZ(GetEntityName() != NULL_STRING, "path_corner without a targetname");
}


//------------------------------------------------------------------------------
// Purpose: Sets the next path corner by name.
// Input  : String ID name of next path corner.
//-----------------------------------------------------------------------------
void CPathCorner::InputSetNextPathCorner( inputdata_t &inputdata )
{
	m_target = inputdata.value.StringID();
}


//-----------------------------------------------------------------------------
// Purpose: Fired by path followers as they pass the path corner.
//-----------------------------------------------------------------------------
void CPathCorner::InputInPass( inputdata_t &inputdata )
{
	m_OnPass.FireOutput( inputdata.pActivator, inputdata.pCaller, 0);
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CPathCorner::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print Target
		// --------------
		char tempstr[255];
		if (m_target!=NULL_STRING) 
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Target: %s",STRING(m_target));
		}
		else
		{
			Q_strncpy(tempstr,"Target:   -  ",sizeof(tempstr));
		}
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


//-----------------------------------------------------------------------------
// Purpose: Override base class to add display of paths
//-----------------------------------------------------------------------------
void CPathCorner::DrawDebugGeometryOverlays(void) 
{
	// ----------------------------------------------
	// Draw line to next target is bbox is selected
	// ----------------------------------------------
	if (m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_ABSBOX_BIT))
	{
		NDebugOverlay::Box(GetAbsOrigin(), Vector(-10,-10,-10), Vector(10,10,10), 255, 100, 100, 0 ,0);

		if (m_target != NULL_STRING)
		{
			CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_target, NULL );
			if (pTarget)
			{
				NDebugOverlay::Line(GetAbsOrigin(),pTarget->GetAbsOrigin(),255,100,100,true,0.0);
			}
		}
	}
	BaseClass::DrawDebugGeometryOverlays();
}

BEGIN_DATADESC( CPathTrack )

	DEFINE_FIELD( CPathTrack, m_length, FIELD_FLOAT ),
	DEFINE_FIELD( CPathTrack, m_pnext, FIELD_CLASSPTR ),
	DEFINE_FIELD( CPathTrack, m_paltpath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CPathTrack, m_pprevious, FIELD_CLASSPTR ),
	
	DEFINE_KEYFIELD( CPathTrack, m_altName, FIELD_STRING, "altpath" ),
	
	DEFINE_INPUTFUNC( CPathTrack, FIELD_VOID, "InPass", InputPass ),
	
	DEFINE_INPUTFUNC( CPathTrack, FIELD_VOID, "EnableAlternatePath", InputEnableAlternatePath ),
	DEFINE_INPUTFUNC( CPathTrack, FIELD_VOID, "DisableAlternatePath", InputDisableAlternatePath ),
	DEFINE_INPUTFUNC( CPathTrack, FIELD_VOID, "ToggleAlternatePath", InputToggleAlternatePath ),

	DEFINE_INPUTFUNC( CPathTrack, FIELD_VOID, "EnablePath", InputEnablePath ),
	DEFINE_INPUTFUNC( CPathTrack, FIELD_VOID, "DisablePath", InputDisablePath ),
	DEFINE_INPUTFUNC( CPathTrack, FIELD_VOID, "TogglePath", InputTogglePath ),

	// Outputs
	DEFINE_OUTPUT(CPathTrack, m_OnPass, "OnPass"),

END_DATADESC()

LINK_ENTITY_TO_CLASS( path_track, CPathTrack );

//-----------------------------------------------------------------------------
// Purpose: Toggles the track to or from its alternate path
//-----------------------------------------------------------------------------
void CPathTrack::ToggleAlternatePath( void )
{
	// Use toggles between two paths
	if ( m_paltpath != NULL )
	{
		if ( FBitSet( m_spawnflags, SF_PATH_ALTERNATE ) == false )
		{
			EnableAlternatePath();
		}
		else
		{
			DisableAlternatePath();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::EnableAlternatePath( void )
{
	if ( m_paltpath != NULL )
	{
		SETBITS( m_spawnflags, SF_PATH_ALTERNATE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::DisableAlternatePath( void )
{
	if ( m_paltpath != NULL )
	{
		CLEARBITS( m_spawnflags, SF_PATH_ALTERNATE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputEnableAlternatePath( inputdata_t &inputdata )
{
	EnableAlternatePath();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputDisableAlternatePath( inputdata_t &inputdata )
{
	DisableAlternatePath();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputToggleAlternatePath( inputdata_t &inputdata )
{
	ToggleAlternatePath();
}

//-----------------------------------------------------------------------------
// Purpose: Toggles the track to or from its alternate path
//-----------------------------------------------------------------------------
void CPathTrack::TogglePath( void )
{
	// Use toggles between two paths
	if ( FBitSet( m_spawnflags, SF_PATH_DISABLED ) )
	{
		EnablePath();
	}
	else
	{
		DisablePath();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::EnablePath( void )
{
	CLEARBITS( m_spawnflags, SF_PATH_DISABLED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::DisablePath( void )
{
	SETBITS( m_spawnflags, SF_PATH_DISABLED );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputEnablePath( inputdata_t &inputdata )
{
	EnablePath();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputDisablePath( inputdata_t &inputdata )
{
	DisablePath();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPathTrack::InputTogglePath( inputdata_t &inputdata )
{
	TogglePath();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathTrack::Link( void  )
{
	CBaseEntity *pTarget;

	if ( m_target != NULL_STRING )
	{
		pTarget = gEntList.FindEntityByName( NULL, m_target, NULL );

		if ( pTarget == this)
		{
			Warning("ERROR: path_track (%s) refers to itself as a target!\n", GetDebugName());
			
			//FIXME: Why were we removing this?  If it was already connected to, we weren't updating the other linked
			//		 end, causing problems with walking through bogus memory links!  -- jdw

			//UTIL_Remove(this);
			//return;
		}
		else if ( pTarget )
		{
			m_pnext = dynamic_cast<CPathTrack*>( pTarget );

			if ( m_pnext )		// If no next pointer, this is the end of a path
			{
				m_pnext->SetPrevious( this );
			}
		}
		else
		{
			Warning("Dead end link: %s\n", STRING( m_target ) );
		}
	}

	// Find "alternate" path
	if ( m_altName != NULL_STRING )
	{
		pTarget = gEntList.FindEntityByName( NULL, m_altName, NULL );
		if ( pTarget )
		{
			m_paltpath = dynamic_cast<CPathTrack*>( pTarget );

			if ( m_paltpath )		// If no next pointer, this is the end of a path
			{
				m_paltpath->SetPrevious( this );
			}
		}
	}
}


void CPathTrack::Spawn( void )
{
	SetSolid( SOLID_NONE );
	UTIL_SetSize(this, Vector(-8, -8, -8), Vector(8, 8, 8));

	m_pnext = NULL;
	m_pprevious = NULL;
}


void CPathTrack::Activate( void )
{
	BaseClass::Activate();

	if ( GetEntityName() != NULL_STRING )		// Link to next, and back-link
		Link();
}

CPathTrack	*CPathTrack::ValidPath( CPathTrack	*ppath, int testFlag )
{
	if ( !ppath )
		return NULL;

	if ( testFlag && FBitSet( ppath->m_spawnflags, SF_PATH_DISABLED ) )
		return NULL;

	return ppath;
}


void CPathTrack::Project( CPathTrack *pstart, CPathTrack *pend, Vector &origin, float dist )
{
	if ( pstart && pend )
	{
		Vector dir = (pend->GetLocalOrigin() - pstart->GetLocalOrigin());
		VectorNormalize( dir );
		origin = pend->GetLocalOrigin() + dir * dist;
	}
}

CPathTrack *CPathTrack::GetNext( void )
{
	if ( m_paltpath && FBitSet( m_spawnflags, SF_PATH_ALTERNATE ) && !FBitSet( m_spawnflags, SF_PATH_ALTREVERSE ) )
		return m_paltpath;
	
	return m_pnext;
}



CPathTrack *CPathTrack::GetPrevious( void )
{
	if ( m_paltpath && FBitSet( m_spawnflags, SF_PATH_ALTERNATE ) && FBitSet( m_spawnflags, SF_PATH_ALTREVERSE ) )
		return m_paltpath;
	
	return m_pprevious;
}



void CPathTrack::SetPrevious( CPathTrack *pprev )
{
	// Only set previous if this isn't my alternate path
	if ( pprev && !FStrEq( STRING(pprev->GetEntityName()), STRING(m_altName) ) )
		m_pprevious = pprev;
}


// Assumes this is ALWAYS enabled
CPathTrack *CPathTrack::LookAhead( Vector &origin, float dist, int move )
{
	CPathTrack *pcurrent;
	float originalDist = dist;
	
	pcurrent = this;
	Vector currentPos = origin;

	if ( dist < 0 )		// Travelling backwards through path
	{
		dist = -dist;
		while ( dist > 0 )
		{
			Vector dir = pcurrent->GetLocalOrigin() - currentPos;
			float length = dir.Length();
			if ( !length )
			{
				if ( !ValidPath(pcurrent->GetPrevious(), move) ) 	// If there is no previous node, or it's disabled, return now.
				{
					if ( !move )
						Project( pcurrent->GetNext(), pcurrent, origin, dist );
					return NULL;
				}
				pcurrent = pcurrent->GetPrevious();
			}
			else if ( length > dist )	// enough left in this path to move
			{
				origin = currentPos + (dir * (dist / length));
				return pcurrent;
			}
			else
			{
				dist -= length;
				currentPos = pcurrent->GetLocalOrigin();
				origin = currentPos;
				if ( !ValidPath(pcurrent->GetPrevious(), move) )	// If there is no previous node, or it's disabled, return now.
					return NULL;

				pcurrent = pcurrent->GetPrevious();
			}
		}
		origin = currentPos;
		return pcurrent;
	}
	else 
	{
		while ( dist > 0 )
		{
			if ( !ValidPath(pcurrent->GetNext(), move) )	// If there is no next node, or it's disabled, return now.
			{
				if ( !move )
					Project( pcurrent->GetPrevious(), pcurrent, origin, dist );
				return NULL;
			}
			Vector dir = pcurrent->GetNext()->GetLocalOrigin() - currentPos;
			float length = dir.Length();
			if ( !length  && !ValidPath( pcurrent->GetNext()->GetNext(), move ) )
			{
				if ( dist == originalDist ) // HACK -- up against a dead end
					return NULL;
				return pcurrent;
			}
			if ( length > dist )	// enough left in this path to move
			{
				origin = currentPos + (dir * (dist / length));
				return pcurrent;
			}
			else
			{
				dist -= length;
				currentPos = pcurrent->GetNext()->GetLocalOrigin();
				pcurrent = pcurrent->GetNext();
				origin = currentPos;
			}
		}
		origin = currentPos;
	}

	return pcurrent;
}

	
// Assumes this is ALWAYS enabled
CPathTrack *CPathTrack::Nearest( Vector origin )
{
	int			deadCount;
	float		minDist, dist;
	Vector		delta;
	CPathTrack	*ppath, *pnearest;


	delta = origin - GetLocalOrigin();
	delta.z = 0;
	minDist = delta.Length();
	pnearest = this;
	ppath = GetNext();

	// Hey, I could use the old 2 racing pointers solution to this, but I'm lazy :)
	deadCount = 0;
	while ( ppath && ppath != this )
	{
		deadCount++;
		if ( deadCount > 9999 )
		{
			Warning( "Bad sequence of path_tracks from %s", GetDebugName() );
			Assert(0);
			return NULL;
		}
		delta = origin - ppath->GetLocalOrigin();
		delta.z = 0;
		dist = delta.Length();
		if ( dist < minDist )
		{
			minDist = dist;
			pnearest = ppath;
		}
		ppath = ppath->GetNext();
	}
	return pnearest;
}


CPathTrack *CPathTrack::Instance( edict_t *pent )
{
	CBaseEntity *pEntity = CBaseEntity::Instance( pent );
	if ( FClassnameIs( pEntity, "path_track" ) )
		return (CPathTrack *)pEntity;
	return NULL;
}

void CPathTrack::InputPass( inputdata_t &inputdata )
{
	m_OnPass.FireOutput( inputdata.pActivator, this );
}
