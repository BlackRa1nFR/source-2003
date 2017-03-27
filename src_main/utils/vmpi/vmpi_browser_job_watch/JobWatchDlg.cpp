// JobWatchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JobWatchDlg.h"
#include "vstdlib/strtools.h"
#include "consolewnd.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJobWatchDlg dialog


CJobWatchDlg::CJobWatchDlg(CWnd* pParent /*=NULL*/)
	: CIdleDialog(CJobWatchDlg::IDD, pParent)
{
	m_CurMessageIndex = 0;
	m_CurGraphTime = 0;
	m_pSQL = NULL;

	//{{AFX_DATA_INIT(CJobWatchDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


CJobWatchDlg::~CJobWatchDlg()
{
	if ( m_pSQL )
		m_pSQL->Release();
}


void CJobWatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJobWatchDlg)
	DDX_Control(pDX, IDC_WORKERS, m_Workers);
	DDX_Control(pDX, IDC_TEXTOUTPUT, m_TextOutput);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CJobWatchDlg, CIdleDialog)
	//{{AFX_MSG_MAP(CJobWatchDlg)
	ON_LBN_SELCHANGE(IDC_WORKERS, OnSelChangeWorkers)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJobWatchDlg message handlers

const char* FindArg( const char *pArgName, const char *pDefault="" )
{
	for ( int i=1; i < __argc; i++ )
	{
		if ( Q_stricmp( pArgName, __argv[i] ) == 0 )
		{
			if ( (i+1) < __argc )
				return __argv[i+1];
			else
				return pDefault;
		}
	}
	return NULL;
}

BOOL CJobWatchDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();


	m_GraphControl.SubclassDlgItem( IDC_GRAPH_AREA, this );


	CString str;

	// Get all our startup info from the command line.
	const char *pJobID = FindArg( "-JobID", NULL );
	const char *pDBName = FindArg( "-dbname", NULL );
	const char *pHostName = FindArg( "-hostname", NULL );
	const char *pUserName = FindArg( "-username", NULL );

	if ( !pJobID || !pDBName || !pHostName || !pUserName )
	{
		str.Format( "Missing a command line parameter (-JobID or -dbname or -hostname or -username)" );
		MessageBox( str, "Error", MB_OK );
		EndDialog( 1 );
		return FALSE;
	}

	m_JobID = atoi( pJobID );
	
	m_pSQL = InitMySQL( pDBName, pHostName, pUserName );
	if ( !m_pSQL )
	{
		str.Format( "Can't init MYSQL db (db = '%s', host = '%s', user = '%s')", pDBName, pHostName, pUserName );
		MessageBox( str, "Error", MB_OK );
		EndDialog( 0 );
		return FALSE;
	}
	
	
	// Fill the workers list.
	CMySQLQuery query;
	query.Format( "select * from job_worker_start where JobID=%lu order by MachineName", m_JobID );
	GetMySQL()->Execute( query );

	while ( GetMySQL()->NextRow() )
	{
		const char *pMachineName = GetMySQL()->GetColumnValue( "MachineName" ).String();

		int index;
		if ( GetMySQL()->GetColumnValue( "IsMaster" ).Int32() )
		{
			char tempStr[512];
			Q_snprintf( tempStr, sizeof( tempStr ), "%s (master)", pMachineName );
			index = m_Workers.AddString( tempStr );
		}
		else
		{
			index = m_Workers.AddString( pMachineName );
		}

		unsigned long jobWorkerID = GetMySQL()->GetColumnValue( "JobWorkerID" ).UInt32();
		m_Workers.SetItemData( index, jobWorkerID );
	}
	

	// (Init the idle processor so we can update text and graphs).
	StartIdleProcessing( 300 );


	// Setup anchors.
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_WORKERS_PANEL ), ANCHOR_LEFT, ANCHOR_TOP, ANCHOR_WIDTH_PERCENT, ANCHOR_HEIGHT_PERCENT );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_WORKERS ), ANCHOR_LEFT, ANCHOR_TOP, ANCHOR_WIDTH_PERCENT, ANCHOR_HEIGHT_PERCENT );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_TEXT_OUTPUT_PANEL ), ANCHOR_LEFT, ANCHOR_HEIGHT_PERCENT, ANCHOR_WIDTH_PERCENT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_TEXTOUTPUT ), ANCHOR_LEFT, ANCHOR_HEIGHT_PERCENT, ANCHOR_WIDTH_PERCENT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_GRAPHS_PANEL ), ANCHOR_WIDTH_PERCENT, ANCHOR_TOP, ANCHOR_RIGHT, ANCHOR_HEIGHT_PERCENT );
	m_AnchorMgr.AddAnchor( this, &m_GraphControl, ANCHOR_WIDTH_PERCENT, ANCHOR_TOP, ANCHOR_RIGHT, ANCHOR_HEIGHT_PERCENT );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


bool CJobWatchDlg::GetCurJobWorkerID( unsigned long &id )
{
	int curSel = m_Workers.GetCurSel();
	if ( curSel == LB_ERR )
		return false;

	id = m_Workers.GetItemData( curSel );
	return id != LB_ERR;
}


void CJobWatchDlg::OnSelChangeWorkers() 
{
	CUtlVector<char> text;

	m_CurMessageIndex = 0;
	GetCurrentWorkerText( text, m_CurMessageIndex );

	int nLen = m_TextOutput.SendMessage( EM_GETLIMITTEXT, 0, 0 );
	m_TextOutput.SendMessage( EM_SETSEL, 0, nLen );
	m_TextOutput.SendMessage( EM_REPLACESEL, FALSE, (LPARAM)"" );

	FormatAndSendToEditControl( m_TextOutput.GetSafeHwnd(), text.Base() );

	m_GraphControl.Clear();
	m_CurGraphTime = -1;
	FillGraph();
}


void CJobWatchDlg::OnIdle()
{
	CUtlVector<char> text;

	// Any new text? This would indicate that the process is running currently.
	if ( GetCurrentWorkerText( text, m_CurMessageIndex ) && text.Count() > 0 )
	{
		FormatAndSendToEditControl( m_TextOutput.GetSafeHwnd(), text.Base() );
	}

	FillGraph();
}


bool CJobWatchDlg::GetCurrentWorkerText( 
	CUtlVector<char> &text, 
	unsigned long &curMessageIndex )
{
	text.SetSize( 0 );

	unsigned long jobWorkerID;
	if ( !GetCurJobWorkerID( jobWorkerID ) )
		return false;

	// Now copy all the text out.
	CMySQLQuery query;
	if ( curMessageIndex == 0 )
		query.Format( "select * from text_messages where JobWorkerID=%lu", jobWorkerID );
	else
		query.Format( "select * from text_messages where JobWorkerID=%lu and MessageIndex >= %lu", jobWorkerID, curMessageIndex );
	
	GetMySQL()->Execute( query );

	while ( GetMySQL()->NextRow() )
	{
		const char *pTextStr = GetMySQL()->GetColumnValue( "text" ).String();
		int len = strlen( pTextStr );
		text.AddMultipleToTail( len, pTextStr );

		curMessageIndex = GetMySQL()->GetColumnValue( "MessageIndex" ).UInt32() + 1;
	}

	text.AddToTail( 0 );
	return true;
}


void CJobWatchDlg::FillGraph()
{
	// Get all the graph samples.
	unsigned long jobWorkerID;
	if ( !GetCurJobWorkerID( jobWorkerID ) )
		return;

	CMySQLQuery query;
	query.Format( "select * from graph_entry where JobWorkerID=%lu", jobWorkerID );
	GetMySQL()->Execute( query );

	int iMSTime = GetMySQL()->GetColumnIndex( "MSSinceJobStart" );	
	int iBytesSent = GetMySQL()->GetColumnIndex( "BytesSent" );
	int iBytesReceived = GetMySQL()->GetColumnIndex( "BytesReceived" );

	// See if there's anything new.
	CUtlVector<CGraphEntry> entries;

	int highest = m_CurGraphTime;
	while ( GetMySQL()->NextRow() )
	{
		CGraphEntry entry;
		entry.m_msTime = GetMySQL()->GetColumnValue( iMSTime ).Int32();
		entry.m_nBytesSent = GetMySQL()->GetColumnValue( iBytesSent ).Int32();
		entry.m_nBytesReceived = GetMySQL()->GetColumnValue( iBytesReceived ).Int32();
		entries.AddToTail( entry );

		highest = max( highest, entry.m_msTime );
	}

	if ( highest > m_CurGraphTime )
	{
		m_CurGraphTime = highest;
		
		m_GraphControl.Clear();
		m_GraphControl.Fill( entries );
	}
}



void CJobWatchDlg::OnSize(UINT nType, int cx, int cy) 
{
	CIdleDialog::OnSize(nType, cx, cy);
	
	m_AnchorMgr.UpdateAnchors( this );	
}
