//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DISP_VRAD_H
#define DISP_VRAD_H
#ifdef _WIN32
#pragma once
#endif


#include "../../common/builddisp.h"


// Blend the normals of neighboring displacement surfaces so they match at edges and corners.
void SmoothNeighboringDispSurfNormals( CCoreDispInfo *pListBase, int listSize );


#endif // DISP_VRAD_H
