//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "../common/MessageDescription.h"
#include "../TrackerNET/Threads.h"
#include "../TrackerNET/TrackerNET_Interface.h"
#include "../common/TrackerMessageFlags.h"

#include "LoadTestApp.h"
#include "User.h"
#include "Random.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CUser::CUser()
{
	m_bActive = false;
	m_szNameBase[0] = 0;
	m_pNet = NULL;
	m_bLoggedIn = false;
	m_iStatus = 0;
	m_iSessionID = 0;
	m_iUID = 0;
	m_iSearchType = SEARCH_NONE;

	// use a fake build number to indicate we're a test user
	m_iBuildNum = 2337;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CUser::~CUser()
{
	m_bActive = false;
	m_pData = NULL;

	if (m_pNet)
	{
//!! commented out because of Shutdown crash
//		m_pNet->Shutdown(true);
//		m_pNet->deleteThis();
//		m_pNet = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUser::IsActive()
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : KeyValues *
//-----------------------------------------------------------------------------
KeyValues *CUser::Data()
{
	return m_pData;
}

// random act dispatch table
typedef void (CUser::*actPtr)();
struct RandomActDispatch_t
{
	actPtr msgFunc;
	float probability;	// likelyhood of this happening
};

// probability values in table should all add up to 1.0f
static RandomActDispatch_t g_RandomActDispatch[] =
{
	{ CUser::Act_ChangeStatus, 0.7f },
	{ CUser::Act_SearchForFriends, 0.2f },
	{ CUser::Act_Logoff, 1.0f },
};

//-----------------------------------------------------------------------------
// Purpose: Chooses an act at random to perform
//-----------------------------------------------------------------------------
void CUser::PerformRandomAct()
{
	if (m_iStatus == -1)
	{
		// user is being created; just wait
		return;
	}
	else if (m_iStatus == 0)
	{
		// we need to log on
		LoginUser();
		return;
	}

	float chance = (float)rand() / (float)RAND_MAX;

	// walk through the table and find our slot
	int arraySize = ARRAYSIZE(g_RandomActDispatch);
	for (int i = 0; i < arraySize; i++)
	{
		chance -= g_RandomActDispatch[i].probability;
		if (chance <= 0.0f)
		{
			// we've found the slot;  perform the act
			(this->*g_RandomActDispatch[i].msgFunc)();
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Random act of status changing
//-----------------------------------------------------------------------------
void CUser::Act_ChangeStatus()
{
	int newStatus = (int)((((float)rand() / (float)RAND_MAX) * 4) + 1);

	if (m_iStatus == newStatus)
	{
		if (m_iStatus == 1)
		{
			m_iStatus = 4;
		}
		else
		{
			m_iStatus = 1;
		}
	}
	else
	{
		m_iStatus = newStatus;
	}

	g_pConsole->Print(0, "Act_ChangeStatus(%d)\n", m_iStatus);
	SendMsg_HeartBeat();
}

//-----------------------------------------------------------------------------
// Purpose: Searches for a friend
//-----------------------------------------------------------------------------
void CUser::Act_SearchForFriends()
{
	// choose search type
	m_iSearchType = ((rand() * 4) / RAND_MAX) + 1;

	// generate an user ID to search for
	int index = (rand() * gApp.GetNumUsers()) / RAND_MAX;
	char buf[64];

	switch (m_iSearchType)
	{
	case SEARCH_USERNAME:
		sprintf(buf, "t_%s%d", gApp.GetUserIdBase(), index);
		SendMsg_FriendSearch(0, "", buf, "", "");
		break;

	case SEARCH_FIRSTNAME:
		sprintf(buf, "t_%s%d_First", gApp.GetUserIdBase(), index);
		SendMsg_FriendSearch(0, "", "", buf, "");
		break;

	case SEARCH_LASTNAME:
		strcpy(buf, "tester_Last");	// will get lots of results with this one
		SendMsg_FriendSearch(0, "", "", "", buf);
		break;

	case SEARCH_EMAIL:
	default:
		sprintf(buf, "t_%s%d@test.vlv", gApp.GetUserIdBase(), index);
		SendMsg_FriendSearch(0, buf, "", "", "");
		break;
	}

	g_pConsole->Print(0, "Act_SearchForFriends(%s)\n", buf);
}

//-----------------------------------------------------------------------------
// Purpose: Random act of logging off the server
//-----------------------------------------------------------------------------
void CUser::Act_Logoff()
{
	g_pConsole->Print(0, "Act_Logoff()\n");
	LogoffUser();
}

//-----------------------------------------------------------------------------
// Purpose: Creates the user if they're not initialized
//-----------------------------------------------------------------------------
void CUser::CreateUser(const char *nameBase, CreateInterfaceFn netFactory)
{
	m_bActive = true;
	strcpy(m_szNameBase, nameBase);
	m_pData = new KeyValues(m_szNameBase);
	m_iStatus = -1;

	// make up some names
	char buf[256];
	sprintf(buf, "%s@test.vlv", m_szNameBase);
	m_pData->SetString("Email", buf);
	sprintf(buf, "%s", m_szNameBase);
	m_pData->SetString("UserName", buf);
	sprintf(buf, "%s_First", m_szNameBase);
	m_pData->SetString("FirstName", buf);
	m_pData->SetString("LastName", "tester_Last");
	m_pData->SetString("password", "test");

	g_pConsole->Print(0, "CreateUser(%s)\n", nameBase);

	// setup networking
	if (!m_pNet)
	{
		m_pNet = (ITrackerNET *)netFactory(TRACKERNET_INTERFACE_VERSION, NULL);
		m_pNet->Initialize(25000, 29000);
	}

	//!! need to randomly pick server address from list

	/*
	int rand = RandomLong(0, 2);
	if (rand == 0)
	{
		m_ServerAddress = m_pNet->GetNetAddress("tracker.valvesoftware.com:1200");
	}
	else if (rand == 1)
	{
		m_ServerAddress = m_pNet->GetNetAddress("tracker3.valvesoftware.com:1200");
	}
	else if (rand == 2)
	{
		m_ServerAddress = m_pNet->GetNetAddress("tracker5.valvesoftware.com:1200");
	}
	*/
	m_ServerAddress = m_pNet->GetNetAddress("johncookdell:1200");

	// send network message
	SendMsg_CreateUser(m_pData->GetString("email"), m_pData->GetString("UserName"), m_pData->GetString("FirstName"), m_pData->GetString("LastName"), "test");
}

// message dispatch table
typedef void (CUser::*funcPtr)( KeyValues * );
struct ServerMsgDispatch_t
{
	const char *msgName;
	funcPtr msgFunc;
};

static ServerMsgDispatch_t g_ServerMsgDispatch[] =
{
	{ "Challenge",	CUser::RecvMsg_Challenge },
	{ "LoginOK",	CUser::RecvMsg_LoginOK },
	{ "LoginFail",	CUser::RecvMsg_LoginFail },
	{ "Friends",	CUser::RecvMsg_Friends },
	{ "FriendUpdate", CUser::RecvMsg_FriendUpdate },
	{ "Heartbeat",	CUser::RecvMsg_Heartbeat },
	{ "PingAck",	CUser::RecvMsg_PingAck },
	{ "UserCreated",	CUser::RecvMsg_UserCreated },
	{ "UserCreateDenied",	CUser::RecvMsg_UserCreateDenied },
	{ "FriendFound",	CUser::RecvMsg_FriendFound },
	{ "Message",		CUser::RecvMsg_Message },
};

//-----------------------------------------------------------------------------
// Purpose: checks, read and respond to any networking messages
//-----------------------------------------------------------------------------
void CUser::ProcessNetworkInput()
{
	if (!m_pNet)
		return;

	IReceiveMessage *data;
	while ((data = m_pNet->GetIncomingData()) != NULL)
	{
		// convert the message to be KeyValues
		KeyValues *msg = new KeyValues(data->GetMsgName());
		ReadIn(msg, data);
		m_pNet->ReleaseMessage(data);

		int arraySize = ARRAYSIZE(g_ServerMsgDispatch);
		for (int i = 0; i < arraySize; i++)
		{
			if (!stricmp(msg->GetName(), g_ServerMsgDispatch[i].msgName))
			{
				(this->*g_ServerMsgDispatch[i].msgFunc)(msg);
				break;
			}
		}

		delete msg;
	}

	// check to see if we need to send a ping
	if (m_bLoggedIn && g_pThreads->GetTime() > m_flHeartbeatTime)
	{
		SendMsg_HeartBeat();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Logs a user onto tracker net
//-----------------------------------------------------------------------------
void CUser::LoginUser()
{
	m_bLoggedIn = true;
	SendMsg_Login(m_iUID, m_pData->GetString("email"), m_pData->GetString("password"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUser::LogoffUser()
{
	m_bLoggedIn = false;
	m_iSessionID = 0;
	SendMsg_Logoff();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUser::UpdateHeartbeatTime()
{
	// heartbeat once per minute
	m_flHeartbeatTime = g_pThreads->GetTime() + 30.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *msgName - 
// Output : ISendMessage
//-----------------------------------------------------------------------------
ISendMessage *CUser::CreateServerMessage(const char *msgName)
{
	ISendMessage *msg = m_pNet->CreateMessage(msgName);
	msg->SetNetAddress(GetServerAddress());
	msg->SetSessionID(m_iSessionID);

	return msg;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CNetAddress
//-----------------------------------------------------------------------------
CNetAddress CUser::GetServerAddress()
{
	return m_ServerAddress;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *dat - 
//			*outFile - 
//-----------------------------------------------------------------------------
void CUser::WriteOut(KeyValues *dat, ISendMessage *outFile)
{
	// data items with no type yet, must have no info, so never serialize them
	if (dat->IsEmpty(NULL))
		return;

	// write out the data
	if (dat->GetFirstSubKey())
	{
		// subkey
		outFile->StartSubMessage(dat->GetName());

		// iterate through saving all sub items in this key
		for (KeyValues *it = dat->GetFirstSubKey(); it != NULL; it = it->GetNextKey())
		{
			WriteOut(it, outFile);
		}

		outFile->EndSubMessage();
	}
	else
	{
		// single item
		switch ( dat->GetDataType(NULL) )
		{
		case KeyValues::TYPE_INT:
			{
				int idat = dat->GetInt(NULL);
				outFile->WriteField( dat->GetName(), MSGFIELD_INTEGER, &idat, 1 );
			}
			break;

		case KeyValues::TYPE_FLOAT:
			{
				float fdat = dat->GetFloat(NULL);
				outFile->WriteField( dat->GetName(), MSGFIELD_FLOAT, &fdat, 1 );
			}
			break;

		case KeyValues::TYPE_STRING:
			{
				const char *str = dat->GetString(NULL);
				outFile->WriteField( dat->GetName(), MSGFIELD_CHARPTR, (void *)&str, 1 );
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
// Input  : *dat - 
//			*inFile - 
//-----------------------------------------------------------------------------
void CUser::ReadIn(KeyValues *dat, IReceiveMessage *inFile)
{
	// loop through reading until there are no more fields
	while (inFile->ReadField())
	{
		// find/create the field to set
		KeyValues *newDat = dat->FindKey(inFile->GetFieldName(), true);

		switch (inFile->GetFieldType())
		{
		case MSGFIELD_SUBMSG:
			{
				// Start a new sub message
				inFile->StartSubMessage();

				// recursively read in the next set of fields
				ReadIn(newDat, inFile);

				// move back to the previous message
				inFile->EndSubMessage();
			}
			break;

		case MSGFIELD_INTEGER:
			{
				int idat;
				inFile->GetFieldData( &idat, 4 );
				newDat->SetInt(NULL, idat);
			}
			break;

		case MSGFIELD_FLOAT:
			{
				float fdat;
				inFile->GetFieldData( &fdat, 4 );
				newDat->SetFloat(NULL, fdat);
			}
			break;

		case MSGFIELD_CHARPTR:
			{
				static char buf[2048];
				inFile->GetFieldData(buf, 2048);
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
// Purpose: Message sending functions
//-----------------------------------------------------------------------------
void CUser::SendMsg_CreateUser(const char *email, const char *userName, const char *firstName, const char *lastName, const char *password)
{
	struct CreateUser_t
	{
		const char *userName;
		const char *firstName;
		const char *lastName;
		const char *email;
		const char *password;
	};

	static messagedescription_t g_RequestCreateUserMessage[] = 
	{
		MSG_FIELD( CreateUser_t, "username", userName, MSGFIELD_CHARPTR ),
		MSG_FIELD( CreateUser_t, "firstname", firstName, MSGFIELD_CHARPTR ),
		MSG_FIELD( CreateUser_t, "lastname", lastName, MSGFIELD_CHARPTR ),
		MSG_FIELD( CreateUser_t, "email", email, MSGFIELD_CHARPTR ),
		MSG_FIELD( CreateUser_t, "password", password, MSGFIELD_CHARPTR ),
	};

	// construct the message
	CreateUser_t userMsg = 
	{ 
		userName, 
		firstName, 
		lastName, 
		email,
		password
	};

	// send the message, registry ourselves as the reply handler
	ISendMessage *sendMsg = CreateServerMessage("CreateUser");
	sendMsg->WriteFieldsFromDescription(&userMsg, g_RequestCreateUserMessage, ARRAYSIZE(g_RequestCreateUserMessage));
	m_pNet->SendMessage(sendMsg, NET_RELIABLE);

	UpdateHeartbeatTime();
}

//-----------------------------------------------------------------------------
// Purpose: Sends the first pack in the login sequence
//-----------------------------------------------------------------------------
void CUser::SendMsg_Login(int uid, const char *email, const char *password)
{
	// assemble login message
	struct msg_login_t
	{
		int iUID;
		const char	*pszEmail;
		const char	*pszPassword;
		int iStatus;
	};

	static messagedescription_t g_LoginMessage[] = 
	{
		MSG_FIELD( msg_login_t, "uid", iUID, MSGFIELD_INTEGER ),
		MSG_FIELD( msg_login_t, "email", pszEmail, MSGFIELD_CHARPTR ),
		MSG_FIELD( msg_login_t, "password", pszPassword, MSGFIELD_CHARPTR ),
		MSG_FIELD( msg_login_t, "status", iStatus, MSGFIELD_INTEGER ),
	};

	// try to log in to the server
	msg_login_t loginData = {	uid, 
								email, 
								password,
								1 };

	// setup the login message
	ISendMessage *loginMsg = m_pNet->CreateMessage( "login" );

	loginMsg->SetNetAddress( GetServerAddress() );
	loginMsg->WriteFieldsFromDescription( &loginData, g_LoginMessage, ARRAYSIZE(g_LoginMessage) );
	m_pNet->SendMessage(loginMsg, NET_RELIABLE);

	UpdateHeartbeatTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUser::SendMsg_FriendSearch(int uid, const char *email, const char *username, const char *firstname, const char *lastname)
{
	struct msg_friend_t
	{
		int iUID;
		const char  *pszEmail;
		const char	*pszUserName;
		const char	*pszFirstName;
		const char	*pszLastName;
	} friendData =
	{
		uid,
		email,
		username,
		firstname,
		lastname
	};

	// the message contains a list of friends found
	static messagedescription_t g_FriendSearchMsg[] =
	{
		MSG_FIELD(msg_friend_t, "UID", iUID, MSGFIELD_INTEGER),
		MSG_FIELD(msg_friend_t, "Email", pszEmail, MSGFIELD_CHARPTR),
		MSG_FIELD(msg_friend_t, "FirstName", pszFirstName, MSGFIELD_CHARPTR),
		MSG_FIELD(msg_friend_t, "LastName", pszLastName, MSGFIELD_CHARPTR),
		MSG_FIELD(msg_friend_t, "UserName", pszUserName, MSGFIELD_CHARPTR),
	};

	ISendMessage *msg = CreateServerMessage("FriendSearch");
	msg->WriteFieldsFromDescription(&friendData, g_FriendSearchMsg, ARRAYSIZE(g_FriendSearchMsg));
	m_pNet->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUser::SendMsg_Logoff()
{
	m_iStatus = 0;
	SendMsg_HeartBeat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUser::SendMsg_HeartBeat()
{
	ISendMessage *msg = CreateServerMessage("heartbeat");
	msg->WriteField("status", MSGFIELD_INTEGER, &m_iStatus, 1);
	m_pNet->SendMessage(msg, NET_UNRELIABLE);

	UpdateHeartbeatTime();
}

//-----------------------------------------------------------------------------
// Purpose: Requests authorization from a user
//-----------------------------------------------------------------------------
void CUser::SendMsg_ReqAuth(int userID)
{
	ISendMessage *authUserMsg = CreateServerMessage("ReqAuth");
	authUserMsg->WriteField("UID", MSGFIELD_INTEGER, &userID, 1);
	m_pNet->SendMessage(authUserMsg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : userID - 
//			authed - 
//-----------------------------------------------------------------------------
void CUser::SendMsg_AuthUser(int targetID, bool allow)
{
	ISendMessage *msg = CreateServerMessage("AuthUser");
	msg->WriteField("uid", MSGFIELD_INTEGER, &m_iUID, 1);
	msg->WriteField("targetID", MSGFIELD_INTEGER, &targetID, 1);
	int auth = allow;
	msg->WriteField("auth", MSGFIELD_INTEGER, &auth, 1);

	m_pNet->SendMessage(msg, NET_RELIABLE);
}

//-----------------------------------------------------------------------------
// Purpose: Received message handlers
// Input  : *msg - 
//-----------------------------------------------------------------------------
void CUser::RecvMsg_Challenge(KeyValues *msg)
{
//	g_pConsole->Print(0, "RecvMsg_Challenge()\n");
	static messagedescription_t g_ChallengeMsg[] =
	{
		MSG_FIELD( CUser, "challenge", m_iChallengeKey, MSGFIELD_INTEGER ),
		MSG_FIELD( CUser, "sessionID", m_iSessionID, MSGFIELD_INTEGER ),
		MSG_FIELD( CUser, "build", m_iBuildNum, MSGFIELD_INTEGER ),
	};

	m_iChallengeKey = msg->GetInt("challenge");
	m_iSessionID = msg->GetInt("sessionID");

	// respond to the challenge
	ISendMessage *reply = CreateServerMessage("response");
	reply->WriteFieldsFromDescription( this, g_ChallengeMsg, ARRAYSIZE(g_ChallengeMsg) );
	m_pNet->SendMessage(reply, NET_RELIABLE);
}

void CUser::RecvMsg_LoginOK(KeyValues *msg)
{
	// login is complete
//	g_pConsole->Print(0, "RecvMsg_LoginOK()\n");
	m_iStatus = 1;
}

void CUser::RecvMsg_LoginFail(KeyValues *msg)
{
	m_bLoggedIn = false;
}

void CUser::RecvMsg_Friends(KeyValues *msg)
{
}

void CUser::RecvMsg_FriendUpdate(KeyValues *msg)
{
}

void CUser::RecvMsg_Heartbeat(KeyValues *msg)
{
}

void CUser::RecvMsg_PingAck(KeyValues *msg)
{
}

void CUser::RecvMsg_UserCreated(KeyValues *msg)
{
	// creation successful, get our userID
	if (msg->GetInt("newUID") < 1)
	{
		// we've been thrown back a bad ID
		RecvMsg_UserCreateDenied(msg);
		return;
	}

	m_iUID = msg->GetInt("newUID");
	Data()->SetInt("UID", m_iUID);

	// login to the server
	LoginUser();
}

void CUser::RecvMsg_UserCreateDenied(KeyValues *msg)
{
}

//-----------------------------------------------------------------------------
// Purpose: Reply message from a friend search
// Input  : *msg - 
//-----------------------------------------------------------------------------
void CUser::RecvMsg_FriendFound(KeyValues *msg)
{
	float chance = (float)rand() / (float)RAND_MAX;
	bool bReqAuth = false;

	if (m_iSearchType == SEARCH_LASTNAME)
	{
		// low chance, since we're going to get so many replies
		if (chance > 0.9)
		{
			bReqAuth = true;
		}
	}
	else
	{
		// high chance
		if (chance > 0.5)
		{
			bReqAuth = true;
		}
	}

	if (bReqAuth && msg->GetInt("UID") > 0)
	{
		SendMsg_ReqAuth(msg->GetInt("UID"));
		g_pConsole->Print(0, "SendMsg_ReqAuth(%d)\n", msg->GetInt("UID"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when receiving a message
// Input  : *msg - 
//-----------------------------------------------------------------------------
void CUser::RecvMsg_Message(KeyValues *msg)
{
	int flags = msg->GetInt("flags");
	if (flags & MESSAGEFLAG_REQUESTAUTH)
	{
		// user has requested authorization

		// reply with a yes
		SendMsg_AuthUser(msg->GetInt("UID"), true);
		g_pConsole->Print(0, "SendMsg_AuthUser(%d)\n", msg->GetInt("UID"));
	}
}



