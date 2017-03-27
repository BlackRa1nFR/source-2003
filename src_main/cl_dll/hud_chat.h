//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef HUD_CHAT_H
#define HUD_CHAT_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include "vgui_basepanel.h"
#include "vgui_controls/frame.h"

class CHudChatLine;
class CHudChatInputLine;
class CHudVoiceIconArea;

namespace vgui
{
	class IScheme;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudChat : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudChat, vgui::EditablePanel );
public:
	DECLARE_MULTIPLY_INHERITED();

	enum
	{
		CHAT_INTERFACE_LINES = 6,
		MAX_CHARS_PER_LINE = 128
	};

					CHudChat( const char *pElementName );
					~CHudChat();
	
	virtual void	Init( void );

	void			LevelInit( const char *newmap );
	void			LevelShutdown( void );

	void			MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf);

	void			Printf( const char *fmt, ... );
	
	void			StartMessageMode( int iMessageModeType );
	void			StopMessageMode( void );
	void			Send( void );

	// Network message handler
	void			MsgFunc_SayText( const char *pszName, int iSize, void *pbuf );

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void	Paint( void );
	virtual void	OnTick( void );

	vgui::Panel		*GetInputPanel( void );
	vgui::Panel		*GetVoiceArea( void );

	static int		m_nLineCounter;

private:
	void			Clear( void );

	CHudChatLine	*FindUnusedChatLine( void );
	void			ExpireOldest( void );

	int				ComputeBreakChar( int width, const char *text, int textlen );

	CHudChatLine	*m_ChatLines[ CHAT_INTERFACE_LINES ];
	CHudChatInputLine	*m_pChatInput;
	CHudVoiceIconArea	*m_pVoiceArea;

	int				m_nMessageMode;

	int				m_nVisibleHeight;
};

#endif // HUD_CHAT_H
