//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( MXBITMAPTOOLS_H )
#define MXBITMAPTOOLS_H
#ifdef _WIN32
#pragma once
#endif

typedef struct mxbitmapdata_s
{
	bool		valid;
	void		*image;
	int			width;
	int			height;
} mxbitmapdata_t;

class mxWindow;

bool LoadBitmapFromFile( const char *filename, mxbitmapdata_t& bitmap );
void DrawBitmapToWindow( mxWindow *wnd, int x, int y, int w, int h, mxbitmapdata_t& bitmap );
void DrawBitmapToDC( void *hdc, int x, int y, int w, int h, mxbitmapdata_t& bitmap );

#endif // MXBITMAPTOOLS_H