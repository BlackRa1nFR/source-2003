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

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Status bar that visually displays discrete progress in the form
//			of a segmented strip
//-----------------------------------------------------------------------------
class ProgressBar : public Panel
{
public:
	ProgressBar(Panel *parent, const char *panelName);

	// 'progress' is in the range [0.0f, 1.0f]
	virtual void SetProgress(float progress);
	virtual void SetSegmentInfo( int gap, int width );

	// utility function for calculating a time remaining string
	static bool ConstructTimeRemainingString(wchar_t *output, int outputBufferSizeInBytes, float startTime, float currentTime, float currentProgress, float lastProgressUpdateTime, bool addRemainingSuffix);

protected:
	virtual void PaintBackground();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	DECLARE_PANELMAP();
	/* CUSTOM MESSAGE HANDLING
		"SetProgress"
			input:	"progress"	- float value of the progress to set
	*/

private:
	int   _segmentCount;
	float _progress;
	int _segmentGap;
	int _segmentWide;
};

} // namespace vgui

#endif // PROGRESSBAR_H
