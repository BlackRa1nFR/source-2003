#include <stdio.h>
#include "snd_mp3_source.h"
#include "riff.h"

#include "SoundService.h"
#include "snd_io.h"
#include "../../cache.h"
#include "vstdlib/strtools.h"
#include "tier0/dbg.h"
#include "snd_wave_mixer.h"
#include "snd_wave_data.h"

CAudioSourceMP3::CAudioSourceMP3( const char *pFileName )
{
	m_sampleRate = 44100;
	m_pName = pFileName;
}

// mixer's references
void CAudioSourceMP3::ReferenceAdd( CAudioMixer * )
{
	m_refCount++;
}

void CAudioSourceMP3::ReferenceRemove( CAudioMixer * )
{
	m_refCount--;
}

// check reference count, return true if nothing is referencing this
bool CAudioSourceMP3::CanDelete( void )
{
	return m_refCount > 0 ? false : true;
}


class CAudioSourceMP3Cache : public CAudioSourceMP3
{
public:
	CAudioSourceMP3Cache( const char *pName );
	~CAudioSourceMP3Cache( void );

	bool					IsCached( void );
	void					CacheTouch( void );
	void					CacheLoad( void );
	void					CacheUnload( void );
	// NOTE: "samples" are bytes for MP3
	int						GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] );
	CAudioMixer				*CreateMixer( void );

protected:
	virtual char			*GetDataPointer( void );

	cache_user_t	m_cache;

private:
	CAudioSourceMP3Cache( const CAudioSourceMP3Cache & );
};


//-----------------------------------------------------------------------------
// Purpose: NULL the wave data pointer (we haven't loaded yet)
//-----------------------------------------------------------------------------
CAudioSourceMP3Cache::CAudioSourceMP3Cache( const char *pName ) : 
	CAudioSourceMP3( pName )
{
	memset( &m_cache, 0, sizeof(m_cache) );
}


//-----------------------------------------------------------------------------
// Purpose: Free any wave data we've allocated
//-----------------------------------------------------------------------------
CAudioSourceMP3Cache::~CAudioSourceMP3Cache( void )
{
	CacheUnload();
}



bool CAudioSourceMP3Cache::IsCached( void )
{
	if ( m_cache.data )
	{
		if ( Cache_Check( &m_cache ) != NULL )
			return true;
	}

	return false;
}


void CAudioSourceMP3Cache::CacheTouch( void )
{
	IsCached();
}


void CAudioSourceMP3Cache::CacheLoad( void )
{
	if ( IsCached() )
		return;

	int file = g_pSndIO->open( m_pName );
	if ( !file )
		return;

	m_fileSize = g_pSndIO->size( file );
	// create a buffer for the samples
	char *pData = (char *)Cache_Alloc( &m_cache, m_fileSize, m_pName );
	// load them into memory
	g_pSndIO->read( pData, m_fileSize, file );

	g_pSndIO->close( file );
}

void CAudioSourceMP3Cache::CacheUnload( void )
{
	if ( !m_cache.data )
		return;

	Cache_Free( &m_cache );
}

char *CAudioSourceMP3Cache::GetDataPointer( void )
{
	char *pData = (char *)Cache_Check( &m_cache );
	if ( !pData )
		CacheLoad();
	return (char *)Cache_Check( &m_cache );
}

int CAudioSourceMP3Cache::GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
{
	// how many bytes are available ?
	int totalSampleCount = m_fileSize - samplePosition;

	// may be asking for a sample out of range, clip at zero
	if ( totalSampleCount < 0 )
		totalSampleCount = 0;

	// clip max output samples to max available
	if ( sampleCount > totalSampleCount )
		sampleCount = totalSampleCount;

	// if we are returning some samples, store the pointer
	if ( sampleCount )
	{
		*pData = GetDataPointer() + samplePosition;
		Assert( *pData );
	}

	return sampleCount;
}

CAudioMixer	*CAudioSourceMP3Cache::CreateMixer( void )
{
	CAudioMixer *pMixer = CreateMP3Mixer( CreateWaveDataMemory(*this) );
	ReferenceAdd( pMixer );

	return pMixer;
}


//-----------------------------------------------------------------------------
// Purpose: Streaming MP3 file
//-----------------------------------------------------------------------------
class CAudioSourceStreamMP3 : public CAudioSourceMP3, public IWaveStreamSource
{
public:
	CAudioSourceStreamMP3( const char * );
	~CAudioSourceStreamMP3() {}

	bool			IsStreaming( void ) { return true; }
	bool			IsStereoWav(void) { return false; }
	CAudioMixer		*CreateMixer( void );
	int				GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] );

	// IWaveStreamSource
	virtual int UpdateLoopingSamplePosition( int samplePosition )
	{
		return samplePosition;
	}
	virtual void UpdateSamples( char *pData, int sampleCount ) {}

private:
	CAudioSourceStreamMP3( const CAudioSourceStreamMP3 & ); // not implemented, not accessible
};

CAudioSourceStreamMP3::CAudioSourceStreamMP3( const char *pName ) :
	CAudioSourceMP3( pName )
{
	int file = g_pSndIO->open( pName );
	m_fileSize = g_pSndIO->size( file );
	g_pSndIO->close( file );
}

CAudioMixer	*CAudioSourceStreamMP3::CreateMixer( void )
{
	// BUGBUG: Source constructs the IWaveData, mixer frees it, fix this?
	IWaveData *pWaveData = CreateWaveDataStream(*this, static_cast<IWaveStreamSource *>(this), *g_pSndIO, m_pName, 0, m_fileSize );
	if ( pWaveData )
	{
		CAudioMixer *pMixer = CreateMP3Mixer( pWaveData );
		if ( pMixer )
		{
			ReferenceAdd( pMixer );
			return pMixer;
		}

		// no mixer, delete the stream buffer/instance
		delete pWaveData;
	}

	return NULL;
}

int	CAudioSourceStreamMP3::GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
{
	return 0;
}

bool Audio_IsMP3( const char *pName )
{
	int len = strlen(pName);
	if ( len > 4 )
	{
		if ( !Q_strnicmp( &pName[len - 4], ".mp3", 4 ) )
		{
			return true;
		}
	}
	return false;
}


CAudioSource *Audio_CreateStreamedMP3( const char *pName )
{
	return new CAudioSourceStreamMP3( pName );
}


CAudioSource *Audio_CreateMemoryMP3( const char *pName )
{
	CAudioSourceMP3Cache *pMP3 = new CAudioSourceMP3Cache( pName );
	pMP3->CacheLoad();
	if ( !pMP3->IsCached() )
	{
		delete pMP3;
		return NULL;
	}

	return pMP3;
}

