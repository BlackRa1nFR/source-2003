//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef EMAILMANAGER_H
#define EMAILMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

class CStatusDlg;

class CEmailItem
{
public:
	CEmailItem();
	
	char		m_szEmail[ 128 ];
	char		m_szFreq[ 128 ];

	double		m_flUpdateInterval;
	double		m_flLastUpdateTime;

	static double ComputeInterval( char const *freq );
};

class CEmailManager
{
public:
	CEmailManager();
	~CEmailManager();

	void Init( CStatusDlg *dlg );
	void Shutdown();

	void Update( double curtime );

	void SendEmailsNow( void );

	void AddEmailAddress( char const *email, char const *frequency, bool savetofile = true );
	void RemoveEmailAddress( char const *email );

	int FindEmailAddress( char const *email );

	int GetItemCount( void );
	void GetItemDescription( int item, char *buf, int maxlen );
	char const *GetItemEmailAddress( int item );
	void RemoveItemByIndex( int item );

	bool	ShouldSendEmail( int item );
	void	NotifyEmailSent( int item );

private:

	void	LoadFromFile();
	void	SaveToFile();

	CUtlVector< CEmailItem > m_Items;

	double	m_flCurTime;
	// Only send emails once per minute
	double	m_flNextUpdateTime;

	CStatusDlg *m_pStatusDlg;

};

extern CEmailManager *GetEmailManager();

#endif // EMAILMANAGER_H
