//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef CBASE_H
#define CBASE_H
#ifdef _WIN32
#pragma once
#endif

// cbase.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

// TODO: reference additional headers your program requires here
#include "mx/mx.h"
#include "mx/mxwindow.h"
#include "tier0/dbg.h"
#include "UtlVector.h"
#include "vstdlib/random.h"
#include "sharedInterface.h"
#include "scenemanager_tools.h"

extern class CSoundEmitterSystemBase *soundemitter;

#endif // CBASE_H
