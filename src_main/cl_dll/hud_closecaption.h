//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef HUD_CLOSECAPTION_H
#define HUD_CLOSECAPTION_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CSentence;
class C_BaseFlex;
class CCloseCaptionItem;
class CCloseCaptionPhrase;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudCloseCaption : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudCloseCaption, vgui::Panel );
public:
	DECLARE_MULTIPLY_INHERITED();

					CHudCloseCaption( const char *pElementName );

	// Expire lingering items
	virtual void	OnTick( void );

	virtual void	LevelInit( void );

	virtual void	LevelShutdown( void )
	{
		Reset();
	}

	// Clear all CC data
	virtual void	Reset( void );
	virtual void	Process( C_BaseFlex *actor, float curtime, CSentence* sentence );

	// Painting methods
	virtual void	Paint();

private:
	void	DrawStream( wrect_t& rect, const CCloseCaptionPhrase *phrase ); 
	void	ComputeStreamSize( int available_width, const CCloseCaptionPhrase *phrase, int& w, int &h );

	CUtlVector< CCloseCaptionItem * > m_Items;

	enum
	{
		FONT_NORMAL = 0x00,
		FONT_BOLD	= 0x01,
		FONT_ITALIC	= 0x02
	};

	vgui::HFont		fontSet[ 4 ];

	void			CreateFonts( void );
	
	vgui::HFont		FindFont( int bits )
	{
		bits &= 0x03;
		return fontSet[ bits ];
	}
};


#endif // HUD_CLOSECAPTION_H
