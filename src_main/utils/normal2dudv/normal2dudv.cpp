//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdlib.h>
#include <stdio.h>
#include "ImageLoader.h"
#include "TGALoader.h"
#include "TGAWriter.h"

int main( int argc, char **argv )
{
	if( argc != 3 )
	{
		fprintf( stderr, "Usage: normal2dudv in.tga out.tga\n" );
		exit( -1 );
	}

	enum ImageFormat imageFormat;
	int width, height;
	float sourceGamma;
	if( !TGALoader::GetInfo( argv[1], &width, &height, &imageFormat, &sourceGamma ) )
	{
		fprintf( stderr, "%s not found\n", argv[1] );
		exit( -1 );
	}

	int memRequired = ImageLoader::GetMemRequired( width, height, IMAGE_FORMAT_RGB888, false );
	unsigned char *pImageRGB888 = new unsigned char[memRequired];
	
	TGALoader::Load( pImageRGB888, argv[1], width, height, IMAGE_FORMAT_RGB888, sourceGamma, false );
	
	char *pImageDUDV = new char[memRequired];

	int i;
	for( i = 0; i < width * height; i++ )
	{
		pImageDUDV[i*3] = ( char )( ( ( int )pImageRGB888[i*3] ) - 127 );
		pImageDUDV[i*3+1] = ( char )( ( ( int )pImageRGB888[i*3+1] ) - 127 );
		pImageDUDV[i*3+2] = ( char )127;
	}
	
	TGAWriter::Write( ( unsigned char * )pImageDUDV, argv[2], width, height, IMAGE_FORMAT_RGB888, IMAGE_FORMAT_RGB888 );
	
	return 0;
}
