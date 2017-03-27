// ModInfoSocket.cpp : implementation file
//

#include "stdafx.h"
#include "Status.h"
#include "Mod.h"
#include "MessageBuffer.h"
#include "ModInfoSocket.h"
#include "proto_oob.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern double Sys_DoubleTime(void);
extern int     (*BigLong) (int l);
/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket

//===================================================================
// CModInfoSocket()
//
// CModInfoSocket:  Constructur, keep a pointer to parent document
//===================================================================
CModInfoSocket::CModInfoSocket( void )
{
	pMessageBuffer = new CMessageBuffer(MASTER_BUFSIZE);
	ASSERT(pMessageBuffer);

	m_modStatus = MOD_PENDING;
	m_nRetries = MOD_RETRIES;
	m_fTimeOut = MOD_TIMEOUT;
	m_fSendTime = 0.0f;

	m_pList = NULL;
}

CModInfoSocket::~CModInfoSocket()
{
	ASSERT(pMessageBuffer);
	pMessageBuffer->SZ_Clear();
	delete pMessageBuffer;
}

// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(CModInfoSocket, CAsyncSocket)
	//{{AFX_MSG_MAP(CModInfoSocket)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif	// 0

/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket member functions

//===================================================================
// OnReceive()
//
// OnReceive:  Receive message from master server.
//===================================================================
void CModInfoSocket::OnReceive(int nErrorCode) 
{
	// We got a response!
	m_modStatus = MOD_SUCCESS;

	if (nErrorCode != 0)
	{
		CAsyncSocket::OnReceive(nErrorCode);
		iBytesRead = 0;
		return;
	}

	// Throw away residual message data.
	pMessageBuffer->SZ_Clear();

	iBytesRead = Receive(pMessageBuffer->GetData(), pMessageBuffer->GetMaxSize());
	if (!iBytesRead)
		return;

	pMessageBuffer->SetCurSize(iBytesRead);

	// Check message type
	int control;
	pMessageBuffer->MSG_BeginReading();
	control = *((int *)pMessageBuffer->GetData());
	pMessageBuffer->MSG_ReadLong();

	// Control should be 0xFFFFFFFF (i.e., -1)
	if (control != -1)
	{
		iBytesRead = 0;
		return;
	}

	// Check type byte and ignore if not valid
	unsigned char c;
	c = pMessageBuffer->MSG_ReadByte();

	switch (c)
	{
	default:  // Unrecognized
		return;

	case M2A_ACTIVEMODS:
		BatchModInfo( true );
		break;
	case M2A_ACTIVEMODS3:
		BatchModInfo( false );
		break;

	}

	CAsyncSocket::OnReceive(nErrorCode);
}

// Owner must call this periodically to check status.
// 1 == continue, 0 == done
int CModInfoSocket::CheckForResend( void )
{
	float fCurTime;

	fCurTime = (float)Sys_DoubleTime();

	if ( m_modStatus != MOD_PENDING )
	{
		// Done, success or failure
		return 0;
	}

	if ( ( fCurTime - m_fSendTime ) < m_fTimeOut )
	{
		return 1;
	}

	if ( m_nRetries-- <= 0 )
	{
		m_modStatus = MOD_FAILED;
		return 0;
	}

	// Resend the request buffer.
	AsyncSelect( FD_READ );
	Send( pMessageBuffer->GetData(), pMessageBuffer->GetCurSize() );
	m_fSendTime = (float)Sys_DoubleTime();
	m_fTimeOut *= 2.0f; // Wait 2x as long now.
	return 1;
}

extern char *CheckParm(const char *psz, char **ppszValue = NULL);

void CModInfoSocket::RequestModInfo( char *pszRequest )
{
	char szRequest[ 128 ];
	if ( CheckParm( "-old" ) )
	{
		sprintf( szRequest, "%c\r\n%s\r\n", A2M_GETACTIVEMODS, pszRequest );
	}
	else
	{
		sprintf( szRequest, "%c\r\n%s\r\n", A2M_GETACTIVEMODS3, pszRequest );
	}

	// Send it.
	pMessageBuffer->SZ_Clear();
	pMessageBuffer->MSG_WriteString( szRequest );

	AsyncSelect( FD_READ );
	Send( pMessageBuffer->GetData(), pMessageBuffer->GetCurSize() );

	m_fSendTime = (float)Sys_DoubleTime();
	m_modStatus     = MOD_PENDING;
	m_nRetries = MOD_RETRIES;
	m_fTimeOut = MOD_TIMEOUT;
}

void CModInfoSocket::StartModRequest( CModList *pList, char *address, int port )
{
	if ( !Create(0,SOCK_DGRAM) )
		return;

	if ( !Connect( address, port ) )
		return;

	m_pList = pList;
	m_modStatus     = MOD_PENDING;
	m_nRetries = MOD_RETRIES;
	m_fTimeOut = MOD_TIMEOUT;
	RequestModInfo( "start-of-list" );
}

// Remove duplicate names from list
void ConsolidateMods( CModList *pList )
{
	// Make all game directories lower case
	CMod *p;
	CModList newlist;
	CMod *pmod;

	int i;

	for ( i = pList->getCount() - 1 ; i >= 0 ; i-- )
	{
		p = pList->getMod( i );
		pList->RemoveMod( p );

		pmod = newlist.FindMod( p->GetGameDir() );
		if ( !pmod )
		{
			pmod = p;
			newlist.AddMod( p );
		}
		else
		{
			// Increment values
			pmod->Add( p );

			// Remove duplicate mod
			delete p;
		}
	}

	pList->copyData( &newlist );
}

void CModInfoSocket::BatchModInfo( bool oldStyle )
{
	static int batch = 0;
	CMod *pMod;
	char *pszModName = NULL;
	char szName[ 256 ];
	int nPlayers = 0, nServers = 0;
	char szServers[ 128 ];
	char szPlayers[ 128 ];
	int nCount = 0;

	pMessageBuffer->MSG_ReadByte(); 

	szName[0] = '\0';

	while ( 1 )
	{
		pszModName = pMessageBuffer->MSG_ReadString(); 

		if ( !pszModName || !pszModName[0] && ( pMessageBuffer->GetReadCount() >= pMessageBuffer->GetCurSize() ) )
			break;

		if ( !stricmp( pszModName, "end-of-list" ) )
		{
			break;
		}

		if ( !stricmp( pszModName, "more-in-list" ) )
		{
			break;
		}

		if ( oldStyle )
		{
			strcpy( szName, pszModName );

			strcpy( szServers, pMessageBuffer->MSG_ReadString() ); 
			strcpy( szPlayers, pMessageBuffer->MSG_ReadString() );

			// Check validity ( no backslashes allowed! )
			if ( !strstr( szName, "\\" ) &&
				 !strstr( szServers, "\\" ) &&
				 !strstr( szPlayers, "\\" ) && 
				 atoi( szServers ) <= 1000000 && 
				 atoi( szServers ) >= 0 &&
				 atoi( szPlayers ) <= 1000000 &&
				 atoi( szPlayers ) >= 0 )
			{
				nCount++;

				pMod = m_pList->FindMod( szName );
				if ( !pMod )
				{
					pMod = new CMod;
					pMod->SetGameDir( szName );

					// Link
					m_pList->AddMod( pMod );
				}
			
				// Slam data
				pMod->total.servers = atoi( szServers );
				pMod->total.players = atoi( szPlayers );
				pMod->total.bots	= 0;
				pMod->total.bots_servers= 0;
			}
		}
		else
		{
			char key[ 128 ];
			char value[ 128 ];

			strcpy( value,  pMessageBuffer->MSG_ReadString() );

			strcpy( szName, value );

			pMod = m_pList->FindMod( value );
			if ( !pMod )
			{
				pMod = new CMod;
				pMod->SetGameDir( value );

				// Link
				m_pList->AddMod( pMod );
			}

			while ( 1 )
			{
				strcpy( key, pMessageBuffer->MSG_ReadString() ); 
				strcpy( value,  pMessageBuffer->MSG_ReadString() );

				if ( !stricmp( key, "end" ) )
				{
					break;
				}

				if ( pMod )
				{
					if ( !stricmp( key, "ip" ) )
					{
						pMod->stats[ MOD_INTERNET ].players = atoi( value );
					}
					else if ( !stricmp( key, "is" ) )
					{
						pMod->stats[ MOD_INTERNET ].servers = atoi( value );
					}
					else if ( !stricmp( key, "lp" ) )
					{
						pMod->stats[ MOD_LAN ].players = atoi( value );
					}
					else if ( !stricmp( key, "ls" ) )
					{
						pMod->stats[ MOD_LAN ].servers = atoi( value );
					}
					else if ( !stricmp( key, "lbs" ) )
					{
						pMod->stats[ MOD_LAN ].bots_servers = atoi( value );
					}	
					else if ( !stricmp( key, "pp" ) )
					{
						pMod->stats[ MOD_PROXY ].players = atoi( value );
					}
					else if ( !stricmp( key, "ps" ) )
					{
						pMod->stats[ MOD_PROXY ].servers = atoi( value );
					}		
					else if ( !stricmp( key, "ib" ) )
					{
						pMod->stats[ MOD_INTERNET ].bots = atoi( value );
					}	
					else if ( !stricmp( key, "ibs" ) )
					{
						pMod->stats[ MOD_INTERNET ].bots_servers = atoi( value );
					}	
					else if ( !stricmp( key, "lb" ) )
					{
						pMod->stats[ MOD_LAN ].bots = atoi( value );
					}
				}
			}

			if ( pMod )
			{
				pMod->ComputeTotals();
			}

			nCount++;
		}
	}

	if ( ( nCount >= 1 ) && pszModName && pszModName[0] && !stricmp( pszModName, "more-in-list" ) ) // More
	{
		// This will reset the state flags.
		RequestModInfo( szName );
	}
	else
	{
		ConsolidateMods( m_pList );
	}
}

