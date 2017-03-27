//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "usercmd.h"
#include "igamesystem.h"
#include "ilagcompensationmanager.h"

static ConVar sv_unlag("sv_unlag", "1", FCVAR_SERVER );
static ConVar sv_maxunlag("sv_maxunlag"	, "0.5", FCVAR_NONE );
static ConVar sv_unlagpush("sv_unlagpush"	, "0.0", FCVAR_NONE );
static ConVar sv_unlagsamples("sv_unlagsamples", "1", FCVAR_NONE );

#define LC_NONE				0
#define LC_ALIVE			(1<<0)

#define LC_ORIGIN_CHANGED	(1<<8)
#define LC_ANGLES_CHANGED	(1<<9)
#define LC_SIZE_CHANGED		(1<<10)

#define LAG_COMPENSATION_TELEPORTED_DISTANCE_SQR ( 64.0f * 64.0f )
#define LAG_COMPENSATION_EPS_SQR ( 0.1f * 0.1f )
// Allow 4 units of error ( about 1 / 8 bbox width )
#define LAG_COMPENSATION_ERROR_EPS_SQR ( 4.0f * 4.0f )
// Only keep 1 second of data
#define LAG_COMPENSATION_DATA_TIME	1.0f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct LagRecord
{
public:
	LagRecord()
	{
		m_nPlayerIndex = 0;
		m_fFlags = 0;
		m_vecOrigin.Init();
		m_vecAngles.Init();
		m_vecMins.Init();
		m_vecMaxs.Init();
		m_bActive = false;		
	}
	LagRecord( const LagRecord& src )
	{
		m_nPlayerIndex = src.m_nPlayerIndex;
		m_fFlags = src.m_fFlags;
		m_vecOrigin = src.m_vecOrigin;
		m_vecAngles = src.m_vecAngles;
		m_vecMins = src.m_vecMins;
		m_vecMaxs = src.m_vecMaxs;
		m_bActive = src.m_bActive;
	}

	bool					m_bActive;
	// Which player this belongs to
	int						m_nPlayerIndex;
	// Did player die this frame
	int						m_fFlags;

	// Player position, orientation and bbox
	Vector					m_vecOrigin;
	QAngle					m_vecAngles;
	Vector					m_vecMins;
	Vector					m_vecMaxs;

	// Fixme, do we care about animation frame?
	// float				m_flFrame;
	// int					m_nSequence;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class LagCompensationContext
{
public:
	LagCompensationContext()
	{ 
		m_flRecordTime = 0.0f;
		memset( m_Records, 0, sizeof( m_Records ) );
	}
	LagCompensationContext( const LagCompensationContext& src )
	{
		m_flRecordTime = src.m_flRecordTime;
		int i;
		for ( i = 0; i < MAX_CLIENTS; i++ )
		{
			m_Records[ i ] = src.m_Records[ i ];
		}
	}
	// Timestamp record was created (at end of frame?)
	float			m_flRecordTime;
	LagRecord		m_Records[ MAX_CLIENTS ];	
};



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLagCompensationManager : public CAutoGameSystem, public ILagCompensationManager
{
public:
	// IServerSystem stuff
	virtual void Shutdown()
	{
		m_Data.Purge();
	}

	virtual void LevelShutdownPostEntity()
	{
		m_Data.RemoveAll();
	}

	// called after entities think
	virtual void FrameUpdatePostEntityThink();

	// ILagCompensationManager stuff

	// Called during player movement to set up/restore after lag compensation
	void			StartLagCompensation( CBasePlayer *player, CUserCmd *cmd );
	void			FinishLagCompensation( CBasePlayer *player );

private:
	float			GetLatency( CBasePlayer *player );
	void			DecayStaleContextData( void );

	bool			FindSpanningContexts( float targettime, int *context1, int *context2 );

	CUtlVector< LagCompensationContext >	m_Data;

	// Scratchpad for determining what needs to be restored
	unsigned int	restorebits;
	bool			m_bNeedToRestore;
	enum
	{
		RESTORE_RECORD = 0,
		CHANGE_RECORD,

		// Must be last
		MAX_RECORDS
	};

	LagRecord		restoreData[ MAX_RECORDS ][ MAX_CLIENTS ];
};

static CLagCompensationManager g_LagCompensationManager;
ILagCompensationManager *lagcompensation = &g_LagCompensationManager;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
// Output : float CLagCompensation::GetLatency
//-----------------------------------------------------------------------------
float CLagCompensationManager::GetLatency ( CBasePlayer *player )
{
	int backtrack = sv_unlagsamples.GetInt();
	backtrack = max( 1,  backtrack );
	// Clamp to backup, or 16 at most
	backtrack = min( backtrack, gpGlobals->maxClients <= 1 ? 4 : 16 );

	float ping = engine->GetPlayerPing( player->entindex(), backtrack ); 
	return ping;
}

void CLagCompensationManager::DecayStaleContextData( void )
{
	float deadtime = gpGlobals->realtime - LAG_COMPENSATION_DATA_TIME;

	int c = m_Data.Size();
	int i;
	for ( i = c - 1; i >= 0; i-- )
	{
		LagCompensationContext *context = &m_Data[ i ];
		if ( context->m_flRecordTime >= deadtime )
			continue;

		m_Data.Remove( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called once per frame after all entities have had a chance to think
//-----------------------------------------------------------------------------
void CLagCompensationManager::FrameUpdatePostEntityThink()
{
	DecayStaleContextData();

	LagCompensationContext context;
	context.m_flRecordTime = gpGlobals->realtime;

	// Iterate all active players
	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		LagRecord *record = &context.m_Records[ i - 1 ];
		record->m_bActive = true;
		record->m_nPlayerIndex = pPlayer->entindex();
		record->m_fFlags = 0;
		if ( pPlayer->IsAlive() )
		{
			record->m_fFlags |= LC_ALIVE;
		}
		record->m_vecAngles			= pPlayer->GetLocalAngles();
		record->m_vecOrigin			= pPlayer->GetLocalOrigin();
		record->m_vecMaxs			= pPlayer->WorldAlignMaxs();
		record->m_vecMins			= pPlayer->WorldAlignMins();
	}

	m_Data.AddToHead( context );
}

bool CLagCompensationManager::FindSpanningContexts( float targettime, int *newer, int *older )
{
	Assert( older && newer );
	*newer = -1;
	*older = -1;

	int count = m_Data.Size();
	if ( count < 2 )
		return false;

	int i;
	int j;
	for ( i = 0; i < count - 1; i++ )
	{
		j = i + 1;

		// New items are pushed onto front of stack
		LagCompensationContext *c1, *c2;
		c1 = &m_Data[ i ]; // newer
		c2 = &m_Data[ j ]; // older

		if ( targettime >= c2->m_flRecordTime && 
			 targettime <= c1->m_flRecordTime )
		{
			*newer = i;
			*older = j;
			return true;
		}
	}

	*newer = count - 2;
	*older = count - 1;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Simple linear interpolation
// Input  : frac - 
//			src - 
//			dest - 
//			output - 
//-----------------------------------------------------------------------------
static void InterpolateVector( float frac, const Vector& src, const Vector& dest, Vector& output )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		output[ i ] = src[ i ] + frac * ( dest[ i ] - src[ i ] );
	}
}

static void InterpolateAngles( float frac, const QAngle& src, const QAngle& dest, QAngle& output )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		output[ i ] = src[ i ] + frac * ( dest[ i ] - src[ i ] );
	}
}

// Called during player movement to set up/restore after lag compensation
void CLagCompensationManager::StartLagCompensation( CBasePlayer *player, CUserCmd *cmd )
{
	// Assume no players need to be restored
	restorebits = 0UL;
	m_bNeedToRestore = false;
	memset( restoreData, 0, sizeof( restoreData ) );

	// Player not wanting lag compensation
	if ( !cmd->lag_compensation )
		return;

	// Get true latency
	float latency = GetLatency( player ) / 1000.0f;

	float update_interval = 0.0f;
	if (  cmd->updaterate > 0 )
	{
		update_interval = 1.0f / (float)cmd->updaterate;
	}

	// Fixup delay based on message interval (cl_updaterate, default 20 so 50 msec)
	latency -= update_interval;

	// Further fixup due to client side delay because packets arrive 1/2 through the frame loop, on average
	latency -= ( cmd->frametime ) /** 0.5f*/;

	// Absolute bounds on lag compensation
	float correct = min( LAG_COMPENSATION_DATA_TIME, latency );

	// See if server is applying a lower cap
	if ( sv_maxunlag.GetFloat() )
	{
		// Make sure it's not negative
		if ( sv_maxunlag.GetFloat() < 0.0f )
		{
			sv_maxunlag.SetValue( 0.0f );
		}

		// Apply server cap
		correct = min( correct, sv_maxunlag.GetFloat() );
	}

	// Get true timestamp
	float realtime = gpGlobals->realtime;

	// Figure out timestamp for which we are looking for data
	float targettime = realtime - correct;

	// Remove lag based on player interpolation, as well
	targettime -= cmd->lerp_msec / 1000.0f;

	// Server can apply a fudge, probably not needed, defaults to 0.0f
	targettime += sv_unlagpush.GetFloat();

	// Cap target to present time, of course
	targettime = min( realtime, targettime );

	int newer = -1;
	int older = -1;

	if ( !FindSpanningContexts( targettime, &newer, &older ) )
	{
		return;
	}

	// Couldn't find suitable span!!!
	if ( newer == -1 || older == -1 )
	{
		return;
	}

	// Iterate all active players
	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
		{
			continue;
		}

		// Don't lag compensate yourself you loser...
		if ( player == pPlayer )
		{
			continue;
		}

		int index = pPlayer->entindex() - 1;

		// Walk context looking for any invalidating event
		Vector prevOrigin;
		prevOrigin.Init();
		bool wasAlive = true;

		bool lost_track = false;

		for ( int j = 0; j <= older; j++ )
		{
			LagCompensationContext *context = &m_Data[ j ];
			Assert( context );

			LagRecord *record = &context->m_Records[ index ];

			if ( !record->m_bActive )
			{
				lost_track = true;
				break;
			}

			if ( j != 0 )
			{
				// Check for state change
				bool alive = ( record->m_fFlags & LC_ALIVE ) ? true : false;
				if ( alive ^ wasAlive )
				{
					// Respawned or died, stop chain here
					lost_track = true;
					break;
				}

				Vector delta = record->m_vecOrigin - prevOrigin;
				if ( delta.LengthSqr() > LAG_COMPENSATION_TELEPORTED_DISTANCE_SQR )
				{
					lost_track = true;
					break;
				}
			}

			wasAlive = ( record->m_fFlags & LC_ALIVE ) ? true : false;
			prevOrigin = record->m_vecOrigin;
		}

		// Player didn't exist all the way through history to spanning contexts!!!
		if ( lost_track )
			continue;

		// Okay, interpolate data
		LagCompensationContext *newcontext, *oldcontext;
		
		newcontext = &m_Data[ newer ];
		oldcontext = &m_Data[ older ];

		LagRecord *newrecord, *oldrecord;

		newrecord = &newcontext->m_Records[ index ];
		oldrecord = &oldcontext->m_Records[ index ];
		Assert( oldrecord && newrecord );

		float frac = 1.0f;
		if ( oldcontext->m_flRecordTime != newcontext->m_flRecordTime )
		{
			frac = ( targettime - oldcontext->m_flRecordTime ) / ( newcontext->m_flRecordTime - oldcontext->m_flRecordTime );
			frac = clamp( frac, 0.0f, 1.0f );
		}

		// Compute interpolated values
		Vector org;
		QAngle ang;
		Vector mins;
		Vector maxs;

		InterpolateVector( frac, oldrecord->m_vecOrigin, newrecord->m_vecOrigin, org );
		InterpolateAngles( frac, oldrecord->m_vecAngles, newrecord->m_vecAngles, ang );
		InterpolateVector( frac, oldrecord->m_vecMins, newrecord->m_vecMins, mins );
		InterpolateVector( frac, oldrecord->m_vecMaxs, newrecord->m_vecMaxs, maxs );

		// See if this represents a change for the player
		int flags = 0;
		LagRecord *restore = &restoreData[ RESTORE_RECORD ][ index ];
		LagRecord *change  = &restoreData[ CHANGE_RECORD ][ index ];

		QAngle angdiff = pPlayer->GetLocalAngles() - ang;

		if ( angdiff.LengthSqr() > LAG_COMPENSATION_EPS_SQR )
		{
			flags |= LC_ANGLES_CHANGED;
			restore->m_vecAngles = pPlayer->GetLocalAngles();
			pPlayer->SetLocalAngles( angdiff );
			change->m_vecAngles = angdiff;
		}

		// Use absoluate equality here
		if ( ( mins != pPlayer->WorldAlignMins() ) ||
			 ( maxs != pPlayer->WorldAlignMaxs() ) )
		{
			flags |= LC_SIZE_CHANGED;
			restore->m_vecMins = pPlayer->WorldAlignMins() ;
			restore->m_vecMaxs = pPlayer->WorldAlignMaxs();
			pPlayer->SetSize( mins, maxs );
			change->m_vecMins = mins;
			change->m_vecMaxs = maxs;
		}

		Vector diff = pPlayer->GetLocalOrigin() - org;

		// Note, do origin at end since it causes a relink into the k/d tree
		if ( diff.LengthSqr() > LAG_COMPENSATION_EPS_SQR )
		{
			flags |= LC_ORIGIN_CHANGED;
			restore->m_vecOrigin = pPlayer->GetLocalOrigin();
			// Move player, but don't fire triggers
			UTIL_SetOrigin( pPlayer, org );
			change->m_vecOrigin = org;
		}

		if ( !flags )
		{
			continue;
		}

		restorebits |= (1<<index);
		m_bNeedToRestore = true;
		restore->m_bActive = true;
		restore->m_fFlags = flags;

		change->m_bActive = true;
		change->m_fFlags = flags;

		/*
		if ( cmd->lc_index == pPlayer->entindex() )
		{
			Vector lc_error;
			lc_error = pPlayer->GetLocalOrigin() - cmd->lc_origin;

			if ( lc_error.LengthSqr() > LAG_COMPENSATION_ERROR_EPS_SQR )
			{
				static float last_message;

				if ( realtime > last_message + 0.5f )
				{
					last_message = realtime;
					//
					//ClientPrint( player, HUD_PRINTCONSOLE, 
					//	UTIL_VarArgs( "LC error on command %i cl %f %f %f sv %f %f %f delta %f %f %f\n",
					//		cmd->command_number,
					//		cmd->lc_origin.x,
					//		cmd->lc_origin.y,
					//		cmd->lc_origin.z,
					//		pPlayer->GetLocalOrigin().x,
					//		pPlayer->GetLocalOrigin().y,
					//		pPlayer->GetLocalOrigin().z,
					//		lc_error.x,
					//		lc_error.y,
					//		lc_error.z ) );
					//

					// FIXME:  Could send down some bboxs overlays, too
					NDebugOverlay::Box( cmd->lc_origin, pPlayer->WorldAlignMins(), pPlayer->WorldAlignMaxs(), 255, 0, 255, 0 ,5.0f);
					NDebugOverlay::Box( pPlayer->GetAbsOrigin(), pPlayer->WorldAlignMins(), pPlayer->WorldAlignMaxs(), 255, 255, 0, 0 ,5.0f);
				}
			}
		}
		*/
	}
}

void CLagCompensationManager::FinishLagCompensation( CBasePlayer *player )
{
	if ( !m_bNeedToRestore )
		return;

	if ( restorebits == 0UL )
		return;

	// Iterate all active players
	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
		{
			continue;
		}

		int index = pPlayer->entindex() - 1;
		int bitmask = (1<<index);

		if ( !( bitmask & restorebits ) )
		{
			continue;
		}

		LagRecord *restore = &restoreData[ RESTORE_RECORD ][ index ];
		LagRecord *change  = &restoreData[ CHANGE_RECORD ][ index ];

		if ( !restore->m_bActive || !change->m_bActive )
		{
			Assert( 0 );
			continue;
		}

		if ( restore->m_fFlags & LC_SIZE_CHANGED )
		{
			// see if simulation made any changes, if no, then do the restore, otherwise,
			//  leave new values in
			if ( pPlayer->WorldAlignMins() == change->m_vecMins && 
				 pPlayer->WorldAlignMaxs() == change->m_vecMaxs )
			{
				// Restore it
				pPlayer->SetSize( restore->m_vecMins, restore->m_vecMaxs );
			}
		}

		if ( restore->m_fFlags & LC_ANGLES_CHANGED )
		{
			if ( pPlayer->GetLocalAngles() == change->m_vecAngles )
			{
				pPlayer->SetLocalAngles( restore->m_vecAngles );
			}
		}

		if ( restore->m_fFlags & LC_ORIGIN_CHANGED )
		{
			if ( pPlayer->GetLocalOrigin() == change->m_vecOrigin )
			{
				UTIL_SetOrigin( pPlayer, restore->m_vecOrigin );
			}
			else
			{
				// Okay, let's see if we can do something reasonable with the change
				Vector delta = pPlayer->GetLocalOrigin() - change->m_vecOrigin;
				// If it moved really far, just leave the player in the new spot!!!
				if ( delta.LengthSqr() < LAG_COMPENSATION_TELEPORTED_DISTANCE_SQR )
				{
					// Otherwise, see if offseting original origin by spot is valid
					Vector testPos = restore->m_vecOrigin + delta;
					trace_t tr;

					UTIL_TraceEntity( pPlayer, testPos, testPos, MASK_PLAYERSOLID, &tr );

					// Okay, see if new point is okay?
					if ( !tr.startsolid && !tr.allsolid )
					{
						// Move player to this other spot, touch any triggers as needed
						UTIL_SetOrigin( pPlayer, testPos, true );
					}
					else
					{
						// Okay, try moving from old origin to new spot instead
						UTIL_TraceEntity( pPlayer, restore->m_vecOrigin, testPos, MASK_PLAYERSOLID, &tr );

						// If this was valid use it, otherwise, leave the player in the spot the server 
						//  put him (i.e., he may get snapped back due to this)
						if ( !tr.startsolid && !tr.allsolid )
						{
							// Move player to this other spot, touch any triggers as needed
							UTIL_SetOrigin( pPlayer, tr.endpos, true );
						}
					}
				}
			}
		}
	}
}
