//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VEHICLE_BASE_H
#define VEHICLE_BASE_H
#ifdef _WIN32
#pragma once
#endif

#include "vphysics/vehicles.h"
#include "iservervehicle.h"
#include "fourwheelvehiclephysics.h"
#include "props.h"
#include "vehicle_sounds.h"
#include "phys_controller.h"

class CNPC_VehicleDriver;
class CFourWheelVehiclePhysics;
class CPropVehicleDriveable;
class CSoundPatch;

//-----------------------------------------------------------------------------
// Purpose: Base class for drivable vehicle handling. Contain it in your 
//			drivable vehicle.
//-----------------------------------------------------------------------------
class CBaseServerVehicle : public IServerVehicle
{
public:
	DECLARE_DATADESC();

	CBaseServerVehicle( void );
	~CBaseServerVehicle( void );

	virtual void			Precache( void );

// IVehicle
public:
	virtual CBasePlayer*	GetPassenger( int nRole = VEHICLE_DRIVER );
	virtual int				GetPassengerRole( CBasePlayer *pPassenger );
	virtual void			GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles );
	virtual bool			IsPassengerUsingStandardWeapons( int nRole = VEHICLE_DRIVER ) { return false; }
	virtual void			SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void			ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual void			FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move );
	virtual void			ItemPostFrame( CBasePlayer *pPlayer );

// IServerVehicle
public:
	virtual CBaseEntity		*GetVehicleEnt( void ) { return m_pVehicle; }
	virtual void			SetPassenger( int nRole, CBasePlayer *pPassenger );
	virtual bool			IsPassengerVisible( int nRole = VEHICLE_DRIVER ) { return false; }
	virtual bool			IsPassengerDamagable( int nRole  = VEHICLE_DRIVER ) { return true; }
	virtual bool			IsVehicleUpright( void ) { return true; }
	virtual void			GetPassengerStartPoint( int nRole, Vector *pPoint, QAngle *pAngles );
	virtual void			GetPassengerExitPoint( int nRole, Vector *pPoint, QAngle *pAngles );
	virtual Class_T			ClassifyPassenger( CBasePlayer *pPassenger, Class_T defaultClassification ) { return defaultClassification; }
	virtual float			DamageModifier ( CTakeDamageInfo &info ) { return 1.0; }
	virtual const vehicleparams_t	*GetVehicleParams( void ) { return NULL; }

	// NPC Driving
	virtual bool			NPC_CanDrive( void ) { return true; }
	virtual void			NPC_SetDriver( CNPC_VehicleDriver *pDriver ) { return; }
	virtual void			NPC_DriveVehicle( void ) { return; }
	virtual void			NPC_ThrottleCenter( void );
	virtual void			NPC_ThrottleReverse( void );
	virtual void			NPC_ThrottleForward( void );
	virtual void			NPC_Brake( void );
	virtual void			NPC_TurnLeft( float flDegrees );
	virtual void			NPC_TurnRight( float flDegrees );
	virtual void			NPC_TurnCenter( void );
	virtual void			NPC_PrimaryFire( void );
	virtual void			NPC_SecondaryFire( void );
	virtual bool			NPC_HasPrimaryWeapon( void ) { return false; }
	virtual bool			NPC_HasSecondaryWeapon( void ) { return false; }
	virtual void			NPC_AimPrimaryWeapon( Vector vecTarget ) { return; }
	virtual void			NPC_AimSecondaryWeapon( Vector vecTarget ) { return; }

	// Weapon handling
	virtual void			Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange );
	virtual void			Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange );	
	virtual float			Weapon_PrimaryCanFireAt( void );		// Return the time at which this vehicle's primary weapon can fire again
	virtual float			Weapon_SecondaryCanFireAt( void );		// Return the time at which this vehicle's secondary weapon can fire again

public:
	// Player Driving
	virtual CBaseEntity		*GetDriver( void );

	// Vehicle entering/exiting
	virtual void			ParseEntryExitAnims( void );
	virtual bool			CheckExitPoint( float yaw, int distance, Vector *pEndPoint );
	virtual int				GetEntryAnimForPoint( const Vector &vecPoint );
	virtual int				GetExitAnimToUse( void );
	virtual void			HandleEntryExitFinish( bool bExitAnimOn );

	virtual void			SetVehicle( CBaseEntity *pVehicle );
	IDrivableVehicle 		*GetDrivableVehicle( void );

	// Sound handling
	bool					Initialize( const char *pScriptName );
	virtual void			SoundStart( void );
	virtual void			SoundShutdown( float flFadeTime = 0.0 );
	virtual void			SoundUpdate( float flFrameTime, float flCurrentSpeed, bool bThrottleDown, bool bReverse );
	virtual void			PlaySound( vehiclesound iSound );
	virtual void			StopSound( vehiclesound iSound );
	virtual void 			RecalculateSoundGear( bool bThrottleDown, bool bReverse );
	virtual void 			PlaySoundForGear( vehiclesound_gear iSound, int iGear = -1 );
	virtual void 			StopSoundForGear( vehiclesound_gear iSound, int iGear = -1 );
	virtual void 			StartGearEngineLoop( vehiclesound_gear iSound );
	virtual void 			StopGearEngineLoop( float flFadeTime = 0.0 );
	void					StopSpeedSound( float flFadeTime = 0.0 );
	void					SetVehicleVolume( float flVolume ) { m_flVehicleVolume = clamp( flVolume, 0.0, 1.0 ); }

public:
	CBaseEntity			*m_pVehicle;
	IDrivableVehicle 	*m_pDrivableVehicle;

	// NPC Driving
	int								m_nNPCButtons;
	int								m_nPrevNPCButtons;
	float							m_flTurnDegrees;

	// Sound handling
	float							m_flVehicleVolume;
	vehiclesounds_t					m_vehicleSounds;
	int								m_iSoundGear;			// The sound "gear" that we're currently in
	int								m_iGearEngineLoop;		// Sound index of the engine loop we're playing
	float							m_flSpeedPercentage;
	CSoundPatch						*m_EngineSoundPatch;
	CSoundPatch						*m_SpeedSoundPatch;
	bool							m_bThrottleDown;
	bool							m_bRestartGear;

	// Entry / Exit anims
	struct entryanim_t
	{
		int		iHitboxGroup;
		char	szAnimName[128];
	};
	CUtlVector< entryanim_t >		m_EntryAnimations;

	struct exitanim_t
	{
		int		iAttachment;
		bool	bUpright;
		char	szAnimName[128];
	};
	CUtlVector< exitanim_t >		m_ExitAnimations;
	bool							m_bParsedAnimations;
	int								m_iCurrentExitAnim;
};

//-----------------------------------------------------------------------------
// Purpose: Four wheel physics vehicle server vehicle
//-----------------------------------------------------------------------------
class CFourWheelServerVehicle : public CBaseServerVehicle
{
	typedef CBaseServerVehicle BaseClass;
// IServerVehicle
public:
	virtual ~CFourWheelServerVehicle( void )
	{
	}

	virtual bool			IsVehicleUpright( void );
	virtual void			GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles );
	const vehicleparams_t	*GetVehicleParams( void );

	// NPC Driving
	void					NPC_SetDriver( CNPC_VehicleDriver *pDriver );
	void					NPC_DriveVehicle( void );

public:
	virtual void	SetVehicle( CBaseEntity *pVehicle );

private:
	CPropVehicleDriveable		*GetFourWheelVehicle( void );
	CFourWheelVehiclePhysics	*GetFourWheelVehiclePhysics( void );
};

//-----------------------------------------------------------------------------
// Purpose: Base class for four wheel physics vehicles
//-----------------------------------------------------------------------------
class CPropVehicle : public CBaseProp
{
	DECLARE_CLASS( CPropVehicle, CBaseProp );
public:
	CPropVehicle();
	virtual ~CPropVehicle();

	void SetVehicleType( unsigned int nVehicleType )			{ m_nVehicleType = nVehicleType; }
	unsigned int GetVehicleType( void )							{ return m_nVehicleType; }

	// CBaseEntity
	void Spawn();
	virtual int Restore( IRestore &restore );
	void VPhysicsUpdate( IPhysicsObject *pPhysics );
	void DrawDebugGeometryOverlays();
	int DrawDebugTextOverlays();
	void Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
	virtual void Think( void );
	CFourWheelVehiclePhysics *GetPhysics( void ) { return &m_VehiclePhysics; }

	Vector GetSmoothedVelocity( void );	//Save and update our smoothed velocity for prediction
	
	// Inputs
	void InputThrottle( inputdata_t &inputdata );
	void InputSteering( inputdata_t &inputdata );
	void InputAction( inputdata_t &inputdata );
	DECLARE_DATADESC();

protected:
	// engine sounds
	void SoundInit();
	void SoundShutdown();
	void SoundUpdate( const vehicle_operatingparams_t &params, const vehicleparams_t &vehicle );
	void CalcWheelData( vehicleparams_t &vehicle );
	void ResetControls();

protected:
	CFourWheelVehiclePhysics		m_VehiclePhysics;
	unsigned int					m_nVehicleType;
	string_t						m_vehicleScript;

private:
	Vector							m_vecSmoothedVelocity;
};

//-----------------------------------------------------------------------------
// Purpose: Drivable four wheel physics vehicles
//-----------------------------------------------------------------------------
class CPropVehicleDriveable : public CPropVehicle, public IDrivableVehicle
{
	DECLARE_CLASS( CPropVehicleDriveable, CPropVehicle );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
public:
	CPropVehicleDriveable( void );
	~CPropVehicleDriveable( void );

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual int		Restore( IRestore &restore );
	virtual void	CreateServerVehicle( void );
	virtual int		ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };
	virtual void	GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const;
	virtual void	VehicleAngleVectors( const QAngle &angles, Vector *pForward, Vector *pRight, Vector *pUp );
	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void	Think( void );
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	// Vehicle handling
	virtual bool	CanControlVehicle( void ) { return true; }
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual int		VPhysicsGetObjectList( IPhysicsObject **pList, int listMax );

	// Inputs
	void	InputLock( inputdata_t &inputdata );
	void	InputUnlock( inputdata_t &inputdata );
	void	InputTurnOn( inputdata_t &inputdata );
	void	InputTurnOff( inputdata_t &inputdata );

	// Locals
	void	ResetUseKey( CBasePlayer *pPlayer );

	// Driving
	void	DriveVehicle( CBasePlayer *pPlayer, CUserCmd *ucmd );	// Player driving entrypoint
	virtual void DriveVehicle( float flFrameTime, int iButtons, int iButtonsDown, int iButtonsReleased ); // Driving Button handling

	virtual bool IsOverturned( void );
		
	// Engine handling
	void	StartEngine( void );
	void	StopEngine( void );
	bool	IsEngineOn( void );

// IDrivableVehicle
public:
	virtual CBaseEntity *GetDriver( void );
	virtual void		ItemPostFrame( CBasePlayer *pPlayer ) { return; }
	virtual void		SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void		ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData ) { return; }
	virtual void		FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) { return; }
	virtual bool		CanEnterVehicle( CBaseEntity *pEntity );
	virtual bool		CanExitVehicle( CBaseEntity *pEntity );
	virtual void		EnterVehicle( CBasePlayer *pPlayer );
	virtual void		ExitVehicle( int iRole );
	virtual bool		PlayExitAnimation( void );

	// If this is a vehicle, returns the vehicle interface
	virtual IServerVehicle *GetServerVehicle() { return m_pServerVehicle; }

protected:
	void DestroyServerVehicle();

	// Contained IServerVehicle
	CFourWheelServerVehicle	*m_pServerVehicle;

	COutputEvent		m_playerOn;
	COutputEvent		m_playerOff;

	COutputEvent		m_pressedAttack;
	COutputEvent		m_pressedAttack2;

	COutputFloat		m_attackaxis;
	COutputFloat		m_attack2axis;

	CNetworkHandle( CBasePlayer, m_hPlayer );
public:
	CNetworkVar( int, m_nSpeed );
	CNetworkVar( int, m_nRPM );
	CNetworkVar( float, m_flThrottle );
	CNetworkVar( int, m_nBoostTimeLeft );
	CNetworkVar( int, m_nHasBoost );
	CNetworkVector( m_vecLookCrosshair );
	CNetworkVector( m_vecGunCrosshair );

	CNetworkVar( bool, m_nScannerDisabledWeapons );
	CNetworkVar( bool, m_nScannerDisabledVehicle );

	// NPC Driver
	CHandle<CNPC_VehicleDriver>	 m_hNPCDriver;
	CHandle<CKeepUpright>		 m_hKeepUpright;

protected:

	Vector		m_savedViewOffset;

	// Entering / Exiting
	bool		m_bLocked;
	float		m_flMinimumSpeedToEnterExit;
	CNetworkVar( bool, m_bEnterAnimOn );
	CNetworkVar( bool, m_bExitAnimOn );

	// Used to turn the keepupright off after a short time
	float		m_flTurnOffKeepUpright;
};

#endif // VEHICLE_BASE_H
