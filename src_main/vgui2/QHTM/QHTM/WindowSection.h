/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	WindowSection.h
Owner:	leea@gipsysoft.com
Purpose:	Manages funnneling windows messages into CSectionABC
----------------------------------------------------------------------*/
#ifndef WINDOWSECTION_H
#define WINDOWSECTION_H


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef SECTIONABC_H
	#include "sectionabc.h"
#endif

class CWindowSection : public CParentSection
{
public:
	enum WindowType { APP_WINDOW = 1 , TIP_WINDOW };

	CWindowSection( WindowType type );
	virtual ~CWindowSection();

	virtual bool RegisterWindow();

	virtual bool Create( const CRect &rcPos );
	virtual void SizeWindow( const CSize &size );


	virtual void OnDestroy();
	virtual void OnWindowDestroyed();

	virtual void ShowWindow( int nCmdShow );

	void OnMouseMove( const CPoint &pt );
	void OnTimer( int nTimerID );

	void DestroyWindow();

	virtual void ForceRedraw( const CRect &rc );
	virtual void DrawNow();

	void GetDesktopRect( CRect &rc );

	inline HWND GetHwnd() const { return m_hwnd; }

	static LPCTSTR m_pcszClassName;
protected:
	//
	//	Setting timers
	int RegisterTimerEvent( CSectionABC *pSect, int nInterval );
	void UnregisterTimerEvent( const int nTimerEventID );

	HWND	m_hwnd;
	int m_nMouseMoveTimerID;
	
private:
	CWindowSection( const CWindowSection &);
	CWindowSection& operator =( const CWindowSection &);

	WindowType	m_nWindowType;

	Container::CMap<int, CSectionABC *> m_mapTimerEvents;
	int m_nNextTimerID;

	CRect m_rcCaptionRect;
	bool	m_bCaptionRectSet;

	CSectionABC *m_pCaptureSection;
	static LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
};

bool IsMouseDown();


#endif //WINDOWSECTION_H