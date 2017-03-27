//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_baseentity.h"
#include "prediction.h"
#include "model_types.h"
#include "iviewrender_beams.h"
#include "dlight.h"
#include "iviewrender.h"
#include "view.h"
#include "iefx.h"
#include "c_team.h"
#include "clientmode.h"
#include "c_clientstats.h"
#include "usercmd.h"
#include "engine/IEngineSound.h"
#include "crtdbg.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "tier0/vprof.h"
#include "fx_line.h"
#include "interface.h"
#include "materialsystem/IMaterialSystem.h"
#include "parsemsg.h"
#include "soundinfo.h"
#include "vmatrix.h"
#include "isaverestore.h"
#include "interval.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

template<class Type>
CUtlFixedLinkedList< CInterpolatedVar<Type>::CInterpolatedVarEntry >	CInterpolatedVar<Type>::m_VarHistory;
template<class Type, int COUNT>
CUtlFixedLinkedList< CInterpolatedVarArray<Type,COUNT>::CInterpolatedVarEntry >	CInterpolatedVarArray<Type,COUNT>::m_VarHistory;

static ConVar  cl_interpolate( "cl_interpolate", "1.0", 0, "Interpolate entities on the client." );

extern ConVar	cl_showerror;

int C_BaseEntity::m_nPredictionRandomSeed = -1;
C_BasePlayer *C_BaseEntity::m_pPredictionPlayer = NULL;
bool C_BaseEntity::s_bAbsQueriesValid = true;
bool C_BaseEntity::s_bAbsRecomputationEnabled = true;


//-----------------------------------------------------------------------------
// Purpose: Maintains a list of predicted or client created entities
//-----------------------------------------------------------------------------
class CPredictableList : public IPredictableList
{
public:
	virtual C_BaseEntity *GetPredictable( int slot );
	virtual int GetPredictableCount( void );

protected:
	void	AddToPredictableList( ClientEntityHandle_t add );
	void	RemoveFromPredictablesList( ClientEntityHandle_t remove );

private:
	CUtlVector< ClientEntityHandle_t >	m_Predictables;

	friend class C_BaseEntity;
};

// Create singleton
static CPredictableList g_Predictables;
IPredictableList *predictables = &g_Predictables;


//-----------------------------------------------------------------------------
// Purpose: Add entity to list
// Input  : add - 
// Output : int
//-----------------------------------------------------------------------------
void CPredictableList::AddToPredictableList( ClientEntityHandle_t add )
{
	// This is a hack to remap slot to index
	if ( m_Predictables.Find( add ) != m_Predictables.InvalidIndex() )
	{
		return;
	}

	// Add to general list
	m_Predictables.AddToTail( add );

	// Maintain sort order by entindex
	int count = m_Predictables.Size();
	if ( count < 2 )
		return;

	int i, j;
	for ( i = 0; i < count; i++ )
	{
		for ( j = i + 1; j < count; j++ )
		{
			ClientEntityHandle_t h1 = m_Predictables[ i ];
			ClientEntityHandle_t h2 = m_Predictables[ j ];

			C_BaseEntity *p1 = cl_entitylist->GetBaseEntityFromHandle( h1 );
			C_BaseEntity *p2 = cl_entitylist->GetBaseEntityFromHandle( h2 );

			if ( !p1 || !p2 )
			{
				Assert( 0 );
				continue;
			}

			if ( p1->entindex() != -1 && 
				 p2->entindex() != -1 )
			{
				if ( p1->entindex() < p2->entindex() )
					continue;
			}

			if ( p2->entindex() == -1 )
				continue;

			m_Predictables[ i ] = h2;
			m_Predictables[ j ] = h1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : remove - 
//-----------------------------------------------------------------------------
void CPredictableList::RemoveFromPredictablesList( ClientEntityHandle_t remove )
{
	m_Predictables.FindAndRemove( remove );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : slot - 
// Output : C_BaseEntity
//-----------------------------------------------------------------------------
C_BaseEntity *CPredictableList::GetPredictable( int slot )
{
	return cl_entitylist->GetBaseEntityFromHandle( m_Predictables[ slot ] );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CPredictableList::GetPredictableCount( void )
{
	return m_Predictables.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Searc predictables for previously created entity (by testId)
// Input  : testId - 
// Output : static C_BaseEntity
//-----------------------------------------------------------------------------
static C_BaseEntity *FindPreviouslyCreatedEntity( CPredictableId& testId )
{
	int c = predictables->GetPredictableCount();

	int i;
	for ( i = 0; i < c; i++ )
	{
		C_BaseEntity *e = predictables->GetPredictable( i );
		if ( !e || !e->IsClientCreated() )
			continue;

		// Found it, note use of operator ==
		if ( testId == e->m_PredictableID )
		{
			return e;
		}
	}

	return NULL;
}

// Should these be somewhere else?
#define PITCH 0

// HACK HACK:  3/28/02 ywb Had to proxy around this or interpolation is borked in multiplayer, not sure what
//  the issue is, just a global optimizer bug I presume
#pragma optimize( "g", off )
//-----------------------------------------------------------------------------
// Purpose: Decodes animtime and notes when it changes
// Input  : *pStruct - ( C_BaseEntity * ) used to flag animtime is changine
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
void RecvProxy_AnimTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int t;
	int tickbase;
	int addt;

	// Unpack the data.
	addt	= pData->m_Value.m_Int;

	// Note, this needs to be encoded relative to packet timestamp, not raw client clock
	tickbase = 100 * (int)( gpGlobals->tickcount / 100 );

	t = tickbase;
											//  and then go back to floating point time.
	t += addt;				// Add in an additional up to 256 100ths from the server

	// center animtime around current time.
	while (t < gpGlobals->tickcount - 127)
		t += 256;
	while (t > gpGlobals->tickcount + 127)
		t -= 256;
	
	C_BaseEntity *pEntity = ( C_BaseEntity * )pStruct;
	Assert( pOut == &pEntity->m_flAnimTime );

	pEntity->m_flAnimTime = ( t * TICK_RATE );
}

void RecvProxy_SimulationTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int t;
	int tickbase;
	int addt;

	// Unpack the data.
	addt	= pData->m_Value.m_Int;

	// Note, this needs to be encoded relative to packet timestamp, not raw client clock
	tickbase = 100 * (int)( gpGlobals->tickcount / 100 );

	t = tickbase;
											//  and then go back to floating point time.
	t += addt;				// Add in an additional up to 256 100ths from the server

	// center animtime around current time.
	while (t < gpGlobals->tickcount - 127)
		t += 256;
	while (t > gpGlobals->tickcount + 127)
		t -= 256;
	
	C_BaseEntity *pEntity = ( C_BaseEntity * )pStruct;
	Assert( pOut == &pEntity->m_flSimulationTime );

	pEntity->m_flSimulationTime = ( t * TICK_RATE );
}

#pragma optimize( "g", on )

// Expose it to the engine.
IMPLEMENT_CLIENTCLASS(C_BaseEntity, DT_BaseEntity, CBaseEntity);

static void RecvProxy_MoveType( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_BaseEntity*)pStruct)->SetMoveType( (MoveType_t)(pData->m_Value.m_Int) );
}

static void RecvProxy_MoveCollide( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_BaseEntity*)pStruct)->SetMoveCollide( (MoveCollide_t)(pData->m_Value.m_Int) );
}

static void RecvProxy_Solid( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_BaseEntity*)pStruct)->SetSolid( (SolidType_t)pData->m_Value.m_Int );
}

static void RecvProxy_SolidFlags( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_BaseEntity*)pStruct)->SetSolidFlags( pData->m_Value.m_Int );
}



BEGIN_RECV_TABLE_NOBASE( C_BaseEntity, DT_AnimTimeMustBeFirst )
	RecvPropInt( RECVINFO(m_flAnimTime), 0, RecvProxy_AnimTime ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_BaseEntity, DT_PredictableId )
	RecvPropPredictableId( RECVINFO( m_PredictableID ) ),
	RecvPropInt( RECVINFO( m_bIsPlayerSimulated ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE(C_BaseEntity, DT_BaseEntity)
	RecvPropDataTable( "AnimTimeMustBeFirst", 0, 0, &REFERENCE_RECV_TABLE(DT_AnimTimeMustBeFirst) ),

	RecvPropInt( RECVINFO(m_flSimulationTime), 0, RecvProxy_SimulationTime ),
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[0], m_angRotation[0] ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[1], m_angRotation[1] ) ),
	RecvPropFloat( RECVINFO_NAME( m_angNetworkAngles[2], m_angRotation[2] ) ),
	RecvPropInt(RECVINFO(m_nModelIndex)),

	RecvPropInt(RECVINFO(m_fEffects)),
	RecvPropInt(RECVINFO(m_nRenderMode)),
	RecvPropInt(RECVINFO(m_nRenderFX)),
	RecvPropInt(RECVINFO(m_clrRender)),
	RecvPropInt(RECVINFO(m_iTeamNum)),
	RecvPropInt(RECVINFO(m_CollisionGroup)),
	RecvPropFloat(RECVINFO(m_flElasticity)),
	RecvPropFloat(RECVINFO(m_flShadowCastDistance)),
	RecvPropEHandle( RECVINFO(m_hOwnerEntity) ),
	RecvPropInt( RECVINFO_NAME(m_hNetworkMoveParent, moveparent), 0, RecvProxy_IntToMoveParent ),
	RecvPropInt( RECVINFO( m_iParentAttachment ) ),

	RecvPropInt( "movetype", 0, SIZEOF_IGNORE, 0, RecvProxy_MoveType ),
	RecvPropInt( "movecollide", 0, SIZEOF_IGNORE, 0, RecvProxy_MoveCollide ),
	RecvPropDataTable( RECVINFO_DT( m_Collision ), 0, &REFERENCE_RECV_TABLE(DT_CollisionProperty) ),
	
	RecvPropInt(RECVINFO(m_iTextureFrameIndex)),

	RecvPropDataTable( "predictable_id", 0, 0, &REFERENCE_RECV_TABLE( DT_PredictableId ) ),

	RecvPropInt		(RECVINFO(m_bSimulatedEveryTick)),
	RecvPropInt		(RECVINFO(m_bAnimatedEveryTick)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Global methods related to when abs data is correct
//-----------------------------------------------------------------------------
void C_BaseEntity::SetAbsQueriesValid( bool bValid )
{
	// FIXME: Enable this once the system has been purged of it's 
	// naughty calls.
//	s_bAbsQueriesValid = bValid;
}

void C_BaseEntity::EnableAbsRecomputations( bool bEnable )
{
	s_bAbsRecomputationEnabled = bEnable;
}


int	C_BaseEntity::GetTextureFrameIndex( void )
{
	return m_iTextureFrameIndex;
}

void C_BaseEntity::SetTextureFrameIndex( int iIndex )
{
	m_iTextureFrameIndex = iIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *map - 
//-----------------------------------------------------------------------------
void C_BaseEntity::Interp_SetupMappings( VarMapping_t *map )
{
	if( !map )
		return;

	int c = map->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;
		void *data = e->data;
		int type = e->type;

		watcher->Setup( data, type );
	}

	Interp_SetupMappings( map->m_pBaseClassVarMapping );
}

void C_BaseEntity::Interp_RestoreToLastNetworked( VarMapping_t *map )
{
	if( !map )
		return;

	int c = map->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;
		watcher->RestoreToLastNetworked();
	}

	Interp_RestoreToLastNetworked( map->m_pBaseClassVarMapping );
}

void C_BaseEntity::Interp_LatchChanges( VarMapping_t *map, int changeType )
{
	if( !map )
		return;

	int c = map->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;

		if ( watcher->GetType() & EXCLUDE_AUTO_LATCH )
			continue;

		watcher->NoteChanged( this, changeType );
	}

	Interp_LatchChanges( map->m_pBaseClassVarMapping, changeType );
}

void C_BaseEntity::Interp_Reset( VarMapping_t *map )
{
	if( !map )
		return;

	int c = map->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;

		watcher->Reset();
	}

	Interp_Reset( map->m_pBaseClassVarMapping );
}

void C_BaseEntity::Interp_Interpolate( VarMapping_t *map, float currentTime )
{
	if( !map )
		return;

	int c = map->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;

		if ( watcher->GetType() & EXCLUDE_AUTO_INTERPOLATE )
			continue;

		watcher->Interpolate( this, currentTime );
	}

	Interp_Interpolate( map->m_pBaseClassVarMapping, currentTime );
}

//-----------------------------------------------------------------------------
// Functions.
//-----------------------------------------------------------------------------
C_BaseEntity::C_BaseEntity()
{
	AddVar( &m_vecOrigin, &m_iv_vecOrigin, LATCH_SIMULATION_VAR );
	AddVar( &m_angRotation, &m_iv_angRotation, LATCH_SIMULATION_VAR );
	AddVar( &m_flAnimTime, &m_iv_flAnimTime, LATCH_SIMULATION_VAR );

	m_DataChangeEventRef = -1;

	m_iParentAttachment = 0;

	SetPredictionEligible( false );
	m_bPredictable = false;
	m_bPredictedEntity = false;

#ifdef _DEBUG
	m_vecAbsOrigin = vec3_origin;
	m_angAbsRotation = vec3_angle;
	m_vecNetworkOrigin.Init();
	m_angNetworkAngles.Init();
	m_vecAbsOrigin.Init();
//	m_vecAbsAngVelocity.Init();
	m_vecVelocity.Init();
	m_vecAbsVelocity.Init();
	m_vecViewOffset.Init();
	m_vecBaseVelocity.Init();

	m_iCurrentThinkContext = NO_THINK_CONTEXT;
#endif

	m_nSimulationTick = -1;

	// Assume drawing everything
	m_bReadyToDraw = true;
	m_bDrawnOnce = false;
	m_flProxyRandomValue = 0.0f;

	m_bVisualizingAbsBox = false;
	m_bVisualizingBBox = false;

	m_pPredictionContext = NULL;

	// setup touch list
	m_EntitiesTouched.nextLink = m_EntitiesTouched.prevLink = &m_EntitiesTouched;

	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
C_BaseEntity::~C_BaseEntity()
{
	Term();
	ClearDataChangedEvent( m_DataChangeEventRef );
	delete m_pPredictionContext;
}

void C_BaseEntity::Clear( void )
{
	m_bDormant = true;

	m_RefEHandle.Term();
	m_Partition = PARTITION_INVALID_HANDLE;
	m_ModelInstance = MODEL_INSTANCE_INVALID;
	m_ShadowHandle = CLIENTSHADOW_INVALID_HANDLE;
	m_hRender = INVALID_CLIENT_RENDER_HANDLE;
	m_hThink = INVALID_THINK_HANDLE;

	index = -1;
	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );
	model = NULL;
	m_Collision.Init( this );
	m_vecAbsOrigin.Init();
	m_angAbsRotation.Init();
	m_vecVelocity.Init();
	ClearFlags();
	m_vecViewOffset.Init();
	m_vecBaseVelocity.Init();
	m_nModelIndex = 0;
	m_flAnimTime = 0;
	m_flSimulationTime = 0;
	SetSolid( SOLID_NONE );
	SetSolidFlags( 0 );
	SetMoveCollide( MOVECOLLIDE_DEFAULT );
	SetMoveType( MOVETYPE_NONE );

	m_fEffects = 0;
	m_iEFlags = 0;
	m_nRenderMode = 0;
	SetRenderColor( 255, 255, 255, 255 );
	m_nRenderFX = 0;
	m_flFriction = 0.0f;       
	m_flGravity = 0.0f;
	SetCheckUntouch( false );

	m_nLastThinkTick = gpGlobals->tickcount;

	Q_memset(&mouth, 0, sizeof(mouth));

	// Remove prediction context if it exists
	delete m_pPredictionContext;
	m_pPredictionContext = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::Spawn( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::SpawnClientEntity( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Attach to entity
// Input  : *pEnt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::Init( int entnum, int iSerialNum )
{
	Assert( entnum >= 0 && entnum < NUM_ENT_ENTRIES );

	cl_entitylist->AddNetworkableEntity( GetIClientUnknown(), entnum, iSerialNum );

	// Put the entity into the spatial partition.
	Assert( m_Partition == PARTITION_INVALID_HANDLE );
	m_Partition = partition->CreateHandle( GetIClientUnknown() );

	Interp_SetupMappings( GetVarMapping() );

	index = entnum;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseEntity::InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup )
{
	int nModelIndex;
	if ( pszModelName != NULL )
	{
		nModelIndex = modelinfo->GetModelIndex( pszModelName );
	}
	else
	{
		nModelIndex = -1;
	}

	Interp_SetupMappings( GetVarMapping() );

	return InitializeAsClientEntityByIndex( nModelIndex, renderGroup );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseEntity::InitializeAsClientEntityByIndex( int iIndex, RenderGroup_t renderGroup )
{
	// Setup model data.
	SetModelByIndex( iIndex );

	// Add the client entity to the master entity list.
	cl_entitylist->AddNonNetworkableEntity( GetIClientUnknown() );
	Assert( GetClientHandle() != ClientEntityList().InvalidHandle() );

	// Add the client entity to the renderable "leaf system." (Renderable)
	SetupEntityRenderHandle( renderGroup );

	// Add the client entity to the spatial partition. (Collidable)
	Assert( m_Partition == PARTITION_INVALID_HANDLE );
	m_Partition = partition->CreateHandle( GetIClientUnknown() );

	index = -1;

	SpawnClientEntity();

	return true;
}


void C_BaseEntity::Term()
{
	UpdateOnRemove();

	C_BaseEntity::PhysicsRemoveTouchedList( this );

	// Remove from the predictables list
	if ( GetPredictable() || IsClientCreated() )
	{
		g_Predictables.RemoveFromPredictablesList( GetClientHandle() );
	}

	if ( GetClientHandle() != INVALID_CLIENTENTITY_HANDLE )
	{		
		ClientThinkList()->RemoveThinkable( GetClientHandle() );

		// Remove from the client entity list.
		ClientEntityList().RemoveEntity( GetClientHandle() );

		// If it's client created, remove from other lists as needed
		if ( IsClientCreated() )
		{
			if ( IsPlayerSimulated() )
			{
				if ( C_BasePlayer::GetLocalPlayer() )
				{
					C_BasePlayer::GetLocalPlayer()->RemoveFromPlayerSimulationList( this );
				}
			}

			m_RefEHandle = INVALID_CLIENTENTITY_HANDLE;
		}
	}
	
	// Are we in the partition?
	if ( m_Partition != PARTITION_INVALID_HANDLE )
	{
		partition->DestroyHandle( m_Partition );
	}

	// If Client side only entity index will be -1
	if ( index != -1 )
	{
		beams->KillDeadBeams( this );
	}

	// Clean up the model instance
	DestroyModelInstance();

	// Clean up the shadow
	if (m_ShadowHandle != CLIENTSHADOW_INVALID_HANDLE)
	{
		g_pClientShadowMgr->DestroyShadow(m_ShadowHandle);
		m_ShadowHandle = CLIENTSHADOW_INVALID_HANDLE;
	}

	RemoveFromLeafSystem();
}


void C_BaseEntity::SetRefEHandle( const CBaseHandle &handle )
{
	m_RefEHandle = handle;
}


const CBaseHandle& C_BaseEntity::GetRefEHandle() const
{
	return m_RefEHandle;
}

//-----------------------------------------------------------------------------
// Purpose: Free beams and destroy object
//-----------------------------------------------------------------------------
void C_BaseEntity::Release()
{
	C_BaseAnimating::PushAllowBoneAccess( true, true );

	UnlinkFromHierarchy();

	C_BaseAnimating::PopBoneAccess();

	// Note that this must be called from here, not the destructor, because otherwise the
	//  vtable is hosed and the derived classes function is not going to get called!!!
	if ( IsIntermediateDataAllocated() )
	{
		DestroyIntermediateData();
	}
	delete this;
}


//-----------------------------------------------------------------------------
// Only meant to be called from subclasses
//-----------------------------------------------------------------------------
void C_BaseEntity::CreateModelInstance()
{
	if (m_ModelInstance == MODEL_INSTANCE_INVALID)
	{
		m_ModelInstance = modelrender->CreateInstance( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::DestroyModelInstance()
{
	if (m_ModelInstance != MODEL_INSTANCE_INVALID)
	{
		modelrender->DestroyInstance( m_ModelInstance );
		m_ModelInstance = MODEL_INSTANCE_INVALID;
	}
}

void C_BaseEntity::SetRemovalFlag( bool bRemove ) 
{ 
	if (bRemove) 
		m_iEFlags |= EFL_KILLME; 
	else 
		m_iEFlags &= ~EFL_KILLME; 
}

//-----------------------------------------------------------------------------
// Effects...
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsEffectActive( int nEffectMask ) const
{
	return (m_fEffects & nEffectMask) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nEffectMask - 
//			bActive - 
//-----------------------------------------------------------------------------
void C_BaseEntity::ActivateEffect( int nEffectMask, bool bActive )
{
	if (bActive)
		m_fEffects |= nEffectMask;
	else
		m_fEffects &= ~nEffectMask;
}


//-----------------------------------------------------------------------------
// Returns the health fraction
//-----------------------------------------------------------------------------
float C_BaseEntity::HealthFraction() const
{
	if (GetMaxHealth() == 0)
		return 1.0f;

	float flFraction = (float)GetHealth() / (float)GetMaxHealth();
	flFraction = clamp( flFraction, 0.0f, 1.0f );
	return flFraction;
}


//-----------------------------------------------------------------------------
// Purpose: Retrieves the coordinate frame for this entity.
// Input  : forward - Receives the entity's forward vector.
//			right - Receives the entity's right vector.
//			up - Receives the entity's up vector.
//-----------------------------------------------------------------------------
void C_BaseEntity::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
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
// Purpose: Returns whether object should render.
//-----------------------------------------------------------------------------
bool C_BaseEntity::ShouldDraw()
{
	// Let the client mode (like commander mode) reject drawing entities.
	if (g_pClientMode && !g_pClientMode->ShouldDrawEntity(this) )
		return false;

	C_BasePlayer * player = C_BasePlayer::GetLocalPlayer();

	if ( player && player->GetObserverMode() == OBS_MODE_IN_EYE )
	{
		// if local player is spectating this entity in first person mode, don't draw it
		if ( player->GetObserverTarget() == this )
			return false;
	}

	return (model != 0) && ( (m_fEffects & EF_NODRAW) == 0) && (index != 0);
}

bool C_BaseEntity::TestCollision( const Ray_t& ray, unsigned int mask, trace_t& trace )
{
	return false;
}

bool C_BaseEntity::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::ComputeSurroundingBox( void )
{
	Vector vecAbsMins, vecAbsMaxs;
	WorldSpaceAABB( &vecAbsMins, &vecAbsMaxs );
	SetAbsMins( vecAbsMins );
	SetAbsMaxs( vecAbsMaxs );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_BaseEntity::SetObjectCollisionBox( void )
{
	ComputeSurroundingBox();

	SetAbsMins( GetAbsMins() + Vector( -1, -1, -1 ) );
	SetAbsMaxs( GetAbsMaxs() + Vector( 1, 1, 1 ) );
}

//-----------------------------------------------------------------------------
// Purpose: Derived classes will have to write their own message cracking routines!!!
// Input  : length - 
//			*data - 
//-----------------------------------------------------------------------------
void C_BaseEntity::ReceiveMessage( const char *msgname, int length, void *data )
{
	// Nothing
}


void* C_BaseEntity::GetDataTableBasePtr()
{
	return this;
}


//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_BaseEntity::ShadowCastType()
{
	if (m_fEffects & (EF_NODRAW | EF_NOSHADOW))
		return SHADOWS_NONE;

	int modelType = modelinfo->GetModelType( model );
	return (modelType == mod_studio) ? SHADOWS_RENDER_TO_TEXTURE : SHADOWS_NONE;
}


//-----------------------------------------------------------------------------
// Should this object receive shadows?
//-----------------------------------------------------------------------------
bool C_BaseEntity::ShouldReceiveShadows()
{
	return (m_fEffects & (EF_NODRAW | EF_NORECEIVESHADOW)) == 0;
}


//-----------------------------------------------------------------------------
// Purpose: Returns index into entities list for this entity
// Output : Index
//-----------------------------------------------------------------------------
int	C_BaseEntity::entindex( void )
{
	return index;
}

//-----------------------------------------------------------------------------
// Get render origin and angles
//-----------------------------------------------------------------------------
const Vector& C_BaseEntity::GetRenderOrigin( void )
{
	return GetAbsOrigin();
}

const QAngle& C_BaseEntity::GetRenderAngles( void )
{
	return GetAbsAngles();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : theMins - 
//			theMaxs - 
//-----------------------------------------------------------------------------
void C_BaseEntity::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	if (modelinfo->GetModelType( model ) == mod_studio)
	{
		modelinfo->GetModelRenderBounds( GetModel(), 0, theMins, theMaxs );
	}
	else
	{
		theMins = EntitySpaceSurroundingMins();
		theMaxs = EntitySpaceSurroundingMaxs();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Last received origin
// Output : const float
//-----------------------------------------------------------------------------
const Vector& C_BaseEntity::GetAbsOrigin( void ) const
{
	Assert( s_bAbsQueriesValid );
	const_cast<C_BaseEntity*>(this)->CalcAbsolutePosition();
	return m_vecAbsOrigin;
}


//-----------------------------------------------------------------------------
// Purpose: Last received angles
// Output : const
//-----------------------------------------------------------------------------
const QAngle& C_BaseEntity::GetAbsAngles( void ) const
{
	Assert( s_bAbsQueriesValid );
	const_cast<C_BaseEntity*>(this)->CalcAbsolutePosition();
	return m_angAbsRotation;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : org - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetNetworkOrigin( const Vector& org )
{
	m_vecNetworkOrigin = org;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ang - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetNetworkAngles( const QAngle& ang )
{
	m_angNetworkAngles = ang;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const Vector&
//-----------------------------------------------------------------------------
const Vector& C_BaseEntity::GetNetworkOrigin() const
{
	return m_vecNetworkOrigin;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const QAngle&
//-----------------------------------------------------------------------------
const QAngle& C_BaseEntity::GetNetworkAngles() const
{
	return m_angNetworkAngles;
}


//-----------------------------------------------------------------------------
// Purpose: Get current model pointer for this entity
// Output : const struct model_s
//-----------------------------------------------------------------------------
const model_t *C_BaseEntity::GetModel( void )
{
	return model;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::ShouldCacheRenderInfo()
{
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Get model index for this entity
// Output : int - model index
//-----------------------------------------------------------------------------
int C_BaseEntity::GetModelIndex( void ) const
{
	return m_nModelIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetModelIndex( int index )
{
	m_nModelIndex = index;
	const model_t *pModel = modelinfo->GetModel( GetModelIndex() );
	SetModelPointer( pModel );
}

void C_BaseEntity::SetModelPointer( const model_t *pModel )
{
	if ( pModel != model )
	{
		DestroyModelInstance();
		model = pModel;
		OnNewModel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : val - 
//			moveCollide - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetMoveType( MoveType_t val, MoveCollide_t moveCollide /*= MOVECOLLIDE_DEFAULT*/ )
{
	// Make sure the move type + move collide are compatible...
#ifdef _DEBUG
	if ((val != MOVETYPE_FLY) && (val != MOVETYPE_FLYGRAVITY))
	{
		Assert( moveCollide == MOVECOLLIDE_DEFAULT );
	}
#endif

	m_MoveType = val;
	SetMoveCollide( moveCollide );
}

void C_BaseEntity::SetMoveCollide( MoveCollide_t val )
{
	m_MoveCollide = val;
}


//-----------------------------------------------------------------------------
// Purpose: Get rendermode
// Output : int - the render mode
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsTransparent( void )
{
	bool modelIsTransparent = modelinfo->IsTranslucent(model);
	return modelIsTransparent || (m_nRenderMode != kRenderNormal);
}


bool C_BaseEntity::UsesFrameBufferTexture()
{
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Get pointer to CMouthInfo data
// Output : CMouthInfo
//-----------------------------------------------------------------------------
CMouthInfo *C_BaseEntity::GetMouth( void )
{
	return &mouth;
}


//-----------------------------------------------------------------------------
// Purpose: Retrieve sound spatialization info for the specified sound on this entity
// Input  : info - 
// Output : Return false to indicate sound is not audible
//-----------------------------------------------------------------------------
bool C_BaseEntity::GetSoundSpatialization( SpatializationInfo_t& info )
{
	// World is always audible
	if ( entindex() == 0 )
	{
		return true;
	}

	// Out of PVS
	if ( IsDormant() )
	{
		return false;
	}
	
	const model_t *pModel = GetModel();
	if ( !pModel )
	{
		return true;
	}

	if ( info.pflRadius )
	{
		*info.pflRadius = modelinfo->GetModelRadius( pModel );
	}
	
	if ( info.pOrigin )
	{
		*info.pOrigin = GetAbsOrigin();
		if ( modelinfo->GetModelType( pModel ) == mod_brush )
		{
			Vector mins, maxs, center;

			modelinfo->GetModelBounds( pModel, mins, maxs );
			VectorAdd( mins, maxs, center );
			VectorScale( center, 0.5f, center );

			(*info.pOrigin) += center;
		}
	}

	if ( info.pAngles )
	{
		VectorCopy( GetAbsAngles(), *info.pAngles );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get attachment point by index
// Input  : number - which point
// Output : float * - the attachment point
//-----------------------------------------------------------------------------
bool C_BaseEntity::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	origin = GetAbsOrigin();
	angles = GetAbsAngles();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsIdentityBrush( void )
{
	if ( modelinfo->GetModelType( model ) != mod_brush )
		return false;

	if (IsTransparent())
		return false;

	if ( GetAbsOrigin()[0] != 0.0f ||
		GetAbsOrigin()[1] != 0.0f || 
		GetAbsOrigin()[2] != 0.0f )
	{
		return false;
	}
	
	if ( fmod(GetAbsAngles()[0], 360) != 0 || 
		fmod(GetAbsAngles()[1], 360) != 0 ||
		fmod(GetAbsAngles()[2], 360) != 0 )
	{
		return false;
	}
	
	// It's not an identity brush if its model has a material which uses a proxy.
	if (modelinfo->ModelHasMaterialProxy( model ))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::DrawBrushModel( bool sort )
{
	VPROF_BUDGET( "C_BaseEntity::DrawBrushModel", VPROF_BUDGETGROUP_BRUSHMODEL_RENDERING );
	MEASURE_TIMED_STAT( CS_DRAW_BRUSH_MODEL_TIME );
	// Identity brushes are drawn in view->DrawWorld as an optimization
	Assert ( modelinfo->GetModelType( model ) == mod_brush );
	Assert ( !IsIdentityBrush() );

	render->DrawBrushModel( this, (model_t *)model, GetAbsOrigin(), GetAbsAngles(), sort );
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the object
// Input  : flags - 
//-----------------------------------------------------------------------------
int C_BaseEntity::DrawModel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

	int drawn = 0;
	if ( !model )
	{
		return drawn;
	}

	int modelType = modelinfo->GetModelType( model );
	switch ( modelType )
	{
	case mod_brush:
		drawn = DrawBrushModel( flags & STUDIO_TRANSPARENCY ? true : false );
		break;
	case mod_studio:
		// All studio models must be derived from C_BaseAnimating.  Issue warning.
		Warning( "ERROR:  Can't draw studio model %s because %s is not derived from C_BaseAnimating\n",
			modelinfo->GetModelName( model ), GetClientClass()->m_pNetworkName ? GetClientClass()->m_pNetworkName : "unknown" );
		break;
	case mod_sprite:
		//drawn = DrawSprite();
		Warning( "ERROR:  Sprite model's not supported any more except in legacy temp ents\n" );
		break;
	default:
		break;
	}

	// If we're visualizing our bboxes, draw them
	if ( m_bVisualizingBBox || m_bVisualizingAbsBox )
	{
		DrawBBoxVisualizations();
	}

	return drawn;
}

//-----------------------------------------------------------------------------
// Purpose: Return false if the object should be culled by the lod test
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::LODTest()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Setup the bones for drawing
//-----------------------------------------------------------------------------
bool C_BaseEntity::SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Setup vertex weights for drawing
//-----------------------------------------------------------------------------
void C_BaseEntity::SetupWeights( )
{
}

//-----------------------------------------------------------------------------
// Purpose: Process any local client-side animation events
//-----------------------------------------------------------------------------
void C_BaseEntity::DoAnimationEvents( )
{
}


void C_BaseEntity::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	// Init should have been called before we get in here.
	Assert( m_Partition != PARTITION_INVALID_HANDLE );
	if ( entindex() < 0 )
		return;
	
	switch( state )
	{
	case SHOULDTRANSMIT_START:
		{
			// We've just been sent by the server. Become active.
			SetDormant( false );
			
			// Don't add the world entity
			CollideType_t shouldCollide = ShouldCollide();

			// Choose the list based on what kind of collisions we want
			int list = PARTITION_CLIENT_NON_STATIC_EDICTS;
			if (shouldCollide == ENTITY_SHOULD_COLLIDE)
				list |= PARTITION_CLIENT_SOLID_EDICTS;
			else if (shouldCollide == ENTITY_SHOULD_RESPOND)
				list |= PARTITION_CLIENT_RESPONSIVE_EDICTS;
			
			// add the entity to the KD tree so we will collide against it
			partition->Insert( list, m_Partition );

			// Note that predictables get a chance to hook up to their server counterparts here
			if ( m_PredictableID.IsActive() )
			{
				// Find corresponding client side predicted entity and remove it from predictables
				m_PredictableID.SetAcknowledged( true );

				C_BaseEntity *otherEntity = FindPreviouslyCreatedEntity( m_PredictableID );
				if ( otherEntity )
				{
					Assert( otherEntity->IsClientCreated() );
					Assert( otherEntity->m_PredictableID.IsActive() );
					Assert( ClientEntityList().IsHandleValid( otherEntity->GetClientHandle() ) );

					otherEntity->m_PredictableID.SetAcknowledged( true );

					if ( OnPredictedEntityRemove( false, otherEntity ) )
					{
						// Mark it for delete after receive all network data
						otherEntity->Release();
					}
				}
			}
		}
		break;

	case SHOULDTRANSMIT_END:
		{
			// Clear out links if we're out of the picture...
			UnlinkFromHierarchy();

			// We're no longer being sent by the server. Become dormant.
			SetDormant( true );
			
			// remove the entity from the KD tree so we won't collide against it
			partition->Remove( PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS, m_Partition );
			
			// Remove it from the BSP tree.
			if ( m_hRender != INVALID_CLIENT_RENDER_HANDLE )
			{
				ClientLeafSystem()->RemoveRenderable( m_hRender );
				m_hRender = INVALID_CLIENT_RENDER_HANDLE;
			}
		}
		break;

	case SHOULDTRANSMIT_STAY:
		{
			partition->Remove( PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS, m_Partition );
			
			// remove the entity from the KD tree so we won't collide against it
			CollideType_t shouldCollide = ShouldCollide();
			
			// Choose the list based on what kind of collisions we want
			int list = PARTITION_CLIENT_NON_STATIC_EDICTS;
			if (shouldCollide == ENTITY_SHOULD_COLLIDE)
				list |= PARTITION_CLIENT_SOLID_EDICTS;
			else if (shouldCollide == ENTITY_SHOULD_RESPOND)
				list |= PARTITION_CLIENT_RESPONSIVE_EDICTS;
			
			// add the entity to the KD tree so we will collide against it
			partition->Insert( list, m_Partition );
		}
		break;
	default:
		Assert( 0 );
		break;
	}
}

//-----------------------------------------------------------------------------
// Call this in PostDataUpdate if you don't chain it down!
//-----------------------------------------------------------------------------
void C_BaseEntity::MarkMessageReceived()
{
	m_flLastMessageTime = engine->GetLastTimeStamp();
}


//-----------------------------------------------------------------------------
// Purpose: Entity is about to be decoded from the network stream
// Input  : bnewentity - is this a new entity this update?
//-----------------------------------------------------------------------------
void C_BaseEntity::PreDataUpdate( DataUpdateType_t updateType )
{
	// Register for an OnDataChanged call and call OnPreDataChanged().
	if ( AddDataChangeEvent( this, updateType, &m_DataChangeEventRef ) )
	{
		OnPreDataChanged( updateType );
	}


	// Need to spawn on client before receiving original network data 
	// in case it overrides any values set up in spawn ( e.g., m_iState )
	bool bnewentity = (updateType == DATA_UPDATE_CREATED);

	if ( !bnewentity )
	{
		Interp_RestoreToLastNetworked( GetVarMapping() );
	}

	if ( bnewentity && !IsClientCreated() )
	{
		m_flSpawnTime = engine->GetLastTimeStamp();
		Spawn();
	}

	// If the entity moves itself every FRAME on the server but doesn't update animtime,
	// then use the current server time as the time for interpolation.
	if ( IsSelfAnimating() )
	{
		m_flAnimTime = engine->GetLastTimeStamp();	
	}

	m_vecOldOrigin = GetNetworkOrigin();
	m_vecOldAngRotation = GetNetworkAngles();

	m_flOldAnimTime = m_flAnimTime;
	m_flOldSimulationTime = m_flSimulationTime;
}


void C_BaseEntity::UnlinkChild( C_BaseEntity *pParent, C_BaseEntity *pChild )
{
	Assert( pChild );
	Assert( pParent != pChild );
	Assert( pChild->GetMoveParent() == pParent );

	// Unlink from parent
	// NOTE: pParent *may well be NULL*! This occurs
	// when a child has unlinked from a parent, and the child
	// remains in the PVS but the parent has not
	if (pParent && (pParent->m_pMoveChild == pChild))
	{
		Assert( !(pChild->m_pMovePrevPeer.IsValid()) );
		pParent->m_pMoveChild = pChild->m_pMovePeer;
	}

	// Unlink from siblings...
	if (pChild->m_pMovePrevPeer)
	{
		pChild->m_pMovePrevPeer->m_pMovePeer = pChild->m_pMovePeer;
	}
	if (pChild->m_pMovePeer)
	{
		pChild->m_pMovePeer->m_pMovePrevPeer = pChild->m_pMovePrevPeer;
	}

	pChild->m_pMovePeer = NULL;
	pChild->m_pMovePrevPeer = NULL;
	pChild->m_pMoveParent = NULL;
}

void C_BaseEntity::LinkChild( C_BaseEntity *pParent, C_BaseEntity *pChild )
{
	Assert( !pChild->m_pMovePeer.IsValid() );
	Assert( !pChild->m_pMovePrevPeer.IsValid() );
	Assert( !pChild->m_pMoveParent.IsValid() );
	Assert( pParent != pChild );

#ifdef _DEBUG
	// Make sure the child isn't already in this list
	C_BaseEntity *pExistingChild;
	for ( pExistingChild = pParent->FirstMoveChild(); pExistingChild; pExistingChild = pExistingChild->NextMovePeer() )
	{
		Assert( pChild != pExistingChild );
	}
#endif

	pChild->m_pMovePrevPeer = NULL;
	pChild->m_pMovePeer = pParent->m_pMoveChild;
	if (pChild->m_pMovePeer)
	{
		pChild->m_pMovePeer->m_pMovePrevPeer = pChild;
	}
	pParent->m_pMoveChild = pChild;
	pChild->m_pMoveParent = pParent;
}

//-----------------------------------------------------------------------------
// Connects us up to hierarchy
//-----------------------------------------------------------------------------
void C_BaseEntity::HierarchySetParent( C_BaseEntity *pNewParent )
{
	// NOTE: When this is called, we expect to have a valid
	// local origin, etc. that we received from network daa
	EHANDLE newParentHandle;
	newParentHandle.Set( pNewParent );
	if (newParentHandle.ToInt() == m_pMoveParent.ToInt())
		return;
	
	if (m_pMoveParent.IsValid())
	{
		UnlinkChild( m_pMoveParent, this );
	}

	if (pNewParent)
	{
		LinkChild( pNewParent, this );
	}
}


//-----------------------------------------------------------------------------
// Unlinks from hierarchy
//-----------------------------------------------------------------------------
void C_BaseEntity::SetParent( C_BaseEntity *pParentEntity )
{
	// NOTE: This version is meant to be called *outside* of PostDataUpdate
	// as it assumes the moveparent has a valid handle
	EHANDLE newParentHandle;
	newParentHandle.Set( pParentEntity );
	if (newParentHandle.ToInt() == m_pMoveParent.ToInt())
		return;

	// NOTE: Have to do this before the unlink to ensure local coords are valid
	Vector vecAbsOrigin = GetAbsOrigin();
	QAngle angAbsRotation = GetAbsAngles();
	Vector vecAbsVelocity = GetAbsVelocity();
//	QAngle vecAbsAngVelocity = GetAbsAngularVelocity();

	// First deal with unlinking
	if (m_pMoveParent.IsValid())
	{
		UnlinkChild( m_pMoveParent, this );
	}

	if (pParentEntity)
	{
		LinkChild( pParentEntity, this );
	}
	
	SetAbsOrigin(vecAbsOrigin);
	SetAbsAngles(angAbsRotation);
	SetAbsVelocity(vecAbsVelocity);
//	SetAbsAngularVelocity(vecAbsAngVelocity);
}


//-----------------------------------------------------------------------------
// Unlinks from hierarchy
//-----------------------------------------------------------------------------
void C_BaseEntity::UnlinkFromHierarchy()
{
	// Clear out links if we're out of the picture...
	SetParent( NULL );

	C_BaseEntity *pChild = FirstMoveChild();
	while( pChild )
	{
		pChild->SetParent( NULL );
		pChild = FirstMoveChild();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Entity data has been parsed and unpacked.  Now do any necessary decoding, munging
// Input  : bnewentity - was this entity new in this update packet?
//-----------------------------------------------------------------------------
void C_BaseEntity::PostDataUpdate( DataUpdateType_t updateType )
{
	// NOTE: This *has* to happen first. Otherwise, Origin + angles may be wrong 
	if ( m_nRenderFX == kRenderFxRagdoll && updateType == DATA_UPDATE_CREATED )
	{
		MoveToLastReceivedPosition( true );
	}
	else
	{
		MoveToLastReceivedPosition( false );
	}

	// If it's the world, force solid flags
	if ( index == 0 )
	{
		m_nModelIndex = 1;
		SetSolid( SOLID_BSP );

		// FIXME: Should these be assertions?
		SetAbsOrigin( vec3_origin );
		SetAbsAngles( vec3_angle );
	}

	bool animTimeChanged = ( m_flAnimTime != m_flOldAnimTime ) ? true : false;
	bool originChanged = ( m_vecOldOrigin != GetLocalOrigin() ) ? true : false;
	bool anglesChanged = ( m_vecOldAngRotation != GetLocalAngles() ) ? true : false;
	bool simTimeChanged = ( m_flSimulationTime != m_flOldSimulationTime ) ? true : false;

	// Detect simulation changes 
	bool simulationChanged = originChanged || anglesChanged || simTimeChanged;

	int changedFlags = 0;
	
	if ( animTimeChanged )
	{
		changedFlags |= LATCH_ANIMATION_VAR;
	}
	if ( simulationChanged )
	{
		changedFlags |= LATCH_SIMULATION_VAR;
	}

	if ( GetPredictable() || IsClientCreated() )
	{
		changedFlags = 0;
	}

	if ( changedFlags != 0 )
	{
		OnLatchInterpolatedVariables( changedFlags );
	}

	// Deal with hierarchy. Have to do it here (instead of in a proxy)
	// because this is the only point at which all entities are loaded
	// If this condition isn't met, then a child was sent without its parent
	Assert( m_hNetworkMoveParent.Get() || !m_hNetworkMoveParent.IsValid() );
	HierarchySetParent(m_hNetworkMoveParent);

	SetSize( GetAbsMaxs() - GetAbsMins() );

	MarkMessageReceived();

	// Make sure that the correct model is referenced for this entity
	SetModelByIndex( m_nModelIndex );

	// If this entity was new, then latch in various values no matter what.
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Construct a random value for this instance
		m_flProxyRandomValue = random->RandomFloat( 0, 1 );

		// Zero out all but last update.
		ResetLatched();
	}

	CheckInitPredictable( "PostDataUpdate" );

	// It's possible that a new entity will need to be forceably added to the 
	//   player simulation list.  If so, do this here
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();

	if ( IsPlayerSimulated() &&
		( NULL != local ) && 
		( local == m_hOwnerEntity ) )
	{
		// Make sure player is driving simulation (field is only ever sent to local player)
		SetPlayerSimulated( local );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *context - 
//-----------------------------------------------------------------------------
void C_BaseEntity::CheckInitPredictable( const char *context )
{
	// Prediction is disabled
	if ( !cl_predict.GetBool() )
		return;

	if ( !GetPredictionEligible() )
	{
		if ( m_PredictableID.IsActive() &&
			( engine->GetPlayer() - 1 ) == m_PredictableID.GetPlayer() )
		{
			// If it comes through with an ID, it should be eligible
			SetPredictionEligible( true );
		}
		else
		{
			return;
		}
	}

	if ( IsClientCreated() )
		return;

	if ( !ShouldPredict() )
		return;

	if ( IsIntermediateDataAllocated() )
		return;

	// Msg( "Predicting init %s at %s\n", GetClassname(), context );

	InitPredictable();
}

bool C_BaseEntity::IsSelfAnimating()
{
	return true;
}


//-----------------------------------------------------------------------------
// Sets the model... 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetModelByIndex( int nModelIndex )
{
	SetModelIndex( nModelIndex );
}


//-----------------------------------------------------------------------------
// Set model... (NOTE: Should only be used by client-only entities
//-----------------------------------------------------------------------------
bool C_BaseEntity::SetModel( const char *pModelName )
{
	if ( pModelName )
	{
		int nModelIndex = modelinfo->GetModelIndex( pModelName );
		SetModelByIndex( nModelIndex );
		return ( nModelIndex != -1 );
	}
	else
	{
		SetModelByIndex( -1 );
		return false;
	}
}
//-----------------------------------------------------------------------------
// Purpose: The animtime is about to be changed in a network update, store off various fields so that
//  we can use them to do blended sequence transitions, etc.
// Input  : *pState - the (mostly) previous state data
//-----------------------------------------------------------------------------
void C_BaseEntity::OnLatchInterpolatedVariables( int flags )
{
	Interp_LatchChanges( GetVarMapping(), flags );
}

//-----------------------------------------------------------------------------
// Purpose: Default interpolation for entities
// Output : true means entity should be drawn, false means probably not
//-----------------------------------------------------------------------------
bool C_BaseEntity::Interpolate( float currentTime )
{
	// Don't mess with the world!!!
	if ( index == 0 )
	{
		return true;
	}

	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();

	// Predicted entities are up to date, no need to interpolate or slam origin
	if ( local && GetPredictable() || IsClientCreated() )
	{
		currentTime = local->GetFinalPredictedTime() + gpGlobals->interpolation_amount;
		//return true;
	}

	// HACK HACK:
	// If standing on something that's moving and we're playing back a demo, don't slam back to
	//  original position here because we're using the player's eyes to set this position of this origin. (see prediction.cpp)
	if ( render->IsPlayingDemo() )
	{
		if ( local && ( this == local->GetGroundEntity() ) )
		{
			return true;
		}
	}

	// Assume current origin ( no interpolation )
	MoveToLastReceivedPosition();

	if ( !cl_interpolate.GetInt() )
	{
		return true;
	}

	// Don't interpolate during timedemo playback
	if ( render->IsPlayingTimeDemo() )
	{
		return true;
	}

	// Can only interpolate visible ents
	if ( !model )
	{
		return true;
	}

	// These get moved to the parent position automatically
	if ( IsFollowingEntity() )
	{
		return true;
	}

	Interp_Interpolate( GetVarMapping(), currentTime );
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM );

	/*
	if ( index == -1 )
	{
		float interpTime = GetInterpolationAmount( LATCH_SIMULATION_VAR );

		Msg( "%i/%i time %f interp %f org %f %f %f\n",
			gpGlobals->framecount,
			gpGlobals->tickcount,
			currentTime,
			interpTime,
			GetLocalOrigin().x,
			GetLocalOrigin().y,
			GetLocalOrigin().z,
			GetLocalAngles().x );

		int i;
		for ( i = m_iv_vecOrigin.GetHead(); m_iv_vecOrigin.IsValidIndex( i ); i = m_iv_vecOrigin.GetNext( i ) )
		{
			float changeTime;
			Vector *a;

			a = m_iv_vecOrigin.GetHistoryValue( i, changeTime );

			Msg( "  at %f org %f %f %f\n", changeTime, a->x, a->y, a->z );
		}
	}

	if ( index == -1 )
	{
		static float lastt;
		static float lastyaw;
		static QAngle lastnetworkangles;
		static float lastnetworkanglestick;

		QAngle ang = GetNetworkAngles();
		bool achanged = ang != lastnetworkangles;
		if ( achanged )
		{
			float now = gpGlobals->tickcount * TICK_RATE;

			float dnetworkyaw = ang.y - lastnetworkangles.y;

			float dt = now - lastnetworkanglestick;
			if ( dt > 0.0f )
			{
				float yv = dnetworkyaw / dt;

				Msg( "%i/%i sample angular vel %f over dt %f\n",
					gpGlobals->framecount,
					gpGlobals->tickcount,
					yv, dt );
			}


			lastnetworkanglestick = now;
			lastnetworkangles = ang;

		}

		float dt = currentTime - lastt;

		if ( dt > 0.0001f )
		{
			float dyaw = GetLocalAngles().y - lastyaw;
			float angvel = dyaw / dt;

			Msg( "%i/%i time %f yaw %f last %f dyaw %f vel %f\n",
				gpGlobals->framecount,
				gpGlobals->tickcount,
				currentTime,
				GetLocalAngles().y, 
				lastyaw, 
				dyaw,
				angvel );
		}

		lastyaw = GetLocalAngles().y;
		lastt = currentTime;

		int i;
		for ( i = m_iv_angRotation.GetHead(); m_iv_angRotation.IsValidIndex( i ); i = m_iv_angRotation.GetNext( i ) )
		{
			float changeTime;
			QAngle *a;

			a = m_iv_angRotation.GetHistoryValue( i, changeTime );

			Msg( "  --- at %f ang yaw %f\n", changeTime, a->y );
		}

	}

	if ( index == -1 )
	{
		C_BaseAnimating *anim = dynamic_cast< C_BaseAnimating * >( this );
		if ( anim )
		{
			float interpTime = GetInterpolationAmount( LATCH_SIMULATION_VAR );

			Msg( "%i/%i time %f interp %f pp %f\n",
				gpGlobals->framecount,
				gpGlobals->tickcount,
				currentTime - interpTime,
				interpTime,
				anim->m_flPoseParameter[ 0 ] );

			int i;
			for ( i = anim->m_iv_flPoseParameter.GetHead(); anim->m_iv_flPoseParameter.IsValidIndex( i ); i = anim->m_iv_flPoseParameter.GetNext( i ) )
			{
				float changeTime;
				float *a;

				a = anim->m_iv_flPoseParameter.GetHistoryValue( i, 0, changeTime );

				Msg( "  at %f pp %f\n", changeTime, *a);
			}
		}
	}
	*/
//	Relink();

	return true;
}

studiohdr_t *C_BaseEntity::OnNewModel()
{
	return NULL;
}

// Above this velocity and we'll assume a warp/teleport
#define MAX_INTERPOLATE_VELOCITY 4000.0f
#define MAX_INTERPOLATE_VELOCITY_PLAYER 1250.0f

//-----------------------------------------------------------------------------
// Purpose: Determine whether entity was teleported ( so we can disable interpolation )
// Input  : *ent - 
// Output : bool
//-----------------------------------------------------------------------------
bool C_BaseEntity::Teleported( void )
{
	// Disable interpolation when hierarchy changes
	if (m_hOldMoveParent != m_hNetworkMoveParent)
	{
		return true;
	}

	// Predicted entities are up to date, no need to interpolate or slam origin
	if ( GetPredictable() || IsClientCreated() )
	{
		return false;
	}

	// bool isplayer = IsPlayer();

	//float maxvel = isplayer ? MAX_INTERPOLATE_VELOCITY_PLAYER : MAX_INTERPOLATE_VELOCITY;

	/*
	// FIXME!!!
	if ( ph.Count() >= 2 )
	{
		float dt = ph[0].animtime - ph[1].animtime;
		if ( dt > 0.0f )
		{
			Vector d = ph[0].origin - ph[1].origin;
			float vel = d.Length() / dt;
			return  ( vel > maxvel ) ? true : false;
		}
	}
	*/

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Is this a submodel of the world ( model name starts with * )?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsSubModel( void )
{
	if ( model &&
		modelinfo->GetModelType( model ) == mod_brush &&
		modelinfo->GetModelName( model )[0] == '*' )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Create entity lighting effects
//-----------------------------------------------------------------------------
void C_BaseEntity::CreateLightEffects( void )
{
	dlight_t *dl;

	// Is this for player flashlights only, if so move to linkplayers?
	if ( index == render->GetViewEntity() )
		return;

	if (m_fEffects & EF_BRIGHTLIGHT)
	{
		dl = effects->CL_AllocDlight ( index );
		dl->origin = GetAbsOrigin();
		dl->origin[2] += 16;
		dl->color.r = dl->color.g = dl->color.b = 250;
		dl->radius = random->RandomFloat(400,431);
		dl->die = gpGlobals->curtime + 0.001;
	}
	if (m_fEffects & EF_DIMLIGHT)
	{			
		dl = effects->CL_AllocDlight ( index );
		dl->origin = GetAbsOrigin();
		dl->color.r = dl->color.g = dl->color.b = 100;
		dl->radius = random->RandomFloat(200,231);
		dl->die = gpGlobals->curtime + 0.001;
	}
}

void C_BaseEntity::MoveToLastReceivedPosition( bool force )
{
	if ( force || ( m_nRenderFX != kRenderFxRagdoll ) )
	{
		SetLocalOrigin( GetNetworkOrigin() );
		SetLocalAngles( GetNetworkAngles() );
	}
}


void C_BaseEntity::UpdatePosition()
{  
	// Predicted entities are up to date, no need to reset origin
	if ( !GetPredictable() && 
		 !IsClientCreated() )
	{
		// Did the entity move too far for interpolation?
		bool teleport = Teleported();
		bool ef_nointerp = IsEffectActive(EF_NOINTERP);
	
		if ( teleport || ef_nointerp )
		{
			// Zero out all but last update.
			ResetLatched();
		}
	}

	// Now interpolate
	m_bReadyToDraw = Interpolate( gpGlobals->curtime );
	if ( m_bReadyToDraw )
	{
		m_bDrawnOnce = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add entity to visibile entities list
//-----------------------------------------------------------------------------
void C_BaseEntity::AddEntity( void )
{
	// Don't ever add the world, it's drawn separately
	if ( index == 0 )
		return;

	// Create flashlight effects, etc.
	CreateLightEffects();
}


//-----------------------------------------------------------------------------
// Returns the aiment render origin + angles
//-----------------------------------------------------------------------------
void C_BaseEntity::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pOrigin, QAngle *pAngles )
{
	// Should be overridden for things that attach to attchment points

	// Slam origin to the origin of the entity we are attached to...
	*pOrigin = pAttachedTo->GetAbsOrigin();
	*pAngles = pAttachedTo->GetAbsAngles();
}


//-----------------------------------------------------------------------------
// If the entity has MOVETYPE_FOLLOW and a valid aiment, then it attaches 
//  the entity to its aiment.
//-----------------------------------------------------------------------------
void C_BaseEntity::MoveAimEnt( void )
{
	// FIXME: This is sort of a hack. It causes the next call to
	// GetAbsOrigin, etc. to forcibly recompute
	if ( GetMoveParent())
	{
		m_iEFlags |= EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY | EFL_DIRTY_ABSANGVELOCITY;
	}
}


//-----------------------------------------------------------------------------
// These methods encapsulate MOVETYPE_FOLLOW, which became obsolete
//-----------------------------------------------------------------------------
void C_BaseEntity::FollowEntity( CBaseEntity *pBaseEntity )
{
	if (pBaseEntity)
	{
		SetParent( pBaseEntity );
		m_fEffects |= EF_BONEMERGE;
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetMoveType( MOVETYPE_NONE );
		SetLocalOrigin( vec3_origin );
		SetLocalAngles( vec3_angle );
	}
	else
	{
		StopFollowingEntity();
	}
}

void C_BaseEntity::StopFollowingEntity( )
{
	Assert( IsFollowingEntity() );

	SetParent( NULL );
	m_fEffects &= ~EF_BONEMERGE;
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
}

bool C_BaseEntity::IsFollowingEntity()
{
	return (m_fEffects & EF_BONEMERGE) && (GetMoveType() == MOVETYPE_NONE) && GetMoveParent();
}

C_BaseEntity *CBaseEntity::GetFollowedEntity()
{
	if (!IsFollowingEntity())
		return NULL;
	return GetMoveParent();
}


//-----------------------------------------------------------------------------
// Default implementation for GetTextureAnimationStartTime
//-----------------------------------------------------------------------------
float C_BaseEntity::GetTextureAnimationStartTime()
{
	return m_flSpawnTime;
}


//-----------------------------------------------------------------------------
// Default implementation, indicates that a texture animation has wrapped
//-----------------------------------------------------------------------------
void C_BaseEntity::TextureAnimationWrapped()
{
}


void C_BaseEntity::ClientThink()
{
}


void C_BaseEntity::UpdateClientSideAnimation()
{
}


void C_BaseEntity::Simulate()
{
	AddEntity();	// Legacy support. Once-per-frame stuff should go in Simulate().
}


// (static function)
void C_BaseEntity::InterpolateServerEntities()
{
	VPROF_BUDGET( "C_BaseEntity::InterpolateServerEntities", VPROF_BUDGETGROUP_INTERPOLATION );
	// Smoothly interplate position for server entities.
	int iHighest = ClientEntityList().GetHighestEntityIndex();
	for ( int i=0; i <= iHighest; i++ )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntity( i );
		if ( pEnt )
			pEnt->UpdatePosition();
	}
}


// (static function)
void C_BaseEntity::AddVisibleEntities()
{
	VPROF_BUDGET( "C_BaseEntity::AddVisibleEntities", VPROF_BUDGETGROUP_WORLD_RENDERING );
	// Smoothly interplate position for server entities.
	C_AllBaseEntityIterator iterator;
	C_BaseEntity *pEnt;
	while ( (pEnt = iterator.Next()) )
	{
		if ( !pEnt->IsDormant() && pEnt->ShouldDraw() )
			view->AddVisibleEntity( pEnt );
		else
			pEnt->RemoveFromLeafSystem();		
	}

	// Let non-dormant client created predictables get added, too
	int c = predictables->GetPredictableCount();
	for ( int i = 0 ; i < c ; i++ )
	{
		C_BaseEntity *pEnt = predictables->GetPredictable( i );
		if ( !pEnt )
			continue;

		if ( !pEnt->IsClientCreated() )
			continue;

		// Only draw until it's ack'd since that means a real entity has arrived
		if ( pEnt->m_PredictableID.GetAcknowledged() )
			continue;

		// Don't draw if dormant
		if ( pEnt->IsDormantPredictable() )
			continue;

		if ( !pEnt->IsDormant() && pEnt->ShouldDraw() )
			view->AddVisibleEntity( pEnt );
		else
			pEnt->RemoveFromLeafSystem();		
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void C_BaseEntity::OnPreDataChanged( DataUpdateType_t type )
{
	m_hOldMoveParent = m_hNetworkMoveParent;
}

void C_BaseEntity::OnDataChanged( DataUpdateType_t type )
{
	// See if it needs to allocate prediction stuff
	CheckInitPredictable( "OnDataChanged" );

	// Set up shadows; do it here so that objects can change shadowcasting state
	ShadowType_t shadowType = ShadowCastType();
	if (shadowType == SHADOWS_NONE)
	{
		if (m_ShadowHandle != CLIENTSHADOW_INVALID_HANDLE)
		{
			g_pClientShadowMgr->DestroyShadow(m_ShadowHandle);
			m_ShadowHandle = CLIENTSHADOW_INVALID_HANDLE;
		}
	}
	else
	{
		if (m_ShadowHandle == CLIENTSHADOW_INVALID_HANDLE)
		{
			int flags = 0;
			if (shadowType != SHADOWS_SIMPLE)
				flags |= SHADOW_FLAGS_USE_RENDER_TO_TEXTURE;
			if (shadowType == SHADOWS_RENDER_TO_TEXTURE_DYNAMIC)
				flags |= SHADOW_FLAGS_ANIMATING_SOURCE;
			m_ShadowHandle = g_pClientShadowMgr->CreateShadow(GetClientHandle(), flags);
		}
	}
}

ClientThinkHandle_t C_BaseEntity::GetThinkHandle()
{
	return m_hThink;
}


void C_BaseEntity::SetThinkHandle( ClientThinkHandle_t hThink )
{
	m_hThink = hThink;
}


//-----------------------------------------------------------------------------
// Purpose: This routine modulates renderamt according to m_nRenderFX's value
//  This is a client side effect and will not be in-sync on machines across a
//  network game.
// Input  : origin - 
//			alpha - 
// Output : int
//-----------------------------------------------------------------------------
void C_BaseEntity::ComputeFxBlend( void )
{
	int blend=0;
	float offset;

	offset = ((int)index) * 363.0;// Use ent index to de-sync these fx

	switch( m_nRenderFX ) 
	{
	case kRenderFxPulseSlowWide:
		blend = m_clrRender->a + 0x40 * sin( gpGlobals->curtime * 2 + offset );	
		break;
		
	case kRenderFxPulseFastWide:
		blend = m_clrRender->a + 0x40 * sin( gpGlobals->curtime * 8 + offset );
		break;
	
	case kRenderFxPulseFastWider:
		blend = ( 0xff * fabs(sin( gpGlobals->curtime * 12 + offset ) ) );
		break;

	case kRenderFxPulseSlow:
		blend = m_clrRender->a + 0x10 * sin( gpGlobals->curtime * 2 + offset );
		break;
		
	case kRenderFxPulseFast:
		blend = m_clrRender->a + 0x10 * sin( gpGlobals->curtime * 8 + offset );
		break;
		
	// JAY: HACK for now -- not time based
	case kRenderFxFadeSlow:			
		if ( m_clrRender->a > 0 ) 
		{
			SetRenderColorA( m_clrRender->a - 1 );
		}
		else
		{
			SetRenderColorA( 0 );
		}
		blend = m_clrRender->a;
		break;
		
	case kRenderFxFadeFast:
		if ( m_clrRender->a > 3 ) 
		{
			SetRenderColorA( m_clrRender->a - 4 );
		}
		else
		{
			SetRenderColorA( 0 );
		}
		blend = m_clrRender->a;
		break;
		
	case kRenderFxSolidSlow:
		if ( m_clrRender->a < 255 )
		{
			SetRenderColorA( m_clrRender->a + 1 );
		}
		else
		{
			SetRenderColorA( 255 );
		}
		blend = m_clrRender->a;
		break;
		
	case kRenderFxSolidFast:
		if ( m_clrRender->a < 252 )
		{
			SetRenderColorA( m_clrRender->a + 4 );
		}
		else
		{
			SetRenderColorA( 255 );
		}
		blend = m_clrRender->a;
		break;
		
	case kRenderFxStrobeSlow:
		blend = 20 * sin( gpGlobals->curtime * 4 + offset );
		if ( blend < 0 )
		{
			blend = 0;
		}
		else
		{
			blend = m_clrRender->a;
		}
		break;
		
	case kRenderFxStrobeFast:
		blend = 20 * sin( gpGlobals->curtime * 16 + offset );
		if ( blend < 0 )
		{
			blend = 0;
		}
		else
		{
			blend = m_clrRender->a;
		}
		break;
		
	case kRenderFxStrobeFaster:
		blend = 20 * sin( gpGlobals->curtime * 36 + offset );
		if ( blend < 0 )
		{
			blend = 0;
		}
		else
		{
			blend = m_clrRender->a;
		}
		break;
		
	case kRenderFxFlickerSlow:
		blend = 20 * (sin( gpGlobals->curtime * 2 ) + sin( gpGlobals->curtime * 17 + offset ));
		if ( blend < 0 )
		{
			blend = 0;
		}
		else
		{
			blend = m_clrRender->a;
		}
		break;
		
	case kRenderFxFlickerFast:
		blend = 20 * (sin( gpGlobals->curtime * 16 ) + sin( gpGlobals->curtime * 23 + offset ));
		if ( blend < 0 )
		{
			blend = 0;
		}
		else
		{
			blend = m_clrRender->a;
		}
		break;
		
	case kRenderFxHologram:
	case kRenderFxDistort:
		{
			Vector	tmp;
			float	dist;
			
			VectorCopy( GetAbsOrigin(), tmp );
			VectorSubtract( tmp, CurrentViewOrigin(), tmp );
			dist = DotProduct( tmp, CurrentViewForward() );
			
			// Turn off distance fade
			if ( m_nRenderFX == kRenderFxDistort )
			{
				dist = 1;
			}
			if ( dist <= 0 )
			{
				blend = 0;
			}
			else 
			{
				SetRenderColorA( 180 );
				if ( dist <= 100 )
					blend = m_clrRender->a;
				else
					blend = (int) ((1.0 - (dist - 100) * (1.0 / 400.0)) * m_clrRender->a);
				blend += random->RandomInt(-32,31);
			}
		}
		break;
	
	case kRenderFxNone:
	case kRenderFxClampMinScale:
	default:
		if (m_nRenderMode == kRenderNormal)
			blend = 255;
		else
			blend = m_clrRender->a;
		break;	
		
	}

	if ( blend > 255 )
	{
		blend = 255;
	}
	else if ( blend < 0 )
	{
		blend = 0;
	}

	m_nRenderFXBlend = blend;

	// Tell our shadow
	if ( m_ShadowHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->SetFalloffBias( m_ShadowHandle, (255 - m_nRenderFXBlend) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_BaseEntity::GetFxBlend( void )
{
	return m_nRenderFXBlend;
}

//-----------------------------------------------------------------------------
// Determine the color modulation amount
//-----------------------------------------------------------------------------

void C_BaseEntity::GetColorModulation( float* color )
{
	color[0] = m_clrRender->r / 255.0f;
	color[1] = m_clrRender->g / 255.0f;
	color[2] = m_clrRender->b / 255.0f;
}


void C_BaseEntity::SetupEntityRenderHandle( RenderGroup_t group )
{
	Assert( m_hRender == INVALID_CLIENT_RENDER_HANDLE );
	m_hRender = ClientLeafSystem()->AddRenderable( this, group );
}


//-----------------------------------------------------------------------------
// Returns true if we should add this to the collision list
//-----------------------------------------------------------------------------
CollideType_t C_BaseEntity::ShouldCollide( )
{
	if ( !m_nModelIndex || !model )
		return ENTITY_SHOULD_NOT_COLLIDE;

	if ( !IsSolid( ) )
		return ENTITY_SHOULD_NOT_COLLIDE;

	// If the model is a bsp or studio (i.e. it can collide with the player
	if ( ( modelinfo->GetModelType( model ) != mod_brush ) && ( modelinfo->GetModelType( model ) != mod_studio ) )
		return ENTITY_SHOULD_NOT_COLLIDE;

	// Don't get stuck on point sized entities ( world doesn't count )
	if ( m_nModelIndex != 1 )
	{
		if ( IsPointSized() )
			return ENTITY_SHOULD_NOT_COLLIDE;
	}

	return ENTITY_SHOULD_COLLIDE;
}


//-----------------------------------------------------------------------------
// Is this a brush model?
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsBrushModel() const
{
	int modelType = modelinfo->GetModelType( model );
	return (modelType == mod_brush);
}


//-----------------------------------------------------------------------------
// This method works when we've got a studio model
//-----------------------------------------------------------------------------
void C_BaseEntity::AddStudioDecal( const Ray_t& ray, int hitbox, int decalIndex, 
								  bool doTrace, trace_t& tr, int maxLODToDecal )
{
	if (doTrace)
	{
		enginetrace->ClipRayToEntity( ray, MASK_SHOT, this, &tr );

		// Trace the ray against the entity
		if (tr.fraction == 1.0f)
			return;

		// Set the trace index appropriately...
		tr.m_pEnt = this;
	}

	// Found the point, now lets apply the decals
	CreateModelInstance();

	// FIXME: Pass in decal up?
	// FIXME: What to do about the body parameter?
	Vector up(0, 0, 1);

	if (doTrace && (GetSolid() == SOLID_VPHYSICS) && !tr.startsolid && !tr.allsolid)
	{
		// Choose a more accurate normal direction
		// Also, since we have more accurate info, we can avoid pokethru
		Vector temp;
		VectorSubtract( tr.endpos, tr.plane.normal, temp );
		Ray_t betterRay;
		betterRay.Init( tr.endpos, temp );
		modelrender->AddDecal( m_ModelInstance, betterRay, up, decalIndex, 0, true, maxLODToDecal );
	}
	else
	{
		modelrender->AddDecal( m_ModelInstance, ray, up, decalIndex, 0, false, maxLODToDecal );
	}
}


//-----------------------------------------------------------------------------
// This method works when we've got a brush model
//-----------------------------------------------------------------------------
void C_BaseEntity::AddBrushModelDecal( const Ray_t& ray, const Vector& decalCenter, 
									  int decalIndex, bool doTrace, trace_t& tr )
{
	if ( doTrace )
	{
		enginetrace->ClipRayToEntity( ray, MASK_SHOT, this, &tr );
		if ( tr.fraction == 1.0f )
			return;
	}

	effects->DecalShoot( decalIndex, index, 
		model, GetAbsOrigin(), GetAbsAngles(), decalCenter, 0, 0 );
}


//-----------------------------------------------------------------------------
// A method to apply a decal to an entity
//-----------------------------------------------------------------------------
void C_BaseEntity::AddDecal( const Vector& rayStart, const Vector& rayEnd,
		const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal )
{
	Ray_t ray;
	ray.Init( rayStart, rayEnd );

	// FIXME: Better bloat?
	// Bloat a little bit so we get the intersection
	ray.m_Delta *= 1.1f;

	int modelType = modelinfo->GetModelType( model );
	switch ( modelType )
	{
	case mod_studio:
		AddStudioDecal( ray, hitbox, decalIndex, doTrace, tr, maxLODToDecal );
		break;

	case mod_brush:
		AddBrushModelDecal( ray, decalCenter, decalIndex, doTrace, tr );
		break;

	default:
		// By default, no collision
		tr.fraction = 1.0f;
		break;
	}
}

//-----------------------------------------------------------------------------
// A method to remove all decals from an entity
//-----------------------------------------------------------------------------
void C_BaseEntity::RemoveAllDecals( void )
{
	// For now, we only handle removing decals from studiomodels
	if ( modelinfo->GetModelType( model ) == mod_studio )
	{
		CreateModelInstance();
		modelrender->RemoveAllDecals( m_ModelInstance );
	}
}


#include "tier0/memdbgoff.h"

//-----------------------------------------------------------------------------
// C_BaseEntity new/delete
// All fields in the object are all initialized to 0.
//-----------------------------------------------------------------------------
void *C_BaseEntity::operator new( size_t stAllocateBlock )
{
	Assert( stAllocateBlock != 0 );				
	void *pMem = malloc( stAllocateBlock );
	memset( pMem, 0, stAllocateBlock );
	return pMem;												
}

void *C_BaseEntity::operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )
{
	Assert( stAllocateBlock != 0 );				
	void *pMem = _malloc_dbg( stAllocateBlock, nBlockUse, pFileName, nLine );
	memset( pMem, 0, stAllocateBlock );
	return pMem;												
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMem - 
//-----------------------------------------------------------------------------
void C_BaseEntity::operator delete( void *pMem )
{
#ifdef _DEBUG
	// set the memory to a known value
	int size = _msize( pMem );
	Q_memset( pMem, 0xcd, size );
#endif

	// get the engine to free the memory
	free( pMem );
}

#include "tier0/memdbgon.h"


//========================================================================================
// TEAM HANDLING
//========================================================================================
C_Team *C_BaseEntity::GetTeam( void )
{
	return GetGlobalTeam( m_iTeamNum );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::GetTeamNumber( void )
{
	return m_iTeamNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_BaseEntity::GetRenderTeamNumber( void )
{
	return GetTeamNumber();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if these entities are both in at least one team together
//-----------------------------------------------------------------------------
bool C_BaseEntity::InSameTeam( C_BaseEntity *pEntity )
{
	if ( !pEntity )
		return false;

	return ( pEntity->GetTeam() == GetTeam() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the entity's on the same team as the local player
//-----------------------------------------------------------------------------
bool C_BaseEntity::InLocalTeam( void )
{
	return ( GetTeam() == GetLocalTeam() );
}


void C_BaseEntity::SetNextClientThink( float nextThinkTime )
{
	ClientThinkList()->SetNextClientThink( GetClientHandle(), nextThinkTime );
}

void C_BaseEntity::RemoveFromLeafSystem()
{
	// Detach from the leaf lists.
	if( m_hRender != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->RemoveRenderable( m_hRender );
		m_hRender = INVALID_CLIENT_RENDER_HANDLE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Mark as predicted entity
// Input  : bPredicted - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetPredictedEntity( bool bPredicted )
{
	m_bPredictedEntity = bPredicted;
}


//-----------------------------------------------------------------------------
// Purpose: Flags this entity as being inside or outside of this client's PVS
//			on the server.
//			NOTE: this is meaningless for client-side only entities.
// Input  : inside_pvs - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetDormant( bool bDormant )
{
	Assert( IsServerEntity() );
	m_bDormant = bDormant;

	// Kill our shadow if we became dormant.
	if (m_bDormant)
	{
		if (m_ShadowHandle != CLIENTSHADOW_INVALID_HANDLE)
		{
			g_pClientShadowMgr->DestroyShadow(m_ShadowHandle);
			m_ShadowHandle = CLIENTSHADOW_INVALID_HANDLE;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this entity is dormant. Client/server entities become
//			dormant when they leave the PVS on the server. Client side entities
//			can decide for themselves whether to become dormant.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsDormant( void )
{
	if ( IsServerEntity() )
	{
		return m_bDormant;
	}

	return false;
}

void C_BaseEntity::SetAbsMins( const Vector& mins )
{
	m_vecAbsMins = mins;
}

const Vector& C_BaseEntity::GetAbsMins( void ) const
{
	return m_vecAbsMins;
}

void C_BaseEntity::SetAbsMaxs( const Vector& maxs )
{
	m_vecAbsMaxs = maxs;
}

const Vector& C_BaseEntity::GetAbsMaxs( void ) const
{
	return m_vecAbsMaxs;
}

//-----------------------------------------------------------------------------
// These methods recompute local versions as well as set abs versions
//-----------------------------------------------------------------------------
void C_BaseEntity::SetAbsOrigin( const Vector& absOrigin )
{
	// This is necessary to get the other fields of m_rgflCoordinateFrame ok
	CalcAbsolutePosition();

	// All children are invalid, but we are not
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM );
	m_iEFlags &= ~EFL_DIRTY_ABSTRANSFORM;

	m_vecAbsOrigin = absOrigin;
	MatrixSetColumn( absOrigin, 3, m_rgflCoordinateFrame ); 

	C_BaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		m_vecOrigin = absOrigin;
		return;
	}

	// Moveparent case: transform the abs position into local space
	VectorITransform( absOrigin, pMoveParent->EntityToWorldTransform(), (Vector&)m_vecOrigin );
}

void C_BaseEntity::SetAbsAngles( const QAngle& absAngles )
{
	// This is necessary to get the other fields of m_rgflCoordinateFrame ok
	CalcAbsolutePosition();

	// All children are invalid, but we are not
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM, EFL_DIRTY_ABSVELOCITY | EFL_DIRTY_ABSANGVELOCITY );
	m_iEFlags &= ~EFL_DIRTY_ABSTRANSFORM;

	m_angAbsRotation = absAngles;
	AngleMatrix( absAngles, m_rgflCoordinateFrame );
	MatrixSetColumn( m_vecAbsOrigin, 3, m_rgflCoordinateFrame ); 

	C_BaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		m_angRotation = absAngles;
		return;
	}

	// Moveparent case: transform the abs transform into local space
	matrix3x4_t worldToParent, localMatrix;
	MatrixInvert( pMoveParent->EntityToWorldTransform(), worldToParent );
	ConcatTransforms( worldToParent, m_rgflCoordinateFrame, localMatrix );
	MatrixAngles( localMatrix, (QAngle &)m_angRotation );
}

void C_BaseEntity::SetAbsVelocity( const Vector &vecAbsVelocity )
{
	// The abs velocity won't be dirty since we're setting it here
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSVELOCITY );
	m_iEFlags &= ~EFL_DIRTY_ABSVELOCITY;

	m_vecAbsVelocity = vecAbsVelocity;

	C_BaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		m_vecVelocity = vecAbsVelocity;
		return;
	}

	// First subtract out the parent's abs velocity to get a relative
	// velocity measured in world space
	Vector relVelocity;
	VectorSubtract( vecAbsVelocity, pMoveParent->GetAbsVelocity(), relVelocity );

	// Transform velocity into parent space
	VectorIRotate( relVelocity, pMoveParent->EntityToWorldTransform(), m_vecVelocity );
}

/*
void C_BaseEntity::SetAbsAngularVelocity( const QAngle &vecAbsAngVelocity )
{
	// The abs velocity won't be dirty since we're setting it here
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSANGVELOCITY );
	m_iEFlags &= ~EFL_DIRTY_ABSANGVELOCITY;

	m_vecAbsAngVelocity = vecAbsAngVelocity;

	C_BaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		m_vecAngVelocity = vecAbsAngVelocity;
		return;
	}

	// First subtract out the parent's abs velocity to get a relative
	// angular velocity measured in world space
	QAngle relAngVelocity;
	relAngVelocity = vecAbsAngVelocity - pMoveParent->GetAbsAngularVelocity();

	matrix3x4_t entityToWorld;
	AngleMatrix( relAngVelocity, entityToWorld );

	// Moveparent case: transform the abs angular vel into local space
	matrix3x4_t worldToParent, localMatrix;
	MatrixInvert( pMoveParent->EntityToWorldTransform(), worldToParent );
	ConcatTransforms( worldToParent, entityToWorld, localMatrix );
	MatrixAngles( localMatrix, m_vecAngVelocity );
}
*/


// Prevent these for now until hierarchy is properly networked
const Vector& C_BaseEntity::GetLocalOrigin( void ) const
{
	return m_vecOrigin;
}

vec_t C_BaseEntity::GetLocalOriginDim( int iDim ) const
{
	return m_vecOrigin[iDim];
}

// Prevent these for now until hierarchy is properly networked
void C_BaseEntity::SetLocalOrigin( const Vector& origin )
{
	if (m_vecOrigin != origin)
	{
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM );
		m_vecOrigin = origin;
	}
}

void C_BaseEntity::SetLocalOriginDim( int iDim, vec_t flValue )
{
	if (m_vecOrigin[iDim] != flValue)
	{
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM );
		m_vecOrigin[iDim] = flValue;
	}
}


// Prevent these for now until hierarchy is properly networked
const QAngle& C_BaseEntity::GetLocalAngles( void ) const
{
	return m_angRotation;
}

vec_t C_BaseEntity::GetLocalAnglesDim( int iDim ) const
{
	return m_angRotation[iDim];
}

// Prevent these for now until hierarchy is properly networked
void C_BaseEntity::SetLocalAngles( const QAngle& angles )
{
	if (m_angRotation != angles)
	{
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM,
			EFL_DIRTY_ABSVELOCITY | EFL_DIRTY_ABSANGVELOCITY );
		m_angRotation = angles;
	}
}

void C_BaseEntity::SetLocalAnglesDim( int iDim, vec_t flValue )
{
	if (m_angRotation[iDim] != flValue)
	{
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSTRANSFORM );
		m_angRotation[iDim] = flValue;
	}
}

void C_BaseEntity::SetLocalVelocity( const Vector &vecVelocity )
{
	if (m_vecVelocity != vecVelocity)
	{
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSVELOCITY );
		m_vecVelocity = vecVelocity; 
	}
}

void C_BaseEntity::SetLocalAngularVelocity( const QAngle &vecAngVelocity )
{
	if (m_vecAngVelocity != vecAngVelocity)
	{
		InvalidatePhysicsRecursive( EFL_DIRTY_ABSANGVELOCITY );
		m_vecAngVelocity = vecAngVelocity;
	}
}


//-----------------------------------------------------------------------------
// Sets the local position from a transform
//-----------------------------------------------------------------------------
void C_BaseEntity::SetLocalTransform( const matrix3x4_t &localTransform )
{
	Vector vecLocalOrigin;
	QAngle vecLocalAngles;
	MatrixGetColumn( localTransform, 3, vecLocalOrigin );
	MatrixAngles( localTransform, vecLocalAngles );
	SetLocalOrigin( vecLocalOrigin );
	SetLocalAngles( vecLocalAngles );
}


//-----------------------------------------------------------------------------
// FIXME: REMOVE!!!
//-----------------------------------------------------------------------------
void C_BaseEntity::MoveToAimEnt( )
{
	Vector vecAimEntOrigin;
	QAngle vecAimEntAngles;
	GetAimEntOrigin( GetMoveParent(), &vecAimEntOrigin, &vecAimEntAngles );
	SetAbsOrigin( vecAimEntOrigin );
	SetAbsAngles( vecAimEntAngles );
}


matrix3x4_t& C_BaseEntity::GetParentToWorldTransform( matrix3x4_t &tempMatrix )
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
		if ( pMoveParent->GetAttachment( m_iParentAttachment, vOrigin, vAngles ) )
		{
			AngleMatrix( vAngles, vOrigin, tempMatrix );
			return tempMatrix;
		}
	}
	
	// If we fall through to here, then just use the move parent's abs origin and angles.
	return pMoveParent->EntityToWorldTransform();
}


//-----------------------------------------------------------------------------
// Purpose: Calculates the absolute position of an edict in the world
//			assumes the parent's absolute origin has already been calculated
//-----------------------------------------------------------------------------
void C_BaseEntity::CalcAbsolutePosition( )
{
	// There are periods of time where we're gonna have to live with the
	// fact that we're in an indeterminant state and abs queries (which
	// shouldn't be happening at all; I have assertions for those), will
	// just have to accept stale data.
	if (!s_bAbsRecomputationEnabled)
		return;

	// FIXME: Recompute absbox!!!
	if ((m_iEFlags & EFL_DIRTY_ABSTRANSFORM) == 0)
		return;

	m_iEFlags &= ~EFL_DIRTY_ABSTRANSFORM;

	// Construct the entity-to-world matrix
	// Start with making an entity-to-parent matrix
	AngleMatrix( GetLocalAngles(), m_rgflCoordinateFrame );
	MatrixSetColumn( GetLocalOrigin(), 3, m_rgflCoordinateFrame );

	if (!m_pMoveParent)
	{
		m_vecAbsOrigin = m_vecOrigin;
		m_angAbsRotation = m_angRotation;
		NormalizeAngles( m_angAbsRotation );
		return;
	}
	
	if ( m_fEffects & EF_BONEMERGE )
	{
		MoveToAimEnt();
		return;
	}

	// concatenate with our parent's transform
	matrix3x4_t tmpMatrix, scratchMatrix;
	ConcatTransforms( GetParentToWorldTransform( scratchMatrix ), m_rgflCoordinateFrame, tmpMatrix );
	MatrixCopy( tmpMatrix, m_rgflCoordinateFrame ); 

	// pull our absolute position out of the matrix
	MatrixGetColumn( m_rgflCoordinateFrame, 3, m_vecAbsOrigin );

	// if we have any angles, we have to extract our absolute angles from our matrix
	if ( m_angRotation == vec3_angle )
	{
		// just copy our parent's absolute angles
		VectorCopy( m_pMoveParent->GetAbsAngles(), m_angAbsRotation );
	}
	else
	{
		MatrixAngles( m_rgflCoordinateFrame, m_angAbsRotation );
	}
}

void C_BaseEntity::CalcAbsoluteVelocity()
{
	if ((m_iEFlags & EFL_DIRTY_ABSVELOCITY ) == 0)
		return;

	m_iEFlags &= ~EFL_DIRTY_ABSVELOCITY;

	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		m_vecAbsVelocity = m_vecVelocity;
		return;
	}

	VectorRotate( m_vecVelocity, pMoveParent->EntityToWorldTransform(), m_vecAbsVelocity );

	// Now add in the parent abs velocity
	m_vecAbsVelocity += pMoveParent->GetAbsVelocity();
}

/*
void C_BaseEntity::CalcAbsoluteAngularVelocity()
{
	if ((m_iEFlags & EFL_DIRTY_ABSANGVELOCITY ) == 0)
		return;

	m_iEFlags &= ~EFL_DIRTY_ABSANGVELOCITY;

	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		m_vecAbsAngVelocity = m_vecAngVelocity;
		return;
	}

	matrix3x4_t angVelToParent, angVelToWorld;
	AngleMatrix( m_vecAngVelocity, angVelToParent );
	ConcatTransforms( pMoveParent->EntityToWorldTransform(), angVelToParent, angVelToWorld );
	MatrixAngles( angVelToWorld, m_vecAbsAngVelocity );

	// Now add in the parent abs angular velocity
	m_vecAbsAngVelocity += pMoveParent->GetAbsAngularVelocity();
}
*/


//-----------------------------------------------------------------------------
// Computes the abs position of a point specified in local space
//-----------------------------------------------------------------------------
void C_BaseEntity::ComputeAbsPosition( const Vector &vecLocalPosition, Vector *pAbsPosition )
{
	C_BaseEntity *pMoveParent = GetMoveParent();
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
void C_BaseEntity::ComputeAbsDirection( const Vector &vecLocalDirection, Vector *pAbsDirection )
{
	C_BaseEntity *pMoveParent = GetMoveParent();
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
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::ShutdownPredictable( void )
{
	Assert( GetPredictable() );

	g_Predictables.RemoveFromPredictablesList( GetClientHandle() );
	DestroyIntermediateData();
	SetPredictable( false );
}

//-----------------------------------------------------------------------------
// Purpose: Turn entity into something the predicts locally
//-----------------------------------------------------------------------------
void C_BaseEntity::InitPredictable( void )
{
	Assert( !GetPredictable() );

	// Mark as predictable
	SetPredictable( true );
	// Allocate buffers into which we copy data
	AllocateIntermediateData();
	// Add to list of predictables
	g_Predictables.AddToPredictableList( GetClientHandle() );
	// Copy everything from "this" into the original_state_data
	//  object.  Don't care about client local stuff, so pull from slot 0 which

	//  should be empty anyway...
	PostNetworkDataReceived( 0 );

	// Copy original data into all prediction slots, so we don't get an error saying we "mispredicted" any
	//  values which are still at their initial values
	for ( int i = 0; i < MULTIPLAYER_BACKUP; i++ )
	{
		SaveData( "InitPredictable", i, PC_EVERYTHING );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetPredictable( bool state )
{
	m_bPredictable = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::GetPredictable( void ) const
{
	return m_bPredictable;
}

//-----------------------------------------------------------------------------
// Purpose: Transfer data for intermediate frame to current entity
// Input  : copyintermediate - 
//			last_predicted - 
//-----------------------------------------------------------------------------
void C_BaseEntity::PreEntityPacketReceived( int commands_acknowledged )					
{				
	// Don't need to copy intermediate data if server did ack any new commands
	bool copyintermediate = ( commands_acknowledged > 0 ) ? true : false;

	Assert( GetPredictable() );
	Assert( cl_predict.GetBool() );

	// First copy in any intermediate predicted data for non-networked fields
	if ( copyintermediate )
	{
		RestoreData( "PreEntityPacketReceived", commands_acknowledged - 1, PC_NON_NETWORKED_ONLY );
		RestoreData( "PreEntityPacketReceived", SLOT_ORIGINALDATA, PC_NETWORKED_ONLY );
	}
	else
	{
		RestoreData( "PreEntityPacketReceived(no commands ack)", SLOT_ORIGINALDATA, PC_EVERYTHING );
	}

	// At this point the entity has original network data restored as of the last time the 
	// networking was updated, and it has any intermediate predicted values properly copied over
	// Unpacked and OnDataChanged will fill in any changed, networked fields.

	// That networked data will be copied forward into the starting slot for the next prediction round
}	

//-----------------------------------------------------------------------------
// Purpose: Called every time PreEntityPacket received is called
//  copy any networked data into original_state
// Input  : errorcheck - 
//			last_predicted - 
//-----------------------------------------------------------------------------
void C_BaseEntity::PostEntityPacketReceived( void )
{
	Assert( GetPredictable() );
	Assert( cl_predict.GetBool() );

	// Always mark as changed
	AddDataChangeEvent( this, DATA_UPDATE_DATATABLE_CHANGED, &m_DataChangeEventRef );

	// Save networked fields into "original data" store
	SaveData( "PostEntityPacketReceived", SLOT_ORIGINALDATA, PC_NETWORKED_ONLY );
}

//-----------------------------------------------------------------------------
// Purpose: Called once per frame after all updating is done
// Input  : errorcheck - 
//			last_predicted - 
//-----------------------------------------------------------------------------
bool C_BaseEntity::PostNetworkDataReceived( int commands_acknowledged )
{
	bool haderrors = false;

	Assert( GetPredictable() );

	bool errorcheck = ( commands_acknowledged > 0 ) ? true : false;

	// Store network data into post networking pristine state slot (slot 64) 
	SaveData( "PostNetworkDataReceived", SLOT_ORIGINALDATA, PC_EVERYTHING );

	// Show any networked fields that are different
	bool showthis = cl_showerror.GetInt() >= 2;

	if ( cl_showerror.GetInt() < 0 )
	{
		if ( entindex() == -cl_showerror.GetInt() )
		{
			showthis = true;
		}
		else
		{
			showthis = false;
		}
	}

	if ( errorcheck )
	{
		void *predicted_state_data = GetPredictedFrame( commands_acknowledged - 1 );	
		Assert( predicted_state_data );												
		const void *original_state_data = GetOriginalNetworkDataObject();
		Assert( original_state_data );

		bool counterrors = true;
		bool reporterrors = showthis;
		bool copydata	= false;

		CPredictionCopy errorCheckHelper( PC_NETWORKED_ONLY, 
			predicted_state_data, PC_DATA_PACKED, 
			original_state_data, PC_DATA_PACKED, 
			counterrors, reporterrors, copydata );
		// Suppress debugging output
		int ecount = errorCheckHelper.TransferData( "", -1, GetPredDescMap() );
		if ( ecount > 0 )
		{
			haderrors = true;
		//	Msg( "%i errors %i on entity %i %s\n", gpGlobals->tickcount, ecount, index, IsClientCreated() ? "true" : "false" );
		}
	}
	return haderrors;
}

BEGIN_PREDICTION_DATA_NO_BASE( C_BaseEntity )

	// These have a special proxy to handle send/receive
	DEFINE_PRED_TYPEDESCRIPTION( C_BaseEntity, m_Collision, CCollisionProperty ),

	DEFINE_PRED_FIELD( C_BaseEntity, m_MoveType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BaseEntity, m_MoveCollide, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD( C_BaseEntity, m_vecAbsVelocity, FIELD_VECTOR ),
	DEFINE_PRED_FIELD_TOL( C_BaseEntity, m_vecVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.5f ),
//	DEFINE_PRED_FIELD( C_BaseEntity, m_fEffects, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BaseEntity, m_nRenderMode, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BaseEntity, m_nRenderFX, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD( C_BaseEntity, m_flAnimTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD( C_BaseEntity, m_flSimulationTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BaseEntity, m_fFlags, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( C_BaseEntity, m_vecViewOffset, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.25f ),
	DEFINE_PRED_FIELD( C_BaseEntity, m_nModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_PRED_FIELD( C_BaseEntity, m_flFriction, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BaseEntity, m_iTeamNum, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BaseEntity, m_hOwnerEntity, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

//	DEFINE_FIELD( C_BaseEntity, m_nSimulationTick, FIELD_INTEGER ),

	DEFINE_PRED_FIELD( C_BaseEntity, m_hNetworkMoveParent, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD( C_BaseEntity, m_pMoveParent, FIELD_EHANDLE ),
//	DEFINE_PRED_FIELD( C_BaseEntity, m_pMoveChild, FIELD_EHANDLE ),
//	DEFINE_PRED_FIELD( C_BaseEntity, m_pMovePeer, FIELD_EHANDLE ),
//	DEFINE_PRED_FIELD( C_BaseEntity, m_pMovePrevPeer, FIELD_EHANDLE ),

	DEFINE_PRED_FIELD_TOL( C_BaseEntity, m_vecNetworkOrigin, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.125f ),
	DEFINE_PRED_FIELD( C_BaseEntity, m_angNetworkAngles, FIELD_VECTOR, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_FIELD( C_BaseEntity, m_vecAbsOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( C_BaseEntity, m_angAbsRotation, FIELD_VECTOR ),
	DEFINE_FIELD( C_BaseEntity, m_vecOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( C_BaseEntity, m_angRotation, FIELD_VECTOR ),

//	DEFINE_FIELD( C_BaseEntity, m_hGroundEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( C_BaseEntity, m_nWaterLevel, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseEntity, m_nWaterType, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseEntity, m_vecAngVelocity, FIELD_VECTOR ),
//	DEFINE_FIELD( C_BaseEntity, m_vecAbsAngVelocity, FIELD_VECTOR ),


//	DEFINE_FIELD( C_BaseEntity, model, FIELD_INTEGER ), // writing pointer literally
//	DEFINE_FIELD( C_BaseEntity, index, FIELD_INTEGER ),
//	DEFINE_FIELD( C_BaseEntity, m_ClientHandle, FIELD_SHORT ),
//	DEFINE_FIELD( C_BaseEntity, m_Partition, FIELD_SHORT ),
//	DEFINE_FIELD( C_BaseEntity, m_hRender, FIELD_SHORT ),
	DEFINE_FIELD( C_BaseEntity, m_bPredictedEntity, FIELD_BOOLEAN ),
	DEFINE_FIELD( C_BaseEntity, m_bDormant, FIELD_BOOLEAN ),
//	DEFINE_FIELD( C_BaseEntity, current_position, FIELD_INTEGER ),
//	DEFINE_FIELD( C_BaseEntity, m_flLastMessageTime, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseEntity, m_vecBaseVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( C_BaseEntity, m_iEFlags, FIELD_INTEGER ),
	DEFINE_FIELD( C_BaseEntity, m_flGravity, FIELD_FLOAT ),
//	DEFINE_FIELD( C_BaseEntity, m_ModelInstance, FIELD_SHORT ),
	DEFINE_FIELD( C_BaseEntity, m_flProxyRandomValue, FIELD_FLOAT ),
	DEFINE_FIELD( C_BaseEntity, m_vecAbsMins, FIELD_VECTOR ),
	DEFINE_FIELD( C_BaseEntity, m_vecAbsMaxs, FIELD_VECTOR ),

//	DEFINE_FIELD( C_BaseEntity, m_PredictableID, FIELD_INTEGER ),
//	DEFINE_FIELD( C_BaseEntity, m_pPredictionContext, FIELD_POINTER ),

	// Stuff specific to rendering and therefore not to be copied back and forth
	// DEFINE_PRED_FIELD( C_BaseEntity, m_clrRender, color32, FTYPEDESC_INSENDTABLE  ),
	// DEFINE_FIELD( C_BaseEntity, m_bReadyToDraw, FIELD_BOOLEAN ),
	// DEFINE_FIELD( C_BaseEntity, anim, CLatchedAnim ),
	// DEFINE_FIELD( C_BaseEntity, mouth, CMouthInfo ),
	// DEFINE_FIELD( C_BaseEntity, GetAbsOrigin(), FIELD_VECTOR ),
	// DEFINE_FIELD( C_BaseEntity, GetAbsAngles(), FIELD_VECTOR ),
	// DEFINE_FIELD( C_BaseEntity, m_nNumAttachments, FIELD_SHORT ),
	// DEFINE_FIELD( C_BaseEntity, m_pAttachmentAngles, FIELD_VECTOR ),
	// DEFINE_FIELD( C_BaseEntity, m_pAttachmentOrigin, FIELD_VECTOR ),
	// DEFINE_FIELD( C_BaseEntity, m_listentry, CSerialEntity ),
	// DEFINE_FIELD( C_BaseEntity, m_ShadowHandle, ClientShadowHandle_t ),
	// DEFINE_FIELD( C_BaseEntity, m_hThink, ClientThinkHandle_t ),
	// Definitely private and not copied around
	// DEFINE_FIELD( C_BaseEntity, m_bPredictable, FIELD_BOOLEAN ),
	// DEFINE_FIELD( C_BaseEntity, m_CollisionGroup, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseEntity, m_DataChangeEventRef, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseEntity, m_bPredictionEligible, FIELD_BOOLEAN ),

END_PREDICTION_DATA()

// Stuff implemented for weapon prediction code
void C_BaseEntity::SetSize( const Vector &vecMin, const Vector &vecMax )
{
	SetCollisionBounds( vecMin, vecMax );

	Vector size;
	VectorSubtract (vecMax, vecMin, size);
	SetSize( size );
}

//-----------------------------------------------------------------------------
// Purpose: Just look up index
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::PrecacheModel( const char *name )
{
	return modelinfo->GetModelIndex( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
int	C_BaseEntity::PrecacheSound( const char *name )
{
	return enginesound->PrecacheSound( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *obj - 
//-----------------------------------------------------------------------------
void C_BaseEntity::Remove( )
{
	// Nothing for now, if it's a predicted entity, could flag as "delete" or dormant
	if ( GetPredictable() || IsClientCreated() )
	{
		// Make it solid
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetMoveType( MOVETYPE_NONE );

		AddEFlags( EFL_KILLME );	// Make sure to ignore further calls into here or UTIL_Remove.
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::GetPredictionEligible( void ) const
{
	return m_bPredictionEligible;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetCollisionGroup( int collisionGroup )
{
	if ( m_CollisionGroup != collisionGroup )
	{
		m_CollisionGroup = collisionGroup;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::GetCollisionGroup() const
{
	return m_CollisionGroup;
}


C_BaseEntity* C_BaseEntity::Instance( CBaseHandle hEnt )
{
	return ClientEntityList().GetBaseEntityFromHandle( hEnt );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iEnt - 
// Output : C_BaseEntity
//-----------------------------------------------------------------------------
C_BaseEntity *C_BaseEntity::Instance( int iEnt )
{
	return ClientEntityList().GetBaseEntity( iEnt );
}

#pragma warning( push )
#include <typeinfo.h>
#pragma warning( pop )

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *C_BaseEntity::GetClassname( void )
{
	static char outstr[ 256 ];
	outstr[ 0 ] = 0;
	bool gotname = false;
	if ( GetPredDescMap() )
	{
		const char *mapname =  GetClassMap().Lookup( GetPredDescMap()->dataClassName );
		if ( mapname && mapname[ 0 ] ) 
		{
			int Q_snprintf( char *pDest, int maxLen, const char *pFormat, ... );

			Q_snprintf( outstr, sizeof( outstr ), "%s", mapname );
			gotname = true;
		}
	}
	if ( !gotname )
	{
		Q_strncpy( outstr, typeid( *this ).name(), sizeof( outstr ) );
	}

	return outstr;
}

//-----------------------------------------------------------------------------
// Purpose: Creates an entity by string name, but does not spawn it
// Input  : *className - 
// Output : C_BaseEntity
//-----------------------------------------------------------------------------
C_BaseEntity *CreateEntityByName( const char *className )
{
	C_BaseEntity *ent = GetClassMap().CreateEntity( className );
	if ( ent )
	{
		return ent;
	}

	Warning( "Can't find factory for entity: %s\n", className );
	return NULL;
}

CON_COMMAND( cl_sizeof, "Determines the size of the specified client class." )
{
	if ( engine->Cmd_Argc() != 2 )
	{
		Msg( "cl_sizeof <gameclassname>\n" );
		return;
	}

	int size = GetClassMap().GetClassSize( engine->Cmd_Argv(1 ) );

	Msg( "%s is %i bytes\n", engine->Cmd_Argv(1), size );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsClientCreated( void ) const
{
	if ( m_pPredictionContext != NULL )
	{
		// For now can't be both
		Assert( !GetPredictable() );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classname - 
//			*module - 
//			line - 
// Output : C_BaseEntity
//-----------------------------------------------------------------------------
C_BaseEntity *C_BaseEntity::CreatePredictedEntityByName( const char *classname, const char *module, int line, bool persist /*= false */ )
{
	C_BasePlayer *player = C_BaseEntity::GetPredictionPlayer();

	Assert( player );
	Assert( player->m_pCurrentCommand );
	Assert( prediction->InPrediction() );

	C_BaseEntity *ent = NULL;

	// What's my birthday (should match server)
	int command_number	= player->m_pCurrentCommand->command_number;
	// Who's my daddy?
	int player_index	= player->entindex() - 1;

	// Create id/context
	CPredictableId testId;
	testId.Init( player_index, command_number, classname, module, line );

	// If repredicting, should be able to find the entity in the previously created list
	if ( !prediction->IsFirstTimePredicted() )
	{
		// Only find previous instance if entity was created with persist set
		if ( persist )
		{
			ent = FindPreviouslyCreatedEntity( testId );
			if ( ent )
			{
				return ent;
			}
		}

		return NULL;
	}

	// Try to create it
	ent = CreateEntityByName( classname );
	if ( !ent )
	{
		return NULL;
	}

	// It's predictable
	ent->SetPredictionEligible( true );

	// Set up "shared" id number
	ent->m_PredictableID.SetRaw( testId.GetRaw() );

	// Get a context (mostly for debugging purposes)
	PredictionContext *context			= new PredictionContext;
	context->m_bActive					= true;
	context->m_nCreationCommandNumber	= command_number;
	context->m_nCreationLineNumber		= line;
	context->m_pszCreationModule		= module;

	// Attach to entity
	ent->m_pPredictionContext = context;

	// Add to client entity list
	ClientEntityList().AddNonNetworkableEntity( ent );

	//  and predictables
	g_Predictables.AddToPredictableList( ent->GetClientHandle() );

	// Duhhhh..., but might as well be safe
	Assert( !ent->GetPredictable() );
	Assert( ent->IsClientCreated() );

	/*
	// Add the client entity to the renderable "leaf system." (Renderable)
	RenderGroup_t renderGroup = ent->GetRenderGroup();
	bool bTwoPass = ent->IsTwoPass();
	ent->SetupEntityRenderHandle( renderGroup );
	*/

	// Add the client entity to the spatial partition. (Collidable)
	Assert( ent->m_Partition == PARTITION_INVALID_HANDLE );
	ent->m_Partition = partition->CreateHandle( ent->GetIClientUnknown() );

	// CLIENT ONLY FOR NOW!!!
	ent->index = -1;

	if ( AddDataChangeEvent( ent, DATA_UPDATE_CREATED, &ent->m_DataChangeEventRef ) )
	{
		ent->OnPreDataChanged( DATA_UPDATE_CREATED );
	}
	
	return ent;
}

//-----------------------------------------------------------------------------
// Purpose: Called each packet that the entity is created on and finally gets called after the next packet
//  that doesn't have a create message for the "parent" entity so that the predicted version
//  can be removed.  Return true to delete entity right away.
//-----------------------------------------------------------------------------
bool C_BaseEntity::OnPredictedEntityRemove( bool isbeingremoved, C_BaseEntity *predicted )
{
	// Nothing right now, but in theory you could look at the error in origins and set
	//  up something to smooth out the error
	PredictionContext *ctx = predicted->m_pPredictionContext;
	Assert( ctx );
	if ( ctx )
	{
		// Create backlink to actual entity
		ctx->m_hServerEntity = this;

		/*
		Msg( "OnPredictedEntity%s:  %s created %s(%i) instance(%i)\n",
			isbeingremoved ? "Remove" : "Acknowledge",
			predicted->GetClassname(),
			ctx->m_pszCreationModule,
			ctx->m_nCreationLineNumber,
			predicted->m_PredictableID.GetInstanceNumber() );
		*/
	}

	// If it comes through with an ID, it should be eligible
	SetPredictionEligible( true );

	// Start predicting simulation forward from here
	CheckInitPredictable( "OnPredictedEntityRemove" );

	// Always mark it dormant since we are the "real" entity now
	predicted->SetDormantPredictable( true );

	m_iEFlags = predicted->m_iEFlags;
	m_iEFlags |= ( EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY | EFL_DIRTY_ABSANGVELOCITY );

	// By default, signal that it should be deleted right away
	// If a derived class implements this method, it might chain to here but return
	// false if it wants to keep the dormant predictable around until the chain of
	//  DATA_UPDATE_CREATED messages passes
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetOwnerEntity( C_BaseEntity *pOwner )
{
	m_hOwnerEntity = pOwner;
}

//-----------------------------------------------------------------------------
// Purpose: Put the entity in the specified team
//-----------------------------------------------------------------------------
void C_BaseEntity::ChangeTeam( int iTeamNum )
{
	m_iTeamNum = iTeamNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : name - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetModelName( string_t name )
{
	m_ModelName = name;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : string_t
//-----------------------------------------------------------------------------
string_t C_BaseEntity::GetModelName( void ) const
{
	return m_ModelName;
}

//-----------------------------------------------------------------------------
// Purpose: Nothing yet, could eventually supercede Term()
//-----------------------------------------------------------------------------
void C_BaseEntity::UpdateOnRemove( void )
{
	Assert( !GetMoveParent() );
	UnlinkFromHierarchy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : canpredict - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetPredictionEligible( bool canpredict )
{
	m_bPredictionEligible = canpredict;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a value that scales all damage done by this entity.
//-----------------------------------------------------------------------------
float C_BaseEntity::GetAttackDamageScale( void )
{
	float flScale = 1;
// Not hooked up to prediction yet
#if 0
	FOR_EACH_LL( m_DamageModifiers, i )
	{
		if ( !m_DamageModifiers[i]->IsDamageDoneToMe() )
		{
			flScale *= m_DamageModifiers[i]->GetModifier();
		}
	}
#endif
	return flScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsDormantPredictable( void ) const
{
	return m_bDormantPredictable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dormant - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetDormantPredictable( bool dormant )
{
	Assert( IsClientCreated() );

	m_bDormantPredictable = true;
	m_nIncomingPacketEntityBecameDormant = prediction->GetIncomingPacketNumber();

// Do we need to do the following kinds of things?
#if 0
	// Remove from collisions
	SetSolid( SOLID_NOT );
	// Don't render
	m_fEffects |= EF_NODRAW;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Used to determine when a dorman client predictable can be safely deleted
//  Note that it can be deleted earlier than this by OnPredictedEntityRemove returning true
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::BecameDormantThisPacket( void ) const
{
	Assert( IsDormantPredictable() );

	if ( m_nIncomingPacketEntityBecameDormant != prediction->GetIncomingPacketNumber() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsIntermediateDataAllocated( void ) const
{
	return m_pOriginalData != NULL ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::AllocateIntermediateData( void )
{											
	if ( m_pOriginalData )
		return;
	size_t allocsize = GetIntermediateDataSize();
	Assert( allocsize > 0 );

	m_pOriginalData = new unsigned char[ allocsize ];
	Q_memset( m_pOriginalData, 0, allocsize );
	for ( int i = 0; i < MULTIPLAYER_BACKUP; i++ )
	{
		m_pIntermediateData[ i ] = new unsigned char[ allocsize ];
		Q_memset( m_pIntermediateData[ i ], 0, allocsize );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::DestroyIntermediateData( void )
{
	if ( !m_pOriginalData )
		return;
	for ( int i = 0; i < MULTIPLAYER_BACKUP; i++ )
	{
		delete[] m_pIntermediateData[ i ];
		m_pIntermediateData[ i ] = NULL;
	}
	delete[] m_pOriginalData;
	m_pOriginalData = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : slots_to_remove - 
//			number_of_commands_run - 
//-----------------------------------------------------------------------------
void C_BaseEntity::ShiftIntermediateDataForward( int slots_to_remove, int number_of_commands_run )
{
	Assert( m_pIntermediateData );
	if ( !m_pIntermediateData )
		return;

	// Nothing to do
	if ( slots_to_remove <= 1 )
		return;

	Assert( number_of_commands_run >= slots_to_remove );

	// Just moving pointers, yeah
	CUtlVector< unsigned char * > saved;

	// Remember first slots
	for ( int i = 0; i < slots_to_remove; i++ )
	{
		saved.AddToTail( m_pIntermediateData[ i ] );
	}

	// Move rest of slots forward up to last slot
	for ( ; i < number_of_commands_run; i++ )
	{
		m_pIntermediateData[ i - slots_to_remove ] = m_pIntermediateData[ i ];
	}

	// Put remembered slots onto end
	for ( i = 0; i < slots_to_remove; i++ )
	{
		int slot = number_of_commands_run - slots_to_remove + i;

		m_pIntermediateData[ slot ] = saved[ i ];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : framenumber - 
//-----------------------------------------------------------------------------
void *C_BaseEntity::GetPredictedFrame( int framenumber )
{
	Assert( framenumber >= 0 );

	if ( !m_pOriginalData )
	{
		Assert( 0 );
		return NULL;
	}
	return (void *)m_pIntermediateData[ framenumber & (MULTIPLAYER_BACKUP-1) ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void *C_BaseEntity::GetOriginalNetworkDataObject( void )
{
	if ( !m_pOriginalData )
	{
		Assert( 0 );
		return NULL;
	}
	return (void *)m_pOriginalData;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::ComputePackedOffsets( void )
{
	datamap_t *map = GetPredDescMap();
	if ( !map )
		return;

	if ( map->packed_offsets_computed )
		return;

	ComputePackedSize_R( map );

	Assert( map->packed_offsets_computed );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::GetIntermediateDataSize( void )
{
	ComputePackedOffsets();

	const datamap_t *map = GetPredDescMap();

	Assert( map->packed_offsets_computed );

	int size = map->packed_size;

	Assert( size > 0 );	

	// At least 4 bytes to avoid some really bad stuff
	return max( size, 4 );
}	

static int g_FieldSizes[FIELD_TYPECOUNT] = 
{
	0,					// FIELD_VOID
	sizeof(float),		// FIELD_FLOAT
	sizeof(int),		// FIELD_STRING
	sizeof(Vector),		// FIELD_VECTOR
	sizeof(Quaternion),	// FIELD_QUATERNION
	sizeof(int),		// FIELD_INTEGER
	sizeof(char),		// FIELD_BOOLEAN
	sizeof(short),		// FIELD_SHORT
	sizeof(char),		// FIELD_CHARACTER
	sizeof(color32),	// FIELD_COLOR32
	sizeof(int),		// FIELD_EMBEDDED	(handled specially)
	sizeof(int),		// FIELD_CUSTOM		(handled specially)
	
	//---------------------------------

	sizeof(int),		// FIELD_CLASSPTR
	sizeof(EHANDLE),	// FIELD_EHANDLE
	sizeof(int),		// FIELD_EDICT

	sizeof(Vector),		// FIELD_POSITION_VECTOR
	sizeof(float),		// FIELD_TIME
	sizeof(int),		// FIELD_TICK
	sizeof(int),		// FIELD_MODELNAME
	sizeof(int),		// FIELD_SOUNDNAME

	sizeof(int),		// FIELD_INPUT		(uses custom type)
	sizeof(int *),		// FIELD_FUNCTION
	sizeof(VMatrix),	// FIELD_VMATRIX
	sizeof(VMatrix),	// FIELD_VMATRIX_WORLDSPACE
	sizeof(matrix3x4_t),// FIELD_MATRIX3X4_WORLDSPACE	// NOTE: Use array(FIELD_FLOAT, 12) for matrix3x4_t NOT in worldspace
	sizeof(interval_t), // FIELD_INTERVAL
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *map - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::ComputePackedSize_R( datamap_t *map )
{
	if ( !map )
	{
		Assert( 0 );
		return 0;
	}

	// Already computed
	if ( map->packed_offsets_computed )
	{
		return map->packed_size;
	}

	int current_position = 0;

	// Recurse to base classes first...
	if ( map->baseMap )
	{
		current_position += ComputePackedSize_R( map->baseMap );
	}

	int c = map->dataNumFields;
	int i;
	typedescription_t *field;

	for ( i = 0; i < c; i++ )
	{
		field = &map->dataDesc[ i ];

		// Always descend into embedded types...
		if ( field->fieldType != FIELD_EMBEDDED )
		{
			// Skip all private fields
			if ( field->flags & FTYPEDESC_PRIVATE )
				continue;
		}

		switch ( field->fieldType )
		{
		default:
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_TIME:
		case FIELD_TICK:
		case FIELD_CUSTOM:
		case FIELD_CLASSPTR:
		case FIELD_EDICT:
		case FIELD_POSITION_VECTOR:
		case FIELD_FUNCTION:
			Assert( 0 );
			break;

		case FIELD_EMBEDDED:
			{
				Assert( field->td != NULL );

				int embeddedsize = ComputePackedSize_R( field->td );

				field->fieldOffset[ TD_OFFSET_PACKED ] = current_position;

				current_position += embeddedsize;
			}
			break;
		case FIELD_FLOAT:
		case FIELD_STRING:
		case FIELD_VECTOR:
		case FIELD_QUATERNION:
		case FIELD_COLOR32:
		case FIELD_BOOLEAN:
		case FIELD_INTEGER:
		case FIELD_SHORT:
		case FIELD_CHARACTER:
		case FIELD_EHANDLE:
			{
				field->fieldOffset[ TD_OFFSET_PACKED ] = current_position;
				Assert( field->fieldSize >= 1 );
				current_position += g_FieldSizes[ field->fieldType ] * field->fieldSize;
			}
			break;
		case FIELD_VOID:
			{
				// Special case, just skip it
			}
			break;
		}
	}

	map->packed_size = current_position;
	map->packed_offsets_computed = true;

	return current_position;
}

// Convenient way to delay removing oneself
void C_BaseEntity::SUB_Remove( void )
{
	if (m_iHealth > 0)
	{
		// this situation can screw up NPCs who can't tell their entity pointers are invalid.
		m_iHealth = 0;
		DevWarning( 2, "SUB_Remove called on entity with health > 0\n");
	}

	Remove( );
}

//-----------------------------------------------------------------------------
// Purpose: Debug command to wipe the decals off an entity
//-----------------------------------------------------------------------------
static void RemoveDecals_f( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Get the entity under my crosshair
	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );
	UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE,	MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pHit = tr.m_pEnt;
		pHit->RemoveAllDecals();
	}
}

static ConCommand cl_removedecals( "cl_removedecals", RemoveDecals_f, "Remove the decals from the entity under the crosshair.", FCVAR_CHEAT );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::ToggleBBoxVisualization( bool bAbs )
{
	if ( bAbs )
	{
		m_bVisualizingAbsBox = !m_bVisualizingAbsBox;
	}
	else
	{
		m_bVisualizingBBox = !m_bVisualizingBBox;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void ToggleBBoxVisualization( bool bAbs )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Get the entity under my crosshair
	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );
	UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE,	MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pHit = tr.m_pEnt;
		pHit->ToggleBBoxVisualization( bAbs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Command to toggle visualizations of bboxes on the client
//-----------------------------------------------------------------------------
static void ToggleBBoxVisualization_f( void )
{
	ToggleBBoxVisualization( false );
}

static ConCommand cl_ent_bbox( "cl_ent_bbox", ToggleBBoxVisualization_f, "Displays the client's bounding box for the entity under the crosshair.", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: Command to toggle visualizations of bboxes on the client
//-----------------------------------------------------------------------------
static void ToggleAbsBoxVisualization_f( void )
{
	ToggleBBoxVisualization( true );
}

static ConCommand cl_ent_absbox( "cl_ent_absbox", ToggleAbsBoxVisualization_f, "Displays the client's absbox for the entity under the crosshair.", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DrawBBox( const Vector &mins, const Vector &maxs, const color32 &color )
{
	IMaterial *pMaterial = materials->FindMaterial( "debug/debugtranslucentvertexcolor", NULL );

	// Build the box corners
	Vector verts[8];
	verts[0][0] = mins[0];
	verts[0][1] = mins[1];
	verts[0][2] = mins[2];
	
	verts[1][0] = maxs[0];
	verts[1][1] = mins[1];
	verts[1][2] = mins[2];
	
	verts[2][0] = maxs[0];
	verts[2][1] = maxs[1];
	verts[2][2] = mins[2];
	
	verts[3][0] = mins[0];
	verts[3][1] = maxs[1];
	verts[3][2] = mins[2];
	
	verts[4][0] = mins[0];
	verts[4][1] = mins[1];
	verts[4][2] = maxs[2];
	
	verts[5][0] = maxs[0];
	verts[5][1] = mins[1];
	verts[5][2] = maxs[2];
	
	verts[6][0] = maxs[0];
	verts[6][1] = maxs[1];
	verts[6][2] = maxs[2];
	
	verts[7][0] = mins[0];
	verts[7][1] = maxs[1];
	verts[7][2] = maxs[2];

	// Draw the lines
	for ( int i = 0; i < 3; i++ )
	{
		FX_DrawLine( verts[i], verts[i+1], 0.5, pMaterial, color );
	}
	for ( i = 7; i > 4; i-- )
	{
		FX_DrawLine( verts[i], verts[i-1], 0.5, pMaterial, color );
	}
	FX_DrawLine( verts[1], verts[5], 0.5, pMaterial, color );
	FX_DrawLine( verts[6], verts[2], 0.5, pMaterial, color );
	FX_DrawLine( verts[4], verts[0], 0.5, pMaterial, color );
	FX_DrawLine( verts[0], verts[3], 0.5, pMaterial, color );
	FX_DrawLine( verts[3], verts[7], 0.5, pMaterial, color );
	FX_DrawLine( verts[4], verts[7], 0.5, pMaterial, color );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::DrawBBoxVisualizations( void )
{
	if ( m_bVisualizingBBox )
	{
		static color32 color = { 128,128,0,128};
		color.a = 255;
		color.r = 190;
		color.g = 190;
		DrawBBox( WorldAlignMins() + GetAbsOrigin(), WorldAlignMaxs() + GetAbsOrigin(), color );
	}

	if ( m_bVisualizingAbsBox )
	{
		static color32 color = {0,255,0,128};
		color.g = 255;
		color.b = 255;
		DrawBBox( m_vecAbsMins, m_vecAbsMaxs, color );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : RenderGroup_t
//-----------------------------------------------------------------------------
RenderGroup_t C_BaseEntity::GetRenderGroup()
{
	// When an entity has a material proxy, we have to recompute
	// translucency here because the proxy may have changed it.
	if (modelinfo->ModelHasMaterialProxy( GetModel() ))
	{
		modelinfo->RecomputeTranslucency( const_cast<model_t*>(GetModel()) );
	}

	// Figure out its RenderGroup.
	RenderGroup_t renderGroup = RENDER_GROUP_OTHER;
	if ( ( m_nRenderMode != kRenderNormal) || IsTransparent() )
	{
		if ( m_nRenderMode != kRenderEnvironmental )
		{
			renderGroup = RENDER_GROUP_TRANSLUCENT_ENTITY;
		}
	}
	else
	{
		if( IsIdentityBrush() )
		{
			renderGroup = RENDER_GROUP_WORLD;
		}
		else
		{
			renderGroup = RENDER_GROUP_OPAQUE_ENTITY;
		}
	}

	if ( ( renderGroup == RENDER_GROUP_TRANSLUCENT_ENTITY ) &&
		 ( modelinfo->IsTranslucentTwoPass( model ) ) )
	{
		renderGroup = RENDER_GROUP_TWOPASS;
	}

	return renderGroup;
}

// Purpose: Decal removing
//-----------------------------------------------------------------------------
void __MsgFunc_ClearDecals(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int iEntIndex = READ_BYTE();
	C_BaseEntity *pEntity = cl_entitylist->GetEnt( iEntIndex );
	if ( pEntity )
	{
		pEntity->RemoveAllDecals();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copy from this entity into one of the save slots (original or intermediate)
// Input  : slot - 
//			type - 
//			false - 
//			false - 
//			true - 
//			false - 
//			NULL - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::SaveData( const char *context, int slot, int type )
{
	VPROF( "C_BaseEntity::SaveData" );

	void *dest = ( slot == SLOT_ORIGINALDATA ) ? GetOriginalNetworkDataObject() : GetPredictedFrame( slot );
	Assert( dest );
	char sz[ 64 ];
	if ( slot == SLOT_ORIGINALDATA )
	{
		Q_snprintf( sz, sizeof( sz ), "%s SaveData(original)", context );
	}
	else
	{
		Q_snprintf( sz, sizeof( sz ), "%s SaveData(slot %02i)", context, slot );
	}

	CPredictionCopy copyHelper( type, dest, PC_DATA_PACKED, this, PC_DATA_NORMAL );
	int error_count = copyHelper.TransferData( sz, entindex(), GetPredDescMap() );
	return error_count;
}

//-----------------------------------------------------------------------------
// Purpose: Restore data from specified slot into current entity
// Input  : slot - 
//			type - 
//			false - 
//			false - 
//			true - 
//			false - 
//			NULL - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::RestoreData( const char *context, int slot, int type )
{
	VPROF( "C_BaseEntity::RestoreData" );

	const void *src = ( slot == SLOT_ORIGINALDATA ) ? GetOriginalNetworkDataObject() : GetPredictedFrame( slot );
	Assert( src );
	char sz[ 64 ];
	if ( slot == SLOT_ORIGINALDATA )
	{
		Q_snprintf( sz, sizeof( sz ), "%s RestoreData(original)", context );
	}
	else
	{
		Q_snprintf( sz, sizeof( sz ), "%s RestoreData(slot %02i)", context, slot );
	}

	CPredictionCopy copyHelper( type, this, PC_DATA_NORMAL, src, PC_DATA_PACKED );
	int error_count = copyHelper.TransferData( sz, entindex(), GetPredDescMap() );
	return error_count;
}

//-----------------------------------------------------------------------------
// Purpose: Determine approximate velocity based on updates from server
// Input  : vel - 
//-----------------------------------------------------------------------------
void C_BaseEntity::EstimateAbsVelocity( Vector& vel )
{
	if ( this == C_BasePlayer::GetLocalPlayer() )
	{
		vel = GetAbsVelocity();
		return;
	}

	vel.Init();

	float dt = m_iv_vecOrigin.GetInterval();
	if ( dt <= 0 )
		return;

	Vector org = m_iv_vecOrigin.GetCurrent();
	Vector prev = m_iv_vecOrigin.GetPrev();

	Vector distance = org - prev;

	if ( dt <= 0.0f )
	{
		vel.Init();
		return;
	}

	VectorScale( distance, 1.0f / dt, vel );
}

void C_BaseEntity::ResetLatched()
{
	Interp_Reset( GetVarMapping() );
}

//-----------------------------------------------------------------------------
// Purpose: Fixme, this needs a better solution
// Input  : flags - 
// Output : float
//-----------------------------------------------------------------------------
float C_BaseEntity::GetInterpolationAmount( int flags )
{
	if ( GetPredictable() || IsClientCreated() )
	{
		return TICK_RATE;
	}

	// Always fully interpolation in multiplayer...
	if ( gpGlobals->maxClients > 1 )
	{
		return TICK_RATE * ( TIME_TO_TICKS( render->GetInterpolationTime() ) + 1 );
	}

	if ( IsAnimatedEveryTick() && IsSimulatedEveryTick() )
	{
		return TICK_RATE;
	}

	if ( ( flags & LATCH_ANIMATION_VAR ) && IsAnimatedEveryTick() )
	{
		return TICK_RATE;
	}
	if ( ( flags & LATCH_SIMULATION_VAR ) && IsSimulatedEveryTick() )
	{
		return TICK_RATE;
	}

	return TICK_RATE * ( TIME_TO_TICKS( render->GetInterpolationTime() ) + 1 );
}

float C_BaseEntity::GetLastChangeTime( int flags )
{
	if ( GetPredictable() || IsClientCreated() )
	{
		return gpGlobals->curtime;
	}
	if ( flags & LATCH_ANIMATION_VAR )
	{
		return GetAnimTime();
	}

	if ( flags & LATCH_SIMULATION_VAR )
	{
		float st = GetSimulationTime();
		if ( st == 0.0f )
		{
			return gpGlobals->curtime;
		}
		return st;
	}

	Assert( 0 );

	return gpGlobals->curtime;
}

const Vector& C_BaseEntity::GetPrevLocalOrigin() const
{
	return m_iv_vecOrigin.GetPrev();
}

const QAngle& C_BaseEntity::GetPrevLocalAngles() const
{
	return m_iv_angRotation.GetPrev();
}

BEGIN_DATADESC_NO_BASE( C_BaseEntity )

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::ShouldSavePhysics()
{
	return false;
}

//-----------------------------------------------------------------------------
// handler to do stuff before you are saved
//-----------------------------------------------------------------------------
void C_BaseEntity::OnSave()
{
	// Here, we must force recomputation of all abs data so it gets saved correctly
	// We can't leave the dirty bits set because the loader can't cope with it.
	CalcAbsolutePosition();
	CalcAbsoluteVelocity();
}


//-----------------------------------------------------------------------------
// handler to do stuff after you are restored
//-----------------------------------------------------------------------------
void C_BaseEntity::OnRestore()
{
}

//-----------------------------------------------------------------------------
// Purpose: Saves the current object out to disk, by iterating through the objects
//			data description hierarchy
// Input  : &save - save buffer which the class data is written to
// Output : int	- 0 if the save failed, 1 on success
//-----------------------------------------------------------------------------
int C_BaseEntity::Save( ISave &save )
{
	// loop through the data description list, saving each data desc block
	int status = SaveDataDescBlock( save, GetDataDescMap() );

	return status;
}

//-----------------------------------------------------------------------------
// Purpose: Recursively saves all the classes in an object, in reverse order (top down)
// Output : int 0 on failure, 1 on success
//-----------------------------------------------------------------------------
int C_BaseEntity::SaveDataDescBlock( ISave &save, datamap_t *dmap )
{
	int nResult = save.WriteAll( this, dmap );
	return nResult;
}


//-----------------------------------------------------------------------------
// Purpose: Restores the current object from disk, by iterating through the objects
//			data description hierarchy
// Input  : &restore - restore buffer which the class data is read from
// Output : int	- 0 if the restore failed, 1 on success
//-----------------------------------------------------------------------------
int C_BaseEntity::Restore( IRestore &restore )
{
	// loops through the data description list, restoring each data desc block in order
	int status = RestoreDataDescBlock( restore, GetDataDescMap() );
	return status;
}

//-----------------------------------------------------------------------------
// Purpose: Recursively restores all the classes in an object, in reverse order (top down)
// Output : int 0 on failure, 1 on success
//-----------------------------------------------------------------------------
int C_BaseEntity::RestoreDataDescBlock( IRestore &restore, datamap_t *dmap )
{
	return restore.ReadAll( this, dmap );
}

//-----------------------------------------------------------------------------
// capabilities
//-----------------------------------------------------------------------------
int C_BaseEntity::ObjectCaps( void ) 
{
	return 0; 
}

