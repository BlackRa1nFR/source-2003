//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( ILAUNCHERUI_H )
#define ILAUNCHERUI_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Create/shows hide launcher vgui interface elements
//-----------------------------------------------------------------------------
class ILauncherUI
{
public:
	virtual void		Create( void *root ) = 0;
	virtual void		Show( bool showlauncher ) = 0;

	virtual bool		IsActive( void ) = 0;
	virtual void		RequestFocus( void ) = 0;
};

extern ILauncherUI *launcherui;

#endif // ILAUNCHERUI_H