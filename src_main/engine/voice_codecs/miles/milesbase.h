//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MILESBASE_H
#define MILESBASE_H
#ifdef _WIN32
#pragma once
#endif

// windows.h / platform.h conflicts
#pragma warning(disable:4005)
#include "Miles/mss.h"


class CProvider
{
public:					   
				CProvider( HPROVIDER hProvider );

	static CProvider*	FindProvider( HPROVIDER hProvider );
	static void			FreeAllProviders();

	HPROVIDER	GetProviderHandle();
	
private:
	
				~CProvider();

private:

	HPROVIDER	m_hProvider;

	static CProvider	*s_pHead;
	CProvider			*m_pNext;
};


// This holds the handles and function pointers we want from a compressor/decompressor.
class ASISTRUCT
{
public:
				ASISTRUCT();
				~ASISTRUCT();

	bool		Init( void *pCallbackObject, const char *pInputFileType, 
						const char *pOutputFileType, AILASIFETCHCB cb );
	void		Shutdown();
	int			Process( void *pBuffer, unsigned int bufferSize );
	bool		IsActive() const;
	unsigned int GetAttribute( HATTRIB attribute );
	void		SetAttribute( HATTRIB attribute, unsigned int value );

public:
	HATTRIB			OUTPUT_BITS;
	HATTRIB			OUTPUT_CHANNELS;
	HATTRIB			OUTPUT_RATE;

	HATTRIB			INPUT_BITS;
	HATTRIB			INPUT_CHANNELS;
	HATTRIB			INPUT_RATE;
	HATTRIB			INPUT_BLOCK_SIZE;

private:
	void		Clear();

private:
	ASI_STREAM_OPEN				ASI_stream_open;
	ASI_STREAM_PROCESS			ASI_stream_process;
	ASI_STREAM_CLOSE			ASI_stream_close;
	ASI_STREAM_SET_PREFERENCE	ASI_stream_set_preference;
	ASI_STREAM_ATTRIBUTE		ASI_stream_attribute;

	HASISTREAM					m_stream;
	CProvider					*m_pProvider;
};


extern void IncrementRefMiles();
extern void DecrementRefMiles();

#endif // MILESBASE_H
