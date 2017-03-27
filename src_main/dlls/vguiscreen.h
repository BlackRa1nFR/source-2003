//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: This is an entity that represents a vgui screen
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUISCREEN_H
#define VGUISCREEN_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// This is an entity that represents a vgui screen
//-----------------------------------------------------------------------------
class CVGuiScreen : public CBaseEntity
{
public:
	DECLARE_CLASS( CVGuiScreen, CBaseEntity );
	
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CVGuiScreen();

	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual void Spawn();

	const char *GetPanelName() const;

	// Sets the screen size + resolution
	void SetActualSize( float flWidth, float flHeight );

	// Activates/deactivates the screen
	bool IsActive() const;
	void SetActive( bool bActive );

	// Is this screen only visible to teammates?
	bool IsVisibleOnlyToTeammates() const;
	void MakeVisibleOnlyToTeammates( bool bActive );
	bool IsVisibleToTeam( int nTeam );

	// Sets the overlay material
	void SetOverlayMaterial( const char *pMaterial );

	void SetAttachedToViewModel( bool bAttached );
	bool IsAttachedToViewModel() const;

	virtual bool ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );
	
private:
	void SetAttachmentIndex( int nIndex );
 	void SetPanelName( const char *pPanelName );
	void InputSetActive( inputdata_t &inputdata );
	void InputSetInactive( inputdata_t &inputdata );

	CNetworkVar( float, m_flWidth ); 
	CNetworkVar( float, m_flHeight );
	CNetworkVar( int, m_nPanelName );	// The name of the panel 
	CNetworkVar( int, m_nAttachmentIndex );
	CNetworkVar( int, m_nOverlayMaterial );
	CNetworkVar( int, m_fScreenFlags );

	friend CVGuiScreen *CreateVGuiScreen( const char *pScreenClassname, const char *pScreenType, CBaseEntity *pAttachedTo, CBaseEntity *pOwner, int nAttachmentIndex );
};


void PrecacheVGuiScreen( const char *pScreenType );
void PrecacheVGuiScreenOverlayMaterial( const char *pMaterialName );
CVGuiScreen *CreateVGuiScreen( const char *pScreenClassname, const char *pScreenType, CBaseEntity *pAttachedTo, int nAttachmentIndex );
void DestroyVGuiScreen( CVGuiScreen *pVGuiScreen );


#endif // VGUISCREEN_H
