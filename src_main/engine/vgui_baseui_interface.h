//========= Copyright © 1996-2000, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the interface that the GameUI dll exports
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEUI_INTERFACE_H
#define BASEUI_INTERFACE_H
#pragma once


#include <vgui_controls/Panel.h>
#include <vgui/VGUI.h>
#include <vgui_controls/PHandle.h>
#include <BaseUI/IBaseUI.h>

extern HWND* pmainwindow;

#define TRACE_FUNCTION(str) 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnginePanel : public vgui::EditablePanel
{
	typedef vgui::EditablePanel BaseClass;
public:
	CEnginePanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
	{
		//m_bCanFocus = true;
		SetMouseInputEnabled( true );
		SetKeyBoardInputEnabled( true );
	}

	void EnableMouseFocus( bool state )
	{
		//m_bCanFocus = state;
		SetMouseInputEnabled( state );
		SetKeyBoardInputEnabled( state );
	}

/*	virtual vgui::VPANEL IsWithinTraverse(int x, int y, bool traversePopups)
	{
		if ( !m_bCanFocus )
			return NULL;

		vgui::VPANEL retval = BaseClass::IsWithinTraverse( x, y, traversePopups );
		if ( retval == GetVPanel() )
			return NULL;
		return retval;
	}*/

//private:
//	bool		m_bCanFocus;
};


extern CEnginePanel *staticClientDLLPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStaticPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;

public:
	CStaticPanel( vgui::Panel *pParent, const char *pName ) : vgui::Panel( pParent, pName )
	{
		SetCursor( vgui::dc_none );
		SetKeyBoardInputEnabled( false );
		SetMouseInputEnabled( false );
	}

};

class CFocusOverlayPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;

public:
	CFocusOverlayPanel( vgui::Panel *pParent, const char *pName );

	virtual void PostChildPaint( void );
	static void GetColorForSlot( int slot, int& r, int& g, int& b )
	{
		r = (int)( 124.0 + slot * 47.3 ) & 255;
		g = (int)( 63.78 - slot * 71.4 ) & 255;
		b = (int)( 188.42 + slot * 13.57 ) & 255;
	}
};




//-----------------------------------------------------------------------------
// Purpose: Implementation of BaseUI's exposed interface 
//-----------------------------------------------------------------------------
class CBaseUI : public IBaseUI
{
public:
	CBaseUI();
	~CBaseUI();

	void Initialize( CreateInterfaceFn *factories, int count );
	void Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion);
	void Shutdown();
	
//	int	HasExclusiveInput();
	int Key_Event(int down, int keynum, const char *pszCurrentBinding);

	void CallEngineSurfaceProc(void *hwnd, unsigned int msg, unsigned int wparam, long lparam);
	void Paint(int x, int y, int right, int bottom);

	void HideGameUI();
	void ActivateGameUI();
	void HideConsole();
	void ShowConsole();
	bool IsGameUIVisible();

private:
	enum { MAX_NUM_FACTORIES = 5 };
	CreateInterfaceFn m_FactoryList[MAX_NUM_FACTORIES];
	int m_iNumFactories;

	CSysModule *m_hVGuiModule ;
	CSysModule *m_hStaticGameUIModule;
};

// Purpose: singleton accessor
extern CBaseUI &BaseUI();


#endif // BASEUI_INTERFACE_H
