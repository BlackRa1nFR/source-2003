//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DISPBUILD_H
#define DISPBUILD_H
#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Purpose: test to see if the surface contains displacement information
//   Input: pSurf - the surface to test
//  Output: true is the surface has displacement information, false otherwise
//-----------------------------------------------------------------------------

// Add all of the world's displacements to the leaves.
void AddDispSurfsToLeafs( model_t *pWorld );

// Remove the displacement from all leaves it's in and re-add it.
void ReAddDispSurfToLeafs( model_t *pWorld, IDispInfo *pDispInfo );


#endif // DISPBUILD_H
