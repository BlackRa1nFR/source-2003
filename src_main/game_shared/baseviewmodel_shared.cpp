//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "baseviewmodel_shared.h"

#if defined( CLIENT_DLL )
#include "iprediction.h"
#include "prediction.h"
#else
#include "vguiscreen.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define VIEWMODEL_ANIMATION_PARITY_BITS 3
#define SCREEN_OVERLAY_MATERIAL "vgui/screens/vgui_overlay"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseViewModel::CBaseViewModel()
{
#if defined( CLIENT_DLL )
	// NOTE: We do this here because the color is never transmitted for the view model.
	m_nOldAnimationParity = 0;
#endif
	SetRenderColor( 255, 255, 255, 255 );

	// View model of this weapon
	m_sVMName			= NULL_STRING;		
	// Prefix of the animations that should be used by the player carrying this weapon
	m_sAnimationPrefix	= NULL_STRING;

	m_nViewModelIndex	= 0;

	m_nAnimationParity	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseViewModel::~CBaseViewModel()
{
}

void CBaseViewModel::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	DestroyControlPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseViewModel::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseViewModel::Spawn( void )
{
	Precache( );
	SetSize( Vector( -8, -4, -2), Vector(8, 4, 2) );
	SetSolid( SOLID_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseViewModel::SetControlPanelsActive( bool bState )
{
#if defined( TF2_DLL )
	// Activate control panel screens
	for ( int i = m_hScreens.Count(); --i >= 0; )
	{
		if (m_hScreens[i].Get())
		{
			m_hScreens[i]->SetActive( bState );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// This is called by the base object when it's time to spawn the control panels
//-----------------------------------------------------------------------------
void CBaseViewModel::SpawnControlPanels()
{
#if defined( TF2_DLL )
	char buf[64];

	// Destroy existing panels
	DestroyControlPanels();

	CBaseCombatWeapon *weapon = m_hWeapon.Get();

	if ( weapon == NULL )
	{
		return;
	}

	// FIXME: Deal with dynamically resizing control panels?

	// If we're attached to an entity, spawn control panels on it instead of use
	CBaseAnimating *pEntityToSpawnOn = this;
	char *pOrgLL = "controlpanel%d_ll";
	char *pOrgUR = "controlpanel%d_ur";
	char *pAttachmentNameLL = pOrgLL;
	char *pAttachmentNameUR = pOrgUR;
	/*
	if ( IsBuiltOnAttachment() )
	{
		pEntityToSpawnOn = dynamic_cast<CBaseAnimating*>((CBaseEntity*)m_hBuiltOnEntity.Get());
		if ( pEntityToSpawnOn )
		{
			char sBuildPointLL[64];
			char sBuildPointUR[64];
			Q_snprintf( sBuildPointLL, 64, "bp%d_controlpanel%%d_ll", m_iBuiltOnPoint );
			Q_snprintf( sBuildPointUR, 64, "bp%d_controlpanel%%d_ur", m_iBuiltOnPoint );
			pAttachmentNameLL = sBuildPointLL;
			pAttachmentNameUR = sBuildPointUR;
		}
		else
		{
			pEntityToSpawnOn = this;
		}
	}
	*/

	Assert( pEntityToSpawnOn );

	// Lookup the attachment point...
	int nPanel;
	for ( nPanel = 0; true; ++nPanel )
	{
		Q_snprintf( buf, 64, pAttachmentNameLL, nPanel );
		int nLLAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
		if (nLLAttachmentIndex <= 0)
		{
			// Try and use my panels then
			pEntityToSpawnOn = this;
			Q_snprintf( buf, 64, pOrgLL, nPanel );
			nLLAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
			if (nLLAttachmentIndex <= 0)
				return;
		}

		Q_snprintf( buf, 64, pAttachmentNameUR, nPanel );
		int nURAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
		if (nURAttachmentIndex <= 0)
		{
			// Try and use my panels then
			Q_snprintf( buf, 64, pOrgUR, nPanel );
			nURAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
			if (nURAttachmentIndex <= 0)
				return;
		}

		const char *pScreenName;
		weapon->GetControlPanelInfo( nPanel, pScreenName );
		if (!pScreenName)
			continue;

		const char *pScreenClassname;
		weapon->GetControlPanelClassName( nPanel, pScreenClassname );
		if ( !pScreenClassname )
			continue;

		// Compute the screen size from the attachment points...
		matrix3x4_t	panelToWorld;
		pEntityToSpawnOn->GetAttachment( nLLAttachmentIndex, panelToWorld );

		matrix3x4_t	worldToPanel;
		MatrixInvert( panelToWorld, worldToPanel );

		// Now get the lower right position + transform into panel space
		Vector lr, lrlocal;
		pEntityToSpawnOn->GetAttachment( nURAttachmentIndex, panelToWorld );
		MatrixGetColumn( panelToWorld, 3, lr );
		VectorTransform( lr, worldToPanel, lrlocal );

		float flWidth = lrlocal.x;
		float flHeight = lrlocal.y;

		CVGuiScreen *pScreen = CreateVGuiScreen( pScreenClassname, pScreenName, pEntityToSpawnOn, this, nLLAttachmentIndex );
		pScreen->ChangeTeam( GetTeamNumber() );
		pScreen->SetActualSize( flWidth, flHeight );
		pScreen->SetActive( false );
		pScreen->MakeVisibleOnlyToTeammates( false );
		pScreen->SetOverlayMaterial( SCREEN_OVERLAY_MATERIAL );
		pScreen->SetAttachedToViewModel( true );
		int nScreen = m_hScreens.AddToTail( );
		m_hScreens[nScreen].Set( pScreen );
	}
#endif
}

void CBaseViewModel::DestroyControlPanels()
{
#if defined( TF2_DLL )
	// Kill the control panels
	int i;
	for ( i = m_hScreens.Count(); --i >= 0; )
	{
		DestroyVGuiScreen( m_hScreens[i].Get() );
	}
	m_hScreens.RemoveAll();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void CBaseViewModel::SetOwner( CBaseEntity *pEntity )
{
	m_hOwner = pEntity;
#if !defined( CLIENT_DLL )
	// Make sure we're linked into hierarchy
	//SetParent( pEntity );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//-----------------------------------------------------------------------------
void CBaseViewModel::SetIndex( int nIndex )
{
	m_nViewModelIndex = nIndex;
	Assert( m_nViewModelIndex < (1 << VIEWMODEL_INDEX_BITS) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseViewModel::ViewModelIndex( ) const
{
	return m_nViewModelIndex;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *modelname - 
//-----------------------------------------------------------------------------
void CBaseViewModel::SetWeaponModel( const char *modelname, CBaseCombatWeapon *weapon )
{
	m_hWeapon = weapon;

#if defined( CLIENT_DLL )
	SetModel( modelname );
#else
	string_t str;
	if ( modelname != NULL )
	{
		str = MAKE_STRING( modelname );
	}
	else
	{
		str = NULL_STRING;
	}

	if ( str != m_sVMName )
	{
		m_sVMName = str;
		SetModel( STRING( m_sVMName ) );

		// Create any vgui control panels associated with the weapon
		SpawnControlPanels();

		bool showControlPanels = weapon && weapon->ShouldShowControlPanels();
		SetControlPanelsActive( showControlPanels );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatWeapon
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseViewModel::GetOwningWeapon( void )
{
	return m_hWeapon.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : sequence - 
//-----------------------------------------------------------------------------
void CBaseViewModel::SendViewModelMatchingSequence( int sequence )
{
	// since all we do is send a sequence number down to the client, 
	// set this here so other weapons code knows which sequence is playing.
	SetSequence( sequence );

	m_nAnimationParity = ( m_nAnimationParity + 1 ) & ( (1<<VIEWMODEL_ANIMATION_PARITY_BITS) - 1 );

#if defined( CLIENT_DLL )
	m_nOldAnimationParity = m_nAnimationParity;

	// Force frame interpolation to start at exactly frame zero
	m_flAnimTime			= gpGlobals->curtime;
	m_iv_flAnimTime.Reset();
#else
	CBaseCombatWeapon *weapon = m_hWeapon.Get();
	bool showControlPanels = weapon && weapon->ShouldShowControlPanels();
	SetControlPanelsActive( showControlPanels );
#endif

	// Restart animation at frame 0
	m_flCycle = 0;
	ResetSequenceInfo();
}

#if defined( CLIENT_DLL )
#include "ivieweffects.h"
#endif

void CBaseViewModel::CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles )
{
	// UNDONE: Calc this on the server?  Disabled for now as it seems unnecessary to have this info on the server
#if defined( CLIENT_DLL )
	QAngle vmangoriginal = eyeAngles;
	QAngle vmangles = eyeAngles;
	Vector vmorigin = eyePosition;

	CBaseCombatWeapon *pWeapon = m_hWeapon.Get();
	//Allow weapon lagging
	if ( pWeapon != NULL )
	{
#if defined( CLIENT_DLL )
		if ( !prediction->InPrediction() )
#endif
		{
			pWeapon->AddViewmodelBob( this, vmorigin, vmangles );
			CalcViewModelLag( vmorigin, vmangles, vmangoriginal );
		}
	}

#if defined( CLIENT_DLL )
	if ( !prediction->InPrediction() )
	{
		// Let the viewmodel shake at about 10% of the amplitude of the player's view
		vieweffects->ApplyShake( vmorigin, vmangles, 0.9 );	
	}
#endif

	SetAbsOrigin( vmorigin );
	SetAbsAngles( vmangles );
#endif
}

#define MAX_VIEWMODEL_LAG 1.5f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseViewModel::CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles )
{
	// Calculate our drift
	Vector	forward;
	AngleVectors( angles, &forward, NULL, NULL );

	Vector vDifference;
	VectorSubtract( forward, m_vecLastFacing, vDifference );

	if ( fabs( vDifference.x ) > MAX_VIEWMODEL_LAG ||
		 fabs( vDifference.y ) > MAX_VIEWMODEL_LAG ||
		 fabs( vDifference.z ) > MAX_VIEWMODEL_LAG )
	{
		m_vecLastFacing = forward;
	}

	// FIXME:  Needs to be predictable?
	VectorMA( m_vecLastFacing, 5.0f * gpGlobals->frametime, vDifference, m_vecLastFacing );
	// Make sure it doesn't grow out of control!!!
	VectorNormalize( m_vecLastFacing );
	VectorMA( origin, 5, vDifference * -1, origin );

	Vector right, up;
	AngleVectors( original_angles, &forward, &right, &up );

	float pitch = original_angles[ PITCH ];
	if ( pitch > 180.0f )
		pitch -= 360.0f;
	else if ( pitch < -180.0f )
		pitch += 360.0f;

	//FIXME: These are the old settings that caused too many exposed polys on some models
	VectorMA( origin, -pitch * 0.035f,	forward,	origin );
	VectorMA( origin, -pitch * 0.03f,		right,	origin );
	VectorMA( origin, -pitch * 0.02f,		up,		origin);
}

#if !defined( TF2_DLL ) && !defined( TF2_CLIENT_DLL )

#if defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Stub to keep networking consistent for DEM files
//-----------------------------------------------------------------------------
extern void RecvProxy_AnimTime( const CRecvProxyData *pData, void *pStruct, void *pOut );

void RecvProxy_ViewModelAnimTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	RecvProxy_AnimTime( pData, pStruct, pOut );
}

#else

void SendProxy_AnimTime( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

#endif

#endif

LINK_ENTITY_TO_CLASS( viewmodel, CBaseViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( BaseViewModel, DT_BaseViewModel )

BEGIN_NETWORK_TABLE_NOBASE(CBaseViewModel, DT_BaseViewModel)
#if !defined( CLIENT_DLL )
	SendPropModelIndex(SENDINFO(m_nModelIndex)),
	SendPropInt		(SENDINFO(m_nBody), 8),
	SendPropInt		(SENDINFO(m_nSkin), 10),
	SendPropInt		(SENDINFO(m_nSequence),	8, SPROP_UNSIGNED),
	SendPropInt		(SENDINFO(m_nViewModelIndex), VIEWMODEL_INDEX_BITS, SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO(m_flPlaybackRate),	8,	SPROP_ROUNDUP,	-4.0,	12.0f),
	SendPropInt		(SENDINFO(m_fEffects),		10, SPROP_UNSIGNED),
	SendPropInt		(SENDINFO(m_nAnimationParity), 3, SPROP_UNSIGNED ),
	SendPropEHandle (SENDINFO(m_hWeapon)),

	SendPropInt( SENDINFO( m_nNewSequenceParity ), EF_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nResetEventsParity ), EF_PARITY_BITS, SPROP_UNSIGNED ),

#if !defined( TF2_DLL ) && !defined( TF2_CLIENT_DLL )
	SendPropArray	(SendPropFloat(SENDINFO_ARRAY(m_flPoseParameter),	8, 0, 0.0f, 1.0f), m_flPoseParameter),
	SendPropInt		(SENDINFO(m_flAnimTime),	8, SPROP_UNSIGNED, SendProxy_AnimTime ),
#endif
#else
	RecvPropInt		(RECVINFO(m_nModelIndex)),
	RecvPropInt		(RECVINFO(m_nSkin)),
	RecvPropInt		(RECVINFO(m_nBody)),
	RecvPropInt		(RECVINFO(m_nSequence)),
	RecvPropInt		(RECVINFO(m_nViewModelIndex)),
	RecvPropFloat	(RECVINFO(m_flPlaybackRate)),
	RecvPropInt		(RECVINFO(m_fEffects)),
	RecvPropInt		(RECVINFO(m_nAnimationParity)),
	RecvPropEHandle (RECVINFO(m_hWeapon)),

	RecvPropInt( RECVINFO( m_nNewSequenceParity )),
	RecvPropInt( RECVINFO( m_nResetEventsParity )),

#if !defined( TF2_DLL ) && !defined( TF2_CLIENT_DLL )
	RecvPropArray(RecvPropFloat(RECVINFO(m_flPoseParameter[0]) ), m_flPoseParameter ),
	RecvPropInt(	RECVINFO(m_flAnimTime), 0, RecvProxy_ViewModelAnimTime ),
#endif
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CBaseViewModel )

	// Networked
	DEFINE_PRED_FIELD( CBaseViewModel, m_nModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_PRED_FIELD( CBaseViewModel, m_nSkin, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CBaseViewModel, m_nBody, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CBaseViewModel, m_nSequence, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CBaseViewModel, m_nViewModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( CBaseViewModel, m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.125f ),
	DEFINE_PRED_FIELD( CBaseViewModel, m_fEffects, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_OVERRIDE ),
	DEFINE_PRED_FIELD( CBaseViewModel, m_nAnimationParity, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CBaseViewModel, m_hWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD( CBaseViewModel, m_hOwner, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseViewModel, m_flTimeWeaponIdle, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseViewModel, m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseViewModel, m_flCycle, FIELD_FLOAT ),

#if defined( CLIENT_DLL )
	DEFINE_FIELD( CBaseViewModel, m_bAnimationRestart, FIELD_BOOLEAN ),
#endif

END_PREDICTION_DATA()
