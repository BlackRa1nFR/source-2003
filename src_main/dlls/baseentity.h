//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEENTITY_H
#define BASEENTITY_H
#ifdef _WIN32
#pragma once
#endif

#define TEAMNUM_NUM_BITS	6

#include "entitylist.h"
#include "networkvar.h"
#include "collisionproperty.h"

class CDamageModifier;

struct CSoundParameters;

class AI_CriteriaSet;
class IResponseSystem;

// Matching the high level concept is significantly better than other criteria
// FIXME:  Could do this in the script file by making it required and bumping up weighting there instead...
#define CONCEPT_WEIGHT 5.0f

typedef CHandle<CBaseEntity> EHANDLE;


#define MANUALMODE_GETSET_PROP(type, accessorName, varName) \
	private:\
		type varName;\
	public:\
		inline const type& Get##accessorName##() const { return varName; } \
		inline type& Get##accessorName##() { return varName; } \
		inline void Set##accessorName##( const type &val ) { varName = val; m_NetStateMgr.StateChanged(); }

#define MANUALMODE_GETSET_EHANDLE(type, accessorName, varName) \
	private:\
		CHandle<type> varName;\
	public:\
		inline type* Get##accessorName##() { return varName.Get(); } \
		inline void Set##accessorName##( type *pType ) { varName = pType; m_NetStateMgr.StateChanged(); }


// saverestore.h declarations
class CSaveRestoreData;
struct typedescription_t;
class ISave;
class IRestore;
class CBaseEntity;
class CEntityMapData;
class CBaseCombatWeapon;
class IPhysicsObject;
class IPhysicsShadowController;
class CBaseCombatCharacter;
class CTeam;
class Vector;
struct gamevcollisionevent_t;
class CBaseAnimating;
class CBasePlayer;
class IServerVehicle;
struct solid_t;
struct notify_system_event_params_t;
class CAI_BaseNPC;
class CAI_Senses;
class CSquadNPC;
class variant_t;
class CEventAction;
typedef struct KeyValueData_s KeyValueData;
class CUserCmd;

typedef CUtlVector< CBaseEntity* > EntityList_t;

// For CLASSIFY
enum Class_T
{
	CLASS_NONE=0,				
	CLASS_PLAYER,			
	CLASS_PLAYER_ALLY,
	CLASS_PLAYER_ALLY_VITAL,
	CLASS_ANTLION,
	CLASS_BARNACLE,
	CLASS_BULLSEYE,
	CLASS_BULLSQUID,	
	CLASS_CITIZEN_PASSIVE,	
	CLASS_CITIZEN_REBEL,
	CLASS_COMBINE,
	CLASS_COMBINE_GUNSHIP,
	CLASS_CONSCRIPT,
	CLASS_CREMATOR,
	CLASS_HEADCRAB,
	CLASS_HOUNDEYE,
	CLASS_MANHACK,
	CLASS_METROPOLICE,		
	CLASS_MILITARY,		
	CLASS_MORTAR_SYNTH,		
	CLASS_SCANNER,		
	CLASS_STALKER,		
	CLASS_VORTIGAUNT,
	CLASS_WASTE_SCANNER,		
	CLASS_ZOMBIE,
	CLASS_PROTOSNIPER,
	CLASS_MISSILE,
	CLASS_FLARE,
	CLASS_EARTH_FAUNA,
	
#if defined( HL1_DLL )
//	CLASS_NONE,					// HL1 port, HL2 already defined
	CLASS_MACHINE,				// HL1 port
//	CLASS_PLAYER,				// HL1 port, HL2 already defined
	CLASS_HUMAN_PASSIVE,		// HL1 port
	CLASS_HUMAN_MILITARY,		// HL1 port
	CLASS_ALIEN_MILITARY,		// HL1 port
	CLASS_ALIEN_MONSTER,		// HL1 port
	CLASS_ALIEN_PREY,			// HL1 port
	CLASS_ALIEN_PREDATOR,		// HL1 port
	CLASS_INSECT,				// HL1 port
//	CLASS_PLAYER_ALLY,			// HL1 port, HL2 already defined
	CLASS_PLAYER_BIOWEAPON,		// HL1 port
	CLASS_ALIEN_BIOWEAPON,		// HL1 port
//	CLASS_BARNACLE,				// HL1 port, HL2 already defined
#endif	// defined( HL1_DLL )

	NUM_AI_CLASSES
};

//
// Structure passed to input handlers.
//
struct inputdata_t
{
	CBaseEntity *pActivator;		// The entity that initially caused this chain of output events.
	CBaseEntity *pCaller;			// The entity that fired this particular output.
	variant_t value;				// The data parameter for this output.
	int nOutputID;					// The unique ID of the output that was fired.
};

// Serializable list of context as set by entity i/o and used for deducing proper
//  speech state, et al.
struct ResponseContext_t
{
	DECLARE_SIMPLE_DATADESC();

	string_t		m_iszName;
	string_t		m_iszValue;
};

//-----------------------------------------------------------------------------

typedef void (CBaseEntity::*BASEPTR)(void);
typedef void (CBaseEntity::*ENTITYFUNCPTR)(CBaseEntity *pOther );
typedef void (CBaseEntity::*USEPTR)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

#define DEFINE_THINKFUNC( type, function ) DEFINE_FUNCTION_RAW( type, function, BASEPTR )
#define DEFINE_ENTITYFUNC( type, function ) DEFINE_FUNCTION_RAW( type, function, ENTITYFUNCPTR )
#define DEFINE_USEFUNC( type, function ) DEFINE_FUNCTION_RAW( type, function, USEPTR )

// Things that toggle (buttons/triggers/doors) need this
enum TOGGLE_STATE
{
	TS_AT_TOP,
	TS_AT_BOTTOM,
	TS_GOING_UP,
	TS_GOING_DOWN
};


// Debug overlay bits
enum DebugOverlayBits_t
{
	OVERLAY_TEXT_BIT			=	0x00000001,		// show text debug overlay for this entity
	OVERLAY_NAME_BIT			=	0x00000002,		// show name debug overlay for this entity
	OVERLAY_BBOX_BIT			=	0x00000004,		// show bounding box overlay for this entity
	OVERLAY_PIVOT_BIT			=	0x00000008,		// show pivot for this entity
	OVERLAY_MESSAGE_BIT			=	0x00000010,		// show messages for this entity
	OVERLAY_ABSBOX_BIT			=	0x00000020,		// show abs bounding box overlay
	OVERLAY_RBOX_BIT			=   0x00000040,     // show the rbox overlay

	OVERLAY_NPC_SELECTED_BIT	=	0x00001000,		// the npc is current selected
	OVERLAY_NPC_NEAREST_BIT		=	0x00002000,		// show the nearest node of this npc
	OVERLAY_NPC_ROUTE_BIT		=	0x00004000,		// draw the route for this npc
	OVERLAY_NPC_TRIANGULATE_BIT =	0x00008000,		// draw the triangulation for this npc
	OVERLAY_NPC_ZAP_BIT			=	0x00010000,		// destroy the NPC
	OVERLAY_NPC_ENEMIES_BIT		=	0x00020000,		// show npc's enemies
	OVERLAY_NPC_CONDITIONS_BIT	=	0x00040000,		// show NPC's current conditions
	OVERLAY_NPC_SQUAD_BIT		=	0x00080000,		// show npc squads
	OVERLAY_NPC_TASK_BIT		=	0x00100000,		// show npc task details
	OVERLAY_NPC_FOCUS_BIT		=	0x00200000,		// show line to npc's enemy and target
	OVERLAY_NPC_VIEWCONE_BIT	=	0x00400000,		// show npc's viewcone
	OVERLAY_NPC_KILL_BIT		=	0x00800000,		// kill the NPC, running all appropriate AI.

	OVERLAY_WC_CHANGE_ENTITY	=	0x01000000,		// object changed during WC edit
	OVERLAY_BUDDHA_MODE			=	0x02000000,		// take damage but don't die

	OVERLAY_NPC_STEERING_REGULATIONS	=	0x04000000,	// Show the steering regulations associated with the NPC

	OVERLAY_TASK_TEXT_BIT		=	0x08000000,		// take damage but don't die

};

struct TimedOverlay_t;

/* =========  CBaseEntity  ======== 

  All objects in the game are derived from this.

a list of all CBaseEntitys is kept in gEntList
================================ */

// creates an entity by string name, but does not spawn it
extern CBaseEntity *CreateEntityByName( const char *className );

// creates an entity and calls all the necessary spawn functions
extern void SpawnEntityByName( const char *className, CEntityMapData *mapData = NULL );

// calls the spawn functions for an entity
extern int DispatchSpawn( CBaseEntity *pEntity );

inline CBaseEntity *GetContainingEntity( edict_t *pent );

//-----------------------------------------------------------------------------
// Purpose: think contexts
//-----------------------------------------------------------------------------
struct thinkfunc_t
{
	const char	*m_pszContext;
	BASEPTR m_pfnThink;
	int		m_nNextThinkTick;
	int		m_nLastThinkTick;

	DECLARE_SIMPLE_DATADESC();
};

struct rotatingpushmove_t;

#define CREATE_PREDICTED_ENTITY( className )	\
	CBaseEntity::CreatePredictedEntityByName( className, __FILE__, __LINE__ );

//
// Base Entity.  All entity types derive from this
//
class CBaseEntity : public IServerEntity
{
	friend class CBaseTransmitProxy;

public:
	DECLARE_CLASS_NOBASE( CBaseEntity );	

	//----------------------------------------
	// Class vars and functions
	//----------------------------------------
	static inline void Debug_Pause(bool bPause);
	static inline bool Debug_IsPaused(void);
	static inline void Debug_SetSteps(int nSteps);
	static inline bool Debug_ShouldStep(void);
	static inline bool Debug_Step(void);

	static bool				m_bInDebugSelect;
	static int				m_nDebugPlayer;

protected:

	static bool				m_bDebugPause;		// Whether entity i/o is paused for debugging.
	static int				m_nDebugSteps;		// Number of entity outputs to fire before pausing again.

public:
	// If bServerOnly is true, then the ent never goes to the client. This is used
	// by logical entities.
	CBaseEntity( bool bServerOnly=false );
	virtual ~CBaseEntity();

	// prediction system
	DECLARE_PREDICTABLE();
	// network data
	DECLARE_SERVERCLASS();
	// data description
	DECLARE_DATADESC();
	
	// memory handling
    void *operator new( size_t stAllocateBlock );
    void *operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine );
	void operator delete( void *pMem );

	// Class factory
	static CBaseEntity				*CreatePredictedEntityByName( const char *classname, const char *module, int line, bool persist = false );


// IHandleEntity overrides.
public:
	virtual void			SetRefEHandle( const CBaseHandle &handle );
	virtual const			CBaseHandle& GetRefEHandle() const;


// IServerNetworkable overrides.
public:
	virtual int				GetEFlags() const;
	virtual void			SetEFlags( int iEFlags );
	virtual	edict_t			*GetEdict() const;
	virtual void			CheckTransmit( CCheckTransmitInfo *pInfo );
	virtual EntityChange_t	DetectNetworkStateChanges();
	virtual void			ResetNetworkStateChanges();
	virtual CBaseNetworkable* GetBaseNetworkable();
	virtual CBaseEntity*	GetBaseEntity();


// IServerEntity overrides.
public:
	// Gets the interface to the collideable representation of the entity
	virtual ICollideable	*GetCollideable();
	virtual void			SetObjectCollisionBox();
	virtual void			CalcAbsolutePosition();
	virtual void			SetCheckUntouch( bool check );
	virtual bool			IsCurrentlyTouching( void ) const;
	virtual void			SetSentLastFrame( bool wassent );
 	virtual const Vector&	GetAbsOrigin( void ) const;
	virtual const QAngle&	GetAbsAngles( void ) const;
	virtual const Vector&	GetAbsMins( void ) const;
	virtual const Vector&	GetAbsMaxs( void ) const;
	virtual void			SetModelIndex( int index );
	virtual int				GetModelIndex( void ) const;
 	virtual string_t		GetModelName( void ) const;

public:
	// virtual methods for derived classes to override
	virtual bool			TestCollision( const Ray_t& ray, unsigned int mask, trace_t& trace );
	virtual	bool			TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	// non-virtual methods. Don't override these!
public:
	SolidType_t				GetSolid() const;
	int			 			GetSolidFlags( void ) const;

	void					AddEFlags( int nEFlagMask );
	void					RemoveEFlags( int nEFlagMask );
	bool					IsEFlagSet( int nEFlagMask ) const;

	bool					IsTransparent() const;

	// Called by physics to see if we should avoid a collision test....
	bool					ShouldCollide( int collisionGroup, int contentsMask ) const;

	// Move type / move collide
	MoveType_t				GetMoveType() const;
	MoveCollide_t			GetMoveCollide() const;
	void					SetMoveType( MoveType_t val, MoveCollide_t moveCollide = MOVECOLLIDE_DEFAULT );
	void					SetMoveCollide( MoveCollide_t val );

	// utility function to test against a specific box in world space
	bool					TestCollisionBox( const Ray_t& ray, unsigned int mask, trace_t& trace, const Vector &absmins, const Vector &absmaxs );

	// Returns the entity-to-world transform
	matrix3x4_t				&EntityToWorldTransform();
	const matrix3x4_t		&EntityToWorldTransform() const;

	// Some helper methods that transform a point from entity space to world space + back
	void					EntityToWorldSpace( const Vector &in, Vector *pOut ) const;
	void					WorldToEntitySpace( const Vector &in, Vector *pOut ) const;

	// Transforms an AABB measured in entity space to a box that surrounds it in world space
	// (so, the world-space box will have larger volume than the entity-space box)
	// Also added a version that goes the other way (from world to entity. Again, the box will grow in volume)
	void					EntityAABBToWorldAABB( const Vector &entityMins, const Vector &entityMaxs, Vector *pWorldMins, Vector *pWorldMaxs ) const;
	void					WorldAABBToEntityAABB( const Vector &worldMins, const Vector &worldMaxs, Vector *pEntityMins, Vector *pEntityMaxs ) const;

	// Returns a world-space AABB surrounding the collision representation.
	// NOTE: This may *not* be tightly fitting. For entities that define
	// their collision representation in entity space, this AABB will surround the entity space box
	void					WorldSpaceAABB( Vector *pWorldMins, Vector *pWorldMaxs ) const;

	// This function gets your parent's transform. If you're parented to an attachment,
	// this calculates the attachment's transform and gives you that.
	//
	// You must pass in tempMatrix for scratch space - it may need to fill that in and return it instead of 
	// pointing you right at a variable in your parent.
	matrix3x4_t&			GetParentToWorldTransform( matrix3x4_t &tempMatrix );

	// You can use this to override any entity's ShouldTransmit behavior.
	void					SetTransmitProxy( CBaseTransmitProxy *pProxy );

	// virtual methods; you can override these
public:
	// Owner entity.
	// FIXME: These are virtual only because of the logical point entity
	virtual CBaseEntity		*GetOwnerEntity() const;
	virtual void			SetOwnerEntity( CBaseEntity* pOwner );

	// Only CBaseEntity implements these. CheckTransmit calls the virtual ShouldTransmit to see if the
	// entity wants to be sent. If so, it calls SetTransmit, which will mark any dependents for transmission too.
	virtual bool			ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );

	// This marks the entity for transmission and passes the SetTransmit call to any dependents.
	virtual void			SetTransmit( CCheckTransmitInfo *pInfo );

	// This function finds out if the entity is in the 3D skybox. If so, it sets the EFL_IN_SKYBOX
	// flag so the entity gets transmitted to all the clients.
	// Entities usually call this during their Activate().
	// Returns true if the entity is in the skybox (and EFL_IN_SKYBOX was set).
	bool					DetectInSkybox();

	bool					IsSimulatedEveryTick() const;
	bool					IsAnimatedEveryTick() const;
	void					SetSimulatedEveryTick( bool sim );
	void					SetAnimatedEveryTick( bool anim );

public:

	// returns a pointer to the entities edict, if it has one.  should be removed!
	inline edict_t			*edict( void ) { return pev; }
	inline const edict_t	*edict( void ) const { return pev; }
	inline int				entindex( ) { return ENTINDEX( pev ); };

	// These methods encapsulate MOVETYPE_FOLLOW, which became obsolete
	void FollowEntity( CBaseEntity *pBaseEntity );
	void StopFollowingEntity( );	// will also change to MOVETYPE_NONE
	bool IsFollowingEntity();
	CBaseEntity *GetFollowedEntity();

	virtual void AttachEdict( edict_t *pRequiredEdict = NULL );
	virtual void DetachEdict( void );

	// initialization
	virtual void Spawn( void );
	virtual void Precache( void ) {}

	virtual void SetModel( const char *szModelName );

	virtual void PostConstructor( const char *szClassname );
	virtual void ParseMapData( CEntityMapData *mapData );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual bool KeyValue( const char *szKeyName, float flValue );
	virtual bool KeyValue( const char *szKeyName, Vector vec );

	// Activate - called for each entity after each load game and level load
	virtual void Activate( void );

	// Called once per frame after the server frame loop has finished and after all messages being
	//  sent to clients have been sent.
	virtual void PostClientMessagesSent( void );

	// Hierarchy traversal
	CBaseEntity *GetMoveParent( void );
	CBaseEntity *FirstMoveChild( void );
	CBaseEntity *NextMovePeer( void );

	void		SetName( string_t newTarget );
	void		SetParent( string_t newParent, CBaseEntity *pActivator );
	
	// Set the movement parent. Your local origin and angles will become relative to this parent.
	// If iAttachment is a valid attachment on the parent, then your local origin and angles 
	// are relative to the attachment on this entity.
	void		SetParent( const CBaseEntity* pNewParent, int iAttachment = 0 );
	CBaseEntity* GetParent();

	string_t	GetEntityName();

	bool		NameMatches( const char *pszNameOrWildcard );
	bool		ClassMatches( const char *pszClassOrWildcard );

	int			GetSpawnFlags( void ) const;
	void		AddSpawnFlags( int nFlags );
	void		RemoveSpawnFlags( int nFlags );
	void		ClearSpawnFlags( void );
	bool		HasSpawnFlags( int nFlags ) const;

	// makes the entity inactive
	void		MakeDormant( void );
	int			IsDormant( void );

	// checks to see if the entity is marked for deletion
	bool		IsMarkedForDeletion( void );

	// capabilities
	virtual int	ObjectCaps( void );

	// Verifies that the data description is valid in debug builds.
	#ifdef _DEBUG
	void ValidateDataDescription(void);
	#endif // _DEBUG

	// handles an input (usually caused by outputs)
	// returns true if the the value in the pass in should be set, false if the input is to be ignored
	virtual bool AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID );

	// 
	// Network optimization
	//
	
	// All these functions call through to CNetStateMgr. 
	// See CNetStateMgr for details about these functions.
	void	NetworkStateSetUpdateInterval( float N )	{ m_NetStateMgr.SetUpdateInterval( N ); }
	void	NetworkStateForceUpdate()					{ m_NetStateMgr.StateChanged(); }
	void	NetworkStateManualMode( bool activate )		{ m_NetStateMgr.EnableManualMode( activate ); }
	void	NetworkStateChanged()						{ m_NetStateMgr.StateChanged(); }
	bool	IsUsingNetworkManualMode()					{ return m_NetStateMgr.IsUsingManualMode(); }

	//
	// Input handlers.
	//
	void InputAlpha( inputdata_t &inputdata );
	void InputColor( inputdata_t &inputdata );
	void InputSetParent( inputdata_t &inputdata );
	void InputClearParent( inputdata_t &inputdata );
	void InputSetTeam( inputdata_t &inputdata );
	void InputUse( inputdata_t &inputdata );
	void InputKill( inputdata_t &inputdata );
	void InputSetDamageFilter( inputdata_t &inputdata );
	void InputDispatchEffect( inputdata_t &inputdata );
	void InputAddContext( inputdata_t &inputdata );
	void InputRemoveContext( inputdata_t &inputdata );
	void InputClearContext( inputdata_t &inputdata );
	void InputDispatchResponse( inputdata_t& inputdata );

	// Returns the origin at which to play an inputted dispatcheffect 
	virtual void GetInputDispatchEffectPosition( const char *sInputString, Vector &pOrigin, QAngle &pAngles );

	// tries to read a field from the entities data description - result is placed in variant_t
	virtual bool ReadKeyField( const char *varName, variant_t *var );

	// classname access
	void		SetClassname( const char *className );
	const char* GetClassname();

	// Debug Overlays
	const char	*GetDebugName(void);
	virtual	void DrawDebugGeometryOverlays(void);					
	virtual int  DrawDebugTextOverlays(void);
	void		 DrawTimedOverlays( void );
	void		 DrawBBoxOverlay( void );
	void		 DrawAbsBoxOverlay();
	void		 DrawRBoxOverlay();

	void		 DrawInputOverlay(const char *szInputName, CBaseEntity *pCaller, variant_t Value);
	void		 DrawOutputOverlay(CEventAction *ev);
	void		 SendDebugPivotOverlay( void );
	void		 AddTimedOverlay( const char *msg, int endTime );

	void		SetSolid( SolidType_t val );

	// save/restore
	// only overload these if you have special data to serialize
	virtual int	Save( ISave &save );
	virtual int	Restore( IRestore &restore );

	int			 GetTextureFrameIndex( void );
	void		 SetTextureFrameIndex( int iIndex );

private:
	int SaveDataDescBlock( ISave &save, datamap_t *dmap );
	int RestoreDataDescBlock( IRestore &restore, datamap_t *dmap );
public:

	virtual bool ShouldSavePhysics();

	// handler to reset stuff before you are restored
	// NOTE: Always chain to base class when implementing this!
	virtual void OnSave();

	// handler to reset stuff after you are restored
	// called after all entities have been loaded from all affected levels
	// called before activate
	// NOTE: Always chain to base class when implementing this!
	virtual void OnRestore();

	// returns the edict index the entity requires when used in save/restore (eg players, world)
	// -1 means it doesn't require any special index
	virtual int RequiredEdictIndex( void ) { return -1; } 

	// interface function pts
	void (CBaseEntity::*m_pfnMoveDone)(void);
	virtual void MoveDone( void ) { if (m_pfnMoveDone) (this->*m_pfnMoveDone)();};

	// Why do we have two separate static Instance functions?
	static CBaseEntity *Instance( const CBaseHandle &hEnt );
	static CBaseEntity *Instance( const edict_t *pent );
	static CBaseEntity *Instance( edict_t *pent );
	static CBaseEntity* Instance( int iEnt );

	// Think function handling
	void (CBaseEntity::*m_pfnThink)(void);
	virtual void Think( void ) { if (m_pfnThink) (this->*m_pfnThink)();};

	// Think functions with contexts
	int		RegisterThinkContext( const char *szContext );
	BASEPTR	ThinkSet( BASEPTR func, float flNextThinkTime = 0, const char *szContext = NULL );
	void	SetNextThink( float nextThinkTime, const char *szContext = NULL );
	float	GetNextThink( char *szContext = NULL );
	float	GetLastThink( char *szContext = NULL );
	int		GetNextThinkTick( char *szContext = NULL );
	int		GetLastThinkTick( char *szContext = NULL );

	float				GetAnimTime() const;
	void				SetAnimTime( float at );

	float				GetSimulationTime() const;
	void				SetSimulationTime( float st );

	static void			AddToUpdateClientDataList( CBaseEntity *entity );
	static void			RemoveFromUpdateClientDataList( CBaseEntity *entity );
	static void			PerformUpdateClientData( CBasePlayer *player );


	virtual void			OnUpdateClientData( CBasePlayer *player ) { }

	// members
	string_t m_iClassname;  // identifier for entity creation and save/restore
	string_t m_iGlobalname; // identifier for carrying entity across level transitions
	string_t m_iParent;	// the name of the entities parent; linked into m_pParent during Activate()

public:
	// was pev->speed
	float		m_flSpeed;
	// was pev->renderfx
	CNetworkVar( int, m_nRenderFX );
	// was pev->rendermode
	CNetworkVar( int, m_nRenderMode );

	// was pev->animtime:  consider moving to CBaseAnimating
	float		m_flPrevAnimTime;
	CNetworkVar( float, m_flAnimTime );  // this is the point in time that the client will interpolate to position,angle,frame,etc.
	CNetworkVar( float, m_flSimulationTime );

	int				m_nLastThinkTick;

	// was pev->effects
	CNetworkVar( int, m_fEffects );

	// was pev->rendercolor
	CNetworkColor32( m_clrRender );
	const color32 GetRenderColor() const;
	void SetRenderColor( byte r, byte g, byte b );
	void SetRenderColor( byte r, byte g, byte b, byte a );
	void SetRenderColorR( byte r );
	void SetRenderColorG( byte g );
	void SetRenderColorB( byte b );
	void SetRenderColorA( byte a );

	CNetworkVar( int, m_nModelIndex );

	// Certain entities (projectiles) can be created on the client and thus need a matching id number
	CNetworkVar( CPredictableId, m_PredictableID );

	// pointer to a list of edicts that this entity is in contact with
	touchlink_t m_EntitiesTouched;		

	// used so we know when things are no longer touching
	int			touchStamp;			

protected:

	CNetStateMgr	m_NetStateMgr;

	// think function handling
	enum thinkmethods_t
	{
		THINK_FIRE_ALL_FUNCTIONS,
		THINK_FIRE_BASE_ONLY,
		THINK_FIRE_ALL_BUT_BASE,
	};
	int		GetIndexForContext( const char *pszContext );
	CUtlVector< thinkfunc_t >	m_aThinkFunctions;
	int		m_iCurrentThinkContext;

	int GetContextCount() const;
	const char *GetContextName( int index ) const;
	const char *GetContextValue( int index ) const;
	int FindContextByName( const char *name ) const;
	void	AddContext( const char *nameandvalue );

	CUtlVector< ResponseContext_t > m_ResponseContexts;

	// Map defined context sets
	string_t	m_iszContext;

	IResponseSystem *m_pInstancedResponseSystem;

private:
	CBaseEntity( CBaseEntity& );

	// list handling
	friend class CGlobalEntityList;
	friend class CThinkSyncTester;

	// was pev->nextthink
	CNetworkVarForDerived( int, m_nNextThinkTick );

////////////////////////////////////////////////////////////////////////////


public:
	// Default PVS check ( checks pev against the pvs for sending to pPlayer )
	bool		IsInPVS( const edict_t *pPlayer, const void *pvs );

	// binoculars wedge !!!TESTTEST
	// Binoculars use this function to determine whether the entities in the PVS
	// are potential sources of sound.
	virtual bool IsSoundSource( void ) { return false; };

	// Returns a CBaseAnimating if the entity is derived from CBaseAnimating.
	virtual CBaseAnimating*	GetBaseAnimating() { return 0; }

	virtual IResponseSystem *GetResponseSystem();
	void	InstallCustomResponseSystem( const char *scriptfile );
	virtual void	DispatchResponse( const char *conceptName );

// Classify - returns the type of group (i.e, "houndeye", or "human military" so that NPCs with different classnames
// still realize that they are teammates. (overridden for NPCs that form groups)
	virtual Class_T Classify ( void );
	virtual void	DeathNotice ( CBaseEntity *pVictim ) {}// NPC maker children use this to tell the NPC maker that they have died.

	// Call this to do a TraceAttack on an entity, performs filtering. Don't call TraceAttack() directly except when chaining up to base class
	void			DispatchTraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual bool	PassesDamageFilter( CBaseEntity *pAttacker );


protected:
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

public:

	// returns the amount of damage inflicted
	// LEGACY CODE. Override the TakeDamage_ functions to implement damage the new way.
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	// This is what you should call to apply damage to an entity.
	void TakeDamage( const CTakeDamageInfo &info );

	virtual int		TakeHealth( float flHealth, int bitsDamageType );

	// Entity killed (only fired once)
	virtual void	Event_Killed( const CTakeDamageInfo &info );

	virtual int				BloodColor( void ) { return DONT_BLEED; }
	virtual void			TraceBleed( float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType );
	virtual bool			IsTriggered( CBaseEntity *pActivator ) {return true;}
	virtual CAI_BaseNPC*	MyNPCPointer( void ) { return NULL;}
	virtual CBaseCombatCharacter *MyCombatCharacterPointer( void ) { return NULL; }
	virtual void			AddPoints( int score, bool bAllowNegativeScore ) {}
	virtual void			AddPointsToTeam( int score, bool bAllowNegativeScore ) {}
	virtual float			GetDelay( void ) { return 0; }
	virtual int				IsMoving( void ) { return m_vecVelocity != vec3_origin; }
	virtual char const		*DamageDecal( int bitsDamageType, int gameMaterial );
	virtual void			DecalTrace( trace_t *pTrace, char const *decalName );
	virtual void			ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName = NULL );

	void			RemoveAllDecals( void );

	// This is ONLY used by the node graph to test movement through a door
	virtual void	SetToggleState( int state ) {}
	virtual bool	OnControls( CBaseEntity *pControls ) { return false; }
	virtual bool	IsAlive( void );
	virtual bool	HasTarget( string_t targetname );
	virtual	bool	IsPlayer( void ) const { return false; }
	virtual bool	IsNetClient( void ) { return false; }
	virtual bool	IsTemplate( void ) { return false; }
	virtual bool	IsBaseObject( void ) const { return false; }
	bool			IsBSPModel() const;
	bool			IsInWorld( void ) const;

	// If this is a vehicle, returns the vehicle interface
	virtual IServerVehicle*			GetServerVehicle() { return NULL; }

	virtual bool	IsViewable( void );					// is this something that would be looked at (model, sprite, etc.)?

	// Team Handling
	CTeam			*GetTeam( void ) const;				// Get the Team this entity is on
	int				GetTeamNumber( void ) const;		// Get the Team number of the team this entity is on
	virtual void	ChangeTeam( int iTeamNum );			// Assign this entity to a team.
	bool			IsInTeam( CTeam *pTeam ) const;		// Returns true if this entity's in the specified team
	bool			InSameTeam( CBaseEntity *pEntity ) const;	// Returns true if the specified entity is on the same team as this one
	bool			IsInAnyTeam( void ) const;			// Returns true if this entity is in any team
	const char		*TeamID( void ) const;				// Returns the name of the team this entity is on.

	// can stand on this entity?
	bool IsStandable() const;

	virtual bool	CanStandOn( CBaseEntity *pSurface ) const { return (pSurface && !pSurface->IsStandable()) ? false : true; }
	virtual bool	CanStandOn( edict_t	*ent ) const { return CanStandOn( GetContainingEntity( ent ) ); }

	virtual CBaseEntity		*GetEnemy( void ) { return NULL; }


	virtual void	ViewPunch( const QAngle &angleOffset ) {}
	virtual void	VelocityPunch( const Vector &vecForce ) {}

	virtual CBaseEntity *GetNextTarget( void );
	
	// fundamental callbacks
	void (CBaseEntity ::*m_pfnTouch)( CBaseEntity *pOther );
	void (CBaseEntity ::*m_pfnUse)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void (CBaseEntity ::*m_pfnBlocked)( CBaseEntity *pOther );

	virtual void			Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void			StartTouch( CBaseEntity *pOther );
	virtual void			Touch( CBaseEntity *pOther ); 
	virtual void			EndTouch( CBaseEntity *pOther );
	virtual void			StartBlocked( CBaseEntity *pOther ) {}
	virtual void			Blocked( CBaseEntity *pOther );
	virtual void			EndBlocked( void ) {}

	// Physics simulation
	virtual void			PhysicsSimulate( void );

public:
	// HACKHACK:Get the trace_t from the last physics touch call (replaces the even-hackier global trace vars)
	static trace_t			GetTouchTrace( void );

	// FIXME: Should be private, but I can't make em private just yet
	void					PhysicsImpact( CBaseEntity *other, trace_t &trace );
 	void					PhysicsMarkEntitiesAsTouching( CBaseEntity *other, trace_t &trace );
	void					PhysicsMarkEntitiesAsTouchingEventDriven( CBaseEntity *other, trace_t &trace );

	// Physics helper
	static void				PhysicsRemoveTouchedList( CBaseEntity *ent );
	static void				PhysicsNotifyOtherOfUntouch( CBaseEntity *ent, CBaseEntity *other );
	static void				PhysicsRemoveToucher( CBaseEntity *other, touchlink_t *link );

	virtual void			UpdateOnRemove( void );

	// common member functions
	void					SUB_Remove( void );
	void					SUB_DoNothing( void );
	void					SUB_StartFadeOut ( void );
	void					SUB_FadeOut ( void );
	void					SUB_Vanish( void );
	void					SUB_CallUseToggle( void ) { this->Use( this, this, USE_TOGGLE, 0 ); }

	// change position, velocity, orientation instantly
	// passing NULL means no change
	virtual void			Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
	// notify that another entity (that you were watching) was teleported
	virtual void			NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params );

	int						ShouldToggle( USE_TYPE useType, int currentState );

	// Computes a box that surrounds all rotations of the entity
	void ComputeSurroundingBox( void );

	virtual void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	virtual void FireBullets( int cShots, const Vector &vecSrc, const Vector &vecDirShooting,	const Vector &vecSpread, float flDistance, int iBulletType, int iTracerFreq = 4, int firingEntID = -1, int attachmentID = -1,int iDamage = 0, CBaseEntity *pAttacker = NULL, bool bFirstShotAccurate = false );
	virtual void DoCustomImpactEffect( const trace_t &tr ) { }; // give shooter a chance to do a custom impact.


	virtual CBaseEntity *Respawn( void ) { return NULL; }

	// Method used to deal with attacks passing through triggers
	void TraceAttackToTriggers( const CTakeDamageInfo &info, const Vector& start, const Vector& end, const Vector& dir );

	// Do the bounding boxes of these two intersect?
	int		Intersects( CBaseEntity *pOther );
	virtual bool IsLockedByMaster( void ) { return false; }

	// Health accessors.
	int		GetMaxHealth()  const	{ return m_iMaxHealth; }
	void	SetMaxHealth( int amt )	{ m_iMaxHealth = amt; }

	int		GetHealth() const		{ return m_iHealth; }
	void	SetHealth( int amt )	{ m_iHealth = amt; }

	// Ugly code to lookup all functions to make sure they are in the table when set.
#ifdef _DEBUG
	void FunctionCheck( void *pFunction, char *name );

	ENTITYFUNCPTR TouchSet( ENTITYFUNCPTR func, char *name ) 
	{ 
		COMPILE_TIME_ASSERT( sizeof(func) == 4 );
		m_pfnTouch = func; 
		FunctionCheck( (void *)*((int *)((char *)this + ( offsetof(CBaseEntity,m_pfnTouch)))), name ); 
		return func;
	}
	USEPTR	UseSet( USEPTR func, char *name ) 
	{ 
		COMPILE_TIME_ASSERT( sizeof(func) == 4 );
		m_pfnUse = func; 
		FunctionCheck( (void *)*((int *)((char *)this + ( offsetof(CBaseEntity,m_pfnUse)))), name ); 
		return func;
	}
	ENTITYFUNCPTR	BlockedSet( ENTITYFUNCPTR func, char *name ) 
	{ 
		COMPILE_TIME_ASSERT( sizeof(func) == 4 );
		m_pfnBlocked = func; 
		FunctionCheck( (void *)*((int *)((char *)this + ( offsetof(CBaseEntity,m_pfnBlocked)))), name ); 
		return func;
	}

#endif
	virtual void	 ModifyOrAppendCriteria( AI_CriteriaSet& set );
	void	 AppendContextToCriteria( AI_CriteriaSet& set, const char *prefix = "" );
	
	// old CBaseEntity stuff
	edict_t *pev;

private:
	friend class CAI_Senses;
	CBaseEntity	*m_pLink;// used for temporary link-list operations. 

public:

	// variables promoted from edict_t
	CNetworkVarForDerived( int, m_lifeState );
	string_t	m_target;

	CNetworkVarForDerived( int, m_takedamage );
	CNetworkVarForDerived( int, m_iMaxHealth ); // CBaseEntity doesn't care about changes to this variable, but there are derived classes that do.
	CNetworkVarForDerived( int, m_iHealth );

	// Damage filtering
	string_t	m_iszDamageFilterName;	// The name of the entity to use as our damage filter.
	EHANDLE		m_hDamageFilter;		// The entity that controls who can damage us.

	// Debugging / devolopment fields
	int				m_debugOverlays;	// For debug only (bitfields)
	TimedOverlay_t*	m_pTimedOverlay;	// For debug only

	// virtual functions used by a few classes
	
	// creates an entity of a specified class, by name
	static CBaseEntity *Create( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );
	static CBaseEntity *CreateNoSpawn( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );

	// Collision group accessors
	int				GetCollisionGroup() const;
	void			SetCollisionGroup( int collisionGroup );

	// Damage accessors
	virtual int		GetDamageType() const;
	virtual float	GetDamage() { return 0; }
	virtual void	SetDamage(float flDamage) {}

	virtual Vector	EyePosition( );				// position of eyes
	virtual const QAngle &EyeAngles();			// Direction of eyes in world space
	virtual const QAngle &LocalEyeAngles();		// Direction of eyes
	virtual Vector	EarPosition( );					// position of ears
	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true);		// position to shoot at
	virtual void	GetVectors(Vector* forward, Vector* right, Vector* up) const;

	const Vector&	GetViewOffset() const { return m_vecViewOffset.Get(); }
	void			SetViewOffset( const Vector &vecOffset );

	// NOTE: Setting the abs velocity in either space will cause a recomputation
	// in the other space, so setting the abs velocity will also set the local vel
	void			SetLocalVelocity( const Vector &vecVelocity );
	void			ApplyLocalVelocityImpulse( const Vector &vecImpulse );
	void			SetAbsVelocity( const Vector &vecVelocity );
	void			ApplyAbsVelocityImpulse( const Vector &vecImpulse );

	const Vector&	GetLocalVelocity( ) const;
	const Vector&	GetAbsVelocity( ) const;

	// NOTE: Setting the abs velocity in either space will cause a recomputation
	// in the other space, so setting the abs velocity will also set the local vel
	void			SetLocalAngularVelocity( const QAngle &vecAngVelocity );
	const QAngle&	GetLocalAngularVelocity( ) const;

	// FIXME: While we're using (dPitch, dYaw, dRoll) as our local angular velocity
	// representation, we can't actually solve this problem
//	void			SetAbsAngularVelocity( const QAngle &vecAngVelocity );
//	const QAngle&	GetAbsAngularVelocity( ) const;

	const Vector&	GetBaseVelocity() const;
	void			SetBaseVelocity( const Vector& v );

	virtual Vector	GetSmoothedVelocity( void );

	// FIXME: Figure out what to do about this
	virtual void	GetVelocity(Vector *vVelocity, QAngle *vAngVelocity);

	float			GetGravity( void ) const;
	void			SetGravity( float gravity );
	float			GetFriction( void ) const;
	void			SetFriction( float flFriction );

	virtual	bool FVisible ( CBaseEntity *pEntity, int traceMask = MASK_OPAQUE, CBaseEntity **ppBlocker = NULL );
	virtual bool FVisible( const Vector &vecTarget, int traceMask = MASK_OPAQUE, CBaseEntity **ppBlocker = NULL );

	virtual bool CanBeSeen() { return true; } // allows entities to be 'invisible' to NPC senses.

	// This function returns a value that scales all damage done by this entity.
	// Use CDamageModifier to hook in damage modifiers on a guy.
	virtual float			GetAttackDamageScale( void );
	// This returns a value that scales all damage done to this entity
	// Use CDamageModifier to hook in damage modifiers on a guy.
	virtual float			GetReceivedDamageScale( void );

	// Sorry folks, here lies TF2-specific stuff that really has no other place to go
	virtual bool			CanBePoweredUp( void ) { return false; }
	virtual bool			AttemptToPowerup( int iPowerup, float flTime, float flAmount = 0, CBaseEntity *pAttacker = NULL, CDamageModifier *pDamageModifier = NULL ) { return false; }

	bool					GetCheckUntouch() const;

	bool					GetSentLastFrame( void ) const;

	void					SetGroundEntity( CBaseEntity *ground );
	CBaseEntity				*GetGroundEntity( void );

	// Gets the velocity we impart to a player standing on us
	virtual void			GetGroundVelocityToApply( Vector &vecGroundVel ) { vecGroundVel = vec3_origin; }

	int						GetWaterLevel() const;
	void					SetWaterLevel( int nLevel );
	int						GetWaterType() const;
	void					SetWaterType( int nType );

	void					ClearSolidFlags( void );	
	void					RemoveSolidFlags( int flags );
	void					AddSolidFlags( int flags );
	bool					IsSolidFlagSet( int flagMask ) const;
	void				 	SetSolidFlags( int flags );
	bool					IsSolid() const;
	
	void					SetModelName( string_t name );

	model_t					*GetModel( void );

	// These methods return a *world-aligned* box relative to the absorigin of the entity.
	// This is used for collision purposes and is *not* guaranteed
	// to surround the entire entity's visual representation
	// NOTE: It is illegal to ask for the world-aligned bounds for
	// SOLID_BSP objects
	virtual const Vector&	WorldAlignMins( ) const;
	virtual const Vector&	WorldAlignMaxs( ) const;

	// These methods return a box defined in the space of the entity
	// This OBB is used for collision purposes.
	// NOTE: This OBB is *not* guaranteed to surround the entire thing;
	// It's just used for a collision representation of the entity.
	// NOTE: For brush models, this OBB is guaranteed to be exact.
	// NOTE: It is illegal to ask for the entity-space bounds for
	// SOLID_BBOX objects
	virtual const Vector&	EntitySpaceMins( ) const;
	virtual const Vector&	EntitySpaceMaxs( ) const;

	// This defines collision bounds *in whatever space is currently defined by the solid type*
	//	SOLID_BBOX:		World Align
	//	SOLID_OBB:		Entity space
	//	SOLID_BSP:		Entity space
	//	SOLID_VPHYSICS	Not used
	void					SetCollisionBounds( const Vector& mins, const Vector &maxs );
	bool					IsBoundsDefinedInEntitySpace() const;

	// NOTE: These use the collision OBB to compute a reasonable center point for the entity
	const Vector&			EntitySpaceCenter( ) const;
	virtual const Vector&	WorldSpaceCenter( ) const;

	// FIXME: Do we want this?
	const Vector&			WorldAlignSize( ) const;
	const Vector&			EntitySpaceSize( ) const;
	bool					IsPointSized() const;

	// A box that is guaranteed to surround the entity.
	// It may not be particularly tight-fitting depending on performance-related questions
	// It may also fluctuate in size at any time
	virtual const Vector&	EntitySpaceSurroundingMins() const;
	virtual const Vector&	EntitySpaceSurroundingMaxs() const;
	virtual const Vector&	WorldSpaceSurroundingMins() const;
	virtual const Vector&	WorldSpaceSurroundingMaxs() const;

	// Selects a random point in the bounds given the normalized 0-1 bounds 
	void					RandomPointInBounds( const Vector &vecNormalizedMins, const Vector &vecNormalizedMaxs, Vector *pPoint) const;

	void					SetAbsMins( const Vector& mins );
	void					SetAbsMaxs( const Vector& maxs );

	// NOTE: Setting the abs origin or angles will cause the local origin + angles to be set also
	void					SetAbsOrigin( const Vector& origin );
	void					SetAbsAngles( const QAngle& angles );

	// Origin and angles in local space ( relative to parent )
	// NOTE: Setting the local origin or angles will cause the abs origin + angles to be set also
	void					SetLocalOrigin( const Vector& origin );
	const Vector&			GetLocalOrigin( void ) const;

	void					SetLocalAngles( const QAngle& angles );
	const QAngle&			GetLocalAngles( void ) const;

	void					SetElasticity( float flElasticity );
	float					GetElasticity( void ) const;

	void					SetShadowCastDistance( float flDistance );
	float					GetShadowCastDistance( void )	const;

	float					GetLocalTime( void ) const;
	void					IncrementLocalTime( float flTimeDelta );
	float					GetMoveDoneTime( ) const;
	void					SetMoveDoneTime( float flTime );
	
	// Used by the PAS filters to ask the entity where in world space the sounds it emits come from.
	// This is used right now because if you have something sitting on an incline, using our axis-aligned 
	// bounding boxes can return a position in solid space, so you won't hear sounds emitted by the object.
	// For now, we're hacking around it by moving the sound emission origin up on certain objects like vehicles.
	//
	// When OBBs get in, this can probably go away.
	Vector					GetSoundEmissionOrigin() const;

	void					AddFlag( int flags );
	void					RemoveFlag( int flagsToRemove );
	void					ToggleFlag( int flagToToggle );
	int						GetFlags( void ) const;
	void					ClearFlags( void );

	// Sets the local position from a transform
	void					SetLocalTransform( const matrix3x4_t &localTransform );

	// Relink into the spatial partition.
	void					Relink();

	// See CSoundEmitterSystem
	void					EmitSound( const char *soundname, float soundtime = 0.0f );  // Override for doing the general case of CPASAttenuationFilter filter( this ), and EmitSound( filter, entindex(), etc. );
	void					StopSound( const char *soundname );

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

	static void EmitAmbientSound( CBaseEntity *entity, const Vector& origin, const char *soundname, int flags = 0, float soundtime = 0.0f );

	// These files need to be listed in scripts/game_sounds_manifest.txt
	static void	PrecacheSoundScript( const char *scriptfile );
	static void PrecacheScriptSound( const char *soundname, bool preload = false );

public:

	// Called after spawn, and in the case of self-managing objects, after load
	virtual bool	CreateVPhysics();

	// Convenience routines to init the vphysics simulation for this object.
	// This creates a static object.  Something that behaves like world geometry - solid, but never moves
	IPhysicsObject *VPhysicsInitStatic( void );

	// This creates a normal vphysics simulated object - physics determines where it goes (gravity, friction, etc)
	// and the entity receives updates from vphysics.  SetAbsOrigin(), etc do not affect the object!
	IPhysicsObject *VPhysicsInitNormal( SolidType_t solidType, int nSolidFlags, bool createAsleep, solid_t *pSolid = NULL );

	// This creates a vphysics object with a shadow controller that follows the AI
	// Move the object to where it should be and call UpdatePhysicsShadowToCurrentPosition()
	IPhysicsObject *VPhysicsInitShadow( bool allowPhysicsMovement, bool allowPhysicsRotation, solid_t *pSolid = NULL );

	// Force a non-solid (ie. solid_trigger) physics object to collide with other entities.
	virtual bool	ForceVPhysicsCollide( CBaseEntity *pEntity ) { return false; }

private:
	// called by all vphysics inits
	bool			VPhysicsInitSetup();
public:

	void			VPhysicsSetObject( IPhysicsObject *pPhysics );
	// destroy and remove the physics object for this entity
	virtual void	VPhysicsDestroyObject( void );
	void			VPhysicsSwapObject( IPhysicsObject *pSwap );

	inline IPhysicsObject *VPhysicsGetObject( void ) const { return m_pPhysicsObject; }
	virtual void	VPhysicsUpdate( IPhysicsObject *pPhysics );
	void			VPhysicsUpdatePusher( IPhysicsObject *pPhysics );
	
	// react physically to damage (called from CBaseEntity::OnTakeDamage() by default)
	virtual int		VPhysicsTakeDamage( const CTakeDamageInfo &info );
	virtual void	VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void	VPhysicsShadowUpdate( IPhysicsObject *pPhysics ) {}
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void	VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit );
	
	// update the shadow so it will coincide with the current AI position at some time
	// in the future (or 0 for now)
	virtual void	UpdatePhysicsShadowToCurrentPosition( float deltaTime );
	virtual int		VPhysicsGetObjectList( IPhysicsObject **pList, int listMax );

public:
	// The player drives simulation of this entity
	void					SetPlayerSimulated( CBasePlayer *pOwner );
	void					UnsetPlayerSimulated( void );
	bool					IsPlayerSimulated( void ) const;
	CBasePlayer				*GetSimulatingPlayer( void );

	// FIXME: Make these private!
	void					PhysicsCheckForEntityUntouch( void );
 	bool					PhysicsRunThink( thinkmethods_t thinkMethod = THINK_FIRE_ALL_FUNCTIONS );
	bool					PhysicsRunSpecificThink( int nContextIndex, BASEPTR thinkFunc );
	bool					PhysicsTestEntityPosition( CBaseEntity **ppEntity = NULL );
	trace_t					PhysicsPushEntity( const Vector& push );
	bool					PhysicsCheckWater( void );
	bool					IsEdictFree() const { return pev->free != 0; }

	// Callbacks for the physgun/cannon picking up an entity
	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser ) {}
	virtual void OnPhysGunDrop( CBasePlayer *pPhysGunUser, bool wasLaunched ) {}

	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const;

	// Computes the abs position of a point specified in local space
	void					ComputeAbsPosition( const Vector &vecLocalPosition, Vector *pAbsPosition );

	// Computes the abs position of a direction specified in local space
	void					ComputeAbsDirection( const Vector &vecLocalDirection, Vector *pAbsDirection );

	void					SetPredictionEligible( bool canpredict );

protected:
	int						PhysicsClipVelocity (const Vector& in, const Vector& normal, Vector& out, float overbounce );

	// Which frame did I simulate?
	int						m_nSimulationTick;

private:
	// Physics-related private methods
	void					PhysicsStep( void );
	void					PhysicsPusher( void );
	void					PhysicsNone( void );
	void					PhysicsNoclip( void );
	void					PhysicsStepRunTimestep( float timestep );
	void					PhysicsToss( void );
	void					PhysicsCustom( void );
	void					PerformPush( float movetime );

	// Simulation in local space of rigid children
	void					PhysicsRigidChild( void );

	// Computes the base velocity
	void					UpdateBaseVelocity( void );

	// Implement this if you use MOVETYPE_CUSTOM
	virtual void			PerformCustomPhysics( Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity );

	void					PhysicsDispatchThink( BASEPTR thinkFunc );
	void					PhysicsRelinkChildren( void );

	touchlink_t				*PhysicsMarkEntityAsTouched( CBaseEntity *other );
	void					PhysicsTouch( CBaseEntity *pentOther );
	void					PhysicsStartTouch( CBaseEntity *pentOther );

	CBaseEntity				*PhysicsPushMove( float movetime );
	CBaseEntity				*PhysicsPushRotate( float movetime );

	CBaseEntity				*PhysicsCheckRotateMove( rotatingpushmove_t &rotPushmove, CBaseEntity **pPusherList, int pusherListCount );
	CBaseEntity				*PhysicsCheckPushMove( const Vector& move, CBaseEntity **pPusherList, int pusherListCount );
	int						PhysicsTryMove( float time, trace_t *steptrace );

	void					PhysicsCheckWaterTransition( void );
	void					PhysicsCheckVelocity( void );
	void					PhysicsAddHalfGravity( float timestep );
	void					PhysicsAddGravityMove( Vector &move );

	// Performs the collision resolution for fliers.
	void					PerformFlyCollisionResolution( trace_t &trace, Vector &move );
	void					ResolveFlyCollisionBounce( trace_t &trace, Vector &vecVelocity );
	void					ResolveFlyCollisionSlide( trace_t &trace, Vector &vecVelocity );
	void					ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );

	void					CalcAbsoluteVelocity();
	void					CalcAbsoluteAngularVelocity();

	// Computes the water level + type
	void					UpdateWaterState();

	// Checks a sweep without actually performing the move
	void					PhysicsCheckSweep( const Vector& vecAbsStart, const Vector &vecAbsDelta, trace_t *pTrace );

	// Invalidates the abs state of the entity and all children
	// Specify the second flag if you want additional flags applied to all children
	void					InvalidatePhysicsRecursive( int nDirtyFlag, int nAdditionalChildFlags = 0 );

	// Computes new angles based on the angular velocity
	void					SimulateAngles( float flFrameTime );

	friend class CPushBlockerEnum;

	// Sets/Gets the next think based on context index
	void SetNextThink( int nContextIndex, float thinkTime );
	void SetLastThink( int nContextIndex, float thinkTime );
	float GetNextThink( int nContextIndex ) const;
	int	GetNextThinkTick( int nContextIndex ) const;

	// Damage modifiers
	friend class CDamageModifier;
	CUtlLinkedList<CDamageModifier*,int>	m_DamageModifiers;

	EHANDLE m_pParent;  // for movement hierarchy
	CNetworkVar( unsigned char,  m_iParentAttachment ); // 0 if we're relative to the parent's absorigin and absangles.

	// Our immediate parent in the movement hierarchy.
	// FIXME: clarify m_pParent vs. m_pMoveParent
	CNetworkHandle( CBaseEntity, m_hMoveParent );
	// cached child list
	EHANDLE m_hMoveChild;	
	// generated from m_pMoveParent
	EHANDLE m_hMovePeer;	

	int		m_iEFlags;	// entity flags EFL_*

	string_t m_iName;	// name used to identify this entity

	CNetworkVarEmbedded( CCollisionProperty, m_Collision );

	CNetworkVar( MoveType_t, m_MoveType );		// One of the MOVETYPE_ defines.
	CNetworkVar( MoveCollide_t, m_MoveCollide );
	CNetworkHandle( CBaseEntity, m_hOwnerEntity );	// only used to point to an edict it won't collide with

	CNetworkVar( int, m_CollisionGroup );		// used to cull collision tests
	IPhysicsObject	*m_pPhysicsObject;	// pointer to the entity's physics object (vphysics.dll)

	CNetworkVar( float, m_flElasticity );

	CNetworkVar( float, m_flShadowCastDistance );

	// Team handling
	int			m_iInitialTeamNum;		// Team number of this entity's team read from file
	CNetworkVar( int, m_iTeamNum );				// Team number of this entity's team. 

	// Formerly in pev
	bool			m_bCheckUntouch;
	bool			m_bSentLastFrame;

	CNetworkHandleForDerived( CBaseEntity, m_hGroundEntity );	
	
	string_t		m_ModelName;
	Vector			m_vecAbsMins;
	Vector			m_vecAbsMaxs;

	// Velocity of the thing we're standing on (world space)
	Vector			m_vecBaseVelocity;

	// Global velocity
	Vector			m_vecAbsVelocity;

	// Local angular velocity
	QAngle			m_vecAngVelocity;

	// Global angular velocity
//	QAngle			m_vecAbsAngVelocity;

	// local coordinate frame of entity
	matrix3x4_t		m_rgflCoordinateFrame;

	// Physics state
	CNetworkVarForDerived( int, m_nWaterLevel );
	int				m_nWaterType;
	EHANDLE			m_pBlocker;

	// was pev->gravity;
	float			m_flGravity;  // rename to m_flGravityScale;
	// was pev->friction
	CNetworkVarForDerived( float, m_flFriction );

	// was pev->ltime
	float			m_flLocalTime;
	// local time at the beginning of this frame
	float			m_flVPhysicsUpdateLocalTime;
	// local time the movement has ended
	float			m_flMoveDoneTime;

	// A counter to help quickly build a list of potentially pushed objects for physics
	int				m_nPushEnumCount;

	Vector			m_vecAbsOrigin;
	CNetworkVectorForDerived( m_vecVelocity );
	
	//Adrian
	CNetworkVar( int, m_iTextureFrameIndex );
	
	CNetworkVar( bool, m_bSimulatedEveryTick );
	CNetworkVar( bool, m_bAnimatedEveryTick );

protected:
	// FIXME: Make this private! Still too many references to do so...
	CNetworkVar( int, m_spawnflags );

private:
	CBaseTransmitProxy *m_pTransmitProxy;

	QAngle			m_angAbsRotation;

	CNetworkVector( m_vecOrigin );
	CNetworkQAngle( m_angRotation );

	// was pev->view_ofs ( FIXME:  Move somewhere up the hierarch, CBaseAnimating, etc. )
	CNetworkVectorForDerived( m_vecViewOffset );

	// was pev->flags
	CNetworkVarForDerived( int, m_fFlags );

	CNetworkVar( bool, m_bIsPlayerSimulated );
	// Player who is driving my simulation
	CHandle< CBasePlayer >			m_hPlayerSimulationOwner;

	// So it can get at the physics methods
	friend class CCollisionEvent;

public:
	void							InitPredictable( void );
// Methods shared by client and server
public:
	void							SetSize( const Vector &vecMin, const Vector &vecMax ); // UTIL_SetSize( this, mins, maxs );
	int								PrecacheModel( const char *name ); // engine->PrecacheModel( name );
	int								PrecacheSound( const char *name ); // enginesound->PrecacheSound
	void							Remove( ); // UTIL_Remove( this );

private:

	// This is a random seed used by the networking code to allow client - side prediction code
	//  randon number generators to spit out the same random numbers on both sides for a particular
	//  usercmd input.
	static int						m_nPredictionRandomSeed;
	static CBasePlayer				*m_pPredictionPlayer;

	CBaseHandle m_RefEHandle;

	// FIXME: Make hierarchy a member of CBaseEntity
	// or a contained private class...
	friend void UnlinkChild( CBaseEntity *pParent, CBaseEntity *pChild );
	friend void LinkChild( CBaseEntity *pParent, CBaseEntity *pChild );
	friend void ClearParent( CBaseEntity *pEntity );
	friend void UnlinkAllChildren( CBaseEntity *pParent );

public:
	// Accessors for above
	static int						GetPredictionRandomSeed( void );
	static void						SetPredictionRandomSeed( const CUserCmd *cmd );
	static CBasePlayer				*GetPredictionPlayer( void );
	static void						SetPredictionPlayer( CBasePlayer *player );


	// For debugging shared code
	static bool						IsServer( void )
	{
		return true;
	}

	static bool						IsClient( void )
	{
		return false;
	}

	static char const				*GetDLLType( void )
	{
		return "server";
	}
};

// Send tables exposed in this module.
EXTERN_SEND_TABLE(DT_Edict);
EXTERN_SEND_TABLE(DT_BaseEntity);



// Ugly technique to override base member functions
// Normally it's illegal to cast a pointer to a member function of a derived class to a pointer to a 
// member function of a base class.  static_cast is a sleezy way around that problem.

#ifdef _DEBUG

#define SetTouch( a ) TouchSet( static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a), #a )
#define SetUse( a ) UseSet( static_cast <void (CBaseEntity::*)(	CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a), #a )
#define SetBlocked( a ) BlockedSet( static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a), #a )

#else

#define SetTouch( a ) m_pfnTouch = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)
#define SetUse( a ) m_pfnUse = static_cast <void (CBaseEntity::*)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a)
#define SetBlocked( a ) m_pfnBlocked = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)

#endif

// handling entity/edict transforms
inline CBaseEntity *GetContainingEntity( edict_t *pent )
{
	if ( pent && pent->m_pEnt )
		return pent->m_pEnt->GetBaseEntity();

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Pauses or resumes entity i/o events. When paused, no outputs will
//			fire unless Debug_SetSteps is called with a nonzero step value.
// Input  : bPause - true to pause, false to resume.
//-----------------------------------------------------------------------------
inline void CBaseEntity::Debug_Pause(bool bPause)
{
	CBaseEntity::m_bDebugPause = bPause;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if entity i/o is paused, false if not.
//-----------------------------------------------------------------------------
inline bool CBaseEntity::Debug_IsPaused(void)
{
	return(CBaseEntity::m_bDebugPause);
}


//-----------------------------------------------------------------------------
// Purpose: Decrements the debug step counter. Used when the entity i/o system
//			is in single step mode, this is called every time an output is fired.
// Output : Returns true on to continue firing outputs, false to stop.
//-----------------------------------------------------------------------------
inline bool CBaseEntity::Debug_Step(void)
{
	if (CBaseEntity::m_nDebugSteps > 0)
	{
		CBaseEntity::m_nDebugSteps--;
	}
	return(CBaseEntity::m_nDebugSteps > 0);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the number of entity outputs to allow to fire before pausing
//			the entity i/o system.
// Input  : nSteps - Number of steps to execute.
//-----------------------------------------------------------------------------
inline void CBaseEntity::Debug_SetSteps(int nSteps)
{
	CBaseEntity::m_nDebugSteps = nSteps;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if we should allow outputs to be fired, false if not.
//-----------------------------------------------------------------------------
inline bool CBaseEntity::Debug_ShouldStep(void)
{
	return(!CBaseEntity::m_bDebugPause || CBaseEntity::m_nDebugSteps > 0);
}


//-----------------------------------------------------------------------------
// Collision group accessors
//-----------------------------------------------------------------------------
inline int CBaseEntity::GetCollisionGroup() const
{
	return m_CollisionGroup;
}


//-----------------------------------------------------------------------------
// Methods relating to traversing hierarchy
//-----------------------------------------------------------------------------
inline CBaseEntity *CBaseEntity::GetMoveParent( void )
{
	return m_hMoveParent.Get(); 
}

inline CBaseEntity *CBaseEntity::FirstMoveChild( void )
{
	return m_hMoveChild.Get(); 
}

inline CBaseEntity *CBaseEntity::NextMovePeer( void )
{
	return m_hMovePeer.Get();
}

// FIXME: Remove this! There shouldn't be a difference between moveparent + parent
inline CBaseEntity* CBaseEntity::GetParent()
{
	return m_pParent.Get();
}

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline string_t CBaseEntity::GetEntityName() 
{ 
	return m_iName; 
}

inline bool CBaseEntity::NameMatches( const char *pszNameOrWildcard )
{
	// @TODO (toml 03-18-03): Support real wildcards. Right now, only thing supported is trailing *
	if ( STRING(m_iName) == pszNameOrWildcard )
		return true;

	int len = strlen( pszNameOrWildcard );
	if ( !len )
		return ( m_iName == NULL_STRING );
	
	if ( pszNameOrWildcard[ len - 1 ] != '*' )
		return ( stricmp( STRING(m_iName), pszNameOrWildcard ) == 0 );

	return ( _strnicmp( STRING(m_iName), pszNameOrWildcard, len - 1 ) == 0 );
}

inline bool CBaseEntity::ClassMatches( const char *pszClassOrWildcard )
{
	// @TODO (toml 03-18-03): Support real wildcards. Right now, only thing supported is trailing *
	if ( STRING(m_iClassname) == pszClassOrWildcard )
		return true;

	int len = strlen( pszClassOrWildcard );
	if ( !len )
		return ( m_iName == NULL_STRING );
	
	if ( pszClassOrWildcard[ len - 1 ] != '*' )
		return ( stricmp( STRING(m_iClassname), pszClassOrWildcard ) == 0 );

	return ( _strnicmp( STRING(m_iClassname), pszClassOrWildcard, len - 1 ) == 0 );
}

inline int CBaseEntity::GetSpawnFlags( void ) const
{ 
	return m_spawnflags; 
}

inline void CBaseEntity::AddSpawnFlags( int nFlags ) 
{ 
	m_spawnflags |= nFlags; 
}
inline void CBaseEntity::RemoveSpawnFlags( int nFlags ) 
{ 
	m_spawnflags &= ~nFlags; 
}

inline void CBaseEntity::ClearSpawnFlags( void ) 
{ 
	m_spawnflags = 0; 
}

inline bool CBaseEntity::HasSpawnFlags( int nFlags ) const
{ 
	return (m_spawnflags & nFlags) != 0; 
}

//-----------------------------------------------------------------------------
// checks to see if the entity is marked for deletion
//-----------------------------------------------------------------------------
inline bool CBaseEntity::IsMarkedForDeletion( void ) 
{ 
	return (m_iEFlags & EFL_KILLME); 
}

inline void CBaseEntity::AddEFlags( int nEFlagMask )
{
	m_iEFlags |= nEFlagMask;
}

inline void CBaseEntity::RemoveEFlags( int nEFlagMask )
{
	m_iEFlags &= ~nEFlagMask;
}

inline bool CBaseEntity::IsEFlagSet( int nEFlagMask ) const
{
	return (m_iEFlags & nEFlagMask) != 0;
}

//-----------------------------------------------------------------------------
// Network state optimization
//-----------------------------------------------------------------------------
inline CBaseCombatCharacter *ToBaseCombatCharacter( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	return pEntity->MyCombatCharacterPointer();
}


//-----------------------------------------------------------------------------
// Returns the entity-to-world transform
//-----------------------------------------------------------------------------
inline matrix3x4_t &CBaseEntity::EntityToWorldTransform() 
{ 
	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		CalcAbsolutePosition();
	}
	return m_rgflCoordinateFrame; 
}

inline const matrix3x4_t &CBaseEntity::EntityToWorldTransform() const
{ 
	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		const_cast<CBaseEntity*>(this)->CalcAbsolutePosition();
	}
	return m_rgflCoordinateFrame; 
}


//-----------------------------------------------------------------------------
// Some helper methods that transform a point from entity space to world space + back
//-----------------------------------------------------------------------------
inline void CBaseEntity::EntityToWorldSpace( const Vector &in, Vector *pOut ) const
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

inline void CBaseEntity::WorldToEntitySpace( const Vector &in, Vector *pOut ) const
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


//-----------------------------------------------------------------------------
// Velocity
//-----------------------------------------------------------------------------
inline Vector CBaseEntity::GetSmoothedVelocity( void )
{
	Vector vel;
	GetVelocity( &vel, NULL );
	return vel;
}

inline const Vector &CBaseEntity::GetLocalVelocity( ) const
{
	return m_vecVelocity.Get();
}

inline const Vector &CBaseEntity::GetAbsVelocity( ) const
{
	if (IsEFlagSet(EFL_DIRTY_ABSVELOCITY))
	{
		const_cast<CBaseEntity*>(this)->CalcAbsoluteVelocity();
	}
	return m_vecAbsVelocity;
}

inline const QAngle &CBaseEntity::GetLocalAngularVelocity( ) const
{
	return m_vecAngVelocity;
}

/*
// FIXME: While we're using (dPitch, dYaw, dRoll) as our local angular velocity
// representation, we can't actually solve this problem
inline const QAngle &CBaseEntity::GetAbsAngularVelocity( ) const
{
	if (IsEFlagSet(EFL_DIRTY_ABSANGVELOCITY))
	{
		const_cast<CBaseEntity*>(this)->CalcAbsoluteAngularVelocity();
	}

	return m_vecAbsAngVelocity;
}
*/

inline const Vector& CBaseEntity::GetBaseVelocity() const 
{ 
	return m_vecBaseVelocity; 
}

inline void CBaseEntity::SetBaseVelocity( const Vector& v ) 
{ 
	m_vecBaseVelocity = v; 
}

inline float CBaseEntity::GetGravity( void ) const
{ 
	return m_flGravity; 
}

inline void CBaseEntity::SetGravity( float gravity )
{ 
	m_flGravity = gravity; 
}

inline float CBaseEntity::GetFriction( void ) const
{ 
	return m_flFriction; 
}

inline void CBaseEntity::SetFriction( float flFriction )
{ 
	m_flFriction = flFriction; 
}

inline void	CBaseEntity::SetElasticity( float flElasticity )
{ 
	m_flElasticity = flElasticity; 
}

inline float CBaseEntity::GetElasticity( void )	const			
{ 
	return m_flElasticity; 
}

inline void	CBaseEntity::SetShadowCastDistance( float flDistance )
{ 
	m_flShadowCastDistance = flDistance; 
}

inline float CBaseEntity::GetShadowCastDistance( void )	const			
{ 
	return m_flShadowCastDistance; 
}

inline float CBaseEntity::GetLocalTime( void ) const
{ 
	return m_flLocalTime; 
}

inline void CBaseEntity::IncrementLocalTime( float flTimeDelta )
{ 
	m_flLocalTime += flTimeDelta; 
}

inline void	CBaseEntity::SetMoveDoneTime( float flDelay )
{
	if (flDelay >= 0)
		m_flMoveDoneTime = GetLocalTime() + flDelay;
	else
		m_flMoveDoneTime = -1;
}

inline float CBaseEntity::GetMoveDoneTime( ) const
{
	return (m_flMoveDoneTime >= 0) ? m_flMoveDoneTime - GetLocalTime() : -1;
}

inline CBaseEntity *CBaseEntity::Instance( const edict_t *pent )
{
	if ( pent && pent->m_pEnt )
	{
		return pent->m_pEnt->GetBaseEntity();
	}

	return NULL;
}

inline CBaseEntity *CBaseEntity::Instance( edict_t *pent ) 
{ 
	if ( !pent )
	{
		pent = INDEXENT(0);
	}
	return GetContainingEntity( pent );
}

inline CBaseEntity* CBaseEntity::Instance( int iEnt )
{
	return Instance( INDEXENT( iEnt ) );
}

inline int CBaseEntity::GetWaterLevel() const
{
	return m_nWaterLevel;
}

inline void CBaseEntity::SetWaterLevel( int nLevel )
{
	m_nWaterLevel = nLevel;
}

inline int CBaseEntity::GetWaterType() const
{
	return m_nWaterType;
}

inline void CBaseEntity::SetWaterType( int nType )
{
	m_nWaterType = nType;
}

inline const color32 CBaseEntity::GetRenderColor() const
{
	return m_clrRender;
}

inline void CBaseEntity::SetRenderColor( byte r, byte g, byte b )
{
	m_clrRender.Init( r, g, b );
}

inline void CBaseEntity::SetRenderColor( byte r, byte g, byte b, byte a )
{
	m_clrRender.Init( r, g, b, a );
}

inline void CBaseEntity::SetRenderColorR( byte r )
{
	m_clrRender.SetR( r );
}

inline void CBaseEntity::SetRenderColorG( byte g )
{
	m_clrRender.SetG( g );
}

inline void CBaseEntity::SetRenderColorB( byte b )
{
	m_clrRender.SetB( b );
}

inline void CBaseEntity::SetRenderColorA( byte a )
{
	m_clrRender.SetA( a );
}

inline void CBaseEntity::SetMoveCollide( MoveCollide_t val )
{ 
	m_MoveCollide = val; 
}

inline EntityChange_t CBaseEntity::DetectNetworkStateChanges()
{ 
	return m_NetStateMgr.DetectStateChanges(); 
}

inline void	CBaseEntity::ResetNetworkStateChanges()
{ 
	m_NetStateMgr.ResetStateChanges(); 
}

inline void CBaseEntity::SetGroundEntity( CBaseEntity *ground )
{
	m_hGroundEntity = ground;
}

inline CBaseEntity *CBaseEntity::GetGroundEntity( void )
{
	return m_hGroundEntity;
}

inline void CBaseEntity::ClearSolidFlags( void )
{
	m_Collision.ClearSolidFlags();
}

inline void CBaseEntity::RemoveSolidFlags( int flags )
{
	m_Collision.RemoveSolidFlags( flags );
}

inline void CBaseEntity::AddSolidFlags( int flags )
{
	m_Collision.AddSolidFlags( flags );
}

inline int CBaseEntity::GetSolidFlags( void ) const
{
	return m_Collision.GetSolidFlags();
}

inline bool CBaseEntity::IsSolidFlagSet( int flagMask ) const
{
	return m_Collision.IsSolidFlagSet( flagMask );
}

inline bool CBaseEntity::IsSolid() const
{
	return m_Collision.IsSolid( );
}

inline void CBaseEntity::SetSolid( SolidType_t val )
{
	m_Collision.SetSolid( val );
}

inline void CBaseEntity::SetSolidFlags( int flags )
{
	m_Collision.SetSolidFlags( flags );
}

inline SolidType_t CBaseEntity::GetSolid() const
{
	return m_Collision.GetSolid();
}
		 	 			 
//-----------------------------------------------------------------------------
// Gets the interface to the collideable representation of the entity
//-----------------------------------------------------------------------------
inline ICollideable *CBaseEntity::GetCollideable()
{
	return &m_Collision;
}
	

inline void CBaseEntity::SetModelName( string_t name )
{
	m_ModelName = name;
}

inline string_t CBaseEntity::GetModelName( void ) const
{
	return m_ModelName;
}

inline void CBaseEntity::SetModelIndex( int index )
{
	m_nModelIndex = index;
}

inline int CBaseEntity::GetModelIndex( void ) const
{
	return m_nModelIndex;
}

//-----------------------------------------------------------------------------
// Methods relating to bounds
//-----------------------------------------------------------------------------
inline bool CBaseEntity::IsBoundsDefinedInEntitySpace() const
{
	return m_Collision.IsBoundsDefinedInEntitySpace();
}

inline const Vector& CBaseEntity::WorldAlignMins( ) const
{
	return m_Collision.WorldAlignMins();
}

inline const Vector& CBaseEntity::WorldAlignMaxs( ) const
{
	return m_Collision.WorldAlignMaxs();
}

inline const Vector& CBaseEntity::EntitySpaceMins( ) const
{
	return m_Collision.EntitySpaceMins();
}

inline const Vector& CBaseEntity::EntitySpaceMaxs( ) const
{
	return m_Collision.EntitySpaceMaxs();
}

inline const Vector& CBaseEntity::EntitySpaceCenter( ) const
{
	Assert( IsBoundsDefinedInEntitySpace() );
	VectorLerp( m_Collision.m_vecMins, m_Collision.m_vecMaxs, 0.5f, m_Collision.m_vecCenter );
	return m_Collision.m_vecCenter;
//	return m_vecCenter.Get();
}
  
inline const Vector& CBaseEntity::WorldAlignSize( ) const
{
	return m_Collision.WorldAlignSize();
}

inline const Vector& CBaseEntity::EntitySpaceSize( ) const
{
	return m_Collision.EntitySpaceSize();
}

inline bool CBaseEntity::IsPointSized() const
{
	return m_Collision.m_vecSize == vec3_origin;
}

// It may not be particularly tight-fitting depending on performance-related questions
// It may also fluctuate in size at any time
inline const Vector& CBaseEntity::EntitySpaceSurroundingMins() const
{
	// FIXME!
	return m_Collision.m_vecMins.Get();
}

inline const Vector& CBaseEntity::EntitySpaceSurroundingMaxs() const
{
	// FIXME!
	return m_Collision.m_vecMaxs.Get();
}

inline const Vector& CBaseEntity::WorldSpaceSurroundingMins() const
{
	// FIXME!
	return m_vecAbsMins;
}

inline const Vector& CBaseEntity::WorldSpaceSurroundingMaxs() const
{
	// FIXME!
	return m_vecAbsMaxs;
}

inline void CBaseEntity::SetAbsMins( const Vector& mins )
{
	m_vecAbsMins = mins;
}

inline const Vector& CBaseEntity::GetAbsMins( void ) const
{
	return m_vecAbsMins;
}

inline void CBaseEntity::SetAbsMaxs( const Vector& maxs )
{
	m_vecAbsMaxs = maxs;
}

inline const Vector& CBaseEntity::GetAbsMaxs( void ) const
{
	return m_vecAbsMaxs;
}



// Ugly technique to override base member functions
// Normally it's illegal to cast a pointer to a member function of a derived class to a pointer to a 
// member function of a base class.  static_cast is a sleezy way around that problem.

#define SetThink( a ) ThinkSet( static_cast <void (CBaseEntity::*)(void)> (a), 0, NULL )
#define SetContextThink( a, b, context ) ThinkSet( static_cast <void (CBaseEntity::*)(void)> (a), (b), context )

#define SetMoveDone( a ) m_pfnMoveDone = static_cast <void (CBaseEntity::*)(void)> (a)

inline bool FClassnameIs(CBaseEntity *pEntity, const char *szClassname)
{ 
	return FStrEq( STRING(pEntity->m_iClassname), szClassname); 
}

class CPointEntity : public CBaseEntity
{
	DECLARE_CLASS( CPointEntity, CBaseEntity );
public:
	void	Spawn( void );
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
private:
};

class CLogicalEntity : public CBaseEntity
{
	DECLARE_CLASS( CLogicalEntity, CBaseEntity );
public:
	CLogicalEntity() : CBaseEntity( true ) {}
	
	virtual int ObjectCaps( void ) { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
};

class CLogicalPointEntity : public CLogicalEntity
{
	DECLARE_CLASS( CLogicalPointEntity, CLogicalEntity );

public:
	virtual void SetOwnerEntity( CBaseEntity* pOwner );

	// Returns the owner, origin, and angles
	virtual CBaseEntity	*GetOwnerEntity( );
};



#endif // BASEENTITY_H
