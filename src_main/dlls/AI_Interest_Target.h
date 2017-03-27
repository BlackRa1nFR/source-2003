//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose: Hooks and classes for the support of humanoid NPCs with 
//			groovy facial animation capabilities, aka, "Actors"
//
//=============================================================================

#ifndef AI_INTEREST_TARGET_H
#define AI_INTEREST_TARGET_H

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// CAI_BaseActor
//
// Purpose: The base class for all facially expressive NPCS.
//
//-----------------------------------------------------------------------------

class CAI_InterestTarget_t
{
public:
	enum CAI_InterestTarget_e
	{
		LOOKAT_ENTITY = 0,
		LOOKAT_POSITION, 
		LOOKAT_BOTH
	};

public:
	bool			IsThis( CBaseEntity *pThis )
	{
		return (pThis == m_hTarget);
	};

	const Vector	&GetPosition( void )
	{
		if (m_eType	== LOOKAT_ENTITY && m_hTarget != NULL)
		{
			m_vecPosition = m_hTarget->EyePosition();
		}
		return m_vecPosition;
	};

	bool			IsActive( void )
	{
		if (m_flEndTime < gpGlobals->curtime) return false;
		if (m_eType == LOOKAT_ENTITY && m_hTarget == NULL) return false;
		return true;
	};

	float			Interest( void )
	{
		float t = (gpGlobals->curtime - m_flStartTime) / (m_flEndTime - m_flStartTime);

		if (t < 0.0f || t > 1.0f)
			return 0.0f;

		if (t < m_flRamp)
		{
			t = t / m_flRamp;
		}
		else if (t > 1.0f - m_flRamp)
		{
			t = (1.0 - t) / m_flRamp;
		} 
		else
		{
			t = 1.0f;
		}
		// ramp
		t = 3.0f * t * t - 2.0f * t * t * t;
		t *= m_flInterest;

		return t;
	}

public:
	CAI_InterestTarget_e	m_eType; // ????

	EHANDLE		m_hTarget;
	Vector		m_vecPosition;
	float		m_flStartTime;
	float		m_flEndTime;
	float		m_flRamp;
	float		m_flInterest;
};


class CAI_InterestTarget : public CUtlVector<CAI_InterestTarget_t>
{
public:
	void Add( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp )
	{
		int i;

		for (i = 0; i < Count(); i++)
		{
			CAI_InterestTarget_t &target = Element( i );

			if (target.m_hTarget == pTarget)
			{
				Remove( i );
				break;
			}
		}

		Add( CAI_InterestTarget_t::LOOKAT_ENTITY, pTarget, Vector( 0, 0, 0 ), flImportance, flDuration, flRamp );
	}

	void Add( const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
	{
		int i;

		for (i = 0; i < Count(); i++)
		{
			CAI_InterestTarget_t &target = Element( i );

			if (target.m_vecPosition == vecPosition)
			{
				Remove( i );
				break;
			}
		}

		Add( CAI_InterestTarget_t::LOOKAT_POSITION, NULL, vecPosition, flImportance, flDuration, flRamp );
	}

	void Add( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
	{
		int i;

		for (i = 0; i < Count(); i++)
		{
			CAI_InterestTarget_t &target = Element( i );

			if (target.m_hTarget == pTarget)
			{
				Remove( i );
				break;
			}
		}

		Add( CAI_InterestTarget_t::LOOKAT_BOTH, pTarget, vecPosition, flImportance, flDuration, flRamp );
	}

private:
	void Add( CAI_InterestTarget_t::CAI_InterestTarget_e type, CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
	{
		int i = AddToTail();
		CAI_InterestTarget_t &target = Element( i );

		target.m_eType = type;
		target.m_hTarget = pTarget;
		target.m_vecPosition = vecPosition;
		target.m_flInterest = flImportance;
		target.m_flStartTime = gpGlobals->curtime;
		target.m_flEndTime = gpGlobals->curtime + flDuration;
		target.m_flRamp = flRamp / flDuration;
	}
};


//-----------------------------------------------------------------------------
#endif // AI_INTEREST_TARGET_H
