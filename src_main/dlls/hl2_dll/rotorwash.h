#ifndef ROTORWASH_H
#define ROTORWASH_H

#include "te_particlesystem.h"

class CTERotorWash : public CTEParticleSystem
{
public:
	DECLARE_CLASS( CTERotorWash, CTEParticleSystem );
	DECLARE_SERVERCLASS();

					CTERotorWash( const char *name = "noname");
	virtual 		~CTERotorWash();

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles ) { };
	virtual	void	Create( IRecipientFilter& filter, float delay = 0.0f );

	CNetworkVector( m_vecDown );
	CNetworkVar( float, m_flMaxAltitude );
};

#endif	//ROTORWASH_H