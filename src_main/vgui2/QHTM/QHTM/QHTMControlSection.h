/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	QHTMControlSection.h
Owner:	russf@gipsysoft.com
Purpose:	QHTM control.
----------------------------------------------------------------------*/
#ifndef QHTMCONTROLSECTION_H
#define QHTMCONTROLSECTION_H

#ifndef WINDOWSECTION_H
	#include "WindowSection.h"
#endif //#ifndef WINDOWSECTION_H

#ifndef HTMLSECTION_H
	#include "HTMLSection.h"
#endif //#ifndef HTMLSECTION_H

#ifndef DEFAULTS_H
	#include "Defaults.h"
#endif	//	DEFAULTS_H

class CQHTMControlSection : public CWindowSection
{
public:
	CQHTMControlSection( HWND hwnd );
	virtual ~CQHTMControlSection();
	virtual void OnLayout( const CRect &rc );
	virtual bool OnNotify( const CSectionABC *pChild, const int nEvent );

	void SetText( LPCTSTR pcszText );
	
	//	Called when the HTML is required to be loaded from the resources.
	bool LoadFromResource( HINSTANCE hInst, LPCTSTR pcszName );

	//	Called when the HTML is required to be loaded from a file.
	bool LoadFromFile( LPCTSTR pcszFilename );

	void OnExecuteHyperlink( LPCTSTR pcszLink );
	void OnVScroll( int nScrollCode );
	void OnHScroll( int nScrollCode );

	//
	//	Get the size of the HTML
	CSize GetSize() const { return m_htmlSection.GetSize(); }

	//	Set an option
	int SetOption( UINT uOptionIndex, LPARAM lParam );

	//	Get an option
	LPARAM GetOption( UINT uOptionIndex, LPARAM lParam );

	//	Force the control to jump to a link
	void GotoLink( LPCTSTR pcszLinkName );

	//
	//	Get the length of the HTML title
	UINT GetTitleLength() const;

	//
	//	Get the HTML title
	UINT GetTitle( UINT uBufferLength, LPTSTR pszBuffer ) const;

	bool IsLayingOut() const { return m_bLayingOut; }

	//
	//	Set the font to use
	void SetFont( HFONT hFont );


	//	Get/Set the scroll position of the control.
	void SetScrollPos( int n )
	{
		::SetScrollPos( m_hwnd, SB_VERT, n, TRUE );
		m_htmlSection.SetPos( n );
		InvalidateRect( m_hwnd, NULL, TRUE );
	}
	int GetScrollPos( ) { return m_htmlSection.GetScrollPos(); }

	static LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );

	bool GetBackgroundColours( HDC hdc, HBRUSH &hbr ) const;

private:

	//
	//	!!!! Needs to be first due to construction order !!!!
	CDefaults m_defaults;

	void LayoutHTML();
	void ResetScrollPos();

	class CHTMLSection2 : public CHTMLSection
	{
	public:
		explicit CHTMLSection2( CQHTMControlSection *psectParent, CDefaults *pDefaults ) : CHTMLSection( psectParent, pDefaults ), m_psectParent( psectParent ) {}

		virtual void OnExecuteHyperlink( LPCTSTR pcszLink )
		{
			static_cast<CQHTMControlSection *>( GetParent() )->OnExecuteHyperlink( pcszLink );
		}
		virtual bool GetBackgroundColours( HDC hdc, HBRUSH &hbr ) const
		{
			return m_psectParent->GetBackgroundColours( hdc, hbr );
		}

	private:
		CQHTMControlSection *m_psectParent;
		CHTMLSection2();
		CHTMLSection2( const CHTMLSection2 &);
		CHTMLSection2& operator =( const CHTMLSection2 &);
	};

	CHTMLSection2	m_htmlSection;

	bool m_bEnableTooltips;
	bool m_bLayingOut;
	bool m_bUseColorStatic;
	bool m_bEnableScrollbars;

private:
	CQHTMControlSection( const CQHTMControlSection &);
	CQHTMControlSection &operator =( const CQHTMControlSection &);
};


#endif //QHTMCONTROLSECTION_H