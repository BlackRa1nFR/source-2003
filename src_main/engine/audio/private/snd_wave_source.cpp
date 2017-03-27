
#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 1)
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>			// adpcm format
#pragma warning(pop)

#include <stdio.h>
#include <stdlib.h>

#include "riff.h"
#include "snd_wave_source.h"
#include "snd_wave_mixer_private.h"
#include "snd_audio_source.h"
#include "snd_wave_data.h"
#include "utlbuffer.h"
#include "cache_user.h"

#include "SoundService.h"
#include "snd_io.h"
#include "../../cache.h"
#include "vstdlib/strtools.h"
#include "snd_mp3_source.h"
#include "utlsymbol.h"
#include "filesystem.h"
#include "../../filesystem_engine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// #define DEBUG_CHUNKS

#if defined( DEBUG_CHUNKS ) && defined( _DEBUG )

#define WAVE_LIST	MAKEID( 'L', 'I', 'S', 'T' ) 
//-----------------------------------------------------------------------------
// Purpose: Report chunk error
// Input  : id - chunk FOURCC
//-----------------------------------------------------------------------------
void ChunkError( unsigned int id )
{
	// Legacy format, just ignore
	if ( id == WAVE_LIST )
		return;

	char tmp[256];
	char idname[5];
	idname[4] = 0;
	memcpy( idname, &id, 4 );

	Q_snprintf( tmp, sizeof( tmp ), "Unhandled chunk %s\n", idname );
	OutputDebugString( tmp );
}

#else
void ChunkError( unsigned int id )
{
}

#endif


//-----------------------------------------------------------------------------
// Purpose: Init to empty wave
//-----------------------------------------------------------------------------
CAudioSourceWave::CAudioSourceWave( const char *pName )
{
	m_format = 0;
	m_pHeader = NULL;
	// no looping
	m_loopStart = -1;
	m_sampleSize = 1;
	m_sampleCount = 0;

	m_refCount = 0;

	m_pName = pName;
}


CAudioSourceWave::~CAudioSourceWave( void )
{
#if _DEBUG
	if ( !CanDelete() )
		Assert(0);
#endif

	// for non-standard waves, we store a copy of the header in RAM
	delete[] m_pHeader;
}


//-----------------------------------------------------------------------------
// Purpose: Init the wave data.
// Input  : *pHeaderBuffer - the RIFF fmt chunk
//			headerSize - size of that chunk
//-----------------------------------------------------------------------------
void CAudioSourceWave::Init( const char *pHeaderBuffer, int headerSize )
{
	const WAVEFORMATEX *pHeader = (const WAVEFORMATEX *)pHeaderBuffer;

	// copy the relevant header data
	m_format = pHeader->wFormatTag;
	m_bits = pHeader->wBitsPerSample;
	m_rate = pHeader->nSamplesPerSec;
	m_channels = pHeader->nChannels;

	m_sampleSize = (m_bits * m_channels) / 8;
	
	// this can never be zero -- other functions divide by this. 
	// This should never happen, but avoid crashing
	if ( m_sampleSize <= 0 )
		m_sampleSize = 1;

	// For non-standard waves (like ADPCM) store the header, it has some useful data
	if ( m_format != WAVE_FORMAT_PCM )
	{
		m_pHeader = new char[headerSize];
		memcpy( m_pHeader, pHeader, headerSize );
		if ( m_format == WAVE_FORMAT_ADPCM )
		{
			// treat ADPCM sources as a file of bytes.  They are decoded by the mixer
			m_sampleSize = 1;
		}
	}
}


int	CAudioSourceWave::SampleRate( void ) 
{ 
	return m_rate; 
}


//-----------------------------------------------------------------------------
// Purpose: Size of each sample
// Output : 
//-----------------------------------------------------------------------------
int	CAudioSourceWave::SampleSize( void ) 
{ 
	return m_sampleSize; 
}


//-----------------------------------------------------------------------------
// Purpose: Total number of samples in this source
// Output : int
//-----------------------------------------------------------------------------
int CAudioSourceWave::SampleCount( void ) 
{
	if ( m_format == WAVE_FORMAT_ADPCM )
	{
		ADPCMWAVEFORMAT *pFormat = (ADPCMWAVEFORMAT *)m_pHeader;
		int blockSize = ((pFormat->wSamplesPerBlock - 2) * pFormat->wfx.nChannels ) / 2;
		blockSize += 7 * pFormat->wfx.nChannels;

		int blockCount = m_sampleCount / blockSize;
		int blockRem = m_sampleCount % blockSize;
		
		// total samples in complete blocks
		int sampleCount = blockCount * pFormat->wSamplesPerBlock;

		// add remaining in a short block
		if ( blockRem )
		{
			sampleCount += pFormat->wSamplesPerBlock - (((blockSize - blockRem) * 2) / m_channels);
		}
		return sampleCount;
	}
	return m_sampleCount; 
}


bool CAudioSourceWave::IsVoiceSource()
{
	if ( GetSentence() )
	{
		if ( GetSentence()->GetVoiceDuck() )
			return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Do any sample conversion
//			For 8 bit PCM, convert to signed because the mixing routine assumes this
// Input  : *pData - pointer to sample data
//			sampleCount - number of samples
//-----------------------------------------------------------------------------
void CAudioSourceWave::ConvertSamples( char *pData, int sampleCount )
{
	if ( m_format == WAVE_FORMAT_PCM )
	{
		if ( m_bits == 8 )
		{
			for ( int i = 0; i < sampleCount; i++ )
			{
				for ( int j = 0; j < m_channels; j++ )
				{
					*pData = (unsigned char)((int)((unsigned)*pData) - 128);
					pData++;
				}
			}
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: Parse base chunks
// Input  : &walk - riff file to parse
//		  : chunkName - name of the chunk to parse
//-----------------------------------------------------------------------------
// UNDONE: Move parsing loop here and drop each chunk into a virtual function
//			instead of this being virtual.
void CAudioSourceWave::ParseChunk( IterateRIFF &walk, int chunkName )
{
	switch( chunkName )
	{
		case WAVE_CUE:
			{
				ParseCueChunk( walk );
			}
			break;
		case WAVE_SAMPLER:
			{
				ParseSamplerChunk( walk );
			}
			break;
		case WAVE_VALVEDATA:
			{
				ParseSentence( walk );
			}
			break;

		case WAVE_FACT:
			break;

			// unknown/don't care
		default:
			{
				ChunkError( walk.ChunkName() );
			}
			break;
	}
}

bool CAudioSourceWave::IsLooped( void ) 
{ 
	return (m_loopStart >= 0) ? true : false; 
}

bool CAudioSourceWave::IsStereoWav( void ) 
{ 
	return (m_channels == 2) ? true : false;
}

bool CAudioSourceWave::IsStreaming( void ) 
{ 
	return false; 
}


bool CAudioSourceWave::IsCached( void )
{
	return true;
}


void CAudioSourceWave::CacheTouch( void )
{
	IsCached();
}


void CAudioSourceWave::CacheLoad( void )
{
}

void CAudioSourceWave::CacheUnload( void )
{
}


int	CAudioSourceWave::ZeroCrossingBefore( int sample )
{
	return sample;
}


int	CAudioSourceWave::ZeroCrossingAfter( int sample )
{
	return sample;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &walk - 
//-----------------------------------------------------------------------------
void CAudioSourceWave::ParseSentence( IterateRIFF &walk )
{
	CUtlBuffer buf( 0, 0, true );

	buf.EnsureCapacity( walk.ChunkSize() );
	walk.ChunkRead( buf.Base() );
	buf.SeekPut( CUtlBuffer::SEEK_HEAD, walk.ChunkSize() );

	m_Sentence.InitFromDataChunk( buf.Base(), buf.TellPut() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CSentence
//-----------------------------------------------------------------------------
CSentence *CAudioSourceWave::GetSentence( void )
{
	return &m_Sentence;
}

//-----------------------------------------------------------------------------
// Purpose: Bastardized construction routine.  This is just to avoid complex
//			constructor functions so code can be shared more easily by sub-classes
// Input  : *pFormatBuffer - RIFF header
//			formatSize - header size
//			&walk - RIFF file
//-----------------------------------------------------------------------------
void CAudioSourceWave::Setup( const char *pFormatBuffer, int formatSize, IterateRIFF &walk )
{
	Init( pFormatBuffer, formatSize );

	while ( walk.ChunkAvailable() )
	{
		ParseChunk( walk, walk.ChunkName() );
		walk.ChunkNext();
	}
}


//-----------------------------------------------------------------------------
// Purpose: parses loop information from a cue chunk
// Input  : &walk - RIFF iterator
// Output : int loop start position
//-----------------------------------------------------------------------------
void CAudioSourceWave::ParseCueChunk( IterateRIFF &walk )
{
	// Cue chunk as specified by RIFF format
	// see $/research/jay/sound/riffnew.htm
	struct 
	{
		unsigned int dwName; 
		unsigned int dwPosition;
		unsigned int fccChunk;
		unsigned int dwChunkStart;
		unsigned int dwBlockStart; 
		unsigned int dwSampleOffset;
	} cue_chunk;

	int cueCount;

	// assume that the cue chunk stored in the wave is the start of the loop
	// assume only one cue chunk, UNDONE: Test this assumption here?
	cueCount = walk.ChunkReadInt();

	walk.ChunkReadPartial( &cue_chunk, sizeof(cue_chunk) );
	m_loopStart = cue_chunk.dwSampleOffset;
}

//-----------------------------------------------------------------------------
// Purpose: parses loop information from a 'smpl' chunk
// Input  : &walk - RIFF iterator
// Output : int loop start position
//-----------------------------------------------------------------------------
void CAudioSourceWave::ParseSamplerChunk( IterateRIFF &walk )
{
	// Sampler chunk for MIDI instruments
	// Parse loop info from this chunk too
	struct SampleLoop
	{
		unsigned int	dwIdentifier;
		unsigned int	dwType;
		unsigned int	dwStart;
		unsigned int	dwEnd;
		unsigned int	dwFraction;
		unsigned int	dwPlayCount;
	};

	struct 
	{
		unsigned int	dwManufacturer;
		unsigned int	dwProduct;
		unsigned int	dwSamplePeriod;
		unsigned int	dwMIDIUnityNote;
		unsigned int	dwMIDIPitchFraction;
		unsigned int	dwSMPTEFormat;
		unsigned int	dwSMPTEOffset;
		unsigned int	cSampleLoops;
		unsigned int	cbSamplerData;
		struct SampleLoop Loops[1];
	} samplerChunk;

	// assume that the loop end is the sample end
	// assume that only the first loop is relevant

	walk.ChunkReadPartial( &samplerChunk, sizeof(samplerChunk) );

	// only support normal forward loops
	if ( samplerChunk.Loops[0].dwType == 0 )
	{
		Assert( samplerChunk.cSampleLoops > 0 );
		m_loopStart = samplerChunk.Loops[0].dwStart;
	}
#if _DEBUG
	else
	{
		Msg("Unknown sampler chunk type %d\n", samplerChunk.Loops[0].dwType );
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: get the wave header
//-----------------------------------------------------------------------------
void *CAudioSourceWave::GetHeader( void )
{
	return m_pHeader;
}


//-----------------------------------------------------------------------------
// Purpose: wrap the position wrt looping
// Input  : samplePosition - absolute position
// Output : int - looped position
//-----------------------------------------------------------------------------
int CAudioSourceWave::ConvertLoopedPosition( int samplePosition )
{
	// if the wave is looping and we're past the end of the sample
	// convert to a position within the loop
	// At the end of the loop, we return a short buffer, and subsequent call
	// will loop back and get the rest of the buffer
	if ( m_loopStart >= 0 && samplePosition >= m_sampleCount )
	{
		// size of loop
		int loopSize = m_sampleCount - m_loopStart;
		// subtract off starting bit of the wave
		samplePosition -= m_loopStart;
		
		if ( loopSize )
		{
			// "real" position in memory (mod off extra loops)
			samplePosition = m_loopStart + (samplePosition % loopSize);
		}
		// ERROR? if no loopSize
	}

	return samplePosition;
}

//-----------------------------------------------------------------------------
// Purpose: remove the reference for the mixer getting deleted
// Input  : *pMixer - 
//-----------------------------------------------------------------------------
void CAudioSourceWave::ReferenceRemove( CAudioMixer *pMixer )
{
	m_refCount--;
}


//-----------------------------------------------------------------------------
// Purpose: Add a mixer reference
// Input  : *pMixer - 
//-----------------------------------------------------------------------------
void CAudioSourceWave::ReferenceAdd( CAudioMixer *pMixer )
{
	m_refCount++;
}


//-----------------------------------------------------------------------------
// Purpose: return true if no mixers reference this source
//-----------------------------------------------------------------------------
bool CAudioSourceWave::CanDelete( void )
{
	if ( m_refCount > 0 )
		return false;

	return true;
}


// CAudioSourceMemWave is a bunch of wave data that is all in memory.
// To use it:
// - derive from CAudioSourceMemWave
// - call CAudioSourceWave::Init with a WAVEFORMATEX
// - set m_sampleCount.
// - implement GetDataPointer
class CAudioSourceMemWave : public CAudioSourceWave
{
public:
							CAudioSourceMemWave();
							CAudioSourceMemWave( char const* pName );

	// These are all implemented by CAudioSourceMemWave.
	virtual CAudioMixer*	CreateMixer( void );
	virtual int				GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] );
	virtual int				ZeroCrossingBefore( int sample );
	virtual int				ZeroCrossingAfter( int sample );

protected:								   
	
	// Whoeover derives must implement this.
	virtual char	*GetDataPointer( void )=0;

private:
	CAudioSourceMemWave( const CAudioSourceMemWave & ); // not implemented, not accessible
};


CAudioSourceMemWave::CAudioSourceMemWave() :
	CAudioSourceWave("")
{
}

CAudioSourceMemWave::CAudioSourceMemWave( char const* pName ) : 
	CAudioSourceWave(pName)
{
}

//-----------------------------------------------------------------------------
// Purpose: Creates a mixer and initializes it with an appropriate mixer
//-----------------------------------------------------------------------------
CAudioMixer *CAudioSourceMemWave::CreateMixer( void )
{
	CAudioMixer *pMixer = CreateWaveMixer( CreateWaveDataMemory(*this), m_format, m_channels, m_bits );
	ReferenceAdd( pMixer );

	return pMixer;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : **pData - output pointer to samples
//			samplePosition - position (in samples not bytes) 
//			sampleCount - number of samples (not bytes)
// Output : int - number of samples available
//-----------------------------------------------------------------------------
int CAudioSourceMemWave::GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
{
	// handle position looping
	samplePosition = ConvertLoopedPosition( samplePosition );

	// how many samples are available (linearly not counting looping)
	int totalSampleCount = m_sampleCount - samplePosition;

	// may be asking for a sample out of range, clip at zero
	if ( totalSampleCount < 0 )
		totalSampleCount = 0;

	// clip max output samples to max available
	if ( sampleCount > totalSampleCount )
		sampleCount = totalSampleCount;

	// byte offset in sample database
	samplePosition *= m_sampleSize;

	// if we are returning some samples, store the pointer
	if ( sampleCount )
	{
		*pData = GetDataPointer() + samplePosition;
		Assert( *pData );
	}

	return sampleCount;
}


// Hardcoded macros to test for zero crossing
#define ZERO_X_8(b)		((b)<2 && (b)>-2)
#define ZERO_X_16(b)	((b)<512 && (b)>-512)

//-----------------------------------------------------------------------------
// Purpose: Search backward for a zero crossing starting at sample
// Input  : sample - starting point
// Output : position of zero crossing
//-----------------------------------------------------------------------------
int	CAudioSourceMemWave::ZeroCrossingBefore( int sample )
{
	char *pWaveData = GetDataPointer();

	if ( m_format == WAVE_FORMAT_PCM )
	{
		if ( m_bits == 8 )
		{
			char *pData = pWaveData + sample * m_sampleSize;
			bool zero = false;

			if ( m_channels == 1 )
			{
				while ( sample > 0 && !zero )
				{
					if ( ZERO_X_8(*pData) )
						zero = true;
					else
					{
						sample--;
						pData--;
					}
				}
			}
			else
			{
				while ( sample > 0 && !zero )
				{
					if ( ZERO_X_8(*pData) && ZERO_X_8(pData[1]) )
						zero = true;
					else
					{
						sample--;
						pData--;
					}
				}
			}
		}
		else
		{
			short *pData = (short *)(pWaveData + sample * m_sampleSize);
			bool zero = false;

			if ( m_channels == 1 )
			{
				while ( sample > 0 && !zero )
				{
					if ( ZERO_X_16(*pData) )
						zero = true;
					else
					{
						pData--;
						sample--;
					}
				}
			}
			else
			{
				while ( sample > 0 && !zero )
				{
					if ( ZERO_X_16(*pData) && ZERO_X_16(pData[1]) )
						zero = true;
					else
					{
						sample--;
						pData--;
					}
				}
			}
		}
	}
	return sample;
}


//-----------------------------------------------------------------------------
// Purpose: Search forward for a zero crossing
// Input  : sample - starting point
// Output : position of found zero crossing
//-----------------------------------------------------------------------------
int	CAudioSourceMemWave::ZeroCrossingAfter( int sample )
{
	char *pWaveData = GetDataPointer();
	if ( m_format == WAVE_FORMAT_PCM )
	{
		if ( m_bits == 8 )
		{
			char *pData = pWaveData + sample * m_sampleSize;
			bool zero = false;

			if ( m_channels == 1 )
			{
				while ( sample < SampleCount() && !zero )
				{
					if ( ZERO_X_8(*pData) )
						zero = true;
					else
					{
						sample++;
						pData++;
					}
				}
			}
			else
			{
				while ( sample < SampleCount() && !zero )
				{
					if ( ZERO_X_8(*pData) && ZERO_X_8(pData[1]) )
						zero = true;
					else
					{
						sample++;
						pData++;
					}
				}
			}
		}
		else
		{
			short *pData = (short *)(pWaveData + sample * m_sampleSize);
			bool zero = false;

			if ( m_channels == 1 )
			{
				while ( sample > 0 && !zero )
				{
					if ( ZERO_X_16(*pData) )
						zero = true;
					else
					{
						pData++;
						sample++;
					}
				}
			}
			else
			{
				while ( sample > 0 && !zero )
				{
					if ( ZERO_X_16(*pData) && ZERO_X_16(pData[1]) )
						zero = true;
					else
					{
						sample++;
						pData++;
					}
				}
			}
		}
	}
	return sample;
}



// This is a CAudioSourceMemWave and gets all of its data from the cache.
class CAudioSourceMemWaveCache : public CAudioSourceMemWave
{
public:
	CAudioSourceMemWaveCache( const char *pName );
	~CAudioSourceMemWaveCache( void );

	virtual void			ParseChunk( IterateRIFF &walk, int chunkName );
	void					ParseDataChunk( IterateRIFF &walk );

	bool					IsCached( void );
	void					CacheTouch( void );
	void					CacheLoad( void );
	void					CacheUnload( void );

protected:
	virtual char			*GetDataPointer( void );

	cache_user_t	m_cache;

private:
	CAudioSourceMemWaveCache( const CAudioSourceMemWaveCache & );
};


//-----------------------------------------------------------------------------
// Purpose: NULL the wave data pointer (we haven't loaded yet)
//-----------------------------------------------------------------------------
CAudioSourceMemWaveCache::CAudioSourceMemWaveCache( const char *pName ) : 
	CAudioSourceMemWave( pName )
{
	memset( &m_cache, 0, sizeof(m_cache) );
}


//-----------------------------------------------------------------------------
// Purpose: Free any wave data we've allocated
//-----------------------------------------------------------------------------
CAudioSourceMemWaveCache::~CAudioSourceMemWaveCache( void )
{
	CacheUnload();
}


//-----------------------------------------------------------------------------
// Purpose: parse chunks with unique processing to in-memory waves
// Input  : &walk - RIFF file
//-----------------------------------------------------------------------------
void CAudioSourceMemWaveCache::ParseChunk( IterateRIFF &walk, int chunkName )
{
	switch( chunkName )
	{
		// this is the audio data
	case WAVE_DATA:
		{
			ParseDataChunk( walk );
		}
		return;
	}

	CAudioSourceWave::ParseChunk( walk, chunkName );
}


//-----------------------------------------------------------------------------
// Purpose: reads the actual sample data and parses it
// Input  : &walk - RIFF file
//-----------------------------------------------------------------------------
void CAudioSourceMemWaveCache::ParseDataChunk( IterateRIFF &walk )
{
	int size = walk.ChunkSize();
	
	// create a buffer for the samples
	char *pData = (char *)Cache_Alloc( &m_cache, size, m_pName );

	// load them into memory
	walk.ChunkRead( pData );

	if ( m_format == WAVE_FORMAT_PCM )
	{
		// number of samples loaded
		m_sampleCount = size / m_sampleSize;

		// some samples need to be converted
		ConvertSamples( pData, m_sampleCount );
	}
	else if ( m_format == WAVE_FORMAT_ADPCM )
	{
		// The ADPCM mixers treat the wave source as a flat file of bytes.
		m_sampleSize = 1;
		// Since each "sample" is a byte (this is a flat file), the number of samples is the file size
		m_sampleCount = size;

		// file says 4, output is 16
		m_bits = 16;
	}
}



bool CAudioSourceMemWaveCache::IsCached( void )
{
	if ( m_cache.data )
	{
		if ( Cache_Check( &m_cache ) != NULL )
			return true;
	}

	return false;
}


void CAudioSourceMemWaveCache::CacheTouch( void )
{
	IsCached();
}


void CAudioSourceMemWaveCache::CacheLoad( void )
{
	if ( IsCached() )
		return;

	InFileRIFF riff( m_pName, *g_pSndIO );

	// UNDONE: Don't use printf to handle errors
	if ( riff.RIFFName() != RIFF_WAVE )
	{
		return;
	}

	// set up the iterator for the whole file (root RIFF is a chunk)
	IterateRIFF walk( riff, riff.RIFFSize() );
	while ( walk.ChunkAvailable() )
	{
		switch( walk.ChunkName() )
		{
		case WAVE_DATA:
			ParseDataChunk( walk );
			return;
		}
		walk.ChunkNext();
	}
}

void CAudioSourceMemWaveCache::CacheUnload( void )
{
	if ( !m_cache.data )
		return;

	Cache_Free( &m_cache );
}

char *CAudioSourceMemWaveCache::GetDataPointer( void )
{
	char *pData = (char *)Cache_Check( &m_cache );
	if ( !pData )
		CacheLoad();
	return (char *)Cache_Check( &m_cache );
}

//-----------------------------------------------------------------------------
// Purpose: Wave source for streaming wave files
// UNDONE: Handle looping
//-----------------------------------------------------------------------------
class CAudioSourceStreamWave : public CAudioSourceWave, public IWaveStreamSource
{
public:
	CAudioSourceStreamWave( const char * );
	~CAudioSourceStreamWave();

	CAudioMixer		*CreateMixer( void );
	int				GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] );
	void			ParseChunk( IterateRIFF &walk, int chunkName );
	bool			IsStreaming( void ) { return true; }

	// IWaveStreamSource
	virtual int UpdateLoopingSamplePosition( int samplePosition )
	{
		return ConvertLoopedPosition( samplePosition );
	}
	virtual void UpdateSamples( char *pData, int sampleCount )
	{
		ConvertSamples( pData, sampleCount );
	}

private:
	CAudioSourceStreamWave( const CAudioSourceStreamWave & ); // not implemented, not accessible

	int				m_dataStart;	// offset of wave data chunk
	int				m_dataSize;		// size of wave data chunk

};

//-----------------------------------------------------------------------------
// Purpose: Save a copy of the file name for instances to open later
// Input  : *pFileName - filename
//-----------------------------------------------------------------------------
CAudioSourceStreamWave::CAudioSourceStreamWave( const char *pFileName ) : CAudioSourceWave( pFileName )
{
	m_pName = pFileName;
	m_dataStart = -1;
	m_dataSize = 0;
	m_sampleCount = 0;
}



//-----------------------------------------------------------------------------
// Purpose: free the filename buffer
//-----------------------------------------------------------------------------
CAudioSourceStreamWave::~CAudioSourceStreamWave( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Create an instance (mixer & wavedata) of this sound
// Output : CAudioMixer * - pointer to the mixer
//-----------------------------------------------------------------------------
CAudioMixer *CAudioSourceStreamWave::CreateMixer( void )
{
	// BUGBUG: Source constructs the IWaveData, mixer frees it, fix this?
	IWaveData *pWaveData = CreateWaveDataStream(*this, static_cast<IWaveStreamSource *>(this), *g_pSndIO, m_pName, m_dataStart, m_dataSize);
	if ( pWaveData )
	{
		CAudioMixer *pMixer = CreateWaveMixer( pWaveData, m_format, m_channels, m_bits );
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


//-----------------------------------------------------------------------------
// Purpose: Parse a stream wave file chunk
//			unlike the in-memory file, don't load the data, just get a reference to it.
// Input  : &walk - RIFF file
//-----------------------------------------------------------------------------
void CAudioSourceStreamWave::ParseChunk( IterateRIFF &walk, int chunkName )
{
	// NOTE: It would be nice to break out of parsing once we have the data start and
	//		save seeking over the whole file.  But to do so, we'd have to make
	//		sure that the CUE chunks occur before the data chunks.  In the first
	//		test file I used, this was not the case.
	switch( chunkName )
	{
	case WAVE_DATA:
		{
			// data starts at chunk + 8 (chunk name, chunk size = 8 bytes)
			m_dataStart = walk.ChunkFilePosition() + 8;
			m_dataSize = walk.ChunkSize();
			m_sampleCount = m_dataSize / m_sampleSize;
			// don't load the data, just know where it is so each instance 
			// can load it later
		}
		return;
	}
	CAudioSourceWave::ParseChunk( walk, chunkName );
}

//-----------------------------------------------------------------------------
// Purpose: This is not implemented here.  This source has no data.  It is the
//			WaveData's responsibility to load/serve the data
//-----------------------------------------------------------------------------
int CAudioSourceStreamWave::GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
{
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Create a wave audio source (streaming or in memory)
// Input  : *pName - file name (NOTE: CAUDIOSOURCE KEEPS A POINTER TO pName)
//			streaming - if true, don't load, stream each instance
// Output : CAudioSource * - a new source
//-----------------------------------------------------------------------------
CAudioSource *CreateWave( const char *pName, bool streaming )
{
	char formatBuffer[1024];
	InFileRIFF riff( pName, *g_pSndIO );

	// UNDONE: Don't use printf to handle errors
	if ( riff.RIFFName() != RIFF_WAVE )
	{
		static CUtlSymbolTable wavErrors;

		CUtlSymbol sym;
		sym = wavErrors.Find( pName );
		if ( UTL_INVAL_SYMBOL == sym )
		{
			// See if file exists
			if ( g_pFileSystem->FileExists( pName ) )
			{
				Warning("Bad RIFF file '%s'\n", pName );
			}
			else
			{
				Warning("Missing wav file '%s'\n", pName );
			}
			wavErrors.AddString( pName );
		}
		return NULL;
	}

	// set up the iterator for the whole file (root RIFF is a chunk)
	IterateRIFF walk( riff, riff.RIFFSize() );

	int format = 0;
	int formatSize = 0;

	// This chunk must be first as it contains the wave's format
	// break out when we've parsed it
	while ( walk.ChunkAvailable() && format == 0 )
	{
		switch( walk.ChunkName() )
		{
		case WAVE_FMT:
			{
				if ( walk.ChunkSize() <= 1024 )
				{
					walk.ChunkRead( formatBuffer );
					formatSize = walk.ChunkSize();
					format = ((WAVEFORMATEX *)formatBuffer)->wFormatTag;
				}
			}
			break;
		default:
			{
				ChunkError( walk.ChunkName() );
			}
			break;
		}
		walk.ChunkNext();
	}

	// Not really a WAVE file or no format chunk, bail
	if ( !format )
		return NULL;

	CAudioSourceWave *pWave;

	// create the source from this file
	if ( streaming )
		pWave = new CAudioSourceStreamWave( pName );
	else
		pWave = new CAudioSourceMemWaveCache( pName );

	// init the wave source
	pWave->Setup( formatBuffer, formatSize, walk );

	return pWave;
}


//-----------------------------------------------------------------------------
// Purpose: Wrapper for CreateWave()
//-----------------------------------------------------------------------------
CAudioSource *Audio_CreateStreamedWave( const char *pName )
{
	if ( Audio_IsMP3( pName ) )
	{
		return Audio_CreateStreamedMP3( pName );
	}
	return CreateWave( pName, true );
}


//-----------------------------------------------------------------------------
// Purpose: Wrapper for CreateWave()
//-----------------------------------------------------------------------------
CAudioSource *Audio_CreateMemoryWave( const char *pName )
{
	if ( Audio_IsMP3( pName ) )
	{
		return Audio_CreateMemoryMP3( pName );
	}
	return CreateWave( pName, false );
}

