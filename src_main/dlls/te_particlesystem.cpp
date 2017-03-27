//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "dt_send.h"
#include "server_class.h"
#include "te_particlesystem.h"


IMPLEMENT_SERVERCLASS_ST(CTEParticleSystem, DT_TEParticleSystem)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
END_SEND_TABLE()




