//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef SCENEENTITY_H
#define SCENEENTITY_H
#ifdef _WIN32
#pragma once
#endif

float InstancedScriptedScene( CBaseFlex *pActor, const char *pszScene, EHANDLE *phSceneEnt = NULL );
void StopInstancedScriptedScene( CBaseFlex *pActor, EHANDLE hSceneEnt );


#endif // SCENEENTITY_H