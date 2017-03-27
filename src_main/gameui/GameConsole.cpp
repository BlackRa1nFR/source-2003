//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "GameConsole.h"
#include "GameConsoleDialog.h"
#include <vgui/ISurface.h>

#include <KeyValues.h>
#include <vgui/Cursor.h>
#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifndef min
	#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

static CGameConsole g_GameConsole;
//-----------------------------------------------------------------------------
// Purpose: singleton accessor
//-----------------------------------------------------------------------------
CGameConsole &GameConsole()
{
	return g_GameConsole;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameConsole, IGameConsole, GAMECONSOLE_INTERFACE_VERSION, g_GameConsole);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameConsole::CGameConsole()
{
	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameConsole::~CGameConsole()
{
	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: sets up the console for use
//-----------------------------------------------------------------------------
void CGameConsole::Initialize()
{
	m_pConsole = vgui::SETUP_PANEL( new CGameConsoleDialog() ); // we add text before displaying this so set it up now!
		int swide, stall;
	m_pConsole->SetParent(g_pTaskbar->GetVPanel());

	vgui::surface()->GetScreenSize(swide, stall);
	int offset = 40;
	m_pConsole->SetBounds(
		offset, offset, 
		min( swide - 2 * offset, 560 ), min( stall - 2 * offset, 400 ) );
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: activates the console, makes it visible and brings it to the foreground
//-----------------------------------------------------------------------------
void CGameConsole::Activate()
{
	if (!m_bInitialized)
		return;

	vgui::surface()->RestrictPaintToSinglePanel(NULL);
	m_pConsole->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: hides the console
//-----------------------------------------------------------------------------
void CGameConsole::Hide()
{
	if (!m_bInitialized)
		return;

	m_pConsole->Hide();
}

//-----------------------------------------------------------------------------
// Purpose: clears the console
//-----------------------------------------------------------------------------
void CGameConsole::Clear()
{
	if (!m_bInitialized)
		return;

	m_pConsole->Clear();
}

//-----------------------------------------------------------------------------
// Purpose: prints a message to the console
//-----------------------------------------------------------------------------
void CGameConsole::Printf(const char *format, ...)
{
	if (!m_bInitialized)
		return;

	m_pConsole->Print(format);
}

//-----------------------------------------------------------------------------
// Purpose: printes a debug message to the console
//-----------------------------------------------------------------------------
void CGameConsole::DPrintf(const char *format, ...)
{
	if (!m_bInitialized)
		return;

	m_pConsole->DPrint(format);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : clr - 
//			*format - 
//			... - 
//-----------------------------------------------------------------------------
void CGameConsole::ColorPrintf( Color& clr, const char *format, ...)
{
	if (!m_bInitialized)
		return;

	m_pConsole->ColorPrintf( clr, format );
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the console is currently in focus
//-----------------------------------------------------------------------------
bool CGameConsole::IsConsoleVisible()
{
	if (!m_bInitialized)
		return false;
	
	return m_pConsole->IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: activates the console after a delay
//-----------------------------------------------------------------------------
void CGameConsole::ActivateDelayed(float time)
{
	if (!m_bInitialized)
		return;

	m_pConsole->PostMessage(m_pConsole, new KeyValues("Activate"), time);
}

void CGameConsole::SetParent( int parent )
{	
	if (!m_bInitialized)
		return;

	m_pConsole->SetParent( static_cast<vgui::VPANEL>( parent ));
}

