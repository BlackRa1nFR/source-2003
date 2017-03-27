#include "cbase.h"


#include "particle_smokegrenade.h"


IMPLEMENT_SERVERCLASS_ST(ParticleSmokeGrenade, DT_ParticleSmokeGrenade)
	SendPropFloat(SENDINFO(m_FadeStartTime), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_FadeEndTime), 0, SPROP_NOSCALE),
	SendPropInt(SENDINFO(m_CurrentStage), 1, SPROP_UNSIGNED),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_particlesmokegrenade, ParticleSmokeGrenade );

BEGIN_DATADESC( ParticleSmokeGrenade )

	DEFINE_FIELD( ParticleSmokeGrenade, m_CurrentStage, FIELD_CHARACTER ),
	DEFINE_FIELD( ParticleSmokeGrenade, m_FadeStartTime, FIELD_TIME ),
	DEFINE_FIELD( ParticleSmokeGrenade, m_FadeEndTime, FIELD_TIME ),

END_DATADESC()


ParticleSmokeGrenade::ParticleSmokeGrenade()
{
	m_CurrentStage = 0;
	m_FadeStartTime = 5;
	m_FadeEndTime = 10;
}


void ParticleSmokeGrenade::FillVolume()
{
	m_CurrentStage = 1;
}


void ParticleSmokeGrenade::SetFadeTime(float startTime, float endTime)
{
	m_FadeStartTime = startTime;
	m_FadeEndTime = endTime;
}


