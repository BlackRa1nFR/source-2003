//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: The base class from which all game entities are derived.
//
//=============================================================================

#include "cbase.h"
#include "globalstate.h"
#include "isaverestore.h"
#include "client.h"
#include "decals.h"
#include "gamerules.h"
#include "entityapi.h"
#include "entitylist.h"
#include "entitymapdata.h"
#include "eventqueue.h"
#include "hierarchy.h"
#include "basecombatweapon.h"
#include "const.h"
#include "player.h"		// For debug draw sending
#include "ndebugoverlay.h"
#include "physics.h"
#include "model_types.h"
#include "team.h"
#include "sendproxy.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "basenetworkable.h"
#include "baseentity.h"
#include "collisionutils.h"
#include "coordsize.h"
#include "vstdlib/strtools.h"
#include "engine/IEngineSound.h"
#include "physics_saverestore.h"
#include "saverestore_utlvector.h"
#include "bone_setup.h"
#include "vcollide_parse.h"
#include "filters.h"
#include "te_effect_dispatch.h"
#include "AI_Criteria.h"
#include "AI_ResponseSystem.h"
#include "world.h"
#include "globals.h"
#include "saverestoretypes.h"
#include "skycamera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Init static class variables
bool			CBaseEntity::m_bInDebugSelect			= false;	// Used for selection in debug overlays
int				CBaseEntity::m_nDebugPlayer				= -1;		// Player doing the selection

bool CBaseEntity::m_bDebugPause = false;		// Whether entity i/o is paused.
int CBaseEntity::m_nDebugSteps = 1;				// Number of entity outputs to fire before pausing again.
int CBaseEntity::m_nPredictionRandomSeed = -1;
CBasePlayer *CBaseEntity::m_pPredictionPlayer = NULL;

ConVar sv_netvisdist( "sv_netvisdist", "10000", FCVAR_CHEAT, "Test networking visibility distance" );

class CPredictableList : public IPredictableList
{
public:

	virtual CBaseEntity *GetPredictable( int slot );
	virtual int GetPredictableCount( void );

protected:

	void AddEntity( CBaseEntity *add );
	void RemoveEntity( CBaseEntity *remove );

private:
	CUtlVector< EHANDLE >	m_Predictables;

	friend class CBaseEntity;
};

static CPredictableList g_Predictables;
IPredictableList *predictables = &g_Predictables;

void CPredictableList::AddEntity( CBaseEntity *add )
{
	EHANDLE test;
	test = add;

	if ( m_Predictables.Find( test ) != m_Predictables.InvalidIndex() )
		return;

	m_Predictables.AddToTail( test );
}

void CPredictableList::RemoveEntity( CBaseEntity *remove )
{
	EHANDLE test;
	test = remove;
	m_Predictables.FindAndRemove( test );
}

CBaseEntity *CPredictableList::GetPredictable( int slot )
{
	return m_Predictables[ slot ];
}

int CPredictableList::GetPredictableCount( void )
{
	return m_Predictables.Count();
}

// This table encodes edict data.
void SendProxy_AnimTime( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity *)pStruct;

#if defined( _DEBUG )
	CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();
	Assert( pAnimating );

	if ( pAnimating )
	{
		Assert( !pAnimating->IsUsingClientSideAnimation() );
	}
#endif
	
	int ticknumber = TIME_TO_TICKS( pEntity->m_flAnimTime );
	// Tickbase is current tick rounded down to closes 100 ticks
	int tickbase = 100 * (int)( gpGlobals->tickcount / 100 );
	int addt = 0;
	if ( ticknumber >= tickbase )
	{
		addt = ( ticknumber - tickbase ) & 0xFF;
	}

	pOut->m_Int = addt;
}

// This table encodes edict data.
void SendProxy_SimulationTime( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity *)pStruct;

	int ticknumber = TIME_TO_TICKS( pEntity->m_flSimulationTime );
	// Tickbase is current tick rounded down to closes 100 ticks
	int tickbase = 100 * (int)( gpGlobals->tickcount / 100 );
	int addt = 0;
	if ( ticknumber >= tickbase )
	{
		addt = ( ticknumber - tickbase ) & 0xFF;
	}

	pOut->m_Int = addt;
}

void* SendProxy_ClientSideAnimation( const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity *)pStruct;
	CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();

	if ( pAnimating && !pAnimating->IsUsingClientSideAnimation() )
		return (void*)pVarData;
	else
		return NULL;	// Don't send animtime unless the client needs it.
}	

void SendProxy_MoveType( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	COMPILE_TIME_ASSERT( MOVETYPE_LAST < (1 << MOVETYPE_MAX_BITS) );
	pOut->m_Int = ((CBaseEntity*)pStruct)->GetMoveType();
}

void SendProxy_MoveCollide( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	COMPILE_TIME_ASSERT( MOVECOLLIDE_COUNT < (1 << MOVECOLLIDE_MAX_BITS) );
	pOut->m_Int = ((CBaseEntity*)pStruct)->GetMoveCollide();
}


BEGIN_SEND_TABLE_NOBASE( CBaseEntity, DT_AnimTimeMustBeFirst )
	// NOTE:  Animtime must be sent before origin and angles ( from pev ) because it has a 
	//  proxy on the client that stores off the old values before writing in the new values and
	//  if it is sent after the new values, then it will only have the new origin and studio model, etc.
	//  interpolation will be busted
	SendPropInt		(SENDINFO(m_flAnimTime),	8, SPROP_UNSIGNED, SendProxy_AnimTime),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CBaseEntity, DT_PredictableId )
	SendPropPredictableId( SENDINFO( m_PredictableID ) ),
	SendPropInt( SENDINFO( m_bIsPlayerSimulated ), 1, SPROP_UNSIGNED ),
END_SEND_TABLE()

static void* SendProxy_SendPredictableId( const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity *)pStruct;
	if ( !pEntity || !pEntity->m_PredictableID->IsActive() )
		return NULL;

	int id_player_index = pEntity->m_PredictableID->GetPlayer();
	pRecipients->SetOnly( id_player_index );
	
	return ( void * )pVarData;
}

// This table encodes the CBaseEntity data.
IMPLEMENT_SERVERCLASS_ST_NOBASE( CBaseEntity, DT_BaseEntity )
	SendPropDataTable( "AnimTimeMustBeFirst", 0, &REFERENCE_SEND_TABLE(DT_AnimTimeMustBeFirst), SendProxy_ClientSideAnimation ),
	SendPropInt		(SENDINFO(m_flSimulationTime),	8, SPROP_UNSIGNED, SendProxy_SimulationTime),

	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD),

	SendPropModelIndex(SENDINFO(m_nModelIndex)),
	SendPropDataTable( SENDINFO_DT( m_Collision ), &REFERENCE_SEND_TABLE(DT_CollisionProperty) ),
	SendPropInt		(SENDINFO(m_nRenderFX),		8, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_nRenderMode),	8, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_fEffects),		EF_MAX_BITS, SPROP_UNSIGNED),
	SendPropInt		(SENDINFO(m_clrRender),	32, SPROP_UNSIGNED),
	SendPropInt		(SENDINFO(m_iTeamNum),		TEAMNUM_NUM_BITS, 0),
	SendPropInt		(SENDINFO(m_CollisionGroup), 5, SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO(m_flElasticity), 0, SPROP_COORD),
	SendPropFloat	(SENDINFO(m_flShadowCastDistance), 12, SPROP_UNSIGNED ),
	SendPropEHandle (SENDINFO(m_hOwnerEntity)),
	SendPropEHandle (SENDINFO_NAME(m_hMoveParent, moveparent)),
	SendPropInt		(SENDINFO(m_iParentAttachment), 6, SPROP_UNSIGNED),

	SendPropInt		("movetype", 0, SIZEOF_IGNORE, MOVETYPE_MAX_BITS, SPROP_UNSIGNED, SendProxy_MoveType ),
	SendPropInt		("movecollide", 0, SIZEOF_IGNORE, MOVECOLLIDE_MAX_BITS, SPROP_UNSIGNED, SendProxy_MoveCollide ),

	SendPropAngle	(SENDINFO_VECTORELEM(m_angRotation, 0), 13 ),
	SendPropAngle	(SENDINFO_VECTORELEM(m_angRotation, 1), 13 ),
	SendPropAngle	(SENDINFO_VECTORELEM(m_angRotation, 2), 13 ),

	SendPropInt		( SENDINFO( m_iTextureFrameIndex ),		8, SPROP_UNSIGNED ),

	SendPropDataTable( "predictable_id", 0, &REFERENCE_SEND_TABLE( DT_PredictableId ), SendProxy_SendPredictableId ),

	// FIXME: Collapse into another flag field?
	SendPropInt		(SENDINFO(m_bSimulatedEveryTick),		1, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_bAnimatedEveryTick),		1, SPROP_UNSIGNED ),

END_SEND_TABLE()



extern bool ParseKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue );

CBaseEntity::CBaseEntity( bool bServerOnly )
{
#ifdef _DEBUG
	// necessary since in debug, we initialize vectors to NAN for debugging
	m_vecAngVelocity.Init();
//	m_vecAbsAngVelocity.Init();
	m_vecViewOffset.Init();
	m_vecBaseVelocity.Init();
	m_vecVelocity.Init();
	m_vecAbsVelocity.Init();
#endif

	m_CollisionGroup = COLLISION_GROUP_NONE;
	m_iParentAttachment = 0;
	m_Collision.Init( this );

	// setup touch list
	m_EntitiesTouched.nextLink = m_EntitiesTouched.prevLink = &m_EntitiesTouched;

	// clear the edict
	pev = NULL;

	m_pTransmitProxy = NULL;

	// clear debug overlays
	m_debugOverlays  = 0;
	m_pTimedOverlay  = NULL;
	m_pPhysicsObject = NULL;
	m_flElasticity   = 1.0f;
	m_flShadowCastDistance = 0;
	SetRenderColor( 255, 255, 255, 255 );
	m_iTeamNum = m_iInitialTeamNum = 0;
	m_nLastThinkTick = gpGlobals->tickcount;
	m_nSimulationTick = -1;
	SetIdentityMatrix( m_rgflCoordinateFrame );
	m_pBlocker = NULL;

	m_iCurrentThinkContext = NO_THINK_CONTEXT;

	SetSolid( SOLID_NONE );
	ClearSolidFlags();

	SetMoveType( MOVETYPE_NONE );
	SetOwnerEntity( NULL );
	SetCheckUntouch( false );
	SetSentLastFrame( false );
	SetModelIndex( 0 );
	SetModelName( NULL_STRING );

	SetCollisionBounds( vec3_origin, vec3_origin );
	ClearFlags();

	SetFriction( 1.0f );

	if ( bServerOnly )
		m_iEFlags |= EFL_SERVER_ONLY;
}

extern bool g_bDisableEhandleAccess;

//-----------------------------------------------------------------------------
// Purpose: See note below
//-----------------------------------------------------------------------------
CBaseEntity::~CBaseEntity( )
{
	// Free our transmit proxy.
	if ( m_pTransmitProxy )
	{
		m_pTransmitProxy->Release();
	}

	// FIXME: This can't be called from UpdateOnRemove! There's at least one
	// case where friction sounds are added between the call to UpdateOnRemove + ~CBaseEntity
	PhysCleanupFrictionSounds( this );

	// In debug make sure that we don't call delete on an entity without setting
	//  the disable flag first!
	// EHANDLE accessors will check, in debug, for access to entities during destruction of
	//  another entity.
	// That kind of operation should only occur in UpdateOnRemove calls
	// Deletion should only occur via UTIL_Remove(Immediate) calls, not via naked delete calls
	Assert( g_bDisableEhandleAccess );

	VPhysicsDestroyObject();

	// Need to remove references to this entity before EHANDLES go null
	{
		g_bDisableEhandleAccess = false;
		CBaseEntity::PhysicsRemoveTouchedList( this );
		g_bDisableEhandleAccess = true;

		// Remove this entity from the ent list (NOTE:  This Makes EHANDLES go NULL)
		gEntList.RemoveEntity( GetRefEHandle() );
	}

	// remove the attached edict if it exists
	if ( pev )
	{
		// we're in the process of being deleted, so don't let the edict think it is still attached
		pev->m_pEnt = NULL;
		DetachEdict();
	}
}

BEGIN_PREDICTION_DATA_NO_BASE( CBaseEntity )

	DEFINE_FIELD( CBaseEntity, m_fEffects, FIELD_INTEGER ),
	// DEFINE_FIELD( CBaseEntity, m_pfnThink, void (CBaseEntity::*m_pfnThink ),
	// DEFINE_FIELD( CBaseEntity, m_pfnMoveDone, void (CBaseEntity::*m_pfnMoveDone ),
	// DEFINE_FIELD( CBaseEntity, m_iClassname, string_t ),
	// DEFINE_FIELD( CBaseEntity, m_iGlobalname, string_t ),
	// DEFINE_FIELD( CBaseEntity, m_iParent, string_t ),
	DEFINE_FIELD( CBaseEntity, m_iEFlags, FIELD_INTEGER ),
	// DEFINE_FIELD( CBaseEntity, m_pParent, EHANDLE ),
	DEFINE_FIELD( CBaseEntity, m_nWaterLevel, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_nWaterType, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_pBlocker, FIELD_EHANDLE ),
	// DEFINE_FIELD( CBaseEntity, m_pMoveParent, CBaseEntity ),
	// DEFINE_FIELD( CBaseEntity, m_pMoveChild, CBaseEntity ),
	// DEFINE_FIELD( CBaseEntity, m_pMovePeer, CBaseEntity ),
	DEFINE_FIELD( CBaseEntity, m_flLocalTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseEntity, m_flVPhysicsUpdateLocalTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseEntity, m_flMoveDoneTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseEntity, m_flGravity, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseEntity, m_flFriction, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseEntity, m_flSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseEntity, m_nRenderFX, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_nRenderMode, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_vecAngVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseEntity, m_flPrevAnimTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseEntity, m_flAnimTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseEntity, m_nLastThinkTick, FIELD_TICK ),
	DEFINE_FIELD( CBaseEntity, m_nNextThinkTick, FIELD_TICK ),
	DEFINE_FIELD( CBaseEntity, m_vecViewOffset, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseEntity, m_vecBaseVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseEntity, m_vecVelocity, FIELD_VECTOR ),
	DEFINE_PRED_TYPEDESCRIPTION( CBaseEntity, m_Collision, CCollisionProperty ),
	// DEFINE_FIELD( CBaseEntity, m_clrRender, color32 ),
	// DEFINE_FIELD( CBaseEntity, m_PredictableID, CPredictableId ),
	// DEFINE_FIELD( CBaseEntity, m_EntitiesTouched, touchlink_t ),
	// DEFINE_FIELD( CBaseEntity, touchStamp, FIELD_INTEGER ),
	// DEFINE_FIELD( CBaseEntity, m_NetStateMgr, CNetStateMgr ),
	// DEFINE_FIELD( CBaseEntity, pev, edict_t ),

	// DEFINE_FIELD( CBaseEntity, m_NetStateMgr, CNetStateMgr ),
	// DEFINE_FIELD( CBaseEntity, m_listentry, CSerialEntity ),
	// DEFINE_FIELD( CBaseEntity, CGlobalEntityList, friend class ),
	// DEFINE_FIELD( CBaseEntity, m_pfnTouch, void (CBaseEntity ::*m_pfnTouch ),
	// DEFINE_FIELD( CBaseEntity, m_pfnUse, void (CBaseEntity ::*m_pfnUse ),
	// DEFINE_FIELD( CBaseEntity, m_pfnBlocked, void (CBaseEntity ::*m_pfnBlocked ),
	// DEFINE_FIELD( CBaseEntity, CAI_Senses, friend class ),
	// DEFINE_FIELD( CBaseEntity, m_pLink, CBaseEntity ),
	DEFINE_FIELD( CBaseEntity, m_takedamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_lifeState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_spawnflags, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_iMaxHealth, FIELD_INTEGER ),
	// DEFINE_FIELD( CBaseEntity, m_target, string_t ),
	DEFINE_FIELD( CBaseEntity, m_iHealth, FIELD_INTEGER ),
	//DEFINE_FIELD( CBaseEntity, m_debugOverlays, FIELD_INTEGER ),
	// DEFINE_FIELD( CBaseEntity, m_pTimedOverlay, TimedOverlay_t* ),
	// DEFINE_FIELD( CBaseEntity, m_iName, string_t ),
	// DEFINE_FIELD( CBaseEntity, m_MoveType, MoveType_t ),
	// DEFINE_FIELD( CBaseEntity, m_MoveCollide, MoveCollide_t ),
	// DEFINE_FIELD( CBaseEntity, m_hOwnerEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseEntity, m_CollisionGroup, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_flElasticity, FIELD_FLOAT ),
	//DEFINE_FIELD( CBaseEntity, m_flShadowCastDistance, FIELD_FLOAT ),
	// DEFINE_FIELD( CBaseEntity, m_pPhysicsObject, IPhysicsObject ),
	DEFINE_FIELD( CBaseEntity, m_iInitialTeamNum, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_iTeamNum, FIELD_INTEGER ),
	// DEFINE_FIELD( CBaseEntity, m_hAimEnt, EHANDLE ),
	// DEFINE_FIELD( CBaseEntity, CCollisionEvent, friend class ),
	//							m_debugOverlays
	// DEFINE_FIELD( CBaseEntity, CDamageModifier, friend class ),
	// DEFINE_FIELD( CBaseEntity, m_DamageModifiers, CUtlLinkedList < CDamageModifier* , int > ),

END_PREDICTION_DATA()

void CBaseEntity::PostConstructor( const char *szClassname )
{
	if ( szClassname )
	{
		SetClassname(szClassname);
	}

	Assert( m_iClassname != NULL_STRING && STRING(m_iClassname) != NULL );

	// Possibly get an edict, and add self to global list of entites.
	if ( m_iEFlags & EFL_SERVER_ONLY )
	{
		gEntList.AddNonNetworkableEntity( this );
	}
	else
	{
		AttachEdict();
		
		// Some ents like the player override the AttachEdict function and do it at a different time.
		// While precaching, they don't ever have an edict, so we don't need to add them to
		// the entity list in that case.
		if ( pev )
			gEntList.AddNetworkableEntity( this, entindex() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles keys and outputs from the BSP.
// Input  : mapData - Text block of keys and values from the BSP.
//-----------------------------------------------------------------------------
void CBaseEntity::ParseMapData( CEntityMapData *mapData )
{
	char keyName[MAPKEY_MAXLENGTH];
	char value[MAPKEY_MAXLENGTH];

	#ifdef _DEBUG
	ValidateDataDescription();
	#endif // _DEBUG

	// loop through all keys in the data block and pass the info back into the object
	if ( mapData->GetFirstKey(keyName, value) )
	{
		do 
		{
			KeyValue( keyName, value );
		} 
		while ( mapData->GetNextKey(keyName, value) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Verifies that this entity's data description is valid in debug builds.
//-----------------------------------------------------------------------------
#ifdef _DEBUG
typedef CUtlVector< const char * >	KeyValueNameList_t;

static void AddDataMapFieldNamesToList( KeyValueNameList_t &list, datamap_t *pDataMap )
{
	while (pDataMap != NULL)
	{
		for (int i = 0; i < pDataMap->dataNumFields; i++)
		{
			typedescription_t *pField = &pDataMap->dataDesc[i];

			if (pField->fieldType == FIELD_EMBEDDED)
			{
				AddDataMapFieldNamesToList( list, pField->td );
				continue;
			}

			if (pField->flags & FTYPEDESC_KEY)
			{
				list.AddToTail( pField->externalName );
			}
		}
	
		pDataMap = pDataMap->baseMap;
	}
}

void CBaseEntity::ValidateDataDescription(void)
{
	// Multiple key fields that have the same name are not allowed - it creates an
	// ambiguity when trying to parse keyvalues and outputs.
	datamap_t *pDataMap = GetDataDescMap();
	if ((pDataMap == NULL) || pDataMap->bValidityChecked)
		return;

	pDataMap->bValidityChecked = true;

	// Let's generate a list of all keyvalue strings in the entire hierarchy...
	KeyValueNameList_t	names(128);
	AddDataMapFieldNamesToList( names, pDataMap );

	for (int i = names.Count(); --i > 0; )
	{
		for (int j = i - 1; --j >= 0; )
		{
			if (!stricmp(names[i], names[j]))
			{
				Msg( "%s has multiple data description entries for \"%s\"\n", STRING(m_iClassname), names[i]);
				break;
			}
		}
	}
}
#endif // _DEBUG


ConVar ent_debugkeys( "ent_debugkeys", "" );
void UTIL_StringToColor32( color32 *color, const char *pString );


//-----------------------------------------------------------------------------
// Parse data from a map file
//-----------------------------------------------------------------------------
bool CBaseEntity::KeyValue( const char *szKeyName, const char *szValue ) 
{
	//!! temp hack, until worldcraft is fixed
	// strip the # tokens from (duplicate) key names
	char *s = strchr( szKeyName, '#' );
	if ( s )
	{
		*s = '\0';
	}

	if ( szKeyName[0] == 'r' )
	{
		if ( FStrEq( szKeyName, "rendercolor" ) ||
			 FStrEq( szKeyName, "rendercolor32" ))
		{
			color32 tmp;
			UTIL_StringToColor32( &tmp, szValue );
			SetRenderColor( tmp.r, tmp.g, tmp.b );
			// don't copy alpha, legacy support uses renderamt
			return true;
		}
		else if ( FStrEq( szKeyName, "renderamt" ) )
		{
			SetRenderColorA( atoi( szValue ) );
			return true;
		}
	}
 
	if ( FStrEq( szKeyName, "disableshadows" ))
	{
		int val = atoi( szValue );
		if (val)
			m_fEffects |= EF_NOSHADOW;
		return true;
	}

	if ( FStrEq( szKeyName, "disablereceiveshadows" ))
	{
		int val = atoi( szValue );
		if (val)
			m_fEffects |= EF_NORECEIVESHADOW;
		return true;
	}

	// Fix up single angles
	if( FStrEq( szKeyName, "angle" ) )
	{
		static char szBuf[64];

		float y = atof( szValue );
		if (y >= 0)
		{
			Q_snprintf( szBuf,sizeof(szBuf), "%f %f %f", GetLocalAngles()[0], y, GetLocalAngles()[2] );
		}
		else if ((int)y == -1)
		{
			Q_strncpy( szBuf, "-90 0 0", sizeof(szBuf) );
		}
		else
		{
			Q_strncpy( szBuf, "90 0 0", sizeof(szBuf) );
		}

		// Do this so inherited classes looking for 'angles' don't have to bother with 'angle'
		return KeyValue( szKeyName, szBuf );
	}

	// NOTE: Have to do these separate because they set two values instead of one
	if( FStrEq( szKeyName, "angles" ) )
	{
		QAngle angles;
		UTIL_StringToVector( angles.Base(), szValue );

		// If you're hitting this assert, it's probably because you're
		// calling SetLocalAngles from within a KeyValues method.. use SetAbsAngles instead!
		Assert( (GetMoveParent() == NULL) && !IsEFlagSet( EFL_DIRTY_ABSTRANSFORM ) );
		SetAbsAngles( angles );
		return true;
	}

	if( FStrEq( szKeyName, "origin" ) )
	{
		Vector vecOrigin;
		UTIL_StringToVector( vecOrigin.Base(), szValue );

		// If you're hitting this assert, it's probably because you're
		// calling SetLocalOrigin from within a KeyValues method.. use SetAbsOrigin instead!
		Assert( (GetMoveParent() == NULL) && !IsEFlagSet( EFL_DIRTY_ABSTRANSFORM ) );
		SetAbsOrigin( vecOrigin );
		return true;
	}

	// loop through the data description, and try and place the keys in
	if ( !*ent_debugkeys.GetString() )
	{
		for ( datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
		{
			if ( ::ParseKeyvalue(this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
				return true;
		}
	}
	else
	{
		// debug version - can be used to see what keys have been parsed in
		bool printKeyHits = false;
		const char *debugName = "";

		if ( *ent_debugkeys.GetString() && !stricmp(ent_debugkeys.GetString(), STRING(m_iClassname)) )
		{
			// Msg( "-- found entity of type %s\n", STRING(m_iClassname) );
			printKeyHits = true;
			debugName = STRING(m_iClassname);
		}

		// loop through the data description, and try and place the keys in
		for ( datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
		{
			if ( !printKeyHits && *ent_debugkeys.GetString() && !stricmp(dmap->dataClassName, ent_debugkeys.GetString()) )
			{
				// Msg( "-- found class of type %s\n", dmap->dataClassName );
				printKeyHits = true;
				debugName = dmap->dataClassName;
			}

			if ( ::ParseKeyvalue(this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
			{
				if ( printKeyHits )
					Msg( "(%s) key: %-16s value: %s\n", debugName, szKeyName, szValue );
				
				return true;
			}
		}

		if ( printKeyHits )
			Msg( "!! (%s) key not handled: \"%s\" \"%s\"\n", STRING(m_iClassname), szKeyName, szValue );
	}

	// key hasn't been handled
	return false;
}

bool CBaseEntity::KeyValue( const char *szKeyName, float flValue ) 
{
	char	string[256];

	Q_snprintf(string,sizeof(string), "%f", flValue );

	return KeyValue( szKeyName, string );
}

bool CBaseEntity::KeyValue( const char *szKeyName, Vector vec ) 
{
	char	string[256];

	Q_snprintf(string,sizeof(string), "%f %f %f", vec.x, vec.y, vec.z );

	return KeyValue( szKeyName, string );
}

//-----------------------------------------------------------------------------
// Sets the collision bounds + the size
//-----------------------------------------------------------------------------
void CBaseEntity::SetCollisionBounds( const Vector& mins, const Vector &maxs )
{
	m_Collision.SetCollisionBounds( mins, maxs );
}


//-----------------------------------------------------------------------------
// These methods encapsulate MOVETYPE_FOLLOW, which became obsolete
//-----------------------------------------------------------------------------
void CBaseEntity::FollowEntity( CBaseEntity *pBaseEntity )
{
	if (pBaseEntity)
	{
		SetParent( pBaseEntity );
		SetMoveType( MOVETYPE_NONE );
		m_fEffects |= EF_BONEMERGE;
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetLocalOrigin( vec3_origin );
		SetLocalAngles( vec3_angle );
		Relink();
	}
	else
	{
		StopFollowingEntity();
	}
}

void CBaseEntity::StopFollowingEntity( )
{
	if( !IsFollowingEntity() )
	{
		Assert( (m_fEffects & EF_BONEMERGE) == 0 );
		return;
	}

	SetParent( NULL );
	m_fEffects &= ~EF_BONEMERGE;
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	Relink();
}

bool CBaseEntity::IsFollowingEntity()
{
	return (m_fEffects & EF_BONEMERGE) && (GetMoveType() == MOVETYPE_NONE) && GetMoveParent();
}

CBaseEntity *CBaseEntity::GetFollowedEntity()
{
	if (!IsFollowingEntity())
		return NULL;
	return GetMoveParent();
}


//-----------------------------------------------------------------------------
// Purpose: Attaches the game object to a networkable entvars/edict structure
// Input  : *pRequiredEdict - the specific edict to attach to this game object, NULL if should auto-select
//-----------------------------------------------------------------------------
void CBaseEntity::AttachEdict( edict_t *pRequiredEdict )
{
	if ( !pev )
	{
		// see if there is an edict allocated for it, otherwise get one from the engine
		if ( !pRequiredEdict )
		{
			pRequiredEdict = engine->CreateEdict();
		}

		pev = pRequiredEdict;

		// link the entities together
		pRequiredEdict->m_pEnt = this;

		// the edict has the entities classname
		pev->classname = m_iClassname;
	}

	pev->SetFullEdict( true );
}


void CBaseEntity::DetachEdict( void )
{
	if ( pev )
	{
		pev->m_pEnt = NULL;
		pev->classname = MAKE_STRING( "removed" );

		engine->RemoveEdict( pev );

		pev = NULL;
	}
}

void CBaseEntity::SetClassname( const char *className )
{
	m_iClassname = MAKE_STRING( className );

	if ( pev )
	{
		pev->classname = m_iClassname;
	}
}

const char* CBaseEntity::GetClassname()
{
	return STRING(m_iClassname);
}


// position to shoot at
Vector CBaseEntity::BodyTarget( const Vector &posSrc, bool bNoisy) 
{ 
	return WorldSpaceCenter( ); 
}

void CBaseEntity::SetViewOffset( const Vector &vecOffset )
{
	m_vecViewOffset = vecOffset;
}



struct TimedOverlay_t
{
	char 			*msg;
	int				msgEndTime;
	int				msgStartTime;
	TimedOverlay_t	*pNextTimedOverlay; 
};

//-----------------------------------------------------------------------------
// Purpose: Display an error message on the entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseEntity::AddTimedOverlay( const char *msg, int endTime )
{
	TimedOverlay_t *pNewTO = new TimedOverlay_t;
	int len = strlen(msg);
	pNewTO->msg = new char[len + 1];
	Q_strncpy(pNewTO->msg,msg, len+1);
	pNewTO->msgEndTime = gpGlobals->curtime + endTime;
	pNewTO->msgStartTime = gpGlobals->curtime;
	pNewTO->pNextTimedOverlay = m_pTimedOverlay;
	m_pTimedOverlay = pNewTO;
}

//-----------------------------------------------------------------------------
// Purpose: Send debug overlay box to the client
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseEntity::DrawBBoxOverlay()
{
	if (pev)
	{
		NDebugOverlay::EntityBounds(this, 255, 100, 0, 0 ,0);
	}
}


void CBaseEntity::DrawAbsBoxOverlay()
{
	int red = 0;
	int green = 200;

	if ( VPhysicsGetObject() && VPhysicsGetObject()->IsAsleep() )
	{
		red = 90;
		green = 120;
	}

	if (pev)
	{
		// Bounding boxes are axially aligned, so ignore angles
		Vector center = 0.5f * (GetAbsMins() + GetAbsMaxs());
		Vector extents = GetAbsMaxs() - center;
		NDebugOverlay::Box(center, -extents, extents, red, green, 0, 0 ,0);
	}
}

void CBaseEntity::DrawRBoxOverlay()
{	

}

//-----------------------------------------------------------------------------
// Purpose: Send debug overlay line to the client
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseEntity::SendDebugPivotOverlay( void )
{
	if (pev)
	{
		Vector xvec;
		Vector yvec;
		Vector zvec;
		GetVectors(&xvec, &yvec, &zvec);
		xvec	= GetAbsOrigin() + (20 * xvec);
		// actually, y vector is to the left...
		yvec	= GetAbsOrigin() - (20 * yvec);
		zvec	= GetAbsOrigin() + (20 * zvec);

		NDebugOverlay::Line(GetAbsOrigin(),xvec,	255,0,0,true,0);
		NDebugOverlay::Line(GetAbsOrigin(),yvec,	0,255,0,true,0);
		NDebugOverlay::Line(GetAbsOrigin(),zvec,	0,0,255,true,0);
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseEntity::DrawTimedOverlays(void)
{
	// Draw name first if I have an overlay or am in message mode
	if ((m_debugOverlays & OVERLAY_MESSAGE_BIT))
	{
		char tempstr[512];
		Q_strncpy(tempstr,"[",sizeof(tempstr));
		strcat(tempstr,GetDebugName());
		strcat(tempstr,"]");
		NDebugOverlay::EntityText(entindex(),0,tempstr, 0);
	}
	
	// Now draw overlays
	TimedOverlay_t* pTO		= m_pTimedOverlay;
	TimedOverlay_t* pNextTO = NULL;
	TimedOverlay_t* pLastTO = NULL;
	int				nCount	= 1;	// Offset by one
	while (pTO)
	{
		pNextTO = pTO->pNextTimedOverlay;

		// Remove old messages unless messages are paused
		if ((!CBaseEntity::Debug_IsPaused() && gpGlobals->curtime > pTO->msgEndTime) ||
			(nCount > 10))
		{
			if (pLastTO)
			{
				pLastTO->pNextTimedOverlay = pNextTO;
			}
			else
			{
				m_pTimedOverlay = pNextTO;
			}

			delete pTO->msg;
			delete pTO;
		}
		else
		{
			int nAlpha = 0;
			
			// If messages aren't paused fade out
			if (!CBaseEntity::Debug_IsPaused())
			{
				nAlpha = 255*((gpGlobals->curtime - pTO->msgStartTime)/(pTO->msgEndTime - pTO->msgStartTime));
			}
			int r = 185;
			int g = 145;
			int b = 145;

			// Brighter when new message
			if (nAlpha < 50)
			{
				r = 255;
				g = 205;
				b = 205;
			}
			if (nAlpha < 0) nAlpha = 0;
			NDebugOverlay::EntityText(entindex(),nCount,pTO->msg, 0.01, r, g, b, 255-nAlpha);
			nCount++;

			pLastTO = pTO;
		}
		pTO	= pNextTO;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw all overlays (should be implemented by subclass to add
//			any additional non-text overlays)
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
void CBaseEntity::DrawDebugGeometryOverlays(void) 
{
	DrawTimedOverlays();
	DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_NAME_BIT) 
	{	
		NDebugOverlay::EntityText(entindex(),0,GetDebugName(), 0);
	}
	if (m_debugOverlays & OVERLAY_BBOX_BIT) 
	{	
		DrawBBoxOverlay();
	}
	if ( m_debugOverlays & OVERLAY_ABSBOX_BIT )
	{
		DrawAbsBoxOverlay();
	}
	if (m_debugOverlays & OVERLAY_PIVOT_BIT) 
	{	
		SendDebugPivotOverlay();
	}
	if( m_debugOverlays & OVERLAY_RBOX_BIT )
	{
		DrawRBoxOverlay();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any text overlays (override in subclass to add additional text)
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CBaseEntity::DrawDebugTextOverlays(void) 
{
	int offset = 1;
	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf( tempstr, sizeof(tempstr), "(%d) Name: %s (%s)", entindex(), GetDebugName(), GetClassname() );
		NDebugOverlay::EntityText(entindex(),offset,tempstr, 0);
		offset++;
	}

	return offset;
}

void CBaseEntity::SetName( string_t newName )
{
	m_iName = newName;
}


void CBaseEntity::SetParent( string_t newParent, CBaseEntity *pActivator )
{
	// find and notify the new parent
	CBaseEntity *pParent = gEntList.FindEntityByName( NULL, newParent, pActivator );

	// debug check
	if ( newParent != NULL_STRING && pParent == NULL )
	{
		Msg( "Entity %s(%s) has bad parent %s\n", STRING(m_iClassname), GetDebugName(), STRING(newParent) );
	}
	else
	{
		// make sure there isn't any ambiguity
		if ( gEntList.FindEntityByName( pParent, newParent, pActivator ) )
		{
			Msg( "Entity %s(%s) has ambigious parent %s\n", STRING(m_iClassname), GetDebugName(), STRING(newParent) );
		}
		SetParent( pParent );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the movement parent of this entity. This entity will be moved
//			to a local coordinate calculated from its current absolute offset
//			from the parent entity and will then follow the parent entity.
// Input  : pParentEntity - This entity's new parent in the movement hierarchy.
//-----------------------------------------------------------------------------
void CBaseEntity::SetParent( const CBaseEntity *pParentEntity, int iAttachment )
{
	// notify the old parent of the loss
	UnlinkFromParent( this );
	NetworkStateChanged();

	// set the new name
	m_pParent = pParentEntity;
	Assert( m_pParent != this );
	if ( !m_pParent )
	{
		m_iParent = NULL_STRING;
		return;
	}

	m_iParent = m_pParent->m_iName;

	if ( pParentEntity )
	{
		if ( GetSolid() == SOLID_BSP )
		{
			// Must be SOLID_VPHYSICS because parent might rotate
			SetSolid( SOLID_VPHYSICS );
		}
	}
	// set the move parent if we have one
	if ( pev )
	{
		// add ourselves to the list
		LinkChild( m_pParent, this );

		m_iParentAttachment = (char)iAttachment;

		EntityMatrix matrix, childMatrix;
		matrix.InitFromEntity( const_cast<CBaseEntity *>(pParentEntity), m_iParentAttachment ); // parent->world
		childMatrix.InitFromEntityLocal( this ); // child->world
		Vector localOrigin = matrix.WorldToLocal( GetLocalOrigin() );
		
		// I have the axes of local space in world space. (childMatrix)
		// I want to compute those world space axes in the parent's local space
		// and set that transform (as angles) on the child's object so the net
		// result is that the child is now in parent space, but still oriented the same way
		VMatrix tmp = matrix.Transpose(); // world->parent
		tmp.MatrixMul( childMatrix, matrix ); // child->parent
		QAngle angles;
		MatrixToAngles( matrix, angles );
		SetLocalAngles( angles );
		UTIL_SetOrigin( this, localOrigin );
	}
}

void CBaseEntity::Activate( void )
{
#ifdef DEBUG
	extern bool g_bCheckForChainedActivate;
	extern bool g_bReceivedChainedActivate;
	AssertMsg( !g_bCheckForChainedActivate || g_bReceivedChainedActivate == false, "Multiple calls to base class Activate()\n" );
	g_bReceivedChainedActivate = true;
#endif
	// NOTE: This forces a team change so that stuff in the level
	// that starts out on a team correctly changes team
	if (m_iInitialTeamNum)
	{
		ChangeTeam( m_iInitialTeamNum );
	}	

	// Get a handle to my damage filter entity if there is one.
	if ( m_iszDamageFilterName != NULL_STRING )
	{
		m_hDamageFilter = gEntList.FindEntityByName( NULL, m_iszDamageFilterName, NULL );
	}

	// Add any non-null context strings to our context vector
	if ( m_iszContext != NULL_STRING ) 
	{
		AddContext( m_iszContext.ToCStr() );
	}
}

////////////////////////////  old CBaseEntity stuff ///////////////////////////////////


// give health
int CBaseEntity::TakeHealth( float flHealth, int bitsDamageType )
{
	if ( !pev || m_takedamage < DAMAGE_YES )
		return 0;

// heal
	if ( m_iHealth >= m_iMaxHealth )
		return 0;

	m_iHealth += flHealth;

	if (m_iHealth > m_iMaxHealth)
		m_iHealth = m_iMaxHealth;

	return 1;
}

// inflict damage on this entity.  bitsDamageType indicates type of damage inflicted, ie: DMG_CRUSH

int CBaseEntity::OnTakeDamage( const CTakeDamageInfo &info )
{
	Vector			vecTemp;

	if ( !pev || !m_takedamage )
		return 0;

	vecTemp = info.GetInflictor()->GetAbsOrigin() - ( WorldSpaceCenter() );

// this global is still used for glass and other non-NPC killables, along with decals.
	g_vecAttackDir = vecTemp;
	VectorNormalize(g_vecAttackDir);
		
// save damage based on the target's armor level

// figure momentum add (don't let hurt brushes or other triggers move player)

	// physics objects have their own calcs for this: (don't let fire move things around!)
	if ( ( GetMoveType() == MOVETYPE_VPHYSICS ) )
	{
		VPhysicsTakeDamage( info );
	}
	else
	{
		if ( info.GetInflictor() && (GetMoveType() == MOVETYPE_WALK || GetMoveType() == MOVETYPE_STEP) && 
			!info.GetAttacker()->IsSolidFlagSet(FSOLID_TRIGGER) )
		{
			Vector vecDir, vecInflictorCentroid;
			vecDir = WorldSpaceCenter( );
			vecInflictorCentroid = info.GetInflictor()->WorldSpaceCenter( );
			vecDir -= vecInflictorCentroid;
			VectorNormalize( vecDir );

			float flForce = info.GetDamage() * ((32 * 32 * 72.0) / (WorldAlignSize().x * WorldAlignSize().y * WorldAlignSize().z)) * 5;
			
			if (flForce > 1000.0) 
				flForce = 1000.0;
			ApplyAbsVelocityImpulse( vecDir * flForce );
		}
	}

	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
	// do the damage
		m_iHealth -= info.GetDamage();
		if (m_iHealth <= 0)
		{
			Event_Killed( info );
			return 0;
		}
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Scale damage done and call OnTakeDamage
//-----------------------------------------------------------------------------
void CBaseEntity::TakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( !( inputInfo.GetDamageType() & DMG_NO_PHYSICS_FORCE ) && inputInfo.GetDamageType() != DMG_GENERIC )
	{
		// If you hit this assert, you've called TakeDamage with a damage type that requires a physics damage
		// force & position without specifying one or both of them. Decide whether your damage that's causing 
		// this is something you believe should impart physics force on the receiver. If it is, you need to 
		// setup the damage force & position inside the CTakeDamageInfo (Utility functions for this are in
		// takedamageinfo.cpp. If you think the damage shouldn't cause force (unlikely!) then you can set the 
		// damage type to DMG_GENERIC, or | DMG_CRUSH if you need to preserve the damage type for purposes of HUD display.
		Assert( inputInfo.GetDamageForce() != vec3_origin && inputInfo.GetDamagePosition() != vec3_origin );
	}

	// Make sure our damage filter allows the damage.
	if ( !PassesDamageFilter( inputInfo.GetAttacker() ))
	{
		return;
	}

	if ( physenv->IsInSimulation() )
	{
		PhysCallbackDamage( this, inputInfo );
	}
	else
	{
		CTakeDamageInfo info = inputInfo;
		
		// Scale the damage by the attacker's modifier.
		if ( info.GetAttacker() )
		{
			info.ScaleDamage( info.GetAttacker()->GetAttackDamageScale() );
		}

		// Scale the damage by my own modifiers
		info.ScaleDamage( GetReceivedDamageScale() );

		//Msg("%s took %.2f Damage, at %.2f\n", GetClassname(), info.GetDamage(), gpGlobals->curtime );

		OnTakeDamage( info );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns a value that scales all damage done by this entity.
//-----------------------------------------------------------------------------
float CBaseEntity::GetAttackDamageScale( void )
{
	float flScale = 1;
	FOR_EACH_LL( m_DamageModifiers, i )
	{
		if ( !m_DamageModifiers[i]->IsDamageDoneToMe() )
		{
			flScale *= m_DamageModifiers[i]->GetModifier();
		}
	}
	return flScale;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a value that scales all damage done to this entity
//-----------------------------------------------------------------------------
float CBaseEntity::GetReceivedDamageScale( void )
{
	float flScale = 1;
	FOR_EACH_LL( m_DamageModifiers, i )
	{
		if ( m_DamageModifiers[i]->IsDamageDoneToMe() )
		{
			flScale *= m_DamageModifiers[i]->GetModifier();
		}
	}
	return flScale;
}


//-----------------------------------------------------------------------------
// Purpose: Applies forces to our physics object in response to damage.
//-----------------------------------------------------------------------------
int CBaseEntity::VPhysicsTakeDamage( const CTakeDamageInfo &info )
{
	// don't let physics impacts or fire cause objects to move (again)
	if ( info.GetDamageType() & DMG_NO_PHYSICS_FORCE || info.GetDamageType() == DMG_GENERIC )
		return 1;

	Assert(VPhysicsGetObject() != NULL);
	if ( VPhysicsGetObject() )
	{
		Vector force = info.GetDamageForce();
		Vector offset = info.GetDamagePosition();

		// If you hit this assert, you've called TakeDamage with a damage type that requires a physics damage
		// force & position without specifying one or both of them. Decide whether your damage that's causing 
		// this is something you believe should impart physics force on the receiver. If it is, you need to 
		// setup the damage force & position inside the CTakeDamageInfo (Utility functions for this are in
		// takedamageinfo.cpp. If you think the damage shouldn't cause force (unlikely!) then you can set the 
		// damage type to DMG_GENERIC, or | DMG_CRUSH if you need to preserve the damage type for purposes of HUD display.
		Assert( force != vec3_origin && offset != vec3_origin );

		VPhysicsGetObject()->ApplyForceOffset( force, offset );
	}

	return 1;
}

	// Character killed (only fired once)
void CBaseEntity::Event_Killed( const CTakeDamageInfo &info )
{
	m_takedamage = DAMAGE_NO;
	m_lifeState = LIFE_DEAD;
	UTIL_Remove( this );
}



bool CBaseEntity::HasTarget( string_t targetname )
{
	if( targetname != NULL_STRING && m_target != NULL_STRING )
		return FStrEq(STRING(targetname), STRING(m_target) );
	else
		return false;
}


CBaseEntity *CBaseEntity::GetNextTarget( void )
{
	if ( !m_target )
		return NULL;
	return gEntList.FindEntityByName( NULL, m_target, NULL );
}

class CThinkContextsSaveDataOps : public CDefSaveRestoreOps
{
	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		AssertMsg( fieldInfo.pTypeDesc->fieldSize == 1, "CThinkContextsSaveDataOps does not support arrays");

		// Write out the vector
		CUtlVector< thinkfunc_t > *pUtlVector = (CUtlVector< thinkfunc_t > *)fieldInfo.pField;
		SaveUtlVector( pSave, pUtlVector, FIELD_EMBEDDED );

		// Get our owner
		CBaseEntity *pOwner = (CBaseEntity*)fieldInfo.pOwner;

		// Now write out all the functions
		for ( int i = 0; i < pUtlVector->Size(); i++ )
		{
			void *pV = &((*pUtlVector)[i].m_pfnThink);
			pSave->WriteFunction( pOwner->GetDataDescMap(), "m_pfnThink", (int *)(char *)pV, 1 );
		}
	}

	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		AssertMsg( fieldInfo.pTypeDesc->fieldSize == 1, "CThinkContextsSaveDataOps does not support arrays");

		// Read in the vector
		CUtlVector< thinkfunc_t > *pUtlVector = (CUtlVector< thinkfunc_t > *)fieldInfo.pField;
		RestoreUtlVector( pRestore, pUtlVector, FIELD_EMBEDDED );

		// Get our owner
		CBaseEntity *pOwner = (CBaseEntity*)fieldInfo.pOwner;

		// Now read in all the functions
		for ( int i = 0; i < pUtlVector->Size(); i++ )
		{
			void *pV = &((*pUtlVector)[i].m_pfnThink);
			pRestore->ReadFunction( pOwner->GetDataDescMap(), &pV, 1, fieldInfo.pTypeDesc->fieldSize );
		}
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		CUtlVector< thinkfunc_t > *pUtlVector = (CUtlVector< thinkfunc_t > *)fieldInfo.pField;
		return ( pUtlVector->Count() == 0 );
	}

	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		BASEPTR pFunc = *((BASEPTR*)fieldInfo.pField);
		pFunc = NULL;
	}
};
CThinkContextsSaveDataOps g_ThinkContextsSaveDataOps;
ISaveRestoreOps *thinkcontextFuncs = &g_ThinkContextsSaveDataOps;

BEGIN_SIMPLE_DATADESC( thinkfunc_t )

	DEFINE_ARRAY( thinkfunc_t,		m_pszContext,	FIELD_CHARACTER, MAX_CONTEXT_LENGTH ),
	// DEFINE_FIELD( thinkfunc_t,		m_pfnThink,		FIELD_FUNCTION ),		// Manually written

	// NOTE:  If these are ever save/restored in single player, they should probably be encoded
	//  as FIELD_TICK/FIELD_TIME
	DEFINE_FIELD( thinkfunc_t,		m_nNextThinkTick,	FIELD_INTEGER	),
	DEFINE_FIELD( thinkfunc_t,		m_nLastThinkTick,	FIELD_INTEGER	),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( ResponseContext_t )

	DEFINE_FIELD( ResponseContext_t, m_iszName,	 FIELD_STRING ),
	DEFINE_FIELD( ResponseContext_t, m_iszValue, FIELD_STRING ),

END_DATADESC()

BEGIN_DATADESC_NO_BASE( CBaseEntity )

	DEFINE_KEYFIELD( CBaseEntity, m_iClassname, FIELD_STRING, "classname" ),
	DEFINE_GLOBAL_KEYFIELD( CBaseEntity, m_iGlobalname, FIELD_STRING, "globalname" ),
	DEFINE_KEYFIELD( CBaseEntity, m_iParent, FIELD_STRING, "parentname" ),

	DEFINE_KEYFIELD( CBaseEntity, m_flSpeed, FIELD_FLOAT, "speed" ),
	DEFINE_KEYFIELD( CBaseEntity, m_nRenderFX, FIELD_INTEGER, "renderfx" ),
	DEFINE_KEYFIELD( CBaseEntity, m_nRenderMode, FIELD_INTEGER, "rendermode" ),

	// Consider moving to CBaseAnimating?
	DEFINE_FIELD( CBaseEntity, m_flPrevAnimTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseEntity, m_flAnimTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseEntity, m_flSimulationTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseEntity, m_nLastThinkTick, FIELD_TICK ),

	DEFINE_KEYFIELD( CBaseEntity, m_nNextThinkTick, FIELD_TICK, "nextthink" ),
	DEFINE_KEYFIELD( CBaseEntity, m_fEffects, FIELD_INTEGER, "effects" ),
	DEFINE_KEYFIELD( CBaseEntity, m_clrRender, FIELD_COLOR32, "rendercolor" ),
	DEFINE_GLOBAL_KEYFIELD( CBaseEntity, m_nModelIndex, FIELD_INTEGER, "modelindex" ),
	// DEFINE_FIELD( CBaseEntity, m_PredictableID, CPredictableId ),
	// DEFINE_FIELD( CBaseEntity, m_EntitiesTouched, touchlink_t ),
	DEFINE_FIELD( CBaseEntity, touchStamp, FIELD_INTEGER ),
	// DEFINE_EMBEDDED( CBaseEntity, m_NetStateMgr ),
	// DEFINE_FIELD( CBaseEntity, pev, edict_t ),

	DEFINE_CUSTOM_FIELD( CBaseEntity, m_aThinkFunctions, thinkcontextFuncs ),
	//								m_iCurrentThinkContext (not saved, debug field only, and think transient to boot)

	DEFINE_UTLVECTOR(CBaseEntity, m_ResponseContexts,		FIELD_EMBEDDED),
	DEFINE_KEYFIELD( CBaseEntity, m_iszContext, FIELD_STRING, "ResponseContext" ),

	// Needs to be set up in the Activate methods of derived classes
	// DEFINE_FIELD( CBaseEntity, m_pInstancedResponseSystem, IResponseSystem ),

	DEFINE_FIELD( CBaseEntity, m_pfnThink, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseEntity, m_pfnTouch, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseEntity, m_pfnUse, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseEntity, m_pfnBlocked, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseEntity, m_pfnMoveDone, FIELD_FUNCTION ),

	DEFINE_FIELD( CBaseEntity, m_lifeState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_takedamage, FIELD_INTEGER ),
	DEFINE_KEYFIELD( CBaseEntity, m_iMaxHealth, FIELD_INTEGER, "max_health" ),
	DEFINE_KEYFIELD( CBaseEntity, m_iHealth, FIELD_INTEGER, "health" ),
	// DEFINE_FIELD( CBaseEntity, m_pLink, FIELD_CLASSPTR ),
	DEFINE_KEYFIELD( CBaseEntity, m_target, FIELD_STRING, "target" ),

	DEFINE_KEYFIELD( CBaseEntity, m_iszDamageFilterName, FIELD_STRING, "damagefilter" ),
	DEFINE_FIELD( CBaseEntity, m_hDamageFilter, FIELD_EHANDLE ),
	
	// DEFINE_FIELD( CBaseEntity, m_debugOverlays, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseEntity, m_pParent, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseEntity, m_iParentAttachment, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseEntity, m_hMoveParent, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseEntity, m_hMoveChild, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseEntity, m_hMovePeer, FIELD_EHANDLE ),
	
	DEFINE_FIELD( CBaseEntity, m_iEFlags, FIELD_INTEGER ),

	DEFINE_KEYFIELD( CBaseEntity, m_iName, FIELD_STRING, "targetname" ),
	DEFINE_EMBEDDED( CBaseEntity, m_Collision ),

	DEFINE_FIELD( CBaseEntity, m_MoveType, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_MoveCollide, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseEntity, m_hOwnerEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseEntity, m_CollisionGroup, FIELD_INTEGER ),
	DEFINE_PHYSPTR( CBaseEntity, m_pPhysicsObject),
	DEFINE_FIELD( CBaseEntity, m_flElasticity, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseEntity, m_flShadowCastDistance, FIELD_FLOAT ),

	DEFINE_INPUT( CBaseEntity, m_iInitialTeamNum, FIELD_INTEGER, "TeamNum" ),
	DEFINE_FIELD( CBaseEntity, m_iTeamNum, FIELD_INTEGER ),

//	DEFINE_FIELD( CBaseEntity, m_bCheckUntouch, FIELD_INTEGER ),
//	DEFINE_FIELD( CBaseEntity, m_bSentLastFrame, FIELD_INTEGER ),


	DEFINE_FIELD( CBaseEntity, m_hGroundEntity, FIELD_EHANDLE ),
	DEFINE_GLOBAL_KEYFIELD( CBaseEntity, m_ModelName, FIELD_MODELNAME, "model" ),
	
	DEFINE_KEYFIELD( CBaseEntity, m_vecAbsMins, FIELD_POSITION_VECTOR, "absmin" ),
	DEFINE_KEYFIELD( CBaseEntity, m_vecAbsMaxs, FIELD_POSITION_VECTOR, "absmax" ),

	DEFINE_KEYFIELD( CBaseEntity, m_vecBaseVelocity, FIELD_VECTOR, "basevelocity" ),
	DEFINE_FIELD( CBaseEntity, m_vecAbsVelocity, FIELD_VECTOR ),
	DEFINE_KEYFIELD( CBaseEntity, m_vecAngVelocity, FIELD_VECTOR, "avelocity" ),
//	DEFINE_FIELD( CBaseEntity, m_vecAbsAngVelocity, FIELD_VECTOR ),
	DEFINE_ARRAY( CBaseEntity, m_rgflCoordinateFrame, FIELD_FLOAT, 12 ), // NOTE: MUST BE IN LOCAL SPACE, NOT POSITION_VECTOR!!! (see CBaseEntity::Restore)

	DEFINE_KEYFIELD( CBaseEntity, m_nWaterLevel, FIELD_INTEGER, "waterlevel" ),
	DEFINE_KEYFIELD( CBaseEntity, m_nWaterType, FIELD_INTEGER, "watertype" ),
	DEFINE_FIELD( CBaseEntity, m_pBlocker, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( CBaseEntity, m_flGravity, FIELD_FLOAT, "gravity" ),
	DEFINE_KEYFIELD( CBaseEntity, m_flFriction, FIELD_FLOAT, "friction" ),

	// Local time is local to each object.  It doesn't need to be re-based if the clock
	// changes.  Therefore it is saved as a FIELD_FLOAT, not a FIELD_TIME
	DEFINE_KEYFIELD( CBaseEntity, m_flLocalTime, FIELD_FLOAT, "ltime" ),
	DEFINE_FIELD( CBaseEntity, m_flVPhysicsUpdateLocalTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseEntity, m_flMoveDoneTime, FIELD_FLOAT ),

//	DEFINE_FIELD( CBaseEntity, m_nPushEnumCount, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseEntity, m_vecAbsOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_KEYFIELD( CBaseEntity, m_vecVelocity, FIELD_VECTOR, "velocity" ),
	DEFINE_KEYFIELD( CBaseEntity, m_iTextureFrameIndex, FIELD_INTEGER, "texframeindex" ),
	DEFINE_FIELD( CBaseEntity, m_bSimulatedEveryTick, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseEntity, m_bAnimatedEveryTick, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( CBaseEntity, m_spawnflags, FIELD_INTEGER, "spawnflags" ),

//	DEFINE_FIELD( CBaseEntity, m_pTransmitProxy, FIELD_???? ),

	DEFINE_FIELD( CBaseEntity, m_angAbsRotation, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseEntity, m_vecOrigin, FIELD_VECTOR ),			// NOTE: MUST BE IN LOCAL SPACE, NOT POSITION_VECTOR!!! (see CBaseEntity::Restore)
	DEFINE_FIELD( CBaseEntity, m_angRotation, FIELD_VECTOR ),

	DEFINE_KEYFIELD( CBaseEntity, m_vecViewOffset, FIELD_VECTOR, "view_ofs" ),

	DEFINE_KEYFIELD( CBaseEntity, m_fFlags, FIELD_INTEGER, "flags" ),

//	DEFINE_FIELD( CBaseEntity, m_bIsPlayerSimulated, FIELD_INTEGER ),
//	DEFINE_FIELD( CBaseEntity, m_hPlayerSimulationOwner, FIELD_EHANDLE ),

	// DEFINE_FIELD( CBaseEntity, m_pTimedOverlay, TimedOverlay_t* ),
	DEFINE_FIELD( CBaseEntity, m_nSimulationTick, FIELD_TICK ),
	// DEFINE_FIELD( CBaseEntity, m_RefEHandle, CBaseHandle ),

	// NOTE: This is tricky. TeamNum must be saved, but we can't directly
	// read it in, because we can only set it after the team entity has been read in,
	// which may or may not actually occur before the entity is parsed.
	// Therefore, we set the TeamNum from the InitialTeamNum in Activate
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_INTEGER, "SetTeam", InputSetTeam ),

	DEFINE_INPUTFUNC( CBaseEntity, FIELD_VOID, "Kill", InputKill ),
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_VOID, "Use", InputUse ),
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_INTEGER, "Alpha", InputAlpha ),
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_COLOR32, "Color", InputColor ),
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_STRING, "SetParent", InputSetParent ),
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_VOID, "ClearParent", InputClearParent ),
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_STRING, "SetDamageFilter", InputSetDamageFilter ),

	DEFINE_INPUTFUNC( CBaseEntity, FIELD_STRING, "DispatchEffect", InputDispatchEffect ),
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_STRING, "DispatchResponse", InputDispatchResponse ),

	// Entity I/O methods to alter context
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_STRING, "AddContext", InputAddContext ),
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_STRING, "RemoveContext", InputRemoveContext ),
	DEFINE_INPUTFUNC( CBaseEntity, FIELD_STRING, "ClearContext", InputClearContext ),

	// Function Pointers
	DEFINE_FUNCTION( CBaseEntity, SUB_Remove ),
	DEFINE_FUNCTION( CBaseEntity, SUB_DoNothing ),
	DEFINE_FUNCTION( CBaseEntity, SUB_StartFadeOut ),
	DEFINE_FUNCTION( CBaseEntity, SUB_FadeOut ),
	DEFINE_FUNCTION( CBaseEntity, SUB_Vanish ),
	DEFINE_FUNCTION( CBaseEntity, SUB_CallUseToggle ),

	//DEFINE_FIELD( CBaseEntity, m_DamageModifiers, FIELD_?? ), // can't save?

END_DATADESC()

// For code error checking
extern bool g_bReceivedChainedUpdateOnRemove;

//-----------------------------------------------------------------------------
// Purpose: Called just prior to object destruction
//  Entities that need to unlink themselves from other entities should do the unlinking
//  here rather than in their destructor.  The reason why is that when the global entity list
//  is told to Clear(), it first takes a pass through all active entities and calls UTIL_Remove
//  on each such entity.  Then it calls the delete function on each deleted entity in the list.
// In the old code, the objects were simply destroyed in order and there was no guarantee that the
//  destructor of one object would not try to access another object that might already have been
//  destructed (especially since the entity list order is more or less random!).
// NOTE:  You should never call delete directly on an entity (there's an assert now), see note
//  at CBaseEntity::~CBaseEntity for more information.
// 
// NOTE:  You should chain to BaseClass::UpdateOnRemove after doing your own cleanup code, e.g.:
// 
// void CDerived::UpdateOnRemove( void )
// {
//		... cleanup code
//		...
//
//		BaseClass::UpdateOnRemove();
// }
//
// In general, this function updates global tables that need to know about entities being removed
//-----------------------------------------------------------------------------
void CBaseEntity::UpdateOnRemove( void )
{
	g_bReceivedChainedUpdateOnRemove = true;

	// Notifies entity listeners, etc
	gEntList.NotifyRemoveEntity( GetRefEHandle() );

	if ( edict() )
	{
		AddFlag( FL_KILLME );
		if ( GetFlags() & FL_GRAPHED )
		{
			/*	<<TODO>>
			// this entity was a LinkEnt in the world node graph, so we must remove it from
			// the graph since we are removing it from the world.
			for ( int i = 0 ; i < WorldGraph.m_cLinks ; i++ )
			{
				if ( WorldGraph.m_pLinkPool [ i ].m_pLinkEnt == pev )
				{
					// if this link has a link ent which is the same ent that is removing itself, remove it!
					WorldGraph.m_pLinkPool [ i ].m_pLinkEnt = NULL;
				}
			}
			*/
		}
	}

	if ( m_iGlobalname != NULL_STRING )
	{
		// NOTE: During level shutdown the global list will suppress this
		// it assumes your changing levels or the game will end
		// causing the whole list to be flushed
		GlobalEntity_SetState( m_iGlobalname, GLOBAL_DEAD );
	}

	VPhysicsDestroyObject();

	// This is only here to allow the MOVETYPE_NONE to be set without the
	// assertion triggering. Why do we bother setting the MOVETYPE to none here?
	m_fEffects &= ~EF_BONEMERGE;
	SetMoveType(MOVETYPE_NONE);

	// If we have a parent, unlink from it.
	UnlinkFromParent( this );

	// If we have children, move them to the root of the hierarchy.
	UnlinkAllChildren( this );

	g_Predictables.RemoveEntity( this );

	RemoveFromUpdateClientDataList( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseEntity::InitPredictable( void )
{
	g_Predictables.AddEntity( this );
}

//-----------------------------------------------------------------------------
// capabilities
//-----------------------------------------------------------------------------
int CBaseEntity::ObjectCaps( void ) 
{
	model_t *pModel = GetModel();
	if ( pModel && modelinfo->GetModelType( pModel ) == mod_brush )
		return 0;

	return FCAP_ACROSS_TRANSITION; 
}

void CBaseEntity::StartTouch( CBaseEntity *pOther )
{
	// notify parent
	if ( m_pParent != NULL )
		m_pParent->StartTouch( pOther );
}

void CBaseEntity::Touch( CBaseEntity *pOther )
{ 
	if ( m_pfnTouch ) 
		(this->*m_pfnTouch)( pOther );

	// notify parent of touch
	if ( m_pParent != NULL )
		m_pParent->Touch( pOther );
}

void CBaseEntity::EndTouch( CBaseEntity *pOther )
{
	// notify parent
	if ( m_pParent != NULL )
	{
		m_pParent->EndTouch( pOther );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Dispatches blocked events to this entity's blocked handler, set via SetBlocked.
// Input  : pOther - The entity that is blocking us.
//-----------------------------------------------------------------------------
void CBaseEntity::Blocked( CBaseEntity *pOther )
{ 
	if ( m_pfnBlocked )
	{
		(this->*m_pfnBlocked)( pOther );
	}

	//
	// Forward the blocked event to our parent, if any.
	//
	if ( m_pParent != NULL )
	{
		m_pParent->Blocked( pOther );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Dispatches use events to this entity's use handler, set via SetUse.
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CBaseEntity::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) 
{
	if ( m_pfnUse != NULL ) 
	{
		(this->*m_pfnUse)( pActivator, pCaller, useType, value );
	}
	else
	{
		//
		// We don't handle use events. Forward to our parent, if any.
		//
		if ( m_pParent != NULL )
		{
			m_pParent->Use( pActivator, pCaller, useType, value );
		}
	}
}


void CBaseEntity::VPhysicsUpdatePusher( IPhysicsObject *pPhysics )
{
	float movetime = m_flLocalTime - m_flVPhysicsUpdateLocalTime;
	if (movetime <= 0)
		return;

	Vector origin;
	QAngle angles;

	bool physicsMoved = false;

	// physics updated the shadow, so check to see if I got blocked
	// NOTE: SOLID_BSP cannont compute onsistent collisions wrt vphysics, so 
	// don't allow vphysics to block.  Assume game physics has handled it.
	if ( GetSolid() != SOLID_BSP && pPhysics->GetShadowPosition( &origin, &angles ) )
	{
		physicsMoved = true;
		//NDebugOverlay::EntityBox( 255,0,0,0, gpGlobals->frametime);

		bool checkrot = (GetLocalAngularVelocity() != vec3_angle) ? true : false;
		bool checkmove = (GetLocalVelocity() != vec3_origin) ? true : false;

		float physLocalTime = m_flLocalTime;

		if ( checkmove )
		{
			float distSqr = (origin - GetAbsOrigin()).LengthSqr();
			if ( distSqr > 1 )
			{
				float dist = FastSqrt(distSqr);
	#if 0
				const char *pName = STRING(m_iClassname);
				Msg( "%s blocked by %.2f units\n", pName, dist );
	#endif
				float expectedDist = GetAbsVelocity().Length() * movetime;
				float t =  dist / expectedDist;
				t = clamp(t, 0, movetime);
				physLocalTime = m_flLocalTime - t;
			}
		}

		if ( checkrot )
		{
			Vector axis;
			float deltaAngle;
			RotationDeltaAxisAngle( angles, GetAbsAngles(), axis, deltaAngle );

			if ( fabsf(deltaAngle) > 0.5f )
			{
	#if 0
				const char *pName = STRING(m_iClassname);
				Msg( "%s blocked by %.2f degrees\n", pName, deltaAngle );
	#endif
				physLocalTime = m_flVPhysicsUpdateLocalTime;
			}
		}

		float moveback = m_flLocalTime - physLocalTime;
		if ( moveback > 0 )
		{
			Vector origin = GetLocalOrigin();
			QAngle angles = GetLocalAngles();

			if ( checkmove )
			{
				origin -= GetLocalVelocity() * moveback;
			}
			if ( checkrot )
			{
				// BUGBUG: This is pretty hack-tastic!
				angles -= GetLocalAngularVelocity() * moveback;
			}

			SetLocalOrigin( origin );
			SetLocalAngles( angles );

			Relink();

			if ( physLocalTime <= m_flVPhysicsUpdateLocalTime )
			{
				IPhysicsObject *pOther;
				if ( pPhysics->GetContactPoint( NULL, &pOther ) )
				{
					CBaseEntity *pBlocked = static_cast<CBaseEntity *>(pOther->GetGameData());
					Blocked( pBlocked );
				}
			}

			m_flLocalTime = physLocalTime;
		}
	}

	m_flVPhysicsUpdateLocalTime = m_flLocalTime;
	if ( m_flMoveDoneTime <= m_flLocalTime && m_flMoveDoneTime != 0 )
	{
		m_flMoveDoneTime = 0;
		MoveDone();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Relinks all of a parents children into the collision tree
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsRelinkChildren( void )
{
	CBaseEntity *child;

	// iterate through all children
	for ( child = FirstMoveChild(); child != NULL; child = child->NextMovePeer() )
	{
		// UNDONE: Do this always? It MUST be done for moving triggers
		bool reTouch = child->IsSolidFlagSet(FSOLID_TRIGGER);
		engine->RelinkEntity( child->pev, reTouch );

		//
		// Update their physics shadows. We should never have any children of
		// movetype VPHYSICS.
		//
		ASSERT( child->GetMoveType() != MOVETYPE_VPHYSICS );
		if ( child->GetMoveType() != MOVETYPE_VPHYSICS )
		{
			child->UpdatePhysicsShadowToCurrentPosition( gpGlobals->frametime );
		}

		if ( child->FirstMoveChild() )
		{
			child->PhysicsRelinkChildren();
		}
	}
}

// This creates a vphysics object with a shadow controller that follows the AI
IPhysicsObject *CBaseEntity::VPhysicsInitShadow( bool allowPhysicsMovement, bool allowPhysicsRotation, solid_t *pSolid )
{
	if ( !VPhysicsInitSetup() )
		return NULL;

	// No physics
	if ( GetSolid() == SOLID_NONE )
		return NULL;

	const Vector &origin = GetAbsOrigin();
	QAngle angles = GetAbsAngles();
	IPhysicsObject *pPhysicsObject = NULL;

	if ( GetSolid() == SOLID_BBOX )
	{
		pPhysicsObject = PhysModelCreateBox( this, WorldAlignMins(), WorldAlignMaxs(), origin, false );
	}
	else
	{
		pPhysicsObject = PhysModelCreate( this, GetModelIndex(), origin, angles, pSolid );
	}
	if ( !pPhysicsObject )
		return NULL;

	VPhysicsSetObject( pPhysicsObject );
	// UNDONE: Tune these speeds!!!
	pPhysicsObject->SetShadow( Vector(1e4,1e4,1e4), AngularImpulse(1e4,1e4,1e4), allowPhysicsMovement, allowPhysicsRotation );
	pPhysicsObject->UpdateShadow( origin, angles, false, 0 );
	PhysAddShadow( this );
	return pPhysicsObject;
}


void CBaseEntity::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
}



void CBaseEntity::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// filter out ragdoll props hitting other parts of itself too often
	// UNDONE: Store a sound time for this entity (not just this pair of objects)
	// and filter repeats on that?
	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	if ( pEvent->deltaCollisionTime < 0.5 && (pHitEntity == this) )
		return;

	// don't make noise for hidden/invisible/sky materials
	surfacedata_t *phit = physprops->GetSurfaceData( pEvent->surfaceProps[otherIndex] );
	surfacedata_t *pprops = physprops->GetSurfaceData( pEvent->surfaceProps[index] );
	if ( phit->gameMaterial == 'X' || pprops->gameMaterial == 'X' )
		return;

	if ( pHitEntity == this )
	{
		PhysCollisionSound( this, pEvent->pObjects[index], CHAN_BODY, pEvent->surfaceProps[index], pEvent->deltaCollisionTime, pEvent->collisionSpeed );
	}
	else
	{
		PhysCollisionSound( this, pEvent->pObjects[index], CHAN_STATIC, pEvent->surfaceProps[index], pEvent->deltaCollisionTime, pEvent->collisionSpeed );
	}

	//Kick up dust
	Vector	vecPos, vecVel;

	pEvent->pInternalData->GetContactPoint( vecPos );

	vecVel.Random( -1.0f, 1.0f );
	vecVel.z = random->RandomFloat( 0.3f, 1.0f );

	switch ( phit->gameMaterial )
	{
	case CHAR_TEX_DIRT:

		if ( pEvent->collisionSpeed < 200.0f )
			break;
		
		g_pEffects->Dust( vecPos, vecVel, 8.0f, pEvent->collisionSpeed );
		
		break;

	case CHAR_TEX_CONCRETE:

		if ( pEvent->collisionSpeed < 340.0f )
			break;

		g_pEffects->Dust( vecPos, vecVel, 8.0f, pEvent->collisionSpeed );

		break;
	}
}

void CBaseEntity::VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit )
{
	// don't make noise for hidden/invisible/sky materials
	surfacedata_t *phit = physprops->GetSurfaceData( surfacePropsHit );
	surfacedata_t *pprops = physprops->GetSurfaceData( surfaceProps );
	if ( phit->gameMaterial == 'X' || pprops->gameMaterial == 'X' )
		return;

	PhysFrictionSound( this, pObject, energy, surfaceProps );
}


void CBaseEntity::VPhysicsSwapObject( IPhysicsObject *pSwap )
{
	if ( !pSwap )
	{
		PhysRemoveShadow(this);
	}

	if ( !m_pPhysicsObject )
	{
		Warning( "Bad vphysics swap for %s\n", STRING(m_iClassname) );
	}
	m_pPhysicsObject = pSwap;
}


// Tells the physics shadow to update it's target to the current position
void CBaseEntity::UpdatePhysicsShadowToCurrentPosition( float deltaTime )
{
	if ( GetMoveType() != MOVETYPE_VPHYSICS )
	{
		IPhysicsObject *pPhys = VPhysicsGetObject();
		if ( pPhys )
		{
			pPhys->UpdateShadow( GetAbsOrigin(), GetAbsAngles(), false, deltaTime );
		}
	}
}

int CBaseEntity::VPhysicsGetObjectList( IPhysicsObject **pList, int listMax )
{
	IPhysicsObject *pPhys = VPhysicsGetObject();
	if ( pPhys )
	{
		// multi-object entities must implement this function
		Assert( !(pPhys->GetGameFlags() & FVPHYSICS_MULTIOBJECT_ENTITY) );
		if ( listMax > 0 )
		{
			pList[0] = pPhys;
		}
	}
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: Default PVS check ( checks pev against the pvs for sending to pPlayer )
// Input  : *pPlayer - 
//			*pvs - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::IsInPVS( const edict_t *pPlayer, const void *pvs )
{
	// ignore if not touching a PV leaf
	// negative leaf count is a node number
	// If no pvs, add any entity
	if ( pvs && ( pev != pPlayer ) )
	{
		unsigned char *pPVS = ( unsigned char * )pvs;

		if ( pev->clusterCount < 0 )   // too many clusters, use headnode
		{
			if ( !engine->CheckHeadnodeVisible( pev->headnode, pPVS ) )
				return false;
		}
		else
		{
			int i;
			for ( i = 0; i < pev->clusterCount; i++ )
			{
				if (pPVS[pev->clusters[i] >> 3] & (1 << (pev->clusters[i]&7) ))
					break;
			}

			if ( i >= pev->clusterCount )
				return false;		// not visible
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Initialize absmin & absmax to the appropriate box
//-----------------------------------------------------------------------------
void CBaseEntity::ComputeSurroundingBox( void )
{
	Vector vecAbsMins, vecAbsMaxs;
	WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );
	SetAbsMins( vecAbsMins );
	SetAbsMaxs( vecAbsMaxs );
}


int	CBaseEntity::Intersects( CBaseEntity *pOther )
{
	if ( !pev || !pOther->pev )
		return 0;

	return QuickBoxIntersectTest( pOther->GetAbsMins(), pOther->GetAbsMaxs(), GetAbsMins(), GetAbsMaxs() );
}




//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target
//=========================================================
bool CBaseEntity::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	if ( pEntity->GetFlags() & FL_NOTARGET )
		return false;

	// don't look through water
	if ((m_nWaterLevel != 3 && pEntity->m_nWaterLevel == 3) 
		|| (m_nWaterLevel == 3 && pEntity->m_nWaterLevel == 0))
		return false;

	Vector vecLookerOrigin = EyePosition();//look through the caller's 'eyes'
	Vector vecTargetOrigin = pEntity->EyePosition();

	trace_t tr;
	UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, traceMask, this, COLLISION_GROUP_NONE, &tr);
	
	if (tr.fraction != 1.0)
	{
		if (ppBlocker)
		{
			*ppBlocker = tr.m_pEnt;
		}
		return false;// Line of sight is not established
	}

	return true;// line of sight is valid.
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the wished position.
//=========================================================
bool CBaseEntity::FVisible( const Vector &vecTarget, int traceMask, CBaseEntity **ppBlocker )
{
#if 0
	// don't look through water
	// Change to use Point Contents
	if ((m_nWaterLevel != 3 && pEntity->m_nWaterLevel == 3) 
		|| (m_nWaterLevel == 3 && pEntity->m_nWaterLevel == 0))
		return false;
#endif 

	Vector vecLookerOrigin = EyePosition();//look through the caller's 'eyes'

	trace_t tr;
	UTIL_TraceLine(vecLookerOrigin, vecTarget, traceMask, this, COLLISION_GROUP_NONE, &tr);
	
	if (tr.fraction != 1.0)
	{
		if (ppBlocker)
		{
			*ppBlocker = tr.m_pEnt;
		}
		return false;// Line of sight is not established
	}

	return true;// line of sight is valid.
}



Class_T CBaseEntity::Classify ( void )
{ 
	return CLASS_NONE;
}

/*
================
TraceAttack
================
*/

//-----------------------------------------------------------------------------
// Purpose: Returns whether an attacker can damage the entity
//-----------------------------------------------------------------------------
bool CBaseEntity::PassesDamageFilter( CBaseEntity *pAttacker )
{
	if (m_hDamageFilter)
	{
		CBaseFilter *pFilter = (CBaseFilter *)(m_hDamageFilter.Get());
		return pFilter->PassesFilter(pAttacker);
	}

	return true;
}

void CBaseEntity::DispatchTraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	// Make sure our damage filter allows the damage.
	if ( !PassesDamageFilter( info.GetAttacker() ))
	{
		return;
	}

	TraceAttack( info, vecDir, ptr );
}

void CBaseEntity::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	Vector vecOrigin = ptr->endpos - vecDir * 4;

	if ( m_takedamage )
	{
		AddMultiDamage( info, this );

		int blood = BloodColor();
		
		if ( blood != DONT_BLEED )
		{
			SpawnBlood( vecOrigin, blood, info.GetDamage() );// a little surface blood.
			TraceBleed( info.GetDamage(), vecDir, ptr, info.GetDamageType() );
		}
	}
}


void CBaseEntity::MakeDormant( void )
{
	m_iEFlags |= EFL_DORMANT;

	// disable thinking for dormant entities
	SetThink( NULL );

	if ( !pev )
		return;

	SETBITS( m_iEFlags, EFL_DORMANT );
	
	// Don't touch
	AddSolidFlags( FSOLID_NOT_SOLID );
	// Don't move
	SetMoveType( MOVETYPE_NONE );
	// Don't draw
	SETBITS( m_fEffects, EF_NODRAW );
	// Don't think
	SetNextThink( TICK_NEVER_THINK );
	// Relink
	Relink();
}

int CBaseEntity :: IsDormant( void )
{
	return m_iEFlags & EFL_DORMANT;
}


bool CBaseEntity::IsInWorld( void ) const
{  
	if ( !pev )
		return true;

	// position 
	if (GetAbsOrigin().x >= MAX_COORD_INTEGER) return false;
	if (GetAbsOrigin().y >= MAX_COORD_INTEGER) return false;
	if (GetAbsOrigin().z >= MAX_COORD_INTEGER) return false;
	if (GetAbsOrigin().x <= MIN_COORD_INTEGER) return false;
	if (GetAbsOrigin().y <= MIN_COORD_INTEGER) return false;
	if (GetAbsOrigin().z <= MIN_COORD_INTEGER) return false;
	// speed
	if (GetAbsVelocity().x >= 2000) return false;
	if (GetAbsVelocity().y >= 2000) return false;
	if (GetAbsVelocity().z >= 2000) return false;
	if (GetAbsVelocity().x <= -2000) return false;
	if (GetAbsVelocity().y <= -2000) return false;
	if (GetAbsVelocity().z <= -2000) return false;

	return true;
}


bool CBaseEntity::IsViewable( void )
{
	if (m_fEffects & EF_NODRAW)
	{
		return false;
	}

	if (IsBSPModel())
	{
		if (GetMoveType() != MOVETYPE_NONE)
		{
			return true;
		}
	}
	else if (GetModelIndex() != 0)
	{
		// check for total transparency???
		return true;
	}
	return false;
}


int CBaseEntity::ShouldToggle( USE_TYPE useType, int currentState )
{
	if ( useType != USE_TOGGLE && useType != USE_SET )
	{
		if ( (currentState && useType == USE_ON) || (!currentState && useType == USE_OFF) )
			return 0;
	}
	return 1;
}


// NOTE: szName must be a pointer to constant memory, e.g. "NPC_class" because the entity
// will keep a pointer to it after this call.
CBaseEntity *CBaseEntity::Create( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CBaseEntity *pEntity = CreateNoSpawn( szName, vecOrigin, vecAngles, pOwner );

	DispatchSpawn( pEntity );
	return pEntity;
}



// NOTE: szName must be a pointer to constant memory, e.g. "NPC_class" because the entity
// will keep a pointer to it after this call.
CBaseEntity * CBaseEntity::CreateNoSpawn( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	IServerNetworkable *pNet = CreateEntityByName( szName );
	if ( !pNet )
	{
		Msg( "NULL Ent in Create!\n" );
		return NULL;
	}

	CBaseEntity *pEntity = pNet->GetBaseEntity();
	if ( !pEntity )
	{
		Assert( !"CreateNoSpawn: only works for CBaseEntities" );
		return NULL;
	}

	pEntity->SetLocalOrigin( vecOrigin );
	pEntity->SetLocalAngles( vecAngles );
	pEntity->SetOwnerEntity( pOwner );
	return pEntity;
}

Vector CBaseEntity::GetSoundEmissionOrigin() const
{
	return WorldSpaceCenter();
}


//-----------------------------------------------------------------------------
// Purpose: Saves the current object out to disk, by iterating through the objects
//			data description hierarchy
// Input  : &save - save buffer which the class data is written to
// Output : int	- 0 if the save failed, 1 on success
//-----------------------------------------------------------------------------
int CBaseEntity::Save( ISave &save )
{
	// loop through the data description list, saving each data desc block
	int status = SaveDataDescBlock( save, GetDataDescMap() );

	return status;
}

//-----------------------------------------------------------------------------
// Purpose: Recursively saves all the classes in an object, in reverse order (top down)
// Output : int 0 on failure, 1 on success
//-----------------------------------------------------------------------------
int CBaseEntity::SaveDataDescBlock( ISave &save, datamap_t *dmap )
{
	return save.WriteAll( this, dmap );
}

//-----------------------------------------------------------------------------
// Purpose: Restores the current object from disk, by iterating through the objects
//			data description hierarchy
// Input  : &restore - restore buffer which the class data is read from
// Output : int	- 0 if the restore failed, 1 on success
//-----------------------------------------------------------------------------
int CBaseEntity::Restore( IRestore &restore )
{
	// loops through the data description list, restoring each data desc block in order
	int status = RestoreDataDescBlock( restore, GetDataDescMap() );

	// if we have an attached edict, restore those fields
	if ( pev )
	{
		pev->classname = m_iClassname;
	}

    if ( pev && GetModelIndex() != 0 && GetModelName() != NULL_STRING && restore.GetPrecacheMode() )
	{
		Vector mins, maxs;
		if (!IsBoundsDefinedInEntitySpace())
		{
			mins = WorldAlignMins();	// Set model is about to destroy these
			maxs = WorldAlignMaxs();
		}
		else
		{
			mins = EntitySpaceMins();	// Set model is about to destroy these
			maxs = EntitySpaceMaxs();
		}

		engine->PrecacheModel( STRING( GetModelName() ) );
		SetModel( STRING( GetModelName() ) );
		UTIL_SetSize(this, mins, maxs);	// Reset them
	}

	// ---------------------------------------------------------------
	// HACKHACK: We don't know the space of these vectors until now
	// if they are worldspace, fix them up.
	// ---------------------------------------------------------------
	{
		CGameSaveRestoreInfo *pGameInfo = restore.GetGameSaveRestoreInfo();
		Vector parentSpaceOffset = pGameInfo->modelSpaceOffset;
		if ( !GetParent() )
		{
			// parent is the world, so parent space is worldspace
			// so update with the worldspace leveltransition transform
			parentSpaceOffset += pGameInfo->GetLandmark();
		}
		
		Vector position;
		MatrixPosition( m_rgflCoordinateFrame, position );
		position += parentSpaceOffset;
		PositionMatrix( position,  m_rgflCoordinateFrame );
		m_vecOrigin += parentSpaceOffset;
	}

	return status;
}


//-----------------------------------------------------------------------------
// handler to do stuff before you are saved
//-----------------------------------------------------------------------------
void CBaseEntity::OnSave()
{
	// Here, we must force recomputation of all abs data so it gets saved correctly
	// We can't leave the dirty bits set because the loader can't cope with it.
	CalcAbsolutePosition();
	CalcAbsoluteVelocity();
}


//-----------------------------------------------------------------------------
// handler to do stuff after you are restored
//-----------------------------------------------------------------------------
void CBaseEntity::OnRestore()
{
}


//-----------------------------------------------------------------------------
// Purpose: Recursively restores all the classes in an object, in reverse order (top down)
// Output : int 0 on failure, 1 on success
//-----------------------------------------------------------------------------
int CBaseEntity::RestoreDataDescBlock( IRestore &restore, datamap_t *dmap )
{
	return restore.ReadAll( this, dmap );
}

//-----------------------------------------------------------------------------

bool CBaseEntity::ShouldSavePhysics()
{
	return true;
}

//-----------------------------------------------------------------------------

#include "tier0/memdbgoff.h"

//-----------------------------------------------------------------------------
// CBaseEntity new/delete
// allocates and frees memory for itself from the engine->
// All fields in the object are all initialized to 0.
//-----------------------------------------------------------------------------
void *CBaseEntity::operator new( size_t stAllocateBlock )
{
	// call into engine to get memory
	Assert( stAllocateBlock != 0 );
	return engine->PvAllocEntPrivateData(stAllocateBlock);
};

void *CBaseEntity::operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )
{
	// call into engine to get memory
	Assert( stAllocateBlock != 0 );
	return engine->PvAllocEntPrivateData(stAllocateBlock);
}

void CBaseEntity::operator delete( void *pMem )
{
	// get the engine to free the memory
	engine->FreeEntPrivateData( pMem );
}

#include "tier0/memdbgon.h"


#ifdef _DEBUG
void CBaseEntity::FunctionCheck( void *pFunction, char *name )
{ 
#ifdef USES_SAVERESTORE
	// Note, if you crash here and your class is using multiple inheritance, it is
	// probably the case that CBaseEntity (or a descendant) is not the first
	// class in your list of ancestors, which it must be.
	if (pFunction && !UTIL_FunctionToName( GetDataDescMap(), pFunction ) )
	{
		Warning( "FUNCTION NOT IN TABLE!: %s:%s (%08lx)\n", STRING(m_iClassname), name, (unsigned long)pFunction );
		Assert(0);
	}
#endif
}
#endif


bool CBaseEntity::IsTransparent() const
{
	return m_nRenderMode != kRenderNormal;
}


bool CBaseEntity::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	return false;
}

//-----------------------------------------------------------------------------
// Perform hitbox test, returns true *if hitboxes were tested at all*!!
//-----------------------------------------------------------------------------
bool CBaseEntity::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	return false;
}


bool CBaseEntity::TestCollisionBox( const Ray_t &ray, unsigned int mask, trace_t& trace, const Vector &absmins, const Vector &absmaxs )
{
	Vector bmin = absmins;
	Vector bmax = absmaxs;

	if ( !ray.m_IsRay )
	{
		bmax += ray.m_Extents;
		bmin -= ray.m_Extents;
	}

	int hitside;
	bool startsolid;
	float fraction;
	if ( IntersectRayWithBox( ray.m_Start, ray.m_Delta, bmin, bmax, DIST_EPSILON, fraction, hitside, startsolid ) )
	{
		trace.startsolid = startsolid;
		trace.fraction = fraction;
		trace.endpos = (ray.m_Start + fraction * ray.m_Delta) + ray.m_StartOffset;
		trace.contents = CONTENTS_SOLID;
		float sign = -1.0;
		if ( hitside >= 3 )
		{
			hitside -= 3;
			sign = 1.0;
			trace.plane.dist = absmaxs[hitside];
		}
		else
		{
			trace.plane.dist = -absmins[hitside];
		}
		trace.plane.normal = vec3_origin;
		trace.plane.normal[hitside] = sign;

		return true;
	}

	return false;
}


void CBaseEntity::SetCollisionGroup( int collisionGroup )
{
	if ( (int)m_CollisionGroup != collisionGroup )
	{
		m_CollisionGroup = collisionGroup;
		if ( VPhysicsGetObject() )
		{
			VPhysicsGetObject()->RecheckCollisionFilter();
		}
	}
}


void CBaseEntity::SetOwnerEntity( CBaseEntity* pOwner )
{
	if ( m_hOwnerEntity.Get() != pOwner )
	{
		m_hOwnerEntity = pOwner;

		// ivp maintains state based on recent return values from the collision filter, so anything
		// that can change the state that a collision filter will return (like m_Solid) needs to call RecheckCollisionFilter.
		IPhysicsObject *pObj = VPhysicsGetObject();
		if ( pObj )
			pObj->RecheckCollisionFilter();
	}
}


void CBaseEntity::SetMoveType( MoveType_t val, MoveCollide_t moveCollide )
{
#ifdef _DEBUG
	// Make sure the move type + move collide are compatible...
	if ((val != MOVETYPE_FLY) && (val != MOVETYPE_FLYGRAVITY))
	{
		Assert( moveCollide == MOVECOLLIDE_DEFAULT );
	}

	if ( m_MoveType == MOVETYPE_VPHYSICS && val != m_MoveType )
	{
		if ( VPhysicsGetObject() && val != MOVETYPE_NONE )
		{
			// What am I supposed to do with the physics object if
			// you're changing away from MOVETYPE_VPHYSICS without making the object 
			// shadow?  This isn't likely to work, assert.
			// You probably meant to call VPhysicsInitShadow() instead of VPhysicsInitNormal()!
			Assert( VPhysicsGetObject()->GetShadowController() );
		}
	}
#endif

	if ( m_MoveType == val )
	{
		m_MoveCollide = moveCollide;
		return;
	}

	// This is needed to the removal of MOVETYPE_FOLLOW:
	// We can't transition from follow to a different movetype directly
	// or the leaf code will break.
	Assert( (m_fEffects & EF_BONEMERGE) == 0 );
	m_MoveType = val;
	m_MoveCollide = moveCollide;

	// ivp maintains state based on recent return values from the collision filter, so anything
	// that can change the state that a collision filter will return (like m_Solid) needs to call RecheckCollisionFilter.
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->RecheckCollisionFilter();


	switch( m_MoveType )
	{
	case MOVETYPE_WALK:
		{
			SetSimulatedEveryTick( true );
			SetAnimatedEveryTick( true );
		}
		break;
	case MOVETYPE_STEP:
		{
			SetSimulatedEveryTick( false );
			SetAnimatedEveryTick( false );
		}
		break;
	default:
		{
			SetSimulatedEveryTick( true );
			SetAnimatedEveryTick( false );
		}
	}
}


void CBaseEntity::Spawn( void ) 
{
}


void CBaseEntity::SetRefEHandle( const CBaseHandle &handle )
{
	m_RefEHandle = handle;
}


const CBaseHandle& CBaseEntity::GetRefEHandle() const
{
	return m_RefEHandle;
}


CBaseEntity* CBaseEntity::Instance( const CBaseHandle &hEnt )
{
	return gEntList.GetBaseEntity( hEnt );
}


//-----------------------------------------------------------------------------
// Purpose: Note, an entity can override the send table ( e.g., to send less data or to send minimal data for
//  objects ( prob. players ) that are not in the pvs.
// Input  : **ppSendTable - 
//			*recipient - 
//			*pvs - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::ShouldTransmit( const edict_t *recipient, 
								  const void *pvs, int clientArea )
{
	if ( m_fEffects & EF_NODRAW )
		return false;

	if ( !(m_iEFlags & EFL_FORCE_CHECK_TRANSMIT) )
	{
		if ( !GetModelIndex() || !GetModelName() )
			return false;
	}

	// Always send the world
	if ( GetModelIndex() == 1 )
		return true;

	bool bTeamTransmit = false;
	
	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( recipient );
	if ( pRecipientEntity->IsPlayer() )
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer*>( pRecipientEntity );

		// Always send entities in the player's 3d skybox.  
		if ( pev->areanum == pPlayer->m_Local.m_skybox3d.area )
			return true;

		// FIXME: Refactor once notion of "team" is moved into HL2 code
		// Team rules may tell us that we should
		if ( pPlayer->GetTeam() ) 
		{
			if (pPlayer->GetTeam()->ShouldTransmitToPlayer( pPlayer, this ))
				bTeamTransmit = true;
		}
	}

#ifdef TF2_DLL
	// Check test network vis distance stuff. Eventually network LOD will do this.
	float flTestDistSqr = pRecipientEntity->GetAbsOrigin().DistToSqr( WorldSpaceCenter() );
	if ( flTestDistSqr > sv_netvisdist.GetFloat() * sv_netvisdist.GetFloat() )
		return false;
#endif

	// If the team said to transmit it, do so, but check the distance first.
	if ( bTeamTransmit )
		return true;

	if ( !IsEFlagSet( EFL_IN_SKYBOX ) )
	{
		if ( !engine->CheckAreasConnected( clientArea, pev->areanum) )
		{	
			// doors can legally straddle two areas, so
			// we may need to check another one
			if ( !pev->areanum2 || !engine->CheckAreasConnected( clientArea, pev->areanum2 ) )
			{
				return false;		// blocked by a door
			}
		}

		// Can we be seen?
		if ( !IsInPVS( recipient, pvs ) )
			return false;
	}

	return true;
}


void CBaseEntity::SetTransmitProxy( CBaseTransmitProxy *pProxy )
{
	if ( m_pTransmitProxy )
		m_pTransmitProxy->Release();

	m_pTransmitProxy = pProxy;
	if ( m_pTransmitProxy )
		m_pTransmitProxy->AddRef();
}


void CBaseEntity::CheckTransmit( CCheckTransmitInfo *pInfo )
{
	bool bShouldTransmit = ShouldTransmit( pInfo->m_pClientEnt, pInfo->m_pPVS, pInfo->m_iArea );
	
	// If we have a transmit proxy, let it hook our ShouldTransmit return value.
	if ( m_pTransmitProxy )
		bShouldTransmit = m_pTransmitProxy->ShouldTransmit( pInfo->m_pClientEnt, pInfo->m_pPVS, pInfo->m_iArea, bShouldTransmit );

	if ( bShouldTransmit )
		SetTransmit( pInfo );
}

void CBaseEntity::SetTransmit( CCheckTransmitInfo *pInfo )
{
	// Are we already marked for transmission?
	if ( pInfo->WillTransmit( entindex() ) )
		return;

	pInfo->SetTransmit( entindex() );

	// Force our aiment and move parent to be sent.
	if ( GetMoveParent() )
	{
		GetMoveParent()->SetTransmit( pInfo );
	}
}


bool CBaseEntity::DetectInSkybox()
{
	int area = engine->GetArea( WorldSpaceCenter() );

	CSkyCamera *pCur = CSkyList::m_pSkyCameras;
	while ( pCur )
	{
		if ( engine->CheckAreasConnected( area, pCur->m_skyboxData.area ) )
		{
			m_iEFlags |= EFL_IN_SKYBOX;
			return true;
		}

		pCur = pCur->m_pNextSkyCamera;
	}

	m_iEFlags &= ~EFL_IN_SKYBOX;
	return false;
}


CBaseNetworkable* CBaseEntity::GetBaseNetworkable()
{
	return NULL;
}


CBaseEntity* CBaseEntity::GetBaseEntity()
{
	return this;
}


//-----------------------------------------------------------------------------
// Purpose: Initialize absmin & absmax to the appropriate box
//-----------------------------------------------------------------------------
void CBaseEntity::SetObjectCollisionBox( void )
{
	if ( !pev )
		return;

	if ( GetSolid() == SOLID_BSP || GetSolid() == SOLID_VPHYSICS )
	{
		if ( GetMoveType() == MOVETYPE_VPHYSICS )
		{
			IPhysicsObject *pPhysics = VPhysicsGetObject();
			
			// UNDONE: This object is most likely marked for delete 
			// and still getting called back before the delete is flushed
			if ( !pPhysics )
				return;

			// UNDONE: This may not be necessary any more.
			if ( pPhysics->IsAsleep() )
			{
				Vector absmins, absmaxs;

				physcollision->CollideGetAABB( absmins, absmaxs, pPhysics->GetCollide(), GetAbsOrigin(), GetAbsAngles() );
				// BSP models have been shrunk 0.25 inches
				SetAbsMins( absmins - Vector( 0.25, 0.25, 0.25 ) );
				SetAbsMaxs( absmaxs + Vector( 0.25, 0.25, 0.25 ) );

				return;
			}
		}

		ComputeSurroundingBox();
	}
	else
	{
		// FIXME: Make a tighter box
		ComputeSurroundingBox();
	}

	if ( GetSolid() == SOLID_BBOX )
	{
		SetAbsMins( GetAbsMins() + Vector( -1, -1, -1 ) );
		SetAbsMaxs( GetAbsMaxs() + Vector( 1, 1, 1 ) );
	}
}


//------------------------------------------------------------------------------
// Purpose : If name exists returns name, otherwise returns classname
// Input   :
// Output  :
//------------------------------------------------------------------------------
const char *CBaseEntity::GetDebugName(void)
{
	if ( m_iName != NULL_STRING ) 
	{
		return STRING(m_iName);
	}
	else
	{
		return STRING(m_iClassname);
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseEntity::DrawInputOverlay(const char *szInputName, CBaseEntity *pCaller, variant_t Value)
{
	char bigstring[1024];
	if ( Value.FieldType() == FIELD_INTEGER )
	{
		Q_snprintf( bigstring,sizeof(bigstring), "%3.1f  (%s,%d) <-- (%s)\n", gpGlobals->curtime, szInputName, Value.Int(), pCaller ? pCaller->GetDebugName() : NULL);
	}
	else if ( Value.FieldType() == FIELD_STRING )
	{
		Q_snprintf( bigstring,sizeof(bigstring), "%3.1f  (%s,%s) <-- (%s)\n", gpGlobals->curtime, szInputName, Value.String(), pCaller ? pCaller->GetDebugName() : NULL);
	}
	else
	{
		Q_snprintf( bigstring,sizeof(bigstring), "%3.1f  (%s) <-- (%s)\n", gpGlobals->curtime, szInputName, pCaller ? pCaller->GetDebugName() : NULL);
	}
	AddTimedOverlay(bigstring, 10.0);

	if ( Value.FieldType() == FIELD_INTEGER )
	{
		DevMsg( 2, "input: (%s,%d) -> (%s,%s), from (%s)\n", szInputName, Value.Int(), STRING(m_iClassname), GetDebugName(), pCaller ? pCaller->GetDebugName() : NULL);
	}
	else if ( Value.FieldType() == FIELD_STRING )
	{
		DevMsg( 2, "input: (%s,%s) -> (%s,%s), from (%s)\n", szInputName, Value.String(), STRING(m_iClassname), GetDebugName(), pCaller ? pCaller->GetDebugName() : NULL);
	}
	else
		DevMsg( 2, "input: (%s) -> (%s,%s), from (%s)\n", szInputName, STRING(m_iClassname), GetDebugName(), pCaller ? pCaller->GetDebugName() : NULL);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseEntity::DrawOutputOverlay(CEventAction *ev)
{
	// Print to entity
	char bigstring[1024];
	if ( ev->m_flDelay )
	{
		Q_snprintf( bigstring,sizeof(bigstring), "%3.1f  (%s) --> (%s),%.1f) \n", gpGlobals->curtime, STRING(ev->m_iTargetInput), STRING(ev->m_iTarget), ev->m_flDelay);
	}
	else
	{
		Q_snprintf( bigstring,sizeof(bigstring), "%3.1f  (%s) --> (%s)\n", gpGlobals->curtime,  STRING(ev->m_iTargetInput), STRING(ev->m_iTarget));
	}
	AddTimedOverlay(bigstring, 10.0);

	// Now print to the console
	if ( ev->m_flDelay )
	{
		DevMsg( 2, "output: (%s,%s) -> (%s,%s,%.1f)\n", STRING(m_iClassname), GetDebugName(), STRING(ev->m_iTarget), STRING(ev->m_iTargetInput), ev->m_flDelay );
	}
	else
	{
		DevMsg( 2, "output: (%s,%s) -> (%s,%s)\n", STRING(m_iClassname), GetDebugName(), STRING(ev->m_iTarget), STRING(ev->m_iTargetInput) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: calls the appropriate message mapped function in the entity according
//			to the fired action.
// Input  : char *szInputName - input destination
//			*pActivator - entity which initiated this sequence of actions
//			*pCaller - entity from which this event is sent
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID )
{
	// loop through the data description list, restoring each data desc block
	for ( datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the actions in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			if ( dmap->dataDesc[i].flags & FTYPEDESC_INPUT )
			{
				if ( !stricmp(dmap->dataDesc[i].externalName, szInputName) )
				{
					// found a match

					// mapper debug message
					if (pCaller != NULL)
					{
						DevMsg( 2, "input %s: %s.%s(%s)\n", STRING(pCaller->m_iName), GetDebugName(), szInputName, Value.String());
					}
					else
					{
						DevMsg( 2, "input <NULL>: %s.%s(%s)\n", GetDebugName(), szInputName, Value.String());
					}

					if (m_debugOverlays & OVERLAY_MESSAGE_BIT)
					{
						DrawInputOverlay(szInputName,pCaller,Value);
					}

					// convert the value if necessary
					if ( Value.FieldType() != dmap->dataDesc[i].fieldType )
					{
						if ( !(Value.FieldType() == FIELD_VOID && dmap->dataDesc[i].fieldType == FIELD_STRING) ) // allow empty strings
						{
							if ( !Value.Convert( dmap->dataDesc[i].fieldType ) )
							{
								// bad conversion
								Warning( "!! ERROR: bad input/output link:\n!! %s(%s,%s) doesn't match type from %s(%s)\n", 
									STRING(m_iClassname), GetDebugName(), szInputName, 
									( pCaller != NULL ) ? STRING(pCaller->m_iClassname) : "<null>",
									( pCaller != NULL ) ? STRING(pCaller->m_iName) : "<null>" );
								return false;
							}
						}
					}

					// call the input handler, or if there is none just set the value
					inputfunc_t pfnInput = dmap->dataDesc[i].inputFunc;

					if ( pfnInput )
					{ 
						// Package the data into a struct for passing to the input handler.
						inputdata_t data;
						data.pActivator = pActivator;
						data.pCaller = pCaller;
						data.value = Value;
						data.nOutputID = outputID;

						(this->*pfnInput)( data );
					}
					else if ( dmap->dataDesc[i].flags & FTYPEDESC_KEY )
					{
						// set the value directly
						Value.SetOther( ((char*)this) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ]);
					}

					// If this is a manual-networked entity, then mark it dirty.
					NetworkStateChanged();

					return true;
				}
			}
		}
	}

	DevMsg( 2, "unhandled input: (%s) -> (%s,%s)\n", szInputName, STRING(m_iClassname), GetDebugName()/*,", from (%s,%s)" STRING(pCaller->m_iClassname), STRING(pCaller->m_iName)*/ );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for the entity alpha.
// Input  : nAlpha - Alpha value (0 - 255).
//-----------------------------------------------------------------------------
void CBaseEntity::InputAlpha( inputdata_t &inputdata )
{
	SetRenderColorA( clamp( inputdata.value.Int(), 0, 255 ) );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for the entity color. Ignores alpha since that is handled
//			by a separate input handler.
// Input  : Color32 new value for color (alpha is ignored).
//-----------------------------------------------------------------------------
void CBaseEntity::InputColor( inputdata_t &inputdata )
{
	color32 clr = inputdata.value.Color32();

	SetRenderColor( clr.r, clr.g, clr.b );
}


//-----------------------------------------------------------------------------
// Purpose: Called whenever the entity is 'Used'.  This can be when a player hits
//			use, or when an entity targets it without an output name (legacy entities)
//-----------------------------------------------------------------------------
void CBaseEntity::InputUse( inputdata_t &inputdata )
{
	Use( inputdata.pActivator, inputdata.pCaller, (USE_TYPE)inputdata.nOutputID, 0 );
}


//-----------------------------------------------------------------------------
// Purpose: Reads an output variable, by string name, from an entity
// Input  : char *varName - the string name of the variable
//			variant_t *var - the value is stored here
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::ReadKeyField( const char *varName, variant_t *var )
{
	if ( !varName )
		return false;

	// loop through the data description list, restoring each data desc block
	for ( datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the readable fields in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			if ( dmap->dataDesc[i].flags & (FTYPEDESC_OUTPUT | FTYPEDESC_KEY) )
			{
				if ( !stricmp(dmap->dataDesc[i].externalName, varName) )
				{
					var->Set( dmap->dataDesc[i].fieldType, ((char*)this) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ] );
					return true;
				}
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the damage filter on the object
//-----------------------------------------------------------------------------
void CBaseEntity::InputSetDamageFilter( inputdata_t &inputdata )
{
	// Get a handle to my damage filter entity if there is one.
	m_iszDamageFilterName = inputdata.value.StringID();
	if ( m_iszDamageFilterName != NULL_STRING )
	{
		m_hDamageFilter = gEntList.FindEntityByName( NULL, m_iszDamageFilterName, NULL );
	}
	else
	{
		m_hDamageFilter = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dispatch effects on this entity
//-----------------------------------------------------------------------------
void CBaseEntity::InputDispatchEffect( inputdata_t &inputdata )
{
	const char *sEffect = inputdata.value.String();
	if ( sEffect && sEffect[0] )
	{
		CEffectData data;
		GetInputDispatchEffectPosition( sEffect, data.m_vOrigin, data.m_vAngles );
		AngleVectors( data.m_vAngles, &data.m_vNormal );
		data.m_vStart = data.m_vOrigin;
		data.m_nEntIndex = entindex();

		// Clip off leading attachment point numbers
		while ( sEffect[0] >= '0' && sEffect[0] <= '9' )
		{
			sEffect++;
		}
		DispatchEffect( sEffect, data );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the origin at which to play an inputted dispatcheffect 
//-----------------------------------------------------------------------------
void CBaseEntity::GetInputDispatchEffectPosition( const char *sInputString, Vector &pOrigin, QAngle &pAngles )
{
	pOrigin = GetAbsOrigin();
	pAngles = GetAbsAngles();
}

//-----------------------------------------------------------------------------
// Purpose: Marks the entity for deletion
//-----------------------------------------------------------------------------
void CBaseEntity::InputKill( inputdata_t &inputdata )
{
	// tell owner ( if any ) that we're dead.This is mostly for NPCMaker functionality.
	CBaseEntity *pOwner = GetOwnerEntity();
	
	if ( pOwner )
	{
		pOwner->DeathNotice( this );
	}

	UTIL_Remove( this );
}


//------------------------------------------------------------------------------
// Purpose: Input handler for changing this entity's movement parent.
//------------------------------------------------------------------------------
void CBaseEntity::InputSetParent( inputdata_t &inputdata )
{
	SetParent( inputdata.value.StringID(), inputdata.pActivator );
}


//------------------------------------------------------------------------------
// Purpose: Input handler for clearing this entity's movement parent.
//------------------------------------------------------------------------------
void CBaseEntity::InputClearParent( inputdata_t &inputdata )
{
	SetParent( NULL );
}


//------------------------------------------------------------------------------
// Purpose : Returns velcocity of base entity.  If physically simulated gets
//			 velocity from physics object
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseEntity::GetVelocity(Vector *vVelocity, QAngle *vAngVelocity)
{
	if (GetMoveType()==MOVETYPE_VPHYSICS)
	{
		if ( vAngVelocity )
		{
			AngularImpulse tmp;
			m_pPhysicsObject->GetVelocity(vVelocity,&tmp);
			AngularImpulseToQAngle( tmp, *vAngVelocity );
		}
		else
		{
			m_pPhysicsObject->GetVelocity(vVelocity, NULL);
		}
	}
	else
	{
		if (vVelocity != NULL)
		{
			*vVelocity = GetAbsVelocity();
		}
		if (vAngVelocity != NULL)
		{
			*vAngVelocity = GetLocalAngularVelocity();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Retrieves the coordinate frame for this entity.
// Input  : forward - Receives the entity's forward vector.
//			right - Receives the entity's right vector.
//			up - Receives the entity's up vector.
//-----------------------------------------------------------------------------
void CBaseEntity::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
{
	// This call is necessary to cause m_rgflCoordinateFrame to be recomputed
	const matrix3x4_t &entityToWorld = EntityToWorldTransform();

	if (pForward != NULL)
	{
		MatrixGetColumn( entityToWorld, 0, *pForward ); 
	}

	if (pRight != NULL)
	{
		MatrixGetColumn( entityToWorld, 1, *pRight ); 
		*pRight *= -1.0f;
	}

	if (pUp != NULL)
	{
		MatrixGetColumn( entityToWorld, 2, *pUp ); 
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the model, validates that it's of the appropriate type
// Input  : *szModelName - 
//-----------------------------------------------------------------------------
void CBaseEntity::SetModel( const char *szModelName )
{
	int modelIndex = modelinfo->GetModelIndex( szModelName );
	const model_t *model = modelinfo->GetModel( modelIndex );
	if ( model && modelinfo->GetModelType( model ) != mod_brush )
	{
		Msg( "Setting CBaseEntity to non-brush model %s\n", szModelName );
	}
	UTIL_SetModel( this, szModelName );
	NetworkStateChanged( );
}


//-----------------------------------------------------------------------------
// Purpose: Called once per frame after the server frame loop has finished and after all messages being
//  sent to clients have been sent.
//-----------------------------------------------------------------------------
void CBaseEntity::PostClientMessagesSent( void )
{
	// Remove muzzle flash and nointerp flags from entity after every frame
	m_fEffects &= ~EF_MUZZLEFLASH;
	m_fEffects &= ~EF_NOINTERP;
}

//================================================================================
// TEAM HANDLING
//================================================================================
void CBaseEntity::InputSetTeam( inputdata_t &inputdata )
{
	ChangeTeam( inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: Put the entity in the specified team
//-----------------------------------------------------------------------------
void CBaseEntity::ChangeTeam( int iTeamNum )
{
	m_iTeamNum = iTeamNum;
	NetworkStateChanged( );
}

//-----------------------------------------------------------------------------
// Get the Team this entity is on
//-----------------------------------------------------------------------------
CTeam *CBaseEntity::GetTeam( void ) const
{
	return GetGlobalTeam( m_iTeamNum );
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if these players are both in at least one team together
//-----------------------------------------------------------------------------
bool CBaseEntity::InSameTeam( CBaseEntity *pEntity ) const
{
	if ( !pEntity )
		return false;

	return ( pEntity->GetTeam() == GetTeam() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the string name of the players team
//-----------------------------------------------------------------------------
const char *CBaseEntity::TeamID( void ) const
{
	if ( GetTeam() == NULL )
		return "";

	return GetTeam()->GetName();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the player is on the same team
//-----------------------------------------------------------------------------
bool CBaseEntity::IsInTeam( CTeam *pTeam ) const
{
	return ( GetTeam() == pTeam );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseEntity::GetTeamNumber( void ) const
{
	return m_iTeamNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseEntity::IsInAnyTeam( void ) const
{
	return ( GetTeam() != NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the type of damage that this entity inflicts.
//-----------------------------------------------------------------------------
int CBaseEntity::GetDamageType() const
{
	return DMG_GENERIC;
}


//-----------------------------------------------------------------------------
// process notification
//-----------------------------------------------------------------------------

void CBaseEntity::NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
}


static void TeleportEntity_r( CBaseEntity *pTeleport, CBaseEntity *pSourceEntity, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	Vector prevOrigin = pTeleport->GetAbsOrigin();
	QAngle prevAngles = pTeleport->GetAbsAngles();

	int nSolidFlags = pTeleport->GetSolidFlags();
	pTeleport->AddSolidFlags( FSOLID_NOT_SOLID );

	// I'm teleporting myself
	if ( pSourceEntity == pTeleport )
	{
		if ( newAngles )
		{
			pTeleport->SetLocalAngles( *newAngles );
			if ( pTeleport->IsPlayer() )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)pTeleport;
				pPlayer->SnapEyeAngles( *newAngles );
			}
		}

		if ( newVelocity )
		{
			pTeleport->SetAbsVelocity( *newVelocity );
			pTeleport->SetBaseVelocity( vec3_origin );
		}

		if ( newPosition )
		{
			SETBITS( pTeleport->m_fEffects, EF_NOINTERP );
			UTIL_SetOrigin( pTeleport, *newPosition );
		}
	}
	else
	{
		// My parent is teleporting, just update my position & physics
		pTeleport->CalcAbsolutePosition();
	}
	IPhysicsObject *pPhys = pTeleport->VPhysicsGetObject();
	bool rotatePhysics = false;

	// handle physics objects / shadows
	if ( pPhys )
	{
		if ( newVelocity )
		{
			pPhys->SetVelocity( newVelocity, NULL );
		}
		const QAngle *rotAngles = &pTeleport->GetAbsAngles();
		// don't rotate physics on players or bbox entities
		if (pTeleport->IsPlayer() || pTeleport->GetSolid() == SOLID_BBOX )
		{
			rotAngles = &vec3_angle;
		}
		else
		{
			rotatePhysics = true;
		}

		pPhys->SetPosition( pTeleport->GetAbsOrigin(), *rotAngles, true );
	}

	g_pNotify->ReportTeleportEvent( pTeleport, prevOrigin, prevAngles, rotatePhysics );

	CBaseEntity *pList = pTeleport->FirstMoveChild();
	while ( pList )
	{
		TeleportEntity_r( pList, pSourceEntity, NULL, NULL, NULL );
		pList = pList->NextMovePeer();
	}

	pTeleport->SetSolidFlags( nSolidFlags );
}

static CUtlVector<CBaseEntity *> g_TeleportStack;
void CBaseEntity::Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	if ( g_TeleportStack.Find( this ) >= 0 )
		return;
	int index = g_TeleportStack.AddToTail( this );
	TeleportEntity_r( this, this, newPosition, newAngles, newVelocity );
	Assert( g_TeleportStack[index] == this );
	g_TeleportStack.FastRemove( index );
}

// Stuff implemented for weapon prediction code
void CBaseEntity::SetSize( const Vector &vecMin, const Vector &vecMax )
{
	UTIL_SetSize( this, vecMin, vecMax );
}

int CBaseEntity::PrecacheModel( const char *name )
{
	return engine->PrecacheModel( name );
}

int	CBaseEntity::PrecacheSound( const char *name )
{
	return enginesound->PrecacheSound( name );
}

void CBaseEntity::Remove( )
{
	UTIL_Remove( this );
}

// ################################################################################
//   Entity degugging console commands
// ################################################################################
extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
extern void			SetDebugBits( CBasePlayer* pPlayer, char *name, int bit );

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void ConsoleFireTargets( CBasePlayer *pPlayer, char *name)
{
	// If no name was given use the picker
	if (FStrEq(name,"")) 
	{
		CBaseEntity *pEntity = FindPickerEntity( pPlayer );
		if ( pEntity && !pEntity->IsMarkedForDeletion())
		{
			Msg( "[%03d] Found: %s, firing\n", gpGlobals->tickcount%1000, pEntity->GetDebugName());
			pEntity->Use( pPlayer, pPlayer, USE_TOGGLE, 0 );
			return;
		}
	}
	// Otherwise use name or classname
	FireTargets( name, pPlayer, pPlayer, USE_TOGGLE, 0 );
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Name( void )
{
	SetDebugBits(UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_NAME_BIT);
}
static ConCommand ent_name("ent_name", CC_Ent_Name, 0, FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_Text( void )
{
	SetDebugBits(UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_TEXT_BIT);
}
static ConCommand ent_text("ent_text", CC_Ent_Text, "Displays text debugging information about the given entity(ies) on top of the entity (See Overlay Text)\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_BBox( void )
{
	SetDebugBits(UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_BBOX_BIT);
}
static ConCommand ent_bbox("ent_bbox", CC_Ent_BBox, "Displays the movement bounding box for the given entity(ies) in orange.  Some entites will also display entity specific overlays.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);


//------------------------------------------------------------------------------
void CC_Ent_AbsBox( void )
{
	SetDebugBits(UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_ABSBOX_BIT);
}
static ConCommand ent_absbox("ent_absbox", CC_Ent_AbsBox, "Displays the total bounding box for the given entity(s) in green.  Some entites will also display entity specific overlays.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);


void CC_Ent_RBox( void )
{
	SetDebugBits(UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_RBOX_BIT);
}
static ConCommand ent_rbox("ent_rbox", CC_Ent_RBox, "Displays the total bounding box for the given entity(s) in green.  Some entites will also display entity specific overlays.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_Remove( void )
{
	CBaseEntity *pEntity = NULL;

	// If no name was given set bits based on the picked
	if ( FStrEq( engine->Cmd_Argv(1),"") ) 
	{
		pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	}
	else 
	{
		// Otherwise set bits based on name or classname
		CBaseEntity *ent = NULL;
		while ( (ent = gEntList.NextEnt(ent)) != NULL )
		{
			if (  (ent->GetEntityName() != NULL_STRING	&& FStrEq(engine->Cmd_Argv(1), STRING(ent->GetEntityName())))	|| 
				  (ent->m_iClassname != NULL_STRING	&& FStrEq(engine->Cmd_Argv(1), STRING(ent->m_iClassname))) ||
				  (ent->GetClassname()!=NULL && FStrEq(engine->Cmd_Argv(1), ent->GetClassname())))
			{
				pEntity = ent;
				break;
			}
		}
	}

	// Found one?
	if ( pEntity )
	{
		Msg( "Removed %s\n", STRING(pEntity->m_iClassname) );
		UTIL_Remove( pEntity );
	}
}
static ConCommand ent_remove("ent_remove", CC_Ent_Remove, "Removes the given entity(s)\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_RemoveAll( void )
{
	// If no name was given remove based on the picked
	if ( engine->Cmd_Argc() < 2 )
	{
		Msg( "Removes all entities of the specified type\n\tArguments:   	{entity_name} / {class_name}\n" );
	}
	else 
	{
		// Otherwise remove based on name or classname
		int iCount = 0;
		CBaseEntity *ent = NULL;
		while ( (ent = gEntList.NextEnt(ent)) != NULL )
		{
			if (  (ent->GetEntityName() != NULL_STRING	&& FStrEq(engine->Cmd_Argv(1), STRING(ent->GetEntityName())))	|| 
				  (ent->m_iClassname != NULL_STRING	&& FStrEq(engine->Cmd_Argv(1), STRING(ent->m_iClassname))) ||
				  (ent->GetClassname()!=NULL && FStrEq(engine->Cmd_Argv(1), ent->GetClassname())))
			{
				UTIL_Remove( ent );
				iCount++;
			}
		}

		if ( iCount )
		{
			Msg( "Removed %d %s's\n", iCount, engine->Cmd_Argv(1) );
		}
		else
		{
			Msg( "No %s found.\n", engine->Cmd_Argv(1) );
		}
	}
}
static ConCommand ent_remove_all("ent_remove_all", CC_Ent_RemoveAll, "Removes all entities of the specified type\n\tArguments:   	{entity_name} / {class_name} ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_SetName( void )
{
	CBaseEntity *pEntity = NULL;

	if ( engine->Cmd_Argc() < 1 )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
		if (!pPlayer)
			return;

		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:\n   ent_setname <new name> <entity name>\n" );
	}
	else
	{
		// If no name was given set bits based on the picked
		if ( FStrEq( engine->Cmd_Argv(2),"") ) 
		{
			pEntity = FindPickerEntity( UTIL_GetCommandClient() );
		}
		else 
		{
			// Otherwise set bits based on name or classname
			CBaseEntity *ent = NULL;
			while ( (ent = gEntList.NextEnt(ent)) != NULL )
			{
				if (  (ent->GetEntityName() != NULL_STRING	&& FStrEq(engine->Cmd_Argv(1), STRING(ent->GetEntityName())))	|| 
					  (ent->m_iClassname != NULL_STRING	&& FStrEq(engine->Cmd_Argv(1), STRING(ent->m_iClassname))) ||
					  (ent->GetClassname()!=NULL && FStrEq(engine->Cmd_Argv(1), ent->GetClassname())))
				{
					pEntity = ent;
					break;
				}
			}
		}

		// Found one?
		if ( pEntity )
		{
			Msg( "Set the name of %s to %s\n", STRING(pEntity->m_iClassname), engine->Cmd_Argv(1) );
			pEntity->SetName( AllocPooledString( engine->Cmd_Argv(1) ) );
		}
	}
}
static ConCommand ent_setname("ent_setname", CC_Ent_SetName, "Sets the targetname of the given entity(s)\n\tArguments:   	{new entity name} {entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
//------------------------------------------------------------------------------
void CC_Ent_Dump( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
	{
		return;
	}

	if ( engine->Cmd_Argc() < 2 )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:\n   ent_dump <entity name>\n" );
	}
	else
	{
		// iterate through all the ents of this name, printing out their details
		CBaseEntity *ent = NULL;
		bool bFound = false;
		while ( ( ent = gEntList.FindEntityByName(ent, engine->Cmd_Argv(1), NULL ) ) != NULL )
		{
			bFound = true;
			for ( datamap_t *dmap = ent->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
			{
				// search through all the actions in the data description, printing out details
				for ( int i = 0; i < dmap->dataNumFields; i++ )
				{
					variant_t var;
					if ( ent->ReadKeyField( dmap->dataDesc[i].externalName, &var) )
					{
						char buf[256];
						buf[0] = 0;
						switch( var.FieldType() )
						{
						case FIELD_STRING:
							Q_strncpy( buf, var.String() ,sizeof(buf));
							break;
						case FIELD_INTEGER:
							if ( var.Int() )
								Q_snprintf( buf,sizeof(buf), "%d", var.Int() );
							break;
						case FIELD_FLOAT:
							if ( var.Float() )
								Q_snprintf( buf,sizeof(buf), "%.2f", var.Float() );
							break;
						case FIELD_EHANDLE:
							{
								// get the entities name
								if ( var.Entity() )
								{
									Q_snprintf( buf,sizeof(buf), "%s", STRING(var.Entity()->GetEntityName()) );
								}
							}
							break;
						}

						// don't print out the duplicate keys
						if ( !stricmp("parentname",dmap->dataDesc[i].externalName) || !stricmp("targetname",dmap->dataDesc[i].externalName) )
							continue;

						// don't print out empty keys
						if ( buf[0] )
						{
							ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs("  %s: %s\n", dmap->dataDesc[i].externalName, buf) );
						}
					}
				}
			}
		}

		if ( !bFound )
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, "ent_dump: no such entity" );
		}
	}
}
static ConCommand ent_dump("ent_dump", CC_Ent_Dump, "Usage:\n   ent_dump <entity name>\n", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_FireTarget( void )
{
	ConsoleFireTargets(UTIL_GetCommandClient(),engine->Cmd_Argv(1));
}
static ConCommand firetarget("firetarget", CC_Ent_FireTarget, 0, FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Fire( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
	{
		return;
	}

	// fires a command from the console
	if ( engine->Cmd_Argc() < 2 )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:\n   ent_fire <target> [action] [value] [delay]\n" );
	}
	else
	{
		const char *target = "", *action = "Use";
		variant_t value;
		int delay = 0;

		target = STRING( AllocPooledString(engine->Cmd_Argv(1)) );
		if ( engine->Cmd_Argc() >= 3 )
		{
			action = STRING( AllocPooledString(engine->Cmd_Argv(2)) );
		}
		if ( engine->Cmd_Argc() >= 4 )
		{
			value.SetString( MAKE_STRING(engine->Cmd_Argv(3)) );
		}
		if ( engine->Cmd_Argc() >= 5 )
		{
			delay = atoi( engine->Cmd_Argv(4) );
		}

		g_EventQueue.AddEvent( target, action, value, delay, pPlayer, pPlayer );
	}
}
static ConCommand ent_fire("ent_fire", CC_Ent_Fire, "Usage:\n   ent_fire <target> [action] [value] [delay]\n", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Info( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
	{
		return;
	}
	
	if ( engine->Cmd_Argc() < 2 )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:\n   ent_info <class name>\n" );
	}
	else
	{
		// iterate through all the ents printing out their details
		CBaseEntity *ent = CreateEntityByName( engine->Cmd_Argv(1) );

		if ( ent )
		{
			datamap_t *dmap;
			for ( dmap = ent->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
			{
				// search through all the actions in the data description, printing out details
				for ( int i = 0; i < dmap->dataNumFields; i++ )
				{
					if ( dmap->dataDesc[i].flags & FTYPEDESC_OUTPUT )
					{
						ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs("  output: %s\n", dmap->dataDesc[i].externalName) );
					}
				}
			}

			for ( dmap = ent->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
			{
				// search through all the actions in the data description, printing out details
				for ( int i = 0; i < dmap->dataNumFields; i++ )
				{
					if ( dmap->dataDesc[i].flags & FTYPEDESC_INPUT )
					{
						ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs("  input: %s\n", dmap->dataDesc[i].externalName) );
					}
				}
			}

			delete ent;
		}
		else
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs("no such entity %s\n", engine->Cmd_Argv(1)) );
		}
	}
}
static ConCommand ent_info("ent_info", CC_Ent_Info, "Usage:\n   ent_info <class name>\n", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Messages( void )
{
	SetDebugBits(UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_MESSAGE_BIT);
}
static ConCommand ent_messages("ent_messages", CC_Ent_Messages ,"Toggles input/output message display for the selected entity(ies).  The name of the entity will be displayed as well as any messages that it sends or receives.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Pause( void )
{
	if (CBaseEntity::Debug_IsPaused())
	{
		CBaseEntity::Debug_Pause(false);
	}
	else
	{
		CBaseEntity::Debug_Pause(true);
	}
}
static ConCommand ent_pause("ent_pause", CC_Ent_Pause, "Toggles pausing of input/output message processing for entities.  When turned on processing of all message will stop.  Any messages displayed with 'ent_messages' will stop fading and be displayed indefinitely. To step through the messages one by one use 'ent_step'.", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose : Enables the entity picker, revelaing debug information about the 
//           entity under the crosshair.
// Input   : an optional command line argument "full" enables all debug info.
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Picker( void )
{
	CBaseEntity::m_bInDebugSelect = CBaseEntity::m_bInDebugSelect ? false : true;

	// Remember the player that's making this request
	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand picker("picker", CC_Ent_Picker, "Toggles 'picker' mode.  When picker is on, the bounding box, pivot and debugging text is displayed for whatever entity the player is looking at.\n\tArguments:	full - enables all debug information", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Pivot( void )
{
	SetDebugBits(UTIL_GetCommandClient(),engine->Cmd_Argv(1),OVERLAY_PIVOT_BIT);
}
static ConCommand ent_pivot("ent_pivot", CC_Ent_Pivot, "Displays the pivot for the given entity(ies).\n\t(y=up=green, z=forward=blue, x=left=red). \n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Step( void )
{
	int nSteps = atoi(engine->Cmd_Argv(1));
	if (nSteps <= 0)
	{
		nSteps = 1;
	}
	CBaseEntity::Debug_SetSteps(nSteps);
}
static ConCommand ent_step("ent_step", CC_Ent_Step, "When 'ent_pause' is set this will step through one waiting input / output message at a time.", FCVAR_CHEAT);


void CLogicalPointEntity::SetOwnerEntity( CBaseEntity* pOwner )
{
	// Blow off the owner...
}

CBaseEntity* CLogicalPointEntity::GetOwnerEntity( )
{
	return NULL;
}

bool CBaseEntity::IsCurrentlyTouching( void ) const
{
	if ( m_EntitiesTouched.nextLink != &m_EntitiesTouched )
	{
		return true;
	}

	return false;
}

void CBaseEntity::SetCheckUntouch( bool check )
{
	// Invalidate touchstamp
	if ( check )
	{
		touchStamp++;
		if ( !m_bCheckUntouch )
		{
			EntityTouch_Add( this );
		}
	}
	m_bCheckUntouch = check;
}

bool CBaseEntity::GetCheckUntouch() const
{
	return m_bCheckUntouch;
}

void CBaseEntity::SetSentLastFrame( bool wassent )
{
	m_bSentLastFrame = wassent;
}

bool CBaseEntity::GetSentLastFrame( void ) const
{
	return m_bSentLastFrame;
}

model_t *CBaseEntity::GetModel( void )
{
	return (model_t *)modelinfo->GetModel( GetModelIndex() );
}


//-----------------------------------------------------------------------------
// Purpose: Calculates the absolute position of an edict in the world
//			assumes the parent's absolute origin has already been calculated
//-----------------------------------------------------------------------------
void CBaseEntity::CalcAbsolutePosition( void )
{
	if (!IsEFlagSet( EFL_DIRTY_ABSTRANSFORM ))
		return;

	RemoveEFlags( EFL_DIRTY_ABSTRANSFORM );

	// Plop the entity->parent matrix into m_rgflCoordinateFrame
	AngleMatrix( m_angRotation, m_vecOrigin, m_rgflCoordinateFrame );

	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		// no move parent, so just copy existing values
		m_vecAbsOrigin = m_vecOrigin;
		m_angAbsRotation = m_angRotation;
		return;
	}

	// concatenate with our parent's transform
	matrix3x4_t tmpMatrix, scratchSpace;
	ConcatTransforms( GetParentToWorldTransform( scratchSpace ), m_rgflCoordinateFrame, tmpMatrix );
	MatrixCopy( tmpMatrix, m_rgflCoordinateFrame );

	// pull our absolute position out of the matrix
	MatrixGetColumn( m_rgflCoordinateFrame, 3, m_vecAbsOrigin ); 

	// if we have any angles, we have to extract our absolute angles from our matrix
	if ( m_angRotation == vec3_angle )
	{
		// just copy our parent's absolute angles
		VectorCopy( pMoveParent->GetAbsAngles(), m_angAbsRotation );
	}
	else
	{
		MatrixAngles( m_rgflCoordinateFrame, m_angAbsRotation );
	}
}

void CBaseEntity::CalcAbsoluteVelocity()
{
	if (!IsEFlagSet( EFL_DIRTY_ABSVELOCITY ))
		return;

	RemoveEFlags( EFL_DIRTY_ABSVELOCITY );

	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		m_vecAbsVelocity = m_vecVelocity;
		return;
	}

	// This transforms the local velocity into world space
	VectorRotate( m_vecVelocity, pMoveParent->EntityToWorldTransform(), m_vecAbsVelocity );

	// Now add in the parent abs velocity
	m_vecAbsVelocity += pMoveParent->GetAbsVelocity();
}

// FIXME: While we're using (dPitch, dYaw, dRoll) as our local angular velocity
// representation, we can't actually solve this problem
/*
void CBaseEntity::CalcAbsoluteAngularVelocity()
{
	if (!IsEFlagSet( EFL_DIRTY_ABSANGVELOCITY ))
		return;

	RemoveEFlags( EFL_DIRTY_ABSANGVELOCITY );

	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		m_vecAbsAngVelocity = m_vecAngVelocity;
		return;
	}

	// This transforms the local ang velocity into world space
	matrix3x4_t angVelToParent, angVelToWorld;
	AngleMatrix( m_vecAngVelocity, angVelToParent );
	ConcatTransforms( pMoveParent->EntityToWorldTransform(), angVelToParent, angVelToWorld );
	MatrixAngles( angVelToWorld, m_vecAbsAngVelocity );
}
*/

//-----------------------------------------------------------------------------
// Computes the abs position of a point specified in local space
//-----------------------------------------------------------------------------
void CBaseEntity::ComputeAbsPosition( const Vector &vecLocalPosition, Vector *pAbsPosition )
{
	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		*pAbsPosition = vecLocalPosition;
	}
	else
	{
		VectorTransform( vecLocalPosition, pMoveParent->EntityToWorldTransform(), *pAbsPosition );
	}
}


//-----------------------------------------------------------------------------
// Computes the abs position of a point specified in local space
//-----------------------------------------------------------------------------
void CBaseEntity::ComputeAbsDirection( const Vector &vecLocalDirection, Vector *pAbsDirection )
{
	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		*pAbsDirection = vecLocalDirection;
	}
	else
	{
		VectorRotate( vecLocalDirection, pMoveParent->EntityToWorldTransform(), *pAbsDirection );
	}
}


//-----------------------------------------------------------------------------
// Invalidates the abs state of all children
//-----------------------------------------------------------------------------
void CBaseEntity::InvalidatePhysicsRecursive( int nDirtyFlag, int nAdditionalChildFlags )
{
	AddEFlags( nDirtyFlag );
	nDirtyFlag |= nAdditionalChildFlags;
	for (CBaseEntity *pChild = FirstMoveChild(); pChild; pChild = pChild->NextMovePeer())
	{
		pChild->InvalidatePhysicsRecursive( nDirtyFlag );
	}
}


matrix3x4_t& CBaseEntity::GetParentToWorldTransform( matrix3x4_t &tempMatrix )
{
	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		Assert( false );
		SetIdentityMatrix( tempMatrix );
		return tempMatrix;
	}

	if ( m_iParentAttachment != 0 )
	{
		Vector vOrigin;
		QAngle vAngles;
		CBaseAnimating *pAnimating = pMoveParent->GetBaseAnimating();
		if ( pAnimating && pAnimating->GetAttachment( m_iParentAttachment, vOrigin, vAngles ) )
		{
			AngleMatrix( vAngles, vOrigin, tempMatrix );
			return tempMatrix;
		}
	}
	
	// If we fall through to here, then just use the move parent's abs origin and angles.
	return pMoveParent->EntityToWorldTransform();
}


//-----------------------------------------------------------------------------
// These methods recompute local versions as well as set abs versions
//-----------------------------------------------------------------------------
void CBaseEntity::SetAbsOrigin( const Vector& absOrigin )
{
	// This is necessary to get the other fields of m_rgflCoordinateFrame ok
	CalcAbsolutePosition();

	// All children are invalid, but we are not
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM );
	RemoveEFlags( EFL_DIRTY_ABSTRANSFORM );

	m_vecAbsOrigin = absOrigin;
	MatrixSetColumn( absOrigin, 3, m_rgflCoordinateFrame ); 

	Vector vecNewOrigin;
	CBaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		vecNewOrigin = absOrigin;
		SetSimulationTime( gpGlobals->curtime );
	}
	else
	{
		matrix3x4_t tempMat;
		matrix3x4_t &parentTransform = GetParentToWorldTransform( tempMat );

		// Moveparent case: transform the abs position into local space
		VectorITransform( absOrigin, parentTransform, vecNewOrigin );
	}

	if (m_vecOrigin != vecNewOrigin)
	{
		m_vecOrigin = vecNewOrigin;
		SetSimulationTime( gpGlobals->curtime );
		NetworkStateChanged();
	}
}

void CBaseEntity::SetAbsAngles( const QAngle& absAngles )
{
	// This is necessary to get the other fields of m_rgflCoordinateFrame ok
	CalcAbsolutePosition();

	// All children are invalid, but we are not
	// This will cause the velocities of all children to need recomputation
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM, EFL_DIRTY_ABSVELOCITY | EFL_DIRTY_ABSANGVELOCITY );
	RemoveEFlags( EFL_DIRTY_ABSTRANSFORM );

	m_angAbsRotation = absAngles;
	AngleMatrix( absAngles, m_rgflCoordinateFrame );
	MatrixSetColumn( m_vecAbsOrigin, 3, m_rgflCoordinateFrame ); 

	QAngle angNewRotation;
	CBaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		angNewRotation = absAngles;
		SetSimulationTime( gpGlobals->curtime );
	}
	else
	{
		// Moveparent case: transform the abs transform into local space
		matrix3x4_t worldToParent, localMatrix;
		MatrixInvert( pMoveParent->EntityToWorldTransform(), worldToParent );
		ConcatTransforms( worldToParent, m_rgflCoordinateFrame, localMatrix );
		MatrixAngles( localMatrix, angNewRotation );
	}

	if (m_angRotation != angNewRotation)
	{
		m_angRotation = angNewRotation;
		SetSimulationTime( gpGlobals->curtime );
		NetworkStateChanged();
	}
}

void CBaseEntity::SetAbsVelocity( const Vector &vecAbsVelocity )
{
	// The abs velocity won't be dirty since we're setting it here
	// All children are invalid, but we are not
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSVELOCITY );
	RemoveEFlags( EFL_DIRTY_ABSVELOCITY );

	m_vecAbsVelocity = vecAbsVelocity;

	// NOTE: Do *not* do a network state change in this case.
	// m_vecVelocity is only networked for the player, which is not manual mode
	CBaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		m_vecVelocity = vecAbsVelocity;
		return;
	}

	// First subtract out the parent's abs velocity to get a relative
	// velocity measured in world space
	Vector relVelocity;
	VectorSubtract( vecAbsVelocity, pMoveParent->GetAbsVelocity(), relVelocity );

	// Transform relative velocity into parent space
	Vector vNew;
	VectorIRotate( relVelocity, pMoveParent->EntityToWorldTransform(), vNew );
	m_vecVelocity = vNew;
}

// FIXME: While we're using (dPitch, dYaw, dRoll) as our local angular velocity
// representation, we can't actually solve this problem
/*
void CBaseEntity::SetAbsAngularVelocity( const QAngle &vecAbsAngVelocity )
{
	// The abs velocity won't be dirty since we're setting it here
	// All children are invalid, but we are not
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSANGVELOCITY );
	RemoveEFlags( EFL_DIRTY_ABSANGVELOCITY );

	m_vecAbsAngVelocity = vecAbsAngVelocity;

	CBaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		m_vecAngVelocity = vecAbsAngVelocity;
		return;
	}

	// NOTE: We *can't* subtract out parent ang velocity, it's nonsensical
	matrix3x4_t entityToWorld;
	AngleMatrix( vecAbsAngVelocity, entityToWorld );

	// Moveparent case: transform the abs relative angular vel into local space
	matrix3x4_t worldToParent, localMatrix;
	MatrixInvert( pMoveParent->EntityToWorldTransform(), worldToParent );
	ConcatTransforms( worldToParent, entityToWorld, localMatrix );
	MatrixAngles( localMatrix, m_vecAngVelocity );
}
*/


void CBaseEntity::ApplyLocalVelocityImpulse( const Vector &vecImpulse )
{
	// NOTE: Don't have to use GetVelocity here because local values
	// are always guaranteed to be correct, unlike abs values which may 
	// require recomputation
	if (vecImpulse != vec3_origin )
	{
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSVELOCITY );
		m_vecVelocity += vecImpulse;
		NetworkStateChanged();
	}
}

void CBaseEntity::ApplyAbsVelocityImpulse( const Vector &vecImpulse )
{
	// NOTE: Have to use GetAbsVelocity here to ensure it's the correct value
	if (vecImpulse != vec3_origin )
	{
		Vector vecResult;
		VectorAdd( GetAbsVelocity(), vecImpulse, vecResult );
		SetAbsVelocity( vecResult );
	}
}


//-----------------------------------------------------------------------------
// Methods that modify local physics state, and let us know to compute abs state later
//-----------------------------------------------------------------------------
void CBaseEntity::SetLocalOrigin( const Vector& origin )
{
	if (m_vecOrigin != origin)
	{
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM );
		m_vecOrigin = origin;
		SetSimulationTime( gpGlobals->curtime );
		NetworkStateChanged();
	}
}

void CBaseEntity::SetLocalAngles( const QAngle& angles )
{
	if (m_angRotation != angles)
	{
		// This will cause the velocities of all children to need recomputation
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM, EFL_DIRTY_ABSVELOCITY | EFL_DIRTY_ABSANGVELOCITY );
		m_angRotation = angles;
		SetSimulationTime( gpGlobals->curtime );
		NetworkStateChanged();
	}
}

void CBaseEntity::SetLocalVelocity( const Vector &vecVelocity )
{
	if (m_vecVelocity != vecVelocity)
	{
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSVELOCITY );
		m_vecVelocity = vecVelocity;
		NetworkStateChanged();
	}
}

void CBaseEntity::SetLocalAngularVelocity( const QAngle &vecAngVelocity )
{
	if (m_vecAngVelocity != vecAngVelocity)
	{
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSANGVELOCITY );
		m_vecAngVelocity = vecAngVelocity;
		NetworkStateChanged();
	}
}


//-----------------------------------------------------------------------------
// Physics state accessor methods
//-----------------------------------------------------------------------------
const Vector& CBaseEntity::GetLocalOrigin( void ) const
{
	return m_vecOrigin.Get();
}

const QAngle& CBaseEntity::GetLocalAngles( void ) const
{
	return m_angRotation.Get();
}

const Vector& CBaseEntity::GetAbsOrigin( void ) const
{
	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		const_cast<CBaseEntity*>(this)->CalcAbsolutePosition();
	}
	return m_vecAbsOrigin;
}

const QAngle& CBaseEntity::GetAbsAngles( void ) const
{
	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		const_cast<CBaseEntity*>(this)->CalcAbsolutePosition();
	}
	return m_angAbsRotation;
}


//-----------------------------------------------------------------------------
// Sets the local position from a transform
//-----------------------------------------------------------------------------
void CBaseEntity::SetLocalTransform( const matrix3x4_t &localTransform )
{
	// FIXME: Should angles go away? Should we just use transforms?
	Vector vecLocalOrigin;
	QAngle vecLocalAngles;
	MatrixGetColumn( localTransform, 3, vecLocalOrigin );
	MatrixAngles( localTransform, vecLocalAngles );
	SetLocalOrigin( vecLocalOrigin );
	SetLocalAngles( vecLocalAngles );
}

void CBaseEntity::Relink()
{
	if ( pev )
	{
		engine->RelinkEntity( pev );
	}
	else
	{
		CalcAbsolutePosition();
	}
}

edict_t *CBaseEntity::GetEdict() const
{
	return (edict_t *)edict();
}

//-----------------------------------------------------------------------------
// Purpose: Created predictable and sets up Id.  Note that persist is ignored on the server.
// Input  : *classname - 
//			*module - 
//			line - 
//			persist - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CBaseEntity::CreatePredictedEntityByName( const char *classname, const char *module, int line, bool persist /* = false */ )
{
	CBasePlayer *player = CBaseEntity::GetPredictionPlayer();
	Assert( player );

	CBaseEntity *ent = NULL;

	int command_number = player->CurrentCommandNumber();
	int player_index = player->entindex() - 1;

	CPredictableId testId;
	testId.Init( player_index, command_number, classname, module, line );

	ent = CreateEntityByName( classname );
	// No factory???
	if ( !ent )
		return NULL;

	ent->SetPredictionEligible( true );

	// Set up "shared" id number
	ent->m_PredictableID.GetForModify().SetRaw( testId.GetRaw() );

	return ent;
}

void CBaseEntity::SetPredictionEligible( bool canpredict )
{
// Nothing in game code	m_bPredictionEligible = canpredict;
}

static CUtlVector< EHANDLE >	s_UpdateClientData;

void CBaseEntity::AddToUpdateClientDataList( CBaseEntity *entity )
{
	EHANDLE h;
	
	h = entity;
	
	if ( s_UpdateClientData.Find( h ) != s_UpdateClientData.InvalidIndex() )
		return;

	s_UpdateClientData.AddToTail( h );
}

void CBaseEntity::RemoveFromUpdateClientDataList( CBaseEntity *entity )
{
	EHANDLE h;
	
	h = entity;
	s_UpdateClientData.FindAndRemove( h );
}

void CBaseEntity::PerformUpdateClientData( CBasePlayer *player )
{
	int c = s_UpdateClientData.Count();
	int i;

	for ( i = c - 1; i >= 0 ; i-- )
	{
		EHANDLE h = s_UpdateClientData[ i ];

		CBaseEntity *ent = h.Get();
		if ( !ent )
		{
			s_UpdateClientData.Remove( i );
		}
		else
		{
			ent->OnUpdateClientData( player );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tell clients to remove all decals from this entity
//-----------------------------------------------------------------------------
void CBaseEntity::RemoveAllDecals( void )
{
	CRecipientFilter everyone;
	everyone.AddAllPlayers();
	UserMessageBegin( everyone, "ClearDecals" );
		WRITE_BYTE( entindex() );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
//-----------------------------------------------------------------------------
void CBaseEntity::ModifyOrAppendCriteria( AI_CriteriaSet& set )
{
	// TODO
	// Append chapter/day?

	// Append map name
	set.AppendCriteria( "map", gpGlobals->mapname.ToCStr() );
	// Append our classname and game name
	set.AppendCriteria( "classname", GetClassname() );
	set.AppendCriteria( "name", GetEntityName().ToCStr() );

	// Append our health
	set.AppendCriteria( "health", UTIL_VarArgs( "%i", GetHealth() ) );

	float healthfrac = 0.0f;
	if ( GetMaxHealth() > 0 )
	{
		healthfrac = (float)GetHealth() / (float)GetMaxHealth();
	}

	set.AppendCriteria( "healthfrac", UTIL_VarArgs( "%.3f", healthfrac ) );

	// Append anything from I/O or keyvalues pairs
	AppendContextToCriteria( set );

	// Append anything from world I/O/keyvalues with "world" as prefix
	CWorld *world = dynamic_cast< CWorld * >( CBaseEntity::Instance( engine->PEntityOfEntIndex( 0 ) ) );
	if ( world )
	{
		world->AppendContextToCriteria( set, "world" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
//			"" - 
//-----------------------------------------------------------------------------
void CBaseEntity::AppendContextToCriteria( AI_CriteriaSet& set, const char *prefix /*= ""*/ )
{
	int c = GetContextCount();
	int i;

	char sz[ 128 ];
	for ( i = 0; i < c; i++ )
	{
		const char *name = GetContextName( i );
		const char *value = GetContextValue( i );

		Q_snprintf( sz, sizeof( sz ), "%s%s", prefix, name );

		set.AppendCriteria( sz, value );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get current context count
// Output : int
//-----------------------------------------------------------------------------
int CBaseEntity::GetContextCount() const
{
	return m_ResponseContexts.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CBaseEntity::GetContextName( int index ) const
{
	if ( index < 0 || index >= m_ResponseContexts.Count() )
	{
		Assert( 0 );
		return "";
	}

	return  m_ResponseContexts[ index ].m_iszName.ToCStr();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CBaseEntity::GetContextValue( int index ) const
{
	if ( index < 0 || index >= m_ResponseContexts.Count() )
	{
		Assert( 0 );
		return "";
	}

	return  m_ResponseContexts[ index ].m_iszValue.ToCStr();
}


//-----------------------------------------------------------------------------
// Purpose: Search for index of named context string
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CBaseEntity::FindContextByName( const char *name ) const
{
	int c = m_ResponseContexts.Count();
	for ( int i = 0; i < c; i++ )
	{
		if ( FStrEq( name, GetContextName( i ) ) )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *raw - 
//			*key - 
//			keylen - 
//			*value - 
//			valuelen - 
// Output : static bool
//-----------------------------------------------------------------------------
static const char *SplitContext( const char *raw, char *key, int keylen, char *value, int valuelen )
{
	char *colon = Q_strstr( raw, ":" );
	if ( !colon )
	{
		DevMsg( "SplitContext:  warning, ignoring context '%s', missing colon separator!\n", raw );
		return NULL;
	}

	int len = colon - raw;
	Q_strncpy( key, raw, min( len + 1, keylen ) );
	key[ min( len, keylen ) ] = 0;

	bool last = false;
	char *end = Q_strstr( colon + 1, "," );
	if ( !end )
	{
		int remaining = Q_strlen( colon + 1 );
		end = colon + 1 + remaining;
		last = true;
	}
	len = end - ( colon + 1 );

	Q_strncpy( value, colon + 1, len + 1 );
	value[ len ] = 0;

	return last ? NULL : end + 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CBaseEntity::InputAddContext( inputdata_t& inputdata )
{
	const char *contextName = inputdata.value.String();
	AddContext( contextName );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *contextName - 
//-----------------------------------------------------------------------------
void CBaseEntity::AddContext( const char *contextName )
{
	char key[ 128 ];
	char value[ 128 ];

	const char *p = contextName;
	while ( p )
	{
		p = SplitContext( p, key, sizeof( key ), value, sizeof( value ) );

		if ( FindContextByName( key ) != -1 )
			continue;

		ResponseContext_t newContext;
		newContext.m_iszName = AllocPooledString( key );
		newContext.m_iszValue = AllocPooledString( value );

		m_ResponseContexts.AddToTail( newContext );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CBaseEntity::InputRemoveContext( inputdata_t& inputdata )
{
	const char *contextName = inputdata.value.String();
	int idx = FindContextByName( contextName );
	if ( idx == -1 )
		return;

	m_ResponseContexts.Remove( idx );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CBaseEntity::InputClearContext( inputdata_t& inputdata )
{
	m_ResponseContexts.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : IResponseSystem
//-----------------------------------------------------------------------------
IResponseSystem *CBaseEntity::GetResponseSystem()
{
	return m_pInstancedResponseSystem;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scriptfile - 
//-----------------------------------------------------------------------------
void CBaseEntity::InstallCustomResponseSystem( const char *scriptfile )
{
	m_pInstancedResponseSystem = InstancedResponseSystemCreate( scriptfile );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CBaseEntity::InputDispatchResponse( inputdata_t& inputdata )
{
	DispatchResponse( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *conceptName - 
//-----------------------------------------------------------------------------
void CBaseEntity::DispatchResponse( const char *conceptName )
{
	IResponseSystem *rs = GetResponseSystem();
	if ( !rs )
		return;

	AI_CriteriaSet set;
	// Always include the concept name
	set.AppendCriteria( "concept", conceptName, CONCEPT_WEIGHT );
	// Let NPC fill in most match criteria
	ModifyOrAppendCriteria( set );
	// Append local player criteria to set,too
	CBasePlayer::ModifyOrAppendPlayerCriteria( set );

	// Now that we have a criteria set, ask for a suitable response
	AI_Response result;
	bool found = rs->FindBestResponse( set, result );
	if ( !found )
	{
		return;
	}

	// Handle the response here...
	const char *response = result.GetResponse();
	switch ( result.GetType() )
	{
	case AI_Response::RESPONSE_SPEAK:
		{
			EmitSound( response );
		}
		break;
	case AI_Response::RESPONSE_WAV:
		{
			CPASAttenuationFilter filter( this );
			CBaseEntity::EmitSound( filter, entindex(), CHAN_VOICE, response, 1, result.GetSoundLevel(), 0, PITCH_NORM );
		}
		break;
	case AI_Response::RESPONSE_SENTENCE:
		{
			int sentenceIndex = SENTENCEG_Lookup( response );
			if( sentenceIndex == -1 )
			{
				// sentence not found
				break;
			}

			// FIXME:  Get pitch from npc?
			CPASAttenuationFilter filter( this );
			enginesound->EmitSentenceByIndex( filter, entindex(), CHAN_VOICE, sentenceIndex, 1, result.GetSoundLevel(), 0, PITCH_NORM );
		}
		break;
	default:
		// Don't know how to handle .vcds!!!
		break;
	}
}


int	CBaseEntity::GetTextureFrameIndex( void )
{
	return m_iTextureFrameIndex;
}

void CBaseEntity::SetTextureFrameIndex( int iIndex )
{
	m_iTextureFrameIndex = iIndex;
}

