//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "SoundEmitterSystemBase.h"

#if !defined( CLIENT_DLL )

// Game DLL Headers
#include "soundent.h"

#endif

CBaseCombatWeapon::CBaseCombatWeapon()
{
	// Constructor must call this
	// CONSTRUCT_PREDICTABLE( CBaseCombatWeapon );

	// Some default values.  There should be set in the particular weapon classes
	m_fMinRange1		= 65;
	m_fMinRange2		= 65;
	m_fMaxRange1		= 1024;
	m_fMaxRange2		= 1024;

	m_bReloadsSingly	= false;

	// Defaults to zero
	m_nViewModelIndex	= 0;

	m_pConstraint		= NULL;

#if defined( CLIENT_DLL )
	m_iState = m_iOldState = WEAPON_NOT_CARRIED;
	m_iClip1 = -1;
	m_iClip2 = -1;
	m_iPrimaryAmmoType = -1;
	m_iSecondaryAmmoType = -1;
#endif
#if !defined( CLIENT_DLL )
	OnBaseCombatWeaponCreated( this );
#endif

	m_hWeaponFileInfo = GetInvalidWeaponInfoHandle();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBaseCombatWeapon::~CBaseCombatWeapon( void )
{
#if !defined( CLIENT_DLL )
	//Remove our constraint, if we have one
	if ( m_pConstraint != NULL )
	{
		physenv->DestroyConstraint( m_pConstraint );
		m_pConstraint = NULL;
	}
	OnBaseCombatWeaponDestroyed( this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Set mode to world model and start falling to the ground
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::Spawn( void )
{
	Precache();

	SetSize( Vector( -24, -24, -24), Vector(24, 24, 24) );
	SetSolid( SOLID_BBOX );
	m_flNextEmptySoundTime = 0.0f;

	m_iState = WEAPON_NOT_CARRIED;
	// Assume 
	m_nViewModelIndex = 0;

	// If I use clips, set my clips to the default
	if ( UsesClipsForAmmo1() )
	{
		m_iClip1 = GetDefaultClip1();
	}
	else
	{
		m_iClip1 = WEAPON_NOCLIP;
	}
	if ( UsesClipsForAmmo2() )
	{
		m_iClip2 = GetDefaultClip2();
	}
	else
	{
		m_iClip2 = WEAPON_NOCLIP;
	}

	SetModel( GetWorldModel() );

#if !defined( CLIENT_DLL )
	FallInit();
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	// Reset size after SetModel() stomped it.
	SetSize( Vector( -24, -24, -24), Vector(24, 24, 24) );
	m_takedamage = DAMAGE_EVENTS_ONLY;

	// Default to non-removeable, because we don't want the
	// game_weapon_manager entity to remove weapons that have
	// been hand-placed by level designers. We only want to remove
	// weapons that have been dropped by NPC's.
	SetRemoveable( false );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::Precache( void )
{
#if defined( CLIENT_DLL )
	Assert( Q_strlen( GetClassname() ) > 0 );
	// Msg( "Client got %s\n", GetClassname() );
#endif
	m_iPrimaryAmmoType = m_iSecondaryAmmoType = -1;

	// Add this weapon to the weapon registry, and get our index into it
	// Get weapon data from script file
	if ( ReadWeaponDataFromFileForSlot( filesystem, GetClassname(), &m_hWeaponFileInfo ) )
	{
		// Get the ammo indexes for the ammo's specified in the data file
		if ( GetWpnData().szAmmo1[0] )
		{
			m_iPrimaryAmmoType = GetAmmoDef()->Index( GetWpnData().szAmmo1 );
			if (m_iPrimaryAmmoType == -1)
			{
				Msg("ERROR: Weapon (%s) using undefined primary ammo type (%s)\n",GetClassname(), GetWpnData().szAmmo1);
			}
		}
		if ( GetWpnData().szAmmo2[0] )
		{
			m_iSecondaryAmmoType = GetAmmoDef()->Index( GetWpnData().szAmmo2 );
			if (m_iSecondaryAmmoType == -1)
			{
				Msg("ERROR: Weapon (%s) using undefined secondary ammo type (%s)\n",GetClassname(),GetWpnData().szAmmo2);
			}

		}
#if defined( CLIENT_DLL )
		gWR.LoadWeaponSprites( this );
#endif
		// Precache models
		m_iViewModelIndex = PrecacheModel( GetViewModel() );
		m_iWorldModelIndex = PrecacheModel( GetWorldModel() );
	}
	else
	{
		// Couldn't read data file, remove myself
		Msg( "Error reading weapon data file for: %s\n", GetClassname() );
		Remove( );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Get my data in the file weapon info array
//-----------------------------------------------------------------------------
const FileWeaponInfo_t &CBaseCombatWeapon::GetWpnData( void ) const
{
	return *GetFileWeaponInfoFromHandle( m_hWeaponFileInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetViewModel( int /*viewmodelindex = 0 -- this is ignored in the base class here*/ ) const
{
	return GetWpnData().szViewModel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetWorldModel( void ) const
{
	return GetWpnData().szWorldModel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetAnimPrefix( void ) const
{
	return GetWpnData().szAnimationPrefix;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetPrintName( void ) const
{
	return GetWpnData().szPrintName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetMaxClip1( void ) const
{
	return GetWpnData().iMaxClip1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetMaxClip2( void ) const
{
	return GetWpnData().iMaxClip2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetDefaultClip1( void ) const
{
	return GetWpnData().iDefaultClip1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetDefaultClip2( void ) const
{
	return GetWpnData().iDefaultClip2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::UsesClipsForAmmo1( void ) const
{
	return ( GetMaxClip1() != WEAPON_NOCLIP );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::UsesClipsForAmmo2( void ) const
{
	return ( GetMaxClip2() != WEAPON_NOCLIP );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetWeight( void ) const
{
	return GetWpnData().iWeight;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetFlags( void ) const
{
	return GetWpnData().iFlags;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetSlot( void ) const
{
	return GetWpnData().iSlot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetPosition( void ) const
{
	return GetWpnData().iPosition;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetName( void ) const
{
	return GetWpnData().szClassName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteActive( void ) const
{
	return GetWpnData().iconActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteInactive( void ) const
{
	return GetWpnData().iconInactive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteAmmo( void ) const
{
	return GetWpnData().iconAmmo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteAmmo2( void ) const
{
	return GetWpnData().iconAmmo2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteCrosshair( void ) const
{
	return GetWpnData().iconCrosshair;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteAutoaim( void ) const
{
	return GetWpnData().iconAutoaim;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteZoomedCrosshair( void ) const
{
	return GetWpnData().iconZoomedCrosshair;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteZoomedAutoaim( void ) const
{
	return GetWpnData().iconZoomedAutoaim;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetShootSound( int iIndex ) const
{
	return GetWpnData().aShootSounds[ iIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseCombatCharacter	*CBaseCombatWeapon::GetOwner() const
{
	return ToBaseCombatCharacter( m_hOwner.Get() );
}	

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : BaseCombatCharacter - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SetOwner( CBaseCombatCharacter *owner )
{
	m_hOwner = owner;
}

//-----------------------------------------------------------------------------
// Purpose: Return false if this weapon won't let the player switch away from it
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::IsAllowedToSwitch( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon can be selected via the weapon selection
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::CanBeSelected( void )
{
	if ( !VisibleInWeaponSelection() )
		return false;

	return HasAmmo();
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon has some ammo
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::HasAmmo( void )
{
	// Weapons with no ammo types can always be selected
	if ( m_iPrimaryAmmoType == -1 && m_iSecondaryAmmoType == -1  )
		return true;
	if ( GetFlags() & ITEM_FLAG_SELECTONEMPTY )
		return true;

	CBasePlayer *player = ToBasePlayer( GetOwner() );
	if ( !player )
		return false;
	return ( m_iClip1 > 0 || player->GetAmmoCount( m_iPrimaryAmmoType ) || m_iClip2 > 0 || player->GetAmmoCount( m_iSecondaryAmmoType ) );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon should be seen, and hence be selectable, in the weapon selection
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::VisibleInWeaponSelection( void )
{
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::HasWeaponIdleTimeElapsed( void )
{
	if ( gpGlobals->curtime > m_flTimeWeaponIdle )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SetWeaponIdleTime( float time )
{
	m_flTimeWeaponIdle = time;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CBaseCombatWeapon::GetWeaponIdleTime( void )
{
	return m_flTimeWeaponIdle;
}


//-----------------------------------------------------------------------------
// Purpose: Drop/throw the weapon with the given velocity.
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::Drop( const Vector &vecVelocity )
{
#if !defined( CLIENT_DLL )

	// Once somebody drops a gun, it's fair game for removal when/if
	// a game_weapon_manager does a cleanup on surplus weapons in the
	// world.
	SetRemoveable( true );

	StopAnimation();
	StopFollowingEntity( );
	SetMoveType( MOVETYPE_FLYGRAVITY );
	// clear follow stuff, setup for collision
	SetGravity(1.0);
	AddSolidFlags( FSOLID_TRIGGER );
	m_iState = WEAPON_NOT_CARRIED;
	m_fEffects &= ~EF_NODRAW;
	UTIL_Relink( this );
	FallInit();
	RemoveFlag( FL_ONGROUND );
	SetThink( &CBaseCombatWeapon::SetPickupTouch );
	SetTouch(NULL);

	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj != NULL )
	{
		AngularImpulse	angImp( 200, 200, 200 );
		pObj->AddVelocity( &vecVelocity, &angImp );
	}
	else
	{
		SetAbsVelocity( vecVelocity );
	}

	SetNextThink( gpGlobals->curtime + 1.0f );
	SetOwnerEntity( NULL );
	m_hOwner = NULL;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPicker - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::OnPickedUp( CBaseCombatCharacter *pNewOwner )
{
#if !defined( CLIENT_DLL )
	if( pNewOwner->IsPlayer() )
	{
		m_OnPlayerPickup.FireOutput(pNewOwner, this);
	}
	else
	{
		m_OnNPCPickup.FireOutput(pNewOwner, this);
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Default Touch function for player picking up a weapon (not AI)
// Input  : pOther - the entity that touched me
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::DefaultTouch( CBaseEntity *pOther )
{
#if !defined( CLIENT_DLL )
	// if it's not a player, ignore
	CBasePlayer *pPlayer = ToBasePlayer(pOther);
	if ( !pPlayer )
		return;

	if (pPlayer->BumpWeapon(this))
	{
		OnPickedUp( pPlayer );
	}
#endif
}

void CBaseCombatWeapon::SetPickupTouch( void )
{
#if !defined( CLIENT_DLL )
	SetTouch(&CBaseCombatWeapon::DefaultTouch);
#endif
}

//------------------------------------------------------------------------------
// Purpose : Override so we don't step up on
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseCombatWeapon::SetObjectCollisionBox( void )
{
#if !defined( CLIENT_DLL )
	BaseClass::SetObjectCollisionBox();

	if ( !IsFollowingEntity() )
	{
		// Blow out the size a little so it's easier to pick up weapons
		Vector absmins, absmaxs;

		absmins = GetAbsMins();
		absmaxs = GetAbsMaxs();

		for ( int i = 0; i < 2; i++ )
		{
			if ( (GetAbsOrigin()[i] - absmins[i]) < 24 )
			{
				absmins[i] = GetAbsOrigin()[i] - 24;
			}
			if ( (absmaxs[i] - GetAbsOrigin()[i]) < 24 )
			{
				absmaxs[i] = GetAbsOrigin()[i] + 24;
			}
		}

		SetAbsMins( absmins );
		SetAbsMaxs( absmaxs );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Become a child of the owner (MOVETYPE_FOLLOW)
//			disables collisions, touch functions, thinking
// Input  : *pOwner - new owner/operator
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::Equip( CBaseCombatCharacter *pOwner )
{
	// Attach the weapon to an owner
	RemoveSolidFlags( FSOLID_TRIGGER );
	FollowEntity( pOwner );
	m_hOwner = pOwner;
	SetOwnerEntity( pOwner );

	m_flNextPrimaryAttack		= gpGlobals->curtime;
	m_flNextSecondaryAttack		= gpGlobals->curtime;
	SetTouch( NULL );
	SetThink( NULL );
#if !defined( CLIENT_DLL )
	UTIL_Relink( this );
	VPhysicsDestroyObject();
#endif

	if ( pOwner->IsPlayer() )
	{
		SetModel( GetViewModel() );
	}
	else
	{
		SetModel( GetWorldModel() );
	}
}

void CBaseCombatWeapon::SetActivity( Activity act, float duration ) 
{ 
	int sequence = SelectWeightedSequence( act ); 
	
	// FORCE IDLE on sequences we don't have (which should be many)
	if ( sequence == ACTIVITY_NOT_AVAILABLE )
		sequence = SelectWeightedSequence( ACT_VM_IDLE );

	if ( sequence != ACTIVITY_NOT_AVAILABLE )
	{
		SetSequence( sequence );
		SetActivity( act ); 
		m_flCycle = 0;
		ResetSequenceInfo( );

		if ( duration > 0 )
		{
			// FIXME: does this even make sense in non-shoot animations?
			m_flPlaybackRate = SequenceDuration( sequence ) / duration;
			m_flPlaybackRate = min( m_flPlaybackRate, 12.0);  // FIXME; magic number!, network encoding range
		}
		else
		{
			m_flPlaybackRate = 1.0;
		}
	}
}

//====================================================================================
// WEAPON CLIENT HANDLING
//====================================================================================
int CBaseCombatWeapon::UpdateClientData( CBasePlayer *pPlayer )
{
	if ( pPlayer->GetActiveWeapon() == this )
	{
		if ( pPlayer->m_fOnTarget ) 
		{
			m_iState = WEAPON_IS_ONTARGET;
		}
		else
		{
			m_iState = WEAPON_IS_ACTIVE;
		}
	}
	else
	{
		m_iState = WEAPON_IS_CARRIED_BY_PLAYER;
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SetViewModelIndex( int index )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );
	m_nViewModelIndex = index;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iActivity - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SendViewModelAnim( int nSequence )
{
#if defined( CLIENT_DLL )
	if ( !IsPredicted() )
		return;
#endif
	
	if ( nSequence < 0 )
		return;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;
	
	CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
	
	if ( vm == NULL )
		return;

	Assert( vm->ViewModelIndex() == m_nViewModelIndex );

	vm->SetWeaponModel( GetViewModel( m_nViewModelIndex ), this );
	vm->SendViewModelMatchingSequence( nSequence );
}

//-----------------------------------------------------------------------------
// Purpose: Set the desired activity for the weapon and its viewmodel counterpart
// Input  : iActivity - activity to play
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SendWeaponAnim( int iActivity )
{
	//For now, just set the ideal activity and be done with it
	SetIdealActivity( (Activity) iActivity );
}

//====================================================================================
// WEAPON SELECTION
//====================================================================================

//-----------------------------------------------------------------------------
// Purpose: Returns true if the weapon currently has ammo or doesn't need ammo
// Output :
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::HasAnyAmmo( void )
{
	// If I don't use ammo of any kind, I can always fire
	if ( !UsesPrimaryAmmo() && !UsesSecondaryAmmo() )
		return true;

	// Otherwise, I need ammo of either type
	return ( HasPrimaryAmmo() || HasSecondaryAmmo() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the weapon currently has ammo or doesn't need ammo
// Output :
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::HasPrimaryAmmo( void )
{
	// If I use a clip, and have some ammo in it, then I have ammo
	if ( UsesClipsForAmmo1() )
	{
		if ( m_iClip1 > 0 )
			return true;
	}

	// Otherwise, I have ammo if I have some in my ammo counts
	CBaseCombatCharacter		*pOwner = GetOwner();
	if ( pOwner )
	{
		if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 ) 
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the weapon currently has ammo or doesn't need ammo
// Output :
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::HasSecondaryAmmo( void )
{
	// If I use a clip, and have some ammo in it, then I have ammo
	if ( UsesClipsForAmmo2() )
	{
		if ( m_iClip2 > 0 )
			return true;
	}

	// Otherwise, I have ammo if I have some in my ammo counts
	CBaseCombatCharacter		*pOwner = GetOwner();
	if ( pOwner )
	{
		if ( pOwner->GetAmmoCount( m_iSecondaryAmmoType ) > 0 ) 
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the weapon actually uses primary ammo
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::UsesPrimaryAmmo( void )
{
	if ( m_iPrimaryAmmoType < 0 )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the weapon actually uses secondary ammo
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::UsesSecondaryAmmo( void )
{
	if ( m_iSecondaryAmmoType < 0 )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SetWeaponVisible( bool visible )
{
	CBaseViewModel *vm = NULL;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		vm = pOwner->GetViewModel( m_nViewModelIndex );
	}

	if ( visible )
	{
		m_fEffects &= ~EF_NODRAW;
		if ( vm )
		{
			vm->m_fEffects &= ~EF_NODRAW;
		}
	}
	else
	{
		m_fEffects |= EF_NODRAW;
		if ( vm )
		{
			vm->m_fEffects |= EF_NODRAW;
		}
	}
}

bool CBaseCombatWeapon::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt )
{
	if ( !HasAnyAmmo() )
		return false;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
	{
		return false;
	}

	pOwner->SetAnimationExtension( szAnimExt );

	SendWeaponAnim( iActivity );

	pOwner->SetNextAttack( gpGlobals->curtime + SequenceDuration() );
	m_flNextPrimaryAttack	= gpGlobals->curtime;
	m_flNextSecondaryAttack	= gpGlobals->curtime;

	SetWeaponVisible( true );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::Deploy( )
{
	return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_VM_DRAW, (char*)GetAnimPrefix() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{ 
	// cancel any reload in progress.
	m_bInReload = false; 

	// kill any think functions
	SetThink(NULL);

	// No longer using holstering...  New weapon just pulled out
	//SendWeaponAnim( ACT_VM_HOLSTER );

	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner)
	{
		pOwner->SetNextAttack( gpGlobals->curtime + SequenceDuration() );
	}

	// Hide it now
	SetWeaponVisible( false );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::ItemPreFrame( void )
{
	MaintainIdealActivity();
}

//====================================================================================
// WEAPON BEHAVIOUR
//====================================================================================
void CBaseCombatWeapon::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		return;
	}

	//Track the duration of the fire
	//FIXME: Check for IN_ATTACK2 as well?
	m_fFireDuration = ( pOwner->m_nButtons & IN_ATTACK ) ? ( m_fFireDuration + gpGlobals->frametime ) : 0.0f;

	if ( UsesClipsForAmmo1() )
	{
		CheckReload();
	}

	if ((pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		if (UsesSecondaryAmmo() && pOwner->GetAmmoCount(m_iSecondaryAmmoType)<=0 )
		{
			if (m_flNextEmptySoundTime < gpGlobals->curtime)
			{
				WeaponSound(EMPTY);
				m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
			}
		}
		else
		{
			SecondaryAttack();

			// Secondary ammo doesn't have a reload animation
			if ( UsesClipsForAmmo2() )
			{
				// reload clip2 if empty
				if (m_iClip2 < 1)
				{
					pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );
					m_iClip2 = m_iClip2 + 1;
				}
			}
		}
	}
	else if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		// Clip empty? Or out of ammo on a no-clip weapon?
		if ( ( UsesClipsForAmmo1() && m_iClip1 <= 0) || ( !UsesClipsForAmmo1() && pOwner->GetAmmoCount(m_iPrimaryAmmoType)<=0 ) )
		{
			if (m_flNextEmptySoundTime < gpGlobals->curtime)
			{
				WeaponSound(EMPTY);
				m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
			}
			m_bFireOnEmpty = true;
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			if ( pOwner && pOwner->m_afButtonPressed & IN_ATTACK )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}

			PrimaryAttack();
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	// -----------------------
	if ( pOwner->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	if (!((pOwner->m_nButtons & IN_ATTACK) || (pOwner->m_nButtons & IN_ATTACK2) || (pOwner->m_nButtons & IN_RELOAD)))
	{
		// no fire buttons down
		m_bFireOnEmpty = false;

		if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime ) 
		{
			// weapon isn't useable, switch.
			if ( ( (GetFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) == false ) && ( g_pGameRules->SwitchToNextBestWeapon( pOwner, this ) ) )
			{
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
				return;
			}
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if ( UsesClipsForAmmo1() && (m_iClip1 == 0) && !(GetFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime )
			{
				if (Reload())
				{
					// if we're successfully reloading, we're done
					return;
				}
			}
		}

		WeaponIdle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called each frame by the player PostThink, if the player's not ready to attack yet
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::ItemBusyFrame( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for getting bullet type
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetBulletType( void )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for getting spread
// Input  :
// Output :
//-----------------------------------------------------------------------------
const Vector& CBaseCombatWeapon::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_15DEGREES;
	return cone;
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for getting firerate
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CBaseCombatWeapon::GetFireRate( void )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for playing shoot sound
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::WeaponSound( WeaponSound_t sound_type, float soundtime /* = 0.0f */ )
{
	// If we have some sounds from the weapon classname.txt file, play a random one of them
	const char *shootsound = GetShootSound( sound_type );
	if ( !shootsound || !shootsound[0] )
		return;

	CSoundParameters params;
	
	if ( !GetParametersForSound( shootsound, params ) )
		return;

	if ( params.play_to_owner_only )
	{
		// Am I only to play to my owner?
		if ( GetOwner() && GetOwner()->IsPlayer() )
		{
			CSingleUserRecipientFilter filter( ToBasePlayer( GetOwner() ) );
			if ( IsPredicted() )
			{
				filter.UsePredictionRules();
			}
			EmitSound( filter, GetOwner()->entindex(), shootsound, NULL, soundtime );
		}
	}
	else
	{
		// Play weapon sound from the owner
		if ( GetOwner() )
		{
			CPASAttenuationFilter filter( GetOwner(), params.soundlevel );
			if ( IsPredicted() )
			{
				filter.UsePredictionRules();
			}
			EmitSound( filter, GetOwner()->entindex(), shootsound, NULL, soundtime ); 
		}
		// If no owner play from the weapon (this is used for thrown items)
		else
		{
			CPASAttenuationFilter filter( this, params.soundlevel );
			if ( IsPredicted() )
			{
				filter.UsePredictionRules();
			}
			EmitSound( filter, entindex(), shootsound, NULL, soundtime ); 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop a sound played by this weapon.
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::StopWeaponSound( WeaponSound_t sound_type )
{
	//if ( IsPredicted() )
	//	return;

	// If we have some sounds from the weapon classname.txt file, play a random one of them
	const char *shootsound = GetShootSound( sound_type );
	if ( !shootsound || !shootsound[0] )
		return;
	
	CSoundParameters params;
	if ( !GetParametersForSound( shootsound, params ) )
		return;

	// Am I only to play to my owner?
	if ( params.play_to_owner_only )
	{
		if ( GetOwner() )
		{
			StopSound( GetOwner()->entindex(), shootsound );
		}
	}
	else
	{
		// Play weapon sound from the owner
		if ( GetOwner() )
		{
			StopSound( GetOwner()->entindex(), shootsound );
		}
		// If no owner play from the weapon (this is used for thrown items)
		else
		{
			StopSound( entindex(), shootsound );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return false;

	// If I don't have any spare ammo, I can't reload
	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return false;

	bool bReload = false;

	// If you don't have clips, then don't try to reload them.
	if ( UsesClipsForAmmo1() )
	{
		// need to reload primary clip?
		int primary	= min(iClipSize1 - m_iClip1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));
		if ( primary != 0 )
		{
			bReload = true;
		}
	}

	if ( UsesClipsForAmmo2() )
	{
		// need to reload secondary clip?
		int secondary = min(iClipSize2 - m_iClip2, pOwner->GetAmmoCount(m_iSecondaryAmmoType));
		if ( secondary != 0 )
		{
			bReload = true;
		}
	}

	if ( !bReload )
		return false;

#ifdef CLIENT_DLL
	// Play reload
	WeaponSound( RELOAD );
#endif
	SendWeaponAnim( iActivity );

	// Play the player's reload animation
	if ( pOwner->IsPlayer() )
	{
		( ( CBasePlayer * )pOwner)->SetAnimation( PLAYER_RELOAD );
	}

	float flSequenceEndTime = gpGlobals->curtime + SequenceDuration();
	pOwner->SetNextAttack( flSequenceEndTime );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = flSequenceEndTime;

	m_bInReload = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::Reload( void )
{
	return DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
}

//=========================================================
void CBaseCombatWeapon::WeaponIdle( void )
{
	//Idle again if we've finished
	if ( HasWeaponIdleTimeElapsed() )
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}
}


//=========================================================
int CBaseCombatWeapon::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

//=========================================================
int CBaseCombatWeapon::GetSecondaryAttackActivity( void )
{
	return ACT_VM_SECONDARYATTACK;
}

//-----------------------------------------------------------------------------
// Purpose: Adds in view kick and weapon accuracy degradation effect
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::AddViewKick( void )
{
	//NOTENOTE: By default, weapon will not kick up (defined per weapon)
}

//-----------------------------------------------------------------------------
// Purpose: Get the string to print death notices with
//-----------------------------------------------------------------------------
char *CBaseCombatWeapon::GetDeathNoticeName( void )
{
#if !defined( CLIENT_DLL )
	return (char*)STRING( m_iszName );
#else
	return "GetDeathNoticeName not implemented on client yet";
#endif
}

//====================================================================================
// WEAPON RELOAD TYPES
//====================================================================================
void CBaseCombatWeapon::CheckReload( void )
{
	if ( m_bReloadsSingly )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( !pOwner )
			return;

		if ((m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			if ( pOwner->m_nButtons & (IN_ATTACK | IN_ATTACK2) && m_iClip1 > 0 )
			{
				m_bInReload = false;
				return;
			}

			// If out of ammo end reload
			if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <=0)
			{
				FinishReload();
				return;
			}
			// If clip not full reload again
			else if (m_iClip1 < GetMaxClip1())
			{
				// Add them to the clip
				m_iClip1 += 1;
				pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

				Reload();
				return;
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				m_flNextPrimaryAttack	= gpGlobals->curtime;
				m_flNextSecondaryAttack = gpGlobals->curtime;
				return;
			}
		}
	}
	else
	{
		if ( (m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			FinishReload();
			m_flNextPrimaryAttack	= gpGlobals->curtime;
			m_flNextSecondaryAttack = gpGlobals->curtime;
			m_bInReload = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reload has finished.
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::FinishReload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner)
	{
		// If I use primary clips, reload primary
		if ( UsesClipsForAmmo1() )
		{
			int primary	= min( GetMaxClip1() - m_iClip1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));	
			m_iClip1 += primary;
			pOwner->RemoveAmmo( primary, m_iPrimaryAmmoType);
		}

		// If I use secondary clips, reload secondary
		if ( UsesClipsForAmmo2() )
		{
			int secondary = min( GetMaxClip2() - m_iClip2, pOwner->GetAmmoCount(m_iSecondaryAmmoType));
			m_iClip2 += secondary;
			pOwner->RemoveAmmo( secondary, m_iSecondaryAmmoType );
		}

		if ( m_bReloadsSingly )
		{
			m_bInReload = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Abort any reload we have in progress
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::AbortReload( void )
{
#ifdef CLIENT_DLL
	StopWeaponSound( RELOAD ); 
#endif
	m_bInReload = false;
}

//-----------------------------------------------------------------------------
// Purpose: Primary fire button attack
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::PrimaryAttack( void )
{
	// If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && !m_iClip1 ) 
	{
		Reload();
		return;
	}

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->m_fEffects |= EF_MUZZLEFLASH;

	SendWeaponAnim( GetPrimaryAttackActivity() );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = pPlayer->Weapon_ShootPosition( );
	Vector vecAiming = pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );	

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{

		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		iBulletsToFire++;
		if ( !fireRate )
			break;
	}

	// Make sure we don't fire more than the amount in the clip
	if ( iBulletsToFire > m_iClip1 )
		iBulletsToFire = m_iClip1;
	m_iClip1 -= iBulletsToFire;

#if !defined( CLIENT_DLL )
	// Fire the bullets
	pPlayer->FireBullets( iBulletsToFire, vecSrc, vecAiming, pPlayer->GetAttackAccuracy( this ), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
#else
	//!!!HACKHACK - what does the client want this function for? 
	pPlayer->FireBullets( iBulletsToFire, vecSrc, vecAiming, GetActiveWeapon()->GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
#endif // CLIENT_DLL

#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 400, 0.2, pPlayer );
#endif

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	//Add our view kick in
	AddViewKick();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame to check if the weapon is going through transition animations
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::MaintainIdealActivity( void )
{
	//See if we're not at our target animation yet
	if ( ( GetActivity() != m_IdealActivity ) || ( GetSequence() != m_nIdealSequence ) )
	{
		//If we're done with this animation, move to the next
		if ( IsSequenceFinished() )
		{
			//Find the next sequence in the potential chain of sequences leading to our ideal one
			int newSequence = FindTransitionSequence( GetSequence(), m_nIdealSequence, NULL );

			//Set the new sequence on the weapon
			SetSequence( newSequence );
			SendViewModelAnim( newSequence );
			
			//Set the next time the weapon will idle
			SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the ideal activity for the weapon to be in, allowing for transitional animations inbetween
// Input  : ideal - activity to end up at, ideally
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SetIdealActivity( Activity ideal )
{
	int	idealSequence = SelectWeightedSequence( ideal );

	if ( idealSequence == -1 )
		return;

	//Take the new activity
	m_IdealActivity	 = ideal;
	m_nIdealSequence = idealSequence;

	//Find the next sequence in the potential chain of sequences leading to our ideal one
	int nextSequence = FindTransitionSequence( GetSequence(), m_nIdealSequence, NULL );

	//Set our activity to the ideal
	SetActivity( ideal );
	SetSequence( nextSequence );	
	SendViewModelAnim( nextSequence );

	//Set the next time the weapon will idle
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
}

//-----------------------------------------------------------------------------
// Returns information about the various control panels
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = NULL;
}

//-----------------------------------------------------------------------------
// Returns information about the various control panels
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::GetControlPanelClassName( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "vgui_screen";
}



//=========================================================
//=========================================================
class CGameWeaponManager : public CBaseEntity
{
	DECLARE_CLASS( CGameWeaponManager, CBaseEntity );

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

public:
	void Spawn();

#if !defined( CLIENT_DLL )
	void Think();
	void InputSetMaxPieces( inputdata_t &inputdata );
#endif

	string_t	m_iszWeaponName;
	int			m_iMaxPieces;
};

#if !defined( CLIENT_DLL )

BEGIN_DATADESC( CGameWeaponManager )

//fields	
	DEFINE_KEYFIELD( CGameWeaponManager, m_iszWeaponName, FIELD_STRING, "weaponname" ),
	DEFINE_KEYFIELD( CGameWeaponManager, m_iMaxPieces, FIELD_INTEGER, "maxpieces" ),
// funcs
	DEFINE_FUNCTION( CGameWeaponManager, Think ),
// inputs
	DEFINE_INPUTFUNC( CGameWeaponManager, FIELD_INTEGER, "SetMaxPieces", InputSetMaxPieces ),

END_DATADESC()

#endif

LINK_ENTITY_TO_CLASS( game_weapon_manager, CGameWeaponManager );

//---------------------------------------------------------
//---------------------------------------------------------
void CGameWeaponManager::Spawn()
{
#if !defined( CLIENT_DLL )
	SetThink( &CGameWeaponManager::Think );
	SetNextThink( gpGlobals->curtime );
#endif
}

//---------------------------------------------------------
// Count of all the weapons in the world of my type and
// see if we have a surplus. If there is a surplus, try
// to find suitable candidates for removal.
//
// Right now we just remove the first weapons we find that
// are behind the player, or are out of the player's PVS. 
// Later, we may want to score the results so that we
// removed the farthest gun that's not in the player's 
// viewcone, etc.
//
// Some notes and thoughts:
//
// This code is designed NOT to remove weapons that are 
// hand-placed by level designers. It should only clean
// up weapons dropped by dead NPCs, which is useful in
// situations where enemies are spawned in for a sustained
// period of time.
//
// Right now we PREFER to remove weapons that are not in the
// player's PVS, but this could be opposite of what we 
// really want. We may only want to conduct the cleanup on
// weapons that are IN the player's PVS.
//---------------------------------------------------------
#if !defined( CLIENT_DLL )

void CGameWeaponManager::Think()
{

	// Don't have to think all that often. 
	SetNextThink( gpGlobals->curtime + 2.0 );

	CBaseCombatWeapon *pWeapon = NULL;
	const char *pszWeaponName = STRING( m_iszWeaponName );
	int count = 0;
	int removeableCount = 0;

	// Firstly, count the total number of weapons of this type in the world.
	// Also count how many of those can potentially be removed.
	pWeapon = (CBaseCombatWeapon *)gEntList.FindEntityByClassname( pWeapon, pszWeaponName );
	while( pWeapon )
	{
		count++;

		if( pWeapon->IsRemoveable() )
		{
			removeableCount++;
		}

		pWeapon = (CBaseCombatWeapon *)gEntList.FindEntityByClassname( pWeapon, pszWeaponName );
	}

	// Calculate the surplus.
	int surplus = removeableCount - m_iMaxPieces;

	// Based on what the player can see, try to clean up the world by removing weapons that
	// the player cannot see right at the moment.
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
	while( surplus > 0 )
	{
		bool fRemovedOne = false;

		pWeapon = (CBaseCombatWeapon *)gEntList.FindEntityByClassname( NULL, pszWeaponName );
		while( pWeapon )
		{
			if( !(pWeapon->m_fEffects & EF_NODRAW ) && pWeapon->IsRemoveable() )
			{
				// Nodraw serves as a flag that this weapon is already being removed since
				// all we're really doing inside this loop is marking them for removal by
				// the entity system. We don't want to count the same weapon as removed
				// more than once.
				if( !UTIL_FindClientInPVS( pWeapon->GetEdict() ) )
				{
					fRemovedOne = true;
				}
				else if( !pPlayer->FInViewCone( pWeapon ) )
				{
					fRemovedOne = true;
				}

				if( fRemovedOne )
				{
					pWeapon->m_fEffects |= EF_NODRAW;
					UTIL_Remove( pWeapon );

					Msg("Surplus %s removed\n", pszWeaponName);
					surplus--;

					if( surplus == 0 )
						break;
				}
			}

			pWeapon = (CBaseCombatWeapon *)gEntList.FindEntityByClassname( pWeapon, pszWeaponName );
		}

		if( !fRemovedOne )
		{
			// No suitable candidates for removal right now.
			break;
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CGameWeaponManager::InputSetMaxPieces( inputdata_t &inputdata )
{
	m_iMaxPieces = inputdata.value.Int();
}

#endif	// ! CLIENT_DLL

BEGIN_PREDICTION_DATA( CBaseCombatWeapon )

	DEFINE_PRED_FIELD( CBaseCombatWeapon, m_nNextThinkTick, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	// Networked
	DEFINE_PRED_FIELD( CBaseCombatWeapon, m_hOwner, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	// DEFINE_FIELD( CBaseCombatWeapon, m_hWeaponFileInfo, FIELD_SHORT ),
	DEFINE_PRED_FIELD( CBaseCombatWeapon, m_iState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			 
	DEFINE_PRED_FIELD( CBaseCombatWeapon, m_iViewModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_PRED_FIELD( CBaseCombatWeapon, m_iWorldModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_PRED_FIELD_TOL( CBaseCombatWeapon, m_flNextPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD_TOL( CBaseCombatWeapon, m_flNextSecondaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD_TOL( CBaseCombatWeapon, m_flTimeWeaponIdle, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),

	DEFINE_PRED_FIELD( CBaseCombatWeapon, m_iPrimaryAmmoType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CBaseCombatWeapon, m_iSecondaryAmmoType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CBaseCombatWeapon, m_iClip1, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			
	DEFINE_PRED_FIELD( CBaseCombatWeapon, m_iClip2, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			

	DEFINE_PRED_FIELD( CBaseCombatWeapon, m_nViewModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	// Not networked

	DEFINE_FIELD( CBaseCombatWeapon, m_bInReload, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseCombatWeapon, m_bFireOnEmpty, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseCombatWeapon, m_flNextEmptySoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseCombatWeapon, m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatWeapon, m_fFireDuration, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseCombatWeapon, m_iszName, FIELD_INTEGER ),		
	DEFINE_FIELD( CBaseCombatWeapon, m_bFiresUnderwater, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseCombatWeapon, m_fMinRange1, FIELD_FLOAT ),		
	DEFINE_FIELD( CBaseCombatWeapon, m_fMinRange2, FIELD_FLOAT ),		
	DEFINE_FIELD( CBaseCombatWeapon, m_fMaxRange1, FIELD_FLOAT ),		
	DEFINE_FIELD( CBaseCombatWeapon, m_fMaxRange2, FIELD_FLOAT ),		
	DEFINE_FIELD( CBaseCombatWeapon, m_bReloadsSingly, FIELD_BOOLEAN ),	
	DEFINE_FIELD( CBaseCombatWeapon, m_bRemoveable, FIELD_BOOLEAN ),

#if defined( CLIENT_DLL )
	DEFINE_FIELD( CBaseCombatWeapon, m_iOldState, FIELD_INTEGER ),
	// DEFINE_FIELD( CBaseCombatWeapon, m_bJustRestored, FIELD_BOOLEAN ),
#endif
	// DEFINE_FIELD( CBaseCombatWeapon, m_OnPlayerPickup, COutputEvent ),
	// DEFINE_FIELD( CBaseCombatWeapon, m_pConstraint, FIELD_INTEGER ),

END_PREDICTION_DATA()

// Special hack since we're aliasing the name C_BaseCombatWeapon with a macro on the client
IMPLEMENT_NETWORKCLASS_ALIASED( BaseCombatWeapon, DT_BaseCombatWeapon )

#if !defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: Save Data for Base Weapon object
//-----------------------------------------------------------------------------// 
BEGIN_DATADESC( CBaseCombatWeapon )

	DEFINE_FIELD( CBaseCombatWeapon, m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBaseCombatWeapon, m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBaseCombatWeapon, m_flTimeWeaponIdle, FIELD_TIME ),

	DEFINE_FIELD( CBaseCombatWeapon, m_bInReload, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseCombatWeapon, m_bFireOnEmpty, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseCombatWeapon, m_hOwner, FIELD_EHANDLE ),

	DEFINE_FIELD( CBaseCombatWeapon, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatWeapon, m_iszName, FIELD_STRING ),
	DEFINE_FIELD( CBaseCombatWeapon, m_iPrimaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatWeapon, m_iSecondaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatWeapon, m_iClip1, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatWeapon, m_iClip2, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatWeapon, m_bFiresUnderwater, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseCombatWeapon, m_fMinRange1, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseCombatWeapon, m_fMinRange2, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseCombatWeapon, m_fMaxRange1, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseCombatWeapon, m_fMaxRange2, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseCombatWeapon, m_nViewModelIndex, FIELD_INTEGER ),

// don't save these, init to 0 and regenerate
//	DEFINE_FIELD( CBaseCombatWeapon, m_flNextEmptySoundTime, FIELD_TIME ),
//	DEFINE_FIELD( CBaseCombatWeapon, m_Activity, FIELD_INTEGER ),
 	DEFINE_FIELD( CBaseCombatWeapon, m_nIdealSequence, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatWeapon, m_IdealActivity, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseCombatWeapon, m_fFireDuration, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseCombatWeapon, m_bReloadsSingly, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseCombatWeapon, m_iSubType, FIELD_INTEGER ),
 	DEFINE_FIELD( CBaseCombatWeapon, m_bRemoveable, FIELD_BOOLEAN ),

	//	DEFINE_FIELD( CBaseCombatWeapon, m_iViewModelIndex, FIELD_INTEGER ),
	//	DEFINE_FIELD( CBaseCombatWeapon, m_iWorldModelIndex, FIELD_INTEGER ),
	//  DEFINE_FIELD( CBaseCombatWeapon, m_hWeaponFileInfo, ???? ),
	//	DEFINE_FIELD( CBaseCombatWeapon, m_pConstraint, FIELD_INTEGER ),

	// Just to quiet classcheck.. this field exists only on the client
//	DEFINE_FIELD( CBaseCombatWeapon, m_iOldState, FIELD_INTEGER ),

	// Function pointers
	DEFINE_FUNCTION( CBaseCombatWeapon, DefaultTouch ),
	DEFINE_FUNCTION( CBaseCombatWeapon, FallThink ),
	DEFINE_FUNCTION( CBaseCombatWeapon, Materialize ),
	DEFINE_FUNCTION( CBaseCombatWeapon, AttemptToMaterialize ),
	DEFINE_FUNCTION( CBaseCombatWeapon, DestroyItem ),
	DEFINE_FUNCTION( CBaseCombatWeapon, SetPickupTouch ),

	// Outputs
	DEFINE_OUTPUT( CBaseCombatWeapon, m_OnPlayerPickup, "OnPlayerPickup"),
	DEFINE_OUTPUT( CBaseCombatWeapon, m_OnNPCPickup, "OnNPCPickup"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Only send to local player if this weapon is the active weapon
// Input  : *pStruct - 
//			*pVarData - 
//			*pRecipients - 
//			objectID - 
// Output : void*
//-----------------------------------------------------------------------------
void* SendProxy_SendActiveLocalWeaponDataTable( const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pVarData;
	if ( pWeapon )
	{
		// Only send this chunk of data to the player carrying this weapon
		CBasePlayer *pPlayer = ToBasePlayer( pWeapon->GetOwner() );
		if ( pPlayer /*&& pPlayer->GetActiveWeapon() == pWeapon*/ )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
			return (void*)pVarData;
		}
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Only send the LocalWeaponData to the player carrying the weapon
//-----------------------------------------------------------------------------
void* SendProxy_SendLocalWeaponDataTable( const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pVarData;
	if ( pWeapon )
	{
		// Only send this chunk of data to the player carrying this weapon
		CBasePlayer *pPlayer = ToBasePlayer( pWeapon->GetOwner() );
		if ( pPlayer )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
			return (void*)pVarData;
		}
	}
	
	return NULL;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Propagation data for weapons. Only sent when a player's holding it.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CBaseCombatWeapon, DT_LocalActiveWeaponData )
#if !defined( CLIENT_DLL )
	SendPropTime( SENDINFO( m_flNextPrimaryAttack ) ),
	SendPropTime( SENDINFO( m_flNextSecondaryAttack ) ),
	SendPropInt( SENDINFO( m_nNextThinkTick ) ),
	SendPropTime( SENDINFO( m_flTimeWeaponIdle ) ),
#else
	RecvPropTime( RECVINFO( m_flNextPrimaryAttack ) ),
	RecvPropTime( RECVINFO( m_flNextSecondaryAttack ) ),
	RecvPropInt( RECVINFO( m_nNextThinkTick ) ),
	RecvPropTime( RECVINFO( m_flTimeWeaponIdle ) ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Propagation data for weapons. Only sent when a player's holding it.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CBaseCombatWeapon, DT_LocalWeaponData )
#if !defined( CLIENT_DLL )
	SendPropInt( SENDINFO(m_iClip1 ), 8 ),
	SendPropInt( SENDINFO(m_iClip2 ), 8 ),
	SendPropInt( SENDINFO(m_iPrimaryAmmoType ), 8 ),
	SendPropInt( SENDINFO(m_iSecondaryAmmoType ), 8 ),

	SendPropInt( SENDINFO( m_nViewModelIndex ), VIEWMODEL_INDEX_BITS, SPROP_UNSIGNED ),

#else
	RecvPropInt( RECVINFO(m_iClip1 )),
	RecvPropInt( RECVINFO(m_iClip2 )),
	RecvPropInt( RECVINFO(m_iPrimaryAmmoType )),
	RecvPropInt( RECVINFO(m_iSecondaryAmmoType )),

	RecvPropInt( RECVINFO( m_nViewModelIndex ) ),

#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE(CBaseCombatWeapon, DT_BaseCombatWeapon)
#if !defined( CLIENT_DLL )
	SendPropDataTable("LocalWeaponData", 0, &REFERENCE_SEND_TABLE(DT_LocalWeaponData), SendProxy_SendLocalWeaponDataTable ),
	SendPropDataTable("LocalActiveWeaponData", 0, &REFERENCE_SEND_TABLE(DT_LocalActiveWeaponData), SendProxy_SendActiveLocalWeaponDataTable ),
	SendPropModelIndex( SENDINFO(m_iViewModelIndex) ),
	SendPropModelIndex( SENDINFO(m_iWorldModelIndex) ),
	SendPropInt( SENDINFO(m_iState ), 8, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO(m_hOwner) ),
#else
	RecvPropDataTable("LocalWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalWeaponData)),
	RecvPropDataTable("LocalActiveWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalActiveWeaponData)),
	RecvPropInt( RECVINFO(m_iViewModelIndex)),
	RecvPropInt( RECVINFO(m_iWorldModelIndex)),
	RecvPropInt( RECVINFO(m_iState )),
	RecvPropEHandle( RECVINFO(m_hOwner ) ),
#endif
END_NETWORK_TABLE()
