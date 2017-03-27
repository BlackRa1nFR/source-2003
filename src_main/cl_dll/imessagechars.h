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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( IMESSAGECHARS_H )
#define IMESSAGECHARS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

namespace vgui
{
class Panel;
typedef unsigned long HFont;
}

class IMessageChars
{
public:
	enum
	{
		MESSAGESTRINGID_NONE = -1,
		MESSAGESTRINGID_BASE = 0
	};
	
	virtual	void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;

	// messageID can be MESSAGESTRINGID_NONE or MESSAGESTRINGID_BASE plus some offset. You can refer to the message by
	// its ID later.
	virtual int			DrawString( vgui::HFont pCustomFont, int x, int y, int r, int g, int b, int a, const char *fmt, int messageID, ... ) = 0;
	virtual int			DrawString( vgui::HFont pCustomFont, int x, int y, const char *fmt, int messageID, ... ) = 0;
	
	virtual int			DrawStringForTime( float flTime, vgui::HFont pCustomFont, int x, int y, int r, int g, int b, int a, const char *fmt, int messageID,  ... ) = 0;
	virtual int			DrawStringForTime( float flTime, vgui::HFont pCustomFont, int x, int y, const char *fmt, int messageID, ... ) = 0;
	
	// Remove all messages with the specified ID (passed into DrawStringForTime).
	virtual void		RemoveStringsByID( int messageID ) = 0;

	virtual void		GetStringLength( vgui::HFont pCustomFont, int *width, int *height, const char *fmt, ... ) = 0;

	virtual void		Clear( void ) = 0;
};

extern IMessageChars *messagechars;

#endif // IMESSAGECHARS_H