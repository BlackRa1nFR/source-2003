//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
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
#include "glquake.h"
#include "draw.h"
#include "server.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "utlbuffer.h"
#include "utlvector.h"

CUtlVector< Vector >	g_Points;
//-----------------------------------------------------------------------------
// Purpose: Draw the currently loaded line file
// Input  : g_Points - list of points
//-----------------------------------------------------------------------------
void Linefile_Draw( void )
{
	Vector *points = g_Points.Base();
	int pointCount = g_Points.Size();

	for ( int i = 0; i < pointCount-1; i++ )
	{
		Draw_Line( points[i], points[i+1], 255, 255, 0, FALSE );
	}
}


//-----------------------------------------------------------------------------
// Purpose: parse the map.lin file from disk
//			this file contains a list of line segments illustrating a leak in
//			the map
//-----------------------------------------------------------------------------
void Linefile_Read_f( void )
{
	FileHandle_t f;
	Vector	org;
	int		r;
	int		c;
	char	name[MAX_OSPATH];

	g_Points.Purge();

	Q_snprintf (name, sizeof( name ), "maps/%s.lin", sv.name);

	COM_OpenFile (name, &f);
	if (!f)
	{
		Con_Printf ("couldn't open %s\n", name);
		return;
	}
	
	Con_Printf ("Reading %s...\n", name);
	c = 0;

	int size = g_pFileSystem->Size(f);
	CUtlBuffer buf( 0, size, true );
	g_pFileSystem->Read( buf.Base(), size, f );
	g_pFileSystem->Close(f);

	for ( ;; )
	{
		r = buf.Scanf ("%f %f %f\n", &org[0], &org[1], &org[2]);
		if (r != 3)
			break;
		c++;
		
		g_Points.AddToTail( org );
	}

	Con_Printf ("%i lines read\n", c);
}
