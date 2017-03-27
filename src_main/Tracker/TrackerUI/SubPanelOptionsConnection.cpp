//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SubPanelOptionsConnection.h"

#include <VGUI_ComboBox.h>
#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>

#include <stdio.h>

using namespace vgui;

struct speeditem_t
{
	const char *description;
	int speed;
};

// list of all the different internet speeds
speeditem_t g_Speeds[] =
{
	{ "Modem - 14.4k", 1500 },
	{ "Modem - 28.8k", 2500 },
	{ "Modem - 33.6k", 3000 },
	{ "Modem - 56k", 3500 },
	{ "ISDN - 112k", 5000 },
	{ "DSL > 256k", 7500 },
	{ "LAN/T1 > 1M", 9999 },

};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSubPanelOptionsConnection::CSubPanelOptionsConnection()
{
	m_pInternetSpeed = new ComboBox(this, "InternetSpeed", ARRAYSIZE(g_Speeds), false);

	for (int i = 0; i < ARRAYSIZE(g_Speeds); i++)
	{
		m_pInternetSpeed->AddItem(g_Speeds[i].description);
	}

	LoadControlSettings("Friends/SubPanelOptionsConnection.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSubPanelOptionsConnection::~CSubPanelOptionsConnection()
{
}

//-----------------------------------------------------------------------------
// Purpose: Loads data from doc
//-----------------------------------------------------------------------------
void CSubPanelOptionsConnection::OnResetData()
{
	// get our internet speed from the registry
	char speedText[32];
	if (!system()->GetRegistryString("HKEY_CURRENT_USER\\Software\\Valve\\Tracker\\Rate", speedText, sizeof(speedText)-1))
	{
		strcpy(speedText, "7500");
	}
	int speed = atoi(speedText);

	// find the speed in the list and set the combo text to be that
	for (int i = 0; i < ARRAYSIZE(g_Speeds); i++)
	{
		if (speed <= g_Speeds[i].speed)
		{
			m_pInternetSpeed->SetText(g_Speeds[i].description);
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Writes data to doc
//-----------------------------------------------------------------------------
void CSubPanelOptionsConnection::OnApplyChanges()
{
	int speed = 7500;
	// get the speed from the list and set it in the registry
	char text[256];
	m_pInternetSpeed->GetText(0, text, sizeof(text)-1);
	for (int i = 0; i < ARRAYSIZE(g_Speeds); i++)
	{
		if (!stricmp(text, g_Speeds[i].description))
		{
			speed = g_Speeds[i].speed;
			break;
		}
	}

	char speedText[32];
	sprintf(speedText, "%d", speed);
	system()->SetRegistryString("HKEY_CURRENT_USER\\Software\\Valve\\Tracker\\Rate", speedText);
}


