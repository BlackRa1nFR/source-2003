//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <UtlVector.h>

#include <vgui/Cursor.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>

#include <vgui_controls/Menu.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

enum
{
	// maximum size of text buffer
	BUFFER_SIZE=999999,
	SCROLLBAR_SIZE=18,  // the width of a scrollbar
};

using namespace vgui;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

namespace vgui
{
	
	//-----------------------------------------------------------------------------
	// Purpose: Panel used for clickable URL's
	//-----------------------------------------------------------------------------
	class ClickPanel : public Panel
	{
	public:
		ClickPanel(Panel *parent)
		{
			_textIndex = 0;
			SetParent(parent);
			AddActionSignalTarget(parent);
			
			SetCursor(dc_hand);
			
			SetPaintBackgroundEnabled(false);
			SetPaintEnabled(false);
			SetPaintBorderEnabled(false);
		}
		
		void setTextIndex(int index)
		{
			_textIndex = index;
		}
		
		int getTextIndex()
		{
			return _textIndex;
		}
		
		void OnMousePressed(MouseCode code)
		{
			if (code == MOUSE_LEFT)
			{
				PostActionSignal(new KeyValues("ClickPanel", "index", _textIndex));
			}
		}
		
	private:
		int _textIndex;
		
	};
	
};	// namespace vgui

static const int DRAW_OFFSET_X = 3,DRAW_OFFSET_Y = 1; 


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TextEntry::TextEntry(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
	_font = INVALID_FONT;

	m_bAllowNumericInputOnly = false;
	_hideText = false;
	_editable = false;
	_verticalScrollbar = false;
	_cursorPos = 0;
	_currentStartIndex = 0;
	_horizScrollingAllowed = true;
	_cursorIsAtEnd = false;
	_putCursorAtEnd = false;
	_multiline = false;
	_cursorBlinkRate = 400;
	_mouseSelection = false;
	_mouseDragSelection = false;
	_vertScrollBar=NULL;
	_catchEnterKey = false;
	_maxCharCount = -1;
	_charCount = 0;
	_wrap = false; // don't wrap by default
	_sendNewLines = false; // don't pass on a newline msg by default
	_drawWidth = 0;

	//a -1 for _select[0] means that the selection is empty
	_select[0] = -1;
	_select[1] = -1;
	m_pEditMenu = NULL;
	
	//this really just inits it when in here	
	ResetCursorBlink();
	
	SetCursor(dc_ibeam);
	
	//position the cursor so it is at the end of the text
	GotoTextEnd();

	SetEditable(true);
	
	// initialize the line break array
	m_LineBreaks.AddToTail(BUFFER_SIZE);
	
	_recalculateBreaksIndex = 0;
	
	_selectAllOnFirstFocus = false;

	if ( IsProportional() )
	{
		int width, height;
		int sw,sh;
		surface()->GetProportionalBase( width, height );
		surface()->GetScreenSize(sw, sh);
		
		_scrollBarSize = static_cast<int>( static_cast<float>( SCROLLBAR_SIZE )*( static_cast<float>( sw )/ static_cast<float>( width )));

	}
	else
	{
		_scrollBarSize = SCROLLBAR_SIZE;
	}
}


TextEntry::~TextEntry()
{
	delete m_pEditMenu;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *inResourceData - 
//-----------------------------------------------------------------------------
void TextEntry::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	SetFgColor(GetSchemeColor("WindowFgColor", pScheme));
	SetBgColor(GetSchemeColor("WindowBgColor", pScheme));
	
	_cursorColor = GetSchemeColor("TextCursorColor", pScheme);
	_disabledFgColor = GetSchemeColor("WindowDisabledFgColor", pScheme);
	_disabledBgColor = GetSchemeColor("ControlBG", pScheme);
	
	_selectionTextColor = GetSchemeColor("SelectionFgColor", GetFgColor(), pScheme);
	_selectionColor = GetSchemeColor("SelectionBgColor", pScheme);
	_defaultSelectionBG2Color = GetSchemeColor("SelectionBG2", pScheme);
	_focusEdgeColor = GetSchemeColor("BorderSelection", Color(0, 0, 0, 0), pScheme);

	SetBorder( pScheme->GetBorder("ButtonDepressedBorder"));

	_font = pScheme->GetFont("Default", IsProportional() );
	SetFont( _font );
}

//-----------------------------------------------------------------------------
// Purpose: sets the color of the background when the control is disabled
// Input  : col - 
//-----------------------------------------------------------------------------
void TextEntry::SetDisabledBgColor(Color col)
{
	_disabledBgColor = col;
}


//-----------------------------------------------------------------------------
// Purpose: Sends a message if the data has changed
//          Turns off any selected text in the window if we are not using the edit menu
//-----------------------------------------------------------------------------
void TextEntry::OnKillFocus()
{
	if (_dataChanged)
	{
		FireActionSignal();
		_dataChanged = false;
	}
	
	// check if we clicked the right mouse button or if it is down
	bool mouseRightClicked = input()->WasMousePressed(MOUSE_RIGHT);
	bool mouseRightUp = input()->WasMouseReleased(MOUSE_RIGHT);
	bool mouseRightDown = input()->IsMouseDown(MOUSE_RIGHT);
	
	if (mouseRightClicked || mouseRightDown || mouseRightUp )
	{			
		int cursorX, cursorY;
		input()->GetCursorPos(cursorX, cursorY);

		// if we're right clicking within our window, we don't actually kill focus
		if (IsWithin(cursorX, cursorY))
			return;
	}
	
    if (IsEditable())
    {
    	// clear any selection
	    SelectNone();
    }

	// move the cursor to the start
//	GotoTextStart();

	// chain
	BaseClass::OnKillFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Wipe line breaks after the size of a panel has been changed
//-----------------------------------------------------------------------------
void TextEntry::OnSizeChanged(int newWide, int newTall)
{
	BaseClass::OnSizeChanged(newWide, newTall);

   	// blow away the line breaks list 
	_recalculateBreaksIndex = 0;
	m_LineBreaks.RemoveAll();
	m_LineBreaks.AddToTail(BUFFER_SIZE);

    // if we're bigger, see if we can scroll left to put more text in the window
    if (newWide > _drawWidth)
    {
        ScrollLeftForResize();
    }

	_drawWidth = newWide;
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: Set the text array - convert ANSI text to unicode and pass to unicode function
//-----------------------------------------------------------------------------
void TextEntry::SetText(const char *text)
{
	if (!text)
	{
		text = "";
	}

	if (text[0] == '#')
	{
		// check for localization
		wchar_t *wsz = localize()->Find(text);
		if (wsz)
		{
			SetText(wsz);
			return;
		}
	}

	wchar_t unicode[1024];
	localize()->ConvertANSIToUnicode(text, unicode, sizeof(unicode));
	SetText(unicode);
}

//-----------------------------------------------------------------------------
// Purpose: Set the text array
//          Using this function will cause all lineBreaks to be discarded.
//          This is because this fxn replaces the contents of the text buffer.
//          For modifying large buffers use insert functions.
//-----------------------------------------------------------------------------
void TextEntry::SetText(const wchar_t *wszText)
{
	int textLen = wcslen(wszText);
	m_TextStream.RemoveAll();
	m_TextStream.EnsureCapacity(textLen);

	int missed_count = 0;
	for (int i = 0; i < textLen; i++)
	{
		if(wszText[i]=='\r') // don't insert \r characters
		{
			missed_count++;
			continue;
		}
		m_TextStream.AddToTail(wszText[i]);
		SetCharAt(wszText[i], i-missed_count);
	}

	GotoTextStart();
	SelectNone();
	
	// reset the data changed flag
	_dataChanged = false;
	
	// blow away the line breaks list 
	_recalculateBreaksIndex = 0;
	m_LineBreaks.RemoveAll();
	m_LineBreaks.AddToTail(BUFFER_SIZE);
	
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the value of char at index position.
//-----------------------------------------------------------------------------
void TextEntry::SetCharAt(wchar_t ch, int index)
{
	if ((ch == '\n') || (ch == '\0')) 
	{
		// if its not at the end of the buffer it matters.
		// redo the linebreaks
		//if (index != m_TextStream.Count())
		{
			_recalculateBreaksIndex = 0;
			m_LineBreaks.RemoveAll();
			m_LineBreaks.AddToTail(BUFFER_SIZE);
		}
	}
	
	if (index < 0)
		return;

	if (index >= m_TextStream.Count())
	{
		m_TextStream.AddMultipleToTail(index - m_TextStream.Count() + 1);
	}
	m_TextStream[index] = ch;
	_dataChanged = true;
}

//-----------------------------------------------------------------------------
// Purpose: Restarts the time of the next cursor blink
//-----------------------------------------------------------------------------
void TextEntry::ResetCursorBlink()
{
	_cursorBlink=false;
	_cursorNextBlinkTime=system()->GetTimeMillis()+_cursorBlinkRate;
}

//-----------------------------------------------------------------------------
// Purpose: Hides the text buffer so it will not be drawn
//-----------------------------------------------------------------------------
void TextEntry::SetTextHidden(bool bHideText)
{
	_hideText = bHideText;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: return character width
//-----------------------------------------------------------------------------
int getCharWidth(HFont font, wchar_t ch)
{
	if (iswprint(ch))
	{
		int a, b, c;
		surface()->GetCharABCwide(font, ch, a, b, c);
		return (a + b + c);
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Given cursor's position in the text buffer, convert it to
//   the local window's x and y pixel coordinates
// Input: cursorPos: cursor index
// Output: cx, cy, the corresponding coords in the local window
//-----------------------------------------------------------------------------
void TextEntry::CursorToPixelSpace(int cursorPos, int &cx, int &cy)
{
	int yStart = GetYStart();
	
	int x = DRAW_OFFSET_X, y = yStart;
	_pixelsIndent = 0;
	int lineBreakIndexIndex = 0;
	
	for (int i = GetStartDrawIndex(lineBreakIndexIndex); i < m_TextStream.Count(); i++)
	{
		wchar_t ch = m_TextStream[i];
		if (_hideText)
		{
			ch = '*';
		}
		
		// if we've found the position, break
		if (cursorPos == i)
		{
			// even if this is a line break entry for the cursor, the next insert
			// will be at this position, which will push the line break forward one
			// so don't push the cursor down a line here...
			/*if (!_putCursorAtEnd)
			{		
				// if we've passed a line break go to that
				if (m_LineBreaks[lineBreakIndexIndex] == i)
				{
					// add another line
					AddAnotherLine(x,y);
					lineBreakIndexIndex++;
				}
			}*/
			break;
		}
		
		// if we've passed a line break go to that
		if (m_LineBreaks.Count() && 
			lineBreakIndexIndex < m_LineBreaks.Count() &&
			m_LineBreaks[lineBreakIndexIndex] == i)
		{
			// add another line
			AddAnotherLine(x,y);
			lineBreakIndexIndex++;
		}
		
		// add to the current position
		x += getCharWidth(_font, ch);
	}
	
	cx = x;
	cy = y;
}

//-----------------------------------------------------------------------------
// Purpose: Converts local pixel coordinates to an index in the text buffer
//          This function appears to be used only in response to mouse clicking
// Input  : cx - 
//			cy - pixel location
//-----------------------------------------------------------------------------
int TextEntry::PixelToCursorSpace(int cx, int cy)
{
	_putCursorAtEnd = false; //	Start off assuming we clicked somewhere in the text
	
	int fontTall = surface()->GetFontTall(_font);
	
	// where to Start reading
	int yStart = GetYStart();
	int x = DRAW_OFFSET_X, y = yStart;
	_pixelsIndent = 0;
	int lineBreakIndexIndex = 0;
	
	int startIndex = GetStartDrawIndex(lineBreakIndexIndex);
	bool onRightLine = false;
	for (int i = startIndex; i < m_TextStream.Count(); i++)
	{
		wchar_t ch = m_TextStream[i];
		if (_hideText)
		{
			ch = '*';
		}
		
		// if we are on the right line but off the end of if put the cursor at the end of the line
		if (m_LineBreaks[lineBreakIndexIndex] == i )
		{
			// add another line
			AddAnotherLine(x,y);
			lineBreakIndexIndex++;
			
			if (onRightLine)
			{	
				_putCursorAtEnd = true;
				return i;
			}
		}
		
		// check to see if we're on the right line
		if (cy < yStart)
		{
			// cursor is above panel
			onRightLine = true;
			_putCursorAtEnd = true;	// this will make the text scroll up if needed
		}
		else if (cy >= y && (cy < (y + fontTall + DRAW_OFFSET_Y)))
		{
			onRightLine = true;
		}
		
		int wide = getCharWidth(_font, ch);
		
		// if we've found the position, break
		if (onRightLine)
		{
			if (cx > GetWide())	  // off right side of window
			{
			}
			else if (cx < (DRAW_OFFSET_X + _pixelsIndent) || cy < yStart)	 // off left side of window
			{
				return i; // move cursor one to left
			}
			
			if (cx >= x && cx < (x + wide))
			{
				// check which side of the letter they're on
				if (cx < (x + (wide * 0.5)))  // left side
				{
					return i;
				}
				else  // right side
				{						 
					return i + 1;
				}
			}
		}
		x += wide;
	}
	
	return i;
}

//-----------------------------------------------------------------------------
// Purpose: Draws a character in the panel
// Input:	ch - character to draw
//			font - font to use
//			x, y - pixel location to draw char at
// Output:	returns the width of the character drawn
//-----------------------------------------------------------------------------
int TextEntry::DrawChar(wchar_t ch, HFont font, int index, int x, int y)
{
	// add to the current position
	int charWide = getCharWidth(font, ch);
	int fontTall=surface()->GetFontTall(font);
	if (iswprint(ch))
	{
		// draw selection, if any
		int selection0 = -1, selection1 = -1;
		GetSelectedRange(selection0, selection1);
		
		if (index >= selection0 && index < selection1)
		{
			// draw background selection color
            VPANEL focus = input()->GetFocus();
            // if one of the children of the SectionedListPanel has focus, then 'we have focus' if we're selected
            if (HasFocus() || (focus && ipanel()->HasParent(focus, GetVPanel())))
    			surface()->DrawSetColor(_selectionColor);
            else
    			surface()->DrawSetColor(_defaultSelectionBG2Color);

			surface()->DrawFilledRect(x, y, x + charWide, y + 1 + fontTall);
			
			// reset text color
			surface()->DrawSetTextColor(_selectionTextColor);
		}

		surface()->DrawSetTextPos(x, y);
		surface()->DrawUnicodeChar(ch);
		
		return charWide;
	}
	
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Draw the cursor, cursor is not drawn when it is blinked gone
// Input:  x,y where to draw cursor
// Output: returns true if cursor was drawn.
//-----------------------------------------------------------------------------
bool TextEntry::DrawCursor(int x, int y)
{
	if (!_cursorBlink)
	{
		int cx, cy;
		CursorToPixelSpace(_cursorPos, cx, cy);
		surface()->DrawSetColor(_cursorColor);
		int fontTall=surface()->GetFontTall(_font);
		surface()->DrawFilledRect(cx, cy, cx + 1, cy + fontTall);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the text in the panel
//-----------------------------------------------------------------------------
void TextEntry::PaintBackground()
{
	// draw background
	Color col;
	if (IsEnabled())
	{
		col = GetBgColor();
	}
	else
	{
		col = _disabledBgColor;
	}
	surface()->DrawSetColor(col);
	int wide, tall;
	GetSize( wide, tall );
	surface()->DrawFilledRect(0, 0, wide, tall);
	
	surface()->DrawSetTextFont(_font);
	if (IsEnabled())
	{
		col = GetFgColor();
	}
	else
	{
		col = _disabledFgColor;
	}
	surface()->DrawSetTextColor(col);
	_pixelsIndent = 0;
	
	int lineBreakIndexIndex = 0;
	int startIndex = GetStartDrawIndex(lineBreakIndexIndex);
	
	// where to Start drawing
	int x = DRAW_OFFSET_X + _pixelsIndent, y = GetYStart();
	
	// draw text with an elipsis
	if ( (!_multiline) && (!_horizScrollingAllowed) )
	{	
		int wide = DRAW_OFFSET_X; // buffer on left and right end of text.
	
		int endIndex = m_TextStream.Count();
		// In editable windows only do the elipsis if we don't have focus.
		// In non editable windows do it all the time.
		if ( (!HasFocus() && (IsEditable())) || (!IsEditable()) )
		{
			// loop through all the characters and sum their widths	
			bool addElipsis = false;
			for (int i = 0; i < m_TextStream.Count(); ++i)
			{	
				wide += getCharWidth(_font, m_TextStream[i]);
				if (wide > _drawWidth)
				{
					addElipsis = true;
					break;
				}
			}
			if (addElipsis)
			{
				int elipsisWidth = 3 * getCharWidth(_font, '.');
				while (elipsisWidth > 0 && i >= 0)
				{
					elipsisWidth -= getCharWidth(_font, m_TextStream[i]);
					i--;
				}
				endIndex = i + 1;	
			}

			// if we take off less than the last 3 chars we have to make sure
			// we take off the last 3 chars so selected text will look right.
			if (m_TextStream.Count() - endIndex < 3 && m_TextStream.Count() - endIndex > 0 )
			{
				endIndex = m_TextStream.Count() - 3;
			}
		}
		// draw the text
		for ( int i = startIndex; i < endIndex; i++)
		{
			wchar_t ch = m_TextStream[i];
			if (_hideText)
			{
				ch = '*';
			}
		
			// draw the character and update xposition  
			x += DrawChar(ch, _font, i, x, y);
		}
		if (endIndex < m_TextStream.Count()) // add an elipsis
		{
			x += DrawChar('.', _font, i, x, y);
			i++;
			x += DrawChar('.', _font, i, x, y);
			i++;
			x += DrawChar('.', _font, i, x, y);
			i++;
		}
	}	
	else
	{
		// draw the text
		for ( int i = startIndex; i < m_TextStream.Count(); i++)
		{
			wchar_t ch = m_TextStream[i];
			if (_hideText)
			{
				ch = '*';
			}
			
			// if we've passed a line break go to that
			if ( _multiline && m_LineBreaks[lineBreakIndexIndex] == i)
			{
				// add another line
				AddAnotherLine(x, y);
				lineBreakIndexIndex++;
			}
			
			// draw the character and update xposition  
			x += DrawChar(ch, _font, i, x, y);
		}
	}
	// custom border
	//!! need to replace this with scheme stuff (TextEntryBorder/TextEntrySelectedBorder)
	surface()->DrawSetColor(50, 50, 50, 255);
	
	if (IsEnabled() && IsEditable() && HasFocus())
	{
		// set a more distinct border color
		surface()->DrawSetColor(0, 0, 0, 255);
		
		DrawCursor (x, y);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when data changes or panel size changes
//-----------------------------------------------------------------------------
void TextEntry::PerformLayout()
{
	BaseClass::PerformLayout();

	RecalculateLineBreaks();
	
	// recalculate scrollbar position
	if (_verticalScrollbar)
	{
		LayoutVerticalScrollBarSlider();
	}
	
	// force a Repaint
	Repaint();
}

// moves x,y to the Start of the next line of text
void TextEntry::AddAnotherLine(int &cx, int &cy)
{
	cx = DRAW_OFFSET_X + _pixelsIndent;
	cy += (surface()->GetFontTall(_font) + DRAW_OFFSET_Y);
}


//-----------------------------------------------------------------------------
// Purpose: Recalculates line breaks
//-----------------------------------------------------------------------------
void TextEntry::RecalculateLineBreaks()
{
	if (!_multiline || _hideText)
		return;

	if (m_TextStream.Count() < 1)
		return;
	
	HFont font = _font;
	
	// line break to our width -2 pixel to keep cursor blinking in window
	// (assumes borders are 1 pixel)
	int wide = GetWide()-2;
	
	// subtract the scrollbar width
	if (_vertScrollBar)
	{
		wide -= _vertScrollBar->GetWide();
	}
	
	int charWidth;
	int x = DRAW_OFFSET_X, y = DRAW_OFFSET_Y;
		
	int wordStartIndex = 0;
	int wordLength = 0;
	bool hasWord = false;
	bool justStartedNewLine = true;
	bool wordStartedOnNewLine = true;
	
	int startChar;
	if (_recalculateBreaksIndex <= 0)
	{
		m_LineBreaks.RemoveAll();
		startChar=0;
	}
	else
	{
		// remove the rest of the linebreaks list since its out of date.
		for (int i=_recalculateBreaksIndex+1; i < m_LineBreaks.Count(); ++i)
		{
			m_LineBreaks.Remove((int)i);
			--i; // removing shrinks the list!
		}
		startChar = m_LineBreaks[_recalculateBreaksIndex];
	}
	
	// handle the case where this char is a new line, in that case
	// we have already taken its break index into account above so skip it.
	if (m_TextStream[startChar] == '\r' || m_TextStream[startChar] == '\n') 
	{
		startChar++;
	}
	
	// loop through all the characters	
	for (int i = startChar; i < m_TextStream.Count(); ++i)
	{
		wchar_t ch = m_TextStream[i];
		
		// line break only on whitespace characters
		if (!iswspace(ch))
		{
			if (hasWord)
			{
				// append to the current word
			}
			else
			{
				// Start a new word
				wordStartIndex = i;
				hasWord = true;
				wordStartedOnNewLine = justStartedNewLine;
				wordLength = 0;
			}
		}
		else
		{
			// whitespace/punctuation character
			// end the word
			hasWord = false;
		}
		
		// get the width
		charWidth = getCharWidth(font, ch);
		if (iswprint(ch))
		{
			justStartedNewLine = false;
		}
				
		// check to see if the word is past the end of the line [wordStartIndex, i)
		if ((x + charWidth) >= wide || ch == '\r' || ch == '\n')
		{
			// add another line
			AddAnotherLine(x,y);
			
			justStartedNewLine = true;
			hasWord = false;
			
			if (ch == '\r' || ch == '\n')
			{
				// set the break at the current character
				m_LineBreaks.AddToTail(i);
			}
			else if (wordStartedOnNewLine)
			{
				// word is longer than a line, so set the break at the current cursor
				m_LineBreaks.AddToTail(i);
			}
			else
			{
				// set it at the last word Start
				m_LineBreaks.AddToTail(wordStartIndex);
				
				// just back to reparse the next line of text
				i = wordStartIndex;
			}
			
			// reset word length
			wordLength = 0;
		}
		
		// add to the size
		x += charWidth;
		wordLength += charWidth;
	}
	
	_charCount = i-1;
	
	// end the list
	m_LineBreaks.AddToTail(BUFFER_SIZE);
	
	// set up the scrollbar
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate where the vertical scroll bar slider should be 
//			based on the current cursor line we are on.
//-----------------------------------------------------------------------------
void TextEntry::LayoutVerticalScrollBarSlider()
{
	// set up the scrollbar
	if (_vertScrollBar)
	{
		int wide, tall;
		GetSize (wide, tall);
		
		// make sure we factor in insets
		int ileft, iright, itop, ibottom;
		GetInset(ileft, iright, itop, ibottom);
		
		// with a scroll bar we take off the inset
		wide -= iright;
		
		_vertScrollBar->SetPos(wide - _scrollBarSize, 0);
		// scrollbar is inside the borders.
		_vertScrollBar->SetSize(_scrollBarSize, tall - ibottom - itop);
		
		// calculate how many lines we can fully display
		int displayLines = tall / (surface()->GetFontTall(_font) + DRAW_OFFSET_Y);
		int numLines = m_LineBreaks.Count();
		
		if (numLines <= displayLines)
		{
			// disable the scrollbar
			_vertScrollBar->SetEnabled(false);
			_vertScrollBar->SetRange(0, numLines);
			_vertScrollBar->SetRangeWindow(numLines);
			_vertScrollBar->SetValue(0);
		}
		else
		{
			// set the scrollbars range
			_vertScrollBar->SetRange(0, numLines);
			_vertScrollBar->SetRangeWindow(displayLines);
			
			_vertScrollBar->SetEnabled(true);
			
			// this should make it scroll one line at a time
			_vertScrollBar->SetButtonPressedScrollValue(1);
			
			// set the value to view the last entries
			int val = _vertScrollBar->GetValue();
			int maxval = _vertScrollBar->GetValue() + displayLines;
			if (GetCursorLine() < val )
			{
				while (GetCursorLine() < val)
				{
					val--;
				}
			}
			else if (GetCursorLine() >= maxval)
			{
				while (GetCursorLine() >= maxval)
				{
					maxval++;
				}
				maxval -= displayLines;	
				val = maxval;
			}
			else 
			{
				//val = GetCursorLine();
			}
			
			_vertScrollBar->SetValue(val);
			_vertScrollBar->InvalidateLayout();
			_vertScrollBar->Repaint();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Set boolean value of baseclass variables.
//-----------------------------------------------------------------------------
void TextEntry::SetEnabled(bool state)
{
	BaseClass::SetEnabled(state);
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether text wraps around multiple lines or not
// Input  : state - true or false
//-----------------------------------------------------------------------------
void TextEntry::SetMultiline(bool state)
{
	_multiline = state;
}

//-----------------------------------------------------------------------------
// Purpose: sets whether or not the edit catches and stores ENTER key presses
//-----------------------------------------------------------------------------
void TextEntry::SetCatchEnterKey(bool state)
{
	_catchEnterKey = state;
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether a vertical scrollbar is visible
// Input  : state - true or false
//-----------------------------------------------------------------------------
void TextEntry::SetVerticalScrollbar(bool state)
{
	_verticalScrollbar = state;
	
	if (_verticalScrollbar)
	{
		if (!_vertScrollBar)
		{
			_vertScrollBar = new ScrollBar(this, "ScrollBar", true);
			_vertScrollBar->AddActionSignalTarget(this);
		}
		
		_vertScrollBar->SetVisible(true);
	}
	else if (_vertScrollBar)
	{
		_vertScrollBar->SetVisible(false);
	}
	
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: sets _editable flag
// Input  : state - true or false
//-----------------------------------------------------------------------------
void TextEntry::SetEditable(bool state)
{
	_editable = state;
}

//-----------------------------------------------------------------------------
// Purpose: Create cut/copy/paste dropdown menu
//-----------------------------------------------------------------------------
void TextEntry::CreateEditMenu()
{	
	// create a drop down cut/copy/paste menu appropriate for this object's states
	if (m_pEditMenu)
		delete m_pEditMenu;
	m_pEditMenu = new Menu(this, "EditMenu");
	
	
	// add cut/copy/paste drop down options if its editable, just copy if it is not
	if (_editable)
	{	
		m_pEditMenu->AddMenuItem("#TextEntry_Cut", new KeyValues("DoCutSelected"), this);
	}
	
	m_pEditMenu->AddMenuItem("#TextEntry_Copy", new KeyValues("DoCopySelected"), this);
	
	if (_editable)
	{
		m_pEditMenu->AddMenuItem("#TextEntry_Paste", new KeyValues("DoPaste"), this);
	}
	
	m_pEditMenu->SetVisible(false);
	m_pEditMenu->SetParent(this);
	m_pEditMenu->AddActionSignalTarget(this);
}

//-----------------------------------------------------------------------------
// Purpsoe: Returns state of _editable flag
//-----------------------------------------------------------------------------
bool TextEntry::IsEditable()
{
	return _editable;
}

//-----------------------------------------------------------------------------
// Purpose: We want single line windows to scroll horizontally and select text
//          in response to clicking and holding outside window
//-----------------------------------------------------------------------------
void TextEntry::OnMouseFocusTicked()
{
	// if a button is down move the scrollbar slider the appropriate direction
	if (_mouseDragSelection) // text is being selected via mouse clicking and dragging
	{
		OnCursorMoved(0,0);	// we want the text to scroll as if we were dragging
	}	
}

//-----------------------------------------------------------------------------
// Purpose: If a cursor enters the window, we are not elegible for 
//          MouseFocusTicked events
//-----------------------------------------------------------------------------
void TextEntry::OnCursorEntered()
{
	_mouseDragSelection = false; // outside of window dont recieve drag scrolling ticks
}

//-----------------------------------------------------------------------------
// Purpose: When the cursor is outside the window, if we are holding the mouse
//			button down, then we want the window to scroll the text one char at a time
//			using Ticks
//-----------------------------------------------------------------------------
void TextEntry::OnCursorExited() // outside of window recieve drag scrolling ticks
{
	if (_mouseSelection)
		_mouseDragSelection = true;
}

//-----------------------------------------------------------------------------
// Purpose: Handle selection of text by mouse
//-----------------------------------------------------------------------------
void TextEntry::OnCursorMoved(int x, int y)
{
	if (_mouseSelection)
	{
		// update the cursor position
		int x, y;
		input()->GetCursorPos(x, y);
		ScreenToLocal(x, y);
		_cursorPos = PixelToCursorSpace(x, y);
		
		// if we are at Start of buffer don't put cursor at end, this will keep
		// window from scrolling up to a blank line
		if (_cursorPos == 0)
			_putCursorAtEnd = false;
		
		// scroll if we went off left side
		if (_cursorPos == _currentStartIndex)
		{
			if (_cursorPos > 0)
				_cursorPos--;

			ScrollLeft();
			_cursorPos = _currentStartIndex;
		}
		if ( _cursorPos != _select[1])
		{
			_select[1] = _cursorPos;
			Repaint();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle Mouse button down events.
//-----------------------------------------------------------------------------
void TextEntry::OnMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		SelectCheck();
		
		// move the cursor to where the mouse was pressed
		int x, y;
		input()->GetCursorPos(x, y);
		ScreenToLocal(x, y);
		
		_cursorIsAtEnd = _putCursorAtEnd; // save this off before calling PixelToCursorSpace()
		_cursorPos = PixelToCursorSpace(x, y);
		// if we are at Start of buffer don't put cursor at end, this will keep
		// window from scrolling up to a blank line
		if (_cursorPos == 0)
			_putCursorAtEnd = false;
		
		// enter selection mode
		input()->SetMouseCapture(GetVPanel());
		_mouseSelection = true;
		
		if (_select[0] < 0)
		{
			// if no initial selection position, Start selection position at cursor
			_select[0] = _cursorPos;
		}
		_select[1] = _cursorPos;
		
		ResetCursorBlink();
		RequestFocus();
		Repaint();
	}
	else if (code == MOUSE_RIGHT) // check for context menu open
	{	
		CreateEditMenu();
		Assert(m_pEditMenu);
		
		OpenEditMenu();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse button up events
//-----------------------------------------------------------------------------
void TextEntry::OnMouseReleased(MouseCode code)
{
	_mouseSelection = false;
	
	input()->SetMouseCapture(NULL);
	
	// make sure something has been selected
	int cx0, cx1;
	if (GetSelectedRange(cx0, cx1))
	{
		if (cx1 - cx0 == 0)
		{
			// nullify selection
			_select[0] = -1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse double clicks
//-----------------------------------------------------------------------------
void TextEntry::OnMouseDoublePressed(MouseCode code)
{
	// left double clicking on a word selects the word
	if (code == MOUSE_LEFT)
	{
		// move the cursor just as if you single clicked.
		OnMousePressed(code);
		// then find the start and end of the word we are in to highlight it.
		int selectSpot[2];
		GotoWordLeft();
		selectSpot[0] = _cursorPos;
		GotoWordRight();
		selectSpot[1] = _cursorPos;

		if (_cursorPos > 0)
		{
			if (iswspace(m_TextStream[_cursorPos - 1]))
			{
				selectSpot[1]--;
				_cursorPos--;
			}
			
			_select[0] = selectSpot[0];
			_select[1] = selectSpot[1];
			_mouseSelection = true;
		}
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: Turn off text selection code when mouse button is not down
//-----------------------------------------------------------------------------
void TextEntry::OnMouseCaptureLost()
{
	_mouseSelection = false;
}

//-----------------------------------------------------------------------------
// Purpose: Masks which keys get chained up
//			Maps keyboard input to text window functions.
//-----------------------------------------------------------------------------
void TextEntry::OnKeyCodeTyped(KeyCode code)
{
	_cursorIsAtEnd = _putCursorAtEnd;
	_putCursorAtEnd = false;
	
	bool shift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
	bool ctrl = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	bool alt = (input()->IsKeyDown(KEY_LALT) || input()->IsKeyDown(KEY_RALT));
	bool fallThrough = false;
	
	if (ctrl)
	{
		switch(code)
		{
		case KEY_INSERT:
		case KEY_C:
			{
				CopySelected();
				break;
			}
		case KEY_V:
			{
				DeleteSelected();
				Paste();
				break;
			}
		case KEY_X:
			{
				CopySelected();
				DeleteSelected();
				break;
			}
		case KEY_Z:
			{
				Undo();
				break;
			}
		case KEY_RIGHT:
			{
				GotoWordRight();
				break;
			}
		case KEY_LEFT:
			{
				GotoWordLeft();
				break;
			}
		case KEY_ENTER:
			{
				// insert a newline
				if (_multiline)
				{
					DeleteSelected();
					SaveUndoState();
					InsertChar('\n');
				}
				// fire newlines back to the main target if asked to
				if(_sendNewLines) 
				{
					PostActionSignal(new KeyValues("TextNewLine"));
				}
				break;
			}
		case KEY_HOME:
			{
				GotoTextStart();
				break;
			}
		case KEY_END:
			{
				GotoTextEnd();
				break;
			}
		default:
			{
				fallThrough = true;
				break;
			}
		}
	}
	else if (alt)
	{
		// do nothing with ALT-x keys
		fallThrough = true;
	}
	else
	{
		switch(code)
		{
		case KEY_TAB:
		case KEY_LSHIFT:
		case KEY_RSHIFT:
		case KEY_ESCAPE:
			{
				fallThrough = true;
				break;
			}
		case KEY_INSERT:
			{
				if (shift)
				{
					DeleteSelected();
					Paste();
				}
				else
				{
					fallThrough = true;
				}
				
				break;
			}
		case KEY_DELETE:
			{
				if (shift)
				{
					// shift-delete is cut
					CopySelected();
					DeleteSelected();
				}
				else
				{
					Delete();
				}
				break;
			}
		case KEY_LEFT:
			{
				GotoLeft();
				break;
			}
		case KEY_RIGHT:
			{
				GotoRight();
				break;
			}
		case KEY_UP:
			{
				if (_multiline)
				{
					GotoUp();
				}
				else
				{
					fallThrough = true;
				}
				break;
			}
		case KEY_DOWN:
			{
				if (_multiline)
				{
					GotoDown();
				}
				else
				{
					fallThrough = true;
				}
				break;
			}
		case KEY_HOME:
			{
				if (_multiline)
				{
					GotoFirstOfLine();
				}
				else
				{
					GotoTextStart();
				}
				break;
			}
		case KEY_END:
			{
				GotoEndOfLine();
				break;
			}
		case KEY_BACKSPACE:
			{
				int x0, x1;
				if (GetSelectedRange(x0, x1))
				{
					// act just like delete if there is a selection
					DeleteSelected();
				}
				else
				{
					Backspace();
				}
				break;
			}
		case KEY_ENTER:
			{
				// insert a newline
				if (_multiline && _catchEnterKey)
				{
					DeleteSelected();
					SaveUndoState();
					InsertChar('\n');
				}
				else
				{
					fallThrough = true;
				}
				// fire newlines back to the main target if asked to
				if(_sendNewLines) 
				{
					PostActionSignal(new KeyValues("TextNewLine"));
				}
				break;
			}
		case KEY_PAGEUP:
			{
				int val = 0;
				if (_vertScrollBar)
				{
					val = _vertScrollBar->GetValue();
				}
				
				// if there is a scroll bar scroll down one rangewindow
				if (_multiline)
				{
					int displayLines = GetTall() / (surface()->GetFontTall(_font) + DRAW_OFFSET_Y);
					// move the cursor down
					for (int i=0; i < displayLines; i++)
					{
						GotoUp();
					}
				}
				
				// if there is a scroll bar scroll down one rangewindow
				if (_vertScrollBar)
				{
					int window = _vertScrollBar->GetRangeWindow();
					int newval = _vertScrollBar->GetValue();
					int linesToMove = window - (val - newval);
					_vertScrollBar->SetValue(val - linesToMove - 1);
				}
				break;
				
			}
		case KEY_PAGEDOWN:
			{
				int val = 0;
				if (_vertScrollBar)
				{
					val = _vertScrollBar->GetValue();
				}
				
				if (_multiline)
				{
					int displayLines = GetTall() / (surface()->GetFontTall(_font) + DRAW_OFFSET_Y);
					// move the cursor down
					for (int i=0; i < displayLines; i++)
					{
						GotoDown();
					}
				}
				
				// if there is a scroll bar scroll down one rangewindow
				if (_vertScrollBar)
				{
					int window = _vertScrollBar->GetRangeWindow();
					int newval = _vertScrollBar->GetValue();
					int linesToMove = window - (newval - val);
					_vertScrollBar->SetValue(val + linesToMove + 1);
				}
				break;
			}

		case KEY_F1:
		case KEY_F2:
		case KEY_F3:
		case KEY_F4:
		case KEY_F5:
		case KEY_F6:
		case KEY_F7:
		case KEY_F8:
		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
			{
				fallThrough = true;
				break;
			}

		default:
			{
				// return if any other char is pressed.
				// as it will be a unicode char.
				// and we don't want select[1] changed unless a char was pressed that this fxn handles
				return;
			}
		}
	}
	
	// select[1] is the location in the line where the blinking cursor started
	_select[1] = _cursorPos;
	
	if (_dataChanged)
	{
		FireActionSignal();
	}
	
	// chain back on some keys
	if (fallThrough)
	{
		_putCursorAtEnd=_cursorIsAtEnd;	// keep state of cursor on fallthroughs
		BaseClass::OnKeyCodeTyped(code);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Masks which keys get chained up
//			Maps keyboard input to text window functions.
//-----------------------------------------------------------------------------
void TextEntry::OnKeyTyped(wchar_t unichar)
{
	_cursorIsAtEnd = _putCursorAtEnd;
	_putCursorAtEnd=false;
	
	bool fallThrough = false;
	
	// KeyCodes handle all non printable chars
	if (!iswprint(unichar))
		return;
	
	// do readonly keys
	if (!IsEditable())
	{
		BaseClass::OnKeyTyped(unichar);
		return;
	}
	
	if (unichar != 0)
	{
		DeleteSelected();
		SaveUndoState();
		InsertChar(unichar);
	}
	
	// select[1] is the location in the line where the blinking cursor started
	_select[1] = _cursorPos;
	
	if (_dataChanged)
	{
		FireActionSignal();
	}
	
	// chain back on some keys
	if (fallThrough)
	{
		_putCursorAtEnd=_cursorIsAtEnd;	// keep state of cursor on fallthroughs
		BaseClass::OnKeyTyped(unichar);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Scrolls the list according to the mouse wheel movement
//-----------------------------------------------------------------------------
void TextEntry::OnMouseWheeled(int delta)
{
	if (_vertScrollBar)
	{
		MoveScrollBar(delta);
	}
	else
	{
		// if we don't use the input, chain back
		BaseClass::OnMouseWheeled(delta);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scrolls the list 
// Input  : delta - amount to move scrollbar up
//-----------------------------------------------------------------------------
void TextEntry::MoveScrollBar(int delta)
{
	if (_vertScrollBar)
	{
		int val = _vertScrollBar->GetValue();
		val -= (delta * 3);
		_vertScrollBar->SetValue(val);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame the entry has keyboard focus; 
//          blinks the text cursor
//-----------------------------------------------------------------------------
void TextEntry::OnKeyFocusTicked()
{
	int time=system()->GetTimeMillis();
	if(time>_cursorNextBlinkTime)
	{
		_cursorBlink=!_cursorBlink;
		_cursorNextBlinkTime=time+_cursorBlinkRate;
		Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check if we are selecting text (so we can highlight it)
//-----------------------------------------------------------------------------
void TextEntry::SelectCheck()
{
	if (!HasFocus() || !(input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT)))
	{
		_select[0] = -1;
	}
	else if (_select[0] == -1)
	{
		_select[0] = _cursorPos;
	}
}

//-----------------------------------------------------------------------------
// Purpose: set the maximum number of chars in the text buffer
//-----------------------------------------------------------------------------
void TextEntry::SetMaximumCharCount(int maxChars)
{
	_maxCharCount = maxChars;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int TextEntry::GetMaximumCharCount()
{
	return _maxCharCount;
}

//-----------------------------------------------------------------------------
// Purpose: set whether to wrap the text buffer
//-----------------------------------------------------------------------------
void TextEntry::SetWrap(bool wrap)
{
	_wrap = wrap;
}

//-----------------------------------------------------------------------------
// Purpose: set whether to pass newline msgs to parent
//-----------------------------------------------------------------------------
void TextEntry::SendNewLine(bool send)
{
	_sendNewLines = send;
}

//-----------------------------------------------------------------------------
// Purpose: Tell if an index is a linebreakindex
//-----------------------------------------------------------------------------
bool TextEntry::IsLineBreak(int index)
{
	for (int i=0; i<m_LineBreaks.Count(); ++i)
	{
		if (index ==  m_LineBreaks[i])
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor one character to the left, scroll the text
//  horizontally if needed
//-----------------------------------------------------------------------------
void TextEntry::GotoLeft()
{
	SelectCheck();
	
	// if we are on a line break just move the cursor to the prev line
	if (IsLineBreak(_cursorPos))
	{
		// if we're already on the prev line at the end dont put it on the end
		if (!_cursorIsAtEnd)
			_putCursorAtEnd = true;
	}
	// if we are not at Start decrement cursor 
	if (!_putCursorAtEnd && _cursorPos > 0)
	{
		_cursorPos--;	
	}
	
    ScrollLeft();
	
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor one character to the right, scroll the text
//			horizontally if needed
//-----------------------------------------------------------------------------
void TextEntry::GotoRight()
{
	SelectCheck();
	
	// if we are on a line break just move the cursor to the next line
	if (IsLineBreak(_cursorPos))
	{
		if (_cursorIsAtEnd)
		{
			_putCursorAtEnd = false;
		}
		else
		{
			// if we are not at end increment cursor 
			if (_cursorPos < m_TextStream.Count())
			{
				_cursorPos++;
			}	
		}
	}
	else
	{
		// if we are not at end increment cursor 
		if (_cursorPos < m_TextStream.Count())
		{
			_cursorPos++;
		}
		
		// if we are on a line break move the cursor to end of line 
		if (IsLineBreak(_cursorPos))
		{
			if (!_cursorIsAtEnd)
				_putCursorAtEnd = true;
		}
	}
	// scroll right if we need to
	ScrollRight();
	
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Find out what line the cursor is on
//-----------------------------------------------------------------------------
int TextEntry::GetCursorLine()
{
	// find which line the cursor is on
	for (int cursorLine = 0; cursorLine < m_LineBreaks.Count(); cursorLine++)
	{
		if (_cursorPos < m_LineBreaks[cursorLine])
			break;
	}
	
	if (_putCursorAtEnd)  // correct for when cursor is at end of line rather than Start of next
	{
		// we are not at end of buffer, in which case there is no next line to be at the Start of
		if (_cursorPos != m_TextStream.Count() ) 
			cursorLine--;
	}
	
	return cursorLine;
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor one line up 
//-----------------------------------------------------------------------------
void TextEntry::GotoUp()
{
	SelectCheck();
	
	if (_cursorIsAtEnd)
	{
		if ( (GetCursorLine() - 1 ) == 0) // we are on first line
		{
			// stay at end of line
			_putCursorAtEnd = true;
			return;	 // dont move the cursor
		}
		else
			_cursorPos--;  
	}
	
	int cx, cy;
	CursorToPixelSpace(_cursorPos, cx, cy);
	
	// move the cursor to the previous line
	MoveCursor(GetCursorLine() - 1, cx);
}


//-----------------------------------------------------------------------------
// Purpose: Move the cursor one line down
//-----------------------------------------------------------------------------
void TextEntry::GotoDown()
{
	SelectCheck();
	
	if (_cursorIsAtEnd)
	{
		_cursorPos--;
		if (_cursorPos < 0)
			_cursorPos = 0;
	}
	
	int cx, cy;
	CursorToPixelSpace(_cursorPos, cx, cy);
	
	// move the cursor to the next line
	MoveCursor(GetCursorLine() + 1, cx);
	if (!_putCursorAtEnd && _cursorIsAtEnd )
	{
		_cursorPos++;
		if (_cursorPos > m_TextStream.Count())
		{
			_cursorPos = m_TextStream.Count();
		}
	}
	LayoutVerticalScrollBarSlider();	
}

//-----------------------------------------------------------------------------
// Purpose: Set the starting ypixel positon for a walk through the window
//-----------------------------------------------------------------------------
int TextEntry::GetYStart()
{
	if (_multiline)
	{
		// just Start from the top
		return DRAW_OFFSET_Y;
	}

	int fontTall = surface()->GetFontTall(_font);
	return (GetTall() / 2) - (fontTall / 2);
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor to a line, need to know how many pixels are in a line
//-----------------------------------------------------------------------------
void TextEntry::MoveCursor(int line, int pixelsAcross)
{
	// clamp to a valid line
	if (line < 0)
		line = 0;
	if (line >= m_LineBreaks.Count())
		line = m_LineBreaks.Count() -1;
	
	// walk the whole text set looking for our place
	// work out where to Start checking
	
	int yStart = GetYStart();
	
	int x = DRAW_OFFSET_X, y = yStart;
	int lineBreakIndexIndex = 0;
	_pixelsIndent = 0;
	for (int i = 0; i < m_TextStream.Count(); i++)
	{
		wchar_t ch = m_TextStream[i];
		
		if (_hideText)
		{
			ch = '*';
		}
		
		// if we've passed a line break go to that
		if (m_LineBreaks[lineBreakIndexIndex] == i)
		{
			if (lineBreakIndexIndex == line)
			{
				_putCursorAtEnd = true;
				_cursorPos = i;
				break;
			}
			
			// add another line
			AddAnotherLine(x,y);
			lineBreakIndexIndex++;
			
		}
		
		// add to the current position
		int charWidth = getCharWidth(_font, ch);		
		
		if (line == lineBreakIndexIndex)
		{
			// check to see if we're in range
			if ((x + (charWidth / 2)) > pixelsAcross)
			{
				// found position
				_cursorPos = i;
				break;
			}
		}
		
		x += charWidth;
	}
	
	// if we never find the cursor it must be past the end
	// of the text buffer, to let's just slap it on the end of the text buffer then.
	if (i ==  m_TextStream.Count())
	{
		GotoTextEnd();
	}
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Turn horizontal scrolling on or off.
//			Horizontal scrolling is disabled in multline windows. 
//		    Toggling this will disable it in single line windows as well.
//-----------------------------------------------------------------------------
void TextEntry::SetHorizontalScrolling(bool status) 
{
	_horizScrollingAllowed = status;
}

//-----------------------------------------------------------------------------
// Purpose: Horizontal scrolling function, not used in multiline windows
//			Function will scroll the buffer to the left if the cursor is not in the window
//			scroll left if we need to 
//-----------------------------------------------------------------------------
void TextEntry::ScrollLeft()
{
	if (_multiline)	  // early out
	{
		return;
	}
	
	if (!_horizScrollingAllowed)  //early out
	{
		return;
	}
	
	if(_cursorPos < _currentStartIndex)	 // scroll left if we need to
	{
		if (_cursorPos < 0)// dont scroll past the Start of buffer
		{
			_cursorPos=0;			
		}
		_currentStartIndex = _cursorPos;
	}
	
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::ScrollLeftForResize()
{
	if (_multiline)	  // early out
	{
		return;
	}
	
	if (!_horizScrollingAllowed)  //early out
	{
		return;
	}

    while (_currentStartIndex > 0)     // go until we hit leftmost
    {
        _currentStartIndex--;
		int nVal = _currentStartIndex;

        // check if the cursor is now off the screen
        if (IsCursorOffRightSideOfWindow(_cursorPos))
        {
            _currentStartIndex++;   // we've gone too far, return it
            break;
        }

        // IsCursorOffRightSideOfWindow actually fixes the _currentStartIndex, 
        // so if our value changed that menas we really are off the screen
		if (nVal != _currentStartIndex)
			break;
    }
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: Horizontal scrolling function, not used in multiline windows
//			Scroll one char right until the cursor is visible in the window.
//			We do this one char at a time because char width isn't a constant.
//-----------------------------------------------------------------------------
void TextEntry::ScrollRight()
{
	if (!_horizScrollingAllowed)
		return;

	if (_multiline)	  
	{
	}
	// check if cursor is off the right side of window
	else if (IsCursorOffRightSideOfWindow(_cursorPos))
	{
		_currentStartIndex++; //scroll over
		ScrollRight(); // scroll again, check if cursor is in window yet 
	}
	
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: Check and see if cursor position is off the right side of the window
//			just compare cursor's pixel coords with the window size coords.
// Input:	an integer cursor Position, if you pass _cursorPos fxn will tell you
//			if current cursor is outside window.
// Output:	true: cursor is outside right edge or window 
//			false: cursor is inside right edge
//-----------------------------------------------------------------------------
bool TextEntry::IsCursorOffRightSideOfWindow(int cursorPos)
{
	int cx, cy;
	CursorToPixelSpace(cursorPos, cx, cy);
	int wx=GetWide()-1;	//width of inside of window is GetWide()-1
	if ( wx < 0 )
		return false;
	return (cx >= wx);
}

//-----------------------------------------------------------------------------
// Purpose: Check and see if cursor position is off the left side of the window
//			just compare cursor's pixel coords with the window size coords.
// Input:	an integer cursor Position, if you pass _cursorPos fxn will tell you
//			if current cursor is outside window.
// Output:	true - cursor is outside left edge or window 
//			false - cursor is inside left edge
//-----------------------------------------------------------------------------
bool TextEntry::IsCursorOffLeftSideOfWindow(int cursorPos)
{
	int cx, cy;
	CursorToPixelSpace(cursorPos, cx, cy);
	return (cx <= 0);
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor over to the Start of the next word to the right
//-----------------------------------------------------------------------------
void TextEntry::GotoWordRight()
{
	SelectCheck();
	
	// search right until we hit a whitespace character or a newline
	while (++_cursorPos < m_TextStream.Count())
	{
		if (iswspace(m_TextStream[_cursorPos]))
			break;
	}
	
	// search right until we hit an nonspace character
	while (++_cursorPos < m_TextStream.Count())
	{
		if (!iswspace(m_TextStream[_cursorPos]))
			break;
	}
	
	if (_cursorPos > m_TextStream.Count())
		_cursorPos = m_TextStream.Count();
	
	// now we are at the start of the next word
		
	// scroll right if we need to
	ScrollRight();
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor over to the Start of the next word to the left
//-----------------------------------------------------------------------------
void TextEntry::GotoWordLeft()
{
	SelectCheck();
	
	if (_cursorPos < 1)
		return;
	
	// search left until we hit an nonspace character
	while (--_cursorPos >= 0)
	{
		if (!iswspace(m_TextStream[_cursorPos]))
			break;
	}
	
	// search left until we hit a whitespace character
	while (--_cursorPos >= 0)
	{
		if (iswspace(m_TextStream[_cursorPos]))
		{
			break;
		}
	}
	
	// we end one character off
	_cursorPos++;
	// now we are at the Start of the previous word
	
	
	// scroll left if we need to
	ScrollLeft();
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move cursor to the Start of the text buffer
//-----------------------------------------------------------------------------
void TextEntry::GotoTextStart()
{
	SelectCheck();
	_cursorPos = 0;		   // set cursor to Start
	_putCursorAtEnd = false;
	_currentStartIndex=0;  // scroll over to Start
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move cursor to the end of the text buffer
//-----------------------------------------------------------------------------
void TextEntry::GotoTextEnd()
{
	SelectCheck();
	_cursorPos=m_TextStream.Count();	// set cursor to end of buffer
	_putCursorAtEnd = true; // move cursor Start of next line
	ScrollRight();				// scroll over until cursor is on screen
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move cursor to the Start of the current line
//-----------------------------------------------------------------------------
void TextEntry::GotoFirstOfLine()
{
	SelectCheck();
	// to get to the Start of the line you have to take into account line wrap
	// we have to figure out at which point the line wraps
	// given the current cursor position, select[1], find the index that is the
	// line Start to the left of the cursor
	//_cursorPos = 0; //TODO: this is wrong, should go to first non-whitespace first, then to zero
	_cursorPos = GetCurrentLineStart();
	_putCursorAtEnd = false;
	
	_currentStartIndex=_cursorPos;
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Get the index of the first char on the current line
//-----------------------------------------------------------------------------
int TextEntry::GetCurrentLineStart()
{
	if (!_multiline)			// quick out for non multline buffers
		return _currentStartIndex;
	
	if (IsLineBreak(_cursorPos))
	{
		for (int i = 0; i < m_LineBreaks.Count(); ++i )
		{
			if (_cursorPos == m_LineBreaks[i])
				break;
		}
		if (_cursorIsAtEnd)
		{
			return m_LineBreaks[i-1];
		}
		else
			return _cursorPos; // we are already at Start
	}
	
	for (int i = 0; i < m_LineBreaks.Count(); ++i )
	{
		if (_cursorPos < m_LineBreaks[i])
		{
			if (i == 0)
				return 0;
			else
				return m_LineBreaks[i-1];
		}
	}
	// if there were no line breaks, the first char in the line is the Start of the buffer
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Move cursor to the end of the current line
//-----------------------------------------------------------------------------
void TextEntry::GotoEndOfLine()
{
	SelectCheck();
	// to get to the end of the line you have to take into account line wrap in the buffer
	// we have to figure out at which point the line wraps
	// given the current cursor position, select[1], find the index that is the
	// line end to the right of the cursor
	//_cursorPos=m_TextStream.Count(); //TODO: this is wrong, should go to last non-whitespace, then to true EOL
    _cursorPos = GetCurrentLineEnd();
	_putCursorAtEnd = true;
	
	ScrollRight();
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Get the index of the last char on the current line
//-----------------------------------------------------------------------------
int TextEntry::GetCurrentLineEnd()
{
	if (IsLineBreak(_cursorPos)	)
	{
		for (int i = 0; i < m_LineBreaks.Count()-1; ++i )
		{
			if (_cursorPos == m_LineBreaks[i])
				break;
		}
		if (!_cursorIsAtEnd)
		{
			if (i == m_LineBreaks.Count()-2 )
				m_TextStream.Count();		
			else
				return m_LineBreaks[i+1];
		}
		else
			return _cursorPos; // we are already at end
	}
	
	for (int i = 0; i < m_LineBreaks.Count()-1; i++)
	{
		if ( _cursorPos < m_LineBreaks[i])
		{
			return m_LineBreaks[i];
		}
	}
	return m_TextStream.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Insert a character into the text buffer
//-----------------------------------------------------------------------------
void TextEntry::InsertChar(wchar_t ch)
{
	// throw away redundant linefeed characters
	if (ch == '\r')
		return;
	
	// no newline characters in single-line dialogs
	if (!_multiline && ch == '\n')
		return;

	if (m_bAllowNumericInputOnly)
	{
		if (!iswdigit(ch) && ((char)ch != '.'))
		{
			surface()->PlaySound("Resource\\warning.wav");
			return;
		}
	}
	
	// don't add characters if the max char count has been reached
	// ding at the user
	if (_maxCharCount > -1 && m_TextStream.Count() >= _maxCharCount)
	{
		if (_maxCharCount>0 && _multiline && _wrap)
		{
			// if we wrap lines rather than stopping
			while (m_TextStream.Count() > _maxCharCount)
			{
				if (_recalculateBreaksIndex==0) 
				{
					// we can get called before this has been run for the first time :)
					RecalculateLineBreaks();
				}
				if (m_LineBreaks[0]> m_TextStream.Count())
				{
					// if the line break is the past the end of the buffer recalc
					_recalculateBreaksIndex=-1;
					RecalculateLineBreaks();
				}
				
				if (m_LineBreaks[0]+1 < m_TextStream.Count())
				{
					// delete the line
					m_TextStream.RemoveMultiple(0, m_LineBreaks[0]);
					// old Dar version, replaced by above
					// m_TextStream.RemoveElementsBefore(m_LineBreaks[0] + 1);
					
					// in case we just deleted text from where the cursor is
					if( _cursorPos> m_TextStream.Count())
					{
						_cursorPos=m_TextStream.Count(); 
					}
					else
					{ // shift the cursor up. don't let it wander past zero
						_cursorPos-=m_LineBreaks[0]+1;
						if(_cursorPos<0)
						{
							_cursorPos=0;
						}
					}
					
					// move any selection area up
					if(_select[0]>-1)
					{
						_select[0] -=m_LineBreaks[0]+1;
						
						if(_select[0] <=0)
						{
							_select[0] =-1;
						}
						
						_select[1] -=m_LineBreaks[0]+1;
						if(_select[1] <=0)
						{
							_select[1] =-1;
						}
						
					}
					
					// now redraw the buffer
					for (int i = m_TextStream.Count() - 1; i >= 0; i--)
					{
						SetCharAt(m_TextStream[i], i+1);
					}
					
					// redo all the line breaks
					_recalculateBreaksIndex=-1;
					RecalculateLineBreaks();
					
				}
			}
			
		} 
		else
		{
			// make a sound
			surface()->PlaySound("Resource\\warning.wav");
			return;
		}
	}
	
	
	if (_wrap) 
	{
		// when wrapping you always insert the new char at the end of the buffer
		SetCharAt(ch, m_TextStream.Count());
		_cursorPos=m_TextStream.Count(); 
	}
	else 
	{
		// move chars right 1 starting from cursor, then replace cursorPos with char and increment cursor
		for (int i =  m_TextStream.Count()- 1; i >= _cursorPos; i--)
		{
			SetCharAt(m_TextStream[i], i+1);
		}
		
		SetCharAt(ch, _cursorPos);
		_cursorPos++;
	}
	
	// if its a newline char we can't do the slider until we recalc the line breaks
	if (ch == '\n')
	{
		RecalculateLineBreaks();
	}
	
	// scroll right if this pushed the cursor off screen
	ScrollRight();
	
	_dataChanged = true;
	
	CalcBreakIndex();
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Get the lineBreakIndex index of the line before the cursor
//			note _recalculateBreaksIndex < 0 flags RecalculateLineBreaks 
//			to figure it all out from scratch
//-----------------------------------------------------------------------------
void TextEntry::CalcBreakIndex()
{
	// an optimization to handle when the cursor is at the end of the buffer.
	// pays off if the buffer is large, and the search loop would be long.
	if (_cursorPos == m_TextStream.Count())
	{
		// we know m_LineBreaks array always has at least one element in it (99999 sentinel)
		// when there is just one line this will make recalc = -1 which is ok.
		_recalculateBreaksIndex = m_LineBreaks.Count()-2;
		return;
	}
	
	_recalculateBreaksIndex=0;
	// find the line break just before the cursor position
	while (_cursorPos > m_LineBreaks[_recalculateBreaksIndex])
		++_recalculateBreaksIndex;
	
	// -1  is ok.
	--_recalculateBreaksIndex;	
}

//-----------------------------------------------------------------------------
// Purpose: Insert a string into the text buffer, this is just a series
//			of char inserts because we have to check each char is ok to insert
//-----------------------------------------------------------------------------
void TextEntry::InsertString(wchar_t *wszText)
{
	SaveUndoState();

	for (const wchar_t *ch = wszText; *ch != 0; ++ch)
	{
		InsertChar(*ch);
	}
	
	if (_dataChanged)
	{
		FireActionSignal();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Converts an ansi string to unicode and inserts it into the text stream
//-----------------------------------------------------------------------------
void TextEntry::InsertString(const char *text)
{
	// check for to see if the string is in the localization tables
	if (text[0] == '#')
	{
		wchar_t *wsz = localize()->Find(text);
		if (wsz)
		{
			InsertString(wsz);
			return;
		}
	}

	// straight convert the ansi to unicode and insert
	wchar_t unicode[1024];
	localize()->ConvertANSIToUnicode(text, unicode, sizeof(unicode));
	InsertString(unicode);
}

//-----------------------------------------------------------------------------
// Purpose: Handle the effect of user hitting backspace key
//			we delete the char before the cursor and reformat the text so it
//			behaves like in windows.
//-----------------------------------------------------------------------------
void TextEntry::Backspace()
{
	if (!IsEditable())
		return;

	//if you are at the first position don't do anything
	if(_cursorPos==0)
	{
		return;
	}
	
	//if the line is empty, don't do anything
	if(m_TextStream.Count()==0)
	{
		return;
	}
	
	SaveUndoState();
	
	//shift chars left one, starting at the cursor position, then make the line one smaller
	for(int i=_cursorPos;i<m_TextStream.Count(); ++i)
	{
		SetCharAt(m_TextStream[i],i-1);
	}
	m_TextStream.Remove(m_TextStream.Count() - 1);
	
	// As we hit the Start of the window, expose more chars so we can see what we are deleting
	if (_cursorPos==_currentStartIndex)
	{
		// windows tabs over 6 chars
		if (_currentStartIndex-6 >= 0) // dont scroll if there are not enough chars to scroll
		{
			_currentStartIndex-=6; 
		}
		else
			_currentStartIndex=0;
	}
	
	//move the cursor left one
	_cursorPos--;
	
	_dataChanged = true;
	
	CalcBreakIndex();
	
	LayoutVerticalScrollBarSlider();
	ResetCursorBlink();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Deletes the current selection, if any, moving the cursor to the Start
//			of the selection
//-----------------------------------------------------------------------------
void TextEntry::DeleteSelected()
{
	if (!IsEditable())
		return;

	// if the line is empty, don't do anything
	if (m_TextStream.Count() == 0)
		return;
	
	// get the range to delete
	int x0, x1;
	if (!GetSelectedRange(x0, x1))
	{
		// no selection, don't touch anything
		return;
	}
	
	SaveUndoState();
	
	// shift chars left one starting after cursor position, then make the line one smaller
	int dif = x1 - x0;
	for (int i = 0; i < dif; ++i)
	{
		m_TextStream.Remove(x0);
	}
	
	// clear any selection
	SelectNone();
	ResetCursorBlink();
	
	// move the cursor to just after the deleted section
	_cursorPos = x0;
	
	_dataChanged = true;
	
	_recalculateBreaksIndex = 0;
	m_LineBreaks.RemoveAll();
	m_LineBreaks.AddToTail(BUFFER_SIZE);

	CalcBreakIndex();
	
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: Handle the effect of the user hitting the delete key
//			removes the char in front of the cursor
//-----------------------------------------------------------------------------
void TextEntry::Delete()
{
	if (!IsEditable())
		return;

	// if the line is empty, don't do anything
	if (m_TextStream.Count() == 0)
		return;
	
	// get the range to delete
	int x0, x1;
	if (!GetSelectedRange(x0, x1))
	{
		// no selection, so just delete the one character
		x0 = _cursorPos;
		x1 = x0 + 1;
		
		// if we're at the end of the line don't do anything
		if (_cursorPos >= m_TextStream.Count())
			return;
	}
	
	SaveUndoState();
	
	// shift chars left one starting after cursor position, then make the line one smaller
	int dif = x1 - x0;
	for (int i = 0; i < dif; i++)
	{
		m_TextStream.Remove((int)x0);
	}
	
	ResetCursorBlink();
	
	// clear any selection
	SelectNone();
	
	// move the cursor to just after the deleted section
	_cursorPos = x0;
	
	_dataChanged = true;
	
	_recalculateBreaksIndex = 0;
	m_LineBreaks.RemoveAll();
	m_LineBreaks.AddToTail(BUFFER_SIZE);

	CalcBreakIndex();
	
	LayoutVerticalScrollBarSlider();
}

//-----------------------------------------------------------------------------
// Purpose: Declare a selection empty
//-----------------------------------------------------------------------------
void TextEntry::SelectNone()
{
	// tag the selection as empty
	_select[0] = -1;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Load in the selection range so cx0 is the Start and cx1 is the end
//			from smallest to highest (right to left)
//-----------------------------------------------------------------------------
bool TextEntry::GetSelectedRange(int& cx0,int& cx1)
{
	// if there is nothing selected return false
	if (_select[0] == -1)
	{
		return false;
	}
	
	// sort the two position so cx0 is the smallest
	cx0=_select[0];
	cx1=_select[1];
	int temp;
	if(cx1<cx0){temp=cx0;cx0=cx1;cx1=temp;}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the cut/copy/paste dropdown menu
//-----------------------------------------------------------------------------
void TextEntry::OpenEditMenu()
{
	// get cursor position, this is local to this text edit window
	// so we need to adjust it relative to the parent
	int cursorX, cursorY;
	input()->GetCursorPos(cursorX, cursorY);
	
	// find the frame that has no parent (the one on the desktop)
	Panel *panel = this;
	while ( panel->GetParent() != NULL)
	{
		panel = panel->GetParent();
	}
	panel->ScreenToLocal(cursorX, cursorY);
	int x, y;
	// get base panel's postition
	panel->GetPos(x, y);	  
	
	// adjust our cursor position accordingly
	cursorX += x;
	cursorY += y;
	
	int x0, x1;
	if (GetSelectedRange(x0, x1)) // there is something selected
	{
		m_pEditMenu->SetItemEnabled("&Cut", true);
		m_pEditMenu->SetItemEnabled("C&opy", true);
	}
	else	// there is nothing selected, disable cut/copy options
	{
		m_pEditMenu->SetItemEnabled("&Cut", false);
		m_pEditMenu->SetItemEnabled("C&opy", false);
	}
	m_pEditMenu->SetVisible(true);
	m_pEditMenu->RequestFocus();
	
	// relayout the menu immediately so that we know it's size
	m_pEditMenu->InvalidateLayout(true);
	int menuWide, menuTall;
	m_pEditMenu->GetSize(menuWide, menuTall);
	
	// work out where the cursor is and therefore the best place to put the menu
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	
	if (wide - menuWide > cursorX)
	{
		// menu hanging right
		if (tall - menuTall > cursorY)
		{
			// menu hanging down
			m_pEditMenu->SetPos(cursorX, cursorY);
		}
		else
		{
			// menu hanging up
			m_pEditMenu->SetPos(cursorX, cursorY - menuTall);
		}
	}
	else
	{
		// menu hanging left
		if (tall - menuTall > cursorY)
		{
			// menu hanging down
			m_pEditMenu->SetPos(cursorX - menuWide, cursorY);
		}
		else
		{
			// menu hanging up
			m_pEditMenu->SetPos(cursorX - menuWide, cursorY - menuTall);
		}
	}
	
	m_pEditMenu->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Cuts the selected chars from the buffer and 
//          copies them into the clipboard
//-----------------------------------------------------------------------------
void TextEntry::CutSelected()
{
	CopySelected();
	DeleteSelected();
	// have to request focus if we used the menu
	RequestFocus();	
}

//-----------------------------------------------------------------------------
// Purpose: Copies the selected chars into the clipboard
//-----------------------------------------------------------------------------
void TextEntry::CopySelected()
{
	if (_hideText)
		return;
	
	int x0, x1;
	if (GetSelectedRange(x0, x1))
	{
		CUtlVector<wchar_t> buf;
		for (int i = x0; i < x1; i++)
		{
			buf.AddToTail(m_TextStream[i]);
		}
		buf.AddToTail('\0');
		system()->SetClipboardText(buf.Base(), x1 - x0);
	}
	
	// have to request focus if we used the menu
	RequestFocus();	
}

//-----------------------------------------------------------------------------
// Purpose: Pastes the selected chars from the clipboard into the text buffer
//			truncates if text is longer than our _maxCharCount
//-----------------------------------------------------------------------------
void TextEntry::Paste()
{
	if (_hideText)
		return;
	
	if (!IsEditable())
		return;

	CUtlVector<wchar_t> buf;
	int bufferSize = _maxCharCount > 0 ? _maxCharCount + 1 : system()->GetClipboardTextCount();    // +1 for terminator
	buf.AddMultipleToTail(bufferSize);
	int len = system()->GetClipboardText(0, buf.Base(), bufferSize * sizeof(wchar_t));
	if (len < 1)
		return;
	
	SaveUndoState();
	
	for (int i = 0; i < (len - 1) && buf[i] != 0; i++)
	{
		InsertChar(buf[i]);
	}
	
	_dataChanged = true;
	// have to request focus if we used the menu
	RequestFocus();	
}

//-----------------------------------------------------------------------------
// Purpose: Reverts back to last saved changes
//-----------------------------------------------------------------------------
void TextEntry::Undo()
{
	_cursorPos = _undoCursorPos;
	m_TextStream.CopyArray(m_UndoTextStream.Base(), m_UndoTextStream.Count());
	
	InvalidateLayout();
	Repaint();
	SelectNone();
}

//-----------------------------------------------------------------------------
// Purpose: Saves the current state to the undo stack
//-----------------------------------------------------------------------------
void TextEntry::SaveUndoState()
{
	_undoCursorPos = _cursorPos;
	m_UndoTextStream.CopyArray(m_TextStream.Base(), m_TextStream.Count());
}

//-----------------------------------------------------------------------------
// Purpose: Returns the index in the text buffer of the
//          character the drawing should Start at
//-----------------------------------------------------------------------------
int TextEntry::GetStartDrawIndex(int &lineBreakIndexIndex)
{
	int startIndex = 0;
	
	int numLines = m_LineBreaks.Count();
	int startLine = 0;
	
	// determine the Start point from the scroll bar
	// do this only if we are not selecting text in the window with the mouse
	if (_vertScrollBar && !_mouseDragSelection)
	{	
		// skip to line indicated by scrollbar
		startLine = _vertScrollBar->GetValue();
	}
	else
	{
		// check to see if the cursor is off the screen-multiline case
		HFont font = _font;
		int displayLines = GetTall() / (surface()->GetFontTall(font) + DRAW_OFFSET_Y);
		if (numLines > displayLines)
		{
			int cursorLine = GetCursorLine();
			
			startLine = _currentStartLine;
			
			// see if that is visible
			if (cursorLine < _currentStartLine)
			{
				// cursor is above visible area; scroll back
				startLine = cursorLine;
				if (_vertScrollBar)
				{
					MoveScrollBar( 1 ); // should be calibrated for speed 
					// adjust startline incase we hit a limit
					startLine = _vertScrollBar->GetValue(); 
				}
			}
			else if (cursorLine > (_currentStartLine + displayLines - 1))
			{
				// cursor is down below visible area; scroll forward
				startLine = cursorLine - displayLines + 1;
				if (_vertScrollBar)
				{
					MoveScrollBar( -1 );
					startLine = _vertScrollBar->GetValue();
				}
			}
		}
		else if (!_multiline)
		{
			// check to see if cursor is off the right side of screen-single line case
			// get cursor's x coordinate in pixel space
			int x = DRAW_OFFSET_X;
			for (int i = _currentStartIndex; i < m_TextStream.Count(); i++)
			{
				wchar_t ch = m_TextStream[i];			
				if (_hideText)
				{
					ch = '*';
				}
				
				// if we've found the position, break
				if (_cursorPos == i)
				{
					break;
				}
				
				// add to the current position		
				x += getCharWidth(font, ch);				
			}
			
			if ( x >=  GetWide() )
			{
				_currentStartIndex++;
			}
			else if ( x <= 0 )
			{
				// dont go past the Start of buffer
				if (_currentStartIndex > 0)
					_currentStartIndex--;
			}
		}
	}
	
	if (startLine > 0)
	{
		lineBreakIndexIndex = startLine;
		if (startLine && startLine < m_LineBreaks.Count())
		{
			startIndex = m_LineBreaks[startLine - 1];
		}
	}
	
	if (!_horizScrollingAllowed)
		return 0;

	_currentStartLine = startLine;
	if (_multiline)
		return startIndex;
	else 
		return _currentStartIndex;

	
}

//-----------------------------------------------------------------------------
// Purpose: Get a string from text buffer
// Input:	offset - index to Start reading from 
//			bufLen - length of string
//-----------------------------------------------------------------------------
void TextEntry::GetText(char *buf, int bufLen)
{
	if (m_TextStream.Count())
	{
		// temporarily null terminate the text stream so we can use the conversion function
		int nullTerminatorIndex = m_TextStream.AddToTail((wchar_t)0);
		localize()->ConvertUnicodeToANSI(m_TextStream.Base(), buf, bufLen);
		m_TextStream.FastRemove(nullTerminatorIndex);
	}
	else
	{
		// no characters in the stream
		buf[0] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get a string from text buffer
// Input:	offset - index to Start reading from 
//			bufLen - length of string
//-----------------------------------------------------------------------------
void TextEntry::GetText(wchar_t *wbuf, int bufLen)
{
	int len = m_TextStream.Count();
	if (m_TextStream.Count())
	{
		wcsncpy(wbuf, m_TextStream.Base(), bufLen);
		int terminator = __min(len, bufLen);
		wbuf[terminator] = 0;
	}
	else
	{
		wbuf[0] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message that the text has changed
//-----------------------------------------------------------------------------
void TextEntry::FireActionSignal()
{
	PostActionSignal(new KeyValues("TextChanged"));
	_dataChanged = false;	// reset the data changed flag
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Set the font of the buffer text 
// Input:	font to change to
//-----------------------------------------------------------------------------
void TextEntry::SetFont(HFont font)
{
	_font = font;
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the scrollbar slider is moved
//-----------------------------------------------------------------------------
void TextEntry::OnSliderMoved()
{
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool TextEntry::RequestInfo(KeyValues *outputData)
{
	if (!stricmp(outputData->GetName(), "GetText"))
	{
		wchar_t wbuf[256];
		GetText(wbuf, 255);
		outputData->SetWString("text", wbuf);
		return true;
	}
	else if (!stricmp(outputData->GetName(), "GetState"))
	{
		wchar_t wbuf[64];
		GetText(wbuf, sizeof(wbuf));
		outputData->SetInt("state", _wtoi(wbuf));
		return true;
	}
	return BaseClass::RequestInfo(outputData);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::OnSetText(const wchar_t *text)
{
	SetText(text);
}

//-----------------------------------------------------------------------------
// Purpose: as above, but sets an integer
//-----------------------------------------------------------------------------
void TextEntry::OnSetState(int state)
{
	char buf[64];
	_snprintf(buf, sizeof(buf), "%d", state);
	SetText(buf);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
//	_font = scheme()->GetFont(GetScheme(), "Default", IsProportional() );
//	SetFont( _font );

	SetTextHidden((bool)inResourceData->GetInt("textHidden", 0));
	SetEditable((bool)inResourceData->GetInt("editable", 1));
	SetMaximumCharCount(inResourceData->GetInt("maxchars", -1));
	SetAllowNumericInputOnly(inResourceData->GetInt("NumericInputOnly", 0));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::GetSettings( KeyValues *outResourceData )
{
	BaseClass::GetSettings( outResourceData );
	outResourceData->SetInt("textHidden", _hideText);
	outResourceData->SetInt("editable", IsEditable());
	outResourceData->SetInt("maxchars", _maxCharCount);
	outResourceData->SetInt("NumericInputOnly", m_bAllowNumericInputOnly);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *TextEntry::GetDescription()
{
	static char buf[1024];
	_snprintf(buf, sizeof(buf), "%s, bool textHidden, bool editable, bool NumericInputOnly", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of lines in the window
//-----------------------------------------------------------------------------
int TextEntry::GetNumLines()
{
	return m_LineBreaks.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the height of the text entry window so all text will fit inside
//-----------------------------------------------------------------------------
void TextEntry::SetToFullHeight()
{
	PerformLayout();
	int wide, tall;
	GetSize(wide, tall);
	
	tall = GetNumLines() * (surface()->GetFontTall(_font) + DRAW_OFFSET_Y) + DRAW_OFFSET_Y + 2;
	SetSize (wide, tall);
	PerformLayout();
	
}

//-----------------------------------------------------------------------------
// Purpose: Select all the text.
//-----------------------------------------------------------------------------
void TextEntry::SelectAllText(bool bResetCursorPos)
{
	if ( bResetCursorPos )
	{
		_cursorPos = 0;
	}

	// if there's no text at all, select none
	if (m_TextStream.Count() == 0)
	{
		_select[0] = -1;
	}
	else
	{
		_select[0] = 0;
	}
	_select[1] = m_TextStream.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Select no text.
//-----------------------------------------------------------------------------
void TextEntry::SelectNoText()
{
	_select[0] = -1;
	_select[1] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the width of the text entry window so all text will fit inside
//-----------------------------------------------------------------------------
void TextEntry::SetToFullWidth()
{
	// probably be problems if you try using this on multi line buffers
	// or buffers with clickable text in them.
	if (_multiline)
		return;
	
	PerformLayout();
	int wide = 2*DRAW_OFFSET_X; // buffer on left and right end of text.
	
	// loop through all the characters and sum their widths	
	for (int i = 0; i < m_TextStream.Count(); ++i)
	{
		wide += getCharWidth(_font, m_TextStream[i]);	
	}
	
	// height of one line of text
	int tall = (surface()->GetFontTall(_font) + DRAW_OFFSET_Y) + DRAW_OFFSET_Y + 2;
	
	SetSize (wide, tall);
	PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TextEntry::SelectAllOnFirstFocus( bool status )
{
	_selectAllOnFirstFocus = status;
}

//-----------------------------------------------------------------------------
// Purpose: called when the text entry receives focus
//-----------------------------------------------------------------------------
void TextEntry::OnSetFocus()
{ 
	// see if we should highlight all on selection
    if (_selectAllOnFirstFocus)
	{
		_select[0] = 0;
		_select[1] = m_TextStream.Count();
		_cursorPos = _select[1]; // cursor at end of line
		_selectAllOnFirstFocus = false;
    }
	else if (!input()->IsMouseDown(MOUSE_LEFT) && !input()->IsMouseDown(MOUSE_RIGHT))
	{
		// if we've tabbed to this field then move to the end of the text
		GotoTextEnd();
		// clear any selection
		SelectNone();
	}
	
	BaseClass::OnSetFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Set the width we have to draw text in.
//			Do not use in multiline windows.
//-----------------------------------------------------------------------------
void TextEntry::SetDrawWidth(int width)
{
	_drawWidth = width;
}

//-----------------------------------------------------------------------------
// Purpose: Get the width we have to draw text in.
//-----------------------------------------------------------------------------
int TextEntry::GetDrawWidth()
{
	return _drawWidth;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void TextEntry::SetAllowNumericInputOnly(bool state)
{
	m_bAllowNumericInputOnly = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
MessageMapItem_t TextEntry::m_MessageMap[] =
{
	MAP_MESSAGE( TextEntry, "ScrollBarSliderMoved", OnSliderMoved ),
	MAP_MESSAGE( TextEntry, "DoCutSelected", CutSelected ),
	MAP_MESSAGE( TextEntry, "DoCopySelected", CopySelected ),
	MAP_MESSAGE( TextEntry, "DoPaste", Paste ),
	MAP_MESSAGE_INT( TextEntry, "SetState", OnSetState, "state" ),			// custom message
	MAP_MESSAGE_CONSTWCHARPTR( TextEntry, "SetText", OnSetText, "text" ),	// custom message
};

IMPLEMENT_PANELMAP(TextEntry, Panel);