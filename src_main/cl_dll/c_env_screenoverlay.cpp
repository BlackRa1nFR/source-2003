//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "shareddefs.h"
#include "materialsystem/imesh.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_EnvScreenOverlay : public C_BaseEntity
{
	DECLARE_CLASS( C_EnvScreenOverlay, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	void	PostDataUpdate( DataUpdateType_t updateType );

	void	StartOverlays( void );
	void	StartCurrentOverlay( void );
	void	ClientThink( void );

protected:
	char	m_iszOverlayNames[ MAX_SCREEN_OVERLAYS ][255];
	float	m_flOverlayTimes[ MAX_SCREEN_OVERLAYS ];
	float	m_flStartTime;
	int		m_iCurrentOverlay;
	float	m_flCurrentOverlayTime;
};

IMPLEMENT_CLIENTCLASS_DT( C_EnvScreenOverlay, DT_EnvScreenOverlay, CEnvScreenOverlay )
	RecvPropArray( RecvPropString( RECVINFO( m_iszOverlayNames[0]) ), m_iszOverlayNames ),
	RecvPropArray( RecvPropFloat( RECVINFO( m_flOverlayTimes[0] ) ), m_flOverlayTimes ),
	RecvPropFloat( RECVINFO( m_flStartTime ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::PostDataUpdate( DataUpdateType_t updateType )
{
	// If we have a start time now, start the overlays going
	if ( m_flStartTime )
	{
		StartOverlays();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::StartOverlays( void )
{
	m_iCurrentOverlay = 0;
	m_flCurrentOverlayTime = 0;
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	StartCurrentOverlay();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::StartCurrentOverlay( void )
{
	if ( m_iCurrentOverlay == MAX_SCREEN_OVERLAYS || !m_iszOverlayNames[m_iCurrentOverlay] || !m_iszOverlayNames[m_iCurrentOverlay][0] )
	{
		// Hit the end of our overlays, so stop.
		m_flStartTime = 0;
		SetNextClientThink( CLIENT_THINK_NEVER );
		view->SetScreenOverlayMaterial( NULL );
		return;
	}

	m_flCurrentOverlayTime = gpGlobals->curtime + m_flOverlayTimes[m_iCurrentOverlay];

	// Bring up the current overlay
	bool bFound;
	IMaterial *pMaterial = materials->FindMaterial( m_iszOverlayNames[m_iCurrentOverlay], &bFound, false );
	if ( bFound )
	{
		view->SetScreenOverlayMaterial( pMaterial );
	}
	else
	{
		Warning("env_screenoverlay couldn't find overlay %s.\n", m_iszOverlayNames[m_iCurrentOverlay] );
		view->SetScreenOverlayMaterial( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::ClientThink( void )
{
	// If the current overlay's run out, go to the next one
	if ( m_flCurrentOverlayTime < gpGlobals->curtime )
	{
		m_iCurrentOverlay++;
		StartCurrentOverlay();
	}
}