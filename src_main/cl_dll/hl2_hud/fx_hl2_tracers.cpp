//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================
#include "cbase.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"

//-----------------------------------------------------------------------------
// Purpose: Gunship's Tracer
//-----------------------------------------------------------------------------
void GunshipTracerCallback( const CEffectData &data )
{
	float flVelocity = data.m_flScale;
	bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	FX_GunshipTracer( (Vector&)data.m_vStart, (Vector&)data.m_vOrigin, flVelocity, bWhiz );
}

DECLARE_CLIENT_EFFECT( "GunshipTracer", GunshipTracerCallback );


//-----------------------------------------------------------------------------
// Purpose: Strider's Tracer
//-----------------------------------------------------------------------------
void StriderTracerCallback( const CEffectData &data )
{
	float flVelocity = data.m_flScale;
	bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	FX_StriderTracer( (Vector&)data.m_vStart, (Vector&)data.m_vOrigin, flVelocity, bWhiz );
}

DECLARE_CLIENT_EFFECT( "StriderTracer", StriderTracerCallback );


//-----------------------------------------------------------------------------
// Purpose: Gauss Gun's Tracer
//-----------------------------------------------------------------------------
void GaussTracerCallback( const CEffectData &data )
{
	float flVelocity = data.m_flScale;
	bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	FX_GaussTracer( (Vector&)data.m_vStart, (Vector&)data.m_vOrigin, flVelocity, bWhiz );
}

DECLARE_CLIENT_EFFECT( "GaussTracer", GaussTracerCallback );
