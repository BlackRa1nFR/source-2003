//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#if !defined( MODEL_TYPES_H )
#define MODEL_TYPES_H
#ifdef _WIN32
#pragma once
#endif

#define STUDIO_NONE						0x00000000
#define STUDIO_RENDER					0x00000001
#define STUDIO_VIEWXFORMATTACHMENTS		0x00000002
#define STUDIO_DRAWTRANSLUCENTSUBMODELS 0x00000004
#define STUDIO_FRUSTUMCULL				0x00000008
#define STUDIO_TWOPASS					0x00000010
#define STUDIO_STATIC_LIGHTING			0x00000020
#define STUDIO_OCCLUSIONCULL			0x00000040

// Not a studio flag, but used to flag model as a non-sorting brush model
#define STUDIO_TRANSPARENCY				0x80000000


enum modtype_t
{
	mod_bad = 0, 
	mod_brush, 
	mod_sprite, 
	mod_studio
};

#endif // MODEL_TYPES_H