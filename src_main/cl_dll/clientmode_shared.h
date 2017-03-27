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
#if !defined( CLIENTMODE_NORMAL_H )
#define CLIENTMODE_NORMAL_H
#ifdef _WIN32
#pragma once
#endif

#include "iclientmode.h"

class CHudChat;
class CBaseHudWeaponSelection;
class CViewSetup;
class C_BaseEntity;
class C_BasePlayer;

namespace vgui
{
class Panel;
}


// This class implements client mode functionality common to HL2 and TF2.
class ClientModeShared : public IClientMode
{
// IClientMode overrides.
public:
	DECLARE_CLASS_NOBASE( ClientModeShared );

					ClientModeShared();
	virtual			~ClientModeShared();
	
	virtual void	Init();
	virtual void	Shutdown();

	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );

	virtual void	Enable();
	virtual void	Disable();
	virtual void	Layout();

	virtual void	OverrideView( CViewSetup *pSetup );
	virtual bool	ShouldDrawDetailObjects( );
	virtual bool	ShouldDrawEntity(C_BaseEntity *pEnt);
	virtual bool	ShouldDrawLocalPlayer( C_BasePlayer *pPlayer );
	virtual bool	ShouldDrawViewModel();
	virtual bool	ShouldDrawParticles( );
	virtual void	AdjustEngineViewport( int& x, int& y, int& width, int& height );
	virtual void	PreRender(CViewSetup *pSetup);
	virtual void	PostRenderWorld();
	virtual void	PostRender();
	virtual void	ProcessInput(bool bActive);
	virtual void	Update();

	// Input
	virtual int		KeyInput( int down, int keynum, const char *pszCurrentBinding );
	virtual void	OverrideMouseInput( int *x, int *y );
	virtual void	StartMessageMode( int iMessageModeType );
	virtual vgui::Panel *GetMessagePanel();

	virtual void	ActivateInGameVGuiContext( vgui::Panel *pPanel );
	virtual void	DeactivateInGameVGuiContext();

	// The mode can choose to not draw fog
	virtual bool	ShouldDrawFog( void );

protected:
	// Message mode handling
	// All modes share a common chat interface
	static CHudChat	*m_pChatElement;
	static CBaseHudWeaponSelection *m_pWeaponSelection;
};

#endif // CLIENTMODE_NORMAL_H

