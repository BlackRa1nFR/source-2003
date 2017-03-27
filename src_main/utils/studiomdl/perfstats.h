//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef PERFSTATS_H
#define PERFSTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "studio.h"
#include "optimize.h"

void SpewPerfStats( studiohdr_t *pStudioHdr, OptimizedModel::FileHeader_t *vtxFile );

#endif // PERFSTATS_H
