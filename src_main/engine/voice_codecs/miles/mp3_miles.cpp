//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "interface.h"
#include "milesbase.h"
#include "vaudio/ivaudio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static S32 AILCALLBACK AudioStreamEventCB( U32 user, void FAR *dest, S32 bytes_requested, S32 offset )
{
	IAudioStreamEvent *pThis = static_cast<IAudioStreamEvent*>( (void *)user);
	return pThis->StreamRequestData( dest, bytes_requested, offset );
}

class CMilesMP3 : public IAudioStream
{
public:
	CMilesMP3( IAudioStreamEvent *pHandler );
	~CMilesMP3();
	
	// IAudioStream functions
	virtual int	Decode( void *pBuffer, unsigned int bufferSize );
	virtual int GetOutputBits();
	virtual int GetOutputRate();
	virtual int GetOutputChannels();

private:
	ASISTRUCT		m_decoder;
};

CMilesMP3::CMilesMP3( IAudioStreamEvent *pHandler )
{
	m_decoder.Init( pHandler, ".MP3", ".RAW", &AudioStreamEventCB );
}


CMilesMP3::~CMilesMP3()
{
	m_decoder.Shutdown();
}
	

// IAudioStream functions
int	CMilesMP3::Decode( void *pBuffer, unsigned int bufferSize )
{
	return m_decoder.Process( pBuffer, bufferSize );
}


int CMilesMP3::GetOutputBits()
{
	return m_decoder.GetAttribute( m_decoder.OUTPUT_BITS );
}


int CMilesMP3::GetOutputRate()
{
	return m_decoder.GetAttribute( m_decoder.OUTPUT_RATE );
}


int CMilesMP3::GetOutputChannels()
{
	return m_decoder.GetAttribute( m_decoder.OUTPUT_CHANNELS );
}




class CVAudio : public IVAudio
{
public:
	CVAudio()
	{
		// Assume the user will be creating multiple miles objects, so
		// keep miles running while this exists
		IncrementRefMiles();
	}

	~CVAudio()
	{
		DecrementRefMiles();
	}

	IAudioStream *CreateMP3StreamDecoder( IAudioStreamEvent *pEventHandler )
	{
		return new CMilesMP3( pEventHandler );
	}

	void DestroyMP3StreamDecoder( IAudioStream *pDecoder )
	{
		delete pDecoder;
	}
};

EXPOSE_INTERFACE( CVAudio, IVAudio, VAUDIO_INTERFACE_VERSION );

