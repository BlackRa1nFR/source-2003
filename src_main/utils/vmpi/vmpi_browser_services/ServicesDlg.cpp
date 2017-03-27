// ServicesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ServicesDlg.h"
#include "vmpi.h"
#include "bitbuf.h"
#include "vstdlib/strtools.h"
#include "patchtimeout.h"
#include "SetPasswordDlg.h"
#include "vmpi_browser_helpers.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SERVICE_OFF_TIMEOUT					(20*1000)	// If we haven't heard from a service in this long,
														// then we assume the service is off.

#define SERVICES_PING_INTERVAL				(3*1000)	// ping the services every so often

#define SERVICE_MAX_UPDATE_INTERVAL			(8*1000)	// Update each service in the listbox at least this often.


// Returns the argument following pName.
// If pName is the last argument on the command line, returns pEndArgDefault.
// Returns NULL if there is no argument with pName.
const char* FindArg( const char *pName, const char *pEndArgDefault="" )
{
	for ( int i=0; i < __argc; i++ )
	{
		if ( stricmp( pName, __argv[i] ) == 0 )
		{
			if ( (i+1) < __argc )
				return __argv[i+1];
			else
				return pEndArgDefault;
		}
	}
	return NULL;
}


// --------------------------------------------------------------------------------------------------------- //
// Column sort functions.
// --------------------------------------------------------------------------------------------------------- //

const char* GetStatusString( CServiceInfo *pInfo )
{
	if ( Plat_MSTime() - pInfo->m_LastPingTimeMS > SERVICE_OFF_TIMEOUT )
		return  "off";
	else if ( pInfo->m_iState == VMPI_STATE_BUSY )
		return  "busy";
	else if ( pInfo->m_iState == VMPI_STATE_PATCHING )
		return "patching";
	else if ( pInfo->m_iState == VMPI_STATE_DISABLED )
		return "disabled";
	else
		return "idle";
}


typedef int (CALLBACK *ServicesSortFn)( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam );

static int CALLBACK SortByName( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	return strcmp( pInfo1->m_ComputerName, pInfo2->m_ComputerName );
}

static int CALLBACK SortByStatus( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	int ret = strcmp( GetStatusString( pInfo2 ), GetStatusString( pInfo1 ) );
	if ( ret == 0 )
		return SortByName( iItem1, iItem2, lpParam );
	else
		return ret;
}

static int CALLBACK SortByRunningTime( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	return pInfo2->m_LiveTimeMS > pInfo1->m_LiveTimeMS;
}

static int CALLBACK SortByMasterName( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	int ret = strcmp( pInfo2->m_MasterName, pInfo1->m_MasterName );
	if ( ret == 0 )
		return SortByName( iItem1, iItem2, lpParam );
	else
		return ret;
}

// --------------------------------------------------------------------------------------------------------- //
// Column information.
// --------------------------------------------------------------------------------------------------------- //

int g_iSortColumn = 0;

struct
{
	char			*pText;
	int				width;
	ServicesSortFn	sortFn;
} g_ColumnInfos[] =
{
	{"Computer Name", 150, SortByName},
	{"Status", 60, SortByStatus},
	{"Running Time", 100, SortByRunningTime},
	{"Master", 150, SortByMasterName}
};
#define COLUMN_COMPUTER_NAME	0
#define COLUMN_STATUS			1
#define COLUMN_RUNNING_TIME		2
#define COLUMN_MASTER_NAME		3


/////////////////////////////////////////////////////////////////////////////
// CServicesDlg dialog


CServicesDlg::CServicesDlg(CWnd* pParent /*=NULL*/)
	: CIdleDialog(CServicesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServicesDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pServicesPingSocket = NULL;
	m_nServices = 0;
}


void CServicesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServicesDlg)
	DDX_Control(pDX, IDC_NUM_SERVICES, m_NumServicesControl);
	DDX_Control(pDX, IDC_CURRENT_PASSWORD, m_PasswordDisplay);
	DDX_Control(pDX, IDC_SERVICES_LIST, m_ServicesList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServicesDlg, CIdleDialog)
	//{{AFX_MSG_MAP(CServicesDlg)
	ON_BN_CLICKED(ID_PATCH_SERVICES, OnPatchServices)
	ON_BN_CLICKED(ID_STOP_SERVICES, OnStopServices)
	ON_BN_CLICKED(ID_STOP_JOBS, OnStopJobs)
	ON_BN_CLICKED(ID_CHANGE_PASSWORD, OnChangePassword)
	ON_NOTIFY(NM_DBLCLK, IDC_SERVICES_LIST, OnDblclkServicesList)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServicesDlg message handlers

BOOL CServicesDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();


	m_ServicesList.SetExtendedStyle( LVS_EX_FULLROWSELECT );
	
	// Setup the headers.
	for ( int i=0; i < ARRAYSIZE( g_ColumnInfos ); i++ )
	{
		m_ServicesList.InsertColumn( i, g_ColumnInfos[i].pText, LVCFMT_LEFT, g_ColumnInfos[i].width, i );
	}
	
	m_pServicesPingSocket = CreateIPSocket();
	if ( m_pServicesPingSocket )
	{
		m_pServicesPingSocket->BindToAny( 0 );
	}

	m_dwLastServicesPing = GetTickCount() - SERVICES_PING_INTERVAL;
	StartIdleProcessing( 100 );	// get idle messages every half second

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_SERVICES_LABEL ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_SERVICES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_CURRENT_PASSWORD_LABEL ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_CURRENT_PASSWORD ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_FLUSH_CACHE ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_PATCH_SERVICES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_STOP_SERVICES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_STOP_JOBS ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_CHANGE_PASSWORD ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_SERVICES_LIST ), ANCHOR_LEFT, ANCHOR_TOP, ANCHOR_RIGHT, ANCHOR_BOTTOM );

	// Sort by name to start with..
	g_iSortColumn = 0;

	// Unless they specify admin mode, hide all the controls that can mess with the services.
	if ( !FindArg( "-Admin" ) )
	{
		::ShowWindow( ::GetDlgItem( m_hWnd, ID_PATCH_SERVICES ), SW_HIDE );
		::ShowWindow( ::GetDlgItem( m_hWnd, ID_STOP_SERVICES ), SW_HIDE );
		::ShowWindow( ::GetDlgItem( m_hWnd, IDC_CURRENT_PASSWORD_LABEL ), SW_HIDE );
		::ShowWindow( ::GetDlgItem( m_hWnd, ID_STOP_JOBS ), SW_HIDE );
		::ShowWindow( ::GetDlgItem( m_hWnd, ID_CHANGE_PASSWORD ), SW_HIDE );
		::ShowWindow( ::GetDlgItem( m_hWnd, ID_FLUSH_CACHE ), SW_HIDE );
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CServicesDlg::BuildVMPIPingPacket( CUtlVector<char> &out, char cPacketID )
{
	out.Purge();
	out.AddToTail( VMPI_PROTOCOL_VERSION );
	
	const char *pPassword = m_Password;
	out.AddMultipleToTail( strlen( pPassword ) + 1, pPassword );	// password.
	
	out.AddToTail( cPacketID );
}


void CServicesDlg::OnIdle()
{
	DWORD curTime = GetTickCount();

	if ( !m_pServicesPingSocket )
		return;

	// Broadcast out to all the services?
	if ( curTime - m_dwLastServicesPing >= SERVICES_PING_INTERVAL )
	{
		m_dwLastServicesPing = curTime;
	
		for ( int i=VMPI_SERVICE_PORT; i <= VMPI_LAST_SERVICE_PORT; i++ )
		{
			CUtlVector<char> data;
			BuildVMPIPingPacket( data, VMPI_PING_REQUEST );
			m_pServicesPingSocket->Broadcast( data.Base(), data.Count(), i );
		}
	}

	// Check for messages from services.
	while ( 1 )
	{
		char in[1024];
		CIPAddr ipFrom;

		int len = m_pServicesPingSocket->RecvFrom( in, sizeof( in ), &ipFrom );
		if ( len < 4 )
			break;

		bf_read buf( in, len );
		if ( buf.ReadByte() == VMPI_PROTOCOL_VERSION )
		{
			int packetID = buf.ReadByte();
			if ( packetID == VMPI_PING_RESPONSE )
			{
				int iState = buf.ReadByte();
				unsigned long liveTimeMS = (unsigned long)buf.ReadLong();

				int iPort = buf.ReadLong();
				char computerName[512];
				buf.ReadString( computerName, sizeof( computerName ) );

				char masterName[512];
				if ( buf.GetNumBitsLeft() )
					buf.ReadString( masterName, sizeof( masterName ) );
				else
					masterName[0] = 0;

				CServiceInfo *pInfo = FindServiceByComputerName( computerName );
				if ( !pInfo )
				{
					pInfo = new CServiceInfo;
					m_Services.AddToTail( pInfo );
					pInfo->m_ComputerName = computerName;

					pInfo->m_pLastStatusText = NULL;
					
					pInfo->m_Addr = ipFrom;
					pInfo->m_Addr.port = iPort;
					pInfo->m_LastPingTimeMS = Plat_MSTime();
					pInfo->m_MasterName = "?";

					int iItem = m_ServicesList.InsertItem( COLUMN_COMPUTER_NAME, pInfo->m_ComputerName, NULL );
					m_ServicesList.SetItemData( iItem, (DWORD)pInfo );

					// Update the display of # of services.
					m_nServices++;
					UpdateServiceCountDisplay();
				}
				
				pInfo->m_MasterName = masterName;
				pInfo->m_LiveTimeMS = liveTimeMS;
				pInfo->m_iState = iState;
				pInfo->m_LastPingTimeMS = Plat_MSTime();

				UpdateServiceInListbox( pInfo );
			}
		}
	}

	FOR_EACH_LL( m_Services, iService )
	{
		CServiceInfo *pInfo = m_Services[iService];
		if ( Plat_MSTime() - pInfo->m_LastUpdateTime > SERVICE_MAX_UPDATE_INTERVAL )
		{
			UpdateServiceInListbox( pInfo );
		}
	}

	if ( m_bListChanged )
	{
		ResortItems();
		m_bListChanged = false;
	}
}


CServiceInfo* CServicesDlg::FindServiceByComputerName( const char *pComputerName )
{
	FOR_EACH_LL( m_Services, i )
	{
		if ( Q_stricmp( m_Services[i]->m_ComputerName, pComputerName ) == 0 )
			return m_Services[i];
	}
	return NULL;
}


void CServicesDlg::SendToSelectedServices( const char *pData, int len )
{
	POSITION pos = m_ServicesList.GetFirstSelectedItemPosition();
	while ( pos )
	{
		int iItem = m_ServicesList.GetNextSelectedItem( pos );

		CServiceInfo *pInfo = (CServiceInfo*)m_ServicesList.GetItemData( iItem );
		m_pServicesPingSocket->SendTo( &pInfo->m_Addr, pData, len );
	}
}


void CServicesDlg::UpdateServiceInListbox( CServiceInfo *pInfo )
{
	// First, find this item in the listbox.
	LVFINDINFO info;
	info.flags = LVFI_PARAM;
	info.lParam = (LPARAM)pInfo;
	int iItem = m_ServicesList.FindItem( &info );
	if ( iItem != -1 )
	{
		m_ServicesList.SetItemText( iItem, COLUMN_COMPUTER_NAME, pInfo->m_ComputerName );
		
		const char *pText = GetStatusString( pInfo );
		m_ServicesList.SetItemText( iItem, COLUMN_STATUS, pText );

		char timeStr[512];
		FormatTimeString( pInfo->m_LiveTimeMS / 1000, timeStr, sizeof( timeStr ) );

		m_ServicesList.SetItemText( iItem, COLUMN_RUNNING_TIME, timeStr );
		m_ServicesList.SetItemText( iItem, COLUMN_MASTER_NAME, pInfo->m_MasterName );

		// Detect changes.
		if ( pText != pInfo->m_pLastStatusText || 
			pInfo->m_LiveTimeMS != pInfo->m_LastLiveTimeMS ||
			pInfo->m_MasterName != pInfo->m_LastMasterName )
		{
			m_bListChanged = true;
		}
			
		pInfo->m_pLastStatusText = pText;
		pInfo->m_LastLiveTimeMS = pInfo->m_LiveTimeMS;
		pInfo->m_LastMasterName = pInfo->m_MasterName;
		pInfo->m_LastUpdateTime = Plat_MSTime();
	}
}


void CServicesDlg::ResortItems()
{
	m_ServicesList.SortItems( g_ColumnInfos[g_iSortColumn].sortFn, (LPARAM)this );
}


void CServicesDlg::UpdateServiceCountDisplay()
{
	CString str;
	str.Format( "%d", m_nServices );
	m_NumServicesControl.SetWindowText( str );
}


void CServicesDlg::OnPatchServices() 
{
	CUtlVector<char> data;

	// Inquire about the timeout.
	CPatchTimeout dlg;
	dlg.m_Timeout = 30;

	if ( dlg.DoModal() == IDOK )
	{
		BuildVMPIPingPacket( data, VMPI_SERVICE_PATCH );
		data.AddMultipleToTail( sizeof( dlg.m_Timeout ), (char*)&dlg.m_Timeout );

		SendToSelectedServices( data.Base(), data.Count() );
	}
}


void CServicesDlg::OnStopServices() 
{
	if ( MessageBox( "Warning: if you stop these services, you won't be able to control them from this application, and must restart them manually. Contine?", "Warning", MB_YESNO ) == IDYES )
	{
		CUtlVector<char> data;
		BuildVMPIPingPacket( data, VMPI_STOP_SERVICE );
		SendToSelectedServices( data.Base(), data.Count() );
	}
}

void CServicesDlg::OnStopJobs() 
{
	CUtlVector<char> data;
	BuildVMPIPingPacket( data, VMPI_KILL_PROCESS );
	SendToSelectedServices( data.Base(), data.Count() );
}


void CServicesDlg::OnChangePassword() 
{
	CSetPasswordDlg dlg;
	dlg.m_Password = m_Password;

	if ( dlg.DoModal() == IDOK )
	{
		m_Password = dlg.m_Password;
		m_PasswordDisplay.SetWindowText( m_Password );

		m_Services.PurgeAndDeleteElements();
		m_ServicesList.DeleteAllItems();

		m_nServices = 0;
		UpdateServiceCountDisplay();

		// Re-ping everyone immediately.
		m_dwLastServicesPing = GetTickCount() - SERVICES_PING_INTERVAL;
	}	
}

void CServicesDlg::OnDblclkServicesList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	POSITION pos = m_ServicesList.GetFirstSelectedItemPosition();
	if ( pos )
	{
		int iItem = m_ServicesList.GetNextSelectedItem( pos );
		if ( iItem != -1 )
		{
			CServiceInfo *pInfo = (CServiceInfo*)m_ServicesList.GetItemData( iItem );
			if ( pInfo )
			{
				// Launch vmpi_browser_job_search and have it auto-select this worker.
				CString cmdLine;
				cmdLine.Format( "vmpi_browser_job_search -SelectWorker %s", pInfo->m_ComputerName );

				STARTUPINFO si;
				memset( &si, 0, sizeof( si ) );
				si.cb = sizeof( si );

				PROCESS_INFORMATION pi;
				memset( &pi, 0, sizeof( pi ) );

				if ( !CreateProcess( 
					NULL, 
					(char*)(const char*)cmdLine, 
					NULL,							// security
					NULL,
					TRUE,
					0,			// flags
					NULL,							// environment
					NULL,							// current directory
					&si,
					&pi ) )
				{
					CString err;
					err.Format( "Can't run '%s'", (LPCTSTR)cmdLine );
					MessageBox( err, "Error", MB_OK );
				}
			}			
		}
	}
	
	*pResult = 0;
}

void CServicesDlg::OnSize(UINT nType, int cx, int cy) 
{
	CIdleDialog::OnSize(nType, cx, cy);
	
	m_AnchorMgr.UpdateAnchors( this );	
}


BOOL CServicesDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR *pHdr = (NMHDR*)lParam;
	if ( pHdr->idFrom == IDC_SERVICES_LIST )
	{
		if ( pHdr->code == LVN_COLUMNCLICK )
		{
			LPNMLISTVIEW pListView = (LPNMLISTVIEW)lParam;

			// Now sort by this column.
			g_iSortColumn = max( 0, min( pListView->iSubItem, ARRAYSIZE( g_ColumnInfos ) - 1 ) );
			ResortItems();
		}
	}

	return CIdleDialog::OnNotify(wParam, lParam, pResult);
}
