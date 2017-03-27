//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Base combat character with no AI
//
// $NoKeywords: $
//=============================================================================

#ifndef BASECOMBATCHARACTER_H
#define BASECOMBATCHARACTER_H

#include <limits.h>

#ifdef _WIN32
#pragma once
#endif

#ifdef TF2_DLL
#include "tf_shareddefs.h"

#define POWERUP_THINK_CONTEXT	"PowerupThink"
#endif

#include "cbase.h"
#include "baseentity.h"
#include "baseflex.h"
#include "damagemodifier.h"
#include "utllinkedlist.h"
#include "ai_hull.h"

class CScriptedTarget;
typedef CHandle<CBaseCombatWeapon> CBaseCombatWeaponHandle;

// ---------------------------
//  Hit Group standards
// ---------------------------
#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4	
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7
#define HITGROUP_GEAR		10			// alerts NPC, but doesn't do damage or bleed (1/100th damage)

struct proficiencyinfo_t
{
	float	value;
	char	*pszName;
};

enum WeaponProficiency_t
{
	WEAPON_PROFICIENCY_INVALID_LOW = -1,
	WEAPON_PROFICIENCY_VERY_LOW = 0,
	WEAPON_PROFICIENCY_LOW,
	WEAPON_PROFICIENCY_AVERAGE,
	WEAPON_PROFICIENCY_HIGH,
	WEAPON_PROFICIENCY_VERY_HIGH,
	WEAPON_PROFICIENCY_PERFECT,
	WEAPON_PROFICIENCY_INVALID_HIGH,
};

// -------------------------------------
//  Capability Bits
// -------------------------------------

enum Capability_t 
{
	bits_CAP_MOVE_GROUND			= 0x00000001, // walk/run
	bits_CAP_MOVE_JUMP				= 0x00000002, // jump/leap
	bits_CAP_MOVE_FLY				= 0x00000004, // can fly, move all around
	bits_CAP_MOVE_CLIMB				= 0x00000008, // climb ladders
	bits_CAP_MOVE_SWIM				= 0x00000010, // navigate in water			// UNDONE - not yet implemented
	bits_CAP_MOVE_CRAWL				= 0x00000020, // crawl						// UNDONE - not yet implemented
	bits_CAP_MOVE_SHOOT				= 0x00000040, // tries to shoot weapon while moving
	//bits_CAP_DETECT_GRENADES		= 0x00000080, // UNUSED
	bits_CAP_USE					= 0x00000100, // open doors/push buttons/pull levers
	//bits_CAP_HEAR					= 0x00000200, // can hear forced sounds
	bits_CAP_AUTO_DOORS				= 0x00000400, // can trigger auto doors
	bits_CAP_OPEN_DOORS				= 0x00000800, // can open manual doors
	bits_CAP_TURN_HEAD				= 0x00001000, // can turn head, always bone controller 0
	bits_CAP_WEAPON_RANGE_ATTACK1	= 0x00002000, // can do a weapon range attack 1
	bits_CAP_WEAPON_RANGE_ATTACK2	= 0x00004000, // can do a weapon range attack 2
	bits_CAP_WEAPON_MELEE_ATTACK1	= 0x00008000, // can do a weapon melee attack 1
	bits_CAP_WEAPON_MELEE_ATTACK2	= 0x00010000, // can do a weapon melee attack 2
	bits_CAP_INNATE_RANGE_ATTACK1	= 0x00020000, // can do a innate range attack 1
	bits_CAP_INNATE_RANGE_ATTACK2	= 0x00040000, // can do a innate range attack 1
	bits_CAP_INNATE_MELEE_ATTACK1	= 0x00080000, // can do a innate melee attack 1
	bits_CAP_INNATE_MELEE_ATTACK2	= 0x00100000, // can do a innate melee attack 1
	bits_CAP_USE_WEAPONS			= 0x00200000, // can use weapons (non-innate attacks)
	bits_CAP_STRAFE					= 0x00400000, // strafe ( walk/run sideways)
	bits_CAP_ANIMATEDFACE			= 0x00800000, // has animated eyes/face
	//bits_CAP_SIDESTEP				= 0x01000000,	// assumes both left and right!
	//bits_CAP_BACKSTEP				= 0x02000000,	// assumes both for and back!
	bits_CAP_SQUAD					= 0x04000000, // can form squads
	bits_CAP_DUCK					= 0x08000000, // cover and reload ducking
	bits_CAP_NO_HIT_PLAYER			= 0x10000000, // don't hit players
	bits_CAP_AIM_GUN				= 0x20000000, // Use arms to aim gun, not just body
	//bits_CAP_NONE					= 0x40000000, // none
	//bits_CAP_NONE					= 0x80000000, // none
};

#define bits_CAP_DOORS_GROUP    (bits_CAP_USE | bits_CAP_AUTO_DOORS | bits_CAP_OPEN_DOORS)
#define bits_CAP_RANGE_ATTACK_GROUP	(bits_CAP_WEAPON_RANGE_ATTACK1 | bits_CAP_WEAPON_RANGE_ATTACK2)
#define bits_CAP_MELEE_ATTACK_GROUP	(bits_CAP_WEAPON_MELEE_ATTACK1 | bits_CAP_WEAPON_MELEE_ATTACK2)


class CBaseCombatWeapon;


enum Disposition_t 
{
	D_ER,		// Undefined - error
	D_HT,		// Hate
	D_FR,		// Fear
	D_LI,		// Like
	D_NU		// Neutral
};

const int DEF_RELATIONSHIP_PRIORITY = INT_MIN;

struct Relationship_t
{
	EHANDLE			entity;			// Relationship to a particular entity
	Class_T			classType;		// Relationship to a class  CLASS_NONE = not class based (Def. in baseentity.h)
	Disposition_t	disposition;	// D_HT (Hate), D_FR (Fear), D_LI (Like), D_NT (Neutral)
	int				priority;		// Relative importance of this relationship (higher numbers mean more important)

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------
// Purpose: This should contain all of the combat entry points / functionality 
// that are common between NPCs and players
//-----------------------------------------------------------------------------
class CBaseCombatCharacter : public CBaseFlex
{
	DECLARE_CLASS( CBaseCombatCharacter, CBaseFlex );

public:
	CBaseCombatCharacter(void);
	~CBaseCombatCharacter(void);

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

public:

	int					TakeHealth( float flHealth, int bitsDamageType );
	void				CauseDeath( const CTakeDamageInfo &info );
	// Override this in combat character to handle rotation differently
	void				SetObjectCollisionBox( void );

	virtual bool		FInViewCone( CBaseEntity *pEntity );
	virtual bool		FInViewCone( const Vector &vecSpot );

	virtual bool		FInAimCone( CBaseEntity *pEntity );
	virtual bool		FInAimCone( const Vector &vecSpot );
	
	virtual bool		ShouldShootMissTarget( CBaseCombatCharacter *pAttacker );
	virtual CBaseEntity *FindMissTarget( void );

	virtual bool		HandleInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt );

	virtual QAngle		BodyAngles();
	virtual Vector		BodyDirection2D( void );
	virtual Vector		BodyDirection3D( void );
	virtual Vector		HeadDirection2D( void )	{ return BodyDirection2D( ); }; // No head motion so just return body dir
	virtual Vector		HeadDirection3D( void )	{ return BodyDirection2D( ); }; // No head motion so just return body dir
	virtual Vector		EyeDirection2D( void ) 	{ return HeadDirection2D( );  }; // No eye motion so just return head dir
	virtual Vector		EyeDirection3D( void ) 	{ return HeadDirection3D( );  }; // No eye motion so just return head dir

	virtual void SetTransmit( CCheckTransmitInfo *pInfo );


	// -----------------------
	// Ammo
	// -----------------------
	virtual int			GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound = false );
	int					GiveAmmo( int iCount, const char *szName, bool bSuppressSound = false );
	void				RemoveAmmo( int iCount, int iAmmoIndex );
	void				RemoveAllAmmo( );
	int					GetAmmoCount( int iAmmoIndex ) const;
	int					GetAmmoCount( char *szName ) const;

	virtual Activity	NPC_TranslateActivity( Activity baseAct );

	// -----------------------
	// Weapons
	// -----------------------
	CBaseCombatWeapon*	Weapon_Create( const char *pWeaponName );
	virtual Activity	Weapon_TranslateActivity( Activity baseAct );
	void				Weapon_SetActivity( Activity newActivity, float duration );
	void				Weapon_FrameUpdate( void );
	void				Weapon_HandleAnimEvent( animevent_t *pEvent );
	CBaseCombatWeapon*	Weapon_OwnsThisType( const char *pszWeapon, int iSubType = 0 ) const;  // True if already owns a weapon of this class
	virtual bool		Weapon_CanUse( CBaseCombatWeapon *pWeapon );		// True is allowed to use this class of weapon
	virtual void		Weapon_Equip( CBaseCombatWeapon *pWeapon );			// Adds weapon to player
	virtual bool		Weapon_EquipAmmoOnly( CBaseCombatWeapon *pWeapon );	// Adds weapon ammo to player, leaves weapon
	bool				Weapon_Detach( CBaseCombatWeapon *pWeapon );		// Clear any pointers to the weapon.
	virtual void		Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	virtual	bool		Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );		// Switch to given weapon if has ammo (false if failed)
	virtual	Vector		Weapon_ShootPosition( );		// gun position at current position/orientation
	CBaseEntity*		Weapon_FindUsable( const Vector &range );			// search for a usable weapon in this range
	bool				Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon);
	virtual bool		Weapon_SlotOccupied( CBaseCombatWeapon *pWeapon );
	virtual CBaseCombatWeapon *Weapon_GetSlot( int slot );

	//
	// Catching on fire.
	//
	virtual bool			IsOnFire( void ) { return m_bOnFire; }
	virtual void			Ignite( float flFlameLifetime );

	virtual bool			AddPlayerItem( CBaseCombatWeapon *pItem ) { return false; }
	virtual bool			RemovePlayerItem( CBaseCombatWeapon *pItem ) { return false; }

	// -----------------------
	// Damage
	// -----------------------
	// Don't override this for characters, override the per-life-state versions below
	virtual int				OnTakeDamage( const CTakeDamageInfo &info );

	// Override these to control how your character takes damage in different states
	virtual int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual int				OnTakeDamage_Dying( const CTakeDamageInfo &info );
	virtual int				OnTakeDamage_Dead( const CTakeDamageInfo &info );

	// utility function to calc damage force
	Vector					CalcDamageForceVector( const CTakeDamageInfo &info );

	virtual int				BloodColor( void ) { return m_bloodColor; }
	virtual Activity		GetDeathActivity( void );

	virtual bool			CorpseGib( const CTakeDamageInfo &info );
	virtual void			CorpseFade( void );	// Called instead of GibNPC() when gibs are disabled
	virtual bool			HasHumanGibs( void );
	virtual bool			HasAlienGibs( void );
	virtual bool			ShouldGib( const CTakeDamageInfo &info ) { return false; }	// Always ragdoll, unless specified by the leaf class

	// Character killed (only fired once)
	virtual void			Event_Killed( const CTakeDamageInfo &info );

	// Killed a character
	void InputKilledNPC( inputdata_t &inputdata );
	virtual void OnKilledNPC( CBaseCombatCharacter *pKilled ) {}; 

	// Exactly one of these happens immediately after killed (gibbed may happen later when the corpse gibs)
	// Character gibbed or faded out (violence controls) (only fired once)
	// returns true if gibs were spawned
	virtual bool			Event_Gibbed( const CTakeDamageInfo &info );
	// Character entered the dying state without being gibbed (only fired once)
	virtual void			Event_Dying( void );
	// character died and should become a ragdoll now
	// return true if converted to a ragdoll, false to use AI death
	virtual bool			BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );


	CBaseEntity*			CheckTraceHullAttack( float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType );
	CBaseEntity*			CheckTraceHullAttack( const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType );

	virtual CBaseCombatCharacter *MyCombatCharacterPointer( void ) { return this; }

	// VPHYSICS
	virtual void			VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void			VPhysicsGetShadowVelocity( IPhysicsObject *pPhysics, Vector &outVel );

	void SetImpactEnergyScale( float fScale ) { m_impactEnergyScale = fScale; }

	virtual void			UpdateOnRemove( void );

	virtual Disposition_t	IRelationType( CBaseEntity *pTarget );
	virtual int				IRelationPriority( CBaseEntity *pTarget );
protected:
	Relationship_t			*FindEntityRelationship( CBaseEntity *pTarget );

public:
	// Blood color (see BLOOD_COLOR_* macros in baseentity.h)
	void SetBloodColor( int nBloodColor );

	// Weapons..
	CBaseCombatWeapon*	GetActiveWeapon() const;
	int					WeaponCount() const;
	CBaseCombatWeapon*	GetWeapon( int i );
	bool				RemoveWeapon( CBaseCombatWeapon *pWeapon );
	void				RemoveAllWeapons();
	int					GetCurrentWeaponProficiency() { return m_CurrentWeaponProficiency; }
	void				SetCurrentWeaponProficiency( int iProficiency ) { m_CurrentWeaponProficiency = iProficiency; }
	virtual int			CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );
	virtual	Vector		GetAttackAccuracy( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL );

	// Interactions
	static void			InitInteractionSystem();

	// Relationships
	static void			AllocateDefaultRelationships( );
	static void			SetDefaultRelationship( Class_T nClass, Class_T nClassTarget,  Disposition_t nDisposition, int nPriority );
	void				AddEntityRelationship( CBaseEntity *pEntity, Disposition_t nDisposition, int nPriority );
	void				AddClassRelationship  ( Class_T nClass, Disposition_t nDisposition, int nPriority );

	// Nav hull type
	Hull_t	GetHullType() const				{ return m_eHull; }
	void	SetHullType( Hull_t hullType )	{ m_eHull = hullType; }

	// FIXME: The following 3 methods are backdoor hack methods
	
	// This is a sort of hack back-door only used by physgun!
	void SetAmmoCount( int iCount, int iAmmoIndex );

	// This is a hack to blat out the current active weapon...
	// Used by weapon_slam + game_ui
	void SetActiveWeapon( CBaseCombatWeapon *pNewWeapon );
	void ClearActiveWeapon() { SetActiveWeapon( NULL ); }
	virtual void OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon ) {}

	// I can't use my current weapon anymore. Switch me to the next best weapon.
	bool SwitchToNextBestWeapon(CBaseCombatWeapon *pCurrent);

	// This is a hack to copy the relationship strings used by monstermaker
	void SetRelationshipString( string_t theString ) { m_RelationshipString = theString; }

	float				GetNextAttack() const { return m_flNextAttack; }
	void				SetNextAttack( float flWait ) { m_flNextAttack = flWait; }

#ifdef TF2_DLL
public:

	// TF2 Powerups
	virtual bool		CanBePoweredUp( void );
	bool				HasPowerup( int iPowerup );
	virtual	bool		CanPowerupNow( int iPowerup );		// Return true if I can be powered by this powerup right now
	virtual	bool		CanPowerupEver( int iPowerup );		// Return true if I ever accept this powerup type

	void				SetPowerup( int iPowerup, bool bState, float flTime = 0, float flAmount = 0, CBaseEntity *pAttacker = NULL, CDamageModifier *pDamageModifier = NULL );
	virtual	bool		AttemptToPowerup( int iPowerup, float flTime, float flAmount = 0, CBaseEntity *pAttacker = NULL, CDamageModifier *pDamageModifier = NULL );
	virtual	float		PowerupDuration( int iPowerup, float flTime );
	virtual	void		PowerupStart( int iPowerup, float flAmount = 0, CBaseEntity *pAttacker = NULL, CDamageModifier *pDamageModifier = NULL );
	virtual	void		PowerupEnd( int iPowerup );

	void				PowerupThink( void );
	virtual	void		PowerupThink( int iPowerup );

public:

	CNetworkVar( int, m_iPowerups );
	float				m_flPowerupAttemptTimes[ MAX_POWERUPS ];
	float				m_flPowerupEndTimes[ MAX_POWERUPS ];
	float				m_flFractionalBoost;	// POWERUP_BOOST health fraction - specific powerup data

#endif

protected:
	// returns the last body region that took damage
	int	LastHitGroup() const				{ return m_LastHitGroup; }
	void SetLastHitGroup( int nHitGroup )	{ m_LastHitGroup = nHitGroup; }

public:
	CNetworkVar( float, m_flNextAttack );			// cannot attack again until this time

protected:
private:
	Hull_t		m_eHull;

protected:
	int			m_bloodColor;			// color of blood particless

	// -------------------
	// combat ability data
	// -------------------
	float		m_flFieldOfView;		// cosine of field of view for this character
	Vector		m_HackedGunPos;			// HACK until we can query end of gun
	string_t	m_RelationshipString;	// Used to load up relationship keyvalues

private:
	friend class CScriptedTarget; // needs to access GetInteractionID()
	
	static int					m_lastInteraction;	// Last registered interaction #
	static int					GetInteractionID();	// Returns the next interaction #
	static Relationship_t**		m_DefaultRelationship;

	bool		m_bOnFire;				// am I burning?


	// attack/damage
	int					m_LastHitGroup;		// the last body region that took damage
	float				m_impactEnergyScale;// scale the amount of energy used to calculate damage this ent takes due to physics
	
	// Weapon proficiency gets calculated each time an NPC changes his weapon, and then
	// cached off as the CurrentWeaponProficiency.
	int					m_CurrentWeaponProficiency;

	// ---------------
	//  Relationships
	// ---------------
	CUtlVector<Relationship_t>		m_Relationship;						// Array of relationships

	// shared ammo slots
	CNetworkArrayForDerived( int, m_iAmmo, MAX_AMMO_SLOTS );

	// Usable character items 
	CNetworkArray( CBaseCombatWeaponHandle, m_hMyWeapons, MAX_WEAPONS );

	CNetworkHandle( CBaseCombatWeapon, m_hActiveWeapon );

	friend class CCleanupDefaultRelationShips;
};

#ifdef TF2_DLL
// Powerup Inlines
inline bool CBaseCombatCharacter::CanBePoweredUp( void )							{ return true; }
inline float CBaseCombatCharacter::PowerupDuration( int iPowerup, float flTime )	{ return flTime; }
inline void	CBaseCombatCharacter::PowerupEnd( int iPowerup )						{ return; }
inline void	CBaseCombatCharacter::PowerupThink( int iPowerup )						{ return; }
#endif

EXTERN_SEND_TABLE(DT_BaseCombatCharacter);

void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore );

#endif // BASECOMBATCHARACTER_H
