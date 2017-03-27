//
// modelgen.h: header file for model generation program
//

// *********************************************************
// * This file must be identical in the modelgen directory *
// * and in the Quake directory, because it's used to      *
// * pass data from one to the other via model files.      *
// *********************************************************

#ifndef MODELGEN_H
#define MODELGEN_H
#pragma once

#ifdef INCLUDELIBS

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib.h"
#include "trilib.h"
#include "lbmlib.h"

#endif

#define ALIAS_ONSEAM				0x0020

// must match definition in spritegn.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
typedef enum {ST_SYNC=0, ST_RAND } synctype_t;
#endif


#define IDPOLYHEADER	(('O'<<24)+('P'<<16)+('D'<<8)+'I')
														// little-endian "IDPO"

#define IDSTUDIOHEADER	(('T'<<24)+('S'<<16)+('D'<<8)+'I')
														// little-endian "IDST"

#define IDSTUDIOSEQHEADER	(('Q'<<24)+('S'<<16)+('D'<<8)+'I')

#endif // !MODELGEN_H
