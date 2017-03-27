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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <gl/gl.h>
#include <gl/glut.h>

static void *font = GLUT_BITMAP_9_BY_15;

int PrintfXY( float x, float y, const char *fmt, ... )
{	
	int retVal, len, i;
	static char errorString[1024];
	
	va_list argptr;
	
	va_start( argptr, fmt );
	retVal = vsprintf( errorString, fmt, argptr );
	va_end( argptr );
	
	glRasterPos2f(x, y);
	len = (int) strlen( errorString );
	for (i = 0; i < len; i++) 
	{
		glutBitmapCharacter( font, errorString[i] );
	}
	
	return retVal;
}

