// gl_vidnt.c -- NT GL vid component

#include "glquake.h"
#include "vid.h"
#include "gl_matsysiface.h"
#include "tgawriter.h"
#include "FileSystem.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"

// ------------------
// avoids including windows.h
struct RECT
{
	long left;
	long top;
	long right;
	long bottom;
};
// ------------------

extern IFileSystem *g_pFileSystem;

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)

qboolean	scr_skipupdate = false;

extern viddef_t	vid;				// global video state

// for recording movies
extern char cl_moviename[];
extern int cl_movieframe;

vrect_t window_rect;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VID_Init ( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VID_Shutdown( void )
{
	// Cleanup if needed
}

/*
===================
VID_TakeSnapshot

Write vid.buffer out as a windows bitmap file
*/
void VID_TakeSnapshot( const char *pFilename )
{
	// bitmap bits
	unsigned char *pImage = ( unsigned char * )malloc( vid.width * 3 * vid.height );

	// Get Bits from the material system
	materialSystemInterface->ReadPixels( 0, 0, vid.width, vid.height, pImage, IMAGE_FORMAT_RGB888 );

	if( !TGAWriter::Write( pImage, pFilename, vid.width, vid.height, IMAGE_FORMAT_RGB888,
		IMAGE_FORMAT_RGB888 ) )
	{
		Sys_Error( "Couldn't write bitmap data snapshot.\n" );
	}
	
	free( pImage );
}


void VID_TakeSnapshotRect( const char *pFilename, int x, int y, int w, int h, int resampleWidth, int resampleHeight )
{
	bool bReadPixelsFromFrontBuffer = g_pMaterialSystemHardwareConfig->ReadPixelsFromFrontBuffer();
	if( bReadPixelsFromFrontBuffer )
	{
		Shader_SwapBuffers();
	}

	// bitmap bits
	unsigned char *pImage = ( unsigned char * )malloc( w * h * 4 );
	unsigned char *pImage1 = ( unsigned char * )malloc( resampleWidth * resampleHeight * 4 );

	// Get Bits from the material system
	materialSystemInterface->ReadPixels( x, y, w, h, pImage, IMAGE_FORMAT_RGBA8888 );

	assert( w == h ); // garymcthack - this only works for square images

	if( !ImageLoader::ResampleRGBA8888( pImage, pImage1, w, h, resampleWidth, resampleHeight, 1.0f, 1.0f ) )
	{
		Sys_Error( "Can't resample\n" );
	}
	
	if( !TGAWriter::Write( pImage1, pFilename, resampleWidth, resampleHeight, IMAGE_FORMAT_RGBA8888,
		IMAGE_FORMAT_RGBA8888 ) )
	{
		Sys_Error( "Couldn't write bitmap data snapshot.\n" );
	}
	
	free( pImage1 );
	free( pImage );
	if( !bReadPixelsFromFrontBuffer )
	{
		Shader_SwapBuffers();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Writes the data in *data to the sequentially number .bmp file filename
// Input  : *filename - 
//			width - 
//			height - 
//			depth - 
//			*data - 
// Output : static void
//-----------------------------------------------------------------------------
static void VID_ProcessMovieFrame( const char *filename, int width, int height, byte *data )
{
	if( !TGAWriter::Write( data, filename, width, height, IMAGE_FORMAT_RGB888,
		IMAGE_FORMAT_RGB888 ) )
	{
		Sys_Error( "Couldn't write movie snapshot.\n" );
		Cbuf_AddText( "endmovie\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Store current frame to numbered .bmp file
// Input  : *pFilename - 
//-----------------------------------------------------------------------------
void VID_WriteMovieFrame( void )
{
	if ( !cl_moviename[0] )
	{
		Cbuf_AddText( "endmovie\n" );
		Con_Printf( "Tried to write movie buffer with no filename set!\n" );
		return;
	}

	int imagesize = vid.height * vid.width * 3;
	byte *hp = new byte[ imagesize ];
	if ( hp == NULL )
	{
		Sys_Error( "Couldn't allocate bitmap header to snapshot.\n" );
	}

	// Get Bits from material system
	materialSystemInterface->ReadPixels( 0, 0, vid.width, vid.height, hp, IMAGE_FORMAT_RGB888 );

	// Store frame to disk
	VID_ProcessMovieFrame( va( "%s%04d.tga", cl_moviename, cl_movieframe++ ), vid.width, vid.height, hp );

	delete[] hp;
}

void VID_UpdateWindowVars(void *prc, int x, int y)
{
	RECT *rect = (RECT *)prc;
	window_rect.x = rect->left;
	window_rect.y = rect->top;
	window_rect.width = rect->right - rect->left;
	window_rect.height = rect->bottom - rect->top;
}

void VID_UpdateVID(viddef_t *pvid)
{
	vid = *pvid;
}
