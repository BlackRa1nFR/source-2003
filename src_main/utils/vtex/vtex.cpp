//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "vstdlib/strtools.h"
#include <sys/stat.h>
#include "TGALoader.h"
#include "mathlib.h"
#include "conio.h"
#include <direct.h>
#include <io.h>
#include "vtf/vtf.h"
#include "UtlBuffer.h"
#include "tier0/dbg.h"
#include "cmdlib.h"
#include "vstdlib/icommandline.h"
#include "windows.h"

#ifdef VTEX_DLL
#include "ivtex.h"
#endif

#define FF_TRYAGAIN 1
#define FF_DONTPROCESS 2

#define LOWRESIMAGE_DIM 16
#define LOWRES_IMAGE_FORMAT IMAGE_FORMAT_DXT1
//#define DEBUG_NO_COMPRESSION
static bool g_NoPause = false;
static bool g_Quiet = false;
static const char *g_ShaderName = NULL;
static bool g_CreateDir = false;


static void Pause( void )
{
	if( !g_NoPause )
	{
		printf( "Hit a key to continue\n" );
		getch();
	}
}


//-----------------------------------------------------------------------------
// Computes the desired texture format based on flags
//-----------------------------------------------------------------------------
static ImageFormat ComputeDesiredImageFormat( IVTFTexture *pTexture, bool bDUDVTarget )
{
	ImageFormat targetFormat;

	int nFlags = pTexture->Flags();

	if( bDUDVTarget)
	{
		targetFormat = IMAGE_FORMAT_UV88;
//		targetFormat = IMAGE_FORMAT_UVWQ8888;
	}
	// can't compress textures that are smaller than 4x4
	else if( nFlags & TEXTUREFLAGS_NOCOMPRESS || nFlags & TEXTUREFLAGS_PROCEDURAL ||
		(pTexture->Width() < 4) || (pTexture->Height() < 4) )
	{
		if( nFlags & ( TEXTUREFLAGS_ONEBITALPHA | TEXTUREFLAGS_EIGHTBITALPHA ) )
		{
			targetFormat = IMAGE_FORMAT_BGRA8888;
		}
		else
		{
			targetFormat = IMAGE_FORMAT_BGR888;
		}
	}
	else if( nFlags & TEXTUREFLAGS_HINT_DXT5 )
	{
#ifdef DEBUG_NO_COMPRESSION
		targetFormat = IMAGE_FORMAT_BGRA8888;
#else
		targetFormat = IMAGE_FORMAT_DXT5;
#endif
	}
	else if( nFlags & TEXTUREFLAGS_EIGHTBITALPHA )
	{
		// compressed with alpha blending
#ifdef DEBUG_NO_COMPRESSION
		targetFormat = IMAGE_FORMAT_BGRA8888;
#else
		targetFormat = IMAGE_FORMAT_DXT5;
#endif
	}
	else if( nFlags & TEXTUREFLAGS_ONEBITALPHA )
	{
		// garymcthack - fixme IMAGE_FORMAT_DXT1_ONEBITALPHA doesn't work yet.
#ifdef DEBUG_NO_COMPRESSION
		targetFormat = IMAGE_FORMAT_BGRA8888;
#else
//		targetFormat = IMAGE_FORMAT_DXT1_ONEBITALPHA;
		targetFormat = IMAGE_FORMAT_DXT5;
#endif
	}
	else
	{
#ifdef DEBUG_NO_COMPRESSION
		targetFormat = IMAGE_FORMAT_BGR888;
#else
		targetFormat = IMAGE_FORMAT_DXT1;
#endif
	}
	return targetFormat;
} 


//-----------------------------------------------------------------------------
// Computes the low res image size
//-----------------------------------------------------------------------------
void VTFGetLowResImageInfo( int cacheWidth, int cacheHeight, int *lowResImageWidth, int *lowResImageHeight,
						    ImageFormat *imageFormat )
{
	if (cacheWidth > cacheHeight)
	{
		int factor = cacheWidth / LOWRESIMAGE_DIM;
		if (factor > 0)
		{
			*lowResImageWidth = LOWRESIMAGE_DIM;
			*lowResImageHeight = cacheHeight / factor;
		}
		else
		{
			*lowResImageWidth = cacheWidth;
			*lowResImageHeight = cacheHeight;
		}
	}
	else
	{
		int factor = cacheHeight / LOWRESIMAGE_DIM;
		if (factor > 0)
		{
			*lowResImageHeight = LOWRESIMAGE_DIM;
			*lowResImageWidth = cacheWidth / factor;
		}
		else
		{
			*lowResImageWidth = cacheWidth;
			*lowResImageHeight = cacheHeight;
		}
	}

	// Can end up with a dimension of zero for high aspect ration images.
	if( *lowResImageWidth < 1 )
	{
		*lowResImageWidth = 1;
	}
	if( *lowResImageHeight < 1 )
	{
		*lowResImageHeight = 1;
	}
	*imageFormat = LOWRES_IMAGE_FORMAT;
}


//-----------------------------------------------------------------------------
// This method creates the low-res image and hooks it into the VTF Texture
//-----------------------------------------------------------------------------
static void CreateLowResImage( IVTFTexture *pVTFTexture )
{
	int iWidth, iHeight;
	ImageFormat imageFormat;
	VTFGetLowResImageInfo( pVTFTexture->Width(), pVTFTexture->Height(), &iWidth, &iHeight, &imageFormat );

	// Allocate the low-res image data
	pVTFTexture->InitLowResImage( iWidth, iHeight, imageFormat );

	// Generate the low-res image bits
	if (!pVTFTexture->ConstructLowResImage())
	{
		fprintf( stderr, "Can't convert image from %s to %s in CalcLowResImage\n",
			ImageLoader::GetName(IMAGE_FORMAT_RGBA8888), ImageLoader::GetName(imageFormat) );
		Pause();
		exit( -1 );
	}
}


//-----------------------------------------------------------------------------
// Computes the source file name
//-----------------------------------------------------------------------------
void MakeSrcFileName( char *pSrcName, unsigned int flags, const char *pFullNameWithoutExtension_, int frameID, 
				      int faceID, bool isCubeMap, int startFrame, int endFrame, bool bNormalToDUDV )
{
	static const char *facingName[7] = { "rt", "lf", "bk", "ft", "up", "dn", "sph" };
	char	pFullNameWithoutExtension[MAX_PATH];
	strcpy( pFullNameWithoutExtension, pFullNameWithoutExtension_ );

	Assert( !( strstr( pFullNameWithoutExtension, "." ) ) );
	bool bAnimated = !( startFrame == -1 || endFrame == -1 );
	
	if( bNormalToDUDV )
	{
		char *pNormalString = Q_stristr( pFullNameWithoutExtension, "_dudv" );
		if( pNormalString )
		{
			Q_strcpy( pNormalString, "_normal" );
		}
		else
		{
			Assert( 0 );
		}
	}

	if( bAnimated )
	{
		if( isCubeMap )
		{
			sprintf( pSrcName, "%s%s%03d.tga", pFullNameWithoutExtension, facingName[faceID], frameID + startFrame );
		}
		else
		{
			sprintf( pSrcName, "%s%03d.tga", pFullNameWithoutExtension, frameID + startFrame );
		}
	}
	else
	{
		if( isCubeMap )
		{
			sprintf( pSrcName, "%s%s.tga", pFullNameWithoutExtension, facingName[faceID] );
		}
		else
		{
			sprintf( pSrcName, "%s.tga", pFullNameWithoutExtension );
		}
	}
}


//-----------------------------------------------------------------------------
// Loads a file into a UTLBuffer
//-----------------------------------------------------------------------------
static bool LoadFile( const char *pFileName, CUtlBuffer &buf, bool bFailOnError )
{
	FILE *fp = fopen( pFileName, "rb" );
	if (!fp)
	{
		if (!bFailOnError)
			return false;

		Error( "Can't open: \"%s\"\n", pFileName );
	}

	fseek( fp, 0, SEEK_END );
	int nFileLength = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	buf.EnsureCapacity( nFileLength );
	fread( buf.Base(), 1, nFileLength, fp );
	fclose( fp );
	return true;
}


//-----------------------------------------------------------------------------
// Creates a texture the size of the TGA image stored in the buffer
//-----------------------------------------------------------------------------
static void InitializeSrcTexture( IVTFTexture *pTexture, const char *pInputFileName,
								 CUtlBuffer &tgaBuffer, int nFrameCount, int nFlags )
{
	int nWidth, nHeight;
	ImageFormat imageFormat;
	float flSrcGamma; 
	bool ok = TGALoader::GetInfo( tgaBuffer, &nWidth, &nHeight, &imageFormat, &flSrcGamma );
	if (!ok)
	{
		Error( "TGA %s is bogus!\n", pInputFileName );
	}

	if (!pTexture->Init( nWidth, nHeight, IMAGE_FORMAT_DEFAULT, nFlags, nFrameCount ))
	{
		Error( "Error initializing texture %s\n", pInputFileName );
	}
}


//-----------------------------------------------------------------------------
// Loads a face from a TGA image
//-----------------------------------------------------------------------------
static bool LoadFaceFromTGA( IVTFTexture *pTexture, CUtlBuffer &tgaBuffer, int nFrame, int nFace, float flGamma )
{
	// NOTE: This only works because all mip levels are stored sequentially
	// in memory, starting with the highest mip level. It also only works
	// because the VTF Texture store *all* mip levels down to 1x1

	// Get the information from the file...
	int nWidth, nHeight;
	ImageFormat imageFormat;
	float flSrcGamma;
	bool ok = TGALoader::GetInfo( tgaBuffer, &nWidth, &nHeight, &imageFormat, &flSrcGamma );
	if (!ok)
		return false;

	if ((nWidth != pTexture->Width()) || (nHeight != pTexture->Height() ))
	{
		Error( "TGA file is an incorrect size!\n" );
		return false;
	}

	// Seek back so TGALoader::Load can see the tga header...
	tgaBuffer.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );

	// Load the tga and create all mipmap levels
	unsigned char *pDestBits = pTexture->ImageData( nFrame, nFace, 0 );
	return TGALoader::Load( pDestBits, tgaBuffer, pTexture->Width(), 
		pTexture->Height(), pTexture->Format(), flGamma, false );
}


//-----------------------------------------------------------------------------
// Loads source image data 
//-----------------------------------------------------------------------------
static bool LoadSourceImages( IVTFTexture *pTexture, const char *pFullNameWithoutExtension, 
							 int nFlags, int nStartFrame, int nEndFrame, bool bNormalToDUDV ) 
{
	static char pSrcName[1024];

	bool bGenerateSpheremaps = false;

	// The input file name here is simply for error reporting
	char *pInputFileName = ( char * )stackalloc( strlen( pFullNameWithoutExtension ) + strlen( ".tga" ) + 1 );
	strcpy( pInputFileName, pFullNameWithoutExtension );
	strcat( pInputFileName, ".tga" );
	
	int nFrameCount;
	bool bAnimated = !( nStartFrame == -1 || nEndFrame == -1 );
	if( !bAnimated )
	{
		nFrameCount = 1;
	}
	else
	{
		nFrameCount = nEndFrame - nStartFrame + 1;
	}

	bool bIsCubeMap = (nFlags & TEXTUREFLAGS_ENVMAP) != 0;

	// Iterate over all faces of all frames
	int nFaceCount = bIsCubeMap ? CUBEMAP_FACE_COUNT : 1;
	for( int iFrame = 0; iFrame < nFrameCount; ++iFrame )
	{
		for( int iFace = 0; iFace < nFaceCount; ++iFace )
		{
			// Generate the filename to load....
			MakeSrcFileName( pSrcName, nFlags, pFullNameWithoutExtension, 
				iFrame, iFace, bIsCubeMap, nStartFrame, nEndFrame, bNormalToDUDV );

			// Don't fail if the 7th iFace of a cubemap isn't loaded...
			// that just means that we're gonna have to build the spheremap ourself.
			bool bFailOnError = !bIsCubeMap || (iFace != CUBEMAP_FACE_SPHEREMAP);

			// Load the TGA from disk...
			CUtlBuffer tgaBuffer;
			if (!LoadFile( pSrcName, tgaBuffer, bFailOnError ))
			{
				// The only way we can get here is if LoadFile tried to load a spheremap and failed
				bGenerateSpheremaps = true;
				continue;
			}

			// Initialize the VTF Texture here if we haven't already....
			// Note that we have to do it here because we have to get the width + height from the file
			if (!pTexture->ImageData())
			{
				InitializeSrcTexture( pTexture, pInputFileName, tgaBuffer, nFrameCount, nFlags );

				// Re-seek back to the front of the buffer so LoadFaceFromTGA works
				tgaBuffer.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
			}

			// NOTE: This here will generate all mip levels of the source image
			if (!LoadFaceFromTGA( pTexture, tgaBuffer, iFrame, iFace, 2.2 ))
			{
				Error( "Error loading texture %s\n", pInputFileName );
			}
		}
	}

	return bGenerateSpheremaps;
}


//-----------------------------------------------------------------------------
// Does the dirty deed and generates a VTF file
//-----------------------------------------------------------------------------
void ProcessFiles( const char *pFullNameWithoutExtension, 
				   const char *pOutputDir, const char *pBaseName, 
				   unsigned int nFlags, int nStartFrame, int nEndFrame, 
				   bool isCubeMap, float flBumpScale, LookDir_t lookDir,
				   bool bNormalToDUDV, bool bDUDV )
{
	// force clamps/clampt for cube maps
	if( isCubeMap )
	{
		nFlags |= TEXTUREFLAGS_ENVMAP;
		nFlags |= TEXTUREFLAGS_CLAMPS;
		nFlags |= TEXTUREFLAGS_CLAMPT;
	}

	// Create the texture we're gonna store out
	IVTFTexture *pVTFTexture = CreateVTFTexture();

	// Load the source images into the texture
	bool bGenerateSpheremaps = LoadSourceImages( pVTFTexture, 
		pFullNameWithoutExtension, nFlags, nStartFrame, nEndFrame, bNormalToDUDV );

	// Bumpmap scale..
	pVTFTexture->SetBumpScale( flBumpScale );

	// Get the texture all internally consistent and happy
	pVTFTexture->PostProcess( bGenerateSpheremaps, lookDir );

	// Set up the low-res image
	if (!pVTFTexture->IsCubeMap())
	{
		CreateLowResImage( pVTFTexture );
	}

	// Compute the preferred image format
	ImageFormat vtfImageFormat = ComputeDesiredImageFormat( pVTFTexture, bNormalToDUDV || bDUDV );

	// Convert to the final format
	pVTFTexture->ConvertImageFormat( vtfImageFormat, bNormalToDUDV );
	
	// Write it!
	static char dstFileName[1024];
	sprintf( dstFileName, "%s/%s.vtf", pOutputDir, pBaseName );

	if ( g_CreateDir == true )
		 mkdir( pOutputDir ); //It'll create it if it doesn't exist.

	CUtlBuffer outputBuf;
	if (!pVTFTexture->Serialize( outputBuf ))
	{
		fprintf( stderr, "ERROR: \"%s\": Unable to serialize the VTF file!\n", dstFileName );
		Pause();
		exit( -1 );
	}

	FILE *fp = fopen( dstFileName, "wb" );
	if( !fp )
	{
		fprintf( stderr, "Can't open: %s\n", dstFileName );
		Pause();
		exit( -1 );
	}
	fwrite( outputBuf.Base(), 1, outputBuf.TellPut(), fp );
	fclose( fp );

	DestroyVTFTexture( pVTFTexture );
}

static bool GetKeyValueFromFP( FILE *fp, char **key, char **val )
{
	static char buf[2048];
	while( !feof( fp ) )
	{
		fgets( buf, 2047, fp );
		char *scan = buf;
		// search for the first quote for the key.
		while( 1 )
		{
			if( *scan == '\"' )
			{
				*key = ++scan;
				break;
			}
			if( *scan == '#' )
			{
				goto next_line; // comment
			}
			if( *scan == '\0' )
			{
				goto next_line; // end of line.
			}
			scan++;
		}
		// read the key until another quote.
		while( 1 )
		{
			if( *scan == '\"' )
			{
				*scan = '\0';
				scan++;
				break;
			}
			if( *scan == '\0' )
			{
				goto next_line;
			}
			scan++;
		}
		// search for the first quote for the value.
		while( 1 )
		{
			if( *scan == '\"' )
			{
				*val = ++scan;
				break;
			}
			if( *scan == '#' )
			{
				goto next_line; // comment
			}
			if( *scan == '\0' )
			{
				goto next_line; // end of line.
			}
			scan++;
		}
		// read the value until another quote.
		while( 1 )
		{
			if( *scan == '\"' )
			{
				*scan = '\0';
				scan++;
				// got a key and a value, so get the hell out of here.
				return true;
			}
			if( *scan == '\0' )
			{
				goto next_line;
			}
			scan++;
		}
next_line:
		;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Loads the .txt file associated with the .tga and gets out various data
//-----------------------------------------------------------------------------
static void LoadConfigFile( const char *pFileName, int *startFrame, int *endFrame, 
						   unsigned int *pFlags, float *bumpScale, LookDir_t *pLookDir, bool *bNormalToDUDV,
						   bool *bDUDV )
{
	*pFlags = 0;
	*bNormalToDUDV = false;
	*bDUDV = false;
	FILE *fp;
	fp = fopen( pFileName, "r" );
	if( !fp )
	{
		fprintf( stderr, "config file %s not present, making an empty one\n", pFileName );
		fp = fopen( pFileName, "w" );
		if( !fp )
		{
			fprintf( stderr, "Can't create default config file %s\n", pFileName );
			Pause();
			exit( -1 );
		}
		fclose( fp );
		return;
	}

	*pLookDir = LOOK_DOWN_Z;

	char *key = NULL;
	char *val = NULL;
	while( GetKeyValueFromFP( fp, &key, &val ) )
	{
		if( stricmp( key, "startframe" ) == 0 )
		{
			*startFrame = atoi( val );
		}
		else if( stricmp( key, "endframe" ) == 0 )
		{
			*endFrame = atoi( val );
		}
		else if( stricmp( key, "spheremap_x" ) == 0 )
		{
			*pLookDir = LOOK_DOWN_X;
		}
		else if( stricmp( key, "spheremap_negx" ) == 0 )
		{
			*pLookDir = LOOK_DOWN_NEGX;
		}
		else if( stricmp( key, "spheremap_y" ) == 0 )
		{
			*pLookDir = LOOK_DOWN_Y;
		}
		else if( stricmp( key, "spheremap_negy" ) == 0 )
		{
			*pLookDir = LOOK_DOWN_NEGY;
		}
		else if( stricmp( key, "spheremap_z" ) == 0 )
		{
			*pLookDir = LOOK_DOWN_Z;
		}
		else if( stricmp( key, "spheremap_negz" ) == 0 )
		{
			*pLookDir = LOOK_DOWN_NEGZ;
		}
		else if( stricmp( key, "bumpscale" ) == 0 )
		{
			*bumpScale = atof( val );
		}
		else if( stricmp( key, "pointsample" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_POINTSAMPLE;
		}
		else if( stricmp( key, "trilinear" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_TRILINEAR;
		}
		else if( stricmp( key, "clamps" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_CLAMPS;
		}
		else if( stricmp( key, "clampt" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_CLAMPT;
		}
		else if( stricmp( key, "anisotropic" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_ANISOTROPIC;
		}
		else if( stricmp( key, "dxt5" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_HINT_DXT5;
		}
		else if( stricmp( key, "nocompress" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_NOCOMPRESS;
		}
		else if( stricmp( key, "normal" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_NORMAL;
		}
		else if( stricmp( key, "nomip" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_NOMIP;
		}
		else if( stricmp( key, "nolod" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_NOLOD;
		}
		else if( stricmp( key, "minmip" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_MINMIP;
		}
		else if( stricmp( key, "procedural" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_PROCEDURAL;
		}
		else if( stricmp( key, "rendertarget" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_RENDERTARGET;
		}
		else if ( stricmp( key, "nodebug" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_NODEBUGOVERRIDE;
		}
		else if ( stricmp( key, "singlecopy" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_SINGLECOPY;
		}
		else if( stricmp( key, "oneovermiplevelinalpha" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_ONEOVERMIPLEVELINALPHA;
		}
		else if( stricmp( key, "premultcolorbyoneovermiplevel" ) == 0 )
		{
			*pFlags |= TEXTUREFLAGS_PREMULTCOLORBYONEOVERMIPLEVEL;
		}
		else if ( stricmp( key, "normaltodudv" ) == 0 )
		{
			if( atoi( val ) )
			{
				*bNormalToDUDV = true;
				*pFlags |= TEXTUREFLAGS_NORMALTODUDV;
			}
		}
		else if ( stricmp( key, "dudv" ) == 0 )
		{
			if( atoi( val ) )
			{
				*bDUDV = true;
			}
		}
	}

	if( ( *bNormalToDUDV || ( *pFlags & TEXTUREFLAGS_NORMALTODUDV ) ) &&
		!( *pFlags & TEXTUREFLAGS_PREMULTCOLORBYONEOVERMIPLEVEL ) )
	{
		printf( "Implicitly setting premultcolorbyoneovermiplevel since you are generating a dudv map\n" );
		*pFlags |= TEXTUREFLAGS_PREMULTCOLORBYONEOVERMIPLEVEL;
	}
	
	if( ( *bNormalToDUDV || ( *pFlags & TEXTUREFLAGS_NORMALTODUDV ) ) )
	{
		printf( "Implicitly setting trilinear since you are generating a dudv map\n" );
		*pFlags |= TEXTUREFLAGS_TRILINEAR;
	}
	
	if( Q_stristr( pFileName, "_normal" ) )
	{
		if( !( *pFlags & TEXTUREFLAGS_NORMAL ) )
		{
			if( !g_Quiet )
			{
				fprintf( stderr, "implicitly setting:\n" );
				fprintf( stderr, "\t\"normal\" \"1\"\n" );
				fprintf( stderr, "since filename ends in \"_normal\"\n" );
			}
			*pFlags |= TEXTUREFLAGS_NORMAL;
		}
	}
	
	if( Q_stristr( pFileName, "_dudv" ) )
	{
		if( !*bNormalToDUDV && !*bDUDV )
		{
			if( !g_Quiet )
			{
				fprintf( stderr, "Implicitly setting:\n" );
				fprintf( stderr, "\t\"dudv\" \"1\"\n" );
				fprintf( stderr, "since filename ends in \"_dudv\"\n" );
				fprintf( stderr, "If you are trying to convert from a normal map to a dudv map, put \"normaltodudv\" \"1\" in \"%s\".\n", pFileName );
			}
			*bDUDV = true;
		}
	}

	fclose( fp );
}

void Usage( void )
{
	fprintf( stderr, "Usage: vtex [-quiet] [-nopause] [-mkdir] [-shader ShaderName] tex1.txt tex2.txt . . .\n" );
	fprintf( stderr, "-quiet   : don't print anything out, don't pause for input\n" );
	fprintf( stderr, "-nopause : don't pause for input\n" );
	fprintf( stderr, "-mkdir   : create destination folder if it doesn't exist\n" );
	fprintf( stderr, "Note that you can use wildcards and that you can also chain them\n");
	fprintf( stderr, "e.g. materialsrc/monster1/*.tga materialsrc/monster2/*.tga\n" );
	Pause();
	exit( -1 );
}

bool GetOutputDir( const char *inputName, char *outputDir )
{
	if( !Q_stristr( inputName, gamedir ) )
	{
		char buf[1024];
		_getcwd( buf, 1024 );
		char* pSlash = strchr(buf, '\\');
		while (pSlash)
		{
			*pSlash = '/';
			pSlash = strchr(buf, '\\');
		}

		int len = strlen(buf);
		if (buf[len] != '/')
		{
			buf[len] = '/';
			++len;
		}
		strcpy( buf + len, inputName );
		if( !Q_stristr( buf, gamedir ) )
		{
			return false;
		}
		inputName = buf;
	}
	const char *pTmp = inputName + strlen( gamedir );
	if( !Q_stristr( pTmp, "materialsrc/" ) )
	{
		return false;
	}
	pTmp += strlen( "materialsrc/" );
	strcpy( outputDir, gamedir );
	strcat( outputDir, "materials/" );
	strcat( outputDir, pTmp );
	StripFilename( outputDir );
	if( !g_Quiet )
	{
		printf( "output directory: %s\n", outputDir );
	}
	return true;
}

bool IsCube( const char *inputName )
{
	char *tgaName = ( char * )stackalloc( strlen( inputName ) + 1 + strlen( "rt.tga" ) );
	strcpy( tgaName, inputName );
	StripExtension( tgaName );
	strcat( tgaName, "rt.tga" );
	struct	_stat buf;
	if( _stat( tgaName, &buf ) != -1 )
	{
		return true;
	}
	else
	{
		return false;
	}
}

int Find_Files( WIN32_FIND_DATA &wfd, HANDLE &hResult, const char *basedir, const char *extension )
{
	char	filename[MAX_PATH];

	BOOL bMoreFiles = FindNextFile( hResult, &wfd);
		
	if ( bMoreFiles )
	{
		// Skip . and ..
		if ( wfd.cFileName[0] == '.' )
		{
			return FF_TRYAGAIN;
		}

		// If it's a subdirectory, just recurse down it
		if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			char	subdir[MAX_PATH];
			sprintf( subdir, "%s\\%s", basedir, wfd.cFileName );

			// Recurse
			Find_Files( wfd, hResult, basedir, extension );
			return FF_TRYAGAIN;
		}
		
		// Check that it's a tga
		//
		
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		
		_splitpath( wfd.cFileName, NULL, NULL, fname, ext );

		// Not the type we want.
		if ( stricmp( ext, extension ) )
			 return FF_DONTPROCESS;

		// Check for .vmt
		sprintf( filename, "%s\\%s.vmt", basedir, fname );
		// Exists, so don't overwrite it
		if ( access( filename, 0 ) != -1 )
			 return FF_TRYAGAIN;

		char texturename[ _MAX_PATH ];
		char *p = ( char * )basedir;

		// Skip over the base path to get a material system relative path
		p += strlen( wfd.cFileName ) + 1;

		// Construct texture name
		sprintf( texturename, "%s\\%s", p, fname );
		
		// Convert all to lower case
		strlwr( texturename );
		strlwr( filename );
	}

	return bMoreFiles;
}

bool Process_File( char *pInputBaseName )
{
	int startFrame = -1;
	int endFrame = -1;
	char outputDir[1024];
	
	COM_FixSlashes( pInputBaseName );
	StripExtension( pInputBaseName );
	
	if( !g_Quiet )
	{
		printf( "input file: %s\n", pInputBaseName );
	}

	if( !GetOutputDir( pInputBaseName, outputDir ) )
	{
		fprintf( stderr, "Problem figuring out outputdir for %s\n", pInputBaseName );
		Pause();
		return FALSE;
	}
	bool isCubeMap = IsCube( pInputBaseName );
	
	char *configFileName = ( char * )stackalloc( strlen( pInputBaseName ) + strlen( ".txt" ) + 1 );
	strcpy( configFileName, pInputBaseName );
	strcat( configFileName, ".txt" );
	float bumpScale = 1.0f;
	unsigned int flags = 0;
	LookDir_t lookDir;
	bool bNormalToDUDV, bDUDV;
	LoadConfigFile( configFileName, &startFrame, &endFrame, &flags, &bumpScale, &lookDir, &bNormalToDUDV, &bDUDV );
	
	if( ( startFrame == -1 && endFrame != -1 ) ||
		( startFrame != -1 && endFrame == -1 ) )
	{
		fprintf( stderr, "ERROR: If you use startframe, you must use endframe, and vice versa.\n" );
		Pause();
		return FALSE;
	}
	
	const char *pBaseName = &pInputBaseName[strlen( pInputBaseName ) - 1];
	while( (pBaseName >= pInputBaseName) && *pBaseName != '\\' && *pBaseName != '/' )
	{
		pBaseName--;
	}
	pBaseName++;
		
	ProcessFiles( pInputBaseName, outputDir, pBaseName, flags, startFrame, endFrame, isCubeMap, bumpScale, lookDir, bNormalToDUDV, bDUDV );

	// create vmts if necessary
	if( g_ShaderName )
	{
		char buf[1024];
		sprintf( buf, "%s/%s.vmt", outputDir, pBaseName );
		const char *tmp = Q_stristr( outputDir, "materials" );
		FILE *fp;
		if( tmp )
		{
			// check if the file already exists.
			fp = fopen( buf, "r" );
			if( fp )
			{
				fprintf( stderr, "vmt file \"%s\" already exists\n", buf );
				fclose( fp );
			}
			else
			{
				fp = fopen( buf, "w" );
				if( fp )
				{
					fprintf( stderr, "Creating vmt file: %s/%s\n", tmp, pBaseName );
					tmp += strlen( "materials/" );
					fprintf( fp, "\"%s\"\n", g_ShaderName );
					fprintf( fp, "{\n" );
					fprintf( fp, "\t\"$baseTexture\" \"%s/%s\"\n", tmp, pBaseName );
					fprintf( fp, "}\n" );
					fclose( fp );
				}
				else
				{
					fprintf( stderr, "Couldn't open \"%s\" for writing\n", buf );
					Pause();
				}
			}

		}
		else
		{
			fprintf( stderr, "Couldn't find \"materials/\" in output path\n", buf );
			Pause();
		}
	}

	return TRUE;
}

static SpewRetval_t VTexOutputFunc( SpewType_t spewType, char const *pMsg )
{
	printf( pMsg );

	if (spewType == SPEW_ERROR)
	{
		Pause();
		return SPEW_ABORT;
	}
	return (spewType == SPEW_ASSERT) ? SPEW_DEBUGGER : SPEW_CONTINUE; 
}


#ifdef VTEX_DLL
class CVTex : public IVTex
{
public:
	int VTex( int argc, char **argv );
};

int CVTex::VTex( int argc, char **argv )
#else
int main( int argc, char **argv )
#endif
{
#ifndef VTEX_DLL
	CommandLine()->CreateCmdLine( argc, argv );
	SpewOutputFunc( VTexOutputFunc );
#endif

	MathLib_Init(  2.2f, 2.2f, 0.0f, 1.0f, false, false, false, false );
	if( argc < 2 )
	{
		Usage();
	}
	char *pInputBaseName = NULL;
	int i;
	i = 1;
	while( i < argc )
	{
		if( stricmp( argv[i], "-quiet" ) == 0 )
		{
			i++;
			g_Quiet = true;
			g_NoPause = true; // no point in pausing if we aren't going to print anything out.
		}
		if( stricmp( argv[i], "-nopause" ) == 0 )
		{
			i++;
			g_NoPause = true;
		}
		if ( stricmp( argv[i], "-mkdir" ) == 0 ) 
		{
			i++;
			g_CreateDir = true;
		}
		else if( stricmp( argv[i], "-shader" ) == 0 )
		{
			i++;
			if( i < argc )
			{
				g_ShaderName = argv[i];
				i++;
			}
		}
		else
		{
			break;
		}
	}

	CmdLib_InitFileSystem( argv[i] );

	for( ; i < argc; i++ )
	{
		pInputBaseName = argv[i];

		if ( strstr( pInputBaseName, "*.") )
		{
			char	search[ 128 ];
			char	basedir[MAX_PATH];
			char	ext[_MAX_EXT];
			char    filename[_MAX_FNAME];

			_splitpath( pInputBaseName, NULL, NULL, NULL, ext ); //find extension wanted

			ExtractFilePath ( pInputBaseName, basedir);
			sprintf( search, "%s\\*.*", basedir );

			WIN32_FIND_DATA wfd;
			HANDLE hResult;
			memset(&wfd, 0, sizeof(WIN32_FIND_DATA));
			
			hResult = FindFirstFile( search, &wfd );
						
			if ( hResult != INVALID_HANDLE_VALUE )
			{
				sprintf( filename, "%s%s", basedir, wfd.cFileName );

				if ( wfd.cFileName[0] != '.' ) 
					 Process_File( filename );
							
				int iFFType = Find_Files( wfd, hResult, basedir, ext );

				while ( iFFType )
				{
					sprintf( filename, "%s%s", basedir, wfd.cFileName );

					if ( wfd.cFileName[0] != '.' && iFFType != FF_DONTPROCESS ) 
						 Process_File( filename );

					iFFType = Find_Files( wfd, hResult, basedir, ext );
				}

				if ( iFFType == 0 )
					 FindClose( hResult );
			}
		}
		else
		{
			Process_File( pInputBaseName ); 			
		}
	}

	Pause();
	return 0;
}

#ifdef VTEX_DLL
EXPOSE_SINGLE_INTERFACE( CVTex, IVTex, IVTEX_VERSION_STRING );
#endif
