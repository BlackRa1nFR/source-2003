//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef DISP_IVP_H
#define DISP_IVP_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

class CPhysCollisionEntry;
struct dmodel_t;

extern void Disp_AddCollisionModels( CUtlVector<CPhysCollisionEntry *> &collisionList, dmodel_t *pModel, int contentsMask );

#endif // DISP_IVP_H
