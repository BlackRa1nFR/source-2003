//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "vbsp.h"
#include "bsplib.h"
#include "UtlBuffer.h"
#include "UtlVector.h"
#include "ImageLoader.h"
#include <KeyValues.h>
#include "vstdlib/strtools.h"
#include "utlsymbol.h"
#include "vtf/vtf.h"
#include "materialpatch.h"

static CUtlVector<char *> s_DefaultCubemapNames;

void Cubemap_InsertSample( const Vector& origin, int size )
{
	dcubemapsample_t *pSample = &g_CubemapSamples[g_nCubemapSamples];
	pSample->origin[0] = ( int )origin[0];	
	pSample->origin[1] = ( int )origin[1];	
	pSample->origin[2] = ( int )origin[2];	
	pSample->size = size;
	g_nCubemapSamples++;
}

static const char *FindSkyboxMaterialName( void )
{
	for( int i = 0; i < num_entities; i++ )
	{
		char* pEntity = ValueForKey(&entities[i], "classname");
		if (!strcmp(pEntity, "worldspawn"))
		{
			return ValueForKey( &entities[i], "skyname" );
		}
	}
	return NULL;
}

static void BackSlashToForwardSlash( char *pname )
{
	while ( *pname ) {
		if ( *pname == '\\' )
			*pname = '/';
		pname++;
	}
}

static void ForwardSlashToBackSlash( char *pname )
{
	while ( *pname ) {
		if ( *pname == '/' )
			*pname = '\\';
		pname++;
	}
}

static bool LoadSrcVTFFiles( IVTFTexture *pSrcVTFTextures[6], const char *pSkyboxBaseName )
{
	const char *facingName[6] = { "rt", "lf", "bk", "ft", "up", "dn" };
	int i;
	for( i = 0; i < 6; i++ )
	{
		char srcVTFFileName[1024];
		sprintf( srcVTFFileName, "%smaterials/skybox/%s%s.vtf", gamedir, pSkyboxBaseName, facingName[i] );

		FILE *fp;
		fp = fopen( srcVTFFileName, "rb" );
		if( !fp )
		{
			return false;
		}
		fseek( fp, 0, SEEK_END );
		int srcVTFLength = ftell( fp );
		fseek( fp, 0, SEEK_SET );

		CUtlBuffer buf;
		buf.EnsureCapacity( srcVTFLength );
		fread( buf.Base(), 1, srcVTFLength, fp );
		fclose( fp );

		pSrcVTFTextures[i] = CreateVTFTexture();
		if (!pSrcVTFTextures[i]->Unserialize(buf))
		{
			Warning("*** Error unserializing skybox texture: %s\n", pSkyboxBaseName );
			return false;
		}

		if ( (pSrcVTFTextures[i]->Width() != pSrcVTFTextures[0]->Width()) ||
			 (pSrcVTFTextures[i]->Flags() != pSrcVTFTextures[0]->Flags()) )
		{
			Warning("*** Error: Skybox vtf files for %s weren't compiled with the same size texture and/or same flags!\n", pSkyboxBaseName );
			return false;
		}
	}

	return true;
}

#define DEFAULT_CUBEMAP_SIZE 32

void Cubemap_CreateDefaultCubemaps( void )
{
	// NOTE: This implementation depends on the fact that all VTF files contain
	// all mipmap levels
	const char *pSkyboxBaseName = FindSkyboxMaterialName();

	IVTFTexture *pSrcVTFTextures[6];

	if( !pSkyboxBaseName )
	{
		if( s_DefaultCubemapNames.Count() )
		{
			Warning( "This map uses env_cubemap, and you don't have a skybox, so no default env_cubemaps will be generated.\n" );
		}
		return;
	}

	if( !LoadSrcVTFFiles( pSrcVTFTextures, pSkyboxBaseName ) )
	{
		Warning( "Can't load skybox file %s to build the default cubemap!\n", pSkyboxBaseName );
		return;
	}
	Msg( "Creating default cubemaps for env_cubemap using skybox %s...\n"
		"Run buildcubemaps in the engine to get the correct cube maps.\n", pSkyboxBaseName );
			
	// Figure out the mip differences between the two textures
	int iMipLevelOffset = 0;
	int tmp = pSrcVTFTextures[0]->Width();
	while( tmp > DEFAULT_CUBEMAP_SIZE )
	{
		iMipLevelOffset++;
		tmp >>= 1;
	}

	// Create the destination cubemap
	IVTFTexture *pDstCubemap = CreateVTFTexture();
	pDstCubemap->Init( DEFAULT_CUBEMAP_SIZE, DEFAULT_CUBEMAP_SIZE, 
		pSrcVTFTextures[0]->Format(), pSrcVTFTextures[0]->Flags() | TEXTUREFLAGS_ENVMAP, 
		pSrcVTFTextures[0]->FrameCount() );

	// First iterate over all frames
	for (int iFrame = 0; iFrame < pDstCubemap->FrameCount(); ++iFrame)
	{
		// Next iterate over all normal cube faces (we know there's 6 cause it's an envmap)
		for (int iFace = 0; iFace < 6; ++iFace )
		{
			// Finally, iterate over all mip levels in the *destination*
			for (int iMip = 0; iMip < pDstCubemap->MipCount(); ++iMip )
			{
				// Copy the bits from the source images into the cube faces
				unsigned char *pSrcBits = pSrcVTFTextures[iFace]->ImageData( iFrame, 0, iMip + iMipLevelOffset );
				unsigned char *pDstBits = pDstCubemap->ImageData( iFrame, iFace, iMip );
				int iSize = pDstCubemap->ComputeMipSize( iMip );

				memcpy( pDstBits, pSrcBits, iSize ); 
			}
		}
	}

	// Convert the cube to format that we can apply tools to it...
	ImageFormat originalFormat = pDstCubemap->Format();
	pDstCubemap->ConvertImageFormat( IMAGE_FORMAT_DEFAULT, false );

	// Fixup the cubemap facing
	pDstCubemap->FixCubemapFaceOrientation();

	// Now that the bits are in place, compute the spheremaps...
	pDstCubemap->GenerateSpheremap();

	// Convert the cubemap to the final format
	pDstCubemap->ConvertImageFormat( originalFormat, false );

	// Write the puppy out!
	char dstVTFFileName[1024];
	sprintf( dstVTFFileName, "materials/maps/%s/cubemapdefault.vtf", mapbase );

	CUtlBuffer outputBuf;
	if (!pDstCubemap->Serialize( outputBuf ))
	{
		Warning( "Error serializing default cubemap %s\n", dstVTFFileName );
		return;
	}

	// stpie out the default one.
	AddBufferToPack( dstVTFFileName, outputBuf.Base(), outputBuf.TellPut(), false );

	// spit out all of the ones that are attached to world geometry.
	int i;
	for( i = 0; i < s_DefaultCubemapNames.Count(); i++ )
	{
		AddBufferToPack( s_DefaultCubemapNames[i], outputBuf.Base(),outputBuf.TellPut(), false );
	}

	// Clean up the textures
	for( i = 0; i < 6; i++ )
	{
		DestroyVTFTexture( pSrcVTFTextures[i] );
	}
	DestroyVTFTexture( pDstCubemap );
}	

typedef CUtlVector<int> IntVector_t;
static CUtlVector<IntVector_t> s_EnvCubemapToBrushSides;

void Cubemap_SaveBrushSides( const char *pSideListStr )
{
	IntVector_t &brushSidesVector = s_EnvCubemapToBrushSides[s_EnvCubemapToBrushSides.AddToTail()];
	char *pTmp = ( char * )_alloca( strlen( pSideListStr ) + 1 );
	strcpy( pTmp, pSideListStr );
	const char *pScan = strtok( pTmp, " " );
	if( !pScan )
	{
		return;
	}
	do
	{
		int brushSideID;
		if( sscanf( pScan, "%d", &brushSideID ) == 1 )
		{
			brushSidesVector.AddToTail( brushSideID );
		}
	} while( ( pScan = strtok( NULL, " " ) ) );
}

static int Cubemap_CreateTexInfo( int originalTexInfo, int origin[3] )
{
	texinfo_t *pTexInfo = &texinfo[originalTexInfo];
	dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );

	char stringToSearchFor[512];
	sprintf( stringToSearchFor, "_%d_%d_%d", origin[0], origin[1], origin[2] );

	// Get out of here if the originalTexInfo is already a generated material for this position.
	if( Q_stristr( TexDataStringTable_GetString( pTexData->nameStringTableID ),
		stringToSearchFor ) )
	{
		return originalTexInfo;
	}

	char generatedTexDataName[1024];
	sprintf( generatedTexDataName, "maps/%s/%s_%d_%d_%d", mapbase, 
		TexDataStringTable_GetString( pTexData->nameStringTableID ), 
		( int )origin[0], ( int )origin[1], ( int )origin[2] );
	BackSlashToForwardSlash( generatedTexDataName );
	static CUtlSymbolTable complainedSymbolTable( 0, 32, true );

	int texDataID = -1;
	bool hasTexData = false;
	// Make sure the texdata doesn't already exist.
	int i;
	for( i = 0; i < numtexdata; i++ )
	{
		if( stricmp( TexDataStringTable_GetString( GetTexData( i )->nameStringTableID ), 
			generatedTexDataName ) == 0 )
		{
			hasTexData = true;
			texDataID = i;
			break;
		}
	}
	
	if( !hasTexData )
	{
		char originalMatName[1024];
		char generatedMatName[1024];
		sprintf( originalMatName, "%s", 
			TexDataStringTable_GetString( pTexData->nameStringTableID ) );
		sprintf( generatedMatName, "maps/%s/%s_%d_%d_%d", mapbase, 
			TexDataStringTable_GetString( pTexData->nameStringTableID ), 
			( int )origin[0], ( int )origin[1], ( int )origin[2] );
		strlwr( generatedTexDataName );

		char envmapVal[512];
		GetValueFromMaterial( originalMatName, "$envmap", envmapVal, 511 );
		if( !GetValueFromMaterial( originalMatName, "$envmap", envmapVal, 511 ) )
		{
			if( UTL_INVAL_SYMBOL == complainedSymbolTable.Find( originalMatName ) )
			{
				Warning( "Ignoring env_cubemap on \"%s\" which doesn't use an envmap\n", originalMatName );
				complainedSymbolTable.AddString( originalMatName );
			}
			return originalTexInfo;
		}
		if( stricmp( envmapVal, "env_cubemap" ) != 0 )
		{
			if( UTL_INVAL_SYMBOL == complainedSymbolTable.Find( originalMatName ) )
			{
				Warning( "Ignoring env_cubemap on \"%s\" which uses envmap \"%s\"\n", originalMatName, envmapVal );
				complainedSymbolTable.AddString( originalMatName );
			}
			return originalTexInfo;
		}

		// Patch "$envmap"
		char textureName[1024];
		sprintf( textureName, "maps/%s/c%d_%d_%d", mapbase, ( int )origin[0], 
			( int )origin[1], ( int )origin[2] );

		CreateMaterialPatch( originalMatName, generatedMatName, "$envmap",
			textureName );
		
		// Store off the name of the cubemap that we need to create.
		char fileName[1024];
		sprintf( fileName, "materials/%s.vtf", textureName );
		int id = s_DefaultCubemapNames.AddToTail();
		s_DefaultCubemapNames[id] = new char[strlen( fileName ) + 1];
		strcpy( s_DefaultCubemapNames[id], fileName );

		// Make a new texdata
		assert( strlen( generatedTexDataName ) < TEXTURE_NAME_LENGTH - 1 );
		if( !( strlen( generatedTexDataName ) < TEXTURE_NAME_LENGTH - 1 ) )
		{
			Error( "generatedTexDataName: %s too long!\n", generatedTexDataName );
		}
		strlwr( generatedTexDataName );
		texDataID = AddCloneTexData( pTexData, generatedTexDataName );
		
	}

	assert( texDataID != -1 );
	
	texinfo_t newTexInfo;
	newTexInfo = *pTexInfo;
	newTexInfo.texdata = texDataID;
	
	int texInfoID = -1;

	// See if we need to make a new texinfo
	bool hasTexInfo = false;
	if( !hasTexData )
	{
		hasTexInfo = false;
	}
	else
	{
		for( i = 0; i < numtexinfo; i++ )
		{
			if( texinfo[i].texdata != texDataID )
			{
				continue;
			}
			if( memcmp( &texinfo[i], &newTexInfo, sizeof( texinfo_t ) ) == 0 )
			{
				hasTexInfo = true;
				texInfoID = i;
				break;
			}
		}
	}
	
	// Make a new texinfo if we need to.
	if( !hasTexInfo )
	{
		if ( numtexinfo >= MAX_MAP_TEXINFO )
		{
			Error( "Map has too many texinfos, MAX_MAP_TEXINFO == %i\n", MAX_MAP_TEXINFO );
		}
		texinfo[numtexinfo] = newTexInfo;
		texInfoID = numtexinfo;
		numtexinfo++;
	}

	assert( texInfoID != -1 );
	return texInfoID;
}

static void Cubemap_CreateMaterialForBrushSide( int sideIndex, int origin[3] )
{
	side_t *pSide = &brushsides[sideIndex];
	int originalTexInfoID = pSide->texinfo;
	pSide->texinfo = Cubemap_CreateTexInfo( originalTexInfoID, origin );
}

static int SideIDToIndex( int brushSideID )
{
	int i;
	for( i = 0; i < nummapbrushsides; i++ )
	{
		if( brushsides[i].id == brushSideID )
		{
			return i;
		}
	}
	assert( 0 );
	return -1;
}

void Cubemap_FixupBrushSidesMaterials( void )
{
	Msg( "fixing up env_cubemap materials on brush sides...\n" );
	assert( s_EnvCubemapToBrushSides.Count() == g_nCubemapSamples );

	int cubemapID;
	for( cubemapID = 0; cubemapID < g_nCubemapSamples; cubemapID++ )
	{
		IntVector_t &brushSidesVector = s_EnvCubemapToBrushSides[cubemapID];
		int i;
		for( i = 0; i < brushSidesVector.Count(); i++ )
		{
			int brushSideID = brushSidesVector[i];
			int sideIndex = SideIDToIndex( brushSideID );
			if( sideIndex < 0 )
			{
				continue;
			}
			Cubemap_CreateMaterialForBrushSide( sideIndex, g_CubemapSamples[cubemapID].origin );
			side_t *pSide = &brushsides[sideIndex];
			texinfo_t *pTexInfo = &texinfo[pSide->texinfo];
			dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
		}
	}
}

void Cubemap_FixupDispMaterials( void )
{
//	printf( "fixing up env_cubemap materials on displacements...\n" );
	assert( s_EnvCubemapToBrushSides.Count() == g_nCubemapSamples );

	// FIXME: this is order N to the gazillion!
	int cubemapID;
	for( cubemapID = 0; cubemapID < g_nCubemapSamples; cubemapID++ )
	{
		IntVector_t &brushSidesVector = s_EnvCubemapToBrushSides[cubemapID];
		int i;
		for( i = 0; i < brushSidesVector.Count(); i++ )
		{
			// brushSideID is the unique id for the side
			int brushSideID = brushSidesVector[i];
			int j;
			for( j = 0; j < numfaces; j++ )
			{
				dface_t *pFace = &dfaces[j];
				if( pFace->dispinfo == -1 )
				{
					continue;
				}
				mapdispinfo_t *pDispInfo = &mapdispinfo[pFace->dispinfo];
				if( brushSideID == pDispInfo->brushSideID )
				{
					pFace->texinfo = Cubemap_CreateTexInfo( pFace->texinfo, g_CubemapSamples[cubemapID].origin );
				}
			}
		}
	}
}

void Cubemap_MakeDefaultVersionsOfEnvCubemapMaterials( void )
{
	int i;
	char originalMaterialName[1024];
	char patchMaterialName[1024];
	char defaultVTFFileName[1024];
	sprintf( defaultVTFFileName, "maps/%s/cubemapdefault", mapbase );
	for( i = 0; i < numtexdata; i++ )
	{
		dtexdata_t *pTexData = GetTexData( i );
		sprintf( originalMaterialName, "%s", 
			TexDataStringTable_GetString( pTexData->nameStringTableID ) );
		sprintf( patchMaterialName, "maps/%s/%s", 
			mapbase,
			TexDataStringTable_GetString( pTexData->nameStringTableID ) );

		// Skip generated materials FIXME!!!
//		if( Q_stristr( originalVMTName, "materials/maps" ) )
//		{
//			continue;
//		}
		
		// Check to make sure that it has an envmap
		char oldEnvmapName[512];
		if( !GetValueFromMaterial( originalMaterialName, "$envmap", oldEnvmapName, 511 ) )
		{
			continue;
		}
		
		CreateMaterialPatch( originalMaterialName, patchMaterialName,
							 "$envmap", defaultVTFFileName );
		// FIXME!:  Should really remove the old name from the string table
		// it it isn't used.
		
		// Fix the texdata so that it points at the new vmt.
		char newTexDataName[1024];
		sprintf( newTexDataName, "maps/%s/%s", mapbase, 
			TexDataStringTable_GetString( pTexData->nameStringTableID ) );
		pTexData->nameStringTableID = TexDataStringTable_AddOrFindString( newTexDataName );
	}
}
