//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( MODES_H )
#define MODES_H
#ifdef _WIN32
#pragma once
#endif

#define MAX_MODE_DESCRIPTION_LEN 32

typedef struct vmode_s
{
	int			width;
	int			height;
	int			bpp;
	char		modedesc[ MAX_MODE_DESCRIPTION_LEN ];
} vmode_t;

#endif // MODES_H
