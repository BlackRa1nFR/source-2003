// Graph.cpp : implementation file
//
#include <assert.h>
#include "stdafx.h"
#include "status.h"
#include "modupdate.h"
#include "Graph.h"
#include "ODStatic.h"
#include "mod.h"
#include "util.h"
#include "status_colors.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern char *game_name;

// A mod must have this many players to show up on the flyover stats list ( 0.5% of max )
#define PRINT_THRESHOLD_SERVERS	40
#define PRINT_THRESHOLD_PLAYERS 100

// Granularity of horizontal scroll bars
#define MAX_SCROLL_POS	1000
// Number of pixels from right side of window that graph actuall starts
#define RIGHT_EDGE_GAP	4
// HELPERS

/*
==============================
DrawDot

Draws a dot, centered at x, y, having radius of gap, using the specified COLORREF
==============================
*/
void DrawDot( CDC& dc, COLORREF clr, int gap, int x, int y )
{
	CPen *old;
	CPen pen( PS_SOLID, 1, clr );
	old = dc.SelectObject( &pen );
	CBrush br( RGB( 0, 0, 0 ) );
	CBrush *oldbr;

	oldbr = dc.SelectObject( &br);

	dc.Ellipse( x- gap, y - gap, x + gap + 1, y + gap + 1 );

	dc.SelectObject( oldbr );
	dc.SelectObject( old );
}

/*
==============================
ComputeSuffix

==============================
*/
void ComputeSuffix( double *time, char *suffix )
{
	if ( *time > 1000000 )
	{
		sprintf( suffix, "M" );
		*time /= 1000000.0;
	}
	else if ( *time > 1000 )
	{
		sprintf( suffix, "K" );
		*time /= 1000.0;
	}
	else
	{
		sprintf( suffix, "" );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGraph

CGraph::CGraph( char *title )
{
	m_pUpdates		= NULL;
	m_gtType		= GRAPHTYPE_PLAYERS;
	strcpy( m_szTitle, title );
	m_bButtonDown	= false;

	m_pStatus		= NULL;
	m_pHistory		= NULL;

	SetScale( 100 );
}

void CGraph::SetScale( int scale )
{
	m_nScale = scale;
}

float CGraph::GetScale( void )
{
	return (float)m_nScale / 100.0f;
}


/*
==============================
SetData

Reset data pointers and scale factors
==============================
*/
void CGraph::SetData( CModStats *mod, int scale, GraphType type )
{
	m_gtType		= type;
	m_pUpdates		= mod;
	m_yscale		= scale;
	m_pHistory		= NULL;
}

bool CGraph::IsPlayerGraph( void )
{
	return m_gtType == GRAPHTYPE_PLAYERS ? true : false;
}

/*
==============================
~CGraph

==============================
*/
CGraph::~CGraph()
{
	if ( m_pStatus )
	{
		delete m_pStatus;
		m_pStatus = NULL;
	}
}

BEGIN_MESSAGE_MAP(CGraph, CWnd)
	//{{AFX_MSG_MAP(CGraph)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOVE()
	ON_WM_HSCROLL()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGraph message handlers

void CGraph::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	RECT rcClient;
	GetClientRect( &rcClient );

	CDC memdc;
	memdc.CreateCompatibleDC( &dc );

	CBitmap *oldbm, bm;

	int w = rcClient.right - rcClient.left;
	int h = rcClient.bottom - rcClient.top;
	bm.CreateCompatibleBitmap( &dc, w, h );

	oldbm = memdc.SelectObject( &bm );

	Redraw( &memdc );

	BitBlt( dc, 0, 0, w, h, memdc, 0, 0, SRCCOPY );

	memdc.SelectObject( oldbm );

	bm.DeleteObject();
	memdc.DeleteDC();

	ValidateRect( NULL );

}

int CGraph::GetStatHeight( CModUpdate *p, int statnum, GraphLineType type )
{
	if ( statnum < 0 || 
		statnum > MOD_STATCOUNT )
	{
		return 0;
	}

	if ( type == GRAPHLINE_NORMAL )
	{
		// Total
		if ( statnum == MOD_STATCOUNT )
		{
			return IsPlayerGraph() ? (p->total.players)  : p->total.servers;
		}

		return IsPlayerGraph() ? p->stats[ statnum ].players : p->stats[ statnum ].servers;
	}
	else
	{
		if ( statnum == MOD_STATCOUNT )
		{
			return IsPlayerGraph() ? p->total.bots : p->total.bots_servers;
		}
		else
		{
			return IsPlayerGraph() ? p->stats[ statnum ].bots :  p->stats[ statnum ].bots_servers;
		}
	}
}

COLORREF CGraph::GetStatColor( int statnum, GraphLineType type )
{
	COLORREF clr;
	
	switch ( statnum )
	{
	case MOD_STATCOUNT:
		if ( type == GRAPHLINE_NORMAL )
		{
			clr = RGB( 50, 50, 255 );
		}
		else
		{
			clr = RGB( 45, 230, 230 );
		}
		break;
	case MOD_INTERNET:
		if ( type == GRAPHLINE_NORMAL )
		{
			clr = RGB( 150, 50, 150 );
		}
		else
		{
			clr = RGB( 245, 85, 245 );
		}
		break;
	case MOD_LAN:
		if ( type == GRAPHLINE_NORMAL )
		{
			clr = RGB( 200, 50, 50 );
		}
		else
		{
			clr = RGB( 225, 230, 100 );
		}	
		break;
	case MOD_PROXY:
	default:	
		clr = RGB( 50, 200, 50 );
		break;
	}

	return clr;
}

void CGraph::DrawLegend( int y, CDC& dc, CFont& fnt )
{
	char sz[ 128 ];

	y += 10;

	CFont *of = dc.SelectObject( &fnt );

	dc.SetTextColor( GetStatColor( MOD_STATCOUNT, GRAPHLINE_NORMAL ) );
	sprintf( sz, "total" );
	dc.TextOut( 5, y, sz, strlen( sz ) );
	y += 10;

	dc.SetTextColor( GetStatColor( MOD_INTERNET, GRAPHLINE_NORMAL ) );
	sprintf( sz, "internet" );
	dc.TextOut( 5, y, sz, strlen( sz ) );
	y += 10;

	dc.SetTextColor( GetStatColor( MOD_LAN , GRAPHLINE_NORMAL ) );
	sprintf( sz, "lan" );
	dc.TextOut( 5, y, sz, strlen( sz ) );
	y += 10;
	
	dc.SetTextColor( GetStatColor( MOD_PROXY , GRAPHLINE_NORMAL ) );
	sprintf( sz, "proxy" );
	dc.TextOut( 5, y, sz, strlen( sz ) );
	y += 10;

	dc.SetTextColor( GetStatColor( MOD_STATCOUNT, GRAPHLINE_BOTS ) );
	sprintf( sz, "total bot" );
	dc.TextOut( 5, y, sz, strlen( sz ) );
	y += 10;

	dc.SetTextColor( GetStatColor( MOD_INTERNET, GRAPHLINE_BOTS ) );
	sprintf( sz, "internet bot" );
	dc.TextOut( 5, y, sz, strlen( sz ) );
	y += 10;

	dc.SetTextColor( GetStatColor( MOD_LAN, GRAPHLINE_BOTS ) );
	sprintf( sz, "lan bot" );
	dc.TextOut( 5, y, sz, strlen( sz ) );
	y += 10;


	dc.SelectObject( of );
}

const char *CGraph::PrintBreakdowns( CModUpdate *p )
{
	return PrintBreakdowns( p->stats );
}

const char *CGraph::PrintBreakdowns( CMod *p )
{
	return PrintBreakdowns( p->stats );
}

const char *CGraph::PrintBreakdowns( CStat *stats )
{
	static char string[8][1024];
	static int last = 0;

	char temp[ 128 ];

	last &= 0x7;

	string[ last ][ 0 ] = 0;

	for ( int i = 0; i < MOD_STATCOUNT; i++ )
	{
		if ( i != MOD_PROXY )
		{
			if ( IsPlayerGraph()  )
			{
				sprintf( temp, "%8i %s %8i bt", stats[ i ].players , g_StatNames[ i ], stats[ i ].bots);
			}
			else
			{
				sprintf( temp, "%8i %s %8i bs", stats[ i ].servers, g_StatNames[ i ] , stats[ i ].bots_servers);
			}
		}
		else
		{
				sprintf( temp, "%8i %s", stats[ i ].servers, g_StatNames[ i ]);
		}

		strcat( string[ last ], temp );

		if ( i != MOD_STATCOUNT - 1 )
		{
			strcat( string[ last ], " /" );
		}
	}

	return string[ last++ ];
}

/*
==============================
Redraw

==============================
*/
void CGraph::Redraw( CDC *pDC )
{
	CModUpdate *p;
	int x, y, h;
	CRect rcClient;
	CBitmap *pOldBitmap, bm;
	CPen penhash( PS_SOLID, 1, RGB( 0, 0, 0 ) );
	CPen penstats( PS_SOLID, 1, RGB( 150, 150, 150 ) );
	CDC dc;
	CFont fnt;
	fnt.CreateFont(
		 -11					         // H
		, 0   					         // W
		, 0								 // Escapement
		, 0							     // Orient
		, FW_NORMAL						 // Wt. 
		, 0								 // Ital.
		, 0             				 // Underl.
		, 0                 			 // SO
		, ANSI_CHARSET		    		 // Charset
		, OUT_TT_PRECIS					 // Out prec.
		, CLIP_DEFAULT_PRECIS			 // Clip prec.
		, PROOF_QUALITY   			     // Qual.
		, VARIABLE_PITCH | FF_DONTCARE   // Pitch and Fam.
		, "Courier New" );

	CFont boldfnt;
	boldfnt.CreateFont(
		 -11					         // H
		, 0   					         // W
		, 0								 // Escapement
		, 0							     // Orient
		, 900						 // Wt. 
		, 0								 // Ital.
		, 0             				 // Underl.
		, 0                 			 // SO
		, ANSI_CHARSET		    		 // Charset
		, OUT_TT_PRECIS					 // Out prec.
		, CLIP_DEFAULT_PRECIS			 // Clip prec.
		, PROOF_QUALITY   			     // Qual.
		, VARIABLE_PITCH | FF_DONTCARE   // Pitch and Fam.
		, "Courier New" );

	GetClientRect( &rcClient );
	dc.CreateCompatibleDC( pDC );
	bm.CreateCompatibleBitmap( pDC, rcClient.Width(), rcClient.Height() );
	pOldBitmap = dc.SelectObject( &bm );

	// Clear window
	dc.FillRect( &rcClient, &CBrush( CLR_BG ) );

	// Draw dark border around window
	dc.FrameRect( &rcClient, &CBrush( RGB( 31, 31, 31 ) ) );

	// Move in a pixel
	rcClient.InflateRect( -1, -1 );

	dc.SelectObject( &penhash );
	dc.SetBkMode( TRANSPARENT );
	
	dc.SetTextColor( RGB( 50, 150, 200 ) );
	dc.SelectObject( &fnt );

	// Draw the title
	dc.TextOut( 5, 5, m_szTitle, strlen( m_szTitle ) );
	
	if ( !m_pUpdates )
	{
		rcClient.InflateRect( 1, 1 );
		pDC->BitBlt( 0, 0, rcClient.Width(), rcClient.Height(), &dc, 0, 0, SRCCOPY );

		dc.SelectObject( pOldBitmap );
		dc.DeleteDC();
		return;
	}
	
	// Draw hash marks.
	int i;
	float hashsize;
	int ytop;
	float hashes;
	float pixelsperhash;

	// Determine hashsize
	if ( m_yscale >= 20000 )
	{
		hashsize = 5000;
	}
	else if ( m_yscale >= 10000 )
	{
		hashsize = 1000;
	}
	else if ( m_yscale >= 5000 )
	{
		hashsize = 500;
	}
	else
	{
		hashsize = 100;
	}

	// Compute number of hashes to be drawn
	hashes = (float)( m_yscale ) / hashsize;
	hashes = max( 0.0f, hashes );

	ytop = 25;

	// Determine full size of draw area
	h = rcClient.Height() - ytop;

	// Determine pixel gap for each hash
	pixelsperhash = (float)h / hashes;

	char sz[ 256 ];
	sprintf( sz, "%i max, %i/hash %i%% scale/%i samples", (int)m_yscale, (int)hashsize, m_nScale, m_pUpdates[ 0 ].count );

	// Draw the graph details
	dc.TextOut( 125, 5, sz, strlen( sz ) );

	DrawLegend( rcClient.bottom - 120, dc, boldfnt );

	dc.SetTextColor( RGB( 31, 31, 255 ) );

	// Set up and draw hash marks now
	dc.MoveTo( rcClient.left, rcClient.bottom - ytop );
	dc.LineTo( rcClient.right, rcClient.bottom - ytop );

	for ( i = 0; i < (int)hashes; i++ )
	{
		y = rcClient.bottom - ytop - (int)( (float)i * pixelsperhash );

		dc.MoveTo( 0, y );
		dc.LineTo( 3, y );

		dc.MoveTo( rcClient.right - 3, y );
		dc.LineTo( rcClient.right, y );
	}

	CPen filepen( PS_SOLID, 1, RGB( 200, 200, 240 ) );

	// Iterate through games and draw the relevant graph
	for ( int stat = 0; stat <= MOD_STATCOUNT; stat++ )
	{
		CPen *old;
	
		float percent;
		int height;
		double xf;
					

		for ( int type = GRAPHLINE_NORMAL; type <= GRAPHLINE_BOTS; type++ )
		{
			if ( type > 0  && stat == MOD_PROXY ) // only player graphs graph the bot line
				continue;

			i = 0;
			int lastx = -1;
			xf = (double)( rcClient.right - RIGHT_EDGE_GAP );
			x = (int)xf;
			float lasth = 0.0;
			int have_drawn_dot[2] = { 0, 0 };
			CPen pen( PS_SOLID, 1, GetStatColor( stat, static_cast<GraphLineType>( type )) );


			// Find rightmost update
			p = FindUpdate( m_pUpdates->list, MapColToData( x ) );

			// While we still have room on the screen...
			while ( p && ( x >= ( rcClient.left + 3 ) ) )
			{
				old = dc.SelectObject( &pen );

				// Determine height of graph
				percent = (float)GetStatHeight( p, stat, static_cast<GraphLineType>( type ) );
				percent /= (float)m_yscale;

				percent = max( 0.0f, percent );
				percent = min( 1.0f, percent );

				height = (int)( percent * (float)h );

				y = rcClient.bottom - ytop;

				// First line, or last was empty, or there's a big gap, just draw a dot, not a line.
				if ( !i || ( lasth == 0 ) || ( percent == 0 ) || ( fabs( lasth - percent ) * (float)h > 30 ) )
				{
					dc.MoveTo( x, y - height );
				}

				// Draw line to next spot.
				if ( type == 0 )
				{
					dc.LineTo( x, y - height + 1 );
				}
				else if ( i%2 ) // make the bots line dotted, well, kinda anyway
				{
					dc.LineTo( x, y - height + 1 );
				}

				// If the mouse button is down then draw any necessary dots
				if ( m_bButtonDown )
				{
					if ( fabs( x - m_ptDown.x ) < GetScale() )
					{
						if ( !have_drawn_dot[ 0 ] )
						{
							DrawDot( dc, GetStatColor( stat, static_cast<GraphLineType>( type ) ), 3, m_ptDown.x, y - height );
						}
						have_drawn_dot[ 0 ] = 1;
					}

					if ( fabs( x - m_ptCurrent.x ) < GetScale() )
					{
						if ( !have_drawn_dot[ 1 ] )
						{
							DrawDot( dc, GetStatColor( stat, static_cast<GraphLineType>(type ) ), 3, m_ptCurrent.x, y - height );
						}
						have_drawn_dot[ 1 ] = 1;
					}
				}
				else
				{
					if ( fabs( x - m_ptCurrent.x ) < GetScale() )
					{
						if ( !have_drawn_dot[ 0 ] )
						{
							DrawDot( dc, GetStatColor( stat, static_cast<GraphLineType>( type ) ), 2, m_ptCurrent.x, y - height );
						}
						have_drawn_dot[ 0 ] = 1;
					}
				}

				if ( p->m_bLoadedFromFile )
				{
					dc.SelectObject( &filepen );

					dc.MoveTo( x, y - 2 );
					dc.LineTo( lastx, y-2 );
					dc.MoveTo( x, y - 1 );
					dc.LineTo( lastx, y-1 );
				//	dc.MoveTo( x, y );
				//	dc.LineTo( lastx, y );
				}

				// Restore the drawing position
				dc.MoveTo( x, y - height + 1 );

				// While screen space x is unchanged, iterate through the data set
				lastx = x;
				while ( p && ( lastx == x ) )
				{
					xf -= GetScale();
					x = (int)xf;
					i++;
					p = p->next;
				}

				dc.SelectObject( old );

				// Remember last height
				lasth = percent;
			}
		}// for(type)
	}

	// Print stats in top right if we are remembering a column
	if ( m_pHistory )
	{
		char sz[ 256 ];
		x = 2;
		y = 40;

		dc.SetTextColor( RGB( 200, 150, 150 ) );
		dc.SelectObject( &penhash );

		CMod *p;
		int i;

		CMod other;

		for ( i = 0 ; i < m_pHistory->Mods.getCount(); i++ )
		{
			p = m_pHistory->Mods.getMod( i );

			bool isBigEnough = false;
			if ( IsPlayerGraph() )
			{
				if ( p->total.players >= PRINT_THRESHOLD_PLAYERS  || p->total.bots >= PRINT_THRESHOLD_PLAYERS)
				{
					isBigEnough = true;
				}
			}
			else
			{
				if ( p->total.servers >= PRINT_THRESHOLD_SERVERS || p->total.bots_servers >= PRINT_THRESHOLD_SERVERS)
				{
					isBigEnough = true;
				}
			}

			if ( isBigEnough )
			{
				float percent;
				
				if ( IsPlayerGraph() )
				{
					percent = (float)p->total.players / (float)m_pHistory->total.players;
				}
				else
				{
					percent = (float)p->total.servers / (float)m_pHistory->total.servers;
				}

				percent *= 100.0f;
				sprintf( sz, "%15s:  %8i (%s) %s (%5.2f%%)", 
					p->GetGameDir(), 
					IsPlayerGraph() ? p->total.players : p->total.servers, 
					PrintBreakdowns( p ),
					IsPlayerGraph() ? "pl" : "sv",
					percent );

				dc.TextOut( x, y, sz, strlen( sz ) );
				y += 13;
			}
			else
			{
				other.total.servers += p->total.servers;
				other.total.players += p->total.players;
				other.total.bots    += p->total.bots;
				other.total.bots_servers    += p->total.bots_servers;

				for ( int j = 0; j < MOD_STATCOUNT; j++ )
				{
					other.stats[ j ].players += p->stats[ j ].players;
					other.stats[ j ].servers += p->stats[ j ].servers;
					other.stats[ j ].bots    += p->stats[ j ].bots;
					other.stats[ j ].bots_servers += p->stats[ j ].bots_servers;
				}
			}
		}

		if ( other.total.servers || other.total.players || other.total.bots || other.total.bots_servers)
		{
			float percent;
			
			if ( IsPlayerGraph() )
			{
				percent = (float)other.total.players / (float)m_pHistory->total.players;
			}
			else
			{
				percent = (float)other.total.servers / (float)m_pHistory->total.servers;
			}			
			
			sprintf( sz, "%15s:  %8i (%s) %s (%5.2f%%)", 
				"other", 
				IsPlayerGraph() ? other.total.players : other.total.servers, 
				PrintBreakdowns( &other ),
				IsPlayerGraph() ? "pl" : "sv",
				percent );
			dc.TextOut( x, y, sz, strlen( sz ) );
			y += 13;
		}

		dc.MoveTo( 10, y );
		dc.LineTo( rcClient.right - 10, y );

		y++;

		// totals
		sprintf( sz, "%15s:  %8i (%s) %s",
			"total", 
			IsPlayerGraph() ? m_pHistory->total.players : m_pHistory->total.servers, 
			PrintBreakdowns( m_pHistory ),
			IsPlayerGraph() ? "pl" : "sv" );
		dc.TextOut( x, y, sz, strlen( sz ) );
	}

	// Blit the final results to the screen
	rcClient.InflateRect( 1, 1 );
	pDC->BitBlt( 0, 0, rcClient.Width(), rcClient.Height(), &dc, 0, 0, SRCCOPY );

	dc.SelectObject( pOldBitmap );
	dc.DeleteDC();
}

/*
==============================
Create

==============================
*/
static char graphwndclass[] = "GRAPHWNDCLS";
BOOL CGraph::Create(DWORD dwStyle, CWnd *pParent, UINT id)
{
	CRect rcWndRect(0,0,100,100);

	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));

	wc.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW; 
    wc.lpfnWndProc   = AfxWndProc; 
    wc.hInstance     = AfxGetInstanceHandle();
    wc.hbrBackground = (HBRUSH)CreateSolidBrush( CLR_BG );
    wc.lpszMenuName  = NULL;
	wc.lpszClassName = graphwndclass;
	wc.hCursor =       ::LoadCursor( NULL, IDC_ARROW );

	// Register it, exit if it fails    
	if (!AfxRegisterClass(&wc))
	{
		return FALSE;
	}

    if (!CWnd::CreateEx( 0, graphwndclass, _T(""), dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_HSCROLL, 
                        rcWndRect.left, rcWndRect.top,
						rcWndRect.Width(), rcWndRect.Height(), // size updated soon
                        pParent->GetSafeHwnd(), (HMENU)id, NULL))
	{
        return FALSE;
	}

	rcWndRect.top = rcWndRect.bottom - 20;

	m_pStatus = new CODStatic();
	m_pStatus->Create( "", WS_VISIBLE | WS_CHILD, rcWndRect, this );

	CFont fnt;
	fnt.CreateFont(
		 -10					             // H
		, 0   					         // W
		, 0								 // Escapement
		, 0							     // Orient
		, FW_NORMAL						// Wt.  (BOLD)
		, 0								 // Ital.
		, 0             				 // Underl.
		, 0                 			 // SO
		, ANSI_CHARSET		    		 // Charset
		, OUT_TT_PRECIS					 // Out prec.
		, CLIP_DEFAULT_PRECIS			 // Clip prec.
		, PROOF_QUALITY   			     // Qual.
		, VARIABLE_PITCH | FF_DONTCARE   // Pitch and Fam.
		, "Arial" );

	m_pStatus->SetFont( &fnt, TRUE );
	m_pStatus->SetOffsets( 5, 0 );

	::SetScrollRange( GetSafeHwnd(), SB_HORZ, 0, MAX_SCROLL_POS, TRUE );
	::SetScrollPos( GetSafeHwnd(), SB_HORZ, MAX_SCROLL_POS, TRUE );

	return TRUE;
}

/*
==============================
ResizeControls

==============================
*/
void CGraph::ResizeControls( void )
{
	CRect rc;

	GetClientRect( &rc );
	rc.top = rc.bottom - 20;
	rc.DeflateRect( 1, 1 );
	if ( m_pStatus && m_pStatus->GetSafeHwnd() )
	{
		m_pStatus->MoveWindow( &rc, TRUE );
	}
}

/*
==============================
OnSize

==============================
*/
void CGraph::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	ResizeControls();
}

/*
==============================
OnMove

==============================
*/
void CGraph::OnMove(int x, int y) 
{
	CWnd::OnMove(x, y);
	ResizeControls();
}

/*
==============================
OnMouseMove

==============================
*/
void CGraph::OnMouseMove(UINT nFlags, CPoint point) 
{
	CWnd::OnMouseMove(nFlags, point);

	// Figure out the point we are over
	//
	CRect rc;
	GetClientRect( &rc );

	int column;

	char sztime[ 256 ];
	int bestdistance = 999999;
	int bestgame = -1;

	sprintf( sztime, "" );
	int ytop = 25;
	int h = rc.Height() - ytop;

	// Find closest game to mouse
	CModUpdate *p;

	column = MapColToData( point.x );

	p = FindUpdate( m_pUpdates->list, column );

	if ( p && p->realtime )
	{
		// Get ascii time out of it
		//
		float percent;
		int height;
		percent = (float)( ( m_gtType == GRAPHTYPE_PLAYERS ) ? p->total.players : p->total.servers ) / (float)m_yscale;
		percent = max( 0.0f, percent );
		percent = min( 1.0f, percent );

		height = (int)( percent * (float)h );

		int dist = abs( ( rc.bottom - height ) - point.y );

		if ( dist < bestdistance )
		{
			struct tm *today;
			today = localtime( &p->realtime );
			char sz[ 256 ];
			sprintf( sz, "%s", asctime( today ) );
			if ( strlen( sz ) > 0 )
			{
				sz[ strlen( sz ) - 1 ] = '\0';
			}

			sprintf( sztime, "%s:  %s", game_name, sz );
			bestdistance = dist;

			m_pHistory = p;
		}
	}

	if ( m_bButtonDown )
	{
		// Determine x and y differentials
		char sz[ 256 ];
		double pm, rawtime;
		double pm_per_month = 0.0;

		ComputeStatistics( m_ptDown.x, point.x, &pm, &rawtime );

		int h, m, w, d;
		double s;

		TIME_SecondsToHMS( rawtime, &w, &d, &h, &m, &s );

		char suffix[ 4 ];
		char suffix_per_month[ 4 ];

		if ( rawtime > 0 )
		{
			pm_per_month = pm * ( 86400.0 * 30.0 ) / rawtime;
		}

		ComputeSuffix( &pm, suffix );
		ComputeSuffix( &pm_per_month, suffix_per_month );

		char szTime[ 256 ];
		if ( w > 0 )
		{
			sprintf( szTime, "%02iw:%02id:%02i:%02i:%02i", w, d, h, m, (int)s );
		}
		else if ( d > 0 )
		{
			sprintf( szTime, "%02id:%02i:%02i:%02i", d, h, m, (int)s );
		}
		else
		{
			sprintf( szTime, "%02i:%02i:%02i", h, m, (int)s );
		}

		sprintf( sz, " %.2f%s (%.2f%s) %s time=%s", pm, suffix, pm_per_month, suffix_per_month,
			( m_gtType == GRAPHTYPE_PLAYERS ) ? "player minutes" : "server minutes", szTime );
		strcat( sztime, sz );
	}

	if ( m_pStatus )
	{
		m_pStatus->SetWindowText( sztime );
	}

	m_ptCurrent = point;

	InvalidateRect( NULL );
}

/*
==============================
OnLButtonDown

==============================
*/
void CGraph::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CWnd::OnLButtonDown(nFlags, point);

	if ( m_bButtonDown )
		return;

	m_bButtonDown = true;
	m_ptDown = point;

	InvalidateRect( NULL );
	SetCapture();

	SetFocus();
}

/*
==============================
OnLButtonUp

==============================
*/
void CGraph::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CWnd::OnLButtonUp(nFlags, point);

	if ( !m_bButtonDown )
		return;

	m_bButtonDown = false;
	ReleaseCapture();
}

/*
==============================
ComputeStatistics

Determines player minutes and elapshed time for a range of mouse positions
==============================
*/
void CGraph::ComputeStatistics( int startx, int endx, double *output, double *rawtime )
{
	int sc, ec;

	// Make sure we have the correct order
	if ( startx >= endx )
	{
		int swap;
		swap = endx;
		endx = startx;
		startx = swap;
	}

	// Get start and end columns
	sc = MapColToData( startx );
	ec = MapColToData( endx );

	double player_minutes = 0.0;
	double time_interval = 0;

	double dt;
	double avg_units;

	CModUpdate *pcurrent, *pprevious, *pstart, *pend;

	// Start at end ( newest ) and go backward in time through list
	pstart = FindUpdate( m_pUpdates->list, ec );
	if ( pstart )
	{
		pcurrent	= pstart;
		pprevious	= pstart->next;

		// Find finishing update
		pend = FindUpdate( m_pUpdates->list, sc );

		// go until pointers intersect
		while ( pcurrent && pprevious && pprevious != pend )
		{
			// Taylor's series, find average over this sample
			if ( m_gtType == GRAPHTYPE_PLAYERS )
			{
				avg_units = pcurrent->total.players + pprevious->total.players;
			}
			else
			{
				avg_units = pcurrent->total.servers + pprevious->total.servers;
			}
			avg_units /= 2.0;

			// Figure out time differential
			dt = difftime( pcurrent->realtime, pprevious->realtime );

			// Area under curve is avg * dt
			player_minutes += dt * avg_units;

			// Increment time interval
			time_interval += dt;

			// Move to next previous data elements
			pcurrent	= pprevious;
			pprevious	= pprevious->next;
		}
	}

	// Convert from seconds to minutes.
	player_minutes /= 60.0; 

	// Write output data
	*output		= player_minutes;
	*rawtime	= time_interval;
}

/*
==============================
MapColToData

==============================
*/
int CGraph::MapColToData( int column )
{
	CRect rc;
	GetClientRect( &rc );

	// Find rightmost pixel
	int right_edge = rc.right - RIGHT_EDGE_GAP;

	// How many pixels in are we from there
	int pixels_from_edge = right_edge - column;

	// At least zero
	if ( pixels_from_edge < 0 )
		pixels_from_edge = 0;

	// Now convert from pixels to game samples using correct scaling factor
	int col = (int)( (float)pixels_from_edge / GetScale() );
	
	// Adjust for horizontal scroll bar position
	col += GetLastElement();

	return col;
}

/*
==============================
OnScaleUp

==============================
*/
void CGraph::OnScaleUp( int amt )
{
	m_nScale += amt;
	m_nScale = max( m_nScale, 1 );

	InvalidateRect( NULL );
}

/*
==============================
OnScaleDown

==============================
*/
void CGraph::OnScaleDown( int amt )
{
	m_nScale -= amt;
	m_nScale = max( m_nScale, 1 );
	
	InvalidateRect( NULL );
}

/*
==============================
OnHScroll

==============================
*/
void CGraph::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	int nCurrent;

	nCurrent = ::GetScrollPos( GetSafeHwnd(), SB_HORZ );

	switch ( nSBCode )
	{
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		::SetScrollPos( GetSafeHwnd(), SB_HORZ, nPos, TRUE );
		InvalidateRect( NULL );
		UpdateWindow();
		return;
	case SB_PAGEUP:
		nCurrent -= 50;
		break;
	case SB_PAGEDOWN:
		nCurrent += 50;
		break;
	case SB_LINEUP:
		nCurrent -= 1;
		break;
	case SB_LINEDOWN:
		nCurrent += 1;
		break;
	default:
		return;
	}

	nCurrent = max( 0, nCurrent );
	nCurrent = min( MAX_SCROLL_POS, nCurrent );

	::SetScrollPos( GetSafeHwnd(), SB_HORZ, nCurrent, TRUE );

	InvalidateRect( NULL );
	UpdateWindow();
}

/*
==============================
FindUpdate

==============================
*/
CModUpdate * CGraph::FindUpdate( CModUpdate *list, int number)
{
	int i = 0;
	CModUpdate *p;

	p = list;
	while ( p )
	{
		if ( i == number )
			return p;

		i++;
		// Return end of list?
		if ( !p->next )
			return p;
		p = p->next;
	}

	// Shouldn't happen 
	return NULL;
}

/*
==============================
GetLastElement

Determine rightmost element based on scroll bar position
==============================
*/
int CGraph::GetLastElement( void )
{
	// Get scroll bar position
	double percent;

	// Get scroll bar position
	int pos = ::GetScrollPos( GetSafeHwnd(), SB_HORZ );

	// Convert to normalized fraction
	percent = (double)pos / (double)MAX_SCROLL_POS;

	double data_elements = 0.0;

	// Find a game that has enough elements.
	if ( m_pUpdates->count > (int)data_elements )
	{
		data_elements = (double)m_pUpdates->count;
	}

	// Scale element count
	int rightmost = (int)( ( 1.0 - percent ) * data_elements );

	// Return result
	return rightmost;
}

/*
==============================
ResetHistory

Clears out history pointer
==============================
*/
void CGraph::ResetHistory( void )
{
	m_pHistory	= NULL;
}

const char *CGraph::GetHistoryString( CModUpdate *update )
{
	if ( !update )
	{
		update = m_pHistory;
		if ( !update )
		{
			return "No history";
		}
	}

	static char history[ 8192 ];
	history[0]=0;
	char sz[ 256 ];

	CMod *p;
	int i;

	CMod other;

	for ( i = 0 ; i < update->Mods.getCount(); i++ )
	{
		p = update->Mods.getMod( i );

		bool isBigEnough = false;
		if ( IsPlayerGraph() )
		{
			if ( p->total.players >= PRINT_THRESHOLD_PLAYERS )
			{
				isBigEnough = true;
			}
			if ( p->total.bots >= PRINT_THRESHOLD_PLAYERS )
			{
				isBigEnough = true;
			}

		}
		else
		{
			if ( p->total.servers >= PRINT_THRESHOLD_SERVERS )
			{
				isBigEnough = true;
			}
			if ( p->total.bots_servers >= PRINT_THRESHOLD_SERVERS )
			{
				isBigEnough = true;
			}
		}

		if ( isBigEnough )
		{
			float percent;
			
			if ( IsPlayerGraph() )
			{
				percent = (float)p->total.players / (float)update->total.players;
			}
			else
			{
				percent = (float)p->total.servers / (float)update->total.servers;
			}

			percent *= 100.0f;
			sprintf( sz, "%15s:  %8i (%s) %s (%5.2f%%)", 
				p->GetGameDir(), 
				IsPlayerGraph() ? p->total.players : p->total.servers, 
				PrintBreakdowns( p ),
				IsPlayerGraph() ? "pl" : "sv",
				percent );

			//dc.TextOut( x, y, sz, strlen( sz ) );
			strcat( history, sz );
			strcat( history, "\r\n" );

		}
		else
		{
			other.total.servers += p->total.servers;
			other.total.players += p->total.players;
			other.total.bots    += p->total.bots;
			other.total.bots_servers    += p->total.bots_servers;

			for ( int j = 0; j < MOD_STATCOUNT; j++ )
			{
				other.stats[ j ].players += p->stats[ j ].players;
				other.stats[ j ].servers += p->stats[ j ].servers;
				other.stats[ j ].bots    += p->stats[ j ].bots;
				other.stats[ j ].bots_servers    += p->stats[ j ].bots_servers;
			}
		}
	}

	if ( other.total.servers || other.total.players )
	{
		float percent;
		
		if ( IsPlayerGraph() )
		{
			percent = (float)other.total.players / (float)update->total.players;
		}
		else
		{
			percent = (float)other.total.servers / (float)update->total.servers;
		}			
		
		sprintf( sz, "%15s:  %8i (%s) %s (%5.2f%%)", 
			"other", 
			IsPlayerGraph() ? other.total.players : other.total.servers, 
			PrintBreakdowns( &other ),
			IsPlayerGraph() ? "pl" : "sv",
			percent );
		strcat( history, sz );
		strcat( history, "\r\n" );

	}

	// totals
	sprintf( sz, "%15s:  %8i (%s) %s",
		"total", 
		IsPlayerGraph() ? update->total.players : update->total.servers, 
		PrintBreakdowns( update ),
		IsPlayerGraph() ? "pl" : "sv" );
	strcat( history, sz );
	strcat( history, "\r\n" );

	return history;
}

void CGraph::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	//CWnd::OnRButtonUp(nFlags, point);

	// Copy current data to clipboard
	// Print stats in top right if we are remembering a column
	const char *history = GetHistoryString( m_pHistory );

	// Copy to clipboard
	if ( !OpenClipboard() )
	{
		AfxMessageBox( "Cannot open the Clipboard" );
		return;
	}

	// Remove the current Clipboard contents
	if( !EmptyClipboard() )
	{
		AfxMessageBox( "Cannot empty the Clipboard" );
		return;
	}

	HANDLE hData = ::GlobalAlloc( GMEM_MOVEABLE, strlen( history ) + 1 );
	char *pdata = (char *)GlobalLock( hData );
	strcpy( pdata, history );
	GlobalUnlock( hData );

	// For the appropriate data formats...
	if ( ::SetClipboardData( CF_TEXT, hData ) == NULL )
	{
		AfxMessageBox( "Unable to set Clipboard data" );
		CloseClipboard();
		return;
	}
	// ...
	CloseClipboard();

	GlobalFree( hData );
}

BOOL CGraph::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if ( zDelta > 0 )
	{
		OnScaleUp( Multiplier( zDelta/120 ) );
	}
	else
	{
		OnScaleDown( Multiplier( -zDelta/120 ) );
	}

	return TRUE;
}

int CGraph::Multiplier( int amt )
{
	if ( amt == 1 )
		return amt;

	if ( m_nScale < 100 )
		return amt * 5;

	if ( m_nScale < 500 )
		return amt * 10;

	if ( m_nScale < 1000 )
		return amt * 25;

	return amt * 50;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ys - 
//-----------------------------------------------------------------------------
void CGraph::SetYScale( int ys )
{
	m_yscale = ys;

	InvalidateRect( NULL );
	UpdateWindow();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CGraph::GetYScale( void )
{
	return m_yscale;
}
