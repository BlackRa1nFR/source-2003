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
#include <math.h>
#include <stdio.h>

#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Controls.h>

#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ProgressBar::ProgressBar(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
	_progress = 0.0f;
	SetSegmentInfo( 4, 8 );
}

void ProgressBar::SetSegmentInfo( int gap, int width )
{
	_segmentGap = gap;
	_segmentWide = width;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ProgressBar::PaintBackground()
{
	int wide, tall;
	GetSize(wide, tall);

	surface()->DrawSetColor(GetBgColor());
	surface()->DrawFilledRect(0, 0, wide, tall);


	// gaps
	int segmentTotal = wide / (_segmentGap + _segmentWide);
	int segmentsDrawn = (int)(segmentTotal * _progress);

	surface()->DrawSetColor(GetFgColor());
	int x = 0, y = 4;
	for (int i = 0; i < segmentsDrawn; i++)
	{
		x += _segmentGap;
		surface()->DrawFilledRect(x, y, x + _segmentWide, y + tall - (y * 2));
		x += _segmentWide;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ProgressBar::SetProgress(float progress)
{
	if (progress != _progress)
	{
		// clamp the progress value within the range
		if (progress < 0.0f)
		{
			progress = 0.0f;
		}
		else if (progress > 1.0f)
		{
			progress = 1.0f;
		}

		_progress = progress;
		Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ProgressBar::ApplySchemeSettings(IScheme *pScheme)
{
	Panel::ApplySchemeSettings(pScheme);

	SetFgColor(GetSchemeColor("BrightControlText", pScheme));
	SetBgColor(GetSchemeColor("WindowBgColor", pScheme));
	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
}

//-----------------------------------------------------------------------------
// Purpose: utility function for calculating a time remaining string
//-----------------------------------------------------------------------------
bool ProgressBar::ConstructTimeRemainingString(wchar_t *output, int outputBufferSizeInBytes, float startTime, float currentTime, float currentProgress, float lastProgressUpdateTime, bool addRemainingSuffix)
{
	assert(lastProgressUpdateTime <= currentTime);

	// calculate pre-extrapolation values
	float timeElapsed = lastProgressUpdateTime - startTime;
	float totalTime = timeElapsed / currentProgress;

	// calculate seconds
	int secondsRemaining = (int)(totalTime - timeElapsed);
	if (lastProgressUpdateTime < currentTime)
	{
		// old update, extrapolate
		float progressRate = currentProgress / timeElapsed;
		float extrapolatedProgress = progressRate * (currentTime - startTime);
		float extrapolatedTotalTime = (currentTime - startTime) / extrapolatedProgress;
		secondsRemaining = (int)(extrapolatedTotalTime - timeElapsed);
	}
	// if there's some time, make sure it's at least one second left
	if ( secondsRemaining == 0 && ( ( totalTime - timeElapsed ) > 0 ) )
	{
		secondsRemaining = 1;
	}

	// calculate minutes
	int minutesRemaining = 0;
	while (secondsRemaining >= 60)
	{
		minutesRemaining++;
		secondsRemaining -= 60;
	}

    char minutesBuf[16];
    sprintf(minutesBuf, "%d", minutesRemaining);
    char secondsBuf[16];
    sprintf(secondsBuf, "%d", secondsRemaining);

	if (minutesRemaining > 0)
	{
		wchar_t unicodeMinutes[16];
		localize()->ConvertANSIToUnicode(minutesBuf, unicodeMinutes, sizeof( unicodeMinutes ));
		wchar_t unicodeSeconds[16];
		localize()->ConvertANSIToUnicode(secondsBuf, unicodeSeconds, sizeof( unicodeSeconds ));

		const char *unlocalizedString = "#vgui_TimeLeftMinutesSeconds";
		if (minutesRemaining == 1 && secondsRemaining == 1)
		{
			unlocalizedString = "#vgui_TimeLeftMinuteSecond";
		}
		else if (minutesRemaining == 1)
		{
			unlocalizedString = "#vgui_TimeLeftMinuteSeconds";
		}
		else if (secondsRemaining == 1)
		{
			unlocalizedString = "#vgui_TimeLeftMinutesSecond";
		}

		char unlocString[64];
		strcpy(unlocString, unlocalizedString);
		if (addRemainingSuffix)
		{
			strcat(unlocString, "Remaining");
		}
		localize()->ConstructString(output, outputBufferSizeInBytes, localize()->Find(unlocString), 2, unicodeMinutes, unicodeSeconds);

	}
	else if (secondsRemaining > 0)
	{
		wchar_t unicodeSeconds[16];
		localize()->ConvertANSIToUnicode(secondsBuf, unicodeSeconds, sizeof( unicodeSeconds ));

		const char *unlocalizedString = "#vgui_TimeLeftSeconds";
		if (secondsRemaining == 1)
		{
			unlocalizedString = "#vgui_TimeLeftSecond";
		}
		char unlocString[64];
		strcpy(unlocString, unlocalizedString);
		if (addRemainingSuffix)
		{
			strcat(unlocString, "Remaining");
		}
		localize()->ConstructString(output, outputBufferSizeInBytes, localize()->Find(unlocString), 1, unicodeSeconds);
	}
	else
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: empty message map
//-----------------------------------------------------------------------------
MessageMapItem_t ProgressBar::m_MessageMap[] =
{
	MAP_MESSAGE_FLOAT( ProgressBar, "SetProgress", SetProgress, "progress" ),	// custom message
};
IMPLEMENT_PANELMAP(ProgressBar, Panel);
