//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Revision: $
// $NoKeywords: $
//
// header for videomodes
//=============================================================================

#ifndef VMODES_H
#define VMODES_H
#pragma once


typedef enum 
{
	VT_None = 0, 
	VT_OpenGL, 
	VT_DX8 
} VidTypes;

extern VidTypes gVidType;

typedef struct viddef_s
{
	unsigned int	width;		
	unsigned int	height;
	int				recalc_refdef;	// if TRUE, recalc vid-based stuff
	int				bits;
	VidTypes		vidtype;
} viddef_t;


#endif //VMODES_H
