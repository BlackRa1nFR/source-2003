#include <stdio.h>
#include "CvarSlider.h"
#include "EngineInterface.h"
#include <KeyValues.h>
#include <vgui/IVGui.h>

#define CVARSLIDER_SCALE_FACTOR 100.0f

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCvarSlider::CCvarSlider( Panel *parent, const char *panelName, char const *caption,
		float minValue, float maxValue, char const *cvarname, bool bAllowOutOfRange )
  : Slider( parent, panelName )
{
	AddActionSignalTarget( this );

	// scale by CVARSLIDER_SCALE_FACTOR
	SetRange( (int)( CVARSLIDER_SCALE_FACTOR * minValue ), (int)( CVARSLIDER_SCALE_FACTOR * maxValue ) );

	char szMin[ 32 ];
	char szMax[ 32 ];

	sprintf( szMin, "%.2f", minValue );
	sprintf( szMax, "%.2f", maxValue );

	SetTickCaptions( szMin, szMax );

	strcpy( m_szCvarName, cvarname );

    m_bModifiedOnce = false;
    m_bAllowOutOfRange = bAllowOutOfRange;

	// Set slider to current value
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCvarSlider::~CCvarSlider()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarSlider::Paint()
{
	// Get engine's current value
//	float curvalue = engine->pfnGetCvarFloat( m_szCvarName );
	ConVar const *var = cvar->FindVar( m_szCvarName );
	if ( !var )
		return;
	float curvalue = var->GetFloat();
	
    // did it get changed from under us?
    if (curvalue != m_fStartValue)
    {
        int val = (int)( CVARSLIDER_SCALE_FACTOR * curvalue );
        m_fStartValue = curvalue;
        m_fCurrentValue = curvalue;
        
        SetValue( val );
        m_iStartValue = GetValue();
    }

	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarSlider::ApplyChanges()
{
    if (m_bModifiedOnce)
    {
        m_iStartValue = GetValue();
        if (m_bAllowOutOfRange)
        {
            m_fStartValue = m_fCurrentValue;
        }
        else
        {
            m_fStartValue = (float) m_iStartValue / CVARSLIDER_SCALE_FACTOR;
        }
    
		//engine->Cvar_SetValue( m_szCvarName, m_fStartValue );
		ConVar *var = (ConVar *)cvar->FindVar( m_szCvarName );
		if ( !var )
			return;
		var->SetValue((float)m_fStartValue);   
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CCvarSlider::GetSliderValue()
{	
    if (m_bAllowOutOfRange)
    {
	    return m_fCurrentValue; 
    }
    else
    {
        return ((float)GetValue())/ CVARSLIDER_SCALE_FACTOR;
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarSlider::SetSliderValue(float fValue)
{
    int nVal = (int)( CVARSLIDER_SCALE_FACTOR * fValue );
    SetValue( nVal, false);

    // remember this slider value
    m_iLastSliderValue = GetValue();

    if (m_fCurrentValue != fValue)
    {
        m_fCurrentValue = fValue;
        m_bModifiedOnce = true;
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarSlider::Reset()
{
	// Set slider to current value
//	m_fStartValue = engine->pfnGetCvarFloat( m_szCvarName );
	ConVar const *var = cvar->FindVar( m_szCvarName );
	if ( !var )
		return;
	float m_fStartValue = var->GetFloat();
    m_fCurrentValue = m_fStartValue;

    int value = (int)( CVARSLIDER_SCALE_FACTOR * m_fStartValue );
	SetValue( value );

	m_iStartValue = GetValue();
    m_iLastSliderValue = m_iStartValue;

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCvarSlider::HasBeenModified()
{
    if (GetValue() != m_iStartValue)
    {
        m_bModifiedOnce = true;
    }

    return m_bModifiedOnce;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : position - 
//-----------------------------------------------------------------------------
void CCvarSlider::OnSliderMoved()
{
	if (HasBeenModified())
	{
        if (m_iLastSliderValue != GetValue())
        {
            m_iLastSliderValue = GetValue();
            m_fCurrentValue = ((float) m_iLastSliderValue)/CVARSLIDER_SCALE_FACTOR;
        }

		// tell parent that we've been modified
		PostActionSignal(new KeyValues("ControlModified"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
MessageMapItem_t CCvarSlider::m_MessageMap[] =
{
	MAP_MESSAGE( CCvarSlider, "SliderMoved", OnSliderMoved ),		// custom message
};

IMPLEMENT_PANELMAP(CCvarSlider, BaseClass);