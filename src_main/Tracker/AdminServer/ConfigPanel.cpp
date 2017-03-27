//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "ConfigPanel.h"
#include "Info.h"
#include "serverpage.h"

#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_ISurface.h>
#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>
#include <VGUI_TextEntry.h>
#include <VGUI_Button.h>
#include <VGUI_ToggleButton.h>
#include <VGUI_CheckButton.h>
#include <VGUI_MessageBox.h>
#include <VGUI_RadioButton.h>

#include <stdio.h>

using namespace vgui;

static const long RETRY_TIME = 10000;		// refresh server every 10 seconds

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CConfigPanel::CConfigPanel(bool autorefresh,bool savercon,int refreshtime,
							bool graphs, int graphsrefreshtime,bool getlogs) : Frame(NULL, "ConfigPanel")
{
	SetMinimumSize(320,320);
	MakePopup();

	m_pOkayButton = new Button(this, "Okay", "&Okay");
	m_pCloseButton = new Button(this, "Close", "&Close");

	m_pRefreshCheckButton = new CheckButton(this, "RefreshCheckButton", "");

	m_pRconCheckButton = new CheckButton(this, "RconCheckButton", "");

	m_pRefreshTextEntry= new TextEntry(this,"RefreshTextEntry");

	m_pGraphsButton = new CheckButton(this, "GraphsButton", "");
	m_pGraphsRefreshTimeTextEntry= new TextEntry(this,"GraphsRefreshTimeTextEntry");

	m_pLogsButton = new CheckButton(this, "LogsButton", "");

	SetTitle("Admin Configuration",true);

	LoadControlSettings("Admin\\ConfigPanel.res");

	m_pRefreshCheckButton->SetSelected(autorefresh);
	m_pRconCheckButton->SetSelected(savercon);
	m_pGraphsButton->SetSelected(graphs);
	m_pLogsButton->SetSelected(getlogs);

	m_pRefreshTextEntry->SetEnabled(m_pRefreshCheckButton->IsSelected());
	m_pRefreshTextEntry->SetEditable(m_pRefreshCheckButton->IsSelected());
	m_pGraphsRefreshTimeTextEntry->SetEnabled(m_pGraphsButton->IsSelected());
	m_pGraphsRefreshTimeTextEntry->SetEditable(m_pGraphsButton->IsSelected());

	char refreshText[20];
	_snprintf(refreshText,20,"%i",refreshtime);

	m_pRefreshTextEntry->SetText(refreshText);
	
	_snprintf(refreshText,20,"%i",graphsrefreshtime);

	m_pGraphsRefreshTimeTextEntry->SetText(refreshText);

	SetVisible(true);

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CConfigPanel::~CConfigPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: Activates the dialog
//-----------------------------------------------------------------------------
void CConfigPanel::Run()
{
	RequestFocus();
}


//-----------------------------------------------------------------------------
// Purpose: Deletes the dialog when it's closed
//-----------------------------------------------------------------------------
void CConfigPanel::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: turn on and off components when check boxes are checked
//-----------------------------------------------------------------------------
void CConfigPanel::OnButtonToggled(Panel *panel)
{
	if (panel == m_pRefreshCheckButton) 
		// you can only edit the refresh time if you allow auto refresh
	{
		m_pRefreshTextEntry->SetEnabled(m_pRefreshCheckButton->IsSelected());
		m_pRefreshTextEntry->SetEditable(m_pRefreshCheckButton->IsSelected());
	}
	else if (panel == m_pGraphsButton) 
		// you can only edit the refresh time if you allow auto refresh
	{
		m_pGraphsRefreshTimeTextEntry->SetEnabled(m_pGraphsButton->IsSelected());
		m_pGraphsRefreshTimeTextEntry->SetEditable(m_pGraphsButton->IsSelected());
	}


	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CConfigPanel::SetControlText(const char *textEntryName, const char *text)
{
	TextEntry *entry = dynamic_cast<TextEntry *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}



//-----------------------------------------------------------------------------
// Purpose: Parse posted messages
//			 
//-----------------------------------------------------------------------------
void CConfigPanel::OnCommand(const char *command)
{

	if(!stricmp(command,"okay"))
	{ // save away the new settings
		char timeText[20];
		int time,timeGraphs;

		m_pRefreshTextEntry->GetText(0,timeText,20);
		sscanf(timeText,"%i",&time);
	
		memset(timeText,0x0,sizeof(timeText));
		m_pGraphsRefreshTimeTextEntry->GetText(0,timeText,20);
		sscanf(timeText,"%i",&timeGraphs);


		if(time>0 && time < 9999 && timeGraphs>0 && timeGraphs< 9999) 
		{

			CServerPage::GetInstance()->SetConfig(m_pRefreshCheckButton->IsSelected(),
													m_pRconCheckButton->IsSelected(),
													time,
													m_pGraphsButton->IsSelected(),
													timeGraphs,
													m_pLogsButton->IsSelected());
			OnClose();

		}
		else
		{
			MessageBox *dlg = new MessageBox ("Config", "Time value is out of range. (0<x<9999)");
			dlg->DoModal();
		}
	}
	else if(!stricmp(command,"close") )
	{
		OnClose();
	}

}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CConfigPanel::m_MessageMap[] =
{
	MAP_MESSAGE_PTR( CConfigPanel, "ButtonToggled", OnButtonToggled, "panel" ),
	MAP_MESSAGE_PTR( CConfigPanel, "RadioButtonChecked", OnButtonToggled, "panel" ),
};

IMPLEMENT_PANELMAP( CConfigPanel, Frame );
