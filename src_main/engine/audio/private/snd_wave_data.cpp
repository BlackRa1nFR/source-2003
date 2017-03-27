#include "snd_wave_data.h"
#include "riff.h"
#include "tier0/platform.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Iterator for wave data (this is to abstract streaming/buffering)
//-----------------------------------------------------------------------------
class CWaveDataMemory : public IWaveData
{
public:
	CWaveDataMemory( CAudioSource &source ) : m_source(source) {}
	~CWaveDataMemory( void ) {}
	CAudioSource &Source( void ) { return m_source; }
	
	// this file is in memory, simply pass along the data request to the source
	virtual int ReadSourceData( void **pData, int sampleIndex, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
	{
		return m_source.GetOutputData( pData, sampleIndex, sampleCount, copyBuf );
	}
private:
	CAudioSource		&m_source;	// pointer to source
};


// UNDONE: Allocate this in cache instead?
#define BUFFER_SIZE 16384
//-----------------------------------------------------------------------------
// Purpose: This is an instance of a stream.
//			This contains the file handle and streaming buffer
//			The mixer doesn't know the file is streaming.  The IWaveData
//			abstracts the data access.  The mixer abstracts data encoding/format
//-----------------------------------------------------------------------------
class CWaveDataStream : public IWaveData
{
public:
	CWaveDataStream( CAudioSource &source, IWaveStreamSource *pStreamSource, IFileReadBinary &io, const char *pFileName, int fileStart, int fileSize );
	~CWaveDataStream( void );

	// return the source pointer (mixer needs this to determine some things like sampling rate)
	CAudioSource &Source( void ) { return m_source; }

	// Read data from the source - this is the primary function of a IWaveData subclass
	// Get the data from the buffer (or reload from disk)
	virtual int ReadSourceData( void **pData, int sampleIndex, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] );

private:
	CWaveDataStream( const CWaveDataStream & );

	CAudioSource			&m_source;					// wave source
	IWaveStreamSource		*m_pStreamSource;			// streaming
	int						m_sampleSize;				// size of a sample in bytes
	int						m_bufferSize;				// size of buffer in samples
	char					m_buffer[BUFFER_SIZE];		// buffer memory
	int						m_sampleIndex;				// sample index of first sample in buffer
	int						m_bufferCount;				// current count of samples in the buffer
	int						m_waveSize;					// total number of samples in the file
	int						m_file;						// file handle (file is open)
	int						m_dataStart;
	// UNDONE: Do we need this?  Just use the global?
	IFileReadBinary			&m_io;						// I/O interface
};


CWaveDataStream::CWaveDataStream( CAudioSource &source, IWaveStreamSource *pStreamSource, IFileReadBinary &io, const char *pFileName, int fileStart, int fileSize ) 
		: m_source(source), m_dataStart(fileStart), m_io(io), m_pStreamSource(pStreamSource)
{
	// nothing in the buffer yet
	m_bufferCount = 0;
	m_sampleIndex = 0;

	m_file = m_io.open( pFileName );

	if ( m_file )
	{
		// position at start of samples
		m_io.seek( m_file, m_dataStart );
		// need to know the size of a sample
		m_sampleSize = source.SampleSize();
		// This is the size in samples of the buffer
		m_bufferSize = BUFFER_SIZE / m_sampleSize;
		// This is the size in samples (not bytes) of the wave itself
		m_waveSize = fileSize / m_sampleSize;

		// UNDONE: Read a buffer here?
	}
}


// close the file
CWaveDataStream::~CWaveDataStream( void ) 
{
	m_io.close( m_file );
}


// Read data from the source - this is the primary function of a IWaveData subclass
// Get the data from the buffer (or reload from disk)
int CWaveDataStream::ReadSourceData( void **pData, int sampleIndex, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
{
	// wrap position if looping
	if ( m_source.IsLooped() )
	{
		sampleIndex = m_pStreamSource->UpdateLoopingSamplePosition( sampleIndex );

		if ( sampleIndex < m_sampleIndex )
		{
			m_sampleIndex = sampleIndex;
			m_bufferCount = 0;
			m_io.seek( m_file, m_dataStart + sampleIndex );
		}
	}

	// UNDONE: This is an error!!
	// The mixer playing back the stream tried to go backwards!?!?!
	// BUGBUG: Just play the beginning of the buffer until we get to a valid linear position
	if ( sampleIndex < m_sampleIndex )
		sampleIndex = m_sampleIndex;

	// calc sample position relative to the current buffer
	// m_sampleIndex is the sample position of the first byte of the buffer
	sampleIndex -= m_sampleIndex;
	
	// out of range? refresh buffer
	if ( sampleIndex >= m_bufferCount )
	{
		// advance one buffer (the file is positioned here)
		m_sampleIndex += m_bufferCount;
		// next sample to load
		sampleIndex -= m_bufferCount;

		// if the remainder is greated than one buffer size, seek over it.  Otherwise, read the next chunk
		// and leave the remainder as an offset.

		// number of buffers to "skip" (as in the case where we are starting a streaming sound not at the beginning)
		int skips = sampleIndex / m_bufferSize;
		
		// If we are skipping over a buffer, do it with a seek instead of a read.
		if ( skips )
		{
			// skip directly to next position
			m_sampleIndex += sampleIndex;
			sampleIndex = 0;

			// move the file to the new position
			m_io.seek( m_file, m_dataStart + (m_sampleIndex * m_sampleSize) );
		}

		// This is the maximum number of samples we could read from the file
		m_bufferCount = m_waveSize - m_sampleIndex;
		
		// past the end of the file?  stop the wave.
		if ( m_bufferCount <= 0 )
			return 0;

		// clamp available samples to buffer size
		if ( m_bufferCount > m_bufferSize )
			m_bufferCount = m_bufferSize;

		// read in the max bufferable, available samples
		m_io.read( m_buffer, m_bufferCount * m_sampleSize, m_file );

		// do any conversion the source needs (mixer will decode/decompress)
		m_pStreamSource->UpdateSamples( m_buffer, m_bufferCount );
	}

	// If we have some samples in the buffer that are within range of the request
	if ( sampleIndex < m_bufferCount )
	{
		// Get the desired starting sample
		*pData = (void *)&m_buffer[sampleIndex * m_sampleSize];
		// max available
		int available = m_bufferCount - sampleIndex;
		// clamp available to max requested
		if ( available > sampleCount )
			available = sampleCount;

		return available;
	}
	return 0;
}


IWaveData *CreateWaveDataStream( CAudioSource &source, IWaveStreamSource *pStreamSource, IFileReadBinary &io, const char *pFileName, int dataOffset, int dataSize )
{
	return new CWaveDataStream( source, pStreamSource, io, pFileName, dataOffset, dataSize );
}

IWaveData *CreateWaveDataMemory( CAudioSource &source )
{
	return new CWaveDataMemory( source );
}
