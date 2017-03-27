//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: area portal entity: toggles visibility areas on/off
//
// NOTE: These are not really brush entities.  They are brush entities from a 
// designer/worldcraft perspective, but by the time they reach the game, the 
// brush model is gone and this is, in effect, a point entity.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

class CFuncOccluder : public CBaseEntity
{
public:
	DECLARE_CLASS( CFuncOccluder, CBaseEntity );

					CFuncOccluder();

	virtual void	Spawn( void );
	virtual bool	ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );

	// Input handlers
	void InputActivate( inputdata_t &inputdata );
	void InputDeactivate( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

private:
	CNetworkVar( bool, m_bActive );
	CNetworkVar( int, m_nOccluderIndex );
};

LINK_ENTITY_TO_CLASS( func_occluder, CFuncOccluder );

IMPLEMENT_SERVERCLASS_ST_NOBASE(CFuncOccluder, DT_FuncOccluder)
	SendPropBool( SENDINFO(m_bActive) ),
	SendPropInt(SENDINFO(m_nOccluderIndex),	10, SPROP_UNSIGNED ),
END_SEND_TABLE()


BEGIN_DATADESC( CFuncOccluder )

	DEFINE_KEYFIELD( CFuncOccluder, m_bActive, FIELD_BOOLEAN, "StartActive" ),

	// NOTE: This keyfield is computed + inserted by VBSP
	DEFINE_KEYFIELD( CFuncOccluder, m_nOccluderIndex, FIELD_INTEGER, "occludernumber" ),

	// Inputs
	DEFINE_INPUTFUNC( CFuncOccluder, FIELD_VOID, "Deactivate",  InputDeactivate ),
	DEFINE_INPUTFUNC( CFuncOccluder, FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC( CFuncOccluder, FIELD_VOID, "Toggle",  InputToggle ),

END_DATADESC()


//------------------------------------------------------------------------------
// Occluder :
//------------------------------------------------------------------------------
CFuncOccluder::CFuncOccluder()
{
	m_bActive = true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncOccluder::Spawn( void )
{
    Precache( );    

	m_takedamage	= DAMAGE_NO;
	SetSolid( SOLID_NONE );
    SetMoveType( MOVETYPE_NONE );
	
	// set size and link into world.
	SetModel( STRING( GetModelName() ) );
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CFuncOccluder::InputDeactivate( inputdata_t &inputdata )
{
	m_bActive = false;
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CFuncOccluder::InputActivate( inputdata_t &inputdata )
{
	m_bActive = true;
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CFuncOccluder::InputToggle( inputdata_t &inputdata )
{
	m_bActive = !m_bActive;
}


//------------------------------------------------------------------------------
// We always want to transmit these bad boys
//------------------------------------------------------------------------------
bool CFuncOccluder::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
{
	return true;
}

