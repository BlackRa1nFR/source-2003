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

#include <vgui/Cursor.h>
#include "clientmode_tfbase.h"
#include <vgui_controls/EditablePanel.h>

namespace vgui
{
	class AnimationController;
}


class ClientModeTFNormal : public ClientModeTFBase
{
DECLARE_CLASS( ClientModeTFNormal, ClientModeTFBase );

private:

	class Viewport : public vgui::EditablePanel
	{
	typedef vgui::EditablePanel BaseClass;
	// Panel overrides.
	public:
		Viewport();

		virtual void	Enable();
		virtual void	Disable();
		virtual void	OnThink();

		vgui::AnimationController *GetAnimationController() { return m_pAnimController; }

		void			ReloadScheme();

		virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
		{
			BaseClass::ApplySchemeSettings( pScheme );

			gHUD.InitColors( pScheme );

			SetPaintBackgroundEnabled( false );
		}

	private:
		vgui::HCursor	m_CursorNone;
		vgui::AnimationController *m_pAnimController;

		bool			m_bHumanScheme;
	};

// IClientMode overrides.
public:

					ClientModeTFNormal();
	virtual			~ClientModeTFNormal();

	virtual void	Init( void );

	virtual void	Enable();
	virtual void	Disable();
	
	virtual void	Update();
	virtual void	Layout();
	virtual void	OverrideView( CViewSetup *pSetup );
	virtual void	CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *cmd );
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual bool	ShouldDrawViewModel( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual void	AdjustEngineViewport( int& x, int& y, int& width, int& height );
	virtual void	PreRender( CViewSetup *pSetup );
	virtual void	PostRenderWorld();
	virtual void	PostRender( void );

	virtual vgui::AnimationController *GetViewportAnimationController();

	virtual vgui::Panel*	GetViewport() { return &m_Viewport; }

	virtual vgui::Panel *GetMinimapParent( void );

public:
	
	void			ReloadScheme( void );


private:

	vgui::HCursor				m_CursorNone;
	ClientModeTFNormal::Viewport	m_Viewport;
};

extern IClientMode *GetClientModeNormal();