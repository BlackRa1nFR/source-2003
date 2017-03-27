/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
File:	DrawContext.h
Owner:	russf@gipsysoft.com
Purpose:	Drawing primitives and adbstraction layer.
----------------------------------------------------------------------*/
#ifndef DRAWCONTEXT_H
#define DRAWCONTEXT_H

#ifndef QHTM_INLCUDES_H
	#include "QHTM_Includes.h"
#endif	//	QHTM_INLCUDES_H

#ifndef QHTM_TYPES_H
	#include "QHTM_Types.h"
#endif	//	QHTM_TYPES_H

#ifndef FONTDEF_H
	#include "FontDef.h"
#endif	//	FONTDEF_H

class CDrawContext
{
public:
	explicit CDrawContext( const CRect *prcClip = NULL, HDC hdc = NULL, bool bIsPrinting = false );
	virtual ~CDrawContext();

	//	Fill a rectangle with a colour
	void FillRect( const CRect &rc, COLORREF cr );

	//	Fill a polygon with a colout and outline it.
	void PolyFillOutlined( const POINT *pt, int nCount, COLORREF cr, COLORREF crOutline );
	
	//	Draw a conected line with multiple points.
	void PolyLine( const POINT *pt, int nCount, COLORREF cr );

	//	Draw a rectangle frame
	void Rectangle( const CRect &rc, COLORREF cr );

	//	Draw some text given a colour and using the current font.
	void DrawText( int x, int y, LPCTSTR pcszText, int nLength, COLORREF crFore );

	//	Select a non-standard font 
	void SelectFont( LPCTSTR pcszFontNames, int nSizePixels, int nWeight, bool bItalic, bool bUnderline, bool bStrike, BYTE cCharSet );

	//	Select a standard font
	void SelectFont( const FontDef &fdef );

	
	//	Get the average height of the font
	inline int GetCurrentFontHeight() const { ASSERT( m_pFontInfo ); /*lint -e613*/return m_pFontInfo->m_nLineSpace; /*lint +e613*/}

	//	Get the baseline for the font
	inline int GetCurrentFontBaseline() const { ASSERT( m_pFontInfo ); /*lint -e613*/ return m_pFontInfo->m_nBaseline; /*lint +e613*/}
	

	//	Get the length of some text using teh currently selected font.
	int GetTextExtent( LPCTSTR pcszText, int nLength );

	//	Get the text that fits within the length given, return the 'size' of the text object
	//	needed.
	//	Returns true if all of the text fits. If this is the case then pcszText points
	//	to the next text needed to be rendered.
	bool GetTextFitWithWordWrap( int nMaxWidth, LPCTSTR &pcszText, CSize &size, bool bStartOfLine = false ) const;

	//	Get the clip rectangle, this is the area of display we need to draw in.
	const CRect &GetClipRect() const;

	//
	//	Clip drawing to a particlar rectangle
	void SetClipRect( const CRect &rc );
	void RemoveClipRect();

	//	Access the underlying device context
	HDC GetSafeHdc();

	//	Easyscaling for non-screen display
	void SetScaling( int cxDeviceScaleNumer, int cxDeviceScaleDenom, int cyDeviceScaleNumer, int cyDeviceScaleDenom );
	int ScaleX(int x) const;
	int ScaleY(int y) const;
	CSize Scale(const CSize& size) const;

	inline bool IsPrinting() const { return m_bIsPrinting; }

protected:
	bool m_bWeCreatedDC;
	HDC m_hdc;
	CRect m_rcClip;
	int	m_cxDeviceScaleNumer, m_cxDeviceScaleDenom;
	int	m_cyDeviceScaleNumer, m_cyDeviceScaleDenom;

private:
	void SelectBrush( COLORREF cr );
	void SelectPen( COLORREF cr );
	HBRUSH GetBrush( COLORREF cr );
	HPEN GetPen( COLORREF cr );

	//	This must be first
	Container::CArray<HGDIOBJ> m_arrObjToDelete;

	struct FontInfo
	{
		HFONT hFont;
		int m_nWidths[ 256];
		int m_nOverhang[ 256];
		int m_nBaseline;
		int m_nLineSpace;
	};
	FontInfo * GetFont( const FontDef &fdef );
	Container::CMap<FontDef, FontInfo*> m_mapFonts;
	Container::CMap< COLORREF, HBRUSH> m_mapBrushes;
	Container::CMap< COLORREF, HPEN> m_mapPens;

	FontInfo *m_pFontInfo;

	bool m_bBrushSelected;
	COLORREF m_crCurrentBrush;

	bool m_bPenSelected;
	COLORREF m_crCurrentPen;

	COLORREF m_crCurrentText;
	bool m_bIsPrinting;
private:
	CDrawContext( const CDrawContext &);
	CDrawContext& operator =( const CDrawContext &);
};


class CBufferedDC : public CDrawContext
//
//	Buffered device context
{
public:
	explicit CBufferedDC( CDrawContext &dc )
		: CDrawContext( &dc.GetClipRect(), CreateCompatibleDC( dc.GetSafeHdc() ) )
		, m_dc( dc )
		, rcClip( dc.GetClipRect() )
		, m_sizeClip( dc.GetClipRect().Size() )
		, m_hbm( CreateCompatibleBitmap( dc.GetSafeHdc(), m_sizeClip.cx, m_sizeClip.cy ) )
	{
		m_hbm = ::SelectObject( GetSafeHdc(), m_hbm );
		VERIFY( SetWindowOrgEx( GetSafeHdc(), rcClip.left, rcClip.top, &m_ptOldOrg ) );
		(void)SelectPalette( GetSafeHdc(), GetCurrentWindowsPalette(), TRUE );
		(void)RealizePalette( GetSafeHdc() );
	}

	~CBufferedDC()
	{
		VERIFY( SetWindowOrgEx( GetSafeHdc(), m_ptOldOrg.x, m_ptOldOrg.y, NULL ) );

		::BitBlt( m_dc.GetSafeHdc(), rcClip.left,  rcClip.top, m_sizeClip.cx, m_sizeClip.cy, GetSafeHdc(), 0, 0, SRCCOPY );
		VAPI( ::DeleteObject( ::SelectObject( GetSafeHdc(), m_hbm ) ) );

		VAPI( ::RestoreDC( m_hdc, -1 ) );
		::DeleteDC( m_hdc );
		m_hdc = NULL;
		m_hbm = NULL;
	}

private:
	POINT m_ptOldOrg;
	/*lint -e1725 */
	CDrawContext &m_dc;
	/*lint +e1725 */

	const CRect rcClip;
	const CSize m_sizeClip;
	HGDIOBJ m_hbm;

private:
	CBufferedDC();
	CBufferedDC( const CBufferedDC &);
	CBufferedDC & operator =( const CBufferedDC &);
};


#endif //DRAWCONTEXT_H
