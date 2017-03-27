#include "quakedef.h"
#include "riff.h"
#include "snd_io.h"
#include "sys.h"
#include "FileSystem_Engine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Implements Audio IO on the engine's COMMON filesystem
//-----------------------------------------------------------------------------
class COM_IOReadBinary : public IFileReadBinary
{
public:
	int open( const char *pFileName );
	int read( void *pOutput, int size, int file );
	void seek( int file, int pos );
	unsigned int tell( int file );
	unsigned int size( int file );
	void close( int file );
};


// prepend sound/ to the filename -- all sounds are loaded from the sound/ directory
int COM_IOReadBinary::open( const char *pFileName )
{
	char namebuffer[512];

	FileHandle_t *hFile = new FileHandle_t;

	Q_strcpy(namebuffer, "sound");

//HACK HACK HACK  the server is sending back sound names with slashes in front... 
	if (pFileName[0]!='/')
		Q_strcat(namebuffer,"/");

	Q_strcat( namebuffer, pFileName );

	if ( COM_OpenFile( namebuffer, hFile ) < 0 )
	{
		delete hFile;
		hFile = 0;
		return 0;
	}

	return (int)hFile;
}

int COM_IOReadBinary::read( void *pOutput, int size, int file )
{
	FileHandle_t *fh = (FileHandle_t *)file;

	if ( !fh )
		return 0;

	return g_pFileSystem->Read( pOutput, size, *fh );
}

void COM_IOReadBinary::seek( int file, int pos )
{
	if ( !file )
		return;

	FileHandle_t *fh = (FileHandle_t *)file;

	g_pFileSystem->Seek( *fh, pos, FILESYSTEM_SEEK_HEAD );
}

unsigned int COM_IOReadBinary::tell( int file )
{
	if ( !file )
		return 0;

	FileHandle_t *fh = (FileHandle_t *)file;

	return g_pFileSystem->Tell( *fh );
}

unsigned int COM_IOReadBinary::size( int file )
{
	FileHandle_t *fh = (FileHandle_t *)file;
	if ( !fh )
		return 0;

	return g_pFileSystem->Size( *fh );
}

void COM_IOReadBinary::close( int file )
{
	FileHandle_t *fh = (FileHandle_t *)file;

	if ( !fh  )
		return;

	COM_CloseFile( *fh );
	delete fh;
}

static COM_IOReadBinary io;
IFileReadBinary *g_pSndIO = &io;

