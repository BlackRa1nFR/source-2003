//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Place where we can build vehicles
//=============================================================================
#include "cbase.h"
#include "tf_func_mass_teleport.h"
#include "tf_team.h"
#include "basetfvehicle.h"
#include "team_spawnpoint.h"
#include "NDebugOverlay.h"
#include "tf_vehicle_teleport_station.h"
#include "tf_gamerules.h"

LINK_ENTITY_TO_CLASS( func_mass_teleport, CFuncMassTeleport);

BEGIN_DATADESC( CFuncMassTeleport )

	// Data
	DEFINE_FIELD(CFuncMassTeleport, m_hTargetEntity, FIELD_EHANDLE),

	// Keys
	DEFINE_KEYFIELD_NOT_SAVED( CFuncMassTeleport, m_bMCV,	FIELD_BOOLEAN, "MCV" ),

	// Inputs
	DEFINE_INPUTFUNC( CFuncMassTeleport, FIELD_VOID, "DoTeleport", InputDoTeleport ),

	// Outputs
	DEFINE_OUTPUT( CFuncMassTeleport, m_OnTeleport, "OnTeleport" ),
	DEFINE_OUTPUT( CFuncMassTeleport, m_OnActive, "OnActive" ),
	DEFINE_OUTPUT( CFuncMassTeleport, m_OnInactive, "OnInactive" ),

END_DATADESC()

PRECACHE_REGISTER( func_mass_teleport );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncMassTeleport::CFuncMassTeleport()
{
	NetworkStateManualMode( true );
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the resource zone
//-----------------------------------------------------------------------------
void CFuncMassTeleport::Spawn( void )
{
	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_TRIGGER );
	SetMoveType( MOVETYPE_NONE );
	m_fEffects |= EF_NODRAW;
	SetModel( STRING( GetModelName() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncMassTeleport::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncMassTeleport::Activate( void )
{
	m_hTargetEntity = GetNextTarget();
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool PointIsWithin( Vector vecPoint, Vector& vecMins, Vector& vecMaxs )
{
	return ( ( vecPoint.x > vecMins.x && vecPoint.x < vecMaxs.x ) &&
  	   ( vecPoint.y > vecMins.y && vecPoint.y < vecMaxs.y ) &&
	   ( vecPoint.z > vecMins.z && vecPoint.z < vecMaxs.z ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncMassTeleport::MassTeleport( 
	CTFTeam *pTeam, 
	Vector vSourceMins, 
	Vector vSourceMaxs, 
	Vector vDest, 
	bool bSwap, 
	bool bPlayersOnly,
	EHANDLE hMCV,
	bool bTelefragDestination )
{
	bSwap = false;   

	Vector vSource = vSourceMins + ( ( vSourceMaxs - vSourceMins ) / 2.f );
	vSource.z = vSourceMins.z;
/*
	if( bTelefragDestination )
	{
		Vector vDestMins = vDest - ( ( vSourceMaxs - vSourceMins ) / 2.f );
		Vector vDestMaxs = vDest + ( ( vSourceMaxs - vSourceMins ) / 2.f );
		MassTelefrag(vDestMins,vDestMaxs);
	}
*/
	CBaseEntity *pList[1024];
	int OriginalSolidFlags[1024];

	int count = UTIL_EntitiesInBox( pList, 1024, vSourceMins, vSourceMaxs, FL_CLIENT|FL_NPC|FL_OBJECT );

	for( int i = 0; i < count; i++ )
	{
		CBaseEntity* pEnt = pList[i];

		if ( !pEnt )
			continue;
		
		if ( bPlayersOnly )
		{
			CBaseTFPlayer *pTestPlayer = dynamic_cast< CBaseTFPlayer* >( pEnt );
			if ( pTestPlayer )
			{
				// If it's players only, then only teleport players attached to the specified MCV.
				if ( pTestPlayer->GetSelectedMCV() != hMCV.Get() )
				{
					pList[i] = NULL;
					continue;
				}
			}
			else
			{
				pList[i] = NULL;
				continue;
			}
		}


		if( pEnt->GetSolidFlags() & FSOLID_NOT_SOLID )
		{
			pList[i] = NULL;
			continue;
		}

		// Only teleport team members
		if ( pTeam && ((CTFTeam*)pEnt->GetTeam()) != pTeam )
		{
			pList[i] = NULL;
			continue;
		}

		// Only teleport items at the root of hierarchies.
		if( pEnt->GetMoveParent() )
		{
			pList[i] = NULL;
			continue;
		}

		CBaseObject* pObj = dynamic_cast<CBaseObject*>(pEnt);

		// NJS: dont teleport map placed objects.
		if( pObj )
		{ 
			if ( pObj->WasMapPlaced() )
			{
				pList[i] = NULL;
				continue;
			}
		}

		OriginalSolidFlags[i] = pEnt->GetSolidFlags();
		pEnt->AddSolidFlags( FSOLID_NOT_SOLID);

		Vector vEntOrigin = pEnt->GetAbsOrigin();
		Vector vDelta = vEntOrigin - vSource;
		Vector vEntTarget = vDest + vDelta + Vector(0,0,3);	// Teleport them a little above the ground, to allow for the suspension drop.
		QAngle aEntAngles= pEnt->GetAbsAngles();
		QAngle aBogus;
		Vector vEntVelocity;
		pEnt->GetVelocity(&vEntVelocity,&aBogus);

		// If it's a player, trace down from the top of the box to find a safe place
		if ( bPlayersOnly && pEnt->IsPlayer() )
		{
			// Start the target up the top of the dest box
			Vector vecTop = vEntTarget;
			vecTop.z = (vDest.z + (vSourceMaxs.z - vSourceMins.z));
			// Trace a hull down
			trace_t tr;
			UTIL_TraceEntity( pEnt, vecTop, vEntTarget, MASK_SOLID, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );
			vEntTarget = tr.endpos + Vector(0,0,3);
		}
		
		/*
		if( ! bSwap )
		{
			Vector origin(0,0,0);
			if ( !EntityPlacementTest( pEnt, vEntTarget, origin, true ) )
			{
				UTIL_Remove( pEnt );
				return;
			}

			vEntTarget = origin;
		}
		*/

		pEnt->Teleport( &vEntTarget, &aEntAngles, &vEntVelocity );
	}

	// If I am to swap source and destination, do it while the current entites from this teleport are incorperal.
	if( bSwap )
	{
		MassTeleport( pTeam, vSourceMins - vSource + vDest, vSourceMaxs - vSource + vDest, vSource, false, bPlayersOnly, hMCV, false );
	}

	for( i = 0; i < count; i++ )
	{
		CBaseEntity* pEnt = pList[i];

		if( !pEnt )
			continue;

		pEnt->SetSolidFlags(OriginalSolidFlags[i]);
	}
}

void CFuncMassTeleport::MassTelefrag( Vector vMins, Vector vMaxs )
{
	CBaseEntity *pList[4096];
	
	int count = UTIL_EntitiesInBox( pList, 4096, vMins, vMaxs, FL_CLIENT|FL_NPC|FL_OBJECT );

	for( int i = 0; i < count; i++ )
	{
		CBaseEntity* pEnt = pList[i];

		if ( !pEnt )
			continue;
		

		if( pEnt->GetSolidFlags() & FSOLID_NOT_SOLID )
		{
			pList[i] = NULL;
			continue;
		}

		// Only teleport items at the root of hierarchies.
		if( pEnt->GetMoveParent() )
		{
			pList[i] = NULL;
			continue;
		}

		CBaseObject* pObj = dynamic_cast<CBaseObject*>(pEnt);

		// dont frag map placed objects.
		if( !pObj )
			continue;

		if ( pObj->WasMapPlaced() )
		{
			pList[i] = NULL;
			continue;
		}
		
		// Do a massive amount of damage (DMG_GENERIC so there's no physics force)
		CTakeDamageInfo info( this, this, 99999, DMG_GENERIC );

		pObj->TakeDamage(info);
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncMassTeleport::DoTeleport( 
	CTFTeam *pTeam, 
	Vector vSourceMins, 
	Vector vSourceMaxs, 
	Vector vDest, 
	bool bSwap, 
	bool bPlayersOnly,
	EHANDLE hMCV )
{ 
	bool bTelefragDestination = hMCV ? false : true;
	MassTeleport( pTeam, vSourceMins, vSourceMaxs, vDest, bSwap, bPlayersOnly, hMCV, bTelefragDestination );

	// Fire our output
	m_OnTeleport.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncMassTeleport::InputDoTeleport( inputdata_t &inputdata )
{ 
	// If I have MCVs attached, tell them to teleport.
	for ( int i = 0; i < m_MCVs.Count(); i++ )
	{
		m_MCVs[i]->DoTeleport();
	}

	// If we have a target entity, teleport to it
	if ( m_hTargetEntity )
	{
		DoTeleport( ((CTFTeam*)GetTeam()), GetAbsMins(), GetAbsMaxs(), m_hTargetEntity->GetAbsOrigin(), true, false, EHANDLE() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified point is within this zone
//-----------------------------------------------------------------------------
bool CFuncMassTeleport::PointIsWithin( const Vector &vecPoint )
{
	return ( ( vecPoint.x > GetAbsMins().x && vecPoint.x < GetAbsMaxs().x ) &&
  	   ( vecPoint.y > GetAbsMins().y && vecPoint.y < GetAbsMaxs().y ) &&
	   ( vecPoint.z > GetAbsMins().z && vecPoint.z < GetAbsMaxs().z ) );
}


//-----------------------------------------------------------------------------
// Purpose: Transmit this to all players who are in commander mode
//-----------------------------------------------------------------------------
bool CFuncMassTeleport::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
{
	// Team rules may tell us that we should
	CBaseEntity* pRecipientEntity = CBaseEntity::Instance( recipient );
	if ( pRecipientEntity->IsPlayer() )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)pRecipientEntity;
		if ( pPlayer->GetTeam() )
		{
			if (pPlayer->GetTeam()->ShouldTransmitToPlayer( pPlayer, this ))
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncMassTeleport::AddMCV( CBaseEntity *pMCV )
{
	// Are we going active?
	if ( !m_MCVs.Count() )
	{
		// Fire our output
		m_OnActive.FireOutput( this, this );
	}

	EHANDLE hMCV;
	hMCV = pMCV;
	m_MCVs.AddToTail( hMCV );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncMassTeleport::RemoveMCV( CBaseEntity *pMCV )
{
	EHANDLE hMCV;
	hMCV = pMCV;
	m_MCVs.FindAndRemove( hMCV );

	// Have we just gone inactive?
	if ( !m_MCVs.Count() )
	{
		// Fire our output
		m_OnInactive.FireOutput( this, this );
	}
}