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
#if !defined( VGUICENTERPRINT_H )
#define VGUICENTERPRINT_H
#ifdef _WIN32
#pragma once
#endif

#include "ivguicenterprint.h"
#include <vgui/vgui.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
namespace vgui
{
class Panel;
}

class CCenterStringPanel;
class CCenterPrint : public ICenterPrint
{
private:
	CCenterStringPanel	*vguiCenterString;

public:
						CCenterPrint( void );

	virtual void		Create( vgui::VPANEL parent );
	virtual void		Destroy( void );
	
	virtual void		SetTextColor( int r, int g, int b, int a );
	virtual void		Print( char *fmt, ... );
	virtual void		ColorPrint( int r, int g, int b, int a, char *fmt, ... );
	virtual void		Clear( void );
};

extern CCenterPrint *internalCenterPrint;

#endif // VGUICENTERPRINT_H