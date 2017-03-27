//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Utility code.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "saverestore.h"
#include "globalstate.h"
#include <stdarg.h>
#include "shake.h"
#include "decals.h"
#include "player.h"
#include "gamerules.h"
#include "entitylist.h"
#include "bspfile.h"
#include "mathlib.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "soundflags.h"
#include "ispatialpartition.h"
#include "igamesystem.h"
#include "saverestoretypes.h"
#include "checksum_crc.h"
#include "hierarchy.h"
#include "iservervehicle.h"
#include "te_effect_dispatch.h"
#include "utldict.h"


extern short		g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud
extern short		g_sModelIndexBloodDrop;		// (in combatweapon.cpp) holds the sprite index for the initial blood
extern short		g_sModelIndexBloodSpray;	// (in combatweapon.cpp) holds the sprite index for splattered blood

#ifdef	DEBUG
void DBG_AssertFunction( bool fExpr, const char *szExpr, const char *szFile, int szLine, const char *szMessage )
{
	if (fExpr)
		return;
	char szOut[512];
	if (szMessage != NULL)
		Q_snprintf(szOut,sizeof(szOut), "ASSERT FAILED:\n %s \n(%s@%d)\n%s", szExpr, szFile, szLine, szMessage);
	else
		Q_snprintf(szOut,sizeof(szOut), "ASSERT FAILED:\n %s \n(%s@%d)\n", szExpr, szFile, szLine);
	Warning( szOut);
}
#endif	// DEBUG


//-----------------------------------------------------------------------------
// Entity creation factory
//-----------------------------------------------------------------------------
class CEntityFactoryDictionary : public IEntityFactoryDictionary
{
public:
	CEntityFactoryDictionary();

	virtual void InstallFactory( IEntityFactory *pFactory, const char *pClassName );
	virtual IServerNetworkable *Create( const char *pClassName );
	virtual void Destroy( const char *pClassName, IServerNetworkable *pNetworkable );

private:
	IEntityFactory *FindFactory( const char *pClassName );

	CUtlDict< IEntityFactory *, unsigned short > m_Factories;
};


//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
IEntityFactoryDictionary *EntityFactoryDictionary()
{
	static CEntityFactoryDictionary s_EntityFactory;
	return &s_EntityFactory;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CEntityFactoryDictionary::CEntityFactoryDictionary() : m_Factories( true, 0, 128 )
{
}


//-----------------------------------------------------------------------------
// Finds a new factory
//-----------------------------------------------------------------------------
IEntityFactory *CEntityFactoryDictionary::FindFactory( const char *pClassName )
{
	unsigned short nIndex = m_Factories.Find( pClassName );
	if ( nIndex == m_Factories.InvalidIndex() )
		return NULL;
	return m_Factories[nIndex];
}


//-----------------------------------------------------------------------------
// Install a new factory
//-----------------------------------------------------------------------------
void CEntityFactoryDictionary::InstallFactory( IEntityFactory *pFactory, const char *pClassName )
{
	Assert( FindFactory( pClassName ) == NULL );
	m_Factories.Insert( pClassName, pFactory );
}


//-----------------------------------------------------------------------------
// Instantiate something using a factory
//-----------------------------------------------------------------------------
IServerNetworkable *CEntityFactoryDictionary::Create( const char *pClassName )
{
	IEntityFactory *pFactory = FindFactory( pClassName );
	if ( !pFactory )
	{
		Warning("Attempted to create unknown entity type %s!\n", pClassName );
		return NULL;
	}

	return pFactory->Create( pClassName );
}


//-----------------------------------------------------------------------------
// Destroy a networkable
//-----------------------------------------------------------------------------
void CEntityFactoryDictionary::Destroy( const char *pClassName, IServerNetworkable *pNetworkable )
{
	IEntityFactory *pFactory = FindFactory( pClassName );
	if ( !pFactory )
	{
		Warning("Attempted to destroy unknown entity type %s!\n", pClassName );
		return;
	}

	pFactory->Destroy( pNetworkable );
}


//-----------------------------------------------------------------------------
// class CFlaggedEntitiesEnum
//-----------------------------------------------------------------------------
// enumerate entities that match a set of edict flags into a static array
class CFlaggedEntitiesEnum : public IPartitionEnumerator
{
public:
	CFlaggedEntitiesEnum( CBaseEntity **pList, int listMax, int flagMask );
	// This gets called	by the enumeration methods with each element
	// that passes the test.
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );
	
	int GetCount() { return m_count; }
	bool AddToList( CBaseEntity *pEntity );
	
private:
	CBaseEntity		**m_pList;
	int				m_listMax;
	int				m_flagMask;
	int				m_count;
};

CFlaggedEntitiesEnum::CFlaggedEntitiesEnum( CBaseEntity **pList, int listMax, int flagMask )
{
	m_pList = pList;
	m_listMax = listMax;
	m_flagMask = flagMask;
	m_count = 0;
}

bool CFlaggedEntitiesEnum::AddToList( CBaseEntity *pEntity )
{
	if ( m_count >= m_listMax )
		return false;
	m_pList[m_count] = pEntity;
	m_count++;
	return true;
}

IterationRetval_t CFlaggedEntitiesEnum::EnumElement( IHandleEntity *pHandleEntity )
{
	CBaseEntity *pEntity = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
	if ( pEntity )
	{
		if ( m_flagMask && !(pEntity->GetFlags() & m_flagMask) )	// Does it meet the criteria?
			return ITERATION_CONTINUE;

		if ( !AddToList( pEntity ) )
			return ITERATION_STOP;
	}

	return ITERATION_CONTINUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int UTIL_PrecacheDecal( const char *name, bool preload )
{
	return engine->PrecacheDecal( name, preload );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int UTIL_EntitiesInBox( CBaseEntity **pList, int listMax, const Vector &mins, const Vector &maxs, int flagMask )
{
	CFlaggedEntitiesEnum boxEnum( pList, listMax, flagMask );
	partition->EnumerateElementsInBox( PARTITION_ENGINE_NON_STATIC_EDICTS, mins, maxs, false, &boxEnum );
	
	return boxEnum.GetCount();
}


int UTIL_EntitiesInSphere( CBaseEntity **pList, int listMax, const Vector &center, float radius, int flagMask )
{
	CFlaggedEntitiesEnum sphereEnum( pList, listMax, flagMask );
	partition->EnumerateElementsInSphere( PARTITION_ENGINE_NON_STATIC_EDICTS, center, radius, false, &sphereEnum );

	return sphereEnum.GetCount();
}

CEntitySphereQuery::CEntitySphereQuery( const Vector &center, float radius, int flagMask )
{
	m_listIndex = 0;
	m_listCount = UTIL_EntitiesInSphere( m_pList, ARRAYSIZE(m_pList), center, radius, flagMask );
}

CBaseEntity *CEntitySphereQuery::GetCurrentEntity()
{
	if ( m_listIndex < m_listCount )
		return m_pList[m_listIndex];
	return NULL;
}


//-----------------------------------------------------------------------------
// Simple trace filter
//-----------------------------------------------------------------------------
class CTracePassFilter : public CTraceFilter
{
public:
	CTracePassFilter( IHandleEntity *pPassEnt ) : m_pPassEnt( pPassEnt ) {}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
			return false;

		if (!PassServerEntityFilter( pHandleEntity, m_pPassEnt ))
			return false;

		return true;
	}

private:
	IHandleEntity *m_pPassEnt;
};


//-----------------------------------------------------------------------------
// Drops an entity onto the floor
//-----------------------------------------------------------------------------
int UTIL_DropToFloor( CBaseEntity *pEntity, unsigned int mask)
{
	Assert( pEntity );

	trace_t	trace;
	UTIL_TraceEntity( pEntity, pEntity->GetAbsOrigin(), pEntity->GetAbsOrigin() - Vector(0,0,256), mask, &trace );

	if (trace.allsolid)
		return -1;

	if (trace.fraction == 1)
		return 0;

	pEntity->SetAbsOrigin( trace.endpos );
	UTIL_Relink( pEntity );
	pEntity->AddFlag( FL_ONGROUND );
	pEntity->SetGroundEntity( trace.m_pEnt );
	return 1;
}

//-----------------------------------------------------------------------------
// Returns false if any part of the bottom of the entity is off an edge that
// is not a staircase.
//-----------------------------------------------------------------------------
bool UTIL_CheckBottom( CBaseEntity *pEntity, ITraceFilter *pTraceFilter, float flStepSize )
{
	Vector	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;

	Assert( pEntity );

	CTracePassFilter traceFilter(pEntity);
	if ( !pTraceFilter )
	{
		pTraceFilter = &traceFilter;
	}

	unsigned int mask = pEntity->PhysicsSolidMaskForEntity();

	VectorAdd (pEntity->GetAbsOrigin(), pEntity->WorldAlignMins(), mins);
	VectorAdd (pEntity->GetAbsOrigin(), pEntity->WorldAlignMaxs(), maxs);

	// if all of the points under the corners are solid world, don't bother
	// with the tougher checks
	// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
	{
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (enginetrace->GetPointContents(start) != CONTENTS_SOLID)
				goto realcheck;
		}
	}
	return true;		// we got out easy

realcheck:
	// check it for real...
	start[2] = mins[2] + flStepSize; // seems to help going up/down slopes.
	
	// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2*flStepSize;
	
	UTIL_TraceLine( start, stop, mask, pTraceFilter, &trace );

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

	// the corners must be within 16 of the midpoint	
	for	(x=0 ; x<=1 ; x++)
	{
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];
			
			UTIL_TraceLine( start, stop, mask, pTraceFilter, &trace );
			
			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > flStepSize)
				return false;
		}
	}
	return true;
}



bool g_bDisableEhandleAccess = false;
bool g_bReceivedChainedUpdateOnRemove = false;
//-----------------------------------------------------------------------------
// Purpose: Sets the entity up for deletion.  Entity will not actually be deleted
//			until the next frame, so there can be no pointer errors.
// Input  : *oldObj - object to delete
//-----------------------------------------------------------------------------
void UTIL_Remove( IServerNetworkable *oldObj )
{
	if ( !oldObj || (oldObj->GetEFlags() & EFL_KILLME) )
		return;

	// mark it for deletion	
	oldObj->SetEFlags( oldObj->GetEFlags() | EFL_KILLME );

	CBaseEntity *pBaseEnt = oldObj->GetBaseEntity();
	if ( pBaseEnt )
	{
		g_bReceivedChainedUpdateOnRemove = false;
		pBaseEnt->UpdateOnRemove();

		Assert( g_bReceivedChainedUpdateOnRemove );

		// clear oldObj targetname / other flags now
		pBaseEnt->SetName( NULL_STRING );
	}

	gEntList.AddToDeleteList( oldObj );
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entity, without any delay.  WARNING! Only use this when sure
//			no pointers rely on this entity.
// Input  : *oldObj - the entity to delete
//-----------------------------------------------------------------------------
void UTIL_RemoveImmediate( CBaseEntity *oldObj )
{
	// valid pointer or already removed?
	if ( !oldObj || oldObj->IsEFlagSet(EFL_KILLME) )
		return;

	oldObj->AddEFlags( EFL_KILLME );	// Make sure to ignore further calls into here or UTIL_Remove.

	g_bReceivedChainedUpdateOnRemove = false;
	oldObj->UpdateOnRemove();
	Assert( g_bReceivedChainedUpdateOnRemove );

	// Entities shouldn't reference other entities in their destructors
	//  that type of code should only occur in an UpdateOnRemove call
	g_bDisableEhandleAccess = true;
	delete oldObj;
	g_bDisableEhandleAccess = false;
}


// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns NULL
// Index is 1 based
CBasePlayer	*UTIL_PlayerByIndex( int playerIndex )
{
	CBasePlayer *pPlayer = NULL;

	if ( playerIndex > 0 && playerIndex <= gpGlobals->maxClients )
	{
		edict_t *pPlayerEdict = INDEXENT( playerIndex );
		if ( pPlayerEdict && !pPlayerEdict->free )
		{
			pPlayer = (CBasePlayer*)GetContainingEntity( pPlayerEdict );
		}
	}
	
	return pPlayer;
}

int ENTINDEX( CBaseEntity *pEnt )
{
	// This works just like ENTINDEX for edicts.
	if ( pEnt )
		return pEnt->entindex();
	else
		return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : playerIndex - 
//			ping - 
//			packetloss - 
//-----------------------------------------------------------------------------
void UTIL_GetPlayerConnectionInfo( int playerIndex, int& ping, int &packetloss )
{
	engine->GetPlayerConnectionInfo( playerIndex, ping, packetloss );
}

static unsigned short FixedUnsigned16( float value, float scale )
{
	int output;

	output = value * scale;
	if ( output < 0 )
		output = 0;
	if ( output > 0xFFFF )
		output = 0xFFFF;

	return (unsigned short)output;
}


//-----------------------------------------------------------------------------
// Compute shake amplitude
//-----------------------------------------------------------------------------
inline float ComputeShakeAmplitude( const Vector &center, const Vector &shakePt, float amplitude, float radius ) 
{
	if ( radius <= 0 )
		return amplitude;

	float localAmplitude = -1;
	Vector delta = center - shakePt;
	float distance = delta.Length();

	if ( distance <= radius )
	{
		// Make the amplitude fall off over distance
		float flPerc = 1.0 - (distance / radius);
		localAmplitude = amplitude * flPerc;
	}

	return localAmplitude;
}


//-----------------------------------------------------------------------------
// Transmits the actual shake event
//-----------------------------------------------------------------------------
inline void TransmitShakeEvent( CBasePlayer *pPlayer, float localAmplitude, float frequency, float duration, ShakeCommand_t eCommand )
{
	if (( localAmplitude > 0 ) || ( eCommand == SHAKE_STOP ))
	{
		if ( eCommand == SHAKE_STOP )
			localAmplitude = 0;

		CSingleUserRecipientFilter user( pPlayer );
		user.MakeReliable();
		UserMessageBegin( user, "Shake" );
			WRITE_BYTE( eCommand );				// shake command (SHAKE_START, STOP, FREQUENCY, AMPLITUDE)
			WRITE_FLOAT( localAmplitude );			// shake magnitude/amplitude
			WRITE_FLOAT( frequency );				// shake noise frequency
			WRITE_FLOAT( duration );				// shake lasts this long
		MessageEnd();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shake the screen of all clients within radius.
//			radius == 0, shake all clients
// UNDONE: Allow caller to shake clients not ONGROUND?
// UNDONE: Fix falloff model (disabled)?
// UNDONE: Affect user controls?
// Input  : center - Center of screen shake, radius is measured from here.
//			amplitude - Amplitude of shake
//			frequency - 
//			duration - duration of shake in seconds.
//			radius - Radius of effect, 0 shakes all clients.
//			command - One of the following values:
//				SHAKE_START - starts the screen shake for all players within the radius
//				SHAKE_STOP - stops the screen shake for all players within the radius
//				SHAKE_AMPLITUDE - modifies the amplitude of the screen shake
//									for all players within the radius
//				SHAKE_FREQUENCY - modifies the frequency of the screen shake
//									for all players within the radius
//              bAirShake       - if this is false, then it will only shake players standing on the ground.
//-----------------------------------------------------------------------------
void UTIL_ScreenShake( const Vector &center, float amplitude, float frequency, float duration, float radius, ShakeCommand_t eCommand, bool bAirShake )
{
	int			i;
	float		localAmplitude;

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		//
		// Only shake players that are on the ground.
		//
		if ( !pPlayer || (!bAirShake && !(pPlayer->GetFlags() & FL_ONGROUND)) )
		{
			continue;
		}

		localAmplitude = ComputeShakeAmplitude( center, pPlayer->WorldSpaceCenter(), amplitude, radius );

		// This happens if the player is outside the radius, in which case we should ignore 
		// all commands
		if (localAmplitude < 0)
			continue;

		TransmitShakeEvent( (CBasePlayer *)pPlayer, localAmplitude, frequency, duration, eCommand );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Shake an object and all players on or near it
//-----------------------------------------------------------------------------
void UTIL_ScreenShakeObject( CBaseEntity *pEnt, const Vector &center, float amplitude, float frequency, float duration, float radius, ShakeCommand_t eCommand, bool bAirShake )
{
	int			i;
	float		localAmplitude;

	CBaseEntity *pHighestParent = GetHighestParent( pEnt );
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if (!pPlayer)
			continue;

		// Shake the object, or anything hierarchically attached to it at maximum amplitude
		localAmplitude = 0;
		if (pHighestParent == GetHighestParent(pPlayer))
		{
			localAmplitude = amplitude;
		}
		else if ((pPlayer->GetFlags() & FL_ONGROUND) && (GetHighestParent(pPlayer->GetGroundEntity()) == pHighestParent))
		{
			// If the player is standing on the object, use maximum amplitude
			localAmplitude = amplitude;
		}
		else
		{
			// Only shake players that are on the ground.
			if ( !bAirShake && !(pPlayer->GetFlags() & FL_ONGROUND) )
			{
				continue;
			}

			localAmplitude = ComputeShakeAmplitude( center, pPlayer->WorldSpaceCenter(), amplitude, radius );

			// This happens if the player is outside the radius, 
			// in which case we should ignore all commands
			if (localAmplitude < 0)
				continue;
		}

		TransmitShakeEvent( (CBasePlayer *)pPlayer, localAmplitude, frequency, duration, eCommand );
	}
}


void UTIL_ScreenFadeBuild( ScreenFade_t &fade, const color32 &color, float fadeTime, float fadeHold, int flags )
{
	fade.duration = FixedUnsigned16( fadeTime, 1<<SCREENFADE_FRACBITS );		// 7.9 fixed
	fade.holdTime = FixedUnsigned16( fadeHold, 1<<SCREENFADE_FRACBITS );		// 7.9 fixed
	fade.r = color.r;
	fade.g = color.g;
	fade.b = color.b;
	fade.a = color.a;
	fade.fadeFlags = flags;
}


void UTIL_ScreenFadeWrite( const ScreenFade_t &fade, CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsNetClient() )
		return;

	CSingleUserRecipientFilter user( (CBasePlayer *)pEntity );
	user.MakeReliable();

	UserMessageBegin( user, "Fade" );		// use the magic #1 for "one client"
		WRITE_SHORT( fade.duration );		// fade lasts this long
		WRITE_SHORT( fade.holdTime );		// fade lasts this long
		WRITE_SHORT( fade.fadeFlags );		// fade type (in / out)
		WRITE_BYTE( fade.r );				// fade red
		WRITE_BYTE( fade.g );				// fade green
		WRITE_BYTE( fade.b );				// fade blue
		WRITE_BYTE( fade.a );				// fade blue
	MessageEnd();
}


void UTIL_ScreenFadeAll( const color32 &color, float fadeTime, float fadeHold, int flags )
{
	int			i;
	ScreenFade_t	fade;


	UTIL_ScreenFadeBuild( fade, color, fadeTime, fadeHold, flags );

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
	
		UTIL_ScreenFadeWrite( fade, pPlayer );
	}
}


void UTIL_ScreenFade( CBaseEntity *pEntity, const color32 &color, float fadeTime, float fadeHold, int flags )
{
	ScreenFade_t	fade;

	UTIL_ScreenFadeBuild( fade, color, fadeTime, fadeHold, flags );
	UTIL_ScreenFadeWrite( fade, pEntity );
}


void UTIL_HudMessage( CBaseEntity *pEntity, const hudtextparms_t &textparms, const char *pMessage )
{
	if ( !pEntity || !pEntity->IsNetClient() )
		return;

	CSingleUserRecipientFilter user( (CBasePlayer *)pEntity );
	user.MakeReliable();
	te->TextMessage( user, 0.0, &textparms, pMessage );
}

void UTIL_HudMessageAll( const hudtextparms_t &textparms, const char *pMessage )
{
	int			i;

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
			UTIL_HudMessage( pPlayer, textparms, pMessage );
	}
}

					 
void UTIL_ClientPrintAll( int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	CRelieableBroadcastRecipientFilter filter;

	UserMessageBegin( filter, "TextMsg" );
		WRITE_BYTE( msg_dest );
		WRITE_STRING( msg_name );

		if ( param1 )
			WRITE_STRING( param1 );
		else
			WRITE_STRING( "" );

		if ( param2 )
			WRITE_STRING( param2 );
		else
			WRITE_STRING( "" );

		if ( param3 )
			WRITE_STRING( param3 );
		else
			WRITE_STRING( "" );

		if ( param4 )
			WRITE_STRING( param4 );
		else
			WRITE_STRING( "" );

	MessageEnd();
}

void ClientPrint( CBasePlayer *player, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	if ( !player )
		return;

	CSingleUserRecipientFilter user( player );
	user.MakeReliable();

	UserMessageBegin( user, "TextMsg" );
		WRITE_BYTE( msg_dest );
		WRITE_STRING( msg_name );

		if ( param1 )
			WRITE_STRING( param1 );
		else
			WRITE_STRING( "" );

		if ( param2 )
			WRITE_STRING( param2 );
		else
			WRITE_STRING( "" );

		if ( param3 )
			WRITE_STRING( param3 );
		else
			WRITE_STRING( "" );

		if ( param4 )
			WRITE_STRING( param4 );
		else
			WRITE_STRING( "" );

	MessageEnd();
}

void UTIL_SayText( const char *pText, CBaseEntity *pEntity )
{
	if ( !pEntity->IsNetClient() )
		return;

	CSingleUserRecipientFilter user( (CBasePlayer *)pEntity );
	user.MakeReliable();

	UserMessageBegin( user, "SayText" );
		WRITE_BYTE( pEntity->entindex() );
		WRITE_STRING( pText );
	MessageEnd();
}

void UTIL_SayTextAll( const char *pText, CBaseEntity *pEntity )
{
	CRelieableBroadcastRecipientFilter filter;

	UserMessageBegin( filter, "SayText" );
		WRITE_BYTE( pEntity->entindex() );
		WRITE_STRING( pText );
	MessageEnd();
}


void UTIL_ShowMessage( const char *pString, CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsNetClient() )
		return;

	CSingleUserRecipientFilter user( (CBasePlayer *)pEntity );
	user.MakeReliable();
	UserMessageBegin( user, "HudText" );
		WRITE_STRING( pString );
	MessageEnd();
}


void UTIL_ShowMessageAll( const char *pString )
{
	int		i;

	// loop through all players

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
			UTIL_ShowMessage( pString, pPlayer );
	}
}

// So we always return a valid surface
static csurface_t	g_NullSurface = { "**empty**", 0 };

void UTIL_SetTrace(trace_t& trace, const Ray_t &ray, edict_t *ent, float fraction, 
				   int hitgroup, unsigned int contents, const Vector& normal, float intercept )
{
	trace.startsolid = (fraction == 0.0f);
	trace.fraction = fraction;
	VectorCopy( ray.m_Start, trace.startpos );
	VectorMA( ray.m_Start, fraction, ray.m_Delta, trace.endpos );
	VectorCopy( normal, trace.plane.normal );
	trace.plane.dist = intercept;
	trace.m_pEnt = CBaseEntity::Instance( ent );
	trace.hitgroup = hitgroup;
	trace.surface =	g_NullSurface;
	trace.contents = contents;
}

void UTIL_ClearTrace( trace_t &trace )
{
	memset( &trace, 0, sizeof(trace));
	trace.fraction = 1.f;
	trace.fractionleftsolid = 0;
	trace.surface = g_NullSurface;
}

	

//-----------------------------------------------------------------------------
// Sets the entity size
//-----------------------------------------------------------------------------
static void SetMinMaxSize (CBaseEntity *pEnt, const Vector& mins, const Vector& maxs, bool rotate)
{
	for (int i=0 ; i<3 ; i++)
	{
		if (mins[i] > maxs[i])
		{
			Error("backwards mins/maxs");
		}
	}

	Assert( pEnt );

	pEnt->SetCollisionBounds( mins, maxs );

	engine->RelinkEntity(pEnt->edict(), false);
}


//-----------------------------------------------------------------------------
// Sets the model size
//-----------------------------------------------------------------------------
void UTIL_SetSize( CBaseEntity *pEnt, const Vector &vecMin, const Vector &vecMax )
{
	SetMinMaxSize (pEnt, vecMin, vecMax, false);
}

	
//-----------------------------------------------------------------------------
// Sets the model to be associated with an entity
//-----------------------------------------------------------------------------
void UTIL_SetModel( CBaseEntity *pEntity, const char *pModelName )
{
	// check to see if model was properly precached
	int i = modelinfo->GetModelIndex( pModelName );
	if ( i < 0 )
	{
		Error("no precache: %s\n", pModelName);
	}

	pEntity->SetModelIndex( i ) ;
	pEntity->SetModelName( MAKE_STRING( pModelName ) );

	// brush model
	const model_t *mod = modelinfo->GetModel( i );
	if ( mod )
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds( mod, mins, maxs );
		SetMinMaxSize (pEntity, mins, maxs, true);
	}
	else
	{
		SetMinMaxSize (pEntity, vec3_origin, vec3_origin, true);
	}
}

	
void UTIL_SetOrigin( CBaseEntity *entity, const Vector &vecOrigin, bool bFireTriggers )
{
	entity->SetLocalOrigin( vecOrigin );
	engine->RelinkEntity( entity->edict(), bFireTriggers );
}


//-----------------------------------------------------------------------------
// Purpose: Relink object in kd-tree (call after solid value changes)
// Input  : edict_t - Pointer to an edict_t
// Output : void
//-----------------------------------------------------------------------------
void UTIL_Relink( CBaseEntity *entity )
{
	entity->Relink();
}

void UTIL_ParticleEffect( const Vector &vecOrigin, const Vector &vecDirection, ULONG ulColor, ULONG ulCount )
{
	Msg( "UTIL_ParticleEffect:  Disabled\n" );
}

void UTIL_Smoke( const Vector &origin, const float scale, const float framerate )
{
	g_pEffects->Smoke( origin, g_sModelIndexSmoke, scale, framerate );
}

// snaps a vector to the nearest axis vector (if within epsilon)
void UTIL_SnapDirectionToAxis( Vector &direction, float epsilon )
{
	float proj = 1 - epsilon;
	for ( int i = 0; i < 3; i ++ )
	{
		if ( fabs(direction[i]) > proj )
		{
			// snap to axis unit vector
			if ( direction[i] < 0 )
				direction[i] = -1.0f;
			else
				direction[i] = 1.0f;
			direction[(i+1)%3] = 0;
			direction[(i+2)%3] = 0;
			return;
		}
	}
}

char *UTIL_VarArgs( char *format, ... )
{
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, format);
	Q_vsnprintf(string, sizeof(string), format,argptr);
	va_end (argptr);

	return string;	
}

bool UTIL_IsMasterTriggered(string_t sMaster, CBaseEntity *pActivator)
{
	if (sMaster != NULL_STRING)
	{
		CBaseEntity *pMaster = gEntList.FindEntityByName( NULL, sMaster, pActivator );
	
		if ( pMaster && (pMaster->ObjectCaps() & FCAP_MASTER) )
		{
			return pMaster->IsTriggered( pActivator );
		}

		Warning( "Master was null or not a master!\n");
	}

	// if this isn't a master entity, just say yes.
	return true;
}

bool UTIL_ShouldShowBlood( int color )
{
	if ( color != DONT_BLEED )
	{
		if ( color == BLOOD_COLOR_RED )
		{
			ConVar const *hblood = cvar->FindVar( "violence_hblood" );
			if ( hblood && hblood->GetInt() != 0 )
			{	
				return true;
			}
		}
		else
		{
			ConVar const *ablood = cvar->FindVar( "violence_ablood" );
			if ( ablood && ablood->GetInt() != 0 )
			{
				return true;
			}
		}
	}
	return false;
}

void UTIL_BloodStream( const Vector &origin, const Vector &direction, int color, int amount )
{
	if ( !UTIL_ShouldShowBlood( color ) )
		return;

	if ( g_Language == LANGUAGE_GERMAN && color == BLOOD_COLOR_RED )
		color = 0;

	CPVSFilter filter( origin );
	te->BloodStream( filter, 0.0, &origin, &direction, 247, 63, 14, 255, min( amount, 255 ) );
}				


void UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount )
{
	if ( !UTIL_ShouldShowBlood( color ) )
		return;

	if ( color == DONT_BLEED || amount == 0 )
		return;

	if ( g_Language == LANGUAGE_GERMAN && color == BLOOD_COLOR_RED )
		color = 0;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// scale up blood effect in multiplayer for better visibility
		amount *= 5;
	}

	if ( amount > 255 )
		amount = 255;

	if (color == BLOOD_COLOR_MECH)
	{
		g_pEffects->Sparks(origin);
		if (random->RandomFloat(0, 2) >= 1)
		{
			UTIL_Smoke(origin, random->RandomInt(10, 15), 10);
		}
	}
	else
	{
		int	r, g, b;

		switch ( color )
		{
		default:
		case BLOOD_COLOR_RED:
			if ( g_pGameRules->IsMultiplayer() )
			{
				r = 255;
				g = 32;
				b = 32;
			}
			else
			{
				r = 64;
				g = 0;
				b = 0;
			}
			break;

		case BLOOD_COLOR_YELLOW:
			r = 128;
			g = 128;
			b = 0;
			break;
		}

		CPVSFilter filter( origin );
		te->BloodSprite( filter, 0.0, &origin, &direction, r, g, b, 255, min( max( 3, amount / 10 ), 16 ) );
	}
}				



Vector UTIL_RandomBloodVector( void )
{
	Vector direction;

	direction.x = random->RandomFloat ( -1, 1 );
	direction.y = random->RandomFloat ( -1, 1 );
	direction.z = random->RandomFloat ( 0, 1 );

	return direction;
}


void UTIL_BloodDecalTrace( trace_t *pTrace, int bloodColor )
{
	if ( UTIL_ShouldShowBlood( bloodColor ) )
	{
		if ( bloodColor == BLOOD_COLOR_RED )
		{
			UTIL_DecalTrace( pTrace, "Blood" );
		}
		else
		{
			UTIL_DecalTrace( pTrace, "YellowBlood" );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose : Use trace to pass a specific decal type to the entity being decaled
// Input   :
// Output  :
//------------------------------------------------------------------------------
void UTIL_DecalTrace( trace_t *pTrace, char const *decalName )
{
	if (pTrace->fraction == 1.0)
		return;

	CBaseEntity *pEntity = pTrace->m_pEnt;
	pEntity->DecalTrace( pTrace, decalName );
}

//-----------------------------------------------------------------------------
// Purpose: Make a tracer effect
//-----------------------------------------------------------------------------
void UTIL_Tracer( const Vector &vecStart, const Vector &vecEnd, int iEntIndex, int iAttachment, float flVelocity, bool bWhiz, char *pCustomTracerName )
{
	CEffectData data;
	data.m_vStart = vecStart;
	data.m_vOrigin = vecEnd;
	data.m_nEntIndex = iEntIndex;
	data.m_flScale = flVelocity;

	// Flags
	if ( bWhiz )
	{
		data.m_fFlags |= TRACER_FLAG_WHIZ;
	}
	if ( iAttachment != TRACER_DONT_USE_ATTACHMENT )
	{
		data.m_fFlags |= TRACER_FLAG_USEATTACHMENT;
		// Stomp the start, since it's not going to be used anyway
		data.m_vStart[0] = iAttachment;
	}

	// Fire it off
	if ( pCustomTracerName )
	{
		DispatchEffect( pCustomTracerName, data );
	}
	else
	{
		DispatchEffect( "Tracer", data );
	}
}

//------------------------------------------------------------------------------
// Purpose : Creates both an decal and any associated impact effects (such
//			 as flecks) for the given iDamageType and the trace's end position
// Input   :
// Output  :
//------------------------------------------------------------------------------
void UTIL_ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
{
	CBaseEntity *pEntity = pTrace->m_pEnt;

	// Is the entity valid, is the surface sky?
	if ( !pEntity || !UTIL_IsValidEntity( pEntity ) || (pTrace->surface.flags & SURF_SKY) )
		return;

	if ( pTrace->fraction == 1.0 )
		return;

	pEntity->ImpactTrace( pTrace, iDamageType, pCustomImpactName );
}

/*
==============
UTIL_PlayerDecalTrace

A player is trying to apply his custom decal for the spray can.
Tell connected clients to display it, or use the default spray can decal
if the custom can't be loaded.
==============
*/
void UTIL_PlayerDecalTrace( trace_t *pTrace, int playernum )
{
	if (pTrace->fraction == 1.0)
		return;

	CBroadcastRecipientFilter filter;

	te->PlayerDecal( filter, 0.0,
		&pTrace->endpos, playernum, pTrace->m_pEnt->entindex() );
}

bool UTIL_TeamsMatch( const char *pTeamName1, const char *pTeamName2 )
{
	// Everyone matches unless it's teamplay
	if ( !g_pGameRules->IsTeamplay() )
		return true;

	// Both on a team?
	if ( *pTeamName1 != 0 && *pTeamName2 != 0 )
	{
		if ( !stricmp( pTeamName1, pTeamName2 ) )	// Same Team?
			return true;
	}

	return false;
}


void UTIL_StringToFloatArray( float *pVector, int count, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	Q_strncpy( tempString, pString, sizeof(tempString) );
	pstr = pfront = tempString;

	for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atof( pfront );

		// skip any leading whitespace
		while ( *pstr && *pstr <= ' ' )
			pstr++;

		// skip to next whitespace
		while ( *pstr && *pstr > ' ' )
			pstr++;

		if (!*pstr)
			break;

		pstr++;
		pfront = pstr;
	}
	for ( j++; j < count; j++ )
	{
		pVector[j] = 0;
	}
}

void UTIL_StringToVector( float *pVector, const char *pString )
{
	UTIL_StringToFloatArray( pVector, 3, pString );
}


void UTIL_AxisStringToPointPoint( Vector &start, Vector &end, const char *pString )
{
	char tmpstr[256];

	Q_strncpy( tmpstr, pString, sizeof(tmpstr) );
	char *pVec = strtok( tmpstr, "," );
	int i = 0;
	while ( pVec != NULL && *pVec )
	{
		if ( i == 0 )
		{
			UTIL_StringToVector( start.Base(), pVec );
			i++;
		}
		else
		{
			UTIL_StringToVector( end.Base(), pVec );
		}
		pVec = strtok( NULL, "," );
	}
}

void UTIL_AxisStringToPointDir( Vector &start, Vector &dir, const char *pString )
{
	Vector end;
	UTIL_AxisStringToPointPoint( start, end, pString );
	dir = end - start;
	VectorNormalize(dir);
}

void UTIL_AxisStringToUnitDir( Vector &dir, const char *pString )
{
	Vector start;
	UTIL_AxisStringToPointDir( start, dir, pString );
}


void UTIL_StringToIntArray( int *pVector, int count, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	Q_strncpy( tempString, pString, sizeof(tempString) );
	pstr = pfront = tempString;

	for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atoi( pfront );

		while ( *pstr && *pstr != ' ' )
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	for ( j++; j < count; j++ )
	{
		pVector[j] = 0;
	}
}


void UTIL_StringToColor32( color32 *color, const char *pString )
{
	int tmp[4];
	UTIL_StringToIntArray( tmp, 4, pString );
	color->r = tmp[0];
	color->g = tmp[1];
	color->b = tmp[2];
	color->a = tmp[3];
}


/*
==================================================
UTIL_ClipPunchAngleOffset
==================================================
*/

void UTIL_ClipPunchAngleOffset( QAngle &in, const QAngle &punch, const QAngle &clip )
{
	QAngle	final = in + punch;

	//Clip each component
	for ( int i = 0; i < 3; i++ )
	{
		if ( final[i] > clip[i] )
		{
			final[i] = clip[i];
		}
		else if ( final[i] < -clip[i] )
		{
			final[i] = -clip[i];
		}

		//Return the result
		in[i] = final[i] - punch[i];
	}
}

float UTIL_WaterLevel( const Vector &position, float minz, float maxz )
{
	Vector midUp = position;
	midUp.z = minz;

	if ( !(UTIL_PointContents(midUp) & MASK_WATER) )
		return minz;

	midUp.z = maxz;
	if ( UTIL_PointContents(midUp) & MASK_WATER )
		return maxz;

	float diff = maxz - minz;
	while (diff > 1.0)
	{
		midUp.z = minz + diff/2.0;
		if ( UTIL_PointContents(midUp) & MASK_WATER )
		{
			minz = midUp.z;
		}
		else
		{
			maxz = midUp.z;
		}
		diff = maxz - minz;
	}

	return midUp.z;
}


extern short	g_sModelIndexBubbles;// holds the index for the bubbles model

void UTIL_Bubbles( const Vector& mins, const Vector& maxs, int count )
{
	Vector mid =  (mins + maxs) * 0.5;

	float flHeight = UTIL_WaterLevel( mid,  mid.z, mid.z + 1024 );
	flHeight = flHeight - mins.z;

	CPASFilter filter( mid );

	te->Bubbles( filter, 0.0,
		&mins, &maxs, flHeight, g_sModelIndexBubbles, count, 8.0 );
}

void UTIL_BubbleTrail( const Vector& from, const Vector& to, int count )
{
	float flHeight = UTIL_WaterLevel( from,  from.z, from.z + 256 );
	flHeight = flHeight - from.z;

	if (flHeight < 8)
	{
		flHeight = UTIL_WaterLevel( to,  to.z, to.z + 256 );
		flHeight = flHeight - to.z;
		if (flHeight < 8)
			return;

		// UNDONE: do a ploink sound
		flHeight = flHeight + to.z - from.z;
	}

	if (count > 255) 
		count = 255;

	CBroadcastRecipientFilter filter;

	te->BubbleTrail( filter, 0.0, &from, &to, flHeight, g_sModelIndexBubbles, count, 8.0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Start - 
//			End - 
//			ModelIndex - 
//			FrameStart - 
//			FrameRate - 
//			Life - 
//			Width - 
//			Noise - 
//			Red - 
//			Green - 
//			Brightness - 
//			Speed - 
//-----------------------------------------------------------------------------
void UTIL_Beam( Vector &Start, Vector &End, int nModelIndex, int nHaloIndex, unsigned char FrameStart, unsigned char FrameRate,
				float Life, unsigned char Width, unsigned char EndWidth, unsigned char FadeLength, unsigned char Noise, unsigned char Red, unsigned char Green,
				unsigned char Blue, unsigned char Brightness, unsigned char Speed)
{
	CBroadcastRecipientFilter filter;

	te->BeamPoints( filter, 0.0,
		&Start, 
		&End, 
		nModelIndex, 
		nHaloIndex, 
		FrameStart,
		FrameRate, 
		Life,
		Width,
		EndWidth,
		FadeLength,
		Noise,
		Red,
		Green,
		Blue,
		Brightness,
		Speed );
}

bool UTIL_IsValidEntity( CBaseEntity *pEnt )
{
	edict_t *pEdict = pEnt->edict();
	if ( !pEdict || pEdict->free )
		return false;
	return true;
}


//#define PRECACHE_OTHER_ONCE
// UNDONE: Do we need this to avoid doing too much of this?  Measure startup times and see
#if PRECACHE_OTHER_ONCE

#include "utlsymbol.h"
class CPrecacheOtherList : public CAutoGameSystem
{
public:
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity();

	bool AddOrMarkPrecached( const char *pClassname );

private:
	CUtlSymbolTable		m_list;
};

void CPrecacheOtherList::LevelInitPreEntity()
{
	m_list.RemoveAll();
}

void CPrecacheOtherList::LevelShutdownPostEntity()
{
	m_list.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: mark or add
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPrecacheOtherList::AddOrMarkPrecached( const char *pClassname )
{
	CUtlSymbol sym = m_list.Find( pClassname );
	if ( sym.IsValid() )
		return false;

	m_list.AddString( pClassname );
	return true;
}

CPrecacheOtherList g_PrecacheOtherList;
#endif

void UTIL_PrecacheOther( const char *szClassname )
{
#if PRECACHE_OTHER_ONCE
	// already done this one?, if not, mark as done
	if ( !g_PrecacheOtherList.AddOrMarkPrecached( szClassname ) )
		return;
#endif

	CBaseEntity	*pEntity = CreateEntityByName( szClassname );
	if ( !pEntity )
	{
		Warning( "NULL Ent in UTIL_PrecacheOther\n" );
		return;
	}
	
	if (pEntity)
		pEntity->Precache( );

	UTIL_RemoveImmediate( pEntity );
}

//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf( char *fmt, ... )
{
	va_list			argptr;
	static char		tempString[1024];
	
	va_start ( argptr, fmt );
	Q_vsnprintf( tempString, sizeof(tempString), fmt, argptr );
	va_end   ( argptr );

	// Print to server console
	engine->LogPrint( tempString );
}

//=========================================================
// UTIL_DotPoints - returns the dot product of a line from
// src to check and vecdir.
//=========================================================
float UTIL_DotPoints ( const Vector &vecSrc, const Vector &vecCheck, const Vector &vecDir )
{
	Vector2D	vec2LOS;

	vec2LOS = ( vecCheck - vecSrc ).AsVector2D();
	Vector2DNormalize( vec2LOS );

	return DotProduct2D(vec2LOS, vecDir.AsVector2D());
}


//=========================================================
// UTIL_StripToken - for redundant keynames
//=========================================================
void UTIL_StripToken( const char *pKey, char *pDest )
{
	int i = 0;

	while ( pKey[i] && pKey[i] != '#' )
	{
		pDest[i] = pKey[i];
		i++;
	}
	pDest[i] = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Implemented for mathlib.c error handling
// Input  : *error - 
//-----------------------------------------------------------------------------
extern "C" void Sys_Error( char *error, ... )
{
	va_list	argptr;
	static char	string[1024];
	
	va_start( argptr, error );
	Q_vsnprintf( string, sizeof(string), error, argptr );
	va_end( argptr );

	Warning( "%s", string );
	Assert(0);
}


//-----------------------------------------------------------------------------
// Purpose: Spawns an entity into the game, initializing it with the map ent data block
// Input  : *pEntity - the newly created entity
//			*mapData - pointer a block of entity map data
// Output : -1 if the entity was not successfully created; 0 on success
//-----------------------------------------------------------------------------
int DispatchSpawn( CBaseEntity *pEntity )
{
	if ( pEntity )
	{
		// keep a smart pointer that will now if the object gets deleted
		EHANDLE pEntSafe;
		pEntSafe = pEntity;

		// Initialize these or entities who don't link to the world won't have anything in here
		// is this necessary?
		//pEntity->SetAbsMins( pEntity->GetOrigin() - Vector(1,1,1) );
		//pEntity->SetAbsMaxs( pEntity->GetOrigin() + Vector(1,1,1) );

		pEntity->Spawn();
		// Try to get the pointer again, in case the spawn function deleted the entity.
		// UNDONE: Spawn() should really return a code to ask that the entity be deleted, but
		// that would touch too much code for me to do that right now.

		if ( pEntSafe == NULL || pEntity->IsMarkedForDeletion() )
			return -1;

		if ( pEntity->m_iGlobalname != NULL_STRING ) 
		{
			// Handle global stuff here
			int globalIndex = GlobalEntity_GetIndex( pEntity->m_iGlobalname );
			if ( globalIndex >= 0 )
			{
				// Already dead? delete
				if ( GlobalEntity_GetState(globalIndex) == GLOBAL_DEAD )
				{
					pEntity->Remove();
					return -1;
				}
				else if ( !FStrEq(STRING(gpGlobals->mapname), GlobalEntity_GetMap(globalIndex)) )
				{
					pEntity->MakeDormant();	// Hasn't been moved to this level yet, wait but stay alive
				}
				// In this level & not dead, continue on as normal
			}
			else
			{
				// Spawned entities default to 'On'
				GlobalEntity_Add( pEntity->m_iGlobalname, gpGlobals->mapname, GLOBAL_ON );
//					Msg( "Added global entity %s (%s)\n", pEntity->GetClassname(), STRING(pEntity->m_iGlobalname) );
			}
		}
		// just relink everyone after they spawn!
		pEntity->Relink();
	}

	return 0;
}

// UNDONE: This could be a better test - can we run the absbox through the bsp and see
// if it contains any solid space?  or would that eliminate some entities we want to keep?
int UTIL_EntityInSolid( CBaseEntity *ent )
{
	Vector	point;

	CBaseEntity *pParent = ent->GetMoveParent();
	// HACKHACK -- If you're attached to a client, always go through
	if ( pParent )
	{
		if ( pParent->IsPlayer() )
			return 0;

		ent = GetHighestParent( ent );
	}

	point = 0.5f * ( ent->GetAbsMins() + ent->GetAbsMaxs() );

	return ( enginetrace->GetPointContents( point ) & MASK_SOLID );
}


//-----------------------------------------------------------------------------
// Purpose: Initialize the matrix from an entity
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void EntityMatrix::InitFromEntity( CBaseEntity *pEntity, int iAttachment )
{
	if ( !pEntity )
	{
		Identity();
		return;
	}

	// Get an attachment's matrix?
	if ( iAttachment != 0 )
	{
		CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();
		if ( pAnimating )
		{
			Vector vOrigin;
			QAngle vAngles;
			if ( pAnimating->GetAttachment( iAttachment, vOrigin, vAngles ) )
			{
				((VMatrix *)this)->SetupMatrixOrgAngles( vOrigin, vAngles );
			}
		}
	}

	((VMatrix *)this)->SetupMatrixOrgAngles( pEntity->GetAbsOrigin(), pEntity->GetAbsAngles() );
}


void EntityMatrix::InitFromEntityLocal( CBaseEntity *entity )
{
	if ( !entity || !entity->edict() )
	{
		Identity();
		return;
	}
	((VMatrix *)this)->SetupMatrixOrgAngles( entity->GetLocalOrigin(), entity->GetLocalAngles() );
}

//==================================================
// Purpose: 
// Input: 
// Output: 
//==================================================

void UTIL_ValidateSoundName( string_t &name, const char *defaultStr )
{
	if ( ( !name || strlen( (char*) STRING( name ) ) < 1 ) )
	{
		name = AllocPooledString( defaultStr );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Slightly modified strtok. Does not modify the input string. Does
//			not skip over more than one separator at a time. This allows parsing
//			strings where tokens between separators may or may not be present:
//
//			Door01,,,0 would be parsed as "Door01"  ""  ""  "0"
//			Door01,Open,,0 would be parsed as "Door01"  "Open"  ""  "0"
//
// Input  : token - Returns with a token, or zero length if the token was missing.
//			str - String to parse.
//			sep - Character to use as separator. UNDONE: allow multiple separator chars
// Output : Returns a pointer to the next token to be parsed.
//-----------------------------------------------------------------------------
const char *nexttoken(char *token, const char *str, char sep)
{
	if ((str == NULL) || (*str == '\0'))
	{
		*token = '\0';
		return(NULL);
	}

	//
	// Copy everything up to the first separator into the return buffer.
	// Do not include separators in the return buffer.
	//
	while ((*str != sep) && (*str != '\0'))
	{
		*token++ = *str++;
	}
	*token = '\0';

	//
	// Advance the pointer unless we hit the end of the input string.
	//
	if (*str == '\0')
	{
		return(str);
	}

	return(++str);
}

//-----------------------------------------------------------------------------
// Purpose: Helper for UTIL_FindClientInPVS
// Input  : check - last checked client
// Output : static int UTIL_GetNewCheckClient
//-----------------------------------------------------------------------------
// FIXME:  include bspfile.h here?
class CCheckClient : public CAutoGameSystem
{
public:
	void LevelInitPreEntity()
	{
		m_checkCluster = -1;
		m_lastcheck = 1;
		m_lastchecktime = -1;
	}


	byte	m_checkPVS[MAX_MAP_LEAFS/8];
	int		m_checkCluster;
	int		m_lastcheck;
	float	m_lastchecktime;
};

CCheckClient g_CheckClient;


static int UTIL_GetNewCheckClient( int check )
{
	int		i;
	edict_t	*ent;
	Vector	org;

// cycle to the next one

	if (check < 1)
		check = 1;
	if (check > gpGlobals->maxClients)
		check = gpGlobals->maxClients;

	if (check == gpGlobals->maxClients)
		i = 1;
	else
		i = check + 1;

	for ( ;  ; i++)
	{
		if ( i > gpGlobals->maxClients )
		{
			i = 1;
		}

		ent = engine->PEntityOfEntIndex( i );
		if ( !ent )
			continue;

		// Looped but didn't find anything else
		if ( i == check )
			break;	

		if ( !ent->m_pEnt )
			continue;

		CBaseEntity *entity = GetContainingEntity( ent );
		if ( !entity )
			continue;

		if ( entity->GetFlags() & FL_NOTARGET )
			continue;

		// anything that is a client, or has a client as an enemy
		break;
	}

	if ( ent )
	{
		// get the PVS for the entity
		CBaseEntity *pce = GetContainingEntity( ent );
		if ( !pce )
			return i;

		org = pce->EyePosition();

		int clusterIndex = engine->GetClusterForOrigin( org );
		if ( clusterIndex != g_CheckClient.m_checkCluster )
		{
			g_CheckClient.m_checkCluster = clusterIndex;
			engine->GetPVSForCluster( clusterIndex, sizeof(g_CheckClient.m_checkPVS), g_CheckClient.m_checkPVS );
		}
	}
	
	return i;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a client (or object that has a client enemy) that would be a valid target.
//  If there are more than one valid options, they are cycled each frame
//  If (self.origin + self.viewofs) is not in the PVS of the current target, it is not returned at all.
// Input  : *pEdict - 
// Output : edict_t*
//-----------------------------------------------------------------------------
edict_t *UTIL_FindClientInPVS(edict_t *pEdict)
{
	edict_t	*ent;
	Vector	view;

// find a new check if on a new frame
	float delta = gpGlobals->curtime - g_CheckClient.m_lastchecktime;
	if ( delta >= 0.1 || delta < 0 )
	{
		g_CheckClient.m_lastcheck = UTIL_GetNewCheckClient( g_CheckClient.m_lastcheck );
		g_CheckClient.m_lastchecktime = gpGlobals->curtime;
	}

	// return check if it might be visible	
	ent = engine->PEntityOfEntIndex( g_CheckClient.m_lastcheck );

	// Allow dead clients -- JAY
	// Our monsters know the difference, and this function gates alot of behavior
	// It's annoying to die and see monsters stop thinking because you're no longer
	// "in" their PVS
	if ( !ent || ent->free || !ent->m_pEnt)
	{
		return NULL;
	}

	// if current entity can't possibly see the check entity, return 0
	// UNDONE: Build a box for this and do it over that box
	// UNDONE: Use CM_BoxLeafnums()
	CBaseEntity *pe = GetContainingEntity( pEdict );
	if ( pe )
	{
		view = pe->EyePosition();
		
		if ( !engine->CheckOriginInPVS( view, g_CheckClient.m_checkPVS ) )
		{
			return NULL;
		}
	}

	// might be able to see it
	return ent;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a chain of entities within the PVS of another entity (client)
//  starting_ent is the ent currently at in the list
//  a starting_ent of NULL signifies the beginning of a search
// Input  : *pplayer - 
//			*starting_ent - 
// Output : edict_t
//-----------------------------------------------------------------------------
CBaseEntity *UTIL_EntitiesInPVS( CBaseEntity *pPVSEntity, CBaseEntity *pStartingEntity )
{
	Vector			org;
	static byte		pvs[ MAX_MAP_CLUSTERS/8 ];
	static Vector	lastOrg( 0, 0, 0 );
	static int		lastCluster = -1;

	if ( !pPVSEntity )
		return NULL;

	// NOTE: These used to be caching code here to prevent this from
	// being called over+over which breaks when you go back + forth
	// across level transitions
	// So, we'll always get the PVS each time we start a new EntitiesInPVS iteration.
	// Given that weapon_binocs + leveltransition code is the only current clients
	// of this, this seems safe.
	if ( !pStartingEntity )
	{
		org = pPVSEntity->EyePosition();
		int clusterIndex = engine->GetClusterForOrigin( org );
		Assert( clusterIndex >= 0 );
		engine->GetPVSForCluster( clusterIndex, sizeof(pvs), pvs );
	}

	for ( CBaseEntity *pEntity = gEntList.NextEnt(pStartingEntity); pEntity; pEntity = gEntList.NextEnt(pEntity) )
	{
		// Only return attached ents.
		if ( !pEntity->edict() )
			continue;

		CBaseEntity *pParent = GetHighestParent( pEntity );

		if ( !engine->CheckBoxInPVS( pParent->GetAbsMins(), pParent->GetAbsMaxs(), pvs ) )
			continue;

		return pEntity;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get the predicted postion of an entity of a certain number of seconds
//			Use this function with caution, it has great potential for annoying the player, especially
//			if used for target firing predition
// Input  : *pTarget - target entity to predict
//			timeDelta - amount of time to predict ahead (in seconds)
//			&vecPredictedPosition - output
//-----------------------------------------------------------------------------
void UTIL_PredictedPosition( CBaseEntity *pTarget, float flTimeDelta, Vector *vecPredictedPosition )
{
	if ( ( pTarget == NULL ) || ( vecPredictedPosition == NULL ) )
		return;

	Vector	vecPredictedVel;

	//FIXME: Should we look at groundspeed or velocity for non-clients??

	//Get the proper velocity to predict with
	CBasePlayer	*pPlayer = ToBasePlayer( pTarget );

	//Player works differently than other entities
	if ( pPlayer != NULL )
	{
		if ( pPlayer->IsInAVehicle() )
		{
			//Calculate the predicted position in this vehicle
			vecPredictedVel = pPlayer->GetVehicle()->GetVehicleEnt()->GetSmoothedVelocity();
		}
		else
		{
			//Get the player's stored velocity
			vecPredictedVel = pPlayer->GetSmoothedVelocity();
		}
	}
	else
	{
		CBaseAnimating *pAnimating = (CBaseAnimating *) pTarget;

		if ( pAnimating == NULL )
		{
			Assert( pAnimating != NULL );
			(*vecPredictedPosition) = pTarget->GetAbsOrigin();
			return;
		}

		vecPredictedVel = pAnimating->GetGroundSpeedVelocity();
	}

	//Get the result
	(*vecPredictedPosition) = pTarget->GetAbsOrigin() + ( vecPredictedVel * flTimeDelta );
}

//-----------------------------------------------------------------------------
// Purpose: Points the destination entity at the target entity
// Input  : *pDest - entity to be pointed at the target
//			*pTarget - target to point at
//-----------------------------------------------------------------------------
bool UTIL_PointAtEntity( CBaseEntity *pDest, CBaseEntity *pTarget )
{
	if ( ( pDest == NULL ) || ( pTarget == NULL ) )
	{
		return false;
	}

	Vector dir = (pTarget->GetAbsOrigin() - pDest->GetAbsOrigin());

	VectorNormalize( dir );

	//Store off as angles
	QAngle angles;
	VectorAngles( dir, angles );
	pDest->SetLocalAngles( angles );
	pDest->SetAbsAngles( angles );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Points the destination entity at the target entity by name
// Input  : *pDest - entity to be pointed at the target
//			strTarget - name of entity to target (will only choose the first!)
//-----------------------------------------------------------------------------
void UTIL_PointAtNamedEntity( CBaseEntity *pDest, string_t strTarget )
{
	//Attempt to find the entity
	if ( !UTIL_PointAtEntity( pDest, gEntList.FindEntityByName( NULL, strTarget, NULL ) ) )
	{
		DevMsg( 1, "Unable to point at entity %s\n", STRING( strTarget ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make a muzzle flash appear
// Input  : &origin - position of the muzzle flash
//			&angles - angles of the fire direction
//			scale - scale of the muzzle flash
//			type - type of muzzle flash
//-----------------------------------------------------------------------------
void UTIL_MuzzleFlash( const Vector &origin, const QAngle &angles, int scale, int type )
{
	CPASFilter filter( origin );

	te->MuzzleFlash( filter, 0.0f, origin, angles, scale, type );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vStartPos - start of the line
//			vEndPos - end of the line
//			vPoint - point to find nearest point to on specified line
//			clampEnds - clamps returned points to being on the line segment specified
// Output : Vector - nearest point on the specified line
//-----------------------------------------------------------------------------
Vector UTIL_PointOnLineNearestPoint(const Vector& vStartPos, const Vector& vEndPos, const Vector& vPoint, bool clampEnds )
{
	Vector	vEndToStart		= (vEndPos - vStartPos);
	Vector	vOrgToStart		= (vPoint  - vStartPos);
	float	fNumerator		= DotProduct(vEndToStart,vOrgToStart);
	float	fDenominator	= vEndToStart.Length() * vOrgToStart.Length();
	float	fIntersectDist	= vOrgToStart.Length()*(fNumerator/fDenominator);
	float	flLineLength	= VectorNormalize( vEndToStart ); 
	
	if ( clampEnds )
	{
		fIntersectDist = clamp( fIntersectDist, 0.0f, flLineLength );
	}
	
	Vector	vIntersectPos	= vStartPos + vEndToStart * fIntersectDist;

	return vIntersectPos;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function get get determinisitc random values for shared/prediction code
// Input  : seedvalue - 
//			*module - 
//			line - 
// Output : static int
//-----------------------------------------------------------------------------
static int SeedFileLineHash( int seedvalue, const char *module, int line, int additionalSeed )
{
	CRC32_t retval;

	CRC32_Init( &retval );

	CRC32_ProcessBuffer( &retval, (void *)&seedvalue, sizeof( int ) );
	CRC32_ProcessBuffer( &retval, (void *)&additionalSeed, sizeof( int ) );
	CRC32_ProcessBuffer( &retval, (void *)module, Q_strlen( module ) );
	CRC32_ProcessBuffer( &retval, (void *)&line, sizeof( int ) );

	CRC32_Final( &retval );

	return (int)( retval );
}

//-----------------------------------------------------------------------------
// Purpose: Accessed by SHARED_RANDOMFLOAT macro to get c/s neutral random float for prediction
// Input  : *filename - 
//			line - 
//			flMinVal - 
//			flMaxVal - 
// Output : float
//-----------------------------------------------------------------------------
float SharedRandomFloat( const char *filename, int line, float flMinVal, float flMaxVal, int additionalSeed /*=0*/ )
{
	Assert( CBaseEntity::GetPredictionRandomSeed() != -1 );

	int seed = SeedFileLineHash( CBaseEntity::GetPredictionRandomSeed(), filename, line, additionalSeed );
	RandomSeed( seed );
	return RandomFloat( flMinVal, flMaxVal );
}

//-----------------------------------------------------------------------------
// Purpose: Accessed by SHARED_RANDOMINT macro to get c/s neutral random int for prediction
// Input  : *filename - 
//			line - 
//			iMinVal - 
//			iMaxVal - 
// Output : int
//-----------------------------------------------------------------------------
int SharedRandomInt( const char *filename, int line, int iMinVal, int iMaxVal, int additionalSeed /*=0*/ )
{
	Assert( CBaseEntity::GetPredictionRandomSeed() != -1 );

	int seed = SeedFileLineHash( CBaseEntity::GetPredictionRandomSeed(), filename, line, additionalSeed );
	RandomSeed( seed );
	return RandomInt( iMinVal, iMaxVal );
}

//-----------------------------------------------------------------------------
// Purpose: Accessed by SHARED_RANDOMVECTOR macro to get c/s neutral random Vector for prediction
// Input  : *filename - 
//			line - 
//			minVal - 
//			maxVal - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector SharedRandomVector( const char *filename, int line, float minVal, float maxVal, int additionalSeed /*=0*/ )
{
	Assert( CBaseEntity::GetPredictionRandomSeed() != -1 );

	int seed = SeedFileLineHash( CBaseEntity::GetPredictionRandomSeed(), filename, line, additionalSeed );
	RandomSeed( seed );
	// HACK:  Can't call RandomVector/Angle because it uses rand() not vstlib Random*() functions!
	// Get a random vector.
	Vector random;
	random.x = RandomFloat( minVal, maxVal );
	random.y = RandomFloat( minVal, maxVal );
	random.z = RandomFloat( minVal, maxVal );
	return random;
}

//-----------------------------------------------------------------------------
// Purpose: Accessed by SHARED_RANDOMANGLE macro to get c/s neutral random QAngle for prediction
// Input  : *filename - 
//			line - 
//			minVal - 
//			maxVal - 
// Output : QAngle
//-----------------------------------------------------------------------------
QAngle SharedRandomAngle( const char *filename, int line, float minVal, float maxVal, int additionalSeed /*=0*/ )
{
	Assert( CBaseEntity::GetPredictionRandomSeed() != -1 );

	int seed = SeedFileLineHash( CBaseEntity::GetPredictionRandomSeed(), filename, line, additionalSeed );
	RandomSeed( seed );
	// HACK:  Can't call RandomVector/Angle because it uses rand() not vstlib Random*() functions!
	// Get a random vector.
	Vector random;
	random.x = RandomFloat( minVal, maxVal );
	random.y = RandomFloat( minVal, maxVal );
	random.z = RandomFloat( minVal, maxVal );
	return QAngle( random.x, random.y, random.z );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
AngularImpulse WorldToLocalRotation( const VMatrix &localToWorld, const Vector &worldAxis, float rotation )
{
	// fix axes of rotation to match axes of vector
	Vector rot = worldAxis * rotation;
	// since the matrix maps local to world, do a transpose rotation to get world to local
	AngularImpulse ang = localToWorld.VMul3x3Transpose( rot );

	return ang;
}
