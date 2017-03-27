//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "GameConsoleDialog.h"
#include "BaseUI/IBaseUI.h"

#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <KeyValues.h>

#include <vgui_controls/Button.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/RichText.h>

#include "igameuifuncs.h"
#include "keydefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

extern IBaseUI *baseuifuncs;


#include "Taskbar.h"
extern CTaskbar *g_pTaskbar;

//-----------------------------------------------------------------------------
// Purpose: forwards tab key presses up from the text entry so we can do autocomplete
//-----------------------------------------------------------------------------
class TabCatchingTextEntry : public TextEntry
{
public:
	TabCatchingTextEntry(Panel *parent, const char *name) : TextEntry(parent, name)
	{
	}

	virtual void OnKeyCodeTyped(KeyCode code)
	{
		if (code == KEY_TAB)
		{
			GetParent()->OnKeyCodeTyped(code);
		}
		else
		{
			TextEntry::OnKeyCodeTyped(code);
		}
	}

	virtual void OnKillFocus()
	{
		PostMessage(GetParent(), new KeyValues("CloseCompletionList"));
	}
};

// Things the user typed in and hit submit/return with
CHistoryItem::CHistoryItem( void )
{
	m_text = NULL;
	m_extraText = NULL;
	m_bHasExtra = false;
}

CHistoryItem::CHistoryItem( const char *text, const char *extra)
{
	Assert( text );
	m_text = NULL;
	m_extraText = NULL;
	m_bHasExtra = false;
	SetText( text , extra );
}

CHistoryItem::CHistoryItem( const CHistoryItem& src )
{
	m_text = NULL;
	m_extraText = NULL;
	m_bHasExtra = false;
	SetText( src.GetText(), src.GetExtra() );
}

CHistoryItem::~CHistoryItem( void )
{
	delete[] m_text;
	delete[] m_extraText;
	m_text = NULL;
}

const char *CHistoryItem::GetText() const
{
	if ( m_text )
	{
		return m_text;
	}
	else
	{
		return "";
	}
}

const char *CHistoryItem::GetExtra() const
{
	if ( m_extraText )
	{
		return m_extraText;
	}
	else
	{
		return NULL;
	}
}

void CHistoryItem::SetText( const char *text, const char *extra )
{
	delete[] m_text;
	m_text = new char[ strlen( text ) + 1 ];
	memset( m_text, 0x0, strlen( text ) + 1);
	strcpy( m_text, text );

	if ( extra )
	{
		m_bHasExtra = true;
		delete[] m_extraText;
		m_extraText = new char[ strlen( extra ) + 1 ];
		memset( m_extraText, 0x0, strlen( extra ) + 1);
		strcpy( m_extraText, extra );
	}
	else
	{
		m_bHasExtra = false;
	}
}



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameConsoleDialog::CGameConsoleDialog() : CTaskFrame(NULL, "GameConsole")
{
	// initialize dialog
	MakePopup();
	SetVisible(false);
	SetMinimumSize(100,100);

	g_pTaskbar->AddTask(GetVPanel());

	SetTitle("#GameUI_Console", true);

	// create controls
	m_pHistory = new RichText(this, "ConsoleHistory");
	m_pHistory->SetVerticalScrollbar(true);
	
	m_pEntry = new TabCatchingTextEntry(this, "ConsoleEntry");
	m_pEntry->AddActionSignalTarget(this);
	m_pEntry->SendNewLine(true);

	m_pSubmit = new Button(this, "ConsoleSubmit", "#GameUI_Submit");
	m_pSubmit->SetCommand("submit");
	m_pCompletionList = new Menu(this, "CompletionList");
	m_pCompletionList->MakePopup();
	m_pCompletionList->SetVisible(false);

	// need to set up default colors, since ApplySchemeSettings won't be called until later
	m_PrintColor = Color(216, 222, 211, 255);
	m_DPrintColor = Color(196, 181, 80, 255);

	m_pEntry->SetTabPosition(1);

	m_bAutoCompleteMode = false;
	m_szPartialText[0] = 0;
	m_szPreviousPartialText[0]=0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameConsoleDialog::~CGameConsoleDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: brings dialog to the fore
//-----------------------------------------------------------------------------
void CGameConsoleDialog::Activate()
{
	BaseClass::Activate();
	m_pEntry->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Clears the console
//-----------------------------------------------------------------------------
void CGameConsoleDialog::Clear()
{
	m_pHistory->SetText("");
}

//-----------------------------------------------------------------------------
// Purpose: normal text print
//-----------------------------------------------------------------------------
void CGameConsoleDialog::Print(const char *msg)
{
	m_pHistory->InsertColorChange(m_PrintColor);
	m_pHistory->InsertString(msg);
}

//-----------------------------------------------------------------------------
// Purpose: debug text print
//-----------------------------------------------------------------------------
void CGameConsoleDialog::DPrint(const char *msg)
{
	m_pHistory->InsertColorChange(m_DPrintColor);
	m_pHistory->InsertString(msg);
}

//-----------------------------------------------------------------------------
// Purpose: debug text print
//-----------------------------------------------------------------------------
void CGameConsoleDialog::ColorPrintf(Color& clr, const char *msg)
{
	m_pHistory->InsertColorChange(clr);
	m_pHistory->InsertString(msg);
}

static ConCommand *FindNamedCommand( char const *name )
{
	// look through the command list for all matches
	ConCommandBase const *cmd = (ConCommandBase const *)cvar->GetCommands();
	while (cmd)
	{
		if (!Q_strcmp( name, cmd->GetName() ) )
		{
			if ( cmd->IsCommand() )
			{
				return ( ConCommand * )cmd;
			}
		}
		cmd = cmd->GetNext();
	}
	return NULL;
}

static ConCommand *FindAutoCompleteCommmandFromPartial( char const *partial )
{
	char command[ 256 ];
	Q_strncpy( command, partial, sizeof( command ) );

	char *space = strstr( command, " " );
	if ( space )
	{
		*space = 0;
	}

	ConCommand *cmd = FindNamedCommand( command );
	if ( !cmd )
		return NULL;

	if ( !cmd->CanAutoComplete() )
		return NULL;

	return cmd;
}

//-----------------------------------------------------------------------------
// Purpose: rebuilds the list of possible completions from the current entered text
//-----------------------------------------------------------------------------
void CGameConsoleDialog::RebuildCompletionList(const char *text)
{
	// clear any old completion list
	m_CompletionList.RemoveAll();

	// we need the length of the text for the partial string compares
	int len = strlen(text);
	if (len < 1)
	{
		// Fill the completion list with history instead
		for ( int i = 0 ; i < m_CommandHistory.Count(); i++ )
		{
			CHistoryItem *item = &m_CommandHistory[ i ];
			CompletionItem *comp = &m_CompletionList[ m_CompletionList.AddToTail() ];
			comp->iscommand = false;
			comp->m_text = new CHistoryItem( *item );
		}
		return;
	}

/*	// look through the command list for all matches
	unsigned int cmd = engine->GetFirstCmdFunctionHandle();
	while (cmd)
	{
		if (!strnicmp(text, engine->GetCmdFunctionName(cmd), len))
		{
			// match found, add to list
			int node = m_CompletionList.AddToTail();
			CompletionItem *item = &m_CompletionList[node];
			item->iscommand = true;
			item->cmd.cmd = cmd;
			item->cmd.cvar = NULL;
			item->m_text = new CHistoryItem( engine->GetCmdFunctionName(cmd) );
		}

		cmd = engine->GetNextCmdFunctionHandle(cmd);
	}

	// walk the cvarlist looking for all the matches
	cvar_t *cvar = engine->GetFirstCvarPtr();
	while (cvar)
	{
		if (!_strnicmp(text, cvar->name, len))
		{
			// match found, add to list
			int node = m_CompletionList.AddToTail();
			CompletionItem *item = &m_CompletionList[node];
			item->iscommand = false;
			item->cmd.cmd = 0;
			item->cmd.cvar = cvar;

			char text[ 256 ];
			_snprintf( text, sizeof( text ), "%s \"%s\"", cvar->name, cvar->string);

			item->m_text = new CHistoryItem( text );
		}

		cvar = cvar->next;
	}
*/
	
	bool normalbuild = true;

	// if there is a space in the text, and the command isn't of the type to know how to autocomplet, then command completion is over
	char *space = strstr( text, " " );
	if (space)
	{
		char commandname[ 128 ];
		Q_strncpy( commandname, text, sizeof( commandname ) );
		// Find first space
		int spot = space - text;
		commandname[ spot ] = 0;

		ConCommand *command = FindNamedCommand( commandname );
		if ( !command || !command->CanAutoComplete() )
		{
			return;
		}

		normalbuild = false;

		char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ];
		int count = command->AutoCompleteSuggest( text, commands );
		int i;

		for ( i = 0; i < count; i++ )
		{
			// match found, add to list
			int node = m_CompletionList.AddToTail();
			CompletionItem *item = &m_CompletionList[node];
			item->iscommand = false;
			item->cmd.cmd = NULL;
			item->m_text = new CHistoryItem( commands[ i ] );
		}
	}

	if ( normalbuild )
	{
		// look through the command list for all matches
		ConCommandBase const *cmd = (ConCommandBase const *)cvar->GetCommands();
		while (cmd)
		{
			if (!strnicmp(text, cmd->GetName(), len))
			{
				// match found, add to list
				int node = m_CompletionList.AddToTail();
				CompletionItem *item = &m_CompletionList[node];
				item->iscommand = true;
				item->cmd.cmd = (ConCommandBase *)cmd;
				const char *tst = cmd->GetName();
				if ( ! cmd->IsCommand() )
				{
					ConVar *var = ( ConVar * )cmd;
					item->m_text = new CHistoryItem( var->GetName(), var->GetString() );
				}
				else
				{
					item->m_text = new CHistoryItem( tst );
				}
			}

			cmd = cmd->GetNext();
		}

		// Now sort the list by command name
		if ( m_CompletionList.Count() >= 2 )
		{
			for ( int i = 0 ; i < m_CompletionList.Count(); i++ )
			{
				for ( int j = i + 1; j < m_CompletionList.Count(); j++ )
				{
					CompletionItem temp;

					CompletionItem const *i1, *i2;

					ConCommandBase const *c1, *c2;

					i1 = &m_CompletionList[ i ];
					i2 = &m_CompletionList[ j ];

					c1 = i1->cmd.cmd;
					c2 = i2->cmd.cmd;
					Assert( c1 && c2 );

					if ( stricmp( c1->GetName(), c2->GetName() ) > 0 )
					{
						temp = m_CompletionList[ i ];
						m_CompletionList[ i ] = m_CompletionList[ j ];
						m_CompletionList[ j ] = temp;
					}
				}
			}
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: auto completes current text
//-----------------------------------------------------------------------------
void CGameConsoleDialog::OnAutoComplete(bool reverse)
{
	if (!m_bAutoCompleteMode)
	{
		// we're not in auto-complete mode, Start
		m_iNextCompletion = 0;
		m_bAutoCompleteMode = true;
	}

	// if we're in reverse, move back to before the current
	if (reverse)
	{
		m_iNextCompletion -= 2;
		if (m_iNextCompletion < 0)
		{
			// loop around in reverse
			m_iNextCompletion = m_CompletionList.Size() - 1;
		}
	}

	// get the next completion
	if (!m_CompletionList.IsValidIndex(m_iNextCompletion))
	{
		// loop completion list
		m_iNextCompletion = 0;
	}

	// make sure everything is still valid
	if (!m_CompletionList.IsValidIndex(m_iNextCompletion))
		return;

	// match found, set text
	char completedText[256];
	CompletionItem *item = &m_CompletionList[m_iNextCompletion];
	Assert( item );

	if ( item->cmd.cmd && !item->cmd.cmd->IsCommand() )
	{
		strncpy(completedText, item->GetCommand(), sizeof(completedText) - 2 );
	}
	else
	{
		strncpy(completedText, item->GetItemText(), sizeof(completedText) - 2 );
	}
	if ( !Q_strstr( completedText, " " ) )
	{
		strcat(completedText, " ");
	}

	m_pEntry->SetText(completedText);
	m_pEntry->SelectNone();
	m_pEntry->GotoTextEnd();

	m_iNextCompletion++;
}

//-----------------------------------------------------------------------------
// Purpose: Called whenever the user types text
//-----------------------------------------------------------------------------
void CGameConsoleDialog::OnTextChanged(Panel *panel)
{
	if (panel != m_pEntry)
		return;

	Q_strncpy( m_szPreviousPartialText, m_szPartialText, sizeof( m_szPreviousPartialText ) );

	// get the partial text the user type
	m_pEntry->GetText(m_szPartialText, sizeof(m_szPartialText));

	// see if they've hit the tilde key (which opens & closes the console)
	int len = strlen(m_szPartialText);
	if (len > 0 && (m_szPartialText[len - 1] == '~' || m_szPartialText[len - 1] == '`'))
	{
		// clear the text entry
		m_pEntry->SetText("");

		// close the console
		PostMessage(this, new KeyValues("Close"));
		if(baseuifuncs)
		{
			baseuifuncs->HideGameUI();
		}
		
		return;
	}

	if (!m_bAutoCompleteMode)
	{
		int oldlen = strlen( m_szPreviousPartialText );

		// check to see if the user just typed a space, so we can auto complete
		if (len > 0 && 
			oldlen == ( len - 1 ) &&
			m_szPartialText[len - 1] == ' ')
		{
			m_iNextCompletion = 0;
			OnAutoComplete(false);
		}
	}
	// clear auto-complete state since the user has typed
	m_bAutoCompleteMode = false;

	RebuildCompletionList(m_szPartialText);

	// build the menu
	if (/*len < 1 ||*/ m_CompletionList.Size() < 1)
	{
		m_pCompletionList->SetVisible(false);
	}
	else
	{
		m_pCompletionList->SetVisible(true);
		m_pCompletionList->DeleteAllItems();
		const int MAX_MENU_ITEMS = 7;

		// add the first ten items to the list
		for (int i = 0; i < m_CompletionList.Size() && i < MAX_MENU_ITEMS; i++)
		{
			char text[256];
			text[0] = 0;
			if (i == MAX_MENU_ITEMS - 1)
			{
				strcpy(text, "...");
			}
			else
			{
				strcpy(text, m_CompletionList[i].GetItemText() );
			}
			text[sizeof(text) - 1] = 0;
			KeyValues *kv = new KeyValues("CompletionCommand");
			kv->SetString("command",text);
			int index = m_pCompletionList->AddMenuItem(text, kv, this);
		}

		UpdateCompletionListPosition();
	}
	
	RequestFocus();
	m_pEntry->RequestFocus();

}

//-----------------------------------------------------------------------------
// Purpose: generic vgui command handler
//-----------------------------------------------------------------------------
void CGameConsoleDialog::OnCommand(const char *command)
{
	if (!stricmp(command, "Submit"))
	{
		// submit the entry as a console commmand
		char szCommand[256];
		m_pEntry->GetText(szCommand, sizeof(szCommand));
		engine->ClientCmd(szCommand);

		// add to the history
		Print("] ");
		Print(szCommand);
		Print("\n");

		// clear the field
		m_pEntry->SetText("");

		// clear the completion state
		OnTextChanged(m_pEntry);

		// always go the end of the buffer when the user has typed something
		m_pHistory->GotoTextEnd();
		// Add the command to the history
		char *extra = strchr(szCommand, ' ');
		if ( extra )
		{
			*extra = '\0';
			extra++;
		}

		if ( Q_strlen( szCommand ) > 0 )
		{
			AddToHistory( szCommand, extra );
		}
		m_pCompletionList->SetVisible(false);

	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: swallows tab key pressed
//-----------------------------------------------------------------------------
void CGameConsoleDialog::OnKeyCodeTyped(KeyCode code)
{
	BaseClass::OnKeyCodeTyped(code);

	// check for processing
	if (input()->GetFocus() == m_pEntry->GetVPanel())
	{
		if (code == KEY_TAB)
		{
			bool reverse = false;
			if (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT))
			{
				reverse = true;
			}

			// attempt auto-completion
			OnAutoComplete(reverse);
			m_pEntry->RequestFocus();
		}
		else if (code == KEY_DOWN)
		{
			OnAutoComplete(false);
		//	UpdateCompletionListPosition();
		//	m_pCompletionList->SetVisible(true);

			m_pEntry->RequestFocus();
		}
		else if (code == KEY_UP)
		{
			OnAutoComplete(true);
			m_pEntry->RequestFocus();
		}
			// HACK: Allow F key bindings to operate even here
		else if ( code >= KEY_F1 && code <= KEY_F12 )
		{
			// See if there is a binding for the FKey
			int translated = ( code - KEY_F1 ) + K_F1;
			const char *binding = gameuifuncs->Key_BindingForKey( translated );
			if ( binding && binding[0] )
			{
				// submit the entry as a console commmand
				char szCommand[256];
				Q_strncpy( szCommand, binding, sizeof( szCommand ) );
				engine->ClientCmd( szCommand );
			}
		}
	}

}


//-----------------------------------------------------------------------------
// Purpose: lays out controls
//-----------------------------------------------------------------------------
void CGameConsoleDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	// setup tab ordering
	GetFocusNavGroup().SetDefaultButton(m_pSubmit);

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	m_pEntry->SetBorder(pScheme->GetBorder("DepressedButtonBorder"));
	m_pHistory->SetBorder(pScheme->GetBorder("DepressedButtonBorder"));

	// layout controls
	int wide, tall;
	GetSize(wide, tall);

	const int inset = 8;
	const int entryHeight = 24;
	const int topHeight = 28;
	const int entryInset = 4;
	const int submitWide = 64;
	const int submitInset = 7; // x inset to pull the submit button away from the frame grab

	m_pHistory->SetPos(inset, inset + topHeight); 
	m_pHistory->SetSize(wide - (inset * 2), tall - (entryInset * 2 + inset * 2 + topHeight + entryHeight));
	m_pHistory->InvalidateLayout();

	m_pEntry->SetPos(inset, tall - (entryInset * 2 + entryHeight));
	m_pEntry->SetSize(wide - (inset * 3 + submitWide + submitInset), entryHeight);

	m_pSubmit->SetPos(wide - (inset + submitWide + submitInset ), tall - (entryInset * 2 + entryHeight));
	m_pSubmit->SetSize(submitWide, entryHeight);

	UpdateCompletionListPosition();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the position of the completion list popup
//-----------------------------------------------------------------------------
void CGameConsoleDialog::UpdateCompletionListPosition()
{
	int ex, ey, x, y;
	GetPos(x, y);
	m_pEntry->GetPos(ex, ey);

	m_pCompletionList->SetPos(x + ex, y + ey + 32);
	m_pEntry->RequestFocus();
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: Closes the completion list
//-----------------------------------------------------------------------------
void CGameConsoleDialog::CloseCompletionList()
{
	m_pCompletionList->SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: sets up colors
//-----------------------------------------------------------------------------
void CGameConsoleDialog::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_PrintColor = GetFgColor();
	m_DPrintColor = GetSchemeColor("BrightControlText", pScheme);
	m_pHistory->SetFont( pScheme->GetFont( "ConsoleText", IsProportional() ) );
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Handles autocompletion menu input
//-----------------------------------------------------------------------------
void CGameConsoleDialog::OnMenuItemSelected(const char *command)
{
	m_pEntry->SetText(command);
	m_pEntry->GotoTextEnd();
	m_pEntry->InsertChar(' ');
	m_pEntry->GotoTextEnd();

}

void CGameConsoleDialog::Hide()
{
	BaseClass::Close();
	m_iNextCompletion = 0;
}

void CGameConsoleDialog::AddToHistory( const char *commandText, const char *extraText )
{
	// Newest at end, oldest at head
	while ( m_CommandHistory.Count() >= MAX_HISTORY_ITEMS )
	{
		// Remove from head until size is reasonable
		m_CommandHistory.Remove( 0 );
	}

	// strip the space off the end of the command before adding it to the history
	char *command = static_cast<char *>( _alloca( (strlen( commandText ) + 1 ) * sizeof( char ) ));
	if ( command )
	{
		memset( command, 0x0, strlen( commandText ) + 1 );
		strncpy( command, commandText, strlen( commandText ));
		if ( command[ strlen( command ) -1 ] == ' ' )
		{
			 command[ strlen( command ) -1 ] = '\0';
		}
	}

	// strip the quotes off the extra text
	char *extra = NULL;

	if ( extraText )
	{
		extra = static_cast<char *>( malloc( (strlen( extraText ) + 1 ) * sizeof( char ) ));
		if ( extra )
		{
			memset( extra, 0x0, strlen( extraText ) + 1 );
			strncpy( extra, extraText, strlen( extraText )); // +1 to dodge the starting quote
			
			// Strip trailing spaces
			int i = strlen( extra ) - 1; 
			while ( i >= 0 &&  // Check I before referencing i == -1 into the extra array!
				extra[ i ] == ' ' )
			{
				extra[ i ] = '\0';
				i--;
			}
		}
	}

	// If it's already there, then remove since we'll add it to the end instead
	CHistoryItem *item = NULL;
	for ( int i = m_CommandHistory.Count() - 1; i >= 0; i-- )
	{
		item = &m_CommandHistory[ i ];
		if ( !item )
			continue;

		if ( stricmp( item->GetText(), command ) )
			continue;

		if ( extra || item->GetExtra() )
		{
			if ( !extra || !item->GetExtra() )
				continue;

			// stricmp so two commands with the same starting text get added
			if ( stricmp( item->GetExtra(), extra ) )	
				continue;
		}
		m_CommandHistory.Remove( i );
	}

	item = &m_CommandHistory[ m_CommandHistory.AddToTail() ];
	Assert( item );
	item->SetText( command, extra );

	m_iNextCompletion = 0;
	RebuildCompletionList( m_szPartialText );

	free( extra );
}


//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
MessageMapItem_t CGameConsoleDialog::m_MessageMap[] =
{
	MAP_MESSAGE( CGameConsoleDialog, "Activate", Activate ),
	MAP_MESSAGE( CGameConsoleDialog, "CloseCompletionList", CloseCompletionList ),
	MAP_MESSAGE_PTR( CGameConsoleDialog, "TextChanged", OnTextChanged, "panel" ),
	MAP_MESSAGE_CONSTCHARPTR( CGameConsoleDialog, "CompletionCommand", OnMenuItemSelected, "command" ),
};
IMPLEMENT_PANELMAP( CGameConsoleDialog, vgui::Frame );
