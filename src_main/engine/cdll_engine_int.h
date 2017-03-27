//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CDLL_ENGINE_INT_H
#define CDLL_ENGINE_INT_H
#ifdef _WIN32
#pragma once
#endif


#include "cdll_int.h"

class IVModelRender;
class IClientLeafSystemEngine;

void ClientDLL_Init( void );
void ClientDLL_Shutdown( void );
void ClientDLL_HudVidInit( void );
void ClientDLL_ProcessInput( void );
void ClientDLL_Update( void );
void ClientDLL_VoiceStatus( int entindex, bool bTalking );
void ClientDLL_FrameStageNotify( ClientFrameStage_t frameStage );

//-----------------------------------------------------------------------------
// slow routine to draw a physics model
//-----------------------------------------------------------------------------
void DebugDrawPhysCollide( const CPhysCollide *pCollide, IMaterial *pMaterial, matrix3x4_t& transform, const color32 &color );


extern IBaseClientDLL *g_ClientDLL;
extern IVModelRender* modelrender;
extern IClientLeafSystemEngine* clientleafsystem;



#endif // CDLL_ENGINE_INT_H
