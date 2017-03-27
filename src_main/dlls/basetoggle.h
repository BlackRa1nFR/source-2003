//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: For the slow removing of the CBaseToggle entity
//			only old entities that need it for backwards-compatibility should
//			include this file
//=============================================================================

#ifndef BASETOGGLE_H
#define BASETOGGLE_H
#pragma once

#include "baseentity.h"


class CBaseToggle : public CBaseEntity
{
	DECLARE_CLASS( CBaseToggle, CBaseEntity );
public:
	CBaseToggle();

	virtual bool		KeyValue( const char *szKeyName, const char *szValue );
	virtual bool		KeyValue( const char *szKeyName, Vector vec ) { return BaseClass::KeyValue( szKeyName, vec ); };
	virtual bool		KeyValue( const char *szKeyName, float flValue ) { return BaseClass::KeyValue( szKeyName, flValue ); };

	TOGGLE_STATE		m_toggle_state;
	float				m_flMoveDistance;// how far a door should slide or rotate
	float				m_flWait;
	float				m_flLip;

	Vector				m_vecPosition1;
	Vector				m_vecPosition2;

	QAngle				m_vecMoveAng;
	QAngle				m_vecAngle1;
	QAngle				m_vecAngle2;

	float				m_flHeight;
	EHANDLE				m_hActivator;
	Vector				m_vecFinalDest;
	QAngle				m_vecFinalAngle;

	int					m_movementType;

	DECLARE_DATADESC();

	virtual float	GetDelay( void ) { return m_flWait; }

	// common member functions
	void LinearMove( const Vector &vecDest, float flSpeed );
	void LinearMoveDone( void );
	void AngularMove( const QAngle &vecDestAngle, float flSpeed );
	void AngularMoveDone( void );
	bool IsLockedByMaster( void );
	virtual void MoveDone( void );

	static float AxisValue( int flags, const QAngle &angles );
	void AxisDir( void );
	static float AxisDelta( int flags, const QAngle &angle1, const QAngle &angle2 );

	string_t m_sMaster;		// If this button has a master switch, this is the targetname.
							// A master switch must be of the multisource type. If all 
							// of the switches in the multisource have been triggered, then
							// the button will be allowed to operate. Otherwise, it will be
							// deactivated.
};



#endif // BASETOGGLE_H
