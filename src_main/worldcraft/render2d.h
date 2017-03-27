//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the interface for rendering in the 2D views.
//
// $NoKeywords: $
//=============================================================================

#ifndef RENDER2D_H
#define RENDER2D_H
#ifdef _WIN32
#pragma once
#endif

#include "Mathlib.h"


class CRender2D
{
public:

    enum { LINE_SOLID = 0, LINE_DASHED, LINE_DOT };
    enum { LINE_THIN = 0, LINE_THICK };

    enum { TEXT_SINGLELINE           =   0x1,             // put all of the text on one line
           TEXT_MULTILINE            =   0x2,             // the text is written on multiple lines
           TEXT_JUSTIFY_BOTTOM       =   0x4,             // justifications....
           TEXT_JUSTIFY_TOP          =   0x8,
           TEXT_JUSTIFY_RIGHT        =  0x10,
           TEXT_JUSTIFY_LEFT         =  0x20,
           TEXT_JUSTIFY_HORZ_CENTER  =  0x40,
           TEXT_JUSTIFY_VERT_CENTER  =  0x80,
           TEXT_CLEAR_BACKGROUND     = 0x100 };           // clear the background behind the text

    //
    // construction/deconstruction
    //
    CRender2D();
    ~CRender2D();

    //
    // setup (view) data
    //
    inline void SetCDC( CDC *pDC );
    inline void Set2DViewInfo( int horzIndex, int vertIndex, float zoom, bool bInvertHorz, bool bInvertVert );

    //
    // rendering attribs
    //
    inline void SetTextColor( unsigned char tR, unsigned char tG, unsigned char tB, unsigned char bkR, unsigned char bkG, unsigned char bkB );
    inline void SetFillColor( unsigned char r, unsigned char g, unsigned char b );
    void SetLineColor( unsigned char r, unsigned char g, unsigned char b );
    void SetLineColor( color32 clr );
    void SetLineType( int type, int thickness, unsigned char r, unsigned char g, unsigned char b );
    void SetLineType( int type, int thickness, color32 clr );

    //
    // rendering
    //
	inline void MoveTo( const CPoint &pt );
	inline void MoveTo( int x, int y );
	inline void LineTo( const CPoint &pt );
	inline void LineTo( int x, int y );

    inline void DrawPoint( const CPoint &pt, int size );
    inline void DrawPoint( const Vector &pt, int size );

    inline void DrawLine( const CPoint &pt1, const CPoint &pt2 );
    inline void DrawLine( const Vector &pt1, const Vector &pt2 );

    void DrawEllipse( CPoint const &ptCenter, int nRadiusX, int nRadiusY, bool bFill );

    void DrawLineLoop( int nPoints, Vector *Points, bool bDrawVerts, int size );
    void DrawText( const char *text, const CPoint &pos, int width, int height, int nFlags );

	inline void Rectangle( const CRect &rect, bool bFill );
	inline void Rectangle( int left, int top, int right, int bottom, bool bFill );

	inline int GetAxisHorz( void );
	inline int GetAxisVert( void );
	inline int GetAxisThird( void );

	inline float GetZoom( void ) { return m_Zoom; }

    //
    // util
    //
    void TransformPoint3D( CPoint &ptOut, const Vector& ptIn );
    void TransformPoint3D( Vector2D &ptOut, const Vector &ptIn );

private:

    CDC     *m_pDC;                 // device context (gdi-rendering)
    int     m_HorzIndex;
    int     m_VertIndex;
	int		m_ThirdIndex;
    float   m_Zoom;
    bool    m_bInvertHorz;
    bool    m_bInvertVert;

    unsigned char m_FillColor[3];         // current fill color
    unsigned char m_TextColor[3];         // current text color

    CPen    *m_pCurPen;						// The current pen being used.
	CPen	*m_pPenCache[2];				// A cache of the last two pens used.

    UINT SetTextFormat( int nFlags );

    //
    // Pen cache management.
    //
	CPen *GetPen(LOGPEN &logpenInfo);
	void FreePenCache();
};


//-----------------------------------------------------------------------------
// Purpose: Returns the index (0 - 2) of the horizontal axis for this renderer.
//-----------------------------------------------------------------------------
int CRender2D::GetAxisHorz( void )
{
	return( m_HorzIndex );
}


//-----------------------------------------------------------------------------
// Purpose: Returns the index (0 - 2) of the vertical axis for this renderer.
//-----------------------------------------------------------------------------
int CRender2D::GetAxisVert( void )
{
	return( m_VertIndex );
}


//-----------------------------------------------------------------------------
// Purpose: Returns the index (0 - 2) of the third (depth) axis for this renderer.
//-----------------------------------------------------------------------------
int CRender2D::GetAxisThird( void )
{
	return( m_ThirdIndex );
}


//-----------------------------------------------------------------------------
// Purpose: sets the device context needed for GDI rendering
//-----------------------------------------------------------------------------
inline void CRender2D::SetCDC( CDC *pDC )
{
    m_pDC = pDC;
}


//-----------------------------------------------------------------------------
// Purpose: sets all the 2DView info needed to render objects into the view
//   Input: horzIndex - the 2D horizontal point index
//          vertIndex - the 2D vertical point index
//          zoom - 2D zoom (scale)
//          bInvertHorz - horizontal values inverted for rendering purposes
//          bInvertVert - vertical values inverted for rendering purposes
//-----------------------------------------------------------------------------
inline void CRender2D::Set2DViewInfo( int horzIndex, int vertIndex, float zoom, 
                                      bool bInvertHorz, bool bInvertVert  )
{
    m_HorzIndex = horzIndex;
    m_VertIndex = vertIndex;

	//
	// Deduce the third (depth) axis from the other two.
	//
	if ((m_HorzIndex != 0) && (m_VertIndex != 0))
	{
		m_ThirdIndex = 0;
		}
	else if ((m_HorzIndex != 1) && (m_VertIndex != 1))
		{
			m_ThirdIndex = 1;
		}
	else
		{
		ASSERT((m_HorzIndex != 2) && (m_VertIndex != 2));
		m_ThirdIndex = 2;
	}

    m_Zoom = zoom;
    m_bInvertHorz = bInvertHorz;
    m_bInvertVert = bInvertVert;
}


//-----------------------------------------------------------------------------
// Purpose: set the text and background (behind text) colors
//   Input: tR, tG, tB - text red, green, and blue values : [0...255]
//          bkR, bkG, bkB - background red, green, blue values : [0...255]
//-----------------------------------------------------------------------------
inline void CRender2D::SetTextColor( unsigned char tR, unsigned char tG, unsigned char tB, 
                                     unsigned char bkR, unsigned char bkG, unsigned char bkB )
{
    ASSERT( m_pDC );

    m_pDC->SetTextColor( RGB( tR, tG, tB ));

    m_pDC->SetBkColor( RGB( bkR, bkG, bkB ));
}


//-----------------------------------------------------------------------------
// Purpose: set the polygon fill color (vertices, etc.)
//   Input: r, g, b - red, green, and blue values : [0...255]
//-----------------------------------------------------------------------------
inline void CRender2D::SetFillColor( unsigned char r, unsigned char g, unsigned char b )
{
    m_FillColor[0] = r;
    m_FillColor[1] = g;
    m_FillColor[2] = b;
}


//-----------------------------------------------------------------------------
// Purpose: Draws a vertex of a given size and the given point.  The color is
//          the fill color (see SetFillColor)
//   Input: pt - the position of the vertex to draw in client coordinates
//          size - the size of the vertex being drawn
//-----------------------------------------------------------------------------
inline void CRender2D::DrawPoint( const CPoint &pt, int size )
{
    ASSERT( m_pDC );

    int halfSize = size / 2;
    CRect r( pt.x - halfSize, 
             pt.y - halfSize, 
             pt.x + halfSize, 
             pt.y + halfSize );
    m_pDC->FillSolidRect( &r, RGB( m_FillColor[0], m_FillColor[1], m_FillColor[2] ));
}


//-----------------------------------------------------------------------------
// Purpose: Draws a vertex of a given size and the given point.  The color is
//          the fill color (see SetFillColor).  The vertex is given in 3D and
//          is transformed based on the 2DView info into a 2D point
//   Input: pt - the position of the vertex to draw in 3D
//          size - the size of the vertex being drawn
//-----------------------------------------------------------------------------
inline void CRender2D::DrawPoint( const Vector &pt, int size )
{
    ASSERT( m_pDC );

    CPoint screenPt;
    TransformPoint3D( screenPt, pt );

    int halfSize = size / 2;
    CRect r( screenPt.x - halfSize, 
             screenPt.y - halfSize, 
             screenPt.x + halfSize, 
             screenPt.y + halfSize );
    m_pDC->FillSolidRect( &r, RGB( m_FillColor[0], m_FillColor[1], m_FillColor[2] ));
}


//-----------------------------------------------------------------------------
// Purpose: Moves the pen to the given point. Should be followed by LineTo to
//			draw a line.
//   Input: pt - the point to move to.
//-----------------------------------------------------------------------------
inline void CRender2D::MoveTo( const CPoint &pt )
{
    ASSERT( m_pDC );
    m_pDC->MoveTo( pt.x, pt.y );
}


//-----------------------------------------------------------------------------
// Purpose: Moves the pen to the given point. Should be followed by LineTo to
//			draw a line.
//   Input: pt - the point to move to.
//-----------------------------------------------------------------------------
inline void CRender2D::MoveTo( int x, int y )
{
    ASSERT( m_pDC );
    m_pDC->MoveTo( x, y );
}


//-----------------------------------------------------------------------------
// Purpose: Draws a line from the current pen position to the given point.
//   Input: pt - the point to draw the line to.
//-----------------------------------------------------------------------------
inline void CRender2D::LineTo( const CPoint &pt )
{
    ASSERT( m_pDC );
    m_pDC->LineTo( pt.x, pt.y );
}


//-----------------------------------------------------------------------------
// Purpose: Draws a line from the current pen position to the given point.
//   Input: pt - the point to draw the line to.
//-----------------------------------------------------------------------------
inline void CRender2D::LineTo( int x, int y )
{
    ASSERT( m_pDC );
    m_pDC->LineTo( x, y );
}


//-----------------------------------------------------------------------------
// Purpose: Draws a line from the current pen position to the given point.
//   Input: pt - the point to draw the line to.
//-----------------------------------------------------------------------------
inline void CRender2D::Rectangle( int left, int top, int right, int bottom, bool bFill )
{
    ASSERT( m_pDC );

	//
	// Select a fill brush if requested.
	//
	CGdiObject *pbrushOld;
	CBrush brushFill( RGB( m_FillColor[0], m_FillColor[1], m_FillColor[2] ) );
	if ( bFill )
	{
		pbrushOld = m_pDC->SelectObject( &brushFill );
	}
	else
	{
		pbrushOld = m_pDC->SelectStockObject( NULL_BRUSH );
	}

    m_pDC->Rectangle( left, top, right, bottom );

	m_pDC->SelectObject( pbrushOld );
}


//-----------------------------------------------------------------------------
// Purpose: Draws a line from the current pen position to the given point.
//   Input: pt - the point to draw the line to.
//-----------------------------------------------------------------------------
inline void CRender2D::Rectangle( const CRect &rect, bool bFill )
{
    ASSERT( m_pDC );

	//
	// Select a fill brush if requested.
	//
	CGdiObject *pbrushOld;
	CBrush brushFill( RGB( m_FillColor[0], m_FillColor[1], m_FillColor[2] ) );
	if ( bFill )
	{
		pbrushOld = m_pDC->SelectObject( &brushFill );
	}
	else
	{
		pbrushOld = m_pDC->SelectStockObject( NULL_BRUSH );
	}

    m_pDC->Rectangle( rect );

	m_pDC->SelectObject( pbrushOld );
}


//-----------------------------------------------------------------------------
// Purpose: Draw a line from the given point 1 to given point 2.  The color 
//          and line style are based on the properties given to SetLineType.
//   Input: pt1 - the starting point (draw from)
//          pt2 - the end point (draw to)
//-----------------------------------------------------------------------------
inline void CRender2D::DrawLine( const CPoint &pt1, const CPoint &pt2 )
{
    ASSERT( m_pDC );

    m_pDC->MoveTo( pt1 );
    m_pDC->LineTo( pt2 );
}


//-----------------------------------------------------------------------------
// Purpose: Draw a line from the given point 1 to given point 2.  The color 
//          and line style are based on the properties given to SetLineType.
//          The vertices are given in 3D and are to be transformed based on the
//          defined 2DView info into a 2D point.
//   Input: pt1 - the starting point (draw from) in 3D
//          pt2 - the end point (draw to) in 3D
//-----------------------------------------------------------------------------
inline void CRender2D::DrawLine( const Vector &pt1, const Vector &pt2 )
{
    ASSERT( m_pDC );

    CPoint screenPt1, screenPt2;
    TransformPoint3D( screenPt1, pt1 );
    TransformPoint3D( screenPt2, pt2 );

    m_pDC->MoveTo( screenPt1 );
    m_pDC->LineTo( screenPt2 );
}

#endif // RENDER2D_H
