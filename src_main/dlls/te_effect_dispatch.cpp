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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "basetempentity.h"
#include "te_effect_dispatch.h"
#include "networkstringtable_gamedll.h"


//-----------------------------------------------------------------------------
// Purpose: This TE provides a simple interface to dispatch effects by name using DispatchEffect().
//-----------------------------------------------------------------------------
class CTEEffectDispatch : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEEffectDispatch, CBaseTempEntity );

					CTEEffectDispatch( const char *name );
	virtual			~CTEEffectDispatch( void );

	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

	DECLARE_SERVERCLASS();

public:
	CEffectData m_EffectData;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEEffectDispatch::CTEEffectDispatch( const char *name ) :
	CBaseTempEntity( name )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEEffectDispatch::~CTEEffectDispatch( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//-----------------------------------------------------------------------------
void CTEEffectDispatch::Create( IRecipientFilter& filter, float delay )
{
	engine->PlaybackTempEntity( filter, delay, 
		(void *)this, GetServerClass()->m_pTable, GetServerClass()->m_ClassID );
}


IMPLEMENT_SERVERCLASS_ST( CTEEffectDispatch, DT_TEEffectDispatch )

	SendPropDataTable( SENDINFO_DT( m_EffectData ), &REFERENCE_SEND_TABLE( DT_EffectData ) )

END_SEND_TABLE()


// Singleton to fire TEEffectDispatch objects
static CTEEffectDispatch g_TEEffectDispatch( "EffectDispatch" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TE_DispatchEffect( IRecipientFilter& filter, float delay, const Vector &pos, const char *pName, const CEffectData &data )
{
	// Copy the supplied effect data.
	g_TEEffectDispatch.m_EffectData = data;

	// Get the entry index in the string table.
	g_TEEffectDispatch.m_EffectData.m_iEffectName = networkstringtable->AddString( g_StringTableEffectDispatch, pName );

	// Send it to anyone who can see the effect's origin.
	g_TEEffectDispatch.Create( filter, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DispatchEffect( const char *pName, const CEffectData &data )
{
	CPASFilter filter( data.m_vOrigin );
	te->DispatchEffect( filter, 0.0, data.m_vOrigin, pName, data );
}
