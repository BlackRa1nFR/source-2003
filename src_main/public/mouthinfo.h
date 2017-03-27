//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( MOUTHINFO_H )
#define MOUTHINFO_H
#ifdef _WIN32
#pragma once
#endif

class CAudioSource;

class CVoiceData
{
public:
	CVoiceData( void )
	{
		m_flVoiceStartTime = 0.0f;
		m_pAudioSource = NULL;
	}

	// voice data for players
	float					m_flVoiceStartTime;
	CAudioSource 			*m_pAudioSource;
};

#define UNKNOWN_VOICE_SOURCE -1

//-----------------------------------------------------------------------------
// Purpose: Describes position of mouth for lip syncing
//-----------------------------------------------------------------------------
class CMouthInfo
{
public:
	// 0 = mouth closed, 255 = mouth agape
	byte					mouthopen;		
	// counter for running average
	byte					sndcount;		
	// running average
	int						sndavg;			

public:
							CMouthInfo( void ) { m_nVoiceSources = 0; }
	virtual					~CMouthInfo( void ) { ClearVoiceSources(); }

	int						GetNumVoiceSources( void );
	CVoiceData				*GetVoiceSource( int number );

	void					ClearVoiceSources( void );
	int						GetIndexForSource( CAudioSource *source );
	bool					IsSourceReferenced( CAudioSource *source );

	CVoiceData				*AddSource( CAudioSource *source, float start = 0.0f );

	void					RemoveSource( CAudioSource *source );
	void					RemoveSourceByIndex( int index );

	bool					IsActive( void );

private:
	enum
	{
		MAX_VOICE_DATA = 4
	};

	CVoiceData				m_VoiceSources[ MAX_VOICE_DATA ];
	int						m_nVoiceSources;
};

inline bool CMouthInfo::IsActive( void )
{
	return ( GetNumVoiceSources() > 0 ) ? true : false;
}

inline int CMouthInfo::GetNumVoiceSources( void )
{
	return m_nVoiceSources;
}

inline CVoiceData *CMouthInfo::GetVoiceSource( int number )
{
	if ( number < 0 || number >= m_nVoiceSources )
		return NULL;

	return &m_VoiceSources[ number ];
}

inline void CMouthInfo::ClearVoiceSources( void )
{
	m_nVoiceSources = 0;
}

inline int CMouthInfo::GetIndexForSource( CAudioSource *source )
{
	for ( int i = 0; i < m_nVoiceSources; i++ )
	{
		CVoiceData *v = &m_VoiceSources[ i ];
		if ( !v )
			continue;

		if ( v->m_pAudioSource == source )
			return i;
	}

	return UNKNOWN_VOICE_SOURCE;
}

inline bool CMouthInfo::IsSourceReferenced( CAudioSource *source )
{
	if ( GetIndexForSource( source ) != UNKNOWN_VOICE_SOURCE )
		return true;

	return false;
}

inline void CMouthInfo::RemoveSource( CAudioSource *source )
{
	int idx = GetIndexForSource( source );
	if ( idx == UNKNOWN_VOICE_SOURCE )
		return;

	RemoveSourceByIndex( idx );
}

inline void CMouthInfo::RemoveSourceByIndex( int index )
{
	if ( index < 0 || index >= m_nVoiceSources )
		return;

	m_VoiceSources[ index ] = m_VoiceSources[ --m_nVoiceSources ];
}

inline CVoiceData *CMouthInfo::AddSource( CAudioSource *source, float start /*= 0.0f*/ )
{
	int idx = GetIndexForSource( source );
	if ( idx == UNKNOWN_VOICE_SOURCE )
	{
		if ( m_nVoiceSources < MAX_VOICE_DATA )
		{
			idx = m_nVoiceSources++;
		}
		else
		{
			// No room!
			return NULL;
		}
	}

	CVoiceData *data = &m_VoiceSources[ idx ];
	data->m_pAudioSource = source;
	data->m_flVoiceStartTime = start;
	return data;
}

#endif // MOUTHINFO_H