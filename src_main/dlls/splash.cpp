//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "splash.h"
#include "dt_send.h"
#include "ndebugoverlay.h"

#define SPLASH_ENTITYNAME		"env_splash"

//Data table
IMPLEMENT_SERVERCLASS_ST(CSplash, DT_Splash)
	SendPropFloat(	SENDINFO(m_flSpawnRate), 8, 0, 1, 1024),
	SendPropVector(	SENDINFO(m_vStartColor), 8, 0, 0, 1),
	SendPropVector(	SENDINFO(m_vEndColor), 8, 0, 0, 1),
	SendPropFloat(	SENDINFO(m_flParticleLifetime), 16, SPROP_ROUNDUP, 0.1, 100),
	SendPropFloat(	SENDINFO(m_flStopEmitTime), 0, SPROP_NOSCALE),
	SendPropFloat(	SENDINFO(m_flSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(	SENDINFO(m_flSpeedRange), -1, SPROP_NOSCALE),
	SendPropFloat(	SENDINFO(m_flWidthMin), -1, SPROP_NOSCALE),
	SendPropFloat(	SENDINFO(m_flWidthMax), -1, SPROP_NOSCALE),
	SendPropFloat(	SENDINFO(m_flNoise), -1, SPROP_NOSCALE),
	SendPropInt	 (	SENDINFO(m_bEmit), 1, SPROP_UNSIGNED),
	SendPropInt	 (	SENDINFO(m_nNumDecals), 4, SPROP_UNSIGNED),
END_SEND_TABLE()

//NOTENOTE: This is currently being disabled until it can be destroyed, reimplemented, or renamed - jdw
//LINK_ENTITY_TO_CLASS(env_splash, CSplash);

BEGIN_DATADESC( CSplash )

	DEFINE_KEYFIELD( CSplash, m_flSpawnRate,		FIELD_FLOAT,	"spawnrate" ),
	DEFINE_KEYFIELD( CSplash, m_rgbaStartColor,		FIELD_COLOR32,	"startcolor" ),
	DEFINE_KEYFIELD( CSplash, m_rgbaEndColor,		FIELD_COLOR32,	"endcolor" ),
	DEFINE_KEYFIELD( CSplash, m_flSpeed,			FIELD_FLOAT,	"speed" ),
	DEFINE_KEYFIELD( CSplash, m_flSpeedRange,		FIELD_FLOAT,	"speedrange" ),
	DEFINE_KEYFIELD( CSplash, m_flWidthMin,			FIELD_FLOAT,	"widthmin" ),
	DEFINE_KEYFIELD( CSplash, m_flWidthMax,			FIELD_FLOAT,	"widthmax" ),
	DEFINE_KEYFIELD( CSplash, m_flNoise,			FIELD_FLOAT,	"noise" ),
	DEFINE_KEYFIELD( CSplash, m_flParticleLifetime,	FIELD_FLOAT,	"lifetime" ),
	DEFINE_KEYFIELD( CSplash, m_bEmit,				FIELD_BOOLEAN,	"startactive" ),
	DEFINE_KEYFIELD( CSplash, m_nNumDecals,			FIELD_INTEGER,	"numdecals" ),

	DEFINE_FIELD( CSplash, m_vStartColor, FIELD_VECTOR ),
	DEFINE_FIELD( CSplash, m_vEndColor, FIELD_VECTOR ),
	DEFINE_FIELD( CSplash, m_flStopEmitTime, FIELD_TIME ),

	DEFINE_INPUTFUNC( CSplash, FIELD_FLOAT, "SetSpawnRate", InputSetSpawnRate ),
	DEFINE_INPUTFUNC( CSplash, FIELD_FLOAT, "SetSpeed", InputSetSpeed ),
	DEFINE_INPUTFUNC( CSplash, FIELD_FLOAT, "SetNoise", InputSetNoise ),
	DEFINE_INPUTFUNC( CSplash, FIELD_FLOAT, "SetLifetime", InputSetParticleLifetime ),
	DEFINE_INPUTFUNC( CSplash, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CSplash, FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CSplash::Spawn(void)
{
	// Never die unless otherwise told to die
	SetNextThink( TICK_NEVER_THINK );

	// Translate colors
	m_vStartColor.Init( m_rgbaStartColor.r/255, m_rgbaStartColor.g/255, m_rgbaStartColor.b/255 );

	m_vEndColor.Init( m_rgbaEndColor.r/255, m_rgbaEndColor.g/255, m_rgbaEndColor.b/255 );
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CSplash::InputSetSpawnRate( inputdata_t &inputdata )
{
	m_flSpawnRate = inputdata.value.Float();
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CSplash::InputSetSpeed( inputdata_t &inputdata )
{
	m_flSpeed = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CSplash::InputSetNoise( inputdata_t &inputdata )
{
	m_flNoise = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CSplash::InputSetParticleLifetime( inputdata_t &inputdata )
{
	m_flParticleLifetime = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CSplash::InputTurnOn( inputdata_t &inputdata )
{
	m_bEmit = true;
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CSplash::InputTurnOff( inputdata_t &inputdata )
{
	m_bEmit = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSplash::CSplash(void)
{
	m_flSpawnRate = 10;
	m_vStartColor.Init(0.5, 0.5, 0.5);
	m_vEndColor.Init(0,0,0);
	m_flParticleLifetime = 5;
	m_flStopEmitTime		= 0; // Don't stop emitting particles
	m_flSpeed			= 3;
	m_flSpeedRange		= 1;
	m_flWidthMin		= 2;
	m_flWidthMax		= 8;
	m_flNoise			= 0.1;
	m_bEmit = true;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CSplash::~CSplash(void)
{
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CSplash::SetEmit(bool bVal)
{
	m_bEmit = bVal;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CSplash*
//-----------------------------------------------------------------------------
CSplash* CSplash::CreateSplash(const Vector &vPosition)
{
	CBaseEntity *pEnt = CreateEntityByName(SPLASH_ENTITYNAME);
	if(pEnt)
	{
		CSplash *pSplash = dynamic_cast<CSplash*>(pEnt);
		if(pSplash)
		{
			pSplash->SetLocalOrigin( vPosition );
			pSplash->Activate();
			return pSplash;
		}
		else
		{
			UTIL_Remove(pEnt);
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CSplash::DrawDebugTextOverlays(void) 
{
	// Skip AIClass debug overlays
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print Target
		// --------------
		char tempstr[255];
		if (m_bEmit) 
		{
			Q_strncpy(tempstr,"State: On",sizeof(tempstr));
		}
		else
		{
			Q_strncpy(tempstr,"State: Off",sizeof(tempstr));
		}
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Rate: %3.2f",m_flSpawnRate);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Speed: %3.2f - %3.2f",m_flSpeed + m_flSpeedRange, m_flSpeed - m_flSpeedRange);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Noise: %3.2f",m_flNoise);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Lifetime: %3.2f",m_flParticleLifetime);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

	}
	return text_offset;
}