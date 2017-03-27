//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#if !defined( C_BASEPLAYER_H )
#define C_BASEPLAYER_H
#ifdef _WIN32
#pragma once
#endif

// Client-side CBasePlayer

#include "c_playerlocaldata.h"
#include "c_basecombatcharacter.h"
#include "playerstate.h"
#include "usercmd.h"

typedef struct dlight_s dlight_t;

class C_BaseCombatWeapon;
class C_BaseViewModel;

class C_CommandContext
{
public:
	bool			needsprocessing;

	CUserCmd		cmd;
	int				command_number;
};

//-----------------------------------------------------------------------------
// Purpose: Base Player class
//-----------------------------------------------------------------------------
class C_BasePlayer : public C_BaseCombatCharacter
{
public:
	DECLARE_CLASS( C_BasePlayer, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_BasePlayer();
	virtual			~C_BasePlayer();

	virtual void	Spawn( void );

	// IClientEntity overrides.
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	virtual void	OnRestore();

	virtual void	AddEntity( void );

	void	SetAnimationExtension( const char *pExtension );

	C_BaseViewModel	*GetViewModel( int viewmodelindex = 0 );

	// View model prediction setup
	void				CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void				CalcVehicleView(IClientVehicle *pVehicle, Vector& eyeOrigin, QAngle& eyeAngles,
							float& zNear, float& zFar, float& fov );
	void				CalcObserverView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void				GetChaseCamView(Vector& eyeOrigin, QAngle& eyeAngles);
	void				GetInEyeCamView(Vector& eyeOrigin, QAngle& eyeAngles);
	

	// Handle view smoothing when going up stairs
	void				SmoothViewOnStairs( Vector& eyeOrigin );
	float				CalcRoll (const QAngle& angles, const Vector& velocity, float rollangle, float rollspeed);
	void				CalcViewRoll( QAngle& eyeAngles );
	float				GetWaterOffset( const Vector& eyePosition );

	virtual Vector			Weapon_ShootPosition();
	virtual void			Weapon_DropPrimary( void ) {}

	Vector					GetAutoaimVector( float flDelta  );
	virtual void			FireBullets( int cShots, const Vector &vecSrc, const Vector &vecDirShooting, const Vector &vecSpread, float flDistance, int iBulletType, int iTracerFreq = 4, int firingEntID = -1, int attachmentID = -1,int iDamage = 0, C_BaseEntity *pAttacker = NULL );
	void					SetSuitUpdate(char *name, int fgroup, int iNoRepeat);

	// Input handling
	virtual void	CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *pCmd );
	virtual void	PlayerUse( void );
	CBaseEntity		*FindUseEntity( void );
	virtual bool	IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps );

	// Data handlers
	virtual bool	IsPlayer( void ) const { return true; };
	virtual bool	IsObserver() const { return (m_iObserverMode != OBS_MODE_NONE); };
	virtual int		GetHealth() const { return m_iHealth; };
	virtual int		GetObserverMode() { return m_iObserverMode; }
	virtual CBaseEntity	 * GetObserverTarget();	// returns players targer or NULL
	virtual void	ResetObserverMode();

	// Eye position..
	virtual Vector		EyePosition();
	virtual const QAngle &EyeAngles();		// Direction of eyes
	virtual const QAngle &LocalEyeAngles();		// Direction of eyes

	// Returns eye vectors
	void			EyeVectors( Vector *pForward, Vector *pRight = NULL, Vector *pUp = NULL );

	bool			IsSuitEquipped( void ) { return m_Local.m_bWearingSuit; };

	// Team handlers
	virtual void	TeamChange( int iNewTeam );

	// Flashlight
	virtual void	Flashlight( void );
	virtual void	UpdateFlashlight( void );

	// Weapon selection code
	virtual bool				IsAllowedToSwitchWeapons( void ) { return true; }
	virtual C_BaseCombatWeapon	*GetActiveWeaponForSelection( void );

	// Returns the view model if this is the local player. If you're in third person or 
	// this is a remote player, it returns the active weapon
	// (and its appropriate left/right weapon if this is TF2).
	virtual C_BaseAnimating*	GetRenderedWeaponModel();

	virtual bool				IsOverridingViewmodel( void ) { return false; };
	virtual int					DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags ) { return 0; };

	virtual float				GetDefaultAnimSpeed( void ) { return 1.0; }

	void						SetMaxSpeed( float flMaxSpeed ) { m_flMaxspeed = flMaxSpeed; }
	float						MaxSpeed() const		{ return m_flMaxspeed; }

	// Should this object cast shadows?
	virtual ShadowType_t		ShadowCastType() { return SHADOWS_NONE; }

	bool						IsLocalPlayer( void );

	// Global/static methods
	static C_BasePlayer			*GetLocalPlayer( void );

	// Called by the view model if its rendering is being overridden.
	virtual bool		ViewModel_IsTransparent( void );

	void						AddToPlayerSimulationList( C_BaseEntity *other );
	void						SimulatePlayerSimulatedEntities( void );
	void						RemoveFromPlayerSimulationList( C_BaseEntity *ent );
	void						ClearPlayerSimulationList( void );

	virtual void				PhysicsSimulate( void );
	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const { return MASK_PLAYERSOLID; }

	// Prediction stuff
	virtual bool				ShouldPredict( void );

	virtual void				PreThink( void );
	virtual void				PostThink( void );

	virtual void				ItemPreFrame( void );
	virtual void				ItemPostFrame( void );
	virtual void				AbortReload( void );

	virtual void				SelectLastItem(void);
	virtual void				Weapon_SetLast( C_BaseCombatWeapon *pWeapon );
	virtual bool				Weapon_ShouldSetLast( C_BaseCombatWeapon *pOldWeapon, C_BaseCombatWeapon *pNewWeapon ) { return true; }
	virtual bool				Weapon_ShouldSelectItem( C_BaseCombatWeapon *pWeapon );
	virtual	bool				Weapon_Switch( C_BaseCombatWeapon *pWeapon, int viewmodelindex = 0 );		// Switch to given weapon if has ammo (false if failed)
	virtual C_BaseCombatWeapon *GetLastWeapon( void ) { return m_hLastWeapon.Get(); }
	void						ResetAutoaim( void );
	virtual void 				SelectItem( const char *pstr, int iSubType = 0 );

	virtual void				UpdateClientData( void );

	virtual void				UpdateFOV( void );
	virtual float				CurrentFOV( void );
	float						GetFOV() const; // Note: this returns m_Local.m_iFOV, and the FOV may be in the
												// middle of interpolating, so CurrentFOV() can return a different value.
												

	virtual void				ViewPunch( const QAngle &angleOffset );
	void						ViewPunchReset( float tolerance = 0 );

	void						UpdateButtonState( int nUserCmdButtonMask );

	virtual void				Simulate();

	virtual bool				ShouldDraw();

	// Called when not in tactical mode. Allows view to be overriden for things like driving a tank.
	virtual void				OverrideView( CViewSetup *pSetup );

	// returns the player name
	const char *		GetPlayerName();

	// Is the player dead?
	bool				IsPlayerDead();

	// Vehicles...
	IClientVehicle			*GetVehicle();

	bool			IsInAVehicle() const	{ return ( NULL != m_hVehicle.Get() ) ? true : false; }
	virtual void	SetVehicleRole( int nRole );
	void					LeaveVehicle( void );

	bool					UsingStandardWeaponsInVehicle( void );

	virtual void			SetAnimation( PLAYER_ANIM playerAnim );

	float					GetTimeBase( void ) const;
	float					GetFinalPredictedTime() const;

	bool					IsInVGuiInputMode() const;

	C_CommandContext		*GetCommandContext()
	{
		return &m_CommandContext;
	}

	int						GetExplosionEffects( void ) const;

	const QAngle& GetPunchAngle();
	void SetPunchAngle( const QAngle &angle );

	float					GetWaterJumpTime() const;

	// CS wants to allow small FOVs for zoomed-in AWPs.
	virtual float GetMinFOV() const;

	virtual void ResetLatched( void );
	virtual bool Interpolate( float currentTime );


public:
	// Data for only the local player
	CNetworkVarEmbedded( CPlayerLocalData, m_Local );

	// Data common to all other players, too
	CPlayerState			pl;

	// For weapon prediction
	bool			m_fOnTarget;		//Is the crosshair on a target?
	
	int				m_iObserverMode;	// if in spectator mode != 0
	EHANDLE			m_hObserverTarget; 
	
	char			m_szAnimExtension[32];

	int				m_afButtonLast;
	int				m_afButtonPressed;
	int				m_afButtonReleased;

	int				m_nButtons;

	CUserCmd		*m_pCurrentCommand;

	// Movement constraints
	EHANDLE			m_hConstraintEntity;
	Vector			m_vecConstraintCenter;
	float			m_flConstraintRadius;
	float			m_flConstraintWidth;
	float			m_flConstraintSpeedFactor;

protected:
	// Check to see if we're in vgui input mode...
	void DetermineVguiInputMode( CUserCmd *pCmd );

	// Used by prediction, sets the view angles for the player
	void SetLocalViewAngles( const QAngle &viewAngles );

protected:
	// Did we just enter a vehicle this frame?
	bool			JustEnteredVehicle();

// DATA
private:
	// Make sure no one calls this...
	C_BasePlayer& operator=( const C_BasePlayer& src );
	C_BasePlayer( const C_BasePlayer & ); // not defined, not accessible

	// Vehicle stuff.
	EHANDLE			m_hVehicle;
	EHANDLE			m_hOldVehicle;
	
	float			m_flMaxspeed;
	int				m_iHealth;

	CInterpolatedVar< Vector >	m_iv_vecViewOffset;

	// Not replicated
	Vector			m_vecWaterJumpVel;
	float			m_flWaterJumpTime;  // used to be called teleport_time
	int				m_nImpulse;

	float			m_flStepSoundTime;
	float			m_flSwimSoundTime;
	Vector			m_vecLadderNormal;

	QAngle			m_vecOldViewAngles;
	int				m_flPhysics;

	int				m_nTickBase;
	int				m_nFinalPredictedTick;

	EHANDLE			m_pCurrentVguiScreen;

	// Player flashlight dynamic light pointers
	dlight_t*		m_pModelLight;
	dlight_t*		m_pEnvironmentLight;
	dlight_t*		m_pBrightLight;

	typedef CHandle<C_BaseCombatWeapon> CBaseCombatWeaponHandle;
	CNetworkVar( CBaseCombatWeaponHandle, m_hLastWeapon );

	CUtlVector< CHandle< C_BaseEntity > > m_SimulatedByThisPlayer;

	CHandle< C_BaseViewModel >	m_hViewModel[ MAX_VIEWMODELS ];

	int						m_nExplosionFX;
	int						m_nOldExplosionFX;

	float					m_flOldPlayerZ;

	// For UI purposes...
	int				m_iOldAmmo[ MAX_AMMO_TYPES ];

	C_CommandContext		m_CommandContext;

	friend class CPrediction;

	// HACK FOR TF2 Prediction
	friend class CTFGameMovementRecon;
	friend class CGameMovement;
	friend class CTFGameMovement;
	friend class CHL1GameMovement;
};

EXTERN_RECV_TABLE(DT_BasePlayer);


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline C_BasePlayer *ToBasePlayer( C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

#if _DEBUG
	return dynamic_cast<C_BasePlayer *>( pEntity );
#else
	return static_cast<C_BasePlayer *>( pEntity );
#endif
}

inline IClientVehicle *C_BasePlayer::GetVehicle() 
{ 
	C_BaseEntity *pVehicleEnt = m_hVehicle.Get();
	return pVehicleEnt ? pVehicleEnt->GetClientVehicle() : NULL;
}

#endif // C_BASEPLAYER_H
