//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SMDFILEIO_H
#define SMDFILEIO_H
#ifdef _WIN32
#pragma once
#endif

extern bool SMDReadTriangles (FILE* fp, int* numTris, NmRawTriangle** tris);

#endif // SMDFILEIO_H
