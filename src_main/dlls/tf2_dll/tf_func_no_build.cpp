//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Place where you can't build certain items.
//=============================================================================
#include "cbase.h"
#include "tf_func_no_build.h"
#include "tf_team.h"
#include "NDebugOverlay.h"

//-----------------------------------------------------------------------------
// Purpose: Defines an area from which resources can be collected
//-----------------------------------------------------------------------------
class CFuncNoBuild : public CBaseEntity
{
	DECLARE_CLASS( CFuncNoBuild, CBaseEntity );
public:
	CFuncNoBuild();

	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Activate( void );

	// Inputs
	void	InputSetActive( inputdata_t &inputdata );
	void	InputSetInactive( inputdata_t &inputdata );
	void	InputToggleActive( inputdata_t &inputdata );

	void	SetActive( bool bActive );
	bool	GetActive() const;

	bool	IsEmpty( void );
	bool	PointIsWithin( const Vector &vecPoint );

	bool	PreventsBuildOf( int iObjectType );

	// need to transmit to players who are in commander mode
	bool	ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );

private:
	bool	m_bActive;
	bool	m_bOnlyPreventTowers;
};

LINK_ENTITY_TO_CLASS( func_no_build, CFuncNoBuild);

BEGIN_DATADESC( CFuncNoBuild )

	// keys 
	DEFINE_KEYFIELD_NOT_SAVED( CFuncNoBuild, m_bOnlyPreventTowers, FIELD_BOOLEAN, "OnlyPreventTowers" ),

	// inputs
	DEFINE_INPUTFUNC( CFuncNoBuild, FIELD_VOID, "SetActive", InputSetActive ),
	DEFINE_INPUTFUNC( CFuncNoBuild, FIELD_VOID, "SetInactive", InputSetInactive ),
	DEFINE_INPUTFUNC( CFuncNoBuild, FIELD_VOID, "ToggleActive", InputToggleActive ),

END_DATADESC()


PRECACHE_REGISTER( func_no_build );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncNoBuild::CFuncNoBuild()
{
	NetworkStateManualMode( true );
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the resource zone
//-----------------------------------------------------------------------------
void CFuncNoBuild::Spawn( void )
{
	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_TRIGGER );
	SetMoveType( MOVETYPE_NONE );
	m_fEffects |= EF_NODRAW;
	SetModel( STRING( GetModelName() ) );
	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncNoBuild::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: See if we've got a gather point specified 
//-----------------------------------------------------------------------------
void CFuncNoBuild::Activate( void )
{
	BaseClass::Activate();
	SetActive( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncNoBuild::InputSetActive( inputdata_t &inputdata )
{
	SetActive( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncNoBuild::InputSetInactive( inputdata_t &inputdata )
{
	SetActive( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncNoBuild::InputToggleActive( inputdata_t &inputdata )
{
	if ( m_bActive )
	{
		SetActive( false );
	}
	else
	{
		SetActive( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncNoBuild::SetActive( bool bActive )
{
	m_bActive = bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncNoBuild::GetActive() const
{
	return m_bActive;
}


//-----------------------------------------------------------------------------
// Purpose: Return true if the specified point is within this zone
//-----------------------------------------------------------------------------
bool CFuncNoBuild::PointIsWithin( const Vector &vecPoint )
{
	return ( ( vecPoint.x > GetAbsMins().x && vecPoint.x < GetAbsMaxs().x ) &&
  	   ( vecPoint.y > GetAbsMins().y && vecPoint.y < GetAbsMaxs().y ) &&
	   ( vecPoint.z > GetAbsMins().z && vecPoint.z < GetAbsMaxs().z ) );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this zone prevents building of the specified type
//-----------------------------------------------------------------------------
bool CFuncNoBuild::PreventsBuildOf( int iObjectType )
{
	if ( m_bOnlyPreventTowers )
		return ( iObjectType == OBJ_TOWER );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Transmit this to all players who are in commander mode
//-----------------------------------------------------------------------------
bool CFuncNoBuild::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
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
// Purpose: Does a nobuild zone prevent us from building?
//-----------------------------------------------------------------------------
bool PointInNoBuild( CBaseObject *pObject, const Vector &vecBuildOrigin )
{
	// Find out whether we're in a resource zone or not
	CBaseEntity *pEntity = NULL;
	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "func_no_build" )) != NULL)
	{
		CFuncNoBuild *pNoBuild = (CFuncNoBuild *)pEntity;

		// Are we within this no build?
		if ( pNoBuild->GetActive() && pNoBuild->PointIsWithin( vecBuildOrigin ) )
		{
			if ( pNoBuild->PreventsBuildOf( pObject->GetType() ) )
				return true;	// prevent building.
		}
	}

	return false; // Building should be ok.
}

//-----------------------------------------------------------------------------
// Purpose: Return true if a nobuild zone prevents building this object at the specified origin
//-----------------------------------------------------------------------------
bool NoBuildPreventsBuild( CBaseObject *pObject, const Vector &vecBuildOrigin )
{
	// Find out whether we're in a no build zone or not
	if ( PointInNoBuild( pObject, vecBuildOrigin ) )
	{
		return true;
	}

	return false;
}
