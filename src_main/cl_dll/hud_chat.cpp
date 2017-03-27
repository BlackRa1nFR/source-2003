//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud_chat.h"
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/RichText.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include "parsemsg.h"
#include "iclientmode.h"
#include "hud_macros.h"
#include "engine/IEngineSound.h"
#include "voice_status.h"
#include "text_message.h"
#include <vgui/ILocalize.h>
#include "vguicenterprint.h"
#include "vgui/keycode.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CHAT_ITEM_HEIGHT 19
#define CHAT_WIDTH_PERCENTAGE 0.6f
#define CHAT_VOICEICON_WIDTH 128

DECLARE_HUD_MESSAGE( CHudChat, TextMsg );

static ConVar hud_saytext_time( "hud_saytext_time", "12", 0 );

class CHudVoiceIconArea : public vgui::Panel
{
public:
	CHudVoiceIconArea( vgui::Panel *parent, const char *panelName );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//			*panelName - 
//-----------------------------------------------------------------------------
CHudVoiceIconArea::CHudVoiceIconArea( vgui::Panel *parent, const char *panelName )
: vgui::Panel( parent, panelName )
{
	// Just a placeholder, don't draw anything right now
	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );
	SetPaintEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: An output/display line of the chat interface
//-----------------------------------------------------------------------------
class CHudChatLine : public vgui::Label
{
	typedef vgui::Label BaseClass;

public:
	CHudChatLine( vgui::Panel *parent, const char *panelName );

	void			SetExpireTime( void );

	bool			IsReadyToExpire( void );

	void			Expire( void );

	float			GetStartTime( void );

	int				GetCount( void );

	virtual void	Paint();
	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	float			m_flExpireTime;
	float			m_flStartTime;
	int				m_nCount;

	vgui::HFont		m_hFont;
	vgui::HFont		m_hFontMarlett;

	Color		m_clrText;

private:
	CHudChatLine( const CHudChatLine & ); // not defined, not accessible
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//			*panelName - 
//-----------------------------------------------------------------------------
CHudChatLine::CHudChatLine( vgui::Panel *parent, const char *panelName ) : 
	vgui::Label( parent, panelName, "" )
{
	m_hFont = m_hFontMarlett = 0;
	m_flExpireTime = 0.0f;
	m_flStartTime = 0.0f;

	SetPaintBackgroundEnabled( false );

	SetContentAlignment( vgui::Label::a_west );
	SetTextInset( 10, 0 );
}


void CHudChatLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont( "Default" );
	m_hFontMarlett = pScheme->GetFont( "Marlett" );

	m_clrText = pScheme->GetColor( "FgColor", GetFgColor() );
	SetFont( m_hFont );
}


#define CHATLINE_NUM_FLASHES 8.0f
#define CHATLINE_FLASH_TIME 5.0f
#define CHATLINE_FADE_TIME 1.0f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudChatLine::Paint()
{
	// Flash + Extra bright when new
	float curtime = gpGlobals->curtime;

	int lr = m_clrText[0];
	int lg = m_clrText[1];
	int lb = m_clrText[2];
	if ( curtime >= m_flStartTime && curtime < m_flStartTime + CHATLINE_FLASH_TIME )
	{
		float frac1 = ( curtime - m_flStartTime ) / CHATLINE_FLASH_TIME;
		float frac = frac1;

		frac *= CHATLINE_NUM_FLASHES;
		frac *= 2 * M_PI;

		frac = cos( frac );

		frac = clamp( frac, 0.0f, 1.0f );

		frac *= (1.0f-frac1);

		int r = lr, g = lg, b = lb;

		r = r + ( 255 - r ) * frac;
		g = g + ( 255 - g ) * frac;
		b = b + ( 255 - b ) * frac;

		SetFgColor( Color( r, g, b, 255 ) );

		// Draw a right facing triangle in red, faded out over time
		int alpha = 63 + 192 * (1.0f - frac1 );
		alpha = clamp( alpha, 0, 255 );

		vgui::surface()->DrawSetTextFont( m_hFontMarlett );

		vgui::surface()->DrawSetTextColor( alpha, 0, 0, alpha );
		vgui::surface()->DrawSetTextPos( 0, 4 );
		wchar_t ch[2];
		vgui::localize()->ConvertANSIToUnicode( "4", ch, sizeof( ch ) );
		vgui::surface()->DrawPrintText( ch, 1 );
	}
	else if ( curtime <= m_flExpireTime && curtime > m_flExpireTime - CHATLINE_FADE_TIME )
	{
		float frac = ( m_flExpireTime - curtime ) / CHATLINE_FADE_TIME;

		int alpha = frac * 255;
		alpha = clamp( alpha, 0, 255 );

		SetFgColor( Color( lr * frac, lg * frac, lb * frac, alpha ) );
	}
	else
	{
		SetFgColor( Color( lr, lg, lb, 255 ) );
	}

	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CHudChatLine::SetExpireTime( void )
{
	m_flStartTime = gpGlobals->curtime;
	m_flExpireTime = m_flStartTime + hud_saytext_time.GetFloat();
	m_nCount = CHudChat::m_nLineCounter++;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHudChatLine::GetCount( void )
{
	return m_nCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudChatLine::IsReadyToExpire( void )
{
	// Engine disconnected, expire right away
	if ( !engine->IsInGame() && !engine->IsConnected() )
		return true;

	if ( gpGlobals->curtime >= m_flExpireTime )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CHudChatLine::GetStartTime( void )
{
	return m_flStartTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudChatLine::Expire( void )
{
	SetVisible( false );

	// Spit out label text now
	char text[ 256 ];
	GetText( text, 256 );

	Msg( "%s\n", text );
}

class CHudChatEntry : public vgui::TextEntry
{
	typedef vgui::TextEntry BaseClass;
public:
	CHudChatEntry( vgui::Panel *parent, char const *panelName, CHudChat *pChat )
		: BaseClass( parent, panelName )
	{
		SetCatchEnterKey( true );
		m_pHudChat = pChat;
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings(pScheme);

		SetPaintBorderEnabled( false );
	}

	virtual void OnKeyCodeTyped(vgui::KeyCode code)
	{
		if ( code == vgui::KEY_ENTER || code == vgui::KEY_PAD_ENTER || code == vgui::KEY_ESCAPE )
		{
			if ( code != vgui::KEY_ESCAPE )
			{
				if ( m_pHudChat )
					m_pHudChat->Send();
			}
		
			// End message mode.
			if ( m_pHudChat )
				m_pHudChat->StopMessageMode();
		}
		else
		{
			BaseClass::OnKeyCodeTyped( code );
		}
	}

private:
	CHudChat *m_pHudChat;
};

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
class CHudChatInputLine : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
	
public:
	CHudChatInputLine( CHudChat *parent, char const *panelName );

	void			SetPrompt( const char *prompt );
	void			ClearEntry( void );
	void			SetEntry( const char *entry );
	void			GetMessageText( char *buffer, int buffersize );

	virtual void	PerformLayout();
	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);

	vgui::Panel		*GetInputPanel( void );
	virtual vgui::VPANEL GetCurrentKeyFocus() { return m_pInput->GetVPanel(); } 

	virtual void Paint()
	{
		BaseClass::Paint();
	}

private:
	vgui::Label		*m_pPrompt;
	CHudChatEntry	*m_pInput;
};

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
CHudChatInputLine::CHudChatInputLine( CHudChat *parent, char const *panelName ) : 
	vgui::Panel( parent, panelName )
{
	SetMouseInputEnabled( false );

	m_pPrompt = new vgui::Label( this, "ChatInputPrompt", "Enter text:" );
	m_pInput = new CHudChatEntry( this, "ChatInput", parent );
}

void CHudChatInputLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// FIXME:  Outline
	vgui::HFont hFont = pScheme->GetFont( "Trebuchet18" );
	m_pPrompt->SetFont( hFont );
	m_pInput->SetFont( hFont );

	SetPaintBackgroundEnabled( false );
	m_pPrompt->SetPaintBackgroundEnabled( false );
	m_pPrompt->SetContentAlignment( vgui::Label::a_west );
	m_pPrompt->SetTextInset( 2, 0 );

	m_pInput->SetFgColor( GetFgColor() );
	m_pInput->SetBgColor( GetBgColor() );

}

void CHudChatInputLine::SetPrompt( const char *prompt )
{
	Assert( m_pPrompt );
	m_pPrompt->SetText( prompt );
	InvalidateLayout();
}

void CHudChatInputLine::ClearEntry( void )
{
	Assert( m_pInput );
	SetEntry( "" );
}

void CHudChatInputLine::SetEntry( const char *entry )
{
	Assert( m_pInput );
	Assert( entry );

	m_pInput->SetText( entry );
}

void CHudChatInputLine::GetMessageText( char *buffer, int buffersize )
{
	// FIXME:!!! Convert me to unicode!!
	m_pInput->GetText( buffer, buffersize);

	//wchar_t *unicode = (wchar_t *)_alloca( buffersize*(sizeof(wchar_t) ));
	//m_pInput->GetText( unicode, buffersize*sizeof(wchar_t) );
	//vgui::localize()->ConvertUnicodeToANSI( unicode, buffer, buffersize);
}

void CHudChatInputLine::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize( wide, tall );

	int w,h;
	m_pPrompt->GetContentSize( w, h); 
	m_pPrompt->SetBounds( 0, 0, w, tall );

	m_pInput->SetBounds( w + 2, 0, wide - w - 2 , tall );
}

vgui::Panel *CHudChatInputLine::GetInputPanel( void )
{
	return m_pInput;
}


DECLARE_HUD_MESSAGE( CHudChat, SayText );
DECLARE_HUDELEMENT( CHudChat );



int CHudChat::m_nLineCounter = 1;
//-----------------------------------------------------------------------------
// Purpose: Text chat input/output hud element
//-----------------------------------------------------------------------------
CHudChat::CHudChat( const char *pElementName )
: CHudElement( pElementName ), BaseClass( NULL, "HudChat" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile("resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);

	m_nMessageMode = 0;

	m_pChatInput = new CHudChatInputLine( this, "ChatInputLine" );
	m_pChatInput->SetVisible( false );

	for ( int i = 0; i < CHAT_INTERFACE_LINES; i++ )
	{
		char sz[ 32 ];
		sprintf( sz, "ChatLine%02i", i );
		m_ChatLines[ i ] = new CHudChatLine( this, sz );
		m_ChatLines[ i ]->SetVisible( false );
	}

	m_pVoiceArea = new CHudVoiceIconArea( this, "VoiceArea" );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	// (We don't actually want input until they bring up the chat line).
	MakePopup();
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );

	m_nVisibleHeight = 0;
}

CHudChat::~CHudChat()
{
}

void CHudChat::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetPaintBackgroundEnabled( false );

	// Put input area at bottom
	int w, h;
	GetSize( w, h );
	m_pChatInput->SetBounds( 1, h - CHAT_ITEM_HEIGHT - 1, w-2, CHAT_ITEM_HEIGHT );
}

void CHudChat::Paint( void )
{
	if ( m_nVisibleHeight == 0 )
		return;

	int w, h;
	GetSize( w, h );

	vgui::surface()->DrawSetColor( GetBgColor() );
	vgui::surface()->DrawFilledRect( 0, h - m_nVisibleHeight, w, h );

	vgui::surface()->DrawSetColor( GetFgColor() );
	vgui::surface()->DrawOutlinedRect( 0, h - m_nVisibleHeight, w, h );
}

void CHudChat::Init( void )
{
	HOOK_MESSAGE( SayText );
	HOOK_MESSAGE( TextMsg );
}

static int __cdecl SortLines( void const *line1, void const *line2 )
{
	CHudChatLine *l1 = *( CHudChatLine ** )line1;
	CHudChatLine *l2 = *( CHudChatLine ** )line2;

	// Invisible at bottom
	if ( l1->IsVisible() && !l2->IsVisible() )
		return -1;
	else if ( !l1->IsVisible() && l2->IsVisible() )
		return 1;

	// Oldest start time at top
	if ( l1->GetStartTime() < l2->GetStartTime() )
		return -1;
	else if ( l1->GetStartTime() > l2->GetStartTime() )
		return 1;

	// Otherwise, compare counter
	if ( l1->GetCount() < l2->GetCount() )
		return -1;
	else if ( l1->GetCount() > l2->GetCount() )
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Do respositioning here to avoid latency due to repositioning of vgui
//  voice manager icon panel
//-----------------------------------------------------------------------------
void CHudChat::OnTick( void )
{
	int i;
	for ( i = 0; i < CHAT_INTERFACE_LINES; i++ )
	{
		CHudChatLine *line = m_ChatLines[ i ];
		if ( !line )
			continue;

		if ( !line->IsVisible() )
			continue;

		if ( !line->IsReadyToExpire() )
			continue;

		line->Expire();
	}

	int voiceAreaHeight = GetClientVoiceMgr()->ComputeRequiredHeight();

	int w, h;

	GetSize( w, h );

	if ( voiceAreaHeight )
	{
		// No regular text, just voice icon
		w -= CHAT_VOICEICON_WIDTH;
	}

	// Sort chat lines 
	qsort( m_ChatLines, CHAT_INTERFACE_LINES, sizeof( CHudChatLine * ), SortLines );

	// Step backward from bottom
	int currentY = h - CHAT_ITEM_HEIGHT - 1;
	int startY = currentY;
	int ystep = CHAT_ITEM_HEIGHT;

	// Move position if it was visible
	if ( m_pChatInput->IsVisible() )
	{
		currentY -= ystep;
	}

	// Walk backward
	for ( i = CHAT_INTERFACE_LINES - 1; i >= 0 ; i-- )
	{
		CHudChatLine *line = m_ChatLines[ i ];
		if ( !line )
			continue;

		if ( !line->IsVisible() )
		{
			line->SetSize( w, CHAT_ITEM_HEIGHT );
			continue;
		}

		line->SetBounds( 
			0, currentY,
			w, CHAT_ITEM_HEIGHT );

		currentY -= ystep;
	}

	// Position voice area in remaining space
	if ( voiceAreaHeight != 0 )
	{
		m_pVoiceArea->SetBounds( w, h - voiceAreaHeight - 1, CHAT_VOICEICON_WIDTH - 1, voiceAreaHeight );
	}

	GetClientVoiceMgr()->RepositionLabels();

	if ( currentY != startY )
	{
		m_nVisibleHeight = startY - max( currentY, voiceAreaHeight ) + 2;
	}
	else
	{
		m_nVisibleHeight = 0;
	}
}

// Release build is crashing on long strings...sigh
#pragma optimize( "", off )

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : width - 
//			*text - 
//			textlen - 
// Output : int
//-----------------------------------------------------------------------------
int CHudChat::ComputeBreakChar( int width, const char *text, int textlen )
{
	CHudChatLine *line = m_ChatLines[ 0 ];
	vgui::HFont font = line->GetFont();

	int currentlen = 0;
	int lastbreak = textlen;
	for (int i = 0; i < textlen ; i++)
	{
		char ch = text[i];

		if ( ch <= 32 )
		{
			lastbreak = i;
		}

		wchar_t wch[2];

		vgui::localize()->ConvertANSIToUnicode( &ch, wch, sizeof( wch ) );

		int a,b,c;

		vgui::surface()->GetCharABCwide(font, wch[0], a, b, c);
		currentlen += a + b + c;

		if ( currentlen >= width )
		{
			// If we haven't found a whitespace char to break on before getting
			//  to the end, but it's still too long, break on the character just before
			//  this one
			if ( lastbreak == textlen )
			{
				lastbreak = max( 0, i - 1 );
			}
			break;
		}
	}

	if ( currentlen >= width )
	{
		return lastbreak;
	}
	return textlen;
}

static char *FixupAmpersands( const char *in )
{
	static char outstr[ 4096 ];

	const char *i = in;
	char *o = outstr;

	while ( *i )
	{
		if ( o - outstr >= 4094 )
		{
			break;
		}

		if ( *i == '&' )
		{
			*o++ = '&';
		}
		*o++ = *i++;
	}
	*o = 0;
	return outstr;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CHudChat::Printf( const char *fmt, ... )
{
	va_list marker;
	char msg[4096];

	va_start(marker, fmt);
	vsprintf(msg, fmt, marker);
	va_end(marker);

	// Strip any trailing '\n'
	if ( strlen( msg ) > 0 && msg[ strlen( msg )-1 ] == '\n' )
	{
		msg[ strlen( msg ) - 1 ] = 0;
	}

	// Strip leading \n characters ( or notify/color signifiers )
	char *pmsg = msg;
	while ( *pmsg && ( *pmsg == '\n' || *pmsg == 1 || *pmsg == 2 ) )
	{
		pmsg++;
	}
	
	if ( !*pmsg )
		return;

	// Search for return characters
	char *pos = strstr( pmsg, "\n" );
	if ( pos )
	{
		char msgcopy[ 8192 ];
		strcpy( msgcopy, pmsg );

		int offset = pos - pmsg;
		
		// Terminate first part
		msgcopy[ offset ] = 0;
		
		// Print first part
		Printf( msgcopy );

		// Print remainder
		Printf( &msgcopy[ offset ] + 1 );
		return;
	}

	CHudChatLine *firstline = m_ChatLines[ 0 ];

	int len = strlen( pmsg );

	// Check for string too long and split into multiple lines 
	// 
	int breakpos = ComputeBreakChar( firstline->GetWide() - 10, pmsg, len );
	if ( breakpos > 0 && breakpos < len )
	{
		char msgcopy[ 8192 ];
		strcpy( msgcopy, pmsg );

		int offset = breakpos;
		
		char savechar;

		savechar = msgcopy[ offset ];

		// Terminate first part
		msgcopy[ offset ] = 0;
		
		// Print first part
		Printf( msgcopy );

		// Was breakpos a printable char?
		if ( savechar > 32 )
		{
			msgcopy[ offset ] = savechar;

			// Print remainder
			Printf( &msgcopy[ offset ] );
		}
		else
		{
			Printf( &msgcopy[ offset ] + 1 );
		}
		return;
	}

	CHudChatLine *line = FindUnusedChatLine();
	if ( !line )
	{
		ExpireOldest();
		line = FindUnusedChatLine();
	}

	if ( !line )
	{
		return;
	}

	pmsg = FixupAmpersands( pmsg );

	line->SetExpireTime();
	line->SetText( pmsg );
	line->SetVisible( true );

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "HudChat.Message" );
}
	
#pragma optimize( "", on )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudChat::StartMessageMode( int iMessageModeType )
{
	m_nMessageMode = iMessageModeType;

	m_pChatInput->ClearEntry();

	if ( m_nMessageMode == MM_SAY )
	{
		m_pChatInput->SetPrompt( "Say :" );
	}
	else
	{
		m_pChatInput->SetPrompt( "Say (TEAM) :" );
	}
	
	SetKeyBoardInputEnabled( true );
	m_pChatInput->SetVisible( true );
	m_pChatInput->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudChat::StopMessageMode( void )
{
	SetKeyBoardInputEnabled( false );
	m_pChatInput->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
//-----------------------------------------------------------------------------
void CHudChat::MsgFunc_SayText( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	READ_BYTE();		// the client who spoke the message
	Printf( "%s", READ_STRING() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CHudChatLine
//-----------------------------------------------------------------------------
CHudChatLine *CHudChat::FindUnusedChatLine( void )
{
	for ( int i = 0; i < CHAT_INTERFACE_LINES; i++ )
	{
		CHudChatLine *line = m_ChatLines[ i ];
		if ( !line )
			continue;

		if ( line->IsVisible() )
			continue;

		return line;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudChat::ExpireOldest( void )
{
	float oldestTime = 100000000.0f;
	CHudChatLine *oldest = NULL;

	for ( int i = 0; i < CHAT_INTERFACE_LINES; i++ )
	{
		CHudChatLine *line = m_ChatLines[ i ];
		if ( !line )
			continue;

		if ( !line->IsVisible() )
			continue;

		if ( !oldest )
		{
			oldest = line;
			oldestTime = line->GetStartTime();
			continue;
		}

		if ( line->GetStartTime() < oldestTime )
		{
			oldest = line;
			oldestTime = line->GetStartTime();
		}
	}

	if ( !oldest )
	{
		oldest = m_ChatLines[ 0  ];
	}

	oldest->Expire();
}

void CHudChat::Send( void )
{
	char szbuf[256];
	char szTextbuf[256];
	m_pChatInput->GetMessageText( szTextbuf, sizeof( szTextbuf ) );
	
	// remove the \n
	if ( strlen( szTextbuf ) > 0 &&
		szTextbuf[ strlen( szTextbuf ) - 1 ] == '\n' )
	{
		szTextbuf[ strlen(szTextbuf) -1 ] = 0;
	}

	Q_snprintf( szbuf, sizeof(szbuf) - 1, "%s \"%s\"", m_nMessageMode == MM_SAY ? "say" : "say_team", szTextbuf );

	szbuf[ sizeof(szbuf) -1 ] = 0;
	engine->ClientCmd(szbuf);

	m_pChatInput->ClearEntry();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : vgui::Panel
//-----------------------------------------------------------------------------
vgui::Panel *CHudChat::GetInputPanel( void )
{
	return m_pChatInput->GetInputPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
vgui::Panel	*CHudChat::GetVoiceArea( void )
{
	return m_pVoiceArea;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudChat::Clear( void )
{
	// Kill input prompt
	StopMessageMode();

	// Expire all messages
	for ( int i = 0; i < CHAT_INTERFACE_LINES; i++ )
	{
		CHudChatLine *line = m_ChatLines[ i ];
		if ( !line )
			continue;

		if ( !line->IsVisible() )
			continue;

		line->Expire();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *newmap - 
//-----------------------------------------------------------------------------
void CHudChat::LevelInit( const char *newmap )
{
	Clear();
}

void CHudChat::LevelShutdown( void )
{
	Clear();
}

static void StripEndNewlineFromString( char *str )
{
	int s = strlen( str ) - 1;
	if ( str[s] == '\n' || str[s] == '\r' )
		str[s] = 0;
}

// converts all '\r' characters to '\n', so that the engine can deal with the properly
// returns a pointer to str
static char* ConvertCRtoNL( char *str )
{
	for ( char *ch = str; *ch != 0; ch++ )
		if ( *ch == '\r' )
			*ch = '\n';
	return str;
}

// Message handler for text messages
// displays a string, looking them up from the titles.txt file, which can be localised
// parameters:
//   byte:   message direction  ( HUD_PRINTCONSOLE, HUD_PRINTNOTIFY, HUD_PRINTCENTER, HUD_PRINTTALK )
//   string: message
// optional parameters:
//   string: message parameter 1
//   string: message parameter 2
//   string: message parameter 3
//   string: message parameter 4
// any string that starts with the character '#' is a message name, and is used to look up the real message in titles.txt
// the next (optional) one to four strings are parameters for that string (which can also be message names if they begin with '#')
void CHudChat::MsgFunc_TextMsg( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int msg_dest = READ_BYTE();

	static char szBuf[6][128];
	char *msg_text = hudtextmessage->LookupString( READ_STRING(), &msg_dest );
	Q_strncpy( szBuf[0], msg_text, sizeof( szBuf[0] ) );
	msg_text = szBuf[0];

	// keep reading strings and using C format strings for subsituting the strings into the localised text string
	char *sstr1 = hudtextmessage->LookupString( READ_STRING() );
	Q_strncpy( szBuf[1], sstr1, sizeof( szBuf[1] ) );
	sstr1 = szBuf[1];

	StripEndNewlineFromString( sstr1 );  // these strings are meant for subsitution into the main strings, so cull the automatic end newlines
	char *sstr2 = hudtextmessage->LookupString( READ_STRING() );
	Q_strncpy( szBuf[2], sstr2, sizeof( szBuf[2] ) );
	sstr2 = szBuf[2];
	
	StripEndNewlineFromString( sstr2 );
	char *sstr3 = hudtextmessage->LookupString( READ_STRING() );
	Q_strncpy( szBuf[3], sstr3, sizeof( szBuf[3] ) );
	sstr3 = szBuf[3];

	StripEndNewlineFromString( sstr3 );
	char *sstr4 = hudtextmessage->LookupString( READ_STRING() );
	Q_strncpy( szBuf[4], sstr4, sizeof( szBuf[4] ) );
	sstr4 = szBuf[4];
	
	StripEndNewlineFromString( sstr4 );
	char *psz = szBuf[5];

	switch ( msg_dest )
	{
	case HUD_PRINTCENTER:
		Q_snprintf( psz, sizeof( szBuf[5] ), msg_text, sstr1, sstr2, sstr3, sstr4 );
		internalCenterPrint->Print( ConvertCRtoNL( psz ) );
		break;

	case HUD_PRINTNOTIFY:
		psz[0] = 1;  // mark this message to go into the notify buffer
		Q_snprintf( psz+1, sizeof( szBuf[5] ) - 1, msg_text, sstr1, sstr2, sstr3, sstr4 );
		Msg( ConvertCRtoNL( psz ) );
		break;

	case HUD_PRINTTALK:
		Q_snprintf( psz, sizeof( szBuf[5] ), msg_text, sstr1, sstr2, sstr3, sstr4 );
		Printf( "%s", ConvertCRtoNL( psz ) );
		break;

	case HUD_PRINTCONSOLE:
		Q_snprintf( psz, sizeof( szBuf[5] ), msg_text, sstr1, sstr2, sstr3, sstr4 );
		Msg( ConvertCRtoNL( psz ) );
		break;
	}
}
