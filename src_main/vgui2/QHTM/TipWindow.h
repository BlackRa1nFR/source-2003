/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	TipWindow.h
Owner:	leea@gipsysoft.com
Purpose:	Tool tips window
----------------------------------------------------------------------*/
#ifndef TIPWINDOW_H
#define TIPWINDOW_H

#ifndef WINDOWSECTION_H
	#include "WindowSection.h"
#endif //#ifndef WINDOWSECTION_H

#ifndef HTMLSECTION_H
	#include "HTMLSection.h"
#endif //#ifndef HTMLSECTION_H

#ifndef DEFAULTS_H
	#include "defaults.h"
#endif //#ifndef DEFAULTS_H

class CTipWindow : public CWindowSection
{
public:
	CTipWindow( LPCTSTR pcszTip, CPoint &pt  );

	void OnDraw( CDrawContext &dc );

	void OnDestroy();
	void OnWindowDestroyed();
	void OnLayout( const CRect &rc );
	virtual void OnMouseMove( const CPoint &pt );

	void Delete();

	static DWORD	LastTipCreated() { return m_nLastTipCreated; }

private:
	CTipWindow( const CTipWindow &);
	CTipWindow& operator =( const CTipWindow &);
	virtual ~CTipWindow();

	//	MUST BE FIRST
	CDefaults m_defaults;

	CHTMLSection	m_htmlSection;

	static DWORD	m_nLastTipCreated;
	StringClass	m_strTip;
};

#endif //TIPWINDOW_H