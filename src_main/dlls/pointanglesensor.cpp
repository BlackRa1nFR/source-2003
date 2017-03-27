//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Used to fire events based on the orientation of a given entity.
//
//			Looks at its target's angles every frame and fires an output if its
//			target's forward vector points at a specified lookat entity for more
//			than a specified length of time.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "mathlib.h"


class CPointAngleSensor : public CPointEntity
{
	DECLARE_CLASS(CPointAngleSensor, CPointEntity);
public:

	bool KeyValue(const char *szKeyName, const char *szValue);
	void Activate(void);
	void Spawn(void);
	void Think(void);

	// Input handlers
	void InputTest(inputdata_t &inputdata);
	void InputSetTargetEntity(inputdata_t &inputdata);

	bool IsFacingWithinTolerance(CBaseEntity *pEntity, CBaseEntity *pTarget, float flTolerance);

	string_t m_nLookAtName;			// Name of the entity that the target must look at to fire the OnTrue output.

	EHANDLE m_hTargetEntity;		// Entity whose angles are being monitored.
	EHANDLE m_hLookAtEntity;		// Entity that the target must look at to fire the OnTrue output.

	float m_flDuration;				// Time in seconds for which the entity must point at the target.
	float m_flDotTolerance;			// Degrees of error allowed to satisfy the condition, expressed as a dot product.
	float m_flFacingTime;			// The time at which the target entity pointed at the lookat entity.
	bool m_bFired;					// Latches the output so it only fires once per true.

	// Outputs
	COutputEvent m_OnFacingLookat;		// Fired when the target points at the lookat entity.
	COutputEvent m_OnNotFacingLookat;	// Fired in response to a Test input if the target is not looking at the lookat entity.
	COutputVector m_TargetDir;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(point_anglesensor, CPointAngleSensor);


BEGIN_DATADESC(CPointAngleSensor)

	// Keys
	DEFINE_KEYFIELD(CPointAngleSensor, m_nLookAtName, FIELD_STRING, "lookatname"),
	DEFINE_FIELD(CPointAngleSensor, m_hTargetEntity, FIELD_EHANDLE),
	DEFINE_FIELD(CPointAngleSensor, m_hLookAtEntity, FIELD_EHANDLE),
	DEFINE_KEYFIELD(CPointAngleSensor, m_flDuration, FIELD_FLOAT, "duration"),
	DEFINE_FIELD(CPointAngleSensor, m_flDotTolerance, FIELD_FLOAT),
	DEFINE_FIELD(CPointAngleSensor, m_flFacingTime, FIELD_TIME),
	DEFINE_FIELD(CPointAngleSensor, m_bFired, FIELD_BOOLEAN),

	// Outputs
	DEFINE_OUTPUT(CPointAngleSensor, m_OnFacingLookat, "OnFacingLookat"),
	DEFINE_OUTPUT(CPointAngleSensor, m_OnNotFacingLookat, "OnNotFacingLookat"),
	DEFINE_OUTPUT(CPointAngleSensor, m_TargetDir, "TargetDir"),

	// Inputs
	DEFINE_INPUTFUNC(CPointAngleSensor, FIELD_VOID, "Test", InputTest),
	DEFINE_INPUTFUNC(CPointAngleSensor, FIELD_STRING, "SetTargetEntity", InputSetTargetEntity),

END_DATADESC()




//-----------------------------------------------------------------------------
// Purpose: Handles keyvalues that require special processing.
// Output : Returns true if handled, false if not.
//-----------------------------------------------------------------------------
bool CPointAngleSensor::KeyValue(const char *szKeyName, const char *szValue)
{
	if (FStrEq(szKeyName, "tolerance"))
	{
		float flTolerance = atof(szValue);
		m_flDotTolerance = cos(DEG2RAD(flTolerance));
	}
	else
	{
		return(BaseClass::KeyValue(szKeyName, szValue));
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Called when spawning after parsing keyvalues.
//-----------------------------------------------------------------------------
void CPointAngleSensor::Spawn(void)
{
	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned on new map or savegame load.
//-----------------------------------------------------------------------------
void CPointAngleSensor::Activate(void)
{
	BaseClass::Activate();

	if (!m_hTargetEntity)
	{
		m_hTargetEntity = gEntList.FindEntityByName(NULL, m_target, NULL);
	}

	if (!m_hLookAtEntity)
	{
		m_hLookAtEntity = gEntList.FindEntityByName(NULL, m_nLookAtName, NULL);
	}

	if (m_hTargetEntity)
	{
		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Determines if one entity is facing within a given tolerance of another
// Input  : pEntity - 
//			pTarget - 
//			flTolerance - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPointAngleSensor::IsFacingWithinTolerance(CBaseEntity *pEntity, CBaseEntity *pTarget, float flTolerance)
{
	if ((pEntity == NULL) || (pTarget == NULL))
	{
		return(false);
	}

	Vector forward;
	pEntity->GetVectors(&forward, NULL, NULL);

	Vector dir = pTarget->GetLocalOrigin() - pEntity->GetLocalOrigin();
	VectorNormalize(dir);

	//
	// Larger dot product corresponds to a smaller angle.
	//
	if (dir.Dot(forward) >= m_flDotTolerance)
	{	
		return(true);
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame.
//-----------------------------------------------------------------------------
void CPointAngleSensor::Think(void)
{
	if (m_hTargetEntity != NULL)
	{
		Vector forward;
		m_hTargetEntity->GetVectors(&forward, NULL, NULL);
		m_TargetDir.Set(forward, this, this); // dvs: need an activator!

		if (m_hLookAtEntity != NULL)
		{
			//
			// Check to see if the measure entity's forward vector has been within
			// given tolerance of the target entity for the given period of time.
			//
			if (IsFacingWithinTolerance(m_hTargetEntity, m_hLookAtEntity, m_flDotTolerance))
			{
				if (!m_bFired)
				{
					if (!m_flFacingTime)
					{
						m_flFacingTime = gpGlobals->curtime;
					}

					if (gpGlobals->curtime >= m_flFacingTime + m_flDuration)
					{
						m_OnFacingLookat.FireOutput(this, this);
						m_bFired = true;
					}
				}
			}
			else if (m_bFired)
			{
				m_bFired = false;
				m_flFacingTime = 0;
			}
		}

		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for forcing an instantaneous test of the condition.
//-----------------------------------------------------------------------------
void CPointAngleSensor::InputTest(inputdata_t &inputdata)
{
	if (IsFacingWithinTolerance(m_hTargetEntity, m_hLookAtEntity, m_flDotTolerance))
	{
		m_OnFacingLookat.FireOutput(inputdata.pActivator, this);
	}
	else
	{
		m_OnNotFacingLookat.FireOutput(inputdata.pActivator, this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointAngleSensor::InputSetTargetEntity(inputdata_t &inputdata)
{
	if ((inputdata.value.String() == NULL) || (inputdata.value.StringID() == NULL_STRING) || (inputdata.value.String()[0] == '\0'))
	{
		m_target = NULL_STRING;
		m_hTargetEntity = NULL;
		SetNextThink( TICK_NEVER_THINK );
	}
	else
	{
		m_target = AllocPooledString(inputdata.value.String());
		m_hTargetEntity = gEntList.FindEntityByName(NULL, m_target, inputdata.pActivator);
		SetNextThink( gpGlobals->curtime );
	}
}
