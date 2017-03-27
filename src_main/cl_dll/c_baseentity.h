//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: A base class for the client-side representation of entities.
//
//			This class encompasses both entities that are created on the server
//			and networked to the client AND entities that are created on the
//			client.
//
// $NoKeywords: $
//=============================================================================

#ifndef C_BASEENTITY_H
#define C_BASEENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"
#include "IClientEntityInternal.h"
#include "mouthinfo.h"
#include "engine/IVModelRender.h"
#include "client_class.h"
#include "IClientShadowMgr.h"
#include "ehandle.h"
#include "iclientunknown.h"
#include "client_thinklist.h"
#include "predictableid.h"
#include "soundflags.h"
#include "shareddefs.h"
#include "networkvar.h"
#include "interpolatedvar.h"
#include "collisionproperty.h"

class C_Team;
class IPhysicsObject;
class IClientVehicle;
class CPredictionCopy;
class C_BasePlayer;
struct studiohdr_t;
class CDamageModifier;
class IRecipientFilter;
class CUserCmd;
struct solid_t;
class ISave;
class IRestore;

struct CSoundParameters;

extern void RecvProxy_IntToColor32( const CRecvProxyData *pData, void *pStruct, void *pOut );

enum CollideType_t
{
	ENTITY_SHOULD_NOT_COLLIDE = 0,
	ENTITY_SHOULD_COLLIDE,
	ENTITY_SHOULD_RESPOND
};

struct VarMapEntry_t
{
	int					type;
	void				*data;
	IInterpolatedVar	*watcher;
};

struct VarMapping_t
{
	VarMapping_t()
	{
		m_bInitialized = false;
		m_pBaseClassVarMapping = NULL;
	}

	CUtlVector< VarMapEntry_t >	m_Entries;
	bool						m_bInitialized;
	VarMapping_t				*m_pBaseClassVarMapping;
};

#define DECLARE_INTERPOLATION_GUTS()								\
private:															\
	void AddVar( void *data, IInterpolatedVar *watcher, int type )	\
	{																\
		VarMapEntry_t map;											\
		map.data = data;											\
		map.watcher = watcher;										\
		map.type = type;											\
																	\
		m_VarMap.m_Entries.AddToTail( map );						\
	}																\
	VarMapping_t	m_VarMap;										\
public:																

#define DECLARE_INTERPOLATION()										\
	DECLARE_INTERPOLATION_GUTS();									\
	virtual VarMapping_t *GetVarMapping(void)						\
	{																\
		if ( !m_VarMap.m_bInitialized )								\
		{															\
			m_VarMap.m_bInitialized = true;							\
			m_VarMap.m_pBaseClassVarMapping = BaseClass::GetVarMapping(); \
		}															\
		return &m_VarMap;											\
	}															

#define DECLARE_INTERPOLATION_NOBASE()								\
	DECLARE_INTERPOLATION_GUTS();									\
	virtual VarMapping_t *GetVarMapping(void)						\
	{																\
		if ( !m_VarMap.m_bInitialized )								\
		{															\
			m_VarMap.m_bInitialized = true;							\
			m_VarMap.m_pBaseClassVarMapping = NULL;					\
		}															\
		return &m_VarMap;											\
	}	

//-----------------------------------------------------------------------------
// What kind of shadows to render?
//-----------------------------------------------------------------------------
enum ShadowType_t
{
	SHADOWS_NONE = 0,
	SHADOWS_SIMPLE,
	SHADOWS_RENDER_TO_TEXTURE,
	SHADOWS_RENDER_TO_TEXTURE_DYNAMIC,	// the shadow is always changing state
};

struct serialentity_t;

typedef CHandle<C_BaseEntity> EHANDLE; // The client's version of EHANDLE.

typedef void (C_BaseEntity::*BASEPTR)(void);
typedef void (C_BaseEntity::*ENTITYFUNCPTR)(C_BaseEntity *pOther );

// For entity creation on the client
typedef C_BaseEntity* (*DISPATCHFUNCTION)( void );

#include "touchlink.h"

//-----------------------------------------------------------------------------
// Purpose: For fully client side entities we use this information to determine
//  authoritatively if the server has acknowledged creating this entity, etc.
//-----------------------------------------------------------------------------
struct PredictionContext
{
	PredictionContext()
	{
		m_bActive					= false;
		m_nCreationCommandNumber	= -1;
		m_pszCreationModule			= NULL;
		m_nCreationLineNumber		= 0;
		m_hServerEntity				= NULL;
	}

	// The command_number of the usercmd which created this entity
	bool						m_bActive;
	int							m_nCreationCommandNumber;
	char const					*m_pszCreationModule;
	int							m_nCreationLineNumber;
	// The entity to whom we are attached
	CHandle< C_BaseEntity >		m_hServerEntity;
};

//-----------------------------------------------------------------------------
// Purpose: think contexts
//-----------------------------------------------------------------------------
struct thinkfunc_t
{
	const char	*m_pszContext;
	BASEPTR m_pfnThink;
	int		m_nNextThinkTick;
	int		m_nLastThinkTick;
};

#define CREATE_PREDICTED_ENTITY( className )	\
	C_BaseEntity::CreatePredictedEntityByName( className, __FILE__, __LINE__ );

//-----------------------------------------------------------------------------
// Purpose: Base client side entity object
//-----------------------------------------------------------------------------
class C_BaseEntity : public IClientEntity
{
// Construction
	DECLARE_CLASS_NOBASE( C_BaseEntity );

	friend class CPrediction;

public:
	DECLARE_DATADESC();
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION_NOBASE();

									C_BaseEntity();
	virtual							~C_BaseEntity();

	static C_BaseEntity				*CreatePredictedEntityByName( const char *classname, const char *module, int line, bool persist = false );

	virtual void					Spawn( void );
	virtual void					SpawnClientEntity( void );
	virtual void					Precache( void );

	void							Interp_SetupMappings( VarMapping_t *map );
	void							Interp_LatchChanges( VarMapping_t *map, int changeType );
	void							Interp_Reset( VarMapping_t *map );
	void							Interp_Interpolate( VarMapping_t *map, float currentTime );
	void							Interp_RestoreToLastNetworked( VarMapping_t *map );

	// Called by the CLIENTCLASS macros.
	virtual bool					Init( int entnum, int iSerialNum );

	// Called in the destructor to shutdown everything.
	void							Term();

	// memory handling, uses calloc so members are zero'd out on instantiation
    void							*operator new( size_t stAllocateBlock );
    void							*operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine );
	void							operator delete( void *pMem );

	// This just picks one of the routes to IClientUnknown.
	IClientUnknown*					GetIClientUnknown()	{ return this; }

// IClientUnknown overrides.
public:

	virtual void SetRefEHandle( const CBaseHandle &handle );
	virtual const CBaseHandle& GetRefEHandle() const;

	virtual void					Release();
	virtual ICollideable*			GetClientCollideable()	{ return &m_Collision; }
	virtual IClientNetworkable*		GetClientNetworkable()	{ return this; }
	virtual IClientRenderable*		GetClientRenderable()	{ return this; }
	virtual IClientEntity*			GetIClientEntity()		{ return this; }
	virtual C_BaseEntity*			GetBaseEntity()			{ return this; }
	virtual IClientThinkable*		GetClientThinkable()	{ return this; }


// Methods of IClientRenderable
public:

	virtual const Vector&			GetRenderOrigin( void );
	virtual const QAngle&			GetRenderAngles( void );
	virtual bool					IsTransparent( void );
	virtual bool					UsesFrameBufferTexture();
	virtual const model_t			*GetModel( void );
	virtual int						DrawModel( int flags );
	virtual void					ComputeFxBlend( void );
	virtual int						GetFxBlend( void );
	virtual bool					LODTest();
	virtual void					GetRenderBounds( Vector& mins, Vector& maxs );
	virtual bool					ShouldCacheRenderInfo();

	// Determine the color modulation amount
	virtual void					GetColorModulation( float* color );

public:
	virtual bool					TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	virtual bool					TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	// To mimic server call convention
	C_BaseEntity					*GetOwnerEntity( void ) const;
	void							SetOwnerEntity( C_BaseEntity *pOwner );

	// This function returns a value that scales all damage done by this entity.
	// Use CDamageModifier to hook in damage modifiers on a guy.
	virtual float					GetAttackDamageScale( void );

// IClientNetworkable implementation.
public:
	virtual void					NotifyShouldTransmit( ShouldTransmitState_t state );

	// save out interpolated values
	virtual void					PreDataUpdate( DataUpdateType_t updateType );
	virtual void					PostDataUpdate( DataUpdateType_t updateType );

	// pvs info. NOTE: Do not override these!!
	virtual void					SetDormant( bool bDormant );
	virtual bool					IsDormant( void );

	virtual int				GetEFlags() const;
	virtual void			SetEFlags( int iEFlags );
	void					AddEFlags( int nEFlagMask );
	void					RemoveEFlags( int nEFlagMask );
	bool					IsEFlagSet( int nEFlagMask ) const;

	// checks to see if the entity is marked for deletion
	bool							IsMarkedForDeletion( void );

	virtual int						entindex( void );

	// Server to client message received
	virtual void					ReceiveMessage( const char *msgname, int length, void *data );

	virtual void*					GetDataTableBasePtr();


// IClientThinkable.
public:
	// Called whenever you registered for a think message (with SetNextClientThink).
	virtual void					ClientThink();

	virtual ClientThinkHandle_t		GetThinkHandle();
	virtual void					SetThinkHandle( ClientThinkHandle_t hThink );


public:

	virtual bool					ShouldSavePhysics();

// save/restore stuff
	virtual void					OnSave();
	virtual void					OnRestore();
	// capabilities for save/restore
	virtual int						ObjectCaps( void );
	// only overload these if you have special data to serialize
	virtual int						Save( ISave &save );
	virtual int						Restore( IRestore &restore );

private:

	int SaveDataDescBlock( ISave &save, datamap_t *dmap );
	int RestoreDataDescBlock( IRestore &restore, datamap_t *dmap );

public:

	// Called after spawn, and in the case of self-managing objects, after load
	virtual bool	CreateVPhysics();

	// Convenience routines to init the vphysics simulation for this object.
	// This creates a static object.  Something that behaves like world geometry - solid, but never moves
	IPhysicsObject *VPhysicsInitStatic( void );
	// This creates a normal vphysics simulated object
	IPhysicsObject *VPhysicsInitNormal( SolidType_t solidType, int nSolidFlags, bool createAsleep, solid_t *pSolid = NULL );

private:
	// called by all vphysics inits
	bool			VPhysicsInitSetup();
public:

	void			VPhysicsSetObject( IPhysicsObject *pPhysics );
	// destroy and remove the physics object for this entity
	virtual void	VPhysicsDestroyObject( void );

	// Purpose: My physics object has been updated, react or extract data
	virtual void					VPhysicsUpdate( IPhysicsObject *pPhysics );
	inline IPhysicsObject			*VPhysicsGetObject( void ) const { return m_pPhysicsObject; }

// IClientEntity implementation.
public:
	virtual bool					SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime );
	virtual void					SetupWeights( void );
	virtual void					DoAnimationEvents( void );

	// Add entity to visible entities list?
	virtual void					AddEntity( void );

	virtual const Vector&			GetAbsOrigin( void ) const;
	virtual const QAngle&			GetAbsAngles( void ) const;

	const Vector&					GetNetworkOrigin() const;
	const QAngle&					GetNetworkAngles() const;

	void SetNetworkOrigin( const Vector& org );
	void SetNetworkAngles( const QAngle& ang );

	virtual const Vector&			GetLocalOrigin( void ) const;
	virtual void					SetLocalOrigin( const Vector& origin );
	vec_t							GetLocalOriginDim( int iDim ) const;		// You can use the X_INDEX, Y_INDEX, and Z_INDEX defines here.
	void							SetLocalOriginDim( int iDim, vec_t flValue );

	virtual const QAngle&			GetLocalAngles( void ) const;
	virtual void					SetLocalAngles( const QAngle& angles );
	vec_t							GetLocalAnglesDim( int iDim ) const;		// You can use the X_INDEX, Y_INDEX, and Z_INDEX defines here.
	void							SetLocalAnglesDim( int iDim, vec_t flValue );

	virtual const Vector&			GetPrevLocalOrigin() const;
	virtual const QAngle&			GetPrevLocalAngles() const;

	void							SetLocalTransform( const matrix3x4_t &localTransform );

	void							SetModelName( string_t name );
	string_t						GetModelName( void ) const;

	int								GetModelIndex( void ) const;
	void							SetModelIndex( int index );

	// These methods return a *world-aligned* box relative to the absorigin of the entity.
	// This is used for collision purposes and is *not* guaranteed
	// to surround the entire entity's visual representation
	// NOTE: It is illegal to ask for the world-aligned bounds for
	// SOLID_BSP objects
	virtual const Vector&			WorldAlignMins( ) const;
	virtual const Vector&			WorldAlignMaxs( ) const;

	// These methods return a box defined in the space of the entity
	// This OBB is used for collision purposes.
	// NOTE: This OBB is *not* guaranteed to surround the entire thing;
	// It's just used for a collision representation of the entity.
	// NOTE: For brush models, this OBB is guaranteed to be exact.
	// NOTE: It is illegal to ask for the entity-space bounds for
	// SOLID_BBOX objects
	virtual const Vector&			EntitySpaceMins( ) const;
	virtual const Vector&			EntitySpaceMaxs( ) const;

	// This defines collision bounds *in whatever space is currently defined by the solid type*
	//	SOLID_BBOX:		World Align
	//	SOLID_OBB:		Entity space
	//	SOLID_BSP:		Entity space
	//	SOLID_VPHYSICS	Not used
	void							SetCollisionBounds( const Vector& mins, const Vector &maxs );
	bool							IsBoundsDefinedInEntitySpace() const;

	// NOTE: These use the collision OBB to compute a reasonable center point for the entity
	const Vector&					EntitySpaceCenter( ) const;
	virtual const Vector&			WorldSpaceCenter( ) const;

	// FIXME: Do we want this?
	const Vector&					WorldAlignSize( ) const;
	const Vector&					EntitySpaceSize( ) const;
	bool							IsPointSized() const;

	// Selects a random point in the bounds given the normalized 0-1 bounds 
	void							RandomPointInBounds( const Vector &vecNormalizedMins, const Vector &vecNormalizedMaxs, Vector *pPoint) const;

	// FIXME: REMOVE! For backward compatibilty
	void							SetSize( const Vector &vecSize );

	// A box that is guaranteed to surround the entity.
	// It may not be particularly tight-fitting depending on performance-related questions
	// It may also fluctuate in size at any time
	virtual const Vector&			EntitySpaceSurroundingMins() const;
	virtual const Vector&			EntitySpaceSurroundingMaxs() const;
	virtual const Vector&			WorldSpaceSurroundingMins() const;
	virtual const Vector&			WorldSpaceSurroundingMaxs() const;

	// Returns the entity-to-world transform
	matrix3x4_t						&EntityToWorldTransform();
	const matrix3x4_t				&EntityToWorldTransform() const;

	// Some helper methods that transform a point from entity space to world space + back
	void							EntityToWorldSpace( const Vector &in, Vector *pOut ) const;
	void							WorldToEntitySpace( const Vector &in, Vector *pOut ) const;

	// Transforms an AABB measured in entity space to a box that surrounds it in world space
	// (so, the world-space box will have larger volume than the entity-space box)
	// Also added a version that goes the other way (from world to entity. Again, the box will grow in volume)
	void							EntityAABBToWorldAABB( const Vector &entityMins, const Vector &entityMaxs, Vector *pWorldMins, Vector *pWorldMaxs ) const;
	void							WorldAABBToEntityAABB( const Vector &worldMins, const Vector &worldMaxs, Vector *pEntityMins, Vector *pEntityMaxs ) const;

	// Returns a world-space AABB surrounding the collision representation.
	// NOTE: This may *not* be tightly fitting. For entities that define
	// their collision representation in entity space, this AABB will surround the entity space box
	void							WorldSpaceAABB( Vector *pWorldMins, Vector *pWorldMaxs ) const;

	// This function gets your parent's transform. If you're parented to an attachment,
	// this calculates the attachment's transform and gives you that.
	//
	// You must pass in tempMatrix for scratch space - it may need to fill that in and return it instead of 
	// pointing you right at a variable in your parent.
	matrix3x4_t&					GetParentToWorldTransform( matrix3x4_t &tempMatrix );

	void							GetVectors(Vector* forward, Vector* right, Vector* up) const;

	virtual void					SetAbsMins( const Vector& mins );
	virtual const Vector&			GetAbsMins( void ) const;

	virtual void					SetAbsMaxs( const Vector& maxs );
	virtual const Vector&			GetAbsMaxs( void ) const;
	
	// Sets abs angles, but also sets local angles to be appropriate
	void							SetAbsOrigin( const Vector& origin );
 	void							SetAbsAngles( const QAngle& angles );

	void							AddFlag( int flags );
	void							RemoveFlag( int flagsToRemove );
	void							ToggleFlag( int flagToToggle );
	int								GetFlags( void ) const;
	void							ClearFlags();

	MoveType_t						GetMoveType( void ) const;
	MoveCollide_t					GetMoveCollide( void ) const;
	virtual SolidType_t				GetSolid( void ) const;

	virtual int			 			GetSolidFlags( void ) const;
	bool							IsSolidFlagSet( int flagMask ) const;
	void							SetSolidFlags( int nFlags );
	void							AddSolidFlags( int nFlags );
	void							RemoveSolidFlags( int nFlags );
	bool							IsSolid() const;

	virtual CMouthInfo				*GetMouth( void );

	// Retrieve sound spatialization info for the specified sound on this entity
	// Return false to indicate sound is not audible
	virtual bool					GetSoundSpatialization( SpatializationInfo_t& info );

	virtual	bool					GetAttachment( int number, Vector &origin, QAngle &angles );

	// Team handling
	virtual C_Team					*GetTeam( void );
	virtual int						GetTeamNumber( void );
	virtual void					ChangeTeam( int iTeamNum );			// Assign this entity to a team.
	virtual int						GetRenderTeamNumber( void );
	virtual bool					InSameTeam( C_BaseEntity *pEntity );	// Returns true if the specified entity is on the same team as this one
	virtual bool					InLocalTeam( void );

	// ID Target handling
	virtual bool					IsValidIDTarget( void ) { return false; }
	virtual char					*GetIDString( void ) { return ""; };

	// See CSoundEmitterSystem
	void	EmitSound( const char *soundname, float soundtime = 0.0f );  // Override for doing the general case of CPASAttenuationFilter( this ), and EmitSound( filter, entindex(), etc. );
	void	StopSound( const char *soundname );

	static bool	GetParametersForSound( const char *soundname, CSoundParameters &params );
	static const char *GetWavFileForSound( const char *soundname );

	static void EmitSound( IRecipientFilter& filter, int iEntIndex, const char *soundname, const Vector *pOrigin = NULL, float soundtime = 0.0f );
	static void StopSound( int iEntIndex, const char *soundname );
	static soundlevel_t LookupSoundLevel( const char *soundname );

	static void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, float flAttenuation, int iFlags = 0, int iPitch = PITCH_NORM, 
		const Vector *pOrigin = NULL, float soundtime = 0.0f );

	static void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM, 
		const Vector *pOrigin = NULL, float soundtime = 0.0f );

	static void StopSound( int iEntIndex, int iChannel, const char *pSample );

	static void EmitAmbientSound( C_BaseEntity *entity, const Vector& origin, const char *soundname, int flags = 0, float soundtime = 0.0f );

	// These files need to be listed in scripts/game_sounds_manifest.txt
	static void	PrecacheSoundScript( const char *scriptfile );
	static void PrecacheScriptSound( const char *soundname, bool preload = false );

// C_BaseEntity local functions
public:

	// This can be used to setup the entity as a client-only entity. 
	// Override this to perform per-entity clientside setup
	virtual bool InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup );

	// Studio models implement this to update animation.
	virtual void					UpdateClientSideAnimation();
	
	// This function gets called on all client entities once per simulation phase.
	// It dispatches events like OnDataChanged(), and calls the legacy function AddEntity().
	virtual void					Simulate();	


	// This event is triggered during the simulation phase if an entity's data has changed. It is 
	// better to hook this instead of PostDataUpdate() because in PostDataUpdate(), server entity origins
	// are incorrect and attachment points can't be used.
	virtual void					OnDataChanged( DataUpdateType_t type );

	// This is called once per frame before any data is read in from the server.
	virtual void					OnPreDataChanged( DataUpdateType_t type );

	bool IsStandable() const;
	bool IsBSPModel() const;

	
	// If this is a vehicle, returns the vehicle interface
	virtual IClientVehicle*			GetClientVehicle() { return NULL; }

	// Returns the aiment render origin + angles
	virtual void					GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles );

	// Methods relating to traversing hierarchy
	C_BaseEntity *GetMoveParent( void ) const;
	C_BaseEntity *FirstMoveChild( void ) const;
	C_BaseEntity *NextMovePeer( void ) const;

	inline ClientEntityHandle_t		GetClientHandle() const	{ return ClientEntityHandle_t( m_RefEHandle ); }
	inline bool						IsServerEntity( void );

	// Sets up a render handle so the leaf system will draw this entity.
	void							SetupEntityRenderHandle( RenderGroup_t group );

	virtual RenderGroup_t			GetRenderGroup();

	// The value returned by here determines whether or not (and how) the entity
	// is put into the spatial partition.
	virtual CollideType_t			ShouldCollide();

	virtual bool					ShouldDraw();
	
	// Returns true if the entity changes its position every frame on the server but it doesn't
	// set animtime. In that case, the client returns true here so it copies the server time to
	// animtime in OnDataChanged and the position history is correct for interpolation.
	virtual bool					IsSelfAnimating();

	// Set appropriate flags and store off data when these fields are about to change
	virtual	void					OnLatchInterpolatedVariables( int flags );
	// Initialize things given a new model.
	virtual studiohdr_t				*OnNewModel();

	bool							IsSimulatedEveryTick() const;
	bool							IsAnimatedEveryTick() const;
	void							SetSimulatedEveryTick( bool sim );
	void							SetAnimatedEveryTick( bool anim );

	virtual void					ResetLatched();
	
	float							GetInterpolationAmount( int flags );
	float							GetLastChangeTime( int flags );

	// Interpolate the position for rendering
	virtual bool					Interpolate( float currentTime );
	// Did the object move so far that it shouldn't interpolate?
	virtual bool					Teleported( void );
	// Is this a submodel of the world ( *1 etc. in name ) ( brush models only )
	virtual bool					IsSubModel( void );
	// Deal with EF_* flags
	virtual void					CreateLightEffects( void );
	// If entity is MOVETYPE_FOLLOW, move it to correct origin
	virtual void	 				MoveAimEnt( void );
	// Reset internal fields
	virtual void					Clear( void );
	// Helper to determine if brush is an identity brush
	virtual bool					IsIdentityBrush( void );
	// Helper to draw raw brush models
	virtual int						DrawBrushModel( bool sort );
	// returns the material animation start time
	virtual float					GetTextureAnimationStartTime();
	// Indicates that a texture animation has wrapped
	virtual void					TextureAnimationWrapped();

	// Set the next think time. Pass in CLIENT_THINK_ALWAYS to have Think() called each frame.
	virtual void					SetNextClientThink( float nextThinkTime );

	// anything that has health can override this...
	virtual int						GetHealth() const { return 0; }
	virtual int						GetMaxHealth() const { return 1; }

	// Returns the health fraction
	float							HealthFraction() const;

	// Should this object cast shadows?
	virtual ShadowType_t			ShadowCastType();

	// Should this object receive shadows?
	virtual bool					ShouldReceiveShadows();

	void							RemoveFromLeafSystem();

	// A method to apply a decal to an entity
	virtual void					AddDecal( const Vector& rayStart, const Vector& rayEnd,
										const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );
	// A method to remove all decals from an entity
	void							RemoveAllDecals( void );

	// Is this a brush model?
	bool							IsBrushModel() const;

	// A random value 0-1 used by proxies to make sure they're not all in sync
	float							ProxyRandomValue() const { return m_flProxyRandomValue; }

	// The spawn time of this entity
	float							SpawnTime() const { return m_flSpawnTime; }

	// Defaults to false. If this is true, then the entity won't destroy its think handles
	// and client entity handles when it is destroyed.
	void							SetPredictedEntity( bool bPredicted );

	bool							IsClientCreated( void ) const;

	virtual void					UpdateOnRemove( void );

	void							SUB_Remove( void );

	// Prediction stuff
	/////////////////
	void							CheckInitPredictable( const char *context );

	void							AllocateIntermediateData( void );
	void							DestroyIntermediateData( void );
	void							ShiftIntermediateDataForward( int slots_to_remove, int previous_last_slot );

	void							*GetPredictedFrame( int framenumber );
	void							*GetOriginalNetworkDataObject( void );
	bool							IsIntermediateDataAllocated( void ) const;

	void							InitPredictable( void );
	void							ShutdownPredictable( void );

	void							SetPredictable( bool state );
	bool							GetPredictable( void ) const;
	void							PreEntityPacketReceived( int commands_acknowledged );
	void							PostEntityPacketReceived( void );
	bool							PostNetworkDataReceived( int commands_acknowledged );
	bool							GetPredictionEligible( void ) const;
	void							SetPredictionEligible( bool canpredict );

	enum
	{
		SLOT_ORIGINALDATA = -1,
	};

	int								SaveData( const char *context, int slot, int type );
	int								RestoreData( const char *context, int slot, int type );

	virtual char const *			DamageDecal( int bitsDamageType, int gameMaterial );
	virtual void					DecalTrace( trace_t *pTrace, char const *decalName );
	virtual void					ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );

	virtual bool					ShouldPredict( void ) { return false; };
	// interface function pointers
	void							(C_BaseEntity::*m_pfnThink)(void);
	virtual void					Think( void )
	{	
		if ( m_pfnThink )
		{
			( this->*m_pfnThink )();
		}
	}

	void							PhysicsDispatchThink( BASEPTR thinkFunc );
	
	// Toggle the visualization of the entity's abs/bbox
	void							ToggleBBoxVisualization( bool bAbs );
	void							DrawBBoxVisualizations( void );


// Methods implemented on both client and server
public:
	void							SetSize( const Vector &vecMin, const Vector &vecMax ); // UTIL_SetSize( pev, mins, maxs );
	char const						*GetClassname( void );
	int								PrecacheModel( const char *name ); // engine->PrecacheModel( name );
	int								PrecacheSound( const char *name ); // enginesound->PrecacheSound
	void							Remove( ); // UTIL_Remove( this );

public:

	// Determine approximate velocity based on updates from server
	void					EstimateAbsVelocity( Vector& vel );

	// The player drives simulation of this entity
	void					SetPlayerSimulated( C_BasePlayer *pOwner );
	bool					IsPlayerSimulated( void ) const;
	CBasePlayer				*GetSimulatingPlayer( void );
	void					UnsetPlayerSimulated( void );

	// Sorry folks, here lies TF2-specific stuff that really has no other place to go
	virtual bool			CanBePoweredUp( void ) { return false; }
	virtual bool			AttemptToPowerup( int iPowerup, float flTime, float flAmount = 0, C_BaseEntity *pAttacker = NULL, CDamageModifier *pDamageModifier = NULL ) { return false; }

	virtual void			SetCheckUntouch( bool check );
	bool					GetCheckUntouch() const;

	virtual bool			IsCurrentlyTouching( void ) const;

	virtual void			StartTouch( C_BaseEntity *pOther );
	virtual void			Touch( C_BaseEntity *pOther ); 
	virtual void			EndTouch( C_BaseEntity *pOther );

	void (C_BaseEntity ::*m_pfnTouch)( C_BaseEntity *pOther );

	void					PhysicsStep( void );

	touchlink_t				*PhysicsMarkEntityAsTouched( C_BaseEntity *other );
	void					PhysicsTouch( C_BaseEntity *pentOther );
	void					PhysicsStartTouch( C_BaseEntity *pentOther );

	// HACKHACK:Get the trace_t from the last physics touch call (replaces the even-hackier global trace vars)
	static trace_t			GetTouchTrace( void );

	// FIXME: Should be private, but I can't make em private just yet
	void					PhysicsImpact( C_BaseEntity *other, trace_t &trace );
 	void					PhysicsMarkEntitiesAsTouching( C_BaseEntity *other, trace_t &trace );
	void					PhysicsMarkEntitiesAsTouchingEventDriven( C_BaseEntity *other, trace_t &trace );

	// Physics helper
	static void				PhysicsRemoveTouchedList( C_BaseEntity *ent );
	static void				PhysicsNotifyOtherOfUntouch( C_BaseEntity *ent, C_BaseEntity *other );
	static void				PhysicsRemoveToucher( C_BaseEntity *other, touchlink_t *link );

	bool					PhysicsCheckWater( void );
	void					PhysicsCheckVelocity( void );
	void					PhysicsAddHalfGravity( float timestep );
	void					PhysicsAddGravityMove( Vector &move );

	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const;

	virtual void			SetGroundEntity( C_BaseEntity *ground );
	virtual C_BaseEntity		*GetGroundEntity( void );

	trace_t					PhysicsPushEntity( const Vector& push );
	void					PhysicsCheckWaterTransition( void );

	// Performs the collision resolution for fliers.
	void					PerformFlyCollisionResolution( trace_t &trace, Vector &move );
	void					ResolveFlyCollisionBounce( trace_t &trace, Vector &vecVelocity );
	void					ResolveFlyCollisionSlide( trace_t &trace, Vector &vecVelocity );
	void					ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );

	void					PhysicsCheckForEntityUntouch( void );

protected:
		// think function handling
	enum thinkmethods_t
	{
		THINK_FIRE_ALL_FUNCTIONS,
		THINK_FIRE_BASE_ONLY,
		THINK_FIRE_ALL_BUT_BASE,
	};
public:

	bool					PhysicsRunThink( thinkmethods_t thinkMethod = THINK_FIRE_ALL_FUNCTIONS );
	bool					PhysicsRunSpecificThink( int nContextIndex, BASEPTR thinkFunc );

	virtual void					PhysicsSimulate( void );
	virtual bool					IsAlive( void );

	bool							IsInWorld( void ) { return true; }

	/////////////////

	virtual bool					IsPlayer( void ) const { return false; };
	virtual bool					IsBaseCombatCharacter( void ) { return false; };
	// TF2 specific
	virtual bool					IsBaseObject( void ) const { return false; }

	// Returns the eye point + angles (used for viewing + shooting)
	virtual Vector		EyePosition();
	virtual const QAngle&	EyeAngles();			// Direction of eyes
	virtual const QAngle&	LocalEyeAngles();		// Direction of eyes in local space (pl.v_angle)
	
	// position of ears
	virtual Vector		EarPosition( void );

	// Called by physics to see if we should avoid a collision test....
	virtual bool		ShouldCollide( int collisionGroup, int contentsMask ) const;

	// Sets physics parameters
	void				SetFriction( float flFriction );

	void				SetGravity( float flGravity );
	float				GetGravity( void ) const;

	// Sets the model from a model index 
	void				SetModelByIndex( int nModelIndex );

	// Set model... (NOTE: Should only be used by client-only entities
	// Returns false if the model name is bogus
	bool				SetModel( const char *pModelName );

	void				SetModelPointer( const model_t *pModel );

	// Access movetype and solid.
	void				SetMoveType( MoveType_t val, MoveCollide_t moveCollide = MOVECOLLIDE_DEFAULT );	// Set to one of the MOVETYPE_ defines.
	void				SetMoveCollide( MoveCollide_t val );	// Set to one of the MOVECOLLIDE_ defines.
	void				SetSolid( SolidType_t val );	// Set to one of the SOLID_ defines.

	// NOTE: Setting the abs velocity in either space will cause a recomputation
	// in the other space, so setting the abs velocity will also set the local vel
	void				SetLocalVelocity( const Vector &vecVelocity );
	void				SetAbsVelocity( const Vector &vecVelocity );
	const Vector&		GetLocalVelocity() const;
	const Vector&		GetAbsVelocity( ) const;

	// NOTE: Setting the abs velocity in either space will cause a recomputation
	// in the other space, so setting the abs velocity will also set the local vel
	void				SetLocalAngularVelocity( const QAngle &vecAngVelocity );
	const QAngle&		GetLocalAngularVelocity( ) const;

//	void				SetAbsAngularVelocity( const QAngle &vecAngAbsVelocity );
//	const QAngle&		GetAbsAngularVelocity( ) const;

	const Vector&		GetBaseVelocity() const;
	void				SetBaseVelocity( const Vector& v );

	const Vector&		GetViewOffset() const { return m_vecViewOffset; }
	void				SetViewOffset( const Vector& v ) { m_vecViewOffset = v; }


	ClientRenderHandle_t	GetRenderHandle() const { return m_hRender; }

	void				SetRemovalFlag( bool bRemove );

	// Effects...
	bool				IsEffectActive( int nEffectMask ) const;
	void				ActivateEffect( int nEffectMask, bool bActive );

	// Makes sure the system knows the object is at its correct spot
	// FIXME: This was made virtual to get E3 strider working, it's a brutal hack
	// See C_Strider::ClientThink() for a description + replace this with a better solution!!
	virtual void		Relink();

	// Computes the abs position of a point specified in local space
	void				ComputeAbsPosition( const Vector &vecLocalPosition, Vector *pAbsPosition );

	// Computes the abs position of a direction specified in local space
	void				ComputeAbsDirection( const Vector &vecLocalDirection, Vector *pAbsDirection );

	// These methods encapsulate MOVETYPE_FOLLOW, which became obsolete
	void				FollowEntity( CBaseEntity *pBaseEntity );
	void				StopFollowingEntity( );	// will also change to MOVETYPE_NONE
	bool				IsFollowingEntity();
	CBaseEntity			*GetFollowedEntity();

	// For shadows rendering the correct body + sequence...
	virtual int GetBody() { return 0; }

	// Stubs on client
	void	NetworkStateManualMode( bool activate )		{ }
	void	NetworkStateChanged()						{ }
	void	NetworkStateSetUpdateInterval( float N )	{ }
	void	NetworkStateForceUpdate()					{ }

	// Think functions with contexts
	int		RegisterThinkContext( const char *szContext );
	BASEPTR	ThinkSet( BASEPTR func, float flNextThinkTime = 0, const char *szContext = NULL );
	void	SetNextThink( float nextThinkTime, const char *szContext = NULL );
	float	GetNextThink( char *szContext = NULL );
	float	GetLastThink( char *szContext = NULL );
	int		GetNextThinkTick( char *szContext = NULL );
	int		GetLastThinkTick( char *szContext = NULL );

	float	GetAnimTime() const;
	void	SetAnimTime( float at );

	float	GetSimulationTime() const;
	void	SetSimulationTime( float st );

#ifdef _DEBUG
	void FunctionCheck( void *pFunction, char *name );

	ENTITYFUNCPTR TouchSet( ENTITYFUNCPTR func, char *name ) 
	{ 
		//COMPILE_TIME_ASSERT( sizeof(func) == 4 );
		m_pfnTouch = func; 
		//FunctionCheck( (void *)*((int *)((char *)this + ( offsetof(C_BaseEntity,m_pfnTouch)))), name ); 
		return func;
	}
#endif

	// Gets the model instance + shadow handle
	ModelInstanceHandle_t GetModelInstance() { return m_ModelInstance; }
	ClientShadowHandle_t GetShadowHandle()	{ return m_ShadowHandle; }

protected:
	// Only meant to be called from subclasses
	void CreateModelInstance();
	void DestroyModelInstance();

	// Methods used by AddEntity
	void UpdatePosition();

	// Call this in OnDataChanged if you don't chain it down!
	void MarkMessageReceived();

	// Gets the last message time
	float	GetLastMessageTime() const		{ return m_flLastMessageTime; }

	CMouthInfo& MouthInfo()					{ return mouth; }

	// For non-players
	int	PhysicsClipVelocity (const Vector& in, const Vector& normal, Vector& out, float overbounce );

	// Sets the origin + angles to match the last position received
	void MoveToLastReceivedPosition( bool force = false );

public:
	// Accessors for above
	static int						GetPredictionRandomSeed( void );
	static void						SetPredictionRandomSeed( const CUserCmd *cmd );
	static C_BasePlayer				*GetPredictionPlayer( void );
	static void						SetPredictionPlayer( C_BasePlayer *player );

	// Collision group accessors
	int GetCollisionGroup() const;
	void SetCollisionGroup( int collisionGroup );

	static C_BaseEntity				*Instance( int iEnt );
	// Doesn't do much, but helps with trace results
	static C_BaseEntity				*Instance( IClientEntity *ent );
	static C_BaseEntity				*Instance( CBaseHandle hEnt );
	// For debugging shared code
	static bool						IsServer( void );
	static bool						IsClient( void );
	static char const				*GetDLLType( void );
	static void	SetAbsQueriesValid( bool bValid );
	static void EnableAbsRecomputations( bool bEnable );


	// Accessors for color.
	const color32 GetRenderColor() const;
	void SetRenderColor( byte r, byte g, byte b );
	void SetRenderColor( byte r, byte g, byte b, byte a );
	void SetRenderColorR( byte r );
	void SetRenderColorG( byte g );
	void SetRenderColorB( byte b );
	void SetRenderColorA( byte a );


public:	

	// Determine what entity this corresponds to
	int								index;	

	// Render information
	int								m_nRenderMode;
	int								m_nRenderFX;
	int								m_nRenderFXBlend;

	CNetworkVar( color32, m_clrRender );

private:
	
	// Model for rendering
	const model_t					*model;	


public:
	// Time animation sequence or frame was last changed
	// Move to c_studiomodel???
	float							m_flAnimTime;
	CInterpolatedVar< float >		m_iv_flAnimTime;
	float							m_flOldAnimTime;

	float							m_flSimulationTime;
	float							m_flOldSimulationTime;
	
	// Effects to apply
	int								m_fEffects;

	// 
	int								m_nNextThinkTick;
	int								m_nLastThinkTick;

	// Object model index
	int								m_nModelIndex;

	int								m_takedamage;
	int								m_lifeState;

	int								m_iHealth;

	// was pev->speed
	float							m_flSpeed;
	// Team Handling
	int								m_iTeamNum;

	// Certain entities (projectiles) can be created on the client
	CPredictableId					m_PredictableID;
	PredictionContext				*m_pPredictionContext;

	// pointer to a list of edicts that this entity is in contact with
	touchlink_t						m_EntitiesTouched;		

	// used so we know when things are no longer touching
	int								touchStamp;	

	bool							m_bCheckUntouch;

	// Called after predicted entity has been acknowledged so that no longer needed entity can
	//  be deleted
	// Return true to force deletion right now, regardless of isbeingremoved
	virtual bool					OnPredictedEntityRemove( bool isbeingremoved, C_BaseEntity *predicted );

	bool							IsDormantPredictable( void ) const;
	bool							BecameDormantThisPacket( void ) const;
	void							SetDormantPredictable( bool dormant );

	int								GetWaterLevel() const;
	void							SetWaterLevel( int nLevel );
	int								GetWaterType() const;
	void							SetWaterType( int nType );

	float							GetElasticity( void ) const;

	int								GetTextureFrameIndex( void );
	void							SetTextureFrameIndex( int iIndex );

	float							GetShadowCastDistance( void )	const;

protected:
	// Interpolation says don't draw yet
	bool							m_bReadyToDraw;
	bool							m_bDrawnOnce;

	// FIXME: Should I move the functions handling these out of C_ClientEntity
	// and into C_BaseEntity? Then we could make these private.
	// Client handle
	CBaseHandle m_RefEHandle;	// Reference ehandle. Used to generate ehandles off this entity.

	// Spatial partition
	SpatialPartitionHandle_t		m_Partition;

	// Used to store the state we were added to the BSP as, so it can
	// relink the entity if the state changes.
	ClientRenderHandle_t			m_hRender;	// link into spatial partition

	// pointer to the entity's physics object (vphysics.dll)
	IPhysicsObject					*m_pPhysicsObject;	

	bool							m_bPredictionEligible;

	int								m_nSimulationTick;

	// Think contexts
	int								GetIndexForContext( const char *pszContext );
	CUtlVector< thinkfunc_t >		m_aThinkFunctions;
	int								m_iCurrentThinkContext;

	// Bbox visualization
	bool							m_bVisualizingBBox;
	bool							m_bVisualizingAbsBox;

	// Object eye position
	Vector							m_vecViewOffset;

private:
	friend void OnRenderStart();

	// This can be used to setup the entity as a client-only entity. It gets an entity handle,
	// a render handle, and is put into the spatial partition.
	bool InitializeAsClientEntityByIndex( int iIndex, RenderGroup_t renderGroup );

	// Figure out the smoothly interpolated origin for all server entities. Happens right before
	// letting all entities simulate.
	static void InterpolateServerEntities();
	
	// Check which entities want to be drawn and add them to the leaf system.
	static void	AddVisibleEntities();

	// Computes the base velocity
	void UpdateBaseVelocity( void );

	// Physics-related private methods
	void PhysicsPusher( void );
	void PhysicsNone( void );
	void PhysicsNoclip( void );
	void PhysicsParent( void );
	void PhysicsStepRunTimestep( float timestep );
	void PhysicsToss( void );
	void PhysicsCustom( void );

	// Simulation in local space of rigid children
	void PhysicsRigidChild( void );

	// Computes absolute position based on hierarchy
	void CalcAbsolutePosition( );
	void CalcAbsoluteVelocity();
	void CalcAbsoluteAngularVelocity();

	// Computes new angles based on the angular velocity
	void SimulateAngles( float flFrameTime );

	// Implement this if you use MOVETYPE_CUSTOM
	virtual void PerformCustomPhysics( Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity );

	// methods related to decal adding
	void AddStudioDecal( const Ray_t& ray, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );
	void AddBrushModelDecal( const Ray_t& ray, const Vector& decalCenter, int decalIndex, bool doTrace, trace_t& tr );

	virtual void SetObjectCollisionBox( void );
	void ComputeSurroundingBox( void );

	void ComputePackedOffsets( void );
	int ComputePackedSize_R( datamap_t *map );
	int GetIntermediateDataSize( void );

	void UnlinkChild( C_BaseEntity *pParent, C_BaseEntity *pChild );
	void LinkChild( C_BaseEntity *pParent, C_BaseEntity *pChild );
	void HierarchySetParent( C_BaseEntity *pNewParent );
	void UnlinkFromHierarchy();

	// Computes the water level + type
	void UpdateWaterState();

	// Checks a sweep without actually performing the move
	void PhysicsCheckSweep( const Vector& vecAbsStart, const Vector &vecAbsDelta, trace_t *pTrace );

	// Unlinks from hierarchy
	void SetParent( C_BaseEntity *pParentEntity );

	// FIXME: REMOVE!!!
	void MoveToAimEnt( );

	// Invalidates the abs state of all children
	void InvalidatePhysicsRecursive( int nDirtyFlag, int nAdditionalChildFlag = 0 );

	// Sets/Gets the next think based on context index
	void SetNextThink( int nContextIndex, float thinkTime );
	void SetLastThink( int nContextIndex, float thinkTime );
	float GetNextThink( int nContextIndex ) const;
	int	GetNextThinkTick( int nContextIndex ) const;

	// Object velocity
	Vector							m_vecVelocity;

	Vector							m_vecAbsVelocity;

	// was pev->avelocity
	QAngle							m_vecAngVelocity;

//	QAngle							m_vecAbsAngVelocity;

	// It's still in the list for "fixup purposes" and simulation, but don't try to render it any more...
	bool							m_bDormantPredictable;

	// So we can clean it up
	int								m_nIncomingPacketEntityBecameDormant;

	// Mouth lipsync/envelope following values
	CMouthInfo						mouth;

	// See SetPredictedEntity.
	bool							m_bPredictedEntity;

	// For client/server entities, true if the entity goes outside the PVS.
	// Unused for client only entities.
	bool							m_bDormant;

	// The spawn time of the entity
	float							m_flSpawnTime;

	// Timestamp of message arrival
	float							m_flLastMessageTime;

	// Base velocity
	Vector							m_vecBaseVelocity;
	
	// Gravity multiplier
	float							m_flGravity;

	// Model instance data..
	ModelInstanceHandle_t			m_ModelInstance;

	// Shadow data
	ClientShadowHandle_t			m_ShadowHandle;

	// A random value used by material proxies for each model instance.
	float							m_flProxyRandomValue;

	ClientThinkHandle_t				m_hThink;

	int								m_iEFlags;	// entity flags EFL_*

	// Object movetype
	MoveType_t						m_MoveType;
	MoveCollide_t					m_MoveCollide;

	// Hierarchy
	CHandle<C_BaseEntity>			m_pMoveParent;
	unsigned char					m_iParentAttachment; // 0 if we're relative to the parent's absorigin and absangles.
	CHandle<C_BaseEntity>			m_pMoveChild;
	CHandle<C_BaseEntity>			m_pMovePeer;
	CHandle<C_BaseEntity>			m_pMovePrevPeer;

	// The moveparent received from networking data
	CHandle<C_BaseEntity>			m_hNetworkMoveParent;
	CHandle<C_BaseEntity>			m_hOldMoveParent;

	string_t						m_ModelName;

	CNetworkVarEmbedded( CCollisionProperty, m_Collision );

	Vector							m_vecAbsMins;
	Vector							m_vecAbsMaxs;

	// Physics state
	float							m_flElasticity;

	float							m_flShadowCastDistance;

	int								m_nWaterLevel;
	EHANDLE							m_hGroundEntity;

	int								m_nWaterType;

	// Friction.
	float							m_flFriction;       

	Vector							m_vecAbsOrigin;

	// Object orientation
	QAngle							m_angAbsRotation;

	Vector							m_vecOldOrigin;
	QAngle							m_vecOldAngRotation;

	Vector							m_vecOrigin;
	CInterpolatedVar< Vector >		m_iv_vecOrigin;
	QAngle							m_angRotation;
	CInterpolatedVar< QAngle >		m_iv_angRotation;

	// Specifies the entity-to-world transform
	matrix3x4_t						m_rgflCoordinateFrame;

	// Last values to come over the wire. Used for interpolation.
	Vector							m_vecNetworkOrigin;
	QAngle							m_angNetworkAngles;

	// Behavior flags
	int								m_fFlags;

	// used to cull collision tests
	int								m_CollisionGroup;

	// Prediction system
	bool							m_bPredictable;

	// For storing prediction results and pristine network state
	byte							*m_pIntermediateData[ MULTIPLAYER_BACKUP ];
	byte							*m_pOriginalData;

	// The list that holds OnDataChanged events uses this to make sure we don't get multiple
	// OnDataChanged calls in the same frame if the client receives multiple packets.
	int								m_DataChangeEventRef;

	bool							m_bIsPlayerSimulated;
	
	// Player who is driving my simulation
	CHandle< CBasePlayer >			m_hPlayerSimulationOwner;

	// The owner!
	EHANDLE							m_hOwnerEntity;
	
	// This is a random seed used by the networking code to allow client - side prediction code
	//  randon number generators to spit out the same random numbers on both sides for a particular
	//  usercmd input.
	static int						m_nPredictionRandomSeed;
	static C_BasePlayer				*m_pPredictionPlayer;
	static bool						s_bAbsQueriesValid;
	static bool						s_bAbsRecomputationEnabled;
	
	//Adrian
	int								m_iTextureFrameIndex;

	CNetworkVar( bool, m_bSimulatedEveryTick );
	CNetworkVar( bool, m_bAnimatedEveryTick );
};

EXTERN_RECV_TABLE(DT_BaseEntity);

inline bool FClassnameIs( C_BaseEntity *pEntity, const char *szClassname )
{ 
	return !strcmp( pEntity->GetClassname(), szClassname ) ? true : false; 
}

#define SetThink( a ) ThinkSet( static_cast <void (CBaseEntity::*)(void)> (a), 0, NULL )
#define SetContextThink( a, b, context ) ThinkSet( static_cast <void (CBaseEntity::*)(void)> (a), (b), context )

#ifdef _DEBUG
#define SetTouch( a ) TouchSet( static_cast <void (C_BaseEntity::*)(C_BaseEntity *)> (a), #a )

#else
#define SetTouch( a ) m_pfnTouch = static_cast <void (C_BaseEntity::*)(C_BaseEntity *)> (a)

#endif

//-----------------------------------------------------------------------------
// Purpose: Returns whether this entity was created on the client.
//-----------------------------------------------------------------------------
inline bool C_BaseEntity::IsServerEntity( void )
{
	return index != -1;
}

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline matrix3x4_t &C_BaseEntity::EntityToWorldTransform()
{ 
	Assert( s_bAbsQueriesValid );
	CalcAbsolutePosition();
	return m_rgflCoordinateFrame; 
}

inline const matrix3x4_t &C_BaseEntity::EntityToWorldTransform() const
{
	Assert( s_bAbsQueriesValid );
	const_cast<C_BaseEntity*>(this)->CalcAbsolutePosition();
	return m_rgflCoordinateFrame; 
}

//-----------------------------------------------------------------------------
// Some helper methods that transform a point from entity space to world space + back
//-----------------------------------------------------------------------------
inline void C_BaseEntity::EntityToWorldSpace( const Vector &in, Vector *pOut ) const
{
	if ( GetAbsAngles() == vec3_angle )
	{
		VectorAdd( in, GetAbsOrigin(), *pOut );
	}
	else
	{
		VectorTransform( in, EntityToWorldTransform(), *pOut );
	}
}

inline void C_BaseEntity::WorldToEntitySpace( const Vector &in, Vector *pOut ) const
{
	if ( GetAbsAngles() == vec3_angle )
	{
		VectorSubtract( in, GetAbsOrigin(), *pOut );
	}
	else
	{
		VectorITransform( in, EntityToWorldTransform(), *pOut );
	}
}

inline const Vector &C_BaseEntity::GetAbsVelocity( ) const
{
	Assert( s_bAbsQueriesValid );
	const_cast<C_BaseEntity*>(this)->CalcAbsoluteVelocity();
	return m_vecAbsVelocity;
}

/*
// Re-enable once angular velocity is stored in a better representation
inline const QAngle &C_BaseEntity::GetAbsAngularVelocity( ) const
{
	Assert( s_bAbsQueriesValid );
	const_cast<C_BaseEntity*>(this)->CalcAbsoluteAngularVelocity();
	return m_vecAbsAngVelocity;
}
*/

inline C_BaseEntity	*C_BaseEntity::Instance( IClientEntity *ent )
{
	return ent ? ent->GetBaseEntity() : NULL;
}

// For debugging shared code
inline bool	C_BaseEntity::IsServer( void )
{
	return false;
}

inline bool	C_BaseEntity::IsClient( void )
{
	return true;
}

inline const char *C_BaseEntity::GetDLLType( void )
{
	return "client";
}


//-----------------------------------------------------------------------------
// Methods relating to solid type + flags
//-----------------------------------------------------------------------------
inline void C_BaseEntity::SetSolidFlags( int nFlags )
{
	m_Collision.SetSolidFlags( nFlags );
}

inline bool C_BaseEntity::IsSolidFlagSet( int flagMask ) const
{
	return m_Collision.IsSolidFlagSet( flagMask );
}

inline int	C_BaseEntity::GetSolidFlags( void ) const
{
	return m_Collision.GetSolidFlags( );
}

inline void C_BaseEntity::AddSolidFlags( int nFlags )
{
	m_Collision.AddSolidFlags( nFlags );
}

inline void C_BaseEntity::RemoveSolidFlags( int nFlags )
{
	m_Collision.RemoveSolidFlags( nFlags );
}

inline bool C_BaseEntity::IsSolid() const
{
	return m_Collision.IsSolid( );
}

inline void C_BaseEntity::SetSolid( SolidType_t val )
{
	m_Collision.SetSolid( val );
}

inline SolidType_t C_BaseEntity::GetSolid( ) const
{
	return m_Collision.GetSolid( );
}

inline void C_BaseEntity::SetCollisionBounds( const Vector& mins, const Vector &maxs )
{
	m_Collision.SetCollisionBounds( mins, maxs );
}


//-----------------------------------------------------------------------------
// Methods relating to bounds
//-----------------------------------------------------------------------------
inline bool C_BaseEntity::IsBoundsDefinedInEntitySpace() const
{
	return m_Collision.IsBoundsDefinedInEntitySpace();
}

inline const Vector& C_BaseEntity::WorldAlignMins( ) const
{
	return m_Collision.WorldAlignMins();
}

inline const Vector& C_BaseEntity::WorldAlignMaxs( ) const
{
	return m_Collision.WorldAlignMaxs();
}

inline const Vector& C_BaseEntity::EntitySpaceMins( ) const
{
	return m_Collision.EntitySpaceMins();
}

inline const Vector& C_BaseEntity::EntitySpaceMaxs( ) const
{
	return m_Collision.EntitySpaceMaxs();
}

inline const Vector& C_BaseEntity::EntitySpaceCenter( ) const
{
	Assert( IsBoundsDefinedInEntitySpace() );
	VectorLerp( m_Collision.m_vecMins, m_Collision.m_vecMaxs, 0.5f, m_Collision.m_vecCenter );
	return m_Collision.m_vecCenter;
//	return m_vecCenter.Get();
}
  
inline const Vector& C_BaseEntity::WorldAlignSize( ) const
{
	return m_Collision.WorldAlignSize();
}

inline const Vector& C_BaseEntity::EntitySpaceSize( ) const
{
	return m_Collision.EntitySpaceSize();
}

inline bool CBaseEntity::IsPointSized() const
{
	return m_Collision.m_vecSize == vec3_origin;
}

// It may not be particularly tight-fitting depending on performance-related questions
// It may also fluctuate in size at any time
inline const Vector& C_BaseEntity::EntitySpaceSurroundingMins() const
{
	// FIXME!
	return m_Collision.m_vecMins.Get();
}

inline const Vector& C_BaseEntity::EntitySpaceSurroundingMaxs() const
{
	// FIXME!
	return m_Collision.m_vecMaxs.Get();
}

inline const Vector& C_BaseEntity::WorldSpaceSurroundingMins() const
{
	// FIXME!
	return m_vecAbsMins;
}

inline const Vector& C_BaseEntity::WorldSpaceSurroundingMaxs() const
{
	// FIXME!
	return m_vecAbsMaxs;
}


// FIXME: REMOVE
inline void C_BaseEntity::SetSize( const Vector &vecSize )
{
	m_Collision.m_vecSize = vecSize;
}


//-----------------------------------------------------------------------------
// Methods relating to traversing hierarchy
//-----------------------------------------------------------------------------
inline C_BaseEntity *C_BaseEntity::GetMoveParent( void ) const
{
	return m_pMoveParent; 
}

inline C_BaseEntity *C_BaseEntity::FirstMoveChild( void ) const
{
	return m_pMoveChild; 
}

inline C_BaseEntity *C_BaseEntity::NextMovePeer( void ) const
{
	return m_pMovePeer; 
}

//-----------------------------------------------------------------------------
// Velocity
//-----------------------------------------------------------------------------
inline const Vector& C_BaseEntity::GetLocalVelocity() const 
{ 
	return m_vecVelocity; 
}

inline const QAngle& C_BaseEntity::GetLocalAngularVelocity( ) const
{
	return m_vecAngVelocity;
}

inline const Vector& C_BaseEntity::GetBaseVelocity() const 
{ 
	return m_vecBaseVelocity; 
}

inline void	C_BaseEntity::SetBaseVelocity( const Vector& v ) 
{ 
	m_vecBaseVelocity = v; 
}

inline void C_BaseEntity::SetFriction( float flFriction ) 
{ 
	m_flFriction = flFriction; 
}

inline void C_BaseEntity::SetGravity( float flGravity ) 
{ 
	m_flGravity = flGravity; 
}

inline float C_BaseEntity::GetGravity( void ) const 
{ 
	return m_flGravity; 
}

inline int C_BaseEntity::GetWaterLevel() const
{
	return m_nWaterLevel;
}

inline void C_BaseEntity::SetWaterLevel( int nLevel )
{
	m_nWaterLevel = nLevel;
}

inline int C_BaseEntity::GetWaterType() const
{
	return m_nWaterType;
}

inline void C_BaseEntity::SetWaterType( int nType )
{
	m_nWaterType = nType;
}

inline float C_BaseEntity::GetElasticity( void )	const			
{ 
	return m_flElasticity; 
}

inline float C_BaseEntity::GetShadowCastDistance( void )	const			
{ 
	return m_flShadowCastDistance; 
}

inline const color32 CBaseEntity::GetRenderColor() const
{
	return m_clrRender;
}

inline void C_BaseEntity::SetRenderColor( byte r, byte g, byte b )
{
	color32 clr = { r, g, b, m_clrRender->a };
	m_clrRender = clr;
}

inline void C_BaseEntity::SetRenderColor( byte r, byte g, byte b, byte a )
{
	color32 clr = { r, g, b, a };
	m_clrRender = clr;
}

inline void C_BaseEntity::SetRenderColorR( byte r )
{
	SetRenderColor( r, GetRenderColor().g, GetRenderColor().b );
}

inline void C_BaseEntity::SetRenderColorG( byte g )
{
	SetRenderColor( GetRenderColor().r, g, GetRenderColor().b );
}

inline void C_BaseEntity::SetRenderColorB( byte b )
{
	SetRenderColor( GetRenderColor().r, GetRenderColor().g, b );
}

inline void C_BaseEntity::SetRenderColorA( byte a )
{
	SetRenderColor( GetRenderColor().r, GetRenderColor().g, GetRenderColor().b, a );
}

//-----------------------------------------------------------------------------
// checks to see if the entity is marked for deletion
//-----------------------------------------------------------------------------
inline bool C_BaseEntity::IsMarkedForDeletion( void ) 
{ 
	return (m_iEFlags & EFL_KILLME); 
}

inline void C_BaseEntity::AddEFlags( int nEFlagMask )
{
	m_iEFlags |= nEFlagMask;
}

inline void C_BaseEntity::RemoveEFlags( int nEFlagMask )
{
	m_iEFlags &= ~nEFlagMask;
}

inline bool CBaseEntity::IsEFlagSet( int nEFlagMask ) const
{
	return (m_iEFlags & nEFlagMask) != 0;
}

C_BaseEntity *CreateEntityByName( const char *className );

#endif // C_BASEENTITY_H
