//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Model loading / unloading interface
//
// $NoKeywords: $
//=============================================================================

#ifndef OVERLAY_H
#define OVERLAY_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Overlay fragments
//-----------------------------------------------------------------------------
typedef unsigned short OverlayFragmentHandle_t;

enum
{
	OVERLAY_FRAGMENT_INVALID = (OverlayFragmentHandle_t)~0
};

//=============================================================================
//
// Overlay Manager Interface
//
class IOverlayMgr
{
public:
	// Memory allocation/de-allocation.
	virtual bool	LoadOverlays( ) = 0;
	virtual void	UnloadOverlays( ) = 0;

	virtual void	CreateFragments( void ) = 0;

	// Drawing
	virtual void	AddFragmentListToRenderList( OverlayFragmentHandle_t iFragment ) = 0;
	virtual void	RenderOverlays( ) = 0;
};


//-----------------------------------------------------------------------------
// Overlay manager singleton
//-----------------------------------------------------------------------------
IOverlayMgr *OverlayMgr();

#endif // OVERLAY_H
