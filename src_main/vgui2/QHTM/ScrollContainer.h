/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	ScrollContainer.h
Owner:	russf@gipsysoft.com
Purpose:	General .
----------------------------------------------------------------------*/
#ifndef SCROLLCONTAINER_H
#define SCROLLCONTAINER_H

#ifndef SECTIONABC_H
	#include "SectionABC.h"
#endif	//	SECTIONABC_H

class CScrollContainer : public CParentSection
{
public:
	CScrollContainer( CParentSection *psect );
	virtual ~CScrollContainer();

	virtual void OnLayout( const CRect &rc );
	void OnDraw( CDrawContext &dc );

	//	Reset the sroll container
	void Reset( int nMaxWidth, int nMaxHeight );

	//	Scroll vertically the amount specificed
	void ScrollV( int nAmount );

	//	Scroll horizontally the amount specificed
	void ScrollH( int nAmount );

	//	Get the current scroll position
	int GetScrollPos() const;
	int GetScrollPosH() const;

	//	Set the absolute position
	void SetPos( int nPos );
	void SetPosH( int nPos );
	//	Ignores extrema
	void SetPosAbsolute( int nHPos, int nVPos );

	//	Determine if the list can scroll up/down any further
	bool CanScrollUp() const;
	bool CanScrollDown() const;

	//	Set whether or not the section will scroll or whether it will move the subsections
	//	and refresh
	inline void SetUsingScroll( bool bUseScroll ) { m_bUseScroll = bUseScroll; }

	//	Determine if the list can scroll left/right any further
	bool CanScrollLeft() const;
	bool CanScrollRight() const;

private:
	void InternalScroll( int cx, int cy, bool bScroll );
	void InternalScrollAbsolute( int cx, int cy, bool bScroll );
	void DoInternalScroll( int nDeltaX, int nDeltaY, bool bScroll );

	int m_nScrollPos, m_nLastScrollPos;
	int m_nMaxHeight;

	int m_nScrollPosH, m_nLastScrollPosH;
	int m_nMaxWidth;

	bool m_bUseScroll;

private:
	CScrollContainer();
	CScrollContainer( const CScrollContainer & );
	CScrollContainer& operator =( const CScrollContainer & );
};

#endif //SCROLLCONTAINER_H
