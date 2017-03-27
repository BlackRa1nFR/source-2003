//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Definitions of all the entities that control logic flow within a map
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "EntityInput.h"
#include "EntityOutput.h"
#include "EventQueue.h"
#include "player.h"
#include "world.h"
#include "ndebugoverlay.h"


//-----------------------------------------------------------------------------
// Purpose: Compares a set of integer inputs to the one main input
//			Outputs true if they are all equivalant, false otherwise
//-----------------------------------------------------------------------------
class CFogController : public CLogicalEntity
{
public:
	DECLARE_CLASS( CFogController, CLogicalEntity );

	CFogController();
	~CFogController();

	// Input handlers
	void InputSetStartDist(inputdata_t &data);
	void InputSetEndDist(inputdata_t &data);
	void InputTurnOn(inputdata_t &data);
	void InputTurnOff(inputdata_t &data);
	void InputSetColor(inputdata_t &data);
	void InputSetColorSecondary(inputdata_t &data);

	int CFogController::DrawDebugTextOverlays(void);

	DECLARE_DATADESC();


public:

	fogparams_t	m_fog;

	static CFogController *s_pFogController;

};


CFogController *CFogController::s_pFogController = NULL;


LINK_ENTITY_TO_CLASS( env_fog_controller, CFogController );


BEGIN_DATADESC( CFogController )

	DEFINE_INPUTFUNC( CFogController, FIELD_FLOAT,		"SetStartDist",	InputSetStartDist ),
	DEFINE_INPUTFUNC( CFogController, FIELD_FLOAT,		"SetEndDist",	InputSetEndDist ),
	DEFINE_INPUTFUNC( CFogController, FIELD_VOID,		"TurnOn",		InputTurnOn ),
	DEFINE_INPUTFUNC( CFogController, FIELD_VOID,		"TurnOff",		InputTurnOff ),
	DEFINE_INPUTFUNC( CFogController, FIELD_COLOR32,	"SetColor",		InputSetColor ),
	DEFINE_INPUTFUNC( CFogController, FIELD_COLOR32,	"SetColorSecondary",	InputSetColorSecondary ),

	// Quiet classcheck
	// DEFINE_FIELD( CFogController, m_fog, fogparams_t ),

	DEFINE_KEYFIELD( CFogController, m_fog.colorPrimary,	FIELD_COLOR32,	"fogcolor" ),
	DEFINE_KEYFIELD( CFogController, m_fog.colorSecondary,	FIELD_COLOR32,	"fogcolor2" ),
	DEFINE_KEYFIELD( CFogController, m_fog.dirPrimary,		FIELD_VECTOR,	"fogdir" ),
	DEFINE_KEYFIELD( CFogController, m_fog.enable,			FIELD_BOOLEAN,	"fogenable" ),
	DEFINE_KEYFIELD( CFogController, m_fog.blend,			FIELD_BOOLEAN,	"fogblend" ),
	DEFINE_KEYFIELD( CFogController, m_fog.start,			FIELD_FLOAT,	"fogstart" ),
	DEFINE_KEYFIELD( CFogController, m_fog.end,				FIELD_FLOAT,	"fogend" ),
	DEFINE_KEYFIELD( CFogController, m_fog.farz,			FIELD_FLOAT,	"farz" ),

END_DATADESC()



CFogController::CFogController()
{
	// Make sure that old maps without fog fields don't get wacked out fog values.
	m_fog.enable = false;

	if ( s_pFogController )
	{
		// There should only be one fog controller in the level. Not a fatal error, but
		// the level designer should fix it.
		Warning( "Found multiple fog controllers in the same level.\n" );
	}
	else
	{
		s_pFogController = this;
	}
}


CFogController::~CFogController()
{
	if ( s_pFogController == this )
	{
		s_pFogController = NULL;
	}
	else
	{
		// There should only be one fog controller in the level. Not a fatal error, but
		// the level designer should fix it.
		Warning( "Found multiple fog controllers in the same level.\n" );
	}
}


//------------------------------------------------------------------------------
// Purpose: Input handler for setting the fog start distance.
//------------------------------------------------------------------------------
void CFogController::InputSetStartDist(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.start = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Purpose: Input handler for setting the fog end distance.
//------------------------------------------------------------------------------
void CFogController::InputSetEndDist(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.end = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Purpose: Input handler for turning on the fog.
//------------------------------------------------------------------------------
void CFogController::InputTurnOn(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.enable = true;
}

//------------------------------------------------------------------------------
// Purpose: Input handler for turning off the fog.
//------------------------------------------------------------------------------
void CFogController::InputTurnOff(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.enable = false;
}

//------------------------------------------------------------------------------
// Purpose: Input handler for setting the primary fog color.
//------------------------------------------------------------------------------
void CFogController::InputSetColor(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.colorPrimary = inputdata.value.Color32();
}


//------------------------------------------------------------------------------
// Purpose: Input handler for setting the secondary fog color.
//------------------------------------------------------------------------------
void CFogController::InputSetColorSecondary(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.colorSecondary = inputdata.value.Color32();
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CFogController::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr,sizeof(tempstr),"State: %s",(m_fog.enable)?"On":"Off");
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Start: %3.0f",m_fog.start);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"End  : %3.0f",m_fog.end);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		color32 color = m_fog.colorPrimary;
		Q_snprintf(tempstr,sizeof(tempstr),"1) Red  : %i",color.r);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"1) Green: %i",color.g);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"1) Blue : %i",color.b);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		color = m_fog.colorSecondary;
		Q_snprintf(tempstr,sizeof(tempstr),"2) Red  : %i",color.r);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"2) Green: %i",color.g);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"2) Blue : %i",color.b);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fogEnable - 
//			fogColorPrimary - 
//			fogColorSecondary - 
//			fogDirPrimary - 
//			fogStart - 
//			fogEnd - 
//-----------------------------------------------------------------------------
void GetWorldFogParams( fogparams_t &fog )
{
	if ( CFogController::s_pFogController )
	{
		fog = CFogController::s_pFogController->m_fog;
	}
	else
	{
		// No fog controller in this level. Use default fog parameters.
		fog.farz = -1;
		fog.enable = false;
	}
}

