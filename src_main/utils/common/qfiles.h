//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef QFILES_H
#define QFILES_H
#pragma once


//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

#include "basetypes.h"
#include "commonmacros.h"
#include "worldsize.h"
#include "bspfile.h"

#define MAX_OSPATH	260
#define MAX_QPATH	64

/*
========================================================================

The .pak files are just a linear collapse of a directory tree

========================================================================
*/

#define IDPAKHEADER		(('K'<<24)+('C'<<16)+('A'<<8)+'P')

#endif // QFILES_H
