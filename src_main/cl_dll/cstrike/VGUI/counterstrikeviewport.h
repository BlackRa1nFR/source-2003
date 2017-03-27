//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef COUNTERSTRIKEVIEWPORT_H
#define COUNTERSTRIKEVIEWPORT_H

// viewport interface for the rest of the dll
#include "vgui/ICstrikeViewPortMsgs.h"

#include <game_controls/vgui_TeamFortressViewport.h>
#include "cs_shareddefs.h"
#include "basemodviewport.h"


using namespace vgui;

namespace vgui 
{
	class Panel;
}

class CCSTeamMenu;
class CCSClassMenu;
class CCSSpectatorGUI;
class CCSClientScoreBoard;
class CBuyMenu;
class CCSClientScoreBoardDialog;



//==============================================================================
class CounterStrikeViewport : public CBaseModViewport, public ICSViewPortMsgs
{

private:
	typedef CBaseModViewport BaseClass;

public:
	CounterStrikeViewport();
	~CounterStrikeViewport();

	virtual void Start( IGameUIFuncs *pGameUIFuncs, IGameEventManager * gameeventmanager );
	virtual void SetParent(vgui::VPANEL parent);
	virtual void Enable();
	virtual void Disable();
	virtual void OnThink();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void HideAllVGUIMenu( void );   
	
	virtual int	 KeyInput( int down, int keynum, const char *pszCurrentBinding );

	virtual void HideVGUIMenu( int iMenu );
	virtual void OnLevelChange(const char * mapname);
	virtual void ReloadScheme();

	AnimationController * GetAnimationController() { return m_pAnimController; }

	virtual void UpdateSpectatorPanel();
	// override this message
	virtual int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf );

private:

	virtual bool IsVGUIMenuActive( int iMenu );
	virtual void DisplayVGUIMenu( int iMenu );


	virtual void OnTick(); // per frame think function

		

	// main panels
	CCSTeamMenu *m_pTeamMenu;
	CCSClassMenu *m_pClassMenu;
	CBuyMenu	*m_pBuyMenu;
	CCSSpectatorGUI *m_pPrivateSpectatorGUI;
	CCSClientScoreBoardDialog *m_pPrivateClientScoreBoard;

	int			m_iDeadSpecMode; // used to update the m_playermenu for spectators

	HCursor					m_CursorNone;
	AnimationController*	m_pAnimController;
};


#endif // COUNTERSTRIKEVIEWPORT_H
