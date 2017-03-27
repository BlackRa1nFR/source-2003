//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Implements shared baseplayer class functionality
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "movevars_shared.h"

#if defined( CLIENT_DLL )

#include "iclientvehicle.h"
#include "prediction.h"
#include "c_basedoor.h"
#include "c_world.h"

#else

#include "iservervehicle.h"
#include "trains.h"
#include "world.h"
#include "doors.h"

extern int TrainSpeed(int iSpeed, int iMax);

#endif

#include "in_buttons.h"
#include "engine/IEngineSound.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CBasePlayer::GetTimeBase( void ) const
{
	return m_nTickBase * TICK_RATE;
}

//-----------------------------------------------------------------------------
// Purpose: Called every usercmd by the player PreThink
//-----------------------------------------------------------------------------
void CBasePlayer::ItemPreFrame()
{
    if ( gpGlobals->curtime < m_flNextAttack )
	{
		return;
	}

	if (!GetActiveWeapon())
		return;

#if defined( CLIENT_DLL )
	// Not predicting this weapon
	if ( !GetActiveWeapon()->IsPredicted() )
		return;
#endif

	GetActiveWeapon()->ItemPreFrame( );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::UsingStandardWeaponsInVehicle( void )
{
	Assert( IsInAVehicle() );
#if !defined( CLIENT_DLL )
	IServerVehicle *pVehicle = GetVehicle();
#else
	IClientVehicle *pVehicle = GetVehicle();
#endif
	Assert( pVehicle );
	if ( !pVehicle )
		return true;

	// NOTE: We *have* to do this before ItemPostFrame because ItemPostFrame
	// may dump us out of the vehicle
	int nRole = pVehicle->GetPassengerRole( this );
	bool bUsingStandardWeapons = pVehicle->IsPassengerUsingStandardWeapons( nRole );

	// Fall through and check weapons, etc. if we're using them 
	if (!bUsingStandardWeapons )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Called every usercmd by the player PostThink
//-----------------------------------------------------------------------------
void CBasePlayer::ItemPostFrame()
{
	Vector dummyOrigin;
	QAngle dummyAngles;
	float dummyFov;

	// Don't process items while in a vehicle.
	if ( GetVehicle() )
	{
#if defined( CLIENT_DLL )
		IClientVehicle *pVehicle = GetVehicle();
#else
		IServerVehicle *pVehicle = GetVehicle();
#endif

		{
			float dummyZnear, dummyZfar;
			// Put viewmodels into basically correct place based on new polayer origin
			CalcVehicleView( pVehicle, dummyOrigin, dummyAngles, dummyZnear, dummyZfar, dummyFov );
		}

		bool bUsingStandardWeapons = UsingStandardWeaponsInVehicle();

#if defined( CLIENT_DLL )
		if ( pVehicle->IsPredicted() )
#endif
		{
			pVehicle->ItemPostFrame( this );
		}

		if (!bUsingStandardWeapons || !GetVehicle())
			return;
	}
	else
	{
		CalcPlayerView( dummyOrigin, dummyAngles, dummyFov );
	}

#if !defined( CLIENT_DLL )
	// check if the player is using something
	if ( m_hUseEntity != NULL )
	{
		PlayerUse();
		Assert( !IsInAVehicle() );
		return;
	}
#endif

    if ( gpGlobals->curtime < m_flNextAttack )
	{
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->ItemBusyFrame();
		}
		return;
	}

#if !defined( CLIENT_DLL )
	ImpulseCommands();
#endif

	if (!GetActiveWeapon())
	{
		return;
	}

	if ( IsInAVehicle() && !UsingStandardWeaponsInVehicle() )
		return;

#if defined( CLIENT_DLL )
	// Not predicting this weapon
	if ( !GetActiveWeapon()->IsPredicted() )
		return;
#endif

	GetActiveWeapon()->ItemPostFrame( );
}


//-----------------------------------------------------------------------------
// Eye angles
//-----------------------------------------------------------------------------
const QAngle &CBasePlayer::EyeAngles( )
{
	// NOTE: Viewangles are measured *relative* to the parent's coordinate system
	CBaseEntity *pMoveParent = const_cast<CBasePlayer*>(this)->GetMoveParent();

	if ( !pMoveParent )
	{
		return pl.v_angle;
	}

	// FIXME: Cache off the angles?
	matrix3x4_t eyesToParent, eyesToWorld;
	AngleMatrix( pl.v_angle, eyesToParent );
	ConcatTransforms( pMoveParent->EntityToWorldTransform(), eyesToParent, eyesToWorld );

	static QAngle angEyeWorld;
	MatrixAngles( eyesToWorld, angEyeWorld );
	return angEyeWorld;
}


const QAngle &CBasePlayer::LocalEyeAngles()
{
	return pl.v_angle;
}


//-----------------------------------------------------------------------------
// Actual Eye position + angles
//-----------------------------------------------------------------------------
Vector CBasePlayer::EyePosition( )
{
#ifdef CLIENT_DLL
	IClientVehicle *pVehicle = GetVehicle();
#else
	IServerVehicle *pVehicle = GetVehicle();
#endif
	if ( pVehicle )
	{
		Assert( pVehicle );
		int nRole = pVehicle->GetPassengerRole( this );

		Vector vecEyeOrigin;
		QAngle angEyeAngles;
		pVehicle->GetVehicleViewPosition( nRole, &vecEyeOrigin, &angEyeAngles );
		return vecEyeOrigin;
	}
	else
	{
		return BaseClass::EyePosition();
	}
}

//-----------------------------------------------------------------------------
// Returns eye vectors
//-----------------------------------------------------------------------------
void CBasePlayer::EyeVectors( Vector *pForward, Vector *pRight, Vector *pUp )
{
	AngleVectors( EyeAngles(), pForward, pRight, pUp );
}

void CBasePlayer::UpdateButtonState( int nUserCmdButtonMask )
{
	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = m_nButtons;

	// Get button states
	m_nButtons = nUserCmdButtonMask;
 	int buttonsChanged = m_afButtonLast ^ m_nButtons;
	
	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed =  buttonsChanged & m_nButtons;		// The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~m_nButtons);	// The ones not down are "released"
}


Vector CBasePlayer::Weapon_ShootPosition( )
{
	return EyePosition();
}

void CBasePlayer::SetAnimationExtension( const char *pExtension )
{
	Q_strncpy( m_szAnimExtension, pExtension, sizeof(m_szAnimExtension) );
}


//-----------------------------------------------------------------------------
// Purpose: Set the weapon to switch to when the player uses the 'lastinv' command
//-----------------------------------------------------------------------------
void CBasePlayer::Weapon_SetLast( CBaseCombatWeapon *pWeapon )
{
	m_hLastWeapon = pWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Override base class so player can reset autoaim
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CBasePlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex /*=0*/ ) 
{
	if ( BaseClass::Weapon_Switch( pWeapon, viewmodelindex ))
	{
		CBaseViewModel *pViewModel = GetViewModel( viewmodelindex );
		Assert( pViewModel );
		if ( pViewModel )
			pViewModel->m_fEffects &= ~EF_NODRAW;
		ResetAutoaim( );
		return true;
	}
	return false;
}

void CBasePlayer::SelectLastItem(void)
{
	if ( m_hLastWeapon.Get() == NULL )
		return;

	if ( GetActiveWeapon() && !GetActiveWeapon()->CanHolster() )
		return;

	SelectItem( m_hLastWeapon.Get()->GetClassname(), m_hLastWeapon.Get()->GetSubType() );
}


//-----------------------------------------------------------------------------
// Purpose: Abort any reloads we're in
//-----------------------------------------------------------------------------
void CBasePlayer::AbortReload( void )
{
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->AbortReload();
	}
}

void CBasePlayer::AddToPlayerSimulationList( CBaseEntity *other )
{
	CHandle< CBaseEntity > h;
	h = other;
	// Already in list
	if ( m_SimulatedByThisPlayer.Find( h ) != m_SimulatedByThisPlayer.InvalidIndex() )
		return;

	Assert( other->IsPlayerSimulated() );

	m_SimulatedByThisPlayer.AddToTail( h );
}

//-----------------------------------------------------------------------------
// Purpose: Fixme, this should occur if the player fails to drive simulation
//  often enough!!!
// Input  : *other - 
//-----------------------------------------------------------------------------
void CBasePlayer::RemoveFromPlayerSimulationList( CBaseEntity *other )
{
	if ( !other )
		return;

	Assert( other->IsPlayerSimulated() );
	Assert( other->GetSimulatingPlayer() == this );

	CHandle< CBaseEntity > h;
	h = other;

	m_SimulatedByThisPlayer.FindAndRemove( h );
}

void CBasePlayer::SimulatePlayerSimulatedEntities( void )
{
	int c = m_SimulatedByThisPlayer.Size();
	int i;

	for ( i = c - 1; i >= 0; i-- )
	{
		CHandle< CBaseEntity > h;
		
		h = m_SimulatedByThisPlayer[ i ];

		CBaseEntity *e = h;
		if ( !e || !e->IsPlayerSimulated() )
		{
			m_SimulatedByThisPlayer.FindAndRemove( h );
			continue;
		}

#if defined( CLIENT_DLL )
		if ( e->IsClientCreated() && !prediction->IsFirstTimePredicted() )
		{
			continue;
		}
#endif
		Assert( e->IsPlayerSimulated() );
		Assert( e->GetSimulatingPlayer() == this );

		e->PhysicsSimulate();
	}

	// Loop through all entities again, checking their untouch if flagged to do so
	c = m_SimulatedByThisPlayer.Size();

	for ( i = c - 1; i >= 0; i-- )
	{
		CHandle< CBaseEntity > h;
		
		h = m_SimulatedByThisPlayer[ i ];

		CBaseEntity *e = h;
		if ( !e || !e->IsPlayerSimulated() )
		{
			m_SimulatedByThisPlayer.FindAndRemove( h );
			continue;
		}

#if defined( CLIENT_DLL )
		if ( e->IsClientCreated() && !prediction->IsFirstTimePredicted() )
		{
			continue;
		}
#endif

		Assert( e->IsPlayerSimulated() );
		Assert( e->GetSimulatingPlayer() == this );

		if ( !e->GetCheckUntouch() )
			continue;

		e->PhysicsCheckForEntityUntouch();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::ClearPlayerSimulationList( void )
{
	int c = m_SimulatedByThisPlayer.Size();
	int i;

	for ( i = c - 1; i >= 0; i-- )
	{
		CHandle< CBaseEntity > h;
		
		h = m_SimulatedByThisPlayer[ i ];
		CBaseEntity *e = h;
		if ( e )
		{
			e->UnsetPlayerSimulated();
		}
	}

	m_SimulatedByThisPlayer.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Return true if we should allow selection of the specified item
//-----------------------------------------------------------------------------
bool CBasePlayer::Weapon_ShouldSelectItem( CBaseCombatWeapon *pWeapon )
{
	return ( pWeapon != GetActiveWeapon() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::SelectItem( const char *pstr, int iSubType )
{
	if (!pstr)
		return;

	CBaseCombatWeapon *pItem = Weapon_OwnsThisType( pstr, iSubType );

	if (!pItem)
		return;

	if ( !Weapon_ShouldSelectItem( pItem ) )
		return;

	// FIX, this needs to queue them up and delay
	// Make sure the current weapon can be holstered
	if ( GetActiveWeapon() )
	{
		if ( !GetActiveWeapon()->CanHolster() )
			return;

		ResetAutoaim( );

		if ( Weapon_ShouldSetLast( GetActiveWeapon(), pItem ) )
		{
			Weapon_SetLast( GetActiveWeapon()->GetLastWeapon() );
		}
	}

	Weapon_Switch( pItem );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CBasePlayer::FindUseEntity()
{
	Vector forward;
	EyeVectors( &forward, NULL, NULL );

	trace_t tr;
	// Search for objects in a sphere (tests for entities that are not solid, yet still useable)
	Vector searchCenter = EyePosition();
	// NOTE: Some debris objects are useable too, so hit those as well
	UTIL_TraceLine( searchCenter, searchCenter + forward * 1024, MASK_SOLID | CONTENTS_DEBRIS, this, COLLISION_GROUP_NONE, &tr );
	
	// try the hit entity if there is one, or the ground entity if there isn't.
	CBaseEntity *pNearest = NULL;
	CBaseEntity *pObject = tr.m_pEnt;
	float nearestDot = CONE_90_DEGREES;
	if ( IsUseableEntity(pObject, 0) )
	{
		float dist = (tr.endpos - tr.startpos).Length();
		if ( dist < PLAYER_USE_RADIUS )
			return pObject;
		
#ifndef CLIENT_DLL
		// only NPCs can be used long-range
		if ( pObject->MyNPCPointer() )
		{
			pNearest = pObject;
			nearestDot = CONE_45_DEGREES;
		}
#endif
	}


	// check ground entity first
	// if you've got a useable ground entity, then shrink the cone of this search to 45 degrees
	// otherwise, search out in a 90 degree cone (hemisphere)
	if ( GetGroundEntity() && IsUseableEntity(GetGroundEntity(), FCAP_USE_ONGROUND) )
	{
		pNearest = GetGroundEntity();
		nearestDot = CONE_45_DEGREES;
	}

	for ( CEntitySphereQuery sphere( searchCenter, PLAYER_USE_RADIUS ); pObject = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		if ( !pObject )
			continue;

		if ( IsUseableEntity( pObject, FCAP_USE_IN_RADIUS ) )
		{
			// see if it's more roughly in front of the player than previous guess
			Vector point;
			CalcClosestPointOnAABB( pObject->GetAbsMins(), pObject->GetAbsMaxs(), searchCenter, point );

			Vector dir = point - searchCenter;
			VectorNormalize(dir);
			float dot = DotProduct( dir, forward );
			if ( dot > nearestDot )
			{
				pNearest = pObject;
				nearestDot = dot;
			}
		}
	}

	return pNearest;
}

//-----------------------------------------------------------------------------
// Purpose: Handles USE keypress
//-----------------------------------------------------------------------------
void CBasePlayer::PlayerUse ( void )
{
	// Was use pressed or released?
	if ( ! ((m_nButtons | m_afButtonPressed | m_afButtonReleased) & IN_USE) )
		return;

#ifndef CLIENT_DLL
	if ( m_afButtonPressed & IN_USE )
	{
		// Controlling some latched entity?
		if ( ClearUseEntity() )
		{
			return;
		}
		else
		{
			if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
			{
				m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
			else
			{	// Start controlling the train!
				CBaseEntity *pTrain = GetGroundEntity();
				if ( pTrain && !(m_nButtons & IN_JUMP) && (GetFlags() & FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pTrain->OnControls(this) )
				{
					m_afPhysicsFlags |= PFLAG_DIROVERRIDE;
					m_iTrain = TrainSpeed(pTrain->m_flSpeed, ((CFuncTrackTrain*)pTrain)->GetMaxSpeed());
					m_iTrain |= TRAIN_NEW;
					EmitSound( "Player.UseTrain" );
					return;
				}
			}
		}
	}
#endif

#ifndef CLIENT_DLL	
	CBaseEntity *pUseEntity = FindUseEntity();

	// Found an object
	if ( pUseEntity )
	{
		//!!!UNDONE: traceline here to prevent +USEing buttons through walls			

		int caps = pUseEntity->ObjectCaps();
		variant_t emptyVariant;
		if ( ( (m_nButtons & IN_USE) && (caps & FCAP_CONTINUOUS_USE) ) || ( (m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE|FCAP_ONOFF_USE)) ) )
		{
			if ( caps & FCAP_CONTINUOUS_USE )
			{
				m_afPhysicsFlags |= PFLAG_USING;
			}

			if ( pUseEntity->ObjectCaps() & FCAP_ONOFF_USE )
			{
				pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_ON );
			}
			else
			{
				pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );
			}
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ( (m_afButtonReleased & IN_USE) && (pUseEntity->ObjectCaps() & FCAP_ONOFF_USE) )	// BUGBUG This is an "off" use
		{
			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_OFF );
		}
	}
	else if ( m_afButtonPressed & IN_USE )
	{
		EmitSound( "Player.UseDeny" );
	}
#endif
}

ConVar	sv_suppress_viewpunch( "sv_suppress_viewpunch", "0", FCVAR_REPLICATED );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::ViewPunch( const QAngle &angleOffset )
{
	//See if we're suppressing the view punching
	//FIXME: Multiplayer shouldn't allow this
	if ( sv_suppress_viewpunch.GetBool() )
		return;

	m_Local.m_vecPunchAngleVel += angleOffset * 20;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::ViewPunchReset( float tolerance )
{
	if ( tolerance != 0 )
	{
		tolerance *= tolerance;	// square
		float check = m_Local.m_vecPunchAngleVel->LengthSqr() + m_Local.m_vecPunchAngle->LengthSqr();
		if ( check > tolerance )
			return;
	}
	m_Local.m_vecPunchAngle = vec3_angle;
	m_Local.m_vecPunchAngleVel = vec3_angle;
}

#if defined( CLIENT_DLL )

#include "iviewrender.h"
#include "ivieweffects.h"

#endif

static ConVar smoothstairs( "smoothstairs", "1", FCVAR_REPLICATED, "Smooth player eye z coordinate when climbing stairs." );

//-----------------------------------------------------------------------------
// Handle view smoothing when going up stairs
//-----------------------------------------------------------------------------
void CBasePlayer::SmoothViewOnStairs( Vector& eyeOrigin )
{
	float flCurrentPlayerZ = GetAbsOrigin()[2];

	// Smooth out stair step ups
	if ( ( GetGroundEntity() != NULL ) && ( flCurrentPlayerZ - m_flOldPlayerZ > 0 ) && smoothstairs.GetBool() )
	{
		float steptime = gpGlobals->frametime;
		if (steptime < 0)
		{
			steptime = 0;
		}

		m_flOldPlayerZ += steptime * 150;
		if (m_flOldPlayerZ > flCurrentPlayerZ)
		{
			m_flOldPlayerZ = flCurrentPlayerZ;
		}
		if (flCurrentPlayerZ - m_flOldPlayerZ > 18)
		{
			m_flOldPlayerZ = flCurrentPlayerZ - 18;
		}

		eyeOrigin[2] += m_flOldPlayerZ - flCurrentPlayerZ;
	}
	else
	{
		m_flOldPlayerZ = flCurrentPlayerZ;
	}
}

static bool IsWaterContents( int contents )
{
	if ( contents & MASK_WATER )
		return true;

//	if ( contents & CONTENTS_TESTFOGVOLUME )
//		return true;

	return false;
}

void CBasePlayer::ResetObserverMode()
{

	m_hObserverTarget = NULL;
	m_iObserverMode = OBS_MODE_NONE;

#ifndef CLIENT_DLL
	m_iObserverLastMode = OBS_MODE_ROAMING;
	m_bForcedObserverMode = false;
	m_hObserverTarget = NULL;
	m_afPhysicsFlags &= ~PFLAG_OBSERVER;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Check for problems around water, move the viewer artificially if necessary 
//  -- this prevents drawing errors in GL due to waves
// Input  : *predicted_origin - 
// Output : float
//-----------------------------------------------------------------------------
float CBasePlayer::GetWaterOffset( const Vector& eyePosition )
{
	float waterOffset = 0.0f;

	if ( GetWaterLevel() <= 1 )
		return waterOffset;

	int		i, waterDist; 
	int		contents;

	Vector	point;

	// BUGBUG: Wave height is in the shader now.  This code won't work!!!

	/*
	IMaterial *pWaterMaterial = render->GetFogVolumeMaterial( visibleFogVolume );
	if( pWaterMaterial )
	{
		IMaterialVar *waveheight = pWaterMaterial->FindVar( "$xxx", NULL, false );
	*/

	waterDist = sv_waterdist.GetFloat();

	IHandleEntity *pHandleEntity = NULL;
	contents = enginetrace->GetPointContents( GetAbsOrigin(), &pHandleEntity );
	if ( pHandleEntity )
	{
		CWorld *world = dynamic_cast< CWorld * >( pHandleEntity );
		if ( world )
		{
			if ( world->GetModel() != NULL )
			{
				// Add in wave height
				waterDist += ( world->GetWaveHeight() * 2 );	
			}
		}
		else
		{
			CBaseDoor *fw = dynamic_cast< CBaseDoor * >( pHandleEntity );
			if ( fw && fw->GetModel() != NULL )
			{
				// Add in wave height
				waterDist += ( fw->m_flWaveHeight * 2 );	
			}
		}
	}

	VectorCopy( eyePosition, point );

	if ( GetWaterLevel() == 2 )	// Eyes are above water, make sure we're above the waves
	{
		point[2] -= waterDist;
		for ( i = 0; i < waterDist; i++ )
		{
			contents = enginetrace->GetPointContents( point );
			if ( !IsWaterContents( contents ) )
				break;
			point[2] += 1;
		}
		waterOffset = (point[2] + waterDist) - eyePosition[2];
	}
	else
	{
		// eyes are under water.  Make sure we're far enough under
		point[2] += waterDist;

		for ( i = 0; i < waterDist; i++ )
		{
			contents = enginetrace->GetPointContents( point );
			if ( IsWaterContents( contents ) )
				break;
			point[2] -= 1;
		}
		waterOffset = (point[2] - waterDist) - eyePosition[2];
	}

	/*
	if ( IsClient() )
	{
		Msg( "%i wl %i waterdist %f wateroffset %f\n",
			gpGlobals->framecount,
			(int)GetWaterLevel(),
			(float)waterDist,
			(float)waterOffset );
	}
	*/

	return waterOffset;
}

void CBasePlayer::CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
#if defined( CLIENT_DLL )
	if ( !prediction->InPrediction() )
	{
		// FIXME: Move into prediction
		view->DriftPitch();
	}
#endif

	VectorCopy( EyePosition(), eyeOrigin );

	VectorCopy( EyeAngles(), eyeAngles );


#if defined( CLIENT_DLL )
	if ( !prediction->InPrediction() )
#endif
	{
		SmoothViewOnStairs( eyeOrigin );
	}

	// Snack off the origin before bob + water offset are applied
	Vector vecBaseEyePosition = eyeOrigin;

	CalcViewRoll( eyeAngles );

	// Apply punch angle
	VectorAdd( eyeAngles, m_Local.m_vecPunchAngle, eyeAngles );

#if defined( CLIENT_DLL )
	if ( !prediction->InPrediction() )
	{
		// Shake it up baby!
		vieweffects->CalcShake();
		vieweffects->ApplyShake( eyeOrigin, eyeAngles, 1.0 );
	}
#endif

	// Determine water offset
	float waterOffset = GetWaterOffset( eyeOrigin );

	// Hack for being in water
	eyeOrigin[2] += waterOffset;
	vecBaseEyePosition[2] += waterOffset;

	for ( int i = 0; i < MAX_VIEWMODELS; i++ )
	{
		CBaseViewModel *vm = GetViewModel( i );
		if ( !vm )
			continue;
	
		vm->CalcViewModelView( this, vecBaseEyePosition, 
			eyeAngles );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The main view setup function for vehicles
//-----------------------------------------------------------------------------
void CBasePlayer::CalcVehicleView( 
#if defined( CLIENT_DLL )
	IClientVehicle *pVehicle, 
#else
	IServerVehicle *pVehicle,
#endif
	Vector& eyeOrigin, QAngle& eyeAngles,
	float& zNear, float& zFar, float& fov )
{
	Assert( pVehicle );

	// Initialize with default origin + angles
	int nRole = pVehicle->GetPassengerRole( this );
	pVehicle->GetVehicleViewPosition( nRole, &eyeOrigin, &eyeAngles );

#if defined( CLIENT_DLL )
	pVehicle->GetVehicleFOV( fov );

	// Allows the vehicle to change the clip planes
	pVehicle->GetVehicleClipPlanes( zNear, zFar );
#endif

	// Snack off the origin before bob + water offset are applied
	Vector vecBaseEyePosition = eyeOrigin;

	CalcViewRoll( eyeAngles );

	// Apply punch angle
	VectorAdd( eyeAngles, m_Local.m_vecPunchAngle, eyeAngles );

#if defined( CLIENT_DLL )
	if ( !prediction->InPrediction() )
	{
		// Shake it up baby!
		vieweffects->CalcShake();
		vieweffects->ApplyShake( eyeOrigin, eyeAngles, 1.0 );
	}
#endif

	for ( int i = 0; i < MAX_VIEWMODELS; i++ )
	{
		CBaseViewModel *vm = GetViewModel( i );
		if ( !vm )
			continue;

		vm->CalcViewModelView( this, vecBaseEyePosition, eyeAngles );
	}
}

void CBasePlayer::CalcObserverView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
#if defined( CLIENT_DLL )
	switch ( m_iObserverMode )
	{

		case OBS_MODE_ROAMING	:	// just copy current position
		case OBS_MODE_FIXED		:	VectorCopy( EyePosition(), eyeOrigin );
									VectorCopy( EyeAngles(), eyeAngles );
									break;

		case OBS_MODE_IN_EYE	:	GetInEyeCamView( eyeOrigin, eyeAngles );
									break;

		case OBS_MODE_CHASE		:	GetChaseCamView( eyeOrigin, eyeAngles );
									break;
	}
#else
	// on server just copy target postions, final view positions will be calculated on client
	VectorCopy( EyePosition(), eyeOrigin );
	VectorCopy( EyeAngles(), eyeAngles );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Compute roll angle for a particular lateral velocity
// Input  : angles - 
//			velocity - 
//			rollangle - 
//			rollspeed - 
// Output : float CViewRender::CalcRoll
//-----------------------------------------------------------------------------
float CBasePlayer::CalcRoll (const QAngle& angles, const Vector& velocity, float rollangle, float rollspeed)
{
    float   sign;
    float   side;
    float   value;
	
	Vector  forward, right, up;
	
    AngleVectors (angles, &forward, &right, &up);
	
	// Get amount of lateral movement
    side = DotProduct( velocity, right );
	// Right or left side?
    sign = side < 0 ? -1 : 1;
    side = fabs(side);
    
	value = rollangle;
	// Hit 100% of rollangle at rollspeed.  Below that get linear approx.
    if ( side < rollspeed )
	{
		side = side * value / rollspeed;
	}
    else
	{
		side = value;
	}

	// Scale by right/left sign
    return side*sign;
}

//-----------------------------------------------------------------------------
// Purpose: Determine view roll, including data kick
//-----------------------------------------------------------------------------
void CBasePlayer::CalcViewRoll( QAngle& eyeAngles )
{
	if ( GetMoveType() == MOVETYPE_NOCLIP )
		return;

	float side = CalcRoll( GetAbsAngles(), GetAbsVelocity(), sv_rollangle.GetFloat(), sv_rollspeed.GetFloat() );
	eyeAngles[ROLL] += side;
}
