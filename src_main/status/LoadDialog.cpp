// LoadDialog.cpp : implementation file
//

#include "stdafx.h"
#include <afxcmn.h>
#include "status.h"
#include "LoadDialog.h"
#include "StatusDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoadDialog dialog


CLoadDialog::CLoadDialog(CStatusDlg* pParent /*=NULL*/)
	: CDialog(CLoadDialog::IDD, pParent)
{
	m_pStatus = pParent;
	ASSERT( m_pStatus );

	//{{AFX_DATA_INIT(CLoadDialog)
	m_strEndDate = _T("");
	m_strStartDate = _T("");
	m_strThreshold = _T("");
	m_strModName = _T("");
	//}}AFX_DATA_INIT

	time_t t = m_pStatus->GetTime();
	struct tm *tval;

	tval = localtime( &t );
	char sz[ 128 ];

	sprintf( sz, "%i/%i/%i",
		tval->tm_mon + 1, tval->tm_mday, tval->tm_year + 1900 );

	m_strStartDate = sz;
	m_strEndDate = sz;

	m_nFlags = 0;
}


void CLoadDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoadDialog)
	DDX_Control(pDX, IDC_STATIC_NUMFILES, m_staticNumFiles);
	DDX_Text(pDX, IDC_ENDDATE, m_strEndDate);
	DDX_Text(pDX, IDC_STARTDATE, m_strStartDate);
	DDX_Text(pDX, IDC_THRESHOLD, m_strThreshold);
	DDX_Text(pDX, IDC_MODNAME, m_strModName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoadDialog, CDialog)
	//{{AFX_MSG_MAP(CLoadDialog)
	ON_BN_CLICKED(IDC_CHOOSEFILES, OnChoosefiles)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoadDialog message handlers

void CLoadDialog::OnChoosefiles() 
{
	// TODO: Add your control notification handler code here
	
}

BOOL CLoadDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	CButton *radio = static_cast< CButton * >( GetDlgItem( IDC_LOADBYDATE ) );
	if ( radio )
	{
		radio->SetCheck( 1 );
	}

	radio = static_cast< CButton * >( GetDlgItem( IDC_LOADALL ) );
	if ( radio )
	{
		radio->SetCheck( 1 );
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int CLoadDialog::GetFlags( void )
{
	return m_nFlags;
}

int CLoadDialog::DetermineFlags( void )
{
	int flags = CStatusDlg::NORMAL;

	CButton *radioNormal = static_cast< CButton * >( GetDlgItem( IDC_LOADALL ) );
	CButton *radioIgnoreMods = static_cast< CButton * >( GetDlgItem( IDC_IGNOREMODS ) );
	CButton *radioThreshold = static_cast< CButton * >( GetDlgItem( IDC_USETHRESHOLD ) );
	CButton *radioSpecificMod = static_cast< CButton * >( GetDlgItem( IDC_SPECIFICMOD ) );

	if ( radioIgnoreMods->GetCheck() == 1 )
	{
		flags = CStatusDlg::IGNOREMODS;
	}
	else if ( radioThreshold->GetCheck() == 1 )
	{
		// Get value from edit field
		float threshold = (float)atof( ( char * )( LPCSTR )m_strThreshold );
		if ( threshold > 0.0001f )
		{
			flags = CStatusDlg::USETHRESHOLD;
		}
	}
	else if ( radioSpecificMod->GetCheck() == 1 )
	{
		if ( !m_strModName.IsEmpty() )
		{
			flags = CStatusDlg::SPECIFICMOD;
		}
	}

	return flags;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CLoadDialog::GetNumFiles( void )
{
	return m_FileNames.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CLoadDialog::GetFile( int index )
{
	if ( index < 0 || index >= GetNumFiles() )
	{
		return "";
	}

	return m_FileNames[ index ].fn;
}

static bool ComposeDate( struct tm *out, int m, int d, int y )
{
	memset( out, 0, sizeof( *out ) );

	if ( m < 1 || m > 12 )
		return false;

	if ( d < 1 || d > 31 )
		return false;

	if ( y < 1980 || y > 2100 )
		return false;

	out->tm_year = y - 1900;
	out->tm_mday = d;
	out->tm_mon = m - 1;

	return true;
}

#define FSECONDS_PER_DAY 86400.0
#define FIVE_YEARS ( FSECONDS_PER_DAY * 365.25 * 5.0 )

bool SuckTimeFromString( char const *in, int *m, int *d, int *y )
{
	if ( !in || !m || !d || !y )
		return false;

	char temp[ 32 ];
	char *p = (char *)in;
	char *out = temp;

	while ( *p && *p != '/' && *p != '\\' )
	{
		*out++ = *p++;
	}
	*out = 0;

	*m = atoi( temp );

	if ( !*p )
		return false;

	out = temp;
	p++;

	while ( *p && *p != '/' && *p != '\\' )
	{
		*out++ = *p++;
	}
	*out = 0;

	*d = atoi( temp );

	if ( !*p )
		return false;

	out = temp;
	p++;	

	while ( *p && *p != '/' && *p != '\\' )
	{
		*out++ = *p++;
	}
	*out = 0;

	*y = atoi( temp );

	return true;
}

void CLoadDialog::OnOK() 
{
	UpdateData( TRUE );

	m_nFlags = DetermineFlags();

	// Create list of filenames, if possible
	CButton *radioByDate = static_cast< CButton * >( GetDlgItem( IDC_LOADBYDATE ) );
	if ( radioByDate && radioByDate->GetCheck() == 1 )
	{
		m_FileNames.RemoveAll();

		// Use dates to determine filename, otherwise, clicking the "choose files button probably
		//  created a list for us

		char const *starttime = m_strStartDate;
		char const *endtime = m_strEndDate;

		int m1, d1, y1;
		int m2, d2, y2;

		if ( SuckTimeFromString( starttime, &m1, &d1, &y1 ) &&
			SuckTimeFromString( endtime, &m2, &d2, &y2 ) )
		{
			struct tm t1;
			struct tm t2;
			
			if ( ComposeDate( &t1, m1, d1, y1 ) &&
				ComposeDate( &t2, m2, d2, y2 ) )
			{
				time_t start = mktime( &t1 );
				time_t end = mktime( &t2 );
				
				if ( start > end )
				{
					time_t temp = start;
					start = end;
					end = temp;
				}
				
				if ( difftime( end, start ) < FIVE_YEARS )
				{
					// Create file list
					m_FileNames.RemoveAll();
					
					for ( time_t t = start; 
					t <= (time_t)( end + FSECONDS_PER_DAY ) || m_pStatus->IsInSameDay( t, end ) ; 
					t += (int)FSECONDS_PER_DAY )
					{
						char const *fname = m_pStatus->FileNameForTime( t );
						ASSERT( fname );
						int i = m_FileNames.AddToTail();
						ASSERT( m_FileNames.IsValidIndex( i ) );
						FilenameList *entry = &m_FileNames[ i ];
						strncpy( entry->fn, fname, sizeof( entry->fn ) );
						entry->fn[ sizeof( entry->fn ) - 1 ] = 0;
					}
					
					char sz[ 512 ];
					sprintf( sz, "(%i) files in list.", GetNumFiles() );
					
					m_staticNumFiles.SetWindowText( sz );
					
				}
			}
		}
	}

	CDialog::OnOK();
}
