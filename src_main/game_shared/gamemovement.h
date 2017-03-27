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
#if !defined( GAMEMOVEMENT_H )
#define GAMEMOVEMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "igamemovement.h"
#include "cmodel.h"

#define CTEXTURESMAX		512			// max number of textures loaded
#define CBTEXTURENAMEMAX	13			// only load first n chars of name

struct surfacedata_t;

class CBasePlayer;

class CGameMovement : public IGameMovement
{
public:
	DECLARE_CLASS_NOBASE( CGameMovement );
	
	CGameMovement( void );
	virtual			~CGameMovement( void );

	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove );
	// Internal movement processor
	virtual void	_ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove );

	virtual const Vector&	GetPlayerMins( bool ducked ) const;
	virtual const Vector&	GetPlayerMaxs( bool ducked ) const;
	virtual const Vector&	GetPlayerViewOffset( bool ducked ) const;

protected:
	// Input/Output for this movement
	CMoveData		*mv;
	CBasePlayer		*player;
	
	int				m_nOldWaterLevel;
	int				m_nOnLadder;

	Vector			m_vecMinsNormal;
	Vector			m_vecMaxsNormal;
	Vector			m_vecMinsDucked;
	Vector			m_vecMaxsDucked;
	Vector			m_vecViewOffsetNormal;
	Vector			m_vecViewOffsetDucked;

	Vector			m_vecForward;
	Vector			m_vecRight;
	Vector			m_vecUp;

	// Texture names
	int				m_surfaceProps;
	surfacedata_t*	m_pSurfaceData;
	float			m_surfaceFriction;
	char			m_chTextureType;

protected:


	// Does most of the player movement logic.
	// Returns with origin, angles, and velocity modified in place.
	// Numtouch and touchindex[] will be set if any of the physents
	// were contacted during the move.
	void			PlayerMove(	void );

	// Set ground data, etc.
	void			FinishMove( void );

	float			CalcRoll( const QAngle &angles, const Vector &velocity, float rollangle, float rollspeed );

	void			DecayPunchAngle( void );

	void			CheckWaterJump(void );

	void			WaterMove( void );

	void			WaterJump( void );

	// Handles both ground friction and water friction
	void			Friction( void );

	void			AirAccelerate( Vector& wishdir, float wishspeed, float accel );

	virtual void	AirMove( void );
	
	virtual bool	CanAccelerate();
	virtual void	Accelerate( Vector& wishdir, float wishspeed, float accel);

	// Only used by players.  Moves along the ground when player is a MOVETYPE_WALK.
	virtual void	WalkMove( void );
		
	// Handle MOVETYPE_WALK.
	virtual void	FullWalkMove( bool bOnLadder );

	// Implement this if you want to know when the player collides during OnPlayerMove
	virtual void	OnTryPlayerMoveCollision( trace_t &tr ) {}

	// Should the step sound play?
	virtual bool	ShouldPlayStepSound( surfacedata_t *psurface, float fvol );
	virtual void	PlayStepSound( surfacedata_t *psurface, float fvol, bool force );

	// Decompoosed gravity
	void			StartGravity( void );
	void			FinishGravity( void );

	// Apply normal ( undecomposed ) gravity
	void			AddGravity( void );

	// Handle movement in noclip mode.
	void			FullNoClipMove( void );

	virtual void	CheckJumpButton( void );	// Overridden by each game.

	// Dead player flying through air., e.g.
	void			FullTossMove( void );
	
	// Player is a Observer chasing another player
	void			FullObserverMove( void );

	// Handle movement when in MOVETYPE_LADDER mode.
	void			FullLadderMove( bool bOnLadder);

	// The basic solid body movement clip that slides along multiple planes
	virtual int		TryPlayerMove( void );
	
	bool			LadderMove( void );
	inline bool		OnLadder( trace_t &trace );

	// See if the player has a bogus velocity value.
	void			CheckVelocity( void );

	// Does not change the entities velocity at all
	trace_t			PushEntity( Vector& push );

	// Slide off of the impacting object
	// returns the blocked flags:
	// 0x01 == floor
	// 0x02 == step / wall
	int				ClipVelocity( Vector& in, Vector& normal, Vector& out, float overbounce );

	// If pmove.origin is in a solid position,
	// try nudging slightly on all axis to
	// allow for the cut precision of the net coordinates
	int				CheckStuck( void );
	
	// Check if the point is in water.
	// Sets refWaterLevel and refWaterType appropriately.
	// If in water, applies current to baseVelocity, and returns true.
	bool			CheckWater( void );
	
	// Determine if player is in water, on ground, etc.
	virtual void CategorizePosition( void );

	// Speeds are multiplied by this when IN_SPEED is pressed.
	virtual float	GetSpeedCropFraction() const;

	void			CheckParameters( void );

	void			ReduceTimers( void );

	void			CheckFalling( void );

	void			PlayerWaterSounds( void );

	void			Duck( void );

	void			FinishUnDuck( void );
	void			FinishDuck( void );
	bool			CanUnduck();
	
	void			SetDuckedEyeOffset( float duckFraction );

	void			FixPlayerCrouchStuck( bool moveup );

	float			SplineFraction( float value, float scale );

	void			CategorizeGroundSurface( void );
	void			UpdateStepSound( void );

	bool			InWater( void );

	// Commander view movement
	void			IsometricMove( void );

	// Traces the player bbox as it is swept from start to end
	void			TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );

	// Tests the player position
	CBaseHandle		TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm );

	// Checks to see if we should actually jump 
	void			PlaySwimSound();

	bool			IsDead( void ) const;

	// Figures out how the constraint should slow us down
	float			ComputeConstraintSpeedFactor( void );

private:
	// Performs the collision resolution for fliers.
	void			PerformFlyCollisionResolution( trace_t &pm, Vector &move );

protected:
	Vector			m_vecProximityMins;		// Used to be globals in sv_user.cpp.
	Vector			m_vecProximityMaxs;

	float			m_fFrameTime;

//private:
	bool			m_bSpeedCropped;
};

#endif // GAMEMOVEMENT_H
