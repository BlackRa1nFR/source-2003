//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MODELENTITIES_H
#define MODELENTITIES_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: basic solid geometry
// enabled state:	brush is visible
// disabled staute:	brush not visible
//-----------------------------------------------------------------------------
class CFuncBrush : public CBaseEntity
{
public:
	DECLARE_CLASS( CFuncBrush, CBaseEntity );

	virtual void Spawn( void );
	bool CreateVPhysics( void );

	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; }

	virtual int DrawDebugTextOverlays( void );

	void TurnOff( void );
	void TurnOn( void );

	// Input handlers
	void InputTurnOff( inputdata_t &inputdata );
	void InputTurnOn( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	bool	ShouldSavePhysics()	{ return false; }
	
	enum BrushSolidities_e {
		BRUSHSOLID_TOGGLE = 0,
		BRUSHSOLID_NEVER  = 1,
		BRUSHSOLID_ALWAYS = 2,
	};

	BrushSolidities_e m_iSolidity;
	int m_iDisabled;

	DECLARE_DATADESC();

	virtual bool IsOn( void );
};


#endif // MODELENTITIES_H
