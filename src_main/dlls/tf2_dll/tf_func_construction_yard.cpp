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
#include "tf_func_construction_yard.h"
#include "tf_team.h"

//-----------------------------------------------------------------------------
// Purpose: Defines an area from which resources can be collected
//-----------------------------------------------------------------------------
class CFuncConstructionYard : public CBaseEntity
{
	DECLARE_CLASS( CFuncConstructionYard, CBaseEntity );
public:
	CFuncConstructionYard();

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	virtual void UpdateOnRemove( void );
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

	// need to transmit to players who are in commander mode
	bool	ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );

private:
	bool m_bActive;
};

LINK_ENTITY_TO_CLASS( func_construction_yard, CFuncConstructionYard);

BEGIN_DATADESC( CFuncConstructionYard )

	DEFINE_INPUTFUNC( CFuncConstructionYard, FIELD_VOID, "SetActive", InputSetActive ),
	DEFINE_INPUTFUNC( CFuncConstructionYard, FIELD_VOID, "SetInactive", InputSetInactive ),
	DEFINE_INPUTFUNC( CFuncConstructionYard, FIELD_VOID, "ToggleActive", InputToggleActive ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CFuncConstructionYard, DT_FuncConstructionYard)
END_SEND_TABLE();

PRECACHE_REGISTER( func_construction_yard );

// List of construction yards for fast lookup
typedef CHandle<CFuncConstructionYard>	YardHandle;
CUtlVector< YardHandle >	g_hConstructionYards;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncConstructionYard::CFuncConstructionYard()
{
	NetworkStateManualMode( true );
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the resource zone
//-----------------------------------------------------------------------------
void CFuncConstructionYard::Spawn( void )
{
	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_TRIGGER );
	SetMoveType( MOVETYPE_NONE );
	m_fEffects |= EF_NODRAW;
	SetModel( STRING( GetModelName() ) );
	m_bActive = false;

	YardHandle hYard;
	hYard = this;
	g_hConstructionYards.AddToTail( hYard );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncConstructionYard::UpdateOnRemove( void )
{
	YardHandle hYard;
	hYard = this;
	g_hConstructionYards.FindAndRemove( hYard );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncConstructionYard::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: See if we've got a gather point specified 
//-----------------------------------------------------------------------------
void CFuncConstructionYard::Activate( void )
{
	BaseClass::Activate();
	SetActive( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncConstructionYard::InputSetActive( inputdata_t &inputdata )
{
	SetActive( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncConstructionYard::InputSetInactive( inputdata_t &inputdata )
{
	SetActive( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncConstructionYard::InputToggleActive( inputdata_t &inputdata )
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
void CFuncConstructionYard::SetActive( bool bActive )
{
	m_bActive = bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncConstructionYard::GetActive() const
{
	return m_bActive;
}


//-----------------------------------------------------------------------------
// Purpose: Return true if the specified point is within this zone
//-----------------------------------------------------------------------------
bool CFuncConstructionYard::PointIsWithin( const Vector &vecPoint )
{
	return ( ( vecPoint.x > GetAbsMins().x && vecPoint.x < GetAbsMaxs().x ) &&
  	   ( vecPoint.y > GetAbsMins().y && vecPoint.y < GetAbsMaxs().y ) &&
	   ( vecPoint.z > GetAbsMins().z && vecPoint.z < GetAbsMaxs().z ) );
}


//-----------------------------------------------------------------------------
// Purpose: Transmit this to all players who are in commander mode
//-----------------------------------------------------------------------------
bool CFuncConstructionYard::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
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
// Does a construction yard (or lack of one) prevent us from building?
//-----------------------------------------------------------------------------
bool PointInConstructionYard( const Vector &vecBuildOrigin )
{
	// Find out whether we're in a construction yard or not
	for ( int i = 0; i < g_hConstructionYards.Count(); i++ )
	{
		CFuncConstructionYard *pYard = g_hConstructionYards[i];
		if ( pYard && pYard->PointIsWithin( vecBuildOrigin ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ConstructionYardPreventsBuild( CBaseObject *pObject, const Vector &vecBuildOrigin )
{
	bool bMustBuildInYard = pObject->MustBeBuiltInConstructionYard();

	// Find out whether we're in a resource zone or not
	if( PointInConstructionYard( vecBuildOrigin ) )
	{
		if( !pObject->MustNotBeBuiltInConstructionYard() )
			return false;		// An object that must be built in a yard is in a yard.
		else
			return true;		// An object that can't be built in a yard is in a yard.
	}

	// If we have to be built in a construction yard, we're not in one
	if ( bMustBuildInYard )
		return true;

	return false;
}
