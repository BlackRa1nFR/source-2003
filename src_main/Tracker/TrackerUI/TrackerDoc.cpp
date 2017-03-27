//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Maintains the tracker document
//=============================================================================

#include "../TrackerNET/TrackerNET_Interface.h"
#include "Buddy.h"
#include "OnlineStatus.h"
#include "ServerSession.h"
#include "Tracker.h"
#include "TrackerDoc.h"
#include "IceKey.h"
#include "Random.h"

#include <stdio.h>
#include <stdlib.h>

#include <VGUI_Controls.h>
#include <VGUI_ISurface.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Panel.h>
#include "FileSystem.h"


//-----------------------------------------------------------------------------
/*		Tracker/AppData
	AppData contains all the information that is specific to the tracker
	application, and contains no user specific information (which is in Tracker/UserData)
	AppData is the first set of data which get loaded.

	AppData/UID		  - the userid of the last user to use the app
	AppData/Password  - the current active password
						if no password saved then the user can't auto log on

*/
//-----------------------------------------------------------------------------

// encryptor
IceKey g_Cipher(2);	/* high level encryption */
unsigned char g_EncryptionKey[16] = { 63, 12, 129, 32, 173, 194, 222, 1, 77, 16, 24, 99, 123, 132, 197, 201 };

static const int MAX_PASSWORD_LEN = 64;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTrackerDoc::CTrackerDoc()
{
	g_Cipher.set(g_EncryptionKey);
	m_BuddyMap.SetLessFunc(BuddyMapItemLessFunc);

	// create the base registry
	m_pDataRegistry = new vgui::KeyValues("Tracker");

	// create the sub registries

	// m_pAppData contains all the application-specific data
	m_pAppData = m_pDataRegistry->FindKey("App", true);

	// m_pUserData contains all the user specific data
	// this will be empty until LoadUserData() is called (after the user logs in)
	m_pUserData = m_pDataRegistry->FindKey("User", true);
	m_pBuddyData = m_pDataRegistry->FindKey("User/Buddies", true);

	// load the app file
	LoadAppData();

	// user data is not loaded UNTIL after a successful login
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTrackerDoc::~CTrackerDoc()
{
	// clear the CBuddy's
	for (int i = 0; i < m_BuddyMap.MaxElement(); i++)
	{
		if (m_BuddyMap.IsValidIndex(i))
		{
			delete m_BuddyMap[i].pBuddy;
		}
	}

	// delete main data registry
	if (m_pDataRegistry)
	{
		m_pDataRegistry->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the userID of the current user, 0 if none
//-----------------------------------------------------------------------------
unsigned int CTrackerDoc::GetUserID()
{
	return Data()->GetInt("App/UID", 0);
}

//-----------------------------------------------------------------------------
// Purpose: returns the user name of the current user
//-----------------------------------------------------------------------------
const char *CTrackerDoc::GetUserName()
{
	return Data()->GetString("User/UserName");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBuddy *CTrackerDoc::GetBuddy(unsigned int buddyID)
{
	// check in the buddy map first
	BuddyMapItem_t searchItem = { buddyID, NULL };
	int buddyIndex = m_BuddyMap.Find(searchItem);
	if (m_BuddyMap.IsValidIndex(buddyIndex))
	{
		return m_BuddyMap[buddyIndex].pBuddy;
	}

	// if it's not in the map, try the raw data
	for (KeyValues *dat = m_pBuddyData->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
	{
		if ((unsigned int)atoi(dat->GetName()) == buddyID)
		{
			// found it, insert it into cache list
			BuddyMapItem_t newItem = { buddyID, new CBuddy(buddyID, dat) };
			buddyIndex = m_BuddyMap.Insert(newItem);
			return m_BuddyMap[buddyIndex].pBuddy;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new buddy to the doc
// Input  : *buddyData - 
//-----------------------------------------------------------------------------
void CTrackerDoc::AddNewBuddy(KeyValues *newBuddy)
{
	int buddyID = newBuddy->GetInt("uid");
	if (buddyID)
	{
		// make sure the buddy doesn't already exist
		// add in their details
		char buf[64];
		sprintf(buf, "%d", buddyID);
		KeyValues *newKey = m_pBuddyData->FindKey(buf, true);

		for (KeyValues *dat = newBuddy->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
		{
			newKey->SetString(dat->GetName(), dat->GetString());
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: moves all the buddies offline
//-----------------------------------------------------------------------------
void CTrackerDoc::ClearBuddyStatus()
{
	KeyValues *buddyData = GetDoc()->Data()->FindKey("User/Buddies", true);
	for (KeyValues *dat = buddyData->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
	{
		// doesn't affect any of the negative offline states
		if (dat->GetInt("Status") > COnlineStatus::RECENTLYONLINE)
		{
			dat->SetInt("Status", COnlineStatus::OFFLINE);
		}

		dat->SetString("Game", "");
		dat->SetInt("GameIP", 0);
		dat->SetInt("GamePort", 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the status of a single user
//-----------------------------------------------------------------------------
void CTrackerDoc::UpdateBuddyStatus(unsigned int buddyID, int status, unsigned int sessionID, unsigned int serverID, unsigned int *IP, unsigned int *port, unsigned int *gameIP, unsigned int *gamePort, const char *gameType)
{
	CBuddy *buddy = GetBuddy(buddyID);
	if (buddy)
	{
		buddy->UpdateStatus(status, sessionID, serverID, IP, port, gameIP, gamePort, gameType);
	}
	else
	{
		// request information about this buddy from the server, as long as it's not us
		if (GetUserID() != buddyID)
		{
			ServerSession().RequestUserInfoFromServer(buddyID);
		}

		// store off their base details
		char buf[64];
		sprintf(buf, "%d", buddyID);
		KeyValues *bud = m_pBuddyData->FindKey(buf, true);

		bud->SetInt("Status", status);
		bud->SetInt("ServerID", serverID);
		if (sessionID)
		{
			bud->SetInt("SessionID", sessionID);
		}
		if (IP)
		{
			bud->SetInt("IP", *IP);
		}
		if (port)
		{
			bud->SetInt("port", *port);
		}
		if (gameIP)
		{
			bud->SetInt("GameIP", *gameIP);
		}
		if (gamePort)
		{
			bud->SetInt("GamePort", *gamePort);
		}
		if (gameType)
		{
			bud->SetString("Game", gameType);
		}

		// set as not authed by default, until we receive 
		// confirmation from the server that we are authorized to see this user
		bud->SetInt("NotAuthed", 1);
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buddyID - 
// Output : KeyValues
//-----------------------------------------------------------------------------
KeyValues *CTrackerDoc::GetFirstMessage(int buddyID)
{
	KeyValues *msgList = GetBuddy(buddyID)->Data()->FindKey("Messages", true);
	KeyValues *msg = msgList->GetFirstSubKey();
	if (!msg)
	{
		// no messages
		GetDoc()->GetBuddy(buddyID)->Data()->SetInt("MessageAvailable", 0);
		return NULL;
	}

	return msg;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *message - 
//-----------------------------------------------------------------------------
void CTrackerDoc::MoveMessageToHistory(int buddyID, KeyValues *msg)
{
	KeyValues *msgList = GetBuddy(buddyID)->Data()->FindKey("Messages", true);

	msgList->RemoveSubKey(msg);
	KeyValues *buddy = GetDoc()->GetBuddy(buddyID)->Data();

	/* MESSAGE HISTORY DISABLED
	KeyValues *msgHistory = buddy->FindKey("MessageHistory", true);
	KeyValues *newMsg = msgHistory->CreateNewKey();

	// copy all the data across
	for (KeyValues *kv = msg->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		newMsg->SetString(kv->GetName(), kv->GetString());
	}
	*/

	// check the message flag
	if (msgList->GetFirstSubKey() == NULL)
	{
		buddy->SetInt("MessageAvailable", 0);
	}

	if (msg)
	{
		msg->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Removes a buddy from use
//-----------------------------------------------------------------------------
void CTrackerDoc::RemoveBuddy(unsigned int buddyID)
{
	// deauthorize the user on the server
	ServerSession().SendAuthUserMessage(buddyID, 0);

	// set them as removed so they disappear from list
	// records will still be kept in database
	GetBuddy(buddyID)->Data()->SetInt("Removed", 1);
	GetBuddy(buddyID)->UpdateStatus(COnlineStatus::REMOVED, 0, 0, NULL, NULL, NULL, NULL, NULL);

	GetBuddy(buddyID)->ShutdownDialogs();
}

//-----------------------------------------------------------------------------
// Purpose: Returns number of buddies in list
//-----------------------------------------------------------------------------
int CTrackerDoc::GetNumberOfBuddies()
{
	return m_BuddyMap.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Used to walk buddy list.
//			index is in the range [0, GetNumberOfBuddies).  
//-----------------------------------------------------------------------------
int CTrackerDoc::GetBuddyByIndex(int index)
{
	// walk the list until we find the specified index
	int walkCount = 0;
	int foundIndex = m_BuddyMap.FirstInorder();
	while (walkCount++ < index)
	{
		foundIndex = m_BuddyMap.NextInorder(foundIndex);
	}

	if (!m_BuddyMap.IsValidIndex(foundIndex))
	{
		return 0;
	}
	return m_BuddyMap[foundIndex].buddyID;
}


//-----------------------------------------------------------------------------
// Purpose: Loads specific user data file, clearing out any old data first
//-----------------------------------------------------------------------------
bool CTrackerDoc::LoadUserData(unsigned int iUID)
{
	// clear the current user data
	m_pUserData->Clear();
	for (int i = 0; i < m_BuddyMap.MaxElement(); i++)
	{
		if (m_BuddyMap.IsValidIndex(i))
		{
			delete m_BuddyMap[i].pBuddy;
		}
	}
	m_BuddyMap.RemoveAll();

	// make up the path to look for the data file
	char szUserFile[128];
	sprintf( szUserFile, "UserData_%d.vdf", iUID );
	m_pUserData->LoadFromFile(vgui::filesystem(), szUserFile, "CONFIG");

	m_pUserData = m_pDataRegistry->FindKey("User", true);
	m_pBuddyData = m_pDataRegistry->FindKey("User/Buddies", true);

	// reset any buddy status
	GetDoc()->ClearBuddyStatus();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Saves the current user data
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTrackerDoc::SaveUserData( void )
{
	int uid = m_pAppData->GetInt("uid");
	if (!uid)
		return false;

	// make up the path to look for the data file
	char szUserFile[128];
	sprintf( szUserFile, "UserData_%d.vdf", uid );

	// write out the userdata registry into the file
	m_pUserData->SaveToFile(vgui::filesystem(), szUserFile, "CONFIG");

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Loads in application data
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTrackerDoc::LoadAppData( void )
{
	// clear the current app data
	m_pAppData->Clear();

	// load from file
	m_pAppData->LoadFromFile(vgui::filesystem(), "AppData.vdf", "CONFIG");

	// load in the password for auto-login
	FileHandle_t f = vgui::filesystem()->Open("masters.dat", "rb", "CONFIG");
	if (f)
	{
		char encryptedPassword[MAX_PASSWORD_LEN], password[MAX_PASSWORD_LEN];

		// load encrypted password from file
		vgui::filesystem()->Read(encryptedPassword, MAX_PASSWORD_LEN, f);
		vgui::filesystem()->Close(f);

		// decrypt 8 bytes at a time
		for (int i = 0; i < MAX_PASSWORD_LEN; i += 8)
		{
			g_Cipher.decrypt((unsigned char *)(encryptedPassword + i), (unsigned char *)(password + i));
		}

		// set the deciphered password in the doc
		m_pAppData->SetString("password", password);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Saves out application data
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTrackerDoc::SaveAppData( void )
{
	// kill the UID password in the app data when saving it
	char password[MAX_PASSWORD_LEN];
	memset(password, 0, MAX_PASSWORD_LEN);
	strncpy(password, m_pAppData->GetString("password"), MAX_PASSWORD_LEN - 1);
	password[MAX_PASSWORD_LEN - 1] = 0;

	// clear password
	m_pAppData->SetString("password", "");

	// make up the path to look for the data file

	// save the data
	m_pAppData->SaveToFile(vgui::filesystem(), "AppData.vdf", "CONFIG");

	// reset password
	m_pAppData->SetString("password", password);


	// encrypt the password
	int pos = strlen(password) + 1;

	// munge the end of the password, deterministically
	while (pos < MAX_PASSWORD_LEN)
	{
		password[pos] = 'a' + pos;
		pos++;
	}

	char encryptedPassword[MAX_PASSWORD_LEN];
	// encryption acts 8 bytes at a time
	for (int i = 0; i < MAX_PASSWORD_LEN; i += 8)
	{
		g_Cipher.encrypt((unsigned char *)(password + i), (unsigned char *)(encryptedPassword + i));
	}
	
	// save the password out to a seperate file
	FileHandle_t f = vgui::filesystem()->Open("masters.dat", "wb", "CONFIG");
	if (f)
	{
		vgui::filesystem()->Write(encryptedPassword, MAX_PASSWORD_LEN, f);
		vgui::filesystem()->Close(f);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDoc::SaveDialogState(vgui::Panel *dialog, const char *dialogName, unsigned int buddyID)
{
	// write the size and position to the document
	int x, y, wide, tall;
	dialog->GetBounds(x, y, wide, tall);

	KeyValues *data;
	if (buddyID)
	{
		data = GetBuddy(buddyID)->Data()->FindKey("Dialogs", true);
	}
	else
	{
		data = Data()->FindKey("User/Dialogs", true);
	}

	data = data->FindKey(dialogName, true);

	// write, tracker running standalone
	if (Tracker_StandaloneMode())
	{
		data->SetInt("x", x);
		data->SetInt("y", y);
		data->SetInt("w", wide);
		data->SetInt("t", tall);
	}
	else
	{
		// write, tracker running in game
		data->SetInt("game_x", x);
		data->SetInt("game_y", y);
		data->SetInt("game_w", wide);
		data->SetInt("game_t", tall);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDoc::LoadDialogState(vgui::Panel *dialog, const char *dialogName, unsigned int buddyID)
{
	// read the size and position from the document
	KeyValues *data;
	if (buddyID)
	{
		data = GetBuddy(buddyID)->Data()->FindKey("Dialogs", true);
	}
	else
	{
		data = Data()->FindKey("User/Dialogs", true);
	}
	data = data->FindKey(dialogName, true);

	// calculate defaults
	int x, y, wide, tall, dwide, dtall;
	int nx, ny, nwide, ntall;
	vgui::surface()->GetScreenSize(wide, tall);
	dialog->GetSize(dwide, dtall);
	x = (wide - dwide) / 2;
	y = (tall - dtall) / 2;

	// set dialog

	if (Tracker_StandaloneMode())
	{
		nx = data->GetInt("x", x);
		ny = data->GetInt("y", y);
		nwide = data->GetInt("w", dwide);
		ntall = data->GetInt("t", dtall);
	}
	else
	{
		nx = data->GetInt("game_x", x);
		ny = data->GetInt("game_y", y);
		nwide = data->GetInt("game_w", dwide);
		ntall = data->GetInt("game_t", dtall);	
	}

	// make sure it's on the screen
	if (nx + nwide > wide)
	{
		nx = wide - nwide;
	}
	if (ny + ntall > tall)
	{
		ny = tall - ntall;
	}
	if (nx < 0)
	{
		nx = 0;
	}
	if (ny < 0)
	{
		ny = 0;
	}

	dialog->SetBounds(nx, ny, nwide, ntall);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDoc::ShutdownAllDialogs()
{
	// clear the CBuddy's
	for (int i = 0; i < m_BuddyMap.MaxElement(); i++)
	{
		if (m_BuddyMap.IsValidIndex(i))
		{
			m_BuddyMap[i].pBuddy->ShutdownDialogs();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDoc::WriteOut(KeyValues *dat, ISendMessage *outFile)
{
	// data items with no type yet, must have no info, so never serialize them
	if (dat->IsEmpty(NULL))
		return;

	// write out the data
	if (dat->GetFirstSubKey())
	{
		// iterate through saving all sub items in this key
		for (vgui::KeyValues *it = dat->GetFirstSubKey(); it != NULL; it = it->GetNextKey())
		{
			WriteOut(it, outFile);
		}
	}
	else
	{
		// single item
		switch (dat->GetDataType(NULL))
		{
		case vgui::KeyValues::TYPE_INT:
			{
				int idat = dat->GetInt(NULL);
				outFile->WriteInt(dat->GetName(), idat);
			}
			break;

		case vgui::KeyValues::TYPE_STRING:
			{
				const char *str = dat->GetString(NULL);
				outFile->WriteString(dat->GetName(), str);
			}
			break;

		default:
			// do nothing
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTrackerDoc::ReadIn(KeyValues *dat, IReceiveMessage *inFile)
{
	// loop through reading until there are no more fields
	while (inFile->ReadField())
	{
		// find/create the field to set
		KeyValues *newDat = dat->FindKey(inFile->GetFieldName(), true);

		switch (inFile->GetFieldType())
		{
		case 1:	// binary, read it as an int
			{
				int idat;
				inFile->GetFieldData(&idat, 4);
				newDat->SetInt(NULL, idat);
			}
			break;

		case 0:	// string
			{
				static char buf[1024];
				inFile->GetFieldData(buf, 1023);
				newDat->SetString(NULL, buf);
			}
			break;

		default:
			// unknown field type
			break;
		};

		// advance to the next field
		inFile->AdvanceField();
	}
}

//-----------------------------------------------------------------------------
// Purpose: comparison function for buddy items
//-----------------------------------------------------------------------------
bool CTrackerDoc::BuddyMapItemLessFunc(const BuddyMapItem_t &r1, const BuddyMapItem_t &r2)
{
	return (r1.buddyID < r2.buddyID);
}
