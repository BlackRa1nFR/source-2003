//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "stdafx.h"
#include <afxcmn.h>
#include <stdio.h>
#include "EmailManager.h"
#include <KeyValues.h>
#include "Status.h"
#include "StatusDlg.h"
#include "FileSystem.h"
#include "FileSystem_Tools.h"
#include "cmdlib.h"

#define STATUSEMAILS_FILE "data/StatusEmails.txt"
#define STATUSEMAILS_FILE_BACKUP "data/StatusEmails_backup.txt"

// Update emails once per minute
#define	EMAIL_UPDATE_INTERVAL 60.0f

CEmailItem::CEmailItem()
{
	memset( m_szEmail, 0, sizeof( m_szEmail ) );
	memset( m_szFreq, 0, sizeof( m_szFreq ) );

	m_flUpdateInterval = 0.0f;
	m_flLastUpdateTime = 0.0f;
}

double CEmailItem::ComputeInterval( char const *freq )
{
	// Assume once per day
	double interval = 3600.0f * 24.0f;

	double magnitude = atof( freq );
	if ( magnitude > 0.0f )
	{
		double scale = 1.0f;
		if ( strstr( freq, "second" ) )
		{
			scale = 1.0f;
		}
		else if ( strstr( freq, "minute" ) )
		{
			scale = 60.0f;
		}
		else if ( strstr( freq, "hour" ) )
		{
			scale = 3600.0f;
		}
		else if ( strstr( freq, "day" ) )
		{
			scale = 3600.0f * 24.0f;
		}
		else if ( strstr( freq, "week" ) )
		{
			scale = 3600.0f * 24.0f * 7.0f;
		}
		else if ( strstr( freq, "month" ) )
		{
			scale = 3600.0f * 24.0f * 30.0f;
		}

		interval = magnitude * scale;
	}

	return interval;
}

CEmailManager::CEmailManager()
{
	m_flCurTime = 0.0f;
}

CEmailManager::~CEmailManager()
{
}

void CEmailManager::Init( CStatusDlg *status )
{
	m_pStatusDlg = status;

	strcpy( basegamedir, "." );
	strcpy( gamedir, "." );

	FileSystem_Init(true);
	if ( g_pFullFileSystem )
	{
		g_pFullFileSystem->AddSearchPath( ".", "GAME" );

		LoadFromFile();
	}
}

void CEmailManager::Shutdown()
{
	m_Items.Purge();
}

void CEmailManager::Update( double curtime )
{
	m_flCurTime = curtime;
	if ( m_flCurTime < m_flNextUpdateTime )
		return;

	m_flNextUpdateTime = m_flCurTime + EMAIL_UPDATE_INTERVAL;


	int c = GetItemCount();
	bool first = true;
	for ( int i = 0; i < c; i++ )
	{
		if ( ShouldSendEmail( i ) )
		{
			if ( first )
			{
				first = false;
				bool bOK =  m_pStatusDlg->GenerateReport();
				if ( !bOK )
				{
					return;
				}
			}
			m_pStatusDlg->EmailReport( GetItemEmailAddress( i ) );
			NotifyEmailSent( i );
		}
	}
}

void CEmailManager::SendEmailsNow( void )
{
	bool bOK =  m_pStatusDlg->GenerateReport();
	if ( !bOK )
		return;

	int c = GetItemCount();
	for ( int i = 0; i < c; i++ )
	{
		m_pStatusDlg->EmailReport( GetItemEmailAddress( i ) );
	}
}

void CEmailManager::AddEmailAddress( char const *email, char const *frequency, bool savetofile /*= true*/ )
{
	bool dirty = false;
	CEmailItem *p = NULL;
	int idx = FindEmailAddress( email );
	if ( idx == -1 )
	{
		idx = m_Items.AddToTail();
		dirty = true;
	}

	p = &m_Items[ idx ];
	
	if ( stricmp( frequency, p->m_szFreq ) )
	{
		strcpy( p->m_szFreq, frequency );
		p->m_flUpdateInterval = CEmailItem::ComputeInterval( frequency );
		dirty = true;
	}

	strcpy( p->m_szEmail, email );
	
	if ( !dirty )
		return;

	if ( savetofile )
	{
		SaveToFile();
	}
}

void CEmailManager::RemoveEmailAddress( char const *email )
{
	int idx = FindEmailAddress( email );
	if ( idx != -1 )
	{
		RemoveItemByIndex( idx );
	}
}

int CEmailManager::GetItemCount( void )
{
	return m_Items.Count();
}

void CEmailManager::GetItemDescription( int item, char *buf, int maxlen )
{
	char sz[ 256 ];
	CEmailItem *p = &m_Items[ item ];
	
	sprintf( sz, "%s, %s", p->m_szEmail, p->m_szFreq );
	strncpy( buf, sz, maxlen );
}

int CEmailManager::FindEmailAddress( char const *email )
{
	for ( int i = 0; i < GetItemCount(); i++ )
	{
		if ( !stricmp( email, GetItemEmailAddress( i ) ) )
			return i;
	}
	
	return -1;
}

char const *CEmailManager::GetItemEmailAddress( int item )
{
	CEmailItem *p = &m_Items[ item ];
	return p->m_szEmail;
}

void CEmailManager::RemoveItemByIndex( int item )
{
	if ( item != -1 )
	{
		m_Items.Remove( item );
		SaveToFile();
	}
}

bool CEmailManager::ShouldSendEmail( int item )
{
	CEmailItem *p = &m_Items[ item ];
	if ( m_flCurTime >= p->m_flLastUpdateTime + p->m_flUpdateInterval )
		return true;

	return false;
}

void CEmailManager::NotifyEmailSent( int item )
{
	CEmailItem *p = &m_Items[ item ];
	p->m_flLastUpdateTime = m_flCurTime;
}

void CEmailManager::LoadFromFile()
{
	m_Items.RemoveAll();

	KeyValues *email = new KeyValues( "StatusEmails" );
	if ( !email )
		return;

	email->LoadFromFile( g_pFileSystem, STATUSEMAILS_FILE );

	KeyValues *kv = email->GetFirstSubKey();
	while ( kv )
	{
		char const *interval = kv->GetString( "interval", "1 days" );
		char const *email = kv->GetName();

		AddEmailAddress( email, interval, false );

		kv = kv->GetNextKey();
	}
	email->deleteThis();
}

void CEmailManager::SaveToFile()
{
	if ( g_pFullFileSystem )
	{
		g_pFullFileSystem->RemoveFile( STATUSEMAILS_FILE_BACKUP, "GAME" );
		g_pFullFileSystem->RenameFile( STATUSEMAILS_FILE, STATUSEMAILS_FILE_BACKUP, "GAME" );
	}

	KeyValues *email = new KeyValues( "StatusEmails" );
	if ( !email )
		return;

	int c = GetItemCount();
	for ( int i = 0; i < c; i++ )
	{
		CEmailItem *item = &m_Items[ i ];

		KeyValues *section = email->FindKey( item->m_szEmail, true );
		if ( section )
		{
			section->SetString( "interval", item->m_szFreq );
		}
	}

	email->SaveToFile( g_pFileSystem, STATUSEMAILS_FILE );

	email->deleteThis();
}

static CEmailManager g_EmailManager;

CEmailManager *GetEmailManager()
{
	return &g_EmailManager; 
}