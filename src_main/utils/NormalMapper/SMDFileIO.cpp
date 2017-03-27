//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <string.h>
#include <assert.h>
#include "NmFileIO.h"

static int CountTris( FILE *fp )
{
	char buf[4096];
	int numTriangles = 0;
	rewind( fp );
	while( fgets( buf, 4095, fp ) )
	{
		if( _strnicmp( "triangles", buf, strlen( "triangles" ) ) == 0 )
		{
			while( 1 )
			{
				if( !fgets( buf, 4095, fp ) )
				{
					assert( 0 );
					return numTriangles;
				}
				if( _strnicmp( "end", buf, strlen( "end" ) ) == 0 )
				{
					return numTriangles;
				}
				fgets( buf, 4095, fp ); // vert 1
				fgets( buf, 4095, fp ); // vert 2
				fgets( buf, 4095, fp ); // vert 3
				numTriangles++;
			}
		}
	}
	return numTriangles;
}

static void ReadTris( FILE *fp, int numTris, NmRawTriangle *tris )
{
	rewind( fp );
	char buf[4096];
	int count = 0;
	rewind( fp );
	while( fgets( buf, 4095, fp ) )
	{
		if( _strnicmp( "triangles", buf, strlen( "triangles" ) ) == 0 )
		{
			while( 1 )
			{
				if( !fgets( buf, 4095, fp ) )
				{
					assert( 0 );
					return;
				}
				if( _strnicmp( "end", buf, strlen( "end" ) ) == 0 )
				{
					assert( count == numTris );
					return;
				}
				int i;
				for( i = 0; i < 3; i++ )
				{
					fgets( buf, 4095, fp );
					int dummy;
					int numRead = sscanf( buf, "%d %f %f %f %f %f %f %f %f", 
						&dummy,
						&tris[count].vert[i].x, &tris[count].vert[i].y, &tris[count].vert[i].z, 
						&tris[count].norm[i].x, &tris[count].norm[i].y, &tris[count].norm[i].z, 
						&tris[count].texCoord[i].u, &tris[count].texCoord[i].v );
					assert( numRead == 9 );
					if( tris[count].texCoord[i].u < 0.0f )
					{
						printf( "texCoord out of range!: %f\n", tris[count].texCoord[i].u );
						tris[count].texCoord[i].u = 0.0f;
					}
					if( tris[count].texCoord[i].v < 0.0f )
					{
						printf( "texCoord out of range!: %f\n", tris[count].texCoord[i].v );
						tris[count].texCoord[i].v = 0.0f;
					}
					if( tris[count].texCoord[i].u > 1.0f )
					{
						printf( "texCoord out of range!: %f\n", tris[count].texCoord[i].u );
						tris[count].texCoord[i].u = 1.0f;
					}
					if( tris[count].texCoord[i].v > 1.0f )
					{
						printf( "texCoord out of range!: %f\n", tris[count].texCoord[i].v );
						tris[count].texCoord[i].v = 1.0f;
					}
#if 0
					printf( "[%f %f %f] [%f %f %f] [%f %f]\n",
						tris[count].vert[i].x, tris[count].vert[i].y, tris[count].vert[i].z, 
						tris[count].norm[i].x, tris[count].norm[i].y, tris[count].norm[i].z, 
						tris[count].texCoord[i].u, tris[count].texCoord[i].v );
#endif

				}
				count++;
			}
		}
	}
}

bool SMDReadTriangles (FILE* fp, int* numTris, NmRawTriangle** tris)
{
	*numTris = CountTris( fp );
	*tris = new NmRawTriangle[*numTris];
	ReadTris( fp, *numTris, *tris );
	return true;
}


