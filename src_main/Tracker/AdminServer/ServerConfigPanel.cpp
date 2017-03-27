//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>

#include "ServerConfigPanel.h"


#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_ISurface.h>
#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>
#include <VGUI_TextEntry.h>
#include <VGUI_Button.h>
#include <VGUI_ToggleButton.h>
#include <VGUI_RadioButton.h>
#include <VGUI_ListPanel.h>
#include <VGUI_ComboBox.h>
#include <VGUI_CheckButton.h>
#include <VGUI_PHandle.h>
#include <VGUI_PropertySheet.h>
#include "filesystem.h"

extern void v_strncpy(char *dest, const char *src, int bufsize);


using namespace vgui;
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CServerConfigPanel::CServerConfigPanel(vgui::Panel *parent, const char *name, const char *mod) : PropertyPage(parent, name)
{
	SetBounds(0, 0, 500, 170);
	AddActionSignalTarget(parent); // so we can send it a refresh

	m_pRcon=NULL;
	m_Rules = NULL;
	m_sHostName[0]= 0;

	m_pSendConfigButton = new Button(this, "SendConfig", "&Send");
	m_pSendConfigButton->SetCommand(new KeyValues("SendConfig"));


	// load the mod's config page, else fall back to the default
	char configName[200];
	_snprintf(configName,200,"Admin\\ServerConfigPanel_%s.res",mod);

	if( filesystem()->FileExists(configName) )
	{
		LoadControlSettings(configName);	
	} 
	else
	{
		LoadControlSettings("Admin\\ServerConfigPanel.res");	
	}

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CServerConfigPanel::~CServerConfigPanel()
{

}

//-----------------------------------------------------------------------------
// Purpose: Activates the page
//-----------------------------------------------------------------------------
void CServerConfigPanel::OnPageShow()
{
	// update the hostname field

	Panel *hostnamePanel=FindChildByName("hostname"); // find the hostname panel
	TextEntry *hostNameEntry = dynamic_cast<TextEntry *>(hostnamePanel);
	if (hostNameEntry)
	{
		hostNameEntry->SetText(m_sHostName);
	}

	// update all the config items with rules values from the server	
	if(m_Rules )
	{
		for (int i = 0; i < m_Rules->Count(); i++)
		{
			const char *rulename=(*m_Rules)[i]->GetString("cvar");
			const char *value=(*m_Rules)[i]->GetString("value");
			Panel *curPanel;

			for(int j=0;j< GetChildCount();j++)
			{
				curPanel = GetChild(j);
				const char *name = curPanel->GetName();

				if(!stricmp(name,rulename)) // this control name matches a rule name
				{ 
					break;
				}

			}

			if(j!=GetChildCount() )
			{
				const char *className= curPanel->GetClassName();

				if(!stricmp(className,"TextEntry"))
				{
					TextEntry *entry = dynamic_cast<TextEntry *>(curPanel);
					if (entry)
					{
						if(!stricmp(rulename,"sv_password") && !stricmp(value,"0"))
							// sv_password is a special case..
						{
							entry->SetText("");
						} 
						else
						{
							entry->SetText(value);
						}
					}
				} // if TextEntry

				if(!stricmp(className,"CheckButton"))
				{
					CheckButton *entry = dynamic_cast<CheckButton *>(curPanel);
					if (entry)
					{
						if(!stricmp(value,"0"))
						{
							entry->SetSelected(false);
						} 
						else
						{
							entry->SetSelected(true);
						}
					}
				} // if Checkbutton
			} // if was found
		} // for all the rules
	} // if m_Rules exists 
}

//-----------------------------------------------------------------------------
// Purpose: Hides the page
//-----------------------------------------------------------------------------
void CServerConfigPanel::OnPageHide()
{
}




//-----------------------------------------------------------------------------
// Purpose: Relayouts the data
//-----------------------------------------------------------------------------
void CServerConfigPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	OnPageShow();
	// setup the layout of the panels
//	m_pServerServerConfigPanel->SetBounds(5,5,GetWide()-5,GetTall()-35);

	//m_pEnterServerConfigPanel->SetBounds(5,GetTall()-25,GetWide()-80,20);
	m_pSendConfigButton->SetBounds(GetWide()-70,GetTall()-25,60,20);
}


//-----------------------------------------------------------------------------
// Purpose: passes the rcon class to use
//-----------------------------------------------------------------------------
void CServerConfigPanel::SetRcon(CRcon *rcon) 
{
	m_pRcon=rcon;
}


//-----------------------------------------------------------------------------
// Purpose: run when the send button is pressed, send a rcon "say" to the server
//-----------------------------------------------------------------------------
void CServerConfigPanel::OnSendConfig()
{
	if(m_pRcon)
	{
		char chat_text[512];
		char tmp_text[512];
	
		// step through every component and update its cvar based on its name
		for(int j=0;j< GetChildCount();j++)
		{
			Panel *curPanel = GetChild(j);
			const char *name = curPanel->GetName();
			const char *className= curPanel->GetClassName();

			tmp_text[0]=0;
		
			if(!stricmp(className,"TextEntry"))
			{
				TextEntry *entry = dynamic_cast<TextEntry *>(curPanel);
				if (entry)
				{
					if(!stricmp(name,"sv_password") ) // this is a special case to turn on passwords
					{
						entry->GetText(0,tmp_text,512);
						if(strlen(tmp_text) > 0) 
						{
							char tmp[512];
							strcpy(tmp,tmp_text);
							_snprintf(tmp_text,512,"sv_password \"%s\"",tmp);
						}
						else
						{
							strcpy(tmp_text,"sv_password \"\"");
						}
						m_pRcon->SendRcon(tmp_text);
						tmp_text[0]=0;
					} 
					else 
					{
						entry->GetText(0,tmp_text,512);
					}
				}
			} // if TextEntry

			if(!stricmp(className,"CheckButton"))
			{
				CheckButton *entry = dynamic_cast<CheckButton *>(curPanel);
				if (entry)
				{	
					if( entry->IsSelected()	)
					{
						strcpy(tmp_text,"1"); // enable this option
					} 
					else
					{
						strcpy(tmp_text,"0"); // disable this option
					}
				}
			}// if Checkbutton

		/* Rcon doesn't seem to like compound statements (i.e ones with ";" in them)...
			
		  if(strlen(tmp_text)>0 && strlen(chat_text)+strlen(name)+strlen(tmp_text)+4<512)
			// if this was a valid panel 
			{
				_snprintf(chat_text,512,"%s %s \"%s\";",chat_text,name,tmp_text);
			}
			else
			*/
			if (strlen(tmp_text)>0) 
			{
				const char *value=NULL;
				const char *rulename=NULL;
					for (int i = 0; i < m_Rules->Count(); i++)
					{
						rulename=(*m_Rules)[i]->GetString("cvar");
						value=(*m_Rules)[i]->GetString("value");
						
						if(rulename && !strnicmp(rulename,name,strlen(rulename))) 
						{
							break;
						}
					}
					if(i!=m_Rules->Count() && value && strnicmp(tmp_text,value,strlen(value))) 
					// if we found the rule, its value, and it differs
					{
						_snprintf(chat_text,512,"%s \"%s\";",name,tmp_text); 
						m_pRcon->SendRcon(chat_text); // send what we have
					}
			}
			
		} // for all panel
	
		PostActionSignal(new KeyValues("Refresh"));
	}
}


void CServerConfigPanel::SetRules(CUtlVector<vgui::KeyValues *> *rules)
{
	m_Rules=rules;
}

void CServerConfigPanel::SetHostName(const char *name)
{
	v_strncpy(m_sHostName,name,512);
}


//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CServerConfigPanel::m_MessageMap[] =
{
	MAP_MESSAGE( CServerConfigPanel, "SendConfig", OnSendConfig ),
//	MAP_MESSAGE_PTR_CONSTCHARPTR( CServerConfigPanel, "TextChanged", OnTextChanged, "panel", "text" ),
};

IMPLEMENT_PANELMAP( CServerConfigPanel, Frame );