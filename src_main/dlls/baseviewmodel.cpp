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
#include "cbase.h"
#include "animation.h"
#include "baseviewmodel.h"
#include "player.h"
#include <KeyValues.h>
#include "studio.h"
#include "vguiscreen.h"
#include "saverestore_utlvector.h"


void SendProxy_AnimTime( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
void SendProxy_SequenceChanged( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

//-----------------------------------------------------------------------------
// Purpose: Save Data for Base Weapon object
//-----------------------------------------------------------------------------// 
BEGIN_DATADESC( CBaseViewModel )

	DEFINE_FIELD( CBaseViewModel, m_hOwner, FIELD_EHANDLE ),

// Client only
//	DEFINE_FIELD( CBaseViewModel, m_bAnimationRestart, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseViewModel, m_nViewModelIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseViewModel, m_flTimeWeaponIdle, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseViewModel, m_nAnimationParity, FIELD_INTEGER ),

	// Client only
//	DEFINE_FIELD( CBaseViewModel, m_nOldAnimationParity, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseViewModel, m_vecLastFacing, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseViewModel, m_hWeapon, FIELD_EHANDLE ),
	DEFINE_UTLVECTOR( CBaseViewModel, m_hScreens, FIELD_EHANDLE ),

// Read from weapons file
//	DEFINE_FIELD( CBaseViewModel, m_sVMName, FIELD_STRING ),
//	DEFINE_FIELD( CBaseViewModel, m_sAnimationPrefix, FIELD_STRING ),

// ---------------------------------------------------------------------

// Don't save these, init to 0 and regenerate
//	DEFINE_FIELD( CBaseViewModel, m_Activity, FIELD_INTEGER ),

END_DATADESC()

bool CBaseViewModel::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
{
	if (m_fEffects & EF_NODRAW)
	{
		return false;
	}

	CBasePlayer *pPlayer = ToBasePlayer( m_hOwner );
	if ( pPlayer && pPlayer->edict() == recipient )
	{
		return true;
	}

	// Don't send to anyone else except the local player
	return false;
}

void CBaseViewModel::SetTransmit( CCheckTransmitInfo *pInfo )
{
	// Are we already marked for transmission?
	if ( pInfo->WillTransmit( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo );
	
	// Force our screens to be sent too.
	for ( int i=0; i < m_hScreens.Count(); i++ )
	{
		CVGuiScreen *pScreen = m_hScreens[i].Get();
		if ( pScreen )
			pScreen->SetTransmit( pInfo );
	}
}
