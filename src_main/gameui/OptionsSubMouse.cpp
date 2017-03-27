#include "OptionsSubMouse.h"
//#include "CommandCheckButton.h"
#include "KeyToggleCheckButton.h"
#include "CvarNegateCheckButton.h"
#include "CvarToggleCheckButton.h"
#include "CvarSlider.h"

#include "EngineInterface.h"

#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <stdio.h>
#include <vgui_controls/TextEntry.h>
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

COptionsSubMouse::COptionsSubMouse(vgui::Panel *parent) : PropertyPage(parent, NULL)
{
	m_pReverseMouseCheckBox = new CCvarNegateCheckButton( 
		this, 
		"ReverseMouse", 
		"#GameUI_ReverseMouse", 
		"m_pitch" );
	
	m_pMouseLookCheckBox = new CKeyToggleCheckButton( 
		this, 
		"MouseLook", 
		"#GameUI_MouseLook", 
		"in_mlook",
		"mlook" );

	m_pMouseFilterCheckBox = new CCvarToggleCheckButton( 
		this, 
		"MouseFilter", 
		"#GameUI_MouseFilter", 
		"m_filter" );

	m_pJoystickCheckBox = new CCvarToggleCheckButton( 
		this, 
		"Joystick", 
		"#GameUI_Joystick", 
		"joystick" );

	m_pJoystickLookCheckBox = new CKeyToggleCheckButton( 
		this, 
		"JoystickLook", 
		"#GameUI_JoystickLook", 
		"in_jlook", 
		"jlook" );

	m_pMouseSensitivitySlider = new CCvarSlider( this, "Slider", "#GameUI_MouseSensitivity",
		1.0f, 20.0f, "sensitivity", true );

    m_pMouseSensitivityLabel = new TextEntry(this, "SensitivityLabel");
    m_pMouseSensitivityLabel->AddActionSignalTarget(this);

	m_pAutoAimCheckBox = new CCvarToggleCheckButton(
		this,
		"Auto-Aim",
		"#GameUI_AutoAim",
		"sv_aim" );

//	vgui::Label *l1 = new vgui::Label( this, "AutoaimLabel", "#GameUI_AutoAimLabel" );

	LoadControlSettings("Resource\\OptionsSubMouse.res");

    //float sensitivity = engine->pfnGetCvarFloat( "sensitivity" );
	ConVar const *var = cvar->FindVar( "sensitivity" );
	if ( !var )
		return;

	float sensitivity = var->GetFloat();

    char buf[64];
    _snprintf(buf, sizeof(buf) - 1, " %.1f", sensitivity);
	buf[sizeof(buf) - 1] = 0;
    m_pMouseSensitivityLabel->SetText(buf);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubMouse::~COptionsSubMouse()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnResetData()
{
	m_pReverseMouseCheckBox->Reset();
	m_pMouseLookCheckBox->Reset();
	m_pMouseFilterCheckBox->Reset();
	m_pJoystickCheckBox->Reset();
	m_pJoystickLookCheckBox->Reset();
	m_pMouseSensitivitySlider->Reset();
	m_pAutoAimCheckBox->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnApplyChanges()
{
	m_pReverseMouseCheckBox->ApplyChanges();
	m_pMouseLookCheckBox->ApplyChanges();
	m_pMouseFilterCheckBox->ApplyChanges();
	m_pJoystickCheckBox->ApplyChanges();
	m_pJoystickLookCheckBox->ApplyChanges();
	m_pMouseSensitivitySlider->ApplyChanges();
	m_pAutoAimCheckBox->ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: sets background color & border
//-----------------------------------------------------------------------------
void COptionsSubMouse::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

//	m_pMouseSensitivityLabel->SetFgColor(GetSchemeColor("WindowDisabledFgColor", GetFgColor()));
//	m_pMouseSensitivityLabel->SetBgColor(GetSchemeColor("WindowDisabledBgColor", GetBgColor()));
	m_pMouseSensitivityLabel->SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnControlModified(Panel *panel)
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));

    // the HasBeenModified() check is so that if the value is outside of the range of the
    // slider, it won't use the slider to determine the display value but leave the
    // real value that we determined in the constructor
    if (panel == m_pMouseSensitivitySlider && m_pMouseSensitivitySlider->HasBeenModified())
    {
        UpdateSensitivityLabel();
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnTextChanged(Panel *panel)
{
    if (panel == m_pMouseSensitivityLabel)
    {
        char buf[64];
        m_pMouseSensitivityLabel->GetText(buf, 64);

        float fValue = (float) atof(buf);
        if (fValue >= 1.0)
        {
            m_pMouseSensitivitySlider->SetSliderValue(fValue);
            PostActionSignal(new KeyValues("ApplyButtonEnable"));
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::UpdateSensitivityLabel()
{
    char buf[64];
    sprintf(buf, " %.1f", m_pMouseSensitivitySlider->GetSliderValue());
    m_pMouseSensitivityLabel->SetText(buf);
}

//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
MessageMapItem_t COptionsSubMouse::m_MessageMap[] =
{
	MAP_MESSAGE_PTR( COptionsSubMouse, "ControlModified", OnControlModified, "panel"),
	MAP_MESSAGE_PTR( COptionsSubMouse, "TextChanged", OnTextChanged, "panel"),
};

IMPLEMENT_PANELMAP( COptionsSubMouse, BaseClass );