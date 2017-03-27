//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef GAMECONSOLEDIALOG_H
#define GAMECONSOLEDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "TaskFrame.h"
#include <Color.h>
#include "UtlVector.h"
#include "EngineInterface.h"
//#include "cvardef.h"

// Things the user typed in and hit submit/return with
class CHistoryItem
{
public:
	CHistoryItem( void );
	CHistoryItem( const char *text, const char *extra = NULL );
	CHistoryItem( const CHistoryItem& src );
	~CHistoryItem( void );

	const char *GetText() const;
	const char *GetExtra() const;
	void SetText( const char *text, const char *extra );
	bool HasExtra() { return m_bHasExtra; }
private:

	char		*m_text;
	char		*m_extraText;
	bool		m_bHasExtra;
};

//-----------------------------------------------------------------------------
// Purpose: Game/dev console dialog
//-----------------------------------------------------------------------------
class CGameConsoleDialog : public CTaskFrame
{
public:
	CGameConsoleDialog();
	~CGameConsoleDialog();

	// brings dialog to the fore
	void Activate();

	// normal text print
	void Print(const char *msg);

	// debug text print
	void DPrint(const char *msg);

	// debug text print
	void ColorPrintf(Color& clr, const char *msg);
	
	// clears the console
	void Clear();

	void Hide();

private:
	enum
	{
		MAX_HISTORY_ITEMS = 100,
	};

	// methods
	void OnAutoComplete(bool reverse);
	void OnTextChanged(vgui::Panel *panel);
	void RebuildCompletionList(const char *partialText);
	void UpdateCompletionListPosition();
	void CloseCompletionList();
	void OnMenuItemSelected(const char *command);
	void AddToHistory( const char *commandText, const char *extraText );

	// vgui overrides
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnCommand(const char *command);
	virtual void OnKeyCodeTyped(enum vgui::KeyCode code);

	DECLARE_PANELMAP();

	vgui::RichText *m_pHistory;
	vgui::TextEntry *m_pEntry;
	vgui::Button *m_pSubmit;
	vgui::Menu *m_pCompletionList;
	Color m_PrintColor;
	Color m_DPrintColor;

	bool m_bAutoCompleteMode;	// true if the user is currently tabbing through completion options
	int m_iNextCompletion;		// the completion that we'll next go to
	char m_szPartialText[256];
	char m_szPreviousPartialText[256];

	struct cmdnode_t
	{
		// pointer to commands (only one will be non-null)
		//unsigned int cmd;
	//	struct cvar_s *cvar;
		ConCommandBase	*cmd;
	};

	class CompletionItem
	{
	public:
		CompletionItem( void )
		{
			iscommand = true;
		//	cmd.cmd = 0;
//			cmd.cvar = NULL;
			cmd.cmd = NULL;

			m_text = NULL;
		}

		CompletionItem( const CompletionItem& src )
		{
			iscommand = src.iscommand;
			cmd = src.cmd;
			if ( src.m_text )
			{
				m_text = new CHistoryItem( (const CHistoryItem& )src.m_text );
			}
			else
			{
				m_text = NULL;
			}
		}
		
		CompletionItem& CompletionItem::operator =( const CompletionItem& src )
		{
			if ( this == &src )
				return *this;

			iscommand = src.iscommand;
			cmd = src.cmd;
			if ( src.m_text )
			{
				m_text = new CHistoryItem( (const CHistoryItem& )*src.m_text );
			}
			else
			{
				m_text = NULL;
			}

			return *this;
		}

		~CompletionItem( void )
		{
			delete m_text;
		}

		char const *GetItemText( void )
		{
			static char text[256];
			text[0] = 0;
			if ( m_text )
			{
				if ( m_text->HasExtra() )
				{
					_snprintf( text, sizeof( text ), "%s %s", m_text->GetText(), m_text->GetExtra() );
				}
				else
				{
					strcpy( text, m_text->GetText() );
				}
			}
			return text;
		}	

		const char *GetCommand( void )
		{
			static char text[256];
			text[0] = 0;
			if ( m_text )
			{
				strcpy( text, m_text->GetText() );
			}
			return text;
		}

		bool			iscommand;
		cmdnode_t cmd;
		CHistoryItem	*m_text;
	};


	CUtlVector<CompletionItem> m_CompletionList;
	CUtlVector<CHistoryItem>	m_CommandHistory;

	typedef CTaskFrame BaseClass;
};


#endif // GAMECONSOLEDIALOG_H
