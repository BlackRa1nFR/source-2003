//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Render2D.h"


//-----------------------------------------------------------------------------
// Purpose: constructor - initialize all the member variables
//-----------------------------------------------------------------------------
CRender2D::CRender2D()
{
    m_pDC = NULL;

    m_HorzIndex = 0;
    m_VertIndex = 1;
    m_Zoom = 1.0f;
    m_bInvertHorz = false;
    m_bInvertVert = false;

    m_FillColor[0] = m_FillColor[1] = m_FillColor[2] = 1.0f;

    m_pCurPen = NULL;

	memset(m_pPenCache, 0, sizeof(m_pPenCache));
}


//-----------------------------------------------------------------------------
// Purpose: deconstructor - free all info that may need to be freed
//-----------------------------------------------------------------------------
CRender2D::~CRender2D()
{
    FreePenCache();
}


//-----------------------------------------------------------------------------
// Purpose: This functions sets the color of the "line" to be drawn.
//   Input: r, g, b - red, green, and blue values [0...255]
//-----------------------------------------------------------------------------
void CRender2D::SetLineColor( unsigned char r, unsigned char g, unsigned char b )
{
    ASSERT( m_pDC );

    //
    // Get the current line info, or use a default pen.
    //
    LOGPEN penInfo;
	if (m_pCurPen != NULL)
	{
		m_pCurPen->GetLogPen( &penInfo );
	}
	else
	{
		penInfo.lopnStyle = PS_SOLID;
		penInfo.lopnWidth.x = 0;
	}

    //
    // Set the new line color.
    //
    penInfo.lopnColor = RGB( r, g, b );

    CPen *pNewPen = GetPen(penInfo);
	if (pNewPen != NULL)
	{
		m_pCurPen = pNewPen;
	    m_pDC->SelectObject( m_pCurPen );
	}
}


//-----------------------------------------------------------------------------
// Purpose: This functions sets the color of the "line" to be drawn.
// Input  : clr - red, green, and blue values [0...255]
//-----------------------------------------------------------------------------
void CRender2D::SetLineColor( color32 clr )
{
	SetLineColor(clr.r, clr.g, clr.b);
}


//-----------------------------------------------------------------------------
// Purpose: This function sets the type of line (solid, dashed) as well as the
//          line's color value.  The "OldLineType" flag determines whether or
//          not you wish to save the previously used line type for resetting 
//          later.
//   Input: type - the line style (solid, dashed)
//          thickness - the width of the line
//          r, g, b - red, green, and blue color values [0.0...1.0]
//-----------------------------------------------------------------------------
void CRender2D::SetLineType( int type, int thickness, unsigned char r, unsigned char g, unsigned char b)
{
    // sanity check
    ASSERT( m_pDC );

    LOGPEN penInfo;

    //
    // set the pen style
    //
    switch( type )
    {
    case LINE_SOLID: penInfo.lopnStyle = PS_SOLID; break;
    case LINE_DASHED: penInfo.lopnStyle = PS_DASH; break;
    case LINE_DOT: penInfo.lopnStyle = PS_DOT; break;
    default: return;
    }

	if (type != LINE_SOLID)
	{
		m_pDC->SetBkMode( TRANSPARENT );
	}

    //
    // set the pen thickness
    //
    switch( thickness )
    {
    case LINE_THIN: penInfo.lopnWidth.x = 0; break;
    case LINE_THICK: penInfo.lopnWidth.x = 2; break;
    default: return;
    }

    // set the pen color
    penInfo.lopnColor = RGB(r, g, b);

	// Get a pen matching the requested style and select it into the DC.
	CPen *pNewPen = GetPen(penInfo);
	if (pNewPen != NULL)
	{
		m_pCurPen = pNewPen;
		m_pDC->SelectObject(m_pCurPen);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			thickness - 
//			clr - 
//-----------------------------------------------------------------------------
void CRender2D::SetLineType( int type, int thickness, color32 clr )
{
	SetLineType( type, thickness, clr.r, clr.g, clr.b );
}


//-----------------------------------------------------------------------------
// Purpose: This functions draws a polyline given a set of ordered points.  A 
//          vertex of a given size can be drawn at each point if desired.  The
//          color of the lines are derived from the SetLineType/SetLineColor
//          functions.  The color of the points are from SetFillColor.
//   Input: nPoints - the number of points making up the polygon
//          Points - the points making the up the polygon
//          bDrawVerts - render the vertices?
//          size - size of the vertices if rendered
//-----------------------------------------------------------------------------
void CRender2D::DrawLineLoop( int nPoints, Vector *Points, bool bDrawVerts, int size )
{
    // sanity check
    ASSERT( m_pDC );

    //
    // check for degenerate faces
    //
    if( nPoints == 0 )
        return;

    //
    // draw the polygon
    //
    CPoint screenPt;
    TransformPoint3D( screenPt, Points[0] );
    m_pDC->MoveTo( screenPt );

    for( int i = 1; i < nPoints; i++ )
    {
        TransformPoint3D( screenPt, Points[i] );
        m_pDC->LineTo( screenPt );
    }

    TransformPoint3D( screenPt, Points[0] );
    m_pDC->MoveTo( screenPt );

    //
    // draw the vertices -- if necessary
    //
    if( bDrawVerts )
    {
        for( int i = 0; i < nPoints; i++ )
        {
            TransformPoint3D( screenPt, Points[i] );
            DrawPoint( screenPt, size );
        }
    }
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ptCenter - the center point in client coordinates
//			nRadiusX - the x radius in pixels
//			nRadiusY - the y radius in pixels
//			bFill - Whether to fill the ellipse with the fill color or not.
//-----------------------------------------------------------------------------
void CRender2D::DrawEllipse( const CPoint &ptCenter, int nRadiusX, int nRadiusY, bool bFill )
{
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

	CRect rect( ptCenter.x - nRadiusX, ptCenter.y - nRadiusY, ptCenter.x + nRadiusX, ptCenter.y + nRadiusY );
	m_pDC->Ellipse( &rect );

	m_pDC->SelectObject( pbrushOld );
}


//-----------------------------------------------------------------------------
// Purpose: This function converts the 2DRenderer bitwise format representation
//          into the GDI text format representation
//              TEXT_SINGLELINE - all text sent is written to a single line;
//                                height >= FONT height, width >= width of text
//              TEXT_MULTILINE - all text is visible, however the width is
//                               defined, and the height is adjusted to fit all
//                               the text
//              TEXT_JUSTIFY_* - text justifications...
//   Input: nFlags - the 2DRenderer bitwise representation of the text format
//  Output: a text format representation formatted for GDI
//-----------------------------------------------------------------------------
UINT CRender2D::SetTextFormat( int nFlags )
{
    UINT format = 0;

    // put all of the text on a single line
    if( nFlags & TEXT_SINGLELINE ) { format |= DT_SINGLELINE; }

    // make sure all of the text shows
    if( nFlags & TEXT_MULTILINE ) { format |= DT_CALCRECT; }

    //
    // handle the text justifications
    //
    if( nFlags & TEXT_JUSTIFY_BOTTOM ) { format |= DT_BOTTOM; }
    if( nFlags & TEXT_JUSTIFY_TOP ) { format |= DT_TOP; }
    if( nFlags & TEXT_JUSTIFY_RIGHT ) { format |= DT_RIGHT; }
    if( nFlags & TEXT_JUSTIFY_LEFT ) { format |= DT_LEFT; }
    if( nFlags & TEXT_JUSTIFY_HORZ_CENTER ) { format |= DT_CENTER; }
    if( nFlags & TEXT_JUSTIFY_VERT_CENTER ) { format |= DT_VCENTER; }

    return format;
}


//-----------------------------------------------------------------------------
// Purpose: This function renders a text string at the given position, width,
//          height, and format
//   Input: text - the string to print
//          pos - the screen space position of the text (the is the top-left of
//                a screen space rect)
//          width - the width to render the text into
//          height - the height to render the text into
//          nFlags - define the text format -- see SetTextFormat
//-----------------------------------------------------------------------------
void CRender2D::DrawText( const char *text, const CPoint &pos, int width, int height, int nFlags )
{
    ASSERT( m_pDC );

    // get the text size
    int textLength = strlen( text );

    //
    // create the bounding rect and inflate if necessary
    //
    CRect textRect;
    textRect.left = pos.x;
    textRect.top = pos.y;
    textRect.right = pos.x + width;
    textRect.bottom = pos.y + height;        

    if( nFlags & TEXT_SINGLELINE )
    {
        CSize size = m_pDC->GetTextExtent( text, textLength );

        if( size.cx > width )
        {
            textRect.right = pos.x + size.cx;
        }

        if( size.cy > height )
        {
            textRect.bottom = pos.y + size.cy;
        }
    }

    // get the text format
    UINT textFormat = SetTextFormat( nFlags );

    //
    // clear the backgound before rendering text?
    //
    if( nFlags & TEXT_CLEAR_BACKGROUND )
    {
        m_pDC->SetBkMode( OPAQUE );
    }
    else
    {
        m_pDC->SetBkMode( TRANSPARENT );
    }

    // render the text
    m_pDC->DrawText( text, textLength, &textRect, textFormat );
}


//-----------------------------------------------------------------------------
// Purpose: This function given the 2DView paramters tranforms a 3D world point
//          into the 2DView coordinate system.
//   Input: ptOut - the transformed 2D point (floating point representation)
//          ptIn - the 3D world point
//-----------------------------------------------------------------------------
void CRender2D::TransformPoint3D( Vector2D &ptOut, const Vector &ptIn )
{
    ptOut[0] = ptIn[m_HorzIndex];
    ptOut[1] = ptIn[m_VertIndex];
	
	if( m_bInvertHorz )
	{
		ptOut[0] = -ptOut[0];
	}
	
	if( m_bInvertVert )
	{
		ptOut[1] = -ptOut[1];
	}

	ptOut[0] *= m_Zoom;
	ptOut[1] *= m_Zoom;
}


//-----------------------------------------------------------------------------
// Purpose: This function given the 2DView paramters tranforms a 3D world point
//          into the 2DView coordinate system.
//   Input: ptOut - the transformed 2D point (integer screen coords)
//          ptIn - the 3D world point
//-----------------------------------------------------------------------------
void CRender2D::TransformPoint3D( CPoint &ptOut, const Vector &ptIn )
{
    Vector2D tmp;

    tmp[0] = ptIn[m_HorzIndex];
    tmp[1] = ptIn[m_VertIndex];

    if( m_bInvertHorz )
    {
        tmp[0] = -tmp[0];
    }

    if( m_bInvertVert )
    {
        tmp[1] = -tmp[1];
    }

    tmp[0] *= m_Zoom;
    tmp[1] *= m_Zoom;

	ptOut.x = tmp[0];
	ptOut.y = tmp[1];
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pen matching the requested style.
// Input  : penInfo - 
// Output : 
//-----------------------------------------------------------------------------
CPen *CRender2D::GetPen(LOGPEN &penInfo)
{
	//
	// Check the cache for the requested pen. Currently the cache consists of
	// the current pen and the previous pen.
	//
	for (int i = 0; i < sizeof(m_pPenCache) / sizeof(m_pPenCache[0]); i++)
	{
		if (m_pPenCache[i] != NULL)
		{
			LOGPEN penInfoOld;
			m_pPenCache[i]->GetLogPen(&penInfoOld);

			if ((penInfoOld.lopnStyle == penInfo.lopnStyle) &&
				(penInfoOld.lopnColor == penInfo.lopnColor) &&
				(penInfoOld.lopnWidth.x == penInfo.lopnWidth.x))
			{
				// It's in the cache, use the one from the cache.
				return m_pPenCache[i];
			}
		}
	}

	//
	// Not found in the cache. Make room in the cache for a new pen.
	//
	delete m_pPenCache[1];
	m_pPenCache[1] = m_pPenCache[0];

	//
	// Create a new pen with the requested line style and stick it in
	// the cache.
	//
	m_pPenCache[0] = new CPen;
	if (m_pPenCache[0] != NULL)
	{
		m_pPenCache[0]->CreatePenIndirect(&penInfo);
	}

	return m_pPenCache[0];
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRender2D::FreePenCache()
{
	for (int i = 0; i < sizeof(m_pPenCache) / sizeof(m_pPenCache[0]); i++)
	{
		delete m_pPenCache[i];
	}
}
