//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef SOUNDSCAPE_H
#define SOUNDSCAPE_H
#ifdef _WIN32
#pragma once
#endif


class CEnvSoundscape : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvSoundscape, CPointEntity );

	CEnvSoundscape();

	bool KeyValue( const char *szKeyName, const char *szValue );
	void Spawn( void );
	void Precache( void );
	void Think( void );
	void WriteAudioParamsTo( audioparams_t &audio );
	bool InRangeOfPlayer( CBasePlayer *pPlayer );
	void DrawDebugGeometryOverlays( void );

	DECLARE_DATADESC();

	float	m_flRadius;
	float	m_flRange;
	string_t m_soundscapeName;
	int		m_soundscapeIndex;
	string_t m_positionNames[4];	
};


#endif // SOUNDSCAPE_H
