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
#if !defined( ITEXTMESSAGE_H )
#define ITEXTMESSAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

namespace vgui
{
class Panel;
}

#include "fontabc.h"

class ITextMessage 
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;

	virtual void		SetPosition( int x, int y ) = 0;
	virtual void		AddChar( int r, int g, int b, int a, char ch ) = 0;

	virtual void		GetLength( int *wide, int *tall, const char *string ) = 0;
	virtual int			GetFontInfo( FONTABC *pABCs, vgui::HFont hFont ) = 0;

	virtual void		SetFont( vgui::HFont hCustomFont ) = 0;
	virtual void		SetDefaultFont( void ) = 0;
};

extern ITextMessage *textmessage;

#endif // ITEXTMESSAGE_H