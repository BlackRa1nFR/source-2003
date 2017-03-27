//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef WRITEBSP_H
#define WRITEBSP_H
#ifdef _WIN32
#pragma once
#endif


#include "bspfile.h"


typedef struct node_s node_t;

//-----------------------------------------------------------------------------
// Emits occluder faces
//-----------------------------------------------------------------------------
void EmitOccluderFaces (node_t *node);


//-----------------------------------------------------------------------------
// Purpose: Free the list of faces stored at the leaves
//-----------------------------------------------------------------------------
void FreeLeafFaces( face_t *pLeafFaceList );

#endif // WRITEBSP_H
