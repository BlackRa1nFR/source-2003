//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
// This class is a message box that has two buttons, ok and cancel instead of
// just the ok button of a message box. We use a message box class for the ok button
// and implement another button here.
//
// $NoKeywords: $
//=============================================================================

#include <vgui/Cursor.h>
#include <vgui/IScheme.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/IVGUI.h>

#include <vgui_controls/HTML.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Image.h>
#include <vgui_controls/ScrollBar.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

enum 
{
	SCROLLBAR_SIZE=18,  // the width of a scrollbar
	WINDOW_BORDER_WIDTH=1
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
HTML::HTML(Panel *parent, const char *name) : Panel(parent, name)
{	
	browser = surface()->CreateHTMLWindow(this, GetVPanel());
	Assert(browser != NULL);
	m_iNextFrameTime=0;
	m_iAnimTime=0;
	loading=NULL;
	picture=NULL;
	m_iScrollBorderX=m_iScrollBorderY=0;
	m_iScrollX=m_iScrollY=0;
	m_bScrollBarEnabled = true;
	m_bContextMenuEnabled = true;


	_hbar = new ScrollBar(this, "HorizScrollBar", false);
	_hbar->SetVisible(false);
	_hbar->AddActionSignalTarget(this);

	_vbar = new ScrollBar(this, "VertScrollBar", true);
	_vbar->SetVisible(false);
	_vbar->AddActionSignalTarget(this);

	m_bRegenerateHTMLBitmap = true;
	SetEnabled(true);
	SetVisible(true);

	if ( IsProportional() ) 
	{
		int width, height;
		int sw,sh;
		surface()->GetProportionalBase( width, height );
		surface()->GetScreenSize(sw, sh);
		
		m_iScrollbarSize = static_cast<int>( static_cast<float>( SCROLLBAR_SIZE )*( static_cast<float>( sw )/ static_cast<float>( width )));

	}
	else
	{
		m_iScrollbarSize = SCROLLBAR_SIZE;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
HTML::~HTML()
{
	surface()->DeleteHTMLWindow(browser);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HTML::ApplySchemeSettings(IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
	SetBorder(pScheme->GetBorder( "BrowserBorder"));
}

//-----------------------------------------------------------------------------
// Purpose: Causes the HTML window to repaint itself every 100ms, to allow animaited gifs and the like
//-----------------------------------------------------------------------------
void HTML::StartAnimate(int time)
{
	// a tick signal to let the web page re-render itself, in case of animated images
	//ivgui()->AddTickSignal(GetVPanel());
	m_iAnimTime=time;

}

//-----------------------------------------------------------------------------
// Purpose: stops the repainting
//-----------------------------------------------------------------------------
void HTML::StopAnimate()
{
	m_iNextFrameTime=0xffffffff; // next update is at infinity :)
	m_iAnimTime=0;

}

//-----------------------------------------------------------------------------
// Purpose: overrides panel class, paints a texture of the HTML window as a background
//-----------------------------------------------------------------------------
void HTML::PaintBackground()
{
	BaseClass::PaintBackground();

	if (m_bRegenerateHTMLBitmap)
	{
		surface()->PaintHTMLWindow(browser);
		m_bRegenerateHTMLBitmap = false;
	}

	// the window is a textured background
	picture = browser->GetBitmap();	
	if (picture)
	{
		surface()->DrawSetColor(GetBgColor());

		picture->SetPos(0,0);
		picture->Paint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: causes a repaint when the layout changes
//-----------------------------------------------------------------------------
void HTML::PerformLayout()
{
	BaseClass::PerformLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: calculates the need for and position of both horizontal and vertical scroll bars
//-----------------------------------------------------------------------------
void HTML::CalcScrollBars(int w, int h)
{
	int img_w, img_h;
	picture = browser->GetBitmap();
	if (m_bScrollBarEnabled && picture != NULL)
	{
		browser->GetHTMLSize(img_w, img_h);

		if (img_h > h)
		{
			//!! need to make it recalculate scroll positions
			_vbar->SetVisible(true);
			_vbar->SetEnabled(false);
			_vbar->SetRangeWindow( h/2-5 );
			_vbar->SetRange( 0, img_h);	
			_vbar->SetButtonPressedScrollValue( 5 );

			_vbar->SetPos(w - (m_iScrollbarSize+WINDOW_BORDER_WIDTH), WINDOW_BORDER_WIDTH);
			if(img_w>w)
			{
				_vbar->SetSize(m_iScrollbarSize, h-m_iScrollbarSize-1-WINDOW_BORDER_WIDTH);
			}
			else
			{
				_vbar->SetSize(m_iScrollbarSize, h-1-WINDOW_BORDER_WIDTH);
			}
			m_iScrollBorderX=m_iScrollbarSize+WINDOW_BORDER_WIDTH;
		}
		else
		{
			m_iScrollBorderX=0;
			_vbar->SetVisible(false);

		}

		if(img_w>w)
		{
			//!! need to make it recalculate scroll positions
			_hbar->SetVisible(true);
			_hbar->SetEnabled(false);
			_hbar->SetRangeWindow( w/2-5 );
			_hbar->SetRange( 0, img_w);	
			_hbar->SetButtonPressedScrollValue( 5 );

			_hbar->SetPos(WINDOW_BORDER_WIDTH,h-(m_iScrollbarSize+WINDOW_BORDER_WIDTH));
			if(img_h>h)
			{
				_hbar->SetSize(w-m_iScrollbarSize-WINDOW_BORDER_WIDTH,m_iScrollbarSize);
			}
			else
			{
				_hbar->SetSize(w-WINDOW_BORDER_WIDTH,m_iScrollbarSize);	
			}
			
			m_iScrollBorderY=m_iScrollbarSize+WINDOW_BORDER_WIDTH+1;

		}
		else
		{
			m_iScrollBorderY=0;
			_hbar->SetVisible(false);

		}
	}
	else
	{
		_vbar->SetVisible(false);
		_hbar->SetVisible(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: opens the URL, will accept any URL that IE accepts
//-----------------------------------------------------------------------------
void HTML::OpenURL(const char *URL)
{
	browser->OpenURL(URL);
}

//-----------------------------------------------------------------------------
// Purpose: opens the URL, will accept any URL that IE accepts
//-----------------------------------------------------------------------------
bool HTML::StopLoading()
{
	return browser->StopLoading();
}

//-----------------------------------------------------------------------------
// Purpose: refreshes the current page
//-----------------------------------------------------------------------------
bool HTML::Refresh()
{
	return browser->Refresh();
}

//-----------------------------------------------------------------------------
// Purpose: empties the current HTML container of any HTML text (used in conjunction with AddText)
//-----------------------------------------------------------------------------
void HTML::Clear()
{
	browser->Clear();
}

//-----------------------------------------------------------------------------
// Purpose: appends "text" to the end of the current page. "text" should be a HTML formatted string
//-----------------------------------------------------------------------------
void HTML::AddText(const char *text)
{
	browser->AddText(text);
}

//-----------------------------------------------------------------------------
// Purpose: handle resizing
//-----------------------------------------------------------------------------
void HTML::OnSizeChanged(int wide,int tall)
{
	BaseClass::OnSizeChanged(wide,tall);
	CalcScrollBars(wide,tall);

	int x,y;
	GetPos(x,y);

	if(browser) browser->OnSize(m_iScrollX,m_iScrollY,wide-m_iScrollBorderX,tall-m_iScrollBorderY);
	m_bRegenerateHTMLBitmap = true;
//	InvalidateLayout();
	Repaint();
}


//-----------------------------------------------------------------------------
// Purpose: used for the animation calls above, to repaint the screen
//			periodically. ( NOT USED !!!)
//-----------------------------------------------------------------------------
void HTML::OnTick()
{
	if (IsVisible() && m_iAnimTime && system()->GetTimeMillis() >= m_iNextFrameTime)
	{
		m_iNextFrameTime = system()->GetTimeMillis() + 	m_iAnimTime;
		m_bRegenerateHTMLBitmap = true;
		Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: passes mouse clicks to the control
//-----------------------------------------------------------------------------
void HTML::OnMousePressed(MouseCode code)
{
	// ask for the focus to come to this window
	RequestFocus();

	// now tell the browser about the click
	// ignore right clicks if context menu has been disabled
	if (code != MOUSE_RIGHT || m_bContextMenuEnabled)
	{
		if (browser) 
		{
			browser->OnMouse(code,IHTML::DOWN,m_iMouseX,m_iMouseY);
		}
	}
	m_bRegenerateHTMLBitmap = true;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: passes mouse up events
//-----------------------------------------------------------------------------
void HTML::OnMouseReleased(MouseCode code)
{
	if(browser) browser->OnMouse(code,IHTML::UP,m_iMouseX,m_iMouseY);
	m_bRegenerateHTMLBitmap = true;
	Repaint();

}

//-----------------------------------------------------------------------------
// Purpose: keeps track of where the cursor is
//-----------------------------------------------------------------------------
void HTML::OnCursorMoved(int x,int y)
{
	MouseCode code=MOUSE_LEFT;
	m_iMouseX=x;
	m_iMouseY=y;
	if(browser) browser->OnMouse(code,IHTML::MOVE,x,y);
}

//-----------------------------------------------------------------------------
// Purpose: passes double click events to the browser
//-----------------------------------------------------------------------------
void HTML::OnMouseDoublePressed(MouseCode code)
{
	if(browser) browser->OnMouse(code,IHTML::DOWN,m_iMouseX,m_iMouseY);
	m_bRegenerateHTMLBitmap = true;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: passes key presses to the browser (we don't current do this)
//-----------------------------------------------------------------------------
void HTML::OnKeyTyped(wchar_t unichar)
{
	// the OnKeyCodeDown member handles this
//	if(browser) browser->OnChar(unichar);
//	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: passes key presses to the browser 
//-----------------------------------------------------------------------------
void HTML::OnKeyCodePressed(KeyCode code)
{
	RequestFocus();
	if( code == KEY_PAGEDOWN || code == KEY_SPACE)
	{
		int val = _vbar->GetValue();
		val += 200;
		_vbar->SetValue(val);
	}
	else if ( code == KEY_PAGEUP )
	{
		int val = _vbar->GetValue();
		val -= 200;
		_vbar->SetValue(val);
		
	}

	if(browser) browser->OnKeyDown(code);
	m_bRegenerateHTMLBitmap = true;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: scrolls the vertical scroll bar on a web page
//-----------------------------------------------------------------------------
void HTML::OnMouseWheeled(int delta)
{	
	if (_vbar)
	{
		int val = _vbar->GetValue();
		val -= (delta * 25);
		_vbar->SetValue(val);
	}
}


//-----------------------------------------------------------------------------
// Purpose: gets called when a URL is first being loaded
// Return: return TRUE to continue loading, FALSE to stop this URL loading.
//-----------------------------------------------------------------------------
bool HTML::OnStartURL(const char *url,bool first)
{
	if( IsVisible() )
	{
		SetCursor(dc_arrow);
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: gets called when a URL is finished loading
//-----------------------------------------------------------------------------
void HTML::OnFinishURL(const char *url)
{
	int w, h;
	GetSize(w, h);
	CalcScrollBars(w, h);

	// reset the scroll bar positions
	_vbar->SetValue(0);
	_hbar->SetValue(0);
	m_iScrollX = m_iScrollY = 0;
	if (browser) 
	{
		browser->OnSize(m_iScrollX,m_iScrollY,w-m_iScrollBorderX,h-m_iScrollBorderY);
	}

	m_bRegenerateHTMLBitmap = true; // repaint the window, as we have a new picture to show
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: gets called while a URL is loading
//-----------------------------------------------------------------------------
void HTML::OnProgressURL(long current, long maximum)
{

}

//-----------------------------------------------------------------------------
// Purpose: gets called with status text from IE as the page loads
//-----------------------------------------------------------------------------
void HTML::OnSetStatusText(const char *text)
{

}

//-----------------------------------------------------------------------------
// Purpose: get called when IE wants us to redraw
//-----------------------------------------------------------------------------
void HTML::OnUpdate()
{
	int w,h;
	GetSize(w,h);
	CalcScrollBars(w,h);

	// only let it redraw every m_iAnimTime milliseconds, so stop it sucking up all the CPU time
	if (m_iAnimTime && system()->GetTimeMillis() >= m_iNextFrameTime)
	{
		m_iNextFrameTime = system()->GetTimeMillis() + m_iAnimTime;
		m_bRegenerateHTMLBitmap = true;
		Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: get called when the cursor moved over a valid URL on the page
//-----------------------------------------------------------------------------
void HTML::OnLink()
{
	if( IsVisible() )
	{
		SetCursor(dc_hand);
	}
}

//-----------------------------------------------------------------------------
// Purpose: get called when the cursor leaves a valid URL
//-----------------------------------------------------------------------------
void HTML::OffLink()
{
	if( IsVisible() )
	{
		SetCursor(dc_arrow);
	}
}

//-----------------------------------------------------------------------------
// Purpose: when a slider moves causes the IE images to re-render itself
//-----------------------------------------------------------------------------
void HTML::OnSliderMoved()
{
	if(_hbar->IsVisible())
	{
		m_iScrollX=_hbar->GetValue();
	}
	else
	{
		m_iScrollX=0;
	}

	if(_vbar->IsVisible())
	{
		m_iScrollY=_vbar->GetValue();
	}
	else
	{
		m_iScrollY=0;
	}
	int wide,tall;
	GetSize(wide,tall);

	if (browser) 
	{
		browser->OnSize(m_iScrollX,m_iScrollY,wide-m_iScrollBorderX,tall-m_iScrollBorderY);
	}
	
	m_bRegenerateHTMLBitmap = true;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void HTML::SetScrollbarsEnabled(bool state)
{
	m_bScrollBarEnabled = state;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void HTML::SetContextMenuEnabled(bool state)
{
	m_bContextMenuEnabled = state;
}

//-----------------------------------------------------------------------------
// Purpose: message map
//-----------------------------------------------------------------------------
MessageMapItem_t HTML::m_MessageMap[] =
{
	MAP_MESSAGE( HTML, "ScrollBarSliderMoved", OnSliderMoved ),
};
IMPLEMENT_PANELMAP(HTML, Panel);
