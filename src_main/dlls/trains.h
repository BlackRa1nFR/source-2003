//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TRAINS_H
#define TRAINS_H
#ifdef _WIN32
#pragma once
#endif


#include "entityoutput.h"


// Tracktrain spawn flags
#define SF_TRACKTRAIN_NOPITCH				0x0001
#define SF_TRACKTRAIN_NOCONTROL				0x0002
#define SF_TRACKTRAIN_FORWARDONLY			0x0004
#define SF_TRACKTRAIN_PASSABLE				0x0008
#define SF_TRACKTRAIN_FIXED_ORIENTATION		0x0010
#define SF_TRACKTRAIN_HL1TRAIN				0x0080

// Spawnflag for CPathTrack
#define SF_PATH_DISABLED		0x00000001
//#define SF_PATH_FIREONCE		0x00000002
#define SF_PATH_ALTREVERSE		0x00000004
#define SF_PATH_DISABLE_TRAIN	0x00000008
#define SF_PATH_TELEPORT		0x00000010
#define SF_PATH_ALTERNATE		0x00008000

// Spawnflags of CPathCorner
#define SF_CORNER_WAITFORTRIG	0x001
#define SF_CORNER_TELEPORT		0x002

#define TRAIN_ACTIVE	0x80 
#define TRAIN_NEW		0xc0
#define TRAIN_OFF		0x00
#define TRAIN_NEUTRAL	0x01
#define TRAIN_SLOW		0x02
#define TRAIN_MEDIUM	0x03
#define TRAIN_FAST		0x04 
#define TRAIN_BACK		0x05

class CPathTrack : public CPointEntity
{
	DECLARE_CLASS( CPathTrack, CPointEntity );
public:
	void		Spawn( void );
	void		Activate( void );
	
	void		SetPrevious( CPathTrack *pprevious );
	void		Link( void );
	
	void		ToggleAlternatePath( void );
	void		EnableAlternatePath( void );
	void		DisableAlternatePath( void );

	void		TogglePath( void );
	void		EnablePath( void );
	void		DisablePath( void );

	static CPathTrack	*ValidPath( CPathTrack *ppath, int testFlag = true );		// Returns ppath if enabled, NULL otherwise

	void		Project( CPathTrack *pstart, CPathTrack *pend, Vector &origin, float dist );

	static CPathTrack *Instance( edict_t *pent );

	CPathTrack	*LookAhead( Vector &origin, float dist, int move );
	CPathTrack	*Nearest( Vector origin );

	CPathTrack	*GetNext( void );
	CPathTrack	*GetPrevious( void );
	
	void InputPass( inputdata_t &inputdata );
	
	void InputToggleAlternatePath( inputdata_t &inputdata );
	void InputEnableAlternatePath( inputdata_t &inputdata );
	void InputDisableAlternatePath( inputdata_t &inputdata );

	void InputTogglePath( inputdata_t &inputdata );
	void InputEnablePath( inputdata_t &inputdata );
	void InputDisablePath( inputdata_t &inputdata );

	DECLARE_DATADESC();

	float		m_length;
	string_t	m_altName;
	CPathTrack	*m_pnext;
	CPathTrack	*m_pprevious;
	CPathTrack	*m_paltpath;

	COutputEvent m_OnPass;
};


class CFuncTrackTrain : public CBaseEntity
{
	DECLARE_CLASS( CFuncTrackTrain, CBaseEntity );
public:
	CFuncTrackTrain();

	void Spawn( void );
	bool CreateVPhysics( void );
	void Precache( void );

	void Blocked( CBaseEntity *pOther );
	bool KeyValue( const char *szKeyName, const char *szValue );

	virtual int DrawDebugTextOverlays();
	void DrawDebugGeometryOverlays();

	void Next( void );
	void Find( void );
	void NearestPath( void );
	void DeadEnd( void );

	void SetTrack( CPathTrack *track ) { m_ppath = track->Nearest(GetLocalOrigin()); }
	void SetControls( CBaseEntity *pControls );
	bool OnControls( CBaseEntity *pControls );

	void SoundStop( void );
	void SoundUpdate( void );

	void Start( void );
	void Stop( void );

	void SetSpeed( float flSpeed );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	// Input handlers
	void InputSetSpeed( inputdata_t &inputdata );
	void InputSetSpeedDir( inputdata_t &inputdata );
	void InputStop( inputdata_t &inputdata );
	void InputResume( inputdata_t &inputdata );
	void InputReverse( inputdata_t &inputdata );
	void InputStartForward( inputdata_t &inputdata );
	void InputStartBackward( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	static CFuncTrackTrain *Instance( edict_t *pent );

	DECLARE_DATADESC();

	virtual int	ObjectCaps( void ) { return BaseClass :: ObjectCaps() | FCAP_DIRECTIONAL_USE | FCAP_USE_ONGROUND; }

	virtual void	OnRestore( void );

	float GetMaxSpeed() const { return m_maxSpeed; }
	float GetCurrentSpeed() const { return m_flSpeed; }

private:
	void ArriveAtNode( CPathTrack *pNode );
	void FirePassInputs( CPathTrack *pStart, CPathTrack *pEnd, bool forward );

public:
	// UNDONE: Add accessors?
	CPathTrack	*m_ppath;
	float		m_length;
private:

	float		m_height;
	float		m_maxSpeed;
	float		m_dir;
	Vector		m_controlMins;
	Vector		m_controlMaxs;
	bool		m_bSoundPlaying;
	float		m_flVolume;
	float		m_flBank;
	float		m_oldSpeed;
	float		m_flBlockDamage;			// Damage to inflict when blocked.

	string_t	m_iszSoundMove;				// Looping sound to play while moving. Pitch shifted based on speed.
	string_t	m_iszSoundStart;			// Sound to play when starting to move.
	string_t	m_iszSoundStop;				// Sound to play when stopping.

	Vector		m_vecOldOrigin;
};


#endif // TRAINS_H
