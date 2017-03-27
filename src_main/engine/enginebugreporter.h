//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef ENGINEBUGREPORTER_H
#define ENGINEBUGREPORTER_H
#ifdef _WIN32
#pragma once
#endif

namespace vgui
{
	class Panel;
};

class IEngineBugReporter
{
public:
	virtual void		Init( void ) = 0;
	virtual void		Shutdown( void ) = 0;

	virtual void		InstallBugReportingUI( vgui::Panel *parent ) = 0;
	virtual bool		ShouldPause() const = 0;
};

extern IEngineBugReporter *bugreporter;

#endif // ENGINEBUGREPORTER_H
