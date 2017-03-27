//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Used to fire events based on the orientation of a given entity.
//
//			Looks at its target's anglular velocity every frame and fires outputs
//			as the angular velocity passes a given threshold value.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "mathlib.h"


enum
{
	AVELOCITY_SENSOR_NO_LAST_RESULT = -2
};


class CPointAngularVelocitySensor : public CPointEntity
{
	DECLARE_CLASS( CPointAngularVelocitySensor, CPointEntity );

public:

	void Activate(void);
	void Spawn(void);
	void Think(void);

private:

	int CompareToThreshold(CBaseEntity *pEntity, float flThreshold);
	void FireCompareOutput(int nCompareResult, CBaseEntity *pActivator);

	// Input handlers
	void InputTest( inputdata_t &inputdata );

	EHANDLE m_hTargetEntity;				// Entity whose angles are being monitored.
	float m_flThreshold;					// The threshold angular velocity that we are looking for.
	int m_nLastCompareResult;				// Tha comparison result from our last measurement, expressed as -1, 0, or 1
	float m_flFireTime;
	float m_flSustainTime;

	// Outputs
	COutputEvent m_OnLessThan;				// Fired when the target's angular velocity becomes less than the threshold velocity.
	COutputEvent m_OnLessThanOrEqualTo;		
	COutputEvent m_OnGreaterThan;			
	COutputEvent m_OnGreaterThanOrEqualTo;
	COutputEvent m_OnEqualTo;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(point_angularvelocitysensor, CPointAngularVelocitySensor);


BEGIN_DATADESC( CPointAngularVelocitySensor )

	// Fields
	DEFINE_FIELD( CPointAngularVelocitySensor, m_hTargetEntity, FIELD_EHANDLE ),
	DEFINE_KEYFIELD(CPointAngularVelocitySensor, m_flThreshold, FIELD_FLOAT, "threshold"),
	DEFINE_FIELD(CPointAngularVelocitySensor, m_nLastCompareResult, FIELD_INTEGER),
	DEFINE_FIELD( CPointAngularVelocitySensor, m_flFireTime, FIELD_TIME ),
	DEFINE_FIELD( CPointAngularVelocitySensor, m_flSustainTime, FIELD_TIME ),
	
	// Inputs
	DEFINE_INPUTFUNC(CPointAngularVelocitySensor, FIELD_VOID, "Test", InputTest),

	// Outputs
	DEFINE_OUTPUT(CPointAngularVelocitySensor, m_OnLessThan, "OnLessThan"),
	DEFINE_OUTPUT(CPointAngularVelocitySensor, m_OnLessThanOrEqualTo, "OnLessThanOrEqualTo"),
	DEFINE_OUTPUT(CPointAngularVelocitySensor, m_OnGreaterThan, "OnGreaterThan"),
	DEFINE_OUTPUT(CPointAngularVelocitySensor, m_OnGreaterThanOrEqualTo, "OnGreaterThanOrEqualTo"),
	DEFINE_OUTPUT(CPointAngularVelocitySensor, m_OnEqualTo, "OnEqualTo"),

END_DATADESC()




//-----------------------------------------------------------------------------
// Purpose: Called when spawning after parsing keyvalues.
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::Spawn(void)
{
	m_flThreshold = fabs(m_flThreshold);
	m_nLastCompareResult = AVELOCITY_SENSOR_NO_LAST_RESULT;
	m_flSustainTime = 0.2;
}


//-----------------------------------------------------------------------------
// Purpose: Called after all entities in the map have spawned.
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::Activate(void)
{
	BaseClass::Activate();

	m_hTargetEntity = gEntList.FindEntityByName(NULL, m_target, NULL);

	if (m_hTargetEntity)
	{
		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Compares the given entity's angular velocity to the threshold velocity.
// Input  : pEntity - Entity whose angular velocity is being measured.
//			flThreshold - 
// Output : Returns -1 if less than, 0 if equal to, or 1 if greater than the threshold.
//-----------------------------------------------------------------------------
int CPointAngularVelocitySensor::CompareToThreshold(CBaseEntity *pEntity, float flThreshold)
{
	if (pEntity == NULL)
	{
		return 0;
	}
	
	if (pEntity->GetMoveType() == MOVETYPE_VPHYSICS)
	{
		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if (pPhys != NULL)
		{
			Vector vecVelocity;
			AngularImpulse vecAngVelocity;
			pPhys->GetVelocity(&vecVelocity, &vecAngVelocity);
			float flAngVelocity = vecAngVelocity.Length();

			if (flAngVelocity > flThreshold)
			{
				return 1;
			}

			if (flAngVelocity == flThreshold)
			{
				return 0;
			}
		}
	}
	else
	{
		if ((fabs(pEntity->GetLocalAngularVelocity()[PITCH]) > flThreshold) ||
			(fabs(pEntity->GetLocalAngularVelocity()[YAW]) > flThreshold) ||
			(fabs(pEntity->GetLocalAngularVelocity()[ROLL]) > flThreshold))
		{
			return 1;
		}

		if ((fabs(pEntity->GetLocalAngularVelocity()[PITCH]) == flThreshold) ||
			(fabs(pEntity->GetLocalAngularVelocity()[YAW]) == flThreshold) ||
			(fabs(pEntity->GetLocalAngularVelocity()[ROLL]) == flThreshold))
		{
			return 0;
		}
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame.
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::Think(void)
{
	if (m_hTargetEntity != NULL)
	{
		//
		// Check to see if the measure entity's forward vector has been within
		// given tolerance of the target entity for the given period of time.
		//
		int nCompare = CompareToThreshold(m_hTargetEntity, m_flThreshold);
		if ((nCompare != m_nLastCompareResult) && (m_nLastCompareResult != AVELOCITY_SENSOR_NO_LAST_RESULT))
		{
			if (!m_flFireTime)
			{
				m_flFireTime = gpGlobals->curtime;
			}

			if (gpGlobals->curtime >= m_flFireTime + m_flSustainTime)
			{
				//
				// The compare result has changed. We need to fire the output.
				//
				FireCompareOutput(nCompare, this);

				// Save the result for next time.
				m_flFireTime = 0;
				m_nLastCompareResult = nCompare;
			}
		}
		else if (m_nLastCompareResult == AVELOCITY_SENSOR_NO_LAST_RESULT) 
		{
			m_nLastCompareResult = nCompare;
		}

		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for forcing an instantaneous test of the condition.
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::InputTest( inputdata_t &inputdata )
{
	int nCompareResult = CompareToThreshold(m_hTargetEntity, m_flThreshold);
	FireCompareOutput(nCompareResult, inputdata.pActivator);
}

	
//-----------------------------------------------------------------------------
// Purpose: Fires the appropriate output based on the given comparison result.
// Input  : nCompareResult - 
//			pActivator - 
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::FireCompareOutput( int nCompareResult, CBaseEntity *pActivator )
{
	if (nCompareResult == -1)
	{
		m_OnLessThan.FireOutput(pActivator, this);
		m_OnLessThanOrEqualTo.FireOutput(pActivator, this);
	}
	else if (nCompareResult == 1)
	{
		m_OnGreaterThan.FireOutput(pActivator, this);
		m_OnGreaterThanOrEqualTo.FireOutput(pActivator, this);
	}
	else
	{
		m_OnEqualTo.FireOutput(pActivator, this);
		m_OnLessThanOrEqualTo.FireOutput(pActivator, this);
		m_OnGreaterThanOrEqualTo.FireOutput(pActivator, this);
	}
}

