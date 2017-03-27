//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUI_SCOREPANEL_H
#define VGUI_SCOREPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Button.h>
#include "shareddefs.h"
#include <ctype.h>
#include "vgui_grid.h"
#include <vgui_controls/ScrollBar.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_SCORES					10
#define MAX_SCOREBOARD_TEAMS		5

// Scoreboard cells
#define COLUMN_TRACKER	0
#define COLUMN_NAME		1
#define COLUMN_CLASS	2
#define COLUMN_KILLS	3
#define COLUMN_DEATHS	4
#define COLUMN_LATENCY	5
#define COLUMN_VOICE	6
#define COLUMN_BLANK	7
#define NUM_COLUMNS		8
#define NUM_ROWS		(MAX_PLAYERS + (MAX_SCOREBOARD_TEAMS * 2))

class BitmapImage;

using namespace vgui;

class CTextImage2 : public Image
{
public:
	CTextImage2( vgui::HScheme hScheme )
	{
		_image[0] = new TextImage("");
		_image[1] = new TextImage("");
	}

	~CTextImage2()
	{
		delete _image[0];
		delete _image[1];
	}

	TextImage *GetImage(int image)
	{
		return _image[image];
	}

	void GetSize(int &wide, int &tall)
	{
		int w1, w2, t1, t2;
		_image[0]->GetContentSize(w1, t1);
		_image[1]->GetContentSize(w2, t2);

		wide = w1 + w2;
		tall = max(t1, t2);
		SetSize(wide, tall);
	}

	void Paint()
	{
		_image[0]->Paint();
		_image[1]->Paint();
	}

	void SetPos(int x, int y)
	{
		_image[0]->SetPos(x, y);
		
		int swide, stall;
		_image[0]->GetSize(swide, stall);

		int wide, tall;
		_image[1]->GetSize(wide, tall);
		_image[1]->SetPos(x + wide, y + (stall * 0.9) - tall);
	}

	void SetColor(Color color)
	{
		_image[0]->SetColor(color);
	}

	void SetColor2(Color color)
	{
		_image[1]->SetColor(color);
	}

private:
	TextImage *_image[2];

};

//-----------------------------------------------------------------------------
// Purpose: Custom label for cells in the Scoreboard's Table Header
//-----------------------------------------------------------------------------
class CLabelHeader : public Label
{
	DECLARE_CLASS_GAMEROOT( CLabelHeader, Label );

public:
	CLabelHeader() : Label( (Panel *)NULL, "CLabelHeader", "" )
	{
		_dualImage = new CTextImage2( GetScheme() );
		_row = -2;
		_dualImage->SetColor2(Color(255, 170, 0, 0));
		_useFgColorAsImageColor = true;
		_offset[0] = 0;
		_offset[1] = 0;
		m_Align = Label::a_west;
	}

	~CLabelHeader()
	{
		delete _dualImage;
	}

	void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings(pScheme);

		_dualImage->SetColor2(Color(255, 170, 0, 0));
		SetBgColor( Color( 0, 0, 0, 0 ) );
	}

	void SetRow(int row)
	{
		_row = row;
	}

	void SetFgColorAsImageColor(bool state)
	{
		_useFgColorAsImageColor = state;
	}

	virtual void SetText(int textBufferLen, const char* text)
	{
		_dualImage->GetImage(0)->SetText(text);

		// calculate the text size
		HFont font = _dualImage->GetImage(0)->GetFont();
		_gap = 0;
		for (const char *ch = text; *ch != 0; ch++)
		{
			int a, b, c;
			vgui::surface()->GetCharABCwide( font, *ch, a, b, c);
			_gap += (a + b + c);
		}

		_gap += XRES(5);
	}

	virtual void SetText(const char* text)
	{
		// strip any non-alnum characters from the end
		char buf[512];
		strcpy(buf, text);

		int len = strlen(buf);
		while (len && isspace(buf[--len]))
		{
			buf[len] = 0;
		}

		CLabelHeader::SetText(0, buf);
	}

	void SetText2(const char *text)
	{
		_dualImage->GetImage(1)->SetText(text);
	}

	void GetTextSize(int &wide, int &tall)
	{
		_dualImage->GetSize(wide, tall);
	}

	void SetFgColor(int r,int g,int b,int a)
	{
		Label::SetFgColor(Color( r,g,b,a ) );
		Color color(r,g,b,a);
		_dualImage->SetColor(color);
		_dualImage->SetColor2(color);
		Repaint();
	}

	void SetFgColor(Color sc)
	{
		int r, g, b, a;
		Label::SetFgColor(sc);
		sc.GetColor( r, g, b, a );

		// Call the r,g,b,a version so it sets the color in the dualImage..
		SetFgColor( r, g, b, a );
	}

	void SetFont(HFont font)
	{
		_dualImage->GetImage(0)->SetFont(font);
	}

	void SetFont2(HFont font)
	{
		_dualImage->GetImage(1)->SetFont(font);
	}

	// this adjust the absolute position of the text after alignment is calculated
	void SetTextOffset(int x, int y)
	{
		_offset[0] = x;
		_offset[1] = y;
	}

	void Paint();
	void PaintBackground();
	void CalcAlignment(int iwide, int itall, int &x, int &y);

	virtual void SetContentAlignment(Alignment alignment);

private:
	CTextImage2 *_dualImage;
	int _row;
	int _gap;
	int _offset[2];
	bool _useFgColorAsImageColor;
	Alignment m_Align;
};

class ScoreTablePanel;


class CPlayerListPanel : public PanelListPanel
{
	DECLARE_CLASS_GAMEROOT( CPlayerListPanel, PanelListPanel );
public:

	CPlayerListPanel( Panel *parent, const char *panelName )
		: BaseClass( parent, panelName )
	{
	}
};


#include "tier0/memdbgoff.h"


//-----------------------------------------------------------------------------
// Purpose: Scoreboard back panel
//-----------------------------------------------------------------------------
class ScorePanel : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( ScorePanel, vgui::Panel );
public:

	ScorePanel( const char *pElementName );
	~ScorePanel();

	DECLARE_MULTIPLY_INHERITED();

	// CHudBase overrides
	virtual void Init( void );
	virtual void OnThink(void);
	virtual bool ShouldDraw( void );
	
	// Command handlers
	void UserCmd_ShowScores( void );
	void UserCmd_HideScores( void );

	void DeathMsg( int killer, int victim );

	void Initialize( void );

	void Open( void );
	void Close( void );

	void MouseOverCell(int row, int col);

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Paint( void );

	virtual void SetVisible( bool state );

	virtual void OnCommand( const char *command );

// InputSignal overrides.
public:

//	virtual void OnMousePressed(MouseCode code);
//	virtual void CursorMoved(int x, int y, Panel *panel);

	int				m_iShowscoresHeld;

private:
	void FillGrid();
	void SortTeams( void );
	void SortPlayers( int iTeam, char *team );
	void RebuildTeams( void );

	Label			m_TitleLabel;
	
	// Here is how these controls are arranged hierarchically.
	// m_HeaderGrid
	//     m_HeaderLabels

	// m_PlayerGridScroll
	//     m_PlayerGrid
	//         m_PlayerEntries 

	CGrid			m_HeaderGrid;
	CLabelHeader	m_HeaderLabels[NUM_COLUMNS];			// Labels above the 
	CLabelHeader	*m_pCurrentHighlightLabel;
	int				m_iHighlightRow;
	
	CPlayerListPanel m_PlayerList;
	CGrid			*m_PlayerGrids[NUM_ROWS];				// The grid with player and team info. 
	CLabelHeader	*m_PlayerEntries[NUM_COLUMNS][NUM_ROWS];	// Labels for the grid entries.

	Button				*m_pCloseButton;
	CLabelHeader*	GetPlayerEntry(int x, int y)	{return m_PlayerEntries[x][y];}

	BitmapImage		*m_pTrackerIcon;

	vgui::HFont		m_hsfont;
	vgui::HFont		m_htfont;
	vgui::HFont		m_hsmallfont;
	
	int				m_iNumTeams;
	int				m_iPlayerNum;

	int				m_iRows;
	int				m_iSortedRows[NUM_ROWS];
	int				m_iIsATeam[NUM_ROWS];
	bool			m_bHasBeenSorted[MAX_PLAYERS];
	int				m_iLastKilledBy;
	int				m_fLastKillTime;


	friend CLabelHeader;
};

#endif // VGUI_SCOREPANEL_H
