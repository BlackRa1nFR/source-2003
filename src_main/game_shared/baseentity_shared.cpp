//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "decals.h"
#include "effect_dispatch_data.h"
#include "model_types.h"

#ifdef CLIENT_DLL
#include "c_te_effect_dispatch.h"
#else
#include "te_effect_dispatch.h"
#endif

// The player drives simulation of this entity
void CBaseEntity::SetPlayerSimulated( CBasePlayer *pOwner )
{
	m_bIsPlayerSimulated = true;
	pOwner->AddToPlayerSimulationList( this );
	m_hPlayerSimulationOwner = pOwner;
}

void CBaseEntity::UnsetPlayerSimulated( void )
{
	if ( m_hPlayerSimulationOwner != NULL )
	{
		m_hPlayerSimulationOwner->RemoveFromPlayerSimulationList( this );
	}
	m_hPlayerSimulationOwner = NULL;
	m_bIsPlayerSimulated = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::IsPlayerSimulated( void ) const
{
	return m_bIsPlayerSimulated;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBasePlayer
//-----------------------------------------------------------------------------
CBasePlayer *CBaseEntity::GetSimulatingPlayer( void )
{
	return m_hPlayerSimulationOwner;
}

// position of eyes
Vector CBaseEntity::EyePosition( ) 
{ 
	return GetAbsOrigin() + m_vecViewOffset; 
}

const QAngle &CBaseEntity::EyeAngles()
{
	return GetAbsAngles();
}

const QAngle &CBaseEntity::LocalEyeAngles()
{
	return GetLocalAngles();
}

// position of ears
Vector CBaseEntity::EarPosition( ) 
{ 
	return EyePosition( ); 
}


//-----------------------------------------------------------------------------
// center point of entity
//-----------------------------------------------------------------------------
const Vector &CBaseEntity::WorldSpaceCenter( ) const 
{
	// Makes sure we don't re-use the same temp twice
	Vector &vecResult = AllocTempVector();

	if ( !IsBoundsDefinedInEntitySpace() )
	{
		VectorAdd( WorldAlignMins(), WorldAlignMaxs(), vecResult );
		vecResult *= 0.5f;
		vecResult += GetAbsOrigin();
	}
	else
	{
		// FIXME: Should we cache this off?
		if ( GetAbsAngles() == vec3_angle )
		{
			VectorAdd( EntitySpaceMins(), EntitySpaceMaxs(), vecResult );
			vecResult *= 0.5f;
			vecResult += GetAbsOrigin();
		}
		else
		{
			VectorTransform( EntitySpaceCenter(), EntityToWorldTransform(), vecResult );
		}
	}
	return vecResult;
}


//-----------------------------------------------------------------------------
// Selects a random point in the bounds given the normalized 0-1 bounds 
//-----------------------------------------------------------------------------
void CBaseEntity::RandomPointInBounds( const Vector &vecNormalizedMins, const Vector &vecNormalizedMaxs, Vector *pPoint) const
{
	float x = random->RandomFloat( vecNormalizedMins.x, vecNormalizedMaxs.x );
	float y = random->RandomFloat( vecNormalizedMins.y, vecNormalizedMaxs.y );
	float z = random->RandomFloat( vecNormalizedMins.z, vecNormalizedMaxs.z );

	if ( !IsBoundsDefinedInEntitySpace() )
	{
		pPoint->x = WorldAlignSize().x * x + WorldAlignMins().x;
		pPoint->y = WorldAlignSize().y * y + WorldAlignMins().y;
		pPoint->z = WorldAlignSize().z * z + WorldAlignMins().z;
		*pPoint += GetAbsOrigin();
	}
	else
	{
		Vector vecEntitySpace;
		vecEntitySpace.x = EntitySpaceSize().x * x + EntitySpaceMins().x;
		vecEntitySpace.y = EntitySpaceSize().y * y + EntitySpaceMins().y;
		vecEntitySpace.z = EntitySpaceSize().z * z + EntitySpaceMins().z;

		// FIXME: Should we cache this off?
		if ( GetAbsAngles() == vec3_angle )
		{
			VectorAdd( vecEntitySpace, GetAbsOrigin(), *pPoint );
		}
		else
		{
			VectorTransform( vecEntitySpace, EntityToWorldTransform(), *pPoint );
		}
	}
}


//-----------------------------------------------------------------------------
// Transforms an AABB measured in entity space to a box that surrounds it in world space
//-----------------------------------------------------------------------------
void CBaseEntity::EntityAABBToWorldAABB( const Vector &entityMins, const Vector &entityMaxs, Vector *pWorldMins, Vector *pWorldMaxs ) const
{
	if ( GetAbsAngles() == vec3_angle )
	{
		VectorAdd( entityMins, GetAbsOrigin(), *pWorldMins );
		VectorAdd( entityMaxs, GetAbsOrigin(), *pWorldMaxs );
	}
	else
	{
		TransformAABB( EntityToWorldTransform(), entityMins, entityMaxs, *pWorldMins, *pWorldMaxs );
	}
}

void CBaseEntity::WorldAABBToEntityAABB( const Vector &worldMins, const Vector &worldMaxs, Vector *pEntityMins, Vector *pEntityMaxs ) const
{
	if ( GetAbsAngles() == vec3_angle )
	{
		VectorSubtract( worldMins, GetAbsOrigin(), *pEntityMins );
		VectorSubtract( worldMaxs, GetAbsOrigin(), *pEntityMaxs );
	}
	else
	{
		ITransformAABB( EntityToWorldTransform(), worldMins, worldMaxs, *pEntityMins, *pEntityMaxs );
	}
}

void CBaseEntity::WorldSpaceAABB( Vector *pWorldMins, Vector *pWorldMaxs ) const
{
	if ( !IsBoundsDefinedInEntitySpace() )
	{
		VectorAdd( WorldAlignMins(), GetAbsOrigin(), *pWorldMins );
		VectorAdd( WorldAlignMaxs(), GetAbsOrigin(), *pWorldMaxs );
	}
	else
	{
		EntityAABBToWorldAABB( EntitySpaceMins(), EntitySpaceMaxs(), pWorldMins, pWorldMaxs ); 
	}
}

	
MoveType_t CBaseEntity::GetMoveType() const
{
	return m_MoveType;
}

MoveCollide_t CBaseEntity::GetMoveCollide() const
{
	return m_MoveCollide;
}

#if !defined( CLIENT_DLL )
#define CHANGE_FLAGS(flags,newFlags) { unsigned int old = flags; flags = (newFlags); gEntList.ReportEntityFlagsChanged( this, old, flags ); }
#else
#define CHANGE_FLAGS(flags,newFlags) (flags = (newFlags))
#endif

void CBaseEntity::AddFlag( int flags )
{
	CHANGE_FLAGS( m_fFlags, m_fFlags | flags );
}

void CBaseEntity::RemoveFlag( int flagsToRemove )
{
	CHANGE_FLAGS( m_fFlags, m_fFlags & ~flagsToRemove );
}

int	CBaseEntity::GetFlags( void ) const
{
	return m_fFlags;
}

void CBaseEntity::ClearFlags( void )
{
	CHANGE_FLAGS( m_fFlags, 0 );
}

void CBaseEntity::ToggleFlag( int flagToToggle )
{
	CHANGE_FLAGS( m_fFlags, m_fFlags ^ flagToToggle );
}


bool CBaseEntity::IsAlive( void )
{
	return m_lifeState == LIFE_ALIVE; 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( m_CollisionGroup == COLLISION_GROUP_DEBRIS )
	{
		if ( ! (contentsMask & CONTENTS_DEBRIS) )
			return false;
	}
	return true;
}

CBaseEntity	*CBaseEntity::GetOwnerEntity() const
{
	return m_hOwnerEntity.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBaseEntity::GetPredictionRandomSeed( void )
{
	return m_nPredictionRandomSeed;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : seed - 
//-----------------------------------------------------------------------------
void CBaseEntity::SetPredictionRandomSeed( const CUserCmd *cmd )
{
	if ( !cmd )
	{
		m_nPredictionRandomSeed = -1;
		return;
	}

	m_nPredictionRandomSeed = ( cmd->random_seed );
}




CBasePlayer *CBaseEntity::GetPredictionPlayer( void )
{
	return m_pPredictionPlayer;
}

void CBaseEntity::SetPredictionPlayer( CBasePlayer *player )
{
	m_pPredictionPlayer = player;
}


//------------------------------------------------------------------------------
// Purpose : Base implimentation for entity handling decals
//------------------------------------------------------------------------------
void CBaseEntity::DecalTrace( trace_t *pTrace, char const *decalName )
{
	int index = decalsystem->GetDecalIndexForName( decalName );
	if ( index < 0 )
		return;

	Assert( pTrace->m_pEnt );

	CBroadcastRecipientFilter filter;
	te->Decal( filter, 0.0, &pTrace->endpos, &pTrace->startpos,
		pTrace->GetEntityIndex(), pTrace->hitbox, index );
}

//-----------------------------------------------------------------------------
// Purpose: Base handling for impacts against entities
//-----------------------------------------------------------------------------
void CBaseEntity::ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
{
	Assert( pTrace->m_pEnt );

	CBaseEntity *pEntity = pTrace->m_pEnt;
	surfacedata_t *pSurfaceData = physprops->GetSurfaceData( pTrace->surface.surfaceProps );
	int iMaterial = pSurfaceData->gameMaterial;
 
	// Build the impact data
	CEffectData data;
	data.m_vOrigin = pTrace->endpos;
	data.m_vStart = pTrace->startpos;
	data.m_nMaterial = iMaterial;
	data.m_nDamageType = iDamageType;
	data.m_nHitBox = pTrace->hitbox;
	data.m_nEntIndex = pEntity->entindex();

	// Send it on its way
	if ( !pCustomImpactName )
	{
		DispatchEffect( "Impact", data );
	}
	else
	{
		DispatchEffect( pCustomImpactName, data );
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the damage decal to use, given a damage type
// Input  : bitsDamageType - the damage type
// Output : the index of the damage decal to use
//-----------------------------------------------------------------------------
char const *CBaseEntity::DamageDecal( int bitsDamageType, int gameMaterial )
{
	if ( m_nRenderMode == kRenderTransAlpha )
		return "";

	if ( m_nRenderMode != kRenderNormal && gameMaterial == 'G' )
		return "BulletProof";

	// This will get translated at a lower layer based on game material
	return "Impact.Concrete";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CBaseEntity::GetIndexForContext( const char *pszContext )
{
	for ( int i = 0; i < m_aThinkFunctions.Size(); i++ )
	{
		if ( !strncmp( m_aThinkFunctions[i].m_pszContext, pszContext, MAX_CONTEXT_LENGTH ) )
			return i;
	}

	return NO_THINK_CONTEXT;
}

//-----------------------------------------------------------------------------
// Purpose: Get a fresh think context for this entity
//-----------------------------------------------------------------------------
int CBaseEntity::RegisterThinkContext( const char *szContext )
{
	int iIndex = GetIndexForContext( szContext );
	if ( iIndex != NO_THINK_CONTEXT )
		return iIndex;

	// Make a new think func
	thinkfunc_t sNewFunc;
	Q_memset( &sNewFunc, 0, sizeof( sNewFunc ) );
	sNewFunc.m_pfnThink = NULL;
	sNewFunc.m_nNextThinkTick = 0;
	sNewFunc.m_pszContext = szContext;

	// Insert it into our list
	return m_aThinkFunctions.AddToTail( sNewFunc );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
BASEPTR	CBaseEntity::ThinkSet( BASEPTR func, float thinkTime, const char *szContext )
{
#if !defined( CLIENT_DLL )
#ifdef _DEBUG
	COMPILE_TIME_ASSERT( sizeof(func) == 4 );
#endif
#endif

	// Old system?
	if ( !szContext )
	{
		m_pfnThink = func;
#if !defined( CLIENT_DLL )
#ifdef _DEBUG
		FunctionCheck( (void *)*((int *)((char *)this + ( offsetof(CBaseEntity,m_pfnThink)))), "BaseThinkFunc" ); 
#endif
#endif
		return m_pfnThink;
	}

	// Find the think function in our list, and if we couldn't find it, register it
	int iIndex = GetIndexForContext( szContext );
	if ( iIndex == NO_THINK_CONTEXT )
	{
		iIndex = RegisterThinkContext( szContext );
	}

	m_aThinkFunctions[ iIndex ].m_pfnThink = func;
#if !defined( CLIENT_DLL )
#ifdef _DEBUG
	//FunctionCheck( (void *)*((int *)((char *)this + ( offsetof(CBaseEntity,m_aThinkFunctions[ iIndex ].m_pfnThink)))), szContext ); 
#endif
#endif

	if ( thinkTime != 0 )
	{
		int thinkTick = ( thinkTime == TICK_NEVER_THINK ) ? TICK_NEVER_THINK : TIME_TO_TICKS( thinkTime );
		m_aThinkFunctions[ iIndex ].m_nNextThinkTick = thinkTick;
	}
	return func;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseEntity::SetNextThink( float thinkTime, const char *szContext )
{
	int thinkTick = ( thinkTime == TICK_NEVER_THINK ) ? TICK_NEVER_THINK : TIME_TO_TICKS( thinkTime );

	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Setting base think function within think context %s\n", m_aThinkFunctions[m_iCurrentThinkContext].m_pszContext );
		}
#endif

		// Old system
		m_nNextThinkTick = thinkTick;
		return;
	}
	else
	{
		// Find the think function in our list, and if we couldn't find it, register it
		iIndex = GetIndexForContext( szContext );
		if ( iIndex == NO_THINK_CONTEXT )
		{
			iIndex = RegisterThinkContext( szContext );
		}
	}

	// Old system
	m_aThinkFunctions[ iIndex ].m_nNextThinkTick = thinkTick;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseEntity::GetNextThink( char *szContext )
{
	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Getting base nextthink time within think context %s\n", m_aThinkFunctions[m_iCurrentThinkContext].m_pszContext );
		}
#endif

		if ( m_nNextThinkTick == TICK_NEVER_THINK )
			return TICK_NEVER_THINK;

		// Old system
		return TICK_RATE * (m_nNextThinkTick );
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForContext( szContext );
	}

	if ( m_aThinkFunctions[ iIndex ].m_nNextThinkTick == TICK_NEVER_THINK )
	{
		return TICK_NEVER_THINK;
	}
	return TICK_RATE * (m_aThinkFunctions[ iIndex ].m_nNextThinkTick );
}

int	CBaseEntity::GetNextThinkTick( char *szContext /*= NULL*/ )
{
	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Getting base nextthink time within think context %s\n", m_aThinkFunctions[m_iCurrentThinkContext].m_pszContext );
		}
#endif

		if ( m_nNextThinkTick == TICK_NEVER_THINK )
			return TICK_NEVER_THINK;

		// Old system
		return m_nNextThinkTick;
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForContext( szContext );
	}

	if ( m_aThinkFunctions[ iIndex ].m_nNextThinkTick == TICK_NEVER_THINK )
	{
		return TICK_NEVER_THINK;
	}
	return m_aThinkFunctions[ iIndex ].m_nNextThinkTick;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseEntity::GetLastThink( char *szContext )
{
	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Getting base lastthink time within think context %s\n", m_aThinkFunctions[m_iCurrentThinkContext].m_pszContext );
		}
#endif
		// Old system
		return m_nLastThinkTick * TICK_RATE;
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForContext( szContext );
	}

	return m_aThinkFunctions[ iIndex ].m_nLastThinkTick * TICK_RATE;
}
	
int CBaseEntity::GetLastThinkTick( char *szContext /*= NULL*/ )
{
	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Getting base lastthink time within think context %s\n", m_aThinkFunctions[m_iCurrentThinkContext].m_pszContext );
		}
#endif
		// Old system
		return m_nLastThinkTick;
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForContext( szContext );
	}

	return m_aThinkFunctions[ iIndex ].m_nLastThinkTick;
}

//-----------------------------------------------------------------------------
// Sets/Gets the next think based on context index
//-----------------------------------------------------------------------------
void CBaseEntity::SetNextThink( int nContextIndex, float thinkTime )
{
	int thinkTick = ( thinkTime == TICK_NEVER_THINK ) ? TICK_NEVER_THINK : TIME_TO_TICKS( thinkTime );

	if (nContextIndex < 0)
	{
		SetNextThink( thinkTime );
	}
	else
	{
		m_aThinkFunctions[nContextIndex].m_nNextThinkTick = thinkTick;
	}
}

void CBaseEntity::SetLastThink( int nContextIndex, float thinkTime )
{
	int thinkTick = ( thinkTime == TICK_NEVER_THINK ) ? TICK_NEVER_THINK : TIME_TO_TICKS( thinkTime );

	if (nContextIndex < 0)
	{
		m_nLastThinkTick = thinkTick;
	}
	else
	{
		m_aThinkFunctions[nContextIndex].m_nLastThinkTick = thinkTick;
	}
}

float CBaseEntity::GetNextThink( int nContextIndex ) const
{
	if (nContextIndex < 0)
		return m_nNextThinkTick * TICK_RATE;

	return m_aThinkFunctions[nContextIndex].m_nNextThinkTick * TICK_RATE; 
}

int	CBaseEntity::GetNextThinkTick( int nContextIndex ) const
{
	if (nContextIndex < 0)
		return m_nNextThinkTick;

	return m_aThinkFunctions[nContextIndex].m_nNextThinkTick; 
}

int CBaseEntity::GetEFlags() const
{
	return m_iEFlags;
}

void CBaseEntity::SetEFlags( int iEFlags )
{
	m_iEFlags = iEFlags;
}

//-----------------------------------------------------------------------------
// Purpose: My physics object has been updated, react or extract data
//-----------------------------------------------------------------------------
void CBaseEntity::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	switch( GetMoveType() )
	{
	case MOVETYPE_VPHYSICS:
		{
			Vector origin;
			QAngle angles;

			pPhysics->GetPosition( &origin, &angles );

			if ( !IsFinite( angles.x ) || !IsFinite( angles.y ) || !IsFinite( angles.x ) )
			{
				Msg( "Infinite values from vphysics!\n" );
			}

			SetAbsOrigin( origin );
			SetAbsAngles( angles );

#ifndef CLIENT_DLL 
			engine->RelinkEntity( pev, true );
			PhysicsRelinkChildren();
#else
			Relink();
#endif
		}
	break;

	case MOVETYPE_STEP:
		break;

	case MOVETYPE_PUSH:
#ifndef CLIENT_DLL
		VPhysicsUpdatePusher( pPhysics );
#endif
	break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Init this object's physics as a static
//-----------------------------------------------------------------------------
IPhysicsObject *CBaseEntity::VPhysicsInitStatic( void )
{
	if ( !VPhysicsInitSetup() )
		return NULL;

#ifndef CLIENT_DLL
	// If this entity has a move parent, it needs to be shadow, not static
	if ( GetMoveParent() )
	{
		// must be SOLID_VPHYSICS if in hierarchy to solve collisions correctly
		if ( GetSolid() == SOLID_BSP )
		{
			SetSolid( SOLID_VPHYSICS );
		}

		return VPhysicsInitShadow( false, false );
	}
#endif

	// No physics
	if ( GetSolid() == SOLID_NONE )
		return NULL;

	// create a static physics objct
	IPhysicsObject *pPhysicsObject = NULL;
	if ( GetSolid() == SOLID_BBOX )
	{
		pPhysicsObject = PhysModelCreateBox( this, WorldAlignMins(), WorldAlignMaxs(), GetAbsOrigin(), true );
	}
	else
	{
		pPhysicsObject = PhysModelCreateUnmoveable( this, GetModelIndex(), GetAbsOrigin(), GetAbsAngles() );
	}
	VPhysicsSetObject( pPhysicsObject );
	return pPhysicsObject;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysics - 
//-----------------------------------------------------------------------------
void CBaseEntity::VPhysicsSetObject( IPhysicsObject *pPhysics )
{
	if ( m_pPhysicsObject && pPhysics )
	{
		// ARRGH!
#ifdef CLIENT_DLL
		Warning( "Overwriting physics object for %s\n", GetClassName() );
#else
		Warning( "Overwriting physics object for %s\n", GetClassname() );
#endif
	}
	m_pPhysicsObject = pPhysics;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseEntity::VPhysicsDestroyObject( void )
{
	if ( m_pPhysicsObject )
	{
#ifndef CLIENT_DLL
		PhysRemoveShadow( this );
#endif
		PhysDestroyObject( m_pPhysicsObject );
		m_pPhysicsObject = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseEntity::VPhysicsInitSetup()
{
#ifndef CLIENT_DLL
	// don't support logical ents
	if ( !pev || IsMarkedForDeletion() )
		return false;
#endif

	// If this entity already has a physics object, then it should have been deleted prior to making this call.
	Assert(!m_pPhysicsObject);
	VPhysicsDestroyObject();

	// make sure absorigin / absangles are correct
	Relink();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: This creates a normal vphysics simulated object
//			physics alone determines where it goes (gravity, friction, etc)
//			and the entity receives updates from vphysics.  SetAbsOrigin(), etc do not affect the object!
//-----------------------------------------------------------------------------
IPhysicsObject *CBaseEntity::VPhysicsInitNormal( SolidType_t solidType, int nSolidFlags, bool createAsleep, solid_t *pSolid )
{
	if ( !VPhysicsInitSetup() )
		return NULL;

	// NOTE: This has to occur before PhysModelCreate because that call will
	// call back into VPhysicsCollisionFilter, which uses solidtype.
	SetSolid( solidType );
	SetSolidFlags( nSolidFlags );

	// No physics
	if ( solidType == SOLID_NONE )
	{
		Relink();
		return NULL;
	}

	// create a normal physics object
	IPhysicsObject *pPhysicsObject = PhysModelCreate( this, GetModelIndex(), GetAbsOrigin(), GetAbsAngles(), pSolid );
	if ( pPhysicsObject )
	{
		VPhysicsSetObject( pPhysicsObject );
		SetMoveType( MOVETYPE_VPHYSICS );

		if ( !createAsleep )
		{
			pPhysicsObject->Wake();
		}
		Relink();
	}

	return pPhysicsObject;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseEntity::CreateVPhysics()
{
	return false;
}

bool CBaseEntity::IsStandable() const
{
	if (GetSolidFlags() & FSOLID_NOT_STANDABLE) 
		return false;

	if ( GetSolid() == SOLID_BSP || GetSolid() == SOLID_VPHYSICS || GetSolid() == SOLID_BBOX )
		return true;

	return IsBSPModel( ); 
}

bool CBaseEntity::IsBSPModel() const
{
	if ( GetSolid() == SOLID_BSP )
		return true;
	
	const model_t *model = modelinfo->GetModel( GetModelIndex() );

	if ( GetSolid() == SOLID_VPHYSICS && modelinfo->GetModelType( model ) == mod_brush )
		return true;

	return false;
}

bool CBaseEntity::IsSimulatedEveryTick() const
{
	return m_bSimulatedEveryTick;
}

bool CBaseEntity::IsAnimatedEveryTick() const
{
	return m_bAnimatedEveryTick;
}

void CBaseEntity::SetSimulatedEveryTick( bool sim )
{
	m_bSimulatedEveryTick = sim;
}

void CBaseEntity::SetAnimatedEveryTick( bool anim )
{
	m_bAnimatedEveryTick = anim;
}

float CBaseEntity::GetAnimTime() const
{
	return m_flAnimTime;
}

float CBaseEntity::GetSimulationTime() const
{
	return m_flSimulationTime;
}

void CBaseEntity::SetAnimTime( float at )
{
	m_flAnimTime = at;
}

void CBaseEntity::SetSimulationTime( float st )
{
	m_flSimulationTime = st;
}
