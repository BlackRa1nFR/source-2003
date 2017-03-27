//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Creates a Message box with a question in it and yes/no buttons
//
// $NoKeywords: $
//=============================================================================

#ifndef TOOLTIP_H
#define TOOLTIP_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <UtlVector.h>

//-----------------------------------------------------------------------------
// Purpose: Creates A Message box with a question in it and yes/no buttons
//-----------------------------------------------------------------------------

namespace vgui
{

class TextEntry;
class Panel;

class Tooltip 
{
public:

	Tooltip(Panel *parent, const char *text = NULL);
	~Tooltip();

	void SetText(const char *text);
	const char *GetText();
	void ShowTooltip(Panel *currentPanel);
	void HideTooltip();
	void SizeTextWindow();
	void ResetDelay();
	void PerformLayout();

	void SetDisplayMode(bool displayOnOneLine);
	void SetTooltipDelay( int tooltipDelay );
	int GetTooltipDelay();

private:
	virtual void ApplySchemeSettings(IScheme *pScheme);

	CUtlVector<char> m_Text;
	static int _delay; // delay that counts down
	static int _tooltipDelay; // delay before tooltip comes up.
	bool _makeVisible;
	bool _displayOnOneLine;
};

};

#endif // TOOLTIP_H
