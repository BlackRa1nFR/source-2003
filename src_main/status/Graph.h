#if !defined(AFX_GRAPH_H__E0645F40_94B1_11D3_A683_0050041C0AC1__INCLUDED_)
#define AFX_GRAPH_H__E0645F40_94B1_11D3_A683_0050041C0AC1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Graph.h : header file
//

class CODStatic;

/////////////////////////////////////////////////////////////////////////////
// CGraph window

class CGraph : public CWnd
{
// Construction
public:
	typedef enum
	{
		GRAPHTYPE_PLAYERS = 0,
		GRAPHTYPE_SERVERS
	} GraphType;

	typedef enum
	{	
		GRAPHLINE_NORMAL = 0,
		GRAPHLINE_BOTS = 1
	} GraphLineType;

	CGraph( char *title );
	virtual	~CGraph();

	BOOL	Create( DWORD dwStyle, CWnd *pParent, UINT id );

	// Resets the data pointers
	void	SetData( CModStats *mod, int scale, GraphType type );
	// Clears any pointers that might have been destroyed
	void	ResetHistory( void );

	int		GetStatHeight( CModUpdate *p, int statnum, GraphLineType type );
	COLORREF GetStatColor( int statnum, GraphLineType type );
	void	DrawLegend( int y, CDC& dc, CFont& fnt );

	const char *PrintBreakdowns( CModUpdate *p );
	const char *PrintBreakdowns( CMod *p );
	
	bool IsPlayerGraph( void );

	void		SetYScale( int ys );
	int			GetYScale( void );

	const char	*GetHistoryString( CModUpdate *update );

public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraph)
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CGraph)
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	// helper function 
	const char * PrintBreakdowns( CStat *stats );

	// Get Index of right most element
	int			GetLastElement( void );
	// Convert mouse click column to game data element number
	int			MapColToData( int column );
	// Search for an update by number
	CModUpdate	*FindUpdate( CModUpdate *list, int number );
	// Compute player minutes and time interval for a range of samples
	void		ComputeStatistics( int startx, int endx, double *output, double *rawtime );

	// Redo control layout
	void		ResizeControls( void );

	// Redraw main drawing area
	void		Redraw( CDC *pDC );

	void		SetScale( int scale );
	float		GetScale( void );
	
	void		OnScaleUp( int amt );
	void		OnScaleDown( int amt );

	int			Multiplier( int amt );


private:
	// Pointer to underlying update data
	CModStats *m_pUpdates;
	
	// Vertical scale for this graph window
	int			m_yscale;
	// Window title
	char		m_szTitle[256];
	// Is this a player graph or a server graph?
	GraphType	m_gtType;

	// Click and drag handlers
	bool		m_bButtonDown;
	CPoint		m_ptDown, m_ptCurrent;

	// Window data scale factor
	int			m_nScale;

	// Which column is mouse hovering over
	CModUpdate *m_pHistory;

	// Window child controls
	CODStatic	*m_pStatus;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPH_H__E0645F40_94B1_11D3_A683_0050041C0AC1__INCLUDED_)
