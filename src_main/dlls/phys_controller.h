//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef PHYS_CONTROLLER_H
#define PHYS_CONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CKeepUpright : public CPointEntity, public IMotionEvent
{
	DECLARE_CLASS( CKeepUpright, CPointEntity );
public:
	DECLARE_DATADESC();

	CKeepUpright();
	~CKeepUpright();
	void Spawn();
	void Activate();

	// IMotionEvent
	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

	static CKeepUpright *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, float flAngularLimit, bool bActive );

	// Inputs
	void InputTurnOn( inputdata_t &inputdata )
	{
		m_bActive = true;
	}
	void InputTurnOff( inputdata_t &inputdata )
	{
		m_bActive = false;
	}

private:	
	Vector						m_worldGoalAxis;
	Vector						m_localTestAxis;
	IPhysicsMotionController	*m_pController;
	string_t					m_nameAttach;
	EHANDLE						m_attachedObject;
	float						m_angularLimit;
	bool						m_bActive;
};

#endif // PHYS_CONTROLLER_H
