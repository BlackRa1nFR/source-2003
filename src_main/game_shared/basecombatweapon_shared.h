//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef COMBATWEAPON_SHARED_H
#define COMBATWEAPON_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "vphysics_interface.h"
#include "predictable_entity.h"
#include "soundflags.h"
#include "weapon_parse.h"
#include "baseviewmodel_shared.h"

#if defined( CLIENT_DLL )
#define CBaseCombatWeapon C_BaseCombatWeapon
#endif

#if !defined( CLIENT_DLL )
extern void OnBaseCombatWeaponCreated( CBaseCombatWeapon * );
extern void OnBaseCombatWeaponDestroyed( CBaseCombatWeapon * );
#endif

class CBasePlayer;
class CBaseCombatCharacter;
class IPhysicsConstraint;

//Start with a constraint in place (don't drop to floor)
#define	SF_WEAPON_START_CONSTRAINED	(1<<0)		

//Percent
#define	CLIP_PERC_THRESHOLD		0.75f	

typedef struct
{
	int			baseAct;
	int			weaponAct;
	bool		required;
} acttable_t;

class CHudTexture;
class Color;

namespace vgui2
{
	typedef unsigned long HFont;
}

// -----------------------------------------
//	Vector cones
// -----------------------------------------
// VECTOR_CONE_PRECALCULATED - this resolves to vec3_origin, but adds some
// context indicating that the person writing the code is not allowing
// FireBullets() to modify the direction of the shot because the shot direction
// being passed into the function has already been modified by another piece of
// code and should be fired as specified. See GetActualShotTrajectory(). 
#define VECTOR_CONE_PRECALCULATED	vec3_origin
#define VECTOR_CONE_1DEGREES		Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES		Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES		Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES		Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES		Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES		Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES		Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES		Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES		Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES		Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES		Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES		Vector( 0.17365, 0.17365, 0.17365 )

//-----------------------------------------------------------------------------
// Purpose: Base weapon class, shared on client and server
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Client side rep of CBaseTFCombatWeapon 
//-----------------------------------------------------------------------------
class CBaseCombatWeapon : public CBaseAnimating
{
public:
	DECLARE_CLASS( CBaseCombatWeapon, CBaseAnimating );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

							CBaseCombatWeapon();
	virtual 				~CBaseCombatWeapon();

	// A derived weapon class should return true here so that weapon sounds, etc, can
	//  apply the proper filter
	virtual bool			IsPredicted( void ) const { return false; }

	virtual void			Spawn( void );
	virtual void			Precache( void );

	virtual void			SetObjectCollisionBox( void );

	// Subtypes are used to manage multiple weapons of the same type on the player.
	virtual int				GetSubType( void ) { return m_iSubType; }
	virtual void			SetSubType( int iType ) { m_iSubType = iType; }

	virtual void			Equip( CBaseCombatCharacter *pOwner );
	virtual void			Drop( const Vector &vecVelocity );

	virtual	int				UpdateClientData( CBasePlayer *pPlayer );

	virtual bool			IsAllowedToSwitch( void );
	virtual bool			CanBeSelected( void );
	virtual bool			VisibleInWeaponSelection( void );
	virtual bool			HasAmmo( void );

	// Weapon Pickup For Player
	virtual void			SetPickupTouch( void );
	void 					DefaultTouch( CBaseEntity *pOther );	// default weapon touch

	// Weapon client handling
	virtual void			SetViewModelIndex( int index = 0 );
	virtual void			SendWeaponAnim( int iActivity );
	virtual void			SendViewModelAnim( int nSequence );

	virtual bool			HasWeaponIdleTimeElapsed( void );
	virtual void			SetWeaponIdleTime( float time );
	virtual float			GetWeaponIdleTime( void );

	// Weapon selection
	virtual bool			HasAnyAmmo( void );							// Returns true is weapon has ammo
	virtual bool			HasPrimaryAmmo( void );						// Returns true is weapon has ammo
	virtual bool			HasSecondaryAmmo( void );					// Returns true is weapon has ammo
	bool					UsesPrimaryAmmo( void );					// returns true if the weapon actually uses primary ammo
	bool					UsesSecondaryAmmo( void );					// returns true if the weapon actually uses secondary ammo

	virtual bool			CanHolster( void ) { return TRUE; };		// returns true if the weapon can be holstered
	bool					DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt );
	virtual bool			CanDeploy( void ) { return true; }			// return true if the weapon's allowed to deploy
	virtual bool			Deploy( void );								// returns true is deploy was successful
	virtual bool			Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual CBaseCombatWeapon *GetLastWeapon( void ) { return this; }
	virtual void			SetWeaponVisible( bool visible );

	// Weapon behaviour
	virtual void			ItemPreFrame( void );					// called each frame by the player PreThink
	virtual void			ItemPostFrame( void );					// called each frame by the player PostThink
	virtual void			ItemBusyFrame( void );					// called each frame by the player PostThink, if the player's not ready to attack yet
	virtual void			WeaponIdle( void );						// called when no buttons pressed

	// Reloading
	virtual	void			CheckReload( void );
	virtual void			FinishReload( void );
	virtual void			AbortReload( void );
	virtual bool			Reload( void );
	bool					DefaultReload( int iClipSize1, int iClipSize2, int iActivity );

	// Weapon firing
	virtual void			PrimaryAttack( void );						// do "+ATTACK"
	virtual void			SecondaryAttack( void ) { return; }			// do "+ATTACK2"

	// Firing animations
	virtual int				GetPrimaryAttackActivity( void );
	virtual int				GetSecondaryAttackActivity( void );
	virtual float			GetDefaultAnimSpeed( void ) { return 1.0; }

	// Bullet launch information
	virtual int				GetBulletType( void );
	virtual const Vector&	GetBulletSpread( void );
	virtual float			GetFireRate( void );
	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 1; }
	virtual int				GetRandomBurst() { return random->RandomInt( GetMinBurst(), GetMaxBurst() ); }
	virtual void			WeaponSound( WeaponSound_t sound_type, float soundtime = 0.0f );
	virtual void			StopWeaponSound( WeaponSound_t sound_type );

	// TF Sprinting functions
	virtual bool			StartSprinting( void ) { return false; };
	virtual bool			StopSprinting( void ) { return false; };

	// TF Injury functions
	virtual float			GetDamage( float flDistance, int iLocation ) { return 0.0; };

	virtual void			SetActivity( Activity act, float duration );
	inline void				SetActivity( Activity eActivity ) { m_Activity = eActivity; }
	inline Activity			GetActivity( void ) { return m_Activity; }

	virtual void			AddViewKick( void );	// Add in the view kick for the weapon

	virtual char			*GetDeathNoticeName( void );	// Get the string to print death notices with

	CBaseCombatCharacter	*GetOwner() const;
	void					SetOwner( CBaseCombatCharacter *owner );
	void					OnPickedUp( CBaseCombatCharacter *pNewOwner );

	virtual void			AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles ) {};
	virtual float			CalcViewmodelBob( void ) { return 0.0f; };

	// Returns information about the various control panels
	virtual void 			GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	virtual void			GetControlPanelClassName( int nPanelIndex, const char *&pPanelName );

	virtual bool			ShouldShowControlPanels( void ) { return true; }

public:

	// Weapon info accessors for data in the weapon's data file
	const FileWeaponInfo_t	&GetWpnData( void ) const;
	virtual const char		*GetViewModel( int viewmodelindex = 0 ) const;
	virtual const char		*GetWorldModel( void ) const;
	virtual const char		*GetAnimPrefix( void ) const;
	virtual int				GetMaxClip1( void ) const;
	virtual int				GetMaxClip2( void ) const;
	virtual int				GetDefaultClip1( void ) const;
	virtual int				GetDefaultClip2( void ) const;
	virtual int				GetWeight( void ) const;
	virtual int				GetFlags( void ) const;
	virtual int				GetSlot( void ) const;
	virtual int				GetPosition( void ) const;
	virtual char const		*GetName( void ) const;
	virtual char const		*GetPrintName( void ) const;
	virtual char const		*GetShootSound( int iIndex ) const;
	virtual bool			UsesClipsForAmmo1( void ) const;
	virtual bool			UsesClipsForAmmo2( void ) const;

	virtual int				GetPrimaryAmmoType( void )  const { return m_iPrimaryAmmoType; }
	virtual int				GetSecondaryAmmoType( void )  const { return m_iSecondaryAmmoType; }
	int						Clip1() const { return m_iClip1; }
	int						Clip2() const { return m_iClip2; }

	virtual CHudTexture const	*GetSpriteActive( void ) const;
	virtual CHudTexture const	*GetSpriteInactive( void ) const;
	virtual CHudTexture const	*GetSpriteAmmo( void ) const;
	virtual CHudTexture const	*GetSpriteAmmo2( void ) const;
	virtual CHudTexture const	*GetSpriteCrosshair( void ) const;
	virtual CHudTexture const	*GetSpriteAutoaim( void ) const;
	virtual CHudTexture const	*GetSpriteZoomedCrosshair( void ) const;
	virtual CHudTexture const	*GetSpriteZoomedAutoaim( void ) const;

public:
// Server Only Methods
#if !defined( CLIENT_DLL )

	DECLARE_DATADESC();
	virtual void			FallInit( void );						// prepare to fall to the ground
	virtual void			FallThink( void );						// make the weapon fall to the ground after spawning

	// Weapon spawning
	bool					IsInBadPosition ( void );				// Is weapon in bad position to pickup?
	bool					RepositionWeapon ( void );				// Attempts to reposition the weapon in a location where it can be
	void					Materialize( void );					// make a weapon visible and tangible
	void					AttemptToMaterialize( void );			// see if the game rules will let the weapon become visible and tangible
	virtual void			CheckRespawn( void );					// see if this weapon should respawn after being picked up
	CBaseEntity				*Respawn ( void );						// copy a weapon

	static int				GetAvailableWeaponsInBox( CBaseCombatWeapon **pList, int listMax, const Vector &mins, const Vector &maxs );

	// Weapon dropping / destruction
	virtual void			Delete( void );
	void					DestroyItem( void );
	virtual void			Kill( void );

	virtual int				CapabilitiesGet( void ) { return 0; }
	virtual	int				ObjectCaps( void );

	virtual bool			IsRemoveable() { return m_bRemoveable; }
	void					SetRemoveable( bool bRemoveable ) { m_bRemoveable = bRemoveable; }
	

	virtual Activity		ActivityOverride( Activity baseAct );
	virtual	acttable_t*		ActivityList( void ) { return NULL; }
	virtual	int				ActivityListCount( void ) { return 0; }

	// Returns bits for	weapon conditions
	virtual bool			WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );	
	virtual	int				WeaponRangeAttack1Condition( float flDot, float flDist );
	virtual	int				WeaponRangeAttack2Condition( float flDot, float flDist );
	virtual	int				WeaponMeleeAttack1Condition( float flDot, float flDist );
	virtual	int				WeaponMeleeAttack2Condition( float flDot, float flDist );

	virtual void			Operator_FrameUpdate( CBaseCombatCharacter  *pOperator );
	virtual void			Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	// NOTE: This should never be called when a character is operating the weapon.  Animation events should be
	// routed through the character, and then back into CharacterAnimEvent() 
	void					HandleAnimEvent( animevent_t *pEvent );

	virtual bool			ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );


// Client only methods
#else

	virtual bool			OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options ) { return false; }

	// Should this object cast shadows?
	virtual ShadowType_t	ShadowCastType();
	virtual void			SetDormant( bool bDormant );
	virtual void			OnPreDataChanged( DataUpdateType_t updateType );
	virtual void			OnDataChanged( DataUpdateType_t updateType );
	virtual void			OnRestore();

	virtual void			Redraw(void);
	virtual void			ViewModelDrawn( CBaseViewModel *pViewModel );
	// Get the position that bullets are seen coming out. Note: the returned values are different
	// for first person and third person.
	bool					GetShootPosition( Vector &vOrigin, QAngle &vAngles );
	virtual void			DrawCrosshair( void );
	virtual void			DrawAmmo( void );
	// Weapon state checking
	virtual bool			IsCarriedByLocalPlayer( void );
	virtual bool			IsActiveByLocalPlayer( void );

	bool					IsBeingCarried() const;

	// Is the carrier alive?
	bool					IsCarrierAlive() const;

	// Returns the aiment render origin + angles
	virtual void			GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pOrigin, QAngle *pAngles );
	virtual int				DrawModel( int flags );
	virtual bool			ShouldDraw( void );
	virtual bool			ShouldDrawPickup( void );
	virtual void			HandleInput( void ) { return; };
	virtual void			OverrideMouseInput( int *x, int *y ) { return; };
	virtual int				KeyInput( int down, int keynum, const char *pszCurrentBinding ) { return 1; }
	virtual bool			AddLookShift( void ) { return true; };

	virtual void			GetViewmodelBoneControllers(C_BaseViewModel *pViewModel, float controllers[MAXSTUDIOBONECTRLS]) { return; }

	virtual void			NotifyShouldTransmit( ShouldTransmitState_t state );

#endif // End client-only methods

private:
	typedef CHandle< CBaseCombatCharacter > CBaseCombatCharacterHandle;
	CNetworkVar( CBaseCombatCharacterHandle, m_hOwner );				// Player carrying this weapon

protected:

public:

	// Networked fields
	CNetworkVar( int, m_nViewModelIndex );

	// Weapon firing
	CNetworkVar( float, m_flNextPrimaryAttack );						// soonest time ItemPostFrame will call PrimaryAttack
	CNetworkVar( float, m_flNextSecondaryAttack );					// soonest time ItemPostFrame will call SecondaryAttack
	CNetworkVar( float, m_flTimeWeaponIdle );							// soonest time ItemPostFrame will call WeaponIdle
	// Weapon state
	bool					m_bInReload;			// Are we in the middle of a reload;
	bool					m_bFireOnEmpty;			// True when the gun is empty and the player is still holding down the attack key(s)
	// Weapon art
	CNetworkVar( int, m_iViewModelIndex );
	CNetworkVar( int, m_iWorldModelIndex );
	// Sounds
	float					m_flNextEmptySoundTime;				// delay on empty sound playing

	void					SetIdealActivity( Activity ideal );
	void					MaintainIdealActivity( void );

private:
	Activity				m_Activity;
	int						m_nIdealSequence;
	Activity				m_IdealActivity;

	bool					m_bRemoveable;

public:

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_nNextThinkTick );

	int						WeaponState() const { return m_iState; }

	// Weapon data
	CNetworkVar( int, m_iState );				// See WEAPON_* definition
	string_t				m_iszName;				// Classname of this weapon.
	CNetworkVar( int, m_iPrimaryAmmoType );		// "primary" ammo index into the ammo info array 
	CNetworkVar( int, m_iSecondaryAmmoType );	// "secondary" ammo index into the ammo info array
	CNetworkVar( int, m_iClip1 );				// number of shots left in the primary weapon clip, -1 it not used
	CNetworkVar( int, m_iClip2 );				// number of shots left in the secondary weapon clip, -1 it not used
	bool					m_bFiresUnderwater;		// true if this weapon can fire underwater
	float					m_fMinRange1;			// What's the closest this weapon can be used?
	float					m_fMinRange2;			// What's the closest this weapon can be used?
	float					m_fMaxRange1;			// What's the furthest this weapon can be used?
	float					m_fMaxRange2;			// What's the furthest this weapon can be used?
	bool					m_bReloadsSingly;		// Tryue if this weapon reloads 1 round at a time
	float					m_fFireDuration;		// The amount of time that the weapon has sustained firing
	int						m_iSubType;

private:
	WEAPON_FILE_INFO_HANDLE	m_hWeaponFileInfo;
	IPhysicsConstraint		*m_pConstraint;
	
	// Server only
#if !defined( CLIENT_DLL )

	// Outputs
	COutputEvent			m_OnPlayerPickup;		// Fired when the player picks up the weapon.
	COutputEvent			m_OnNPCPickup;		// Fired when an NPC picks up the weapon.

#else // Client .dll only
	int						m_iOldState;
	bool					m_bJustRestored;

	// Allow weapons resource to access m_hWeaponFileInfo directly
	friend class			WeaponsResource;

#endif // End Client .dll only
};

#endif // COMBATWEAPON_SHARED_H
