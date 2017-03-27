//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ICLOSECAPTIONMANAGER_H
#define ICLOSECAPTIONMANAGER_H
#ifdef _WIN32
#pragma once
#endif

typedef struct tagRECT RECT;
class CSentence;
class StudioModel;
class CChoreoWidgetDrawHelper;

class ICloseCaptionManager
{
public:
	virtual void Reset( void ) = 0;

	virtual void PreProcess( int framecount ) = 0;
	virtual void Process( int framecount, StudioModel *model, float curtime, CSentence* sentence ) = 0;
	virtual void PostProcess( int framecount, float frametime ) = 0;
	virtual void Draw( CChoreoWidgetDrawHelper &helper, RECT &rcOutput ) = 0;
};

extern ICloseCaptionManager *closecaptionmanager;

#endif // ICLOSECAPTIONMANAGER_H
