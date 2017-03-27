//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "mathlib.h"
#include "tgawriter.h"
#include "vstdlib/strtools.h"
#include "vtf/vtf.h"
#include "UtlBuffer.h"
#include "tier0/dbg.h"

SpewRetval_t VTF2TGAOutputFunc( SpewType_t spewType, char const *pMsg )
{
	printf( pMsg );

	if (spewType == SPEW_ERROR)
		return SPEW_ABORT;
	return (spewType == SPEW_ASSERT) ? SPEW_DEBUGGER : SPEW_CONTINUE; 
}

static void Usage( void )
{
	Error( "Usage: vtf2tga blah.vtf blah.tga\n" );
	exit( -1 );
}

int main( int argc, char **argv )
{
	SpewOutputFunc( VTF2TGAOutputFunc );
	MathLib_Init( 2.2f, 2.2f, 0.0f, 1.0f, false, false, false, false );
	if( argc != 3 )
	{
		Usage();
	}
	const char *pVTFFileName = argv[1];
	const char *pTGAFileName = argv[2];

	FILE *vtfFp = fopen( pVTFFileName, "rb" );
	if( !vtfFp )
	{
		Error( "Can't open %s\n", pVTFFileName );
		exit( -1 );
	}

	fseek( vtfFp, 0, SEEK_END );
	int srcVTFLength = ftell( vtfFp );
	fseek( vtfFp, 0, SEEK_SET );

	CUtlBuffer buf;
	buf.EnsureCapacity( srcVTFLength );
	fread( buf.Base(), 1, srcVTFLength, vtfFp );
	fclose( vtfFp );

	IVTFTexture *pTex = CreateVTFTexture();
	if (!pTex->Unserialize( buf ))
	{
		Error( "*** Error reading in .VTF file %s\n", pVTFFileName );
		exit(-1);
	}
	
	Msg( "vtf width: %d\n", pTex->Width() );
	Msg( "vtf height: %d\n", pTex->Height() );
	Msg( "vtf numFrames: %d\n", pTex->FrameCount() );

	Vector vecReflectivity = pTex->Reflectivity();
	Msg( "vtf reflectivity: %f %f %f\n", vecReflectivity[0], vecReflectivity[1], vecReflectivity[2] );
	Msg( "transparency: " );
	if( pTex->Flags() & TEXTUREFLAGS_EIGHTBITALPHA )
	{
		Msg( "eightbitalpha\n" );
	}
	else if( pTex->Flags() & TEXTUREFLAGS_ONEBITALPHA )
	{
		Msg( "onebitalpha\n" );
	}
	else
	{
		Msg( "noalpha\n" );
	}
	ImageFormat srcFormat = pTex->Format();
	Msg( "vtf format: %s\n", ImageLoader::GetName( srcFormat ) );
		
	int frameNum = 0;
	int mipLevel = 0;

	int iTGANameLen = Q_strlen( pTGAFileName );

	int iWidth = pTex->Width();
	int iHeight = pTex->Height();
	int iFaceCount = pTex->FaceCount();
	bool bIsCubeMap = pTex->IsCubeMap();

	for (int iCubeFace = 0; iCubeFace < iFaceCount; ++iCubeFace)
	{
		// Construct output filename
		static const char *pCubeFaceName[7] = { "rt", "lf", "bk", "ft", "up", "dn", "sph" };
		char *pTempNameBuf = (char *)stackalloc( iTGANameLen + 8 );
		Q_strncpy( pTempNameBuf, pTGAFileName, iTGANameLen + 1 );
		if (bIsCubeMap)
		{
			char *pExt = Q_strrchr( pTempNameBuf, '.' );
			if (!pExt)
				pExt = pTempNameBuf + iTGANameLen;
			Q_snprintf( pExt, iTGANameLen + 8, "%s.tga", pCubeFaceName[iCubeFace] ); 
		}

		unsigned char *pSrcImage = pTex->ImageData( 0, iCubeFace, 0 );
		ImageFormat dstFormat;
		if( ImageLoader::IsTransparent( srcFormat ) )
		{
			dstFormat = IMAGE_FORMAT_BGRA8888;
		}
		else
		{
			dstFormat = IMAGE_FORMAT_BGR888;
		}
	//	dstFormat = IMAGE_FORMAT_RGBA8888;
	//	dstFormat = IMAGE_FORMAT_RGB888;
	//	dstFormat = IMAGE_FORMAT_BGRA8888;
	//	dstFormat = IMAGE_FORMAT_BGR888;
	//	dstFormat = IMAGE_FORMAT_BGRA5551;
	//	dstFormat = IMAGE_FORMAT_BGR565;
	//	dstFormat = IMAGE_FORMAT_BGRA4444;
	//	printf( "dstFormat: %s\n", ImageLoader::GetName( dstFormat ) );
		unsigned char *pDstImage = new unsigned char[ImageLoader::GetMemRequired( iWidth, iHeight, dstFormat, false )];
		if( !ImageLoader::ConvertImageFormat( pSrcImage, srcFormat, 
			pDstImage, dstFormat, iWidth, iHeight, 0, 0 ) )
		{
			Error( "Error converting from %s to %s\n",
				ImageLoader::GetName( srcFormat ), ImageLoader::GetName( dstFormat ) );
			exit( -1 );
		}

		if( ImageLoader::IsTransparent( dstFormat ) && ( dstFormat != IMAGE_FORMAT_RGBA8888 ) )
		{
			unsigned char *tmpImage = pDstImage;
			pDstImage = new unsigned char[ImageLoader::GetMemRequired( iWidth, iHeight, IMAGE_FORMAT_RGBA8888, false )];
			if( !ImageLoader::ConvertImageFormat( tmpImage, dstFormat, pDstImage, IMAGE_FORMAT_RGBA8888,
				iWidth, iHeight, 0, 0 ) )
			{
				Error( "Error converting from %s to %s\n",
					ImageLoader::GetName( dstFormat ), ImageLoader::GetName( IMAGE_FORMAT_RGBA8888 ) );
			}
			dstFormat = IMAGE_FORMAT_RGBA8888;
		}
		else if( !ImageLoader::IsTransparent( dstFormat ) && ( dstFormat != IMAGE_FORMAT_RGB888 ) )
		{
			unsigned char *tmpImage = pDstImage;
			pDstImage = new unsigned char[ImageLoader::GetMemRequired( iWidth, iHeight, IMAGE_FORMAT_RGB888, false )];
			if( !ImageLoader::ConvertImageFormat( tmpImage, dstFormat, pDstImage, IMAGE_FORMAT_RGB888,
				iWidth, iHeight, 0, 0 ) )
			{
				Error( "Error converting from %s to %s\n",
					ImageLoader::GetName( dstFormat ), ImageLoader::GetName( IMAGE_FORMAT_RGB888 ) );
			}
			dstFormat = IMAGE_FORMAT_RGB888;
		}
		
		TGAWriter::Write( pDstImage, pTempNameBuf, iWidth, iHeight,
			dstFormat, dstFormat );
	}

	// leak leak leak leak leak, leak leak, leak leak (Blue Danube)
	return 0;
}