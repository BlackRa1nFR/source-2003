#include "OptionsSubVideo.h"
#include "CvarSlider.h"
#include "EngineInterface.h"
#include "igameuifuncs.h"
#include "modes.h"

#include <vgui_controls/CheckButton.h>
#include <vgui_controls/ComboBox.h>
#include <KeyValues.h>
#include <vgui/ILocalize.h>
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;
	
inline bool IsWideScreen ( int width, int height )
{
	// 16:9 or 16:10 is widescreen :)
	if ( (width * 9) == ( height * 16.0f ) || (width * 5.0) == ( height * 8.0 ))
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubVideo::COptionsSubVideo(vgui::Panel *parent) : PropertyPage(parent, NULL)
{
	memset( &m_OrigSettings, 0, sizeof( m_OrigSettings ) );
	memset( &m_CurrentSettings, 0, sizeof( m_CurrentSettings ) );

	m_pBrightnessSlider = new CCvarSlider( this, "Brightness", "#GameUI_Brightness",
		0.0f, 2.0f, "brightness" );

	m_pGammaSlider = new CCvarSlider( this, "Gamma", "#GameUI_Gamma",
		1.0f, 3.0f, "gamma" );

	GetVidSettings();

	m_pMode = new ComboBox(this, "Resolution", 6, false);

	m_pAspectRatio = new ComboBox( this, "AspectRatio", 2, false );

	wchar_t *unicodeText = localize()->Find("#GameUI_AspectNormal");
    localize()->ConvertUnicodeToANSI(unicodeText, m_pszAspectName[0], 32);
    unicodeText = localize()->Find("#GameUI_AspectWide");
    localize()->ConvertUnicodeToANSI(unicodeText, m_pszAspectName[1], 32);

	int iNormalItemID = m_pAspectRatio->AddItem( m_pszAspectName[0], NULL );
	int iWideItemID = m_pAspectRatio->AddItem( m_pszAspectName[1], NULL );
		
	m_bStartWidescreen = IsWideScreen( m_CurrentSettings.w, m_CurrentSettings.h );
	if ( m_bStartWidescreen )
	{
		m_pAspectRatio->ActivateItem( iWideItemID );
	}
	else
	{
		m_pAspectRatio->ActivateItem( iNormalItemID );
	}

	PrepareResolutionList();

    // load up the renderer display names
    unicodeText = localize()->Find("#GameUI_Software");
    localize()->ConvertUnicodeToANSI(unicodeText, m_pszRenderNames[0], 32);
    unicodeText = localize()->Find("#GameUI_OpenGL");
    localize()->ConvertUnicodeToANSI(unicodeText, m_pszRenderNames[1], 32);
    unicodeText = localize()->Find("#GameUI_D3D");
    localize()->ConvertUnicodeToANSI(unicodeText, m_pszRenderNames[2], 32);

	m_pRenderer = new ComboBox( this, "Renderer", 3, false ); // "#GameUI_Renderer"
	int i;
    for ( i = 0; i < 3; i++)
    {
        m_pRenderer->AddItem( m_pszRenderNames[i], NULL );
    }

	m_pColorDepth = new ComboBox( this, "ColorDepth", 2, false );
	m_pColorDepth->AddItem("Medium (16 bit)", NULL);
	m_pColorDepth->AddItem("Highest (32 bit)", NULL);

    SetCurrentRendererComboItem();

	m_pWindowed = new vgui::CheckButton( this, "Windowed", "#GameUI_Windowed" );
	m_pWindowed->SetSelected( m_CurrentSettings.windowed ? true : false);

	LoadControlSettings("Resource\\OptionsSubVideo.res");
}

void COptionsSubVideo::PrepareResolutionList( void )
{
	vmode_t *plist = NULL;
	int count = 0;

	gameuifuncs->GetVideoModes( &plist, &count );

	// Clean up before filling the info again.
	m_pMode->DeleteAllItems();

	int selectedItemID = -1;
	for (int i = 0; i < count; i++, plist++)
	{
		char sz[ 256 ];
		sprintf( sz, "%i x %i", plist->width, plist->height );

		int itemID = -1;
		if ( IsWideScreen( plist->width, plist->height ) )
		{
			if ( m_bStartWidescreen == true )
			{
				 itemID = m_pMode->AddItem( sz, NULL );
			}
		}
		else
		{
			if ( m_bStartWidescreen == false )
			{
				 itemID = m_pMode->AddItem( sz, NULL);
			}
		}
		if ( plist->width == m_CurrentSettings.w && 
			 plist->height == m_CurrentSettings.h )
		{
			selectedItemID = itemID;
		}
	}

	if ( selectedItemID != -1 )
	{
		m_pMode->ActivateItem( selectedItemID );
	}
	else
	{
		// just activate the first item
		m_pMode->ActivateItem( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubVideo::~COptionsSubVideo()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVideo::OnResetData()
{
    // reset data
	RevertVidSettings();

    // reset UI elements
	m_pBrightnessSlider->Reset();
	m_pGammaSlider->Reset();
    m_pWindowed->SetSelected(m_CurrentSettings.windowed);

    SetCurrentRendererComboItem();
    SetCurrentResolutionComboItem();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVideo::SetCurrentRendererComboItem()
{
	if ( !stricmp( m_CurrentSettings.renderer, "software" ) )
	{
        m_iStartRenderer = 0;
	}
	else if ( !stricmp( m_CurrentSettings.renderer, "gl" ) )
	{
        m_iStartRenderer = 1;
	}
	else if ( !stricmp( m_CurrentSettings.renderer, "d3d" ) )
	{
        m_iStartRenderer = 2;
	}
	else
	{
		// opengl by default
        m_iStartRenderer = 1;
	}
    m_pRenderer->ActivateItemByRow( m_iStartRenderer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVideo::SetCurrentResolutionComboItem()
{
	vmode_t *plist = NULL;
	int count = 0;
	gameuifuncs->GetVideoModes( &plist, &count );

    int resolution = -1;
    for ( int i = 0; i < count; i++, plist++ )
	{
		if ( plist->width == m_CurrentSettings.w && 
			 plist->height == m_CurrentSettings.h )
		{
            resolution = i;
			break;
		}
	}

    if (resolution != -1)
	{
		char sz[256];
		sprintf(sz, "%i x %i", plist->width, plist->height);
        m_pMode->SetText(sz);
	}

	if (m_CurrentSettings.bpp > 16)
	{
		m_pColorDepth->ActivateItemByRow(1);
	}
	else
	{
		m_pColorDepth->ActivateItemByRow(0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVideo::OnApplyChanges()
{
    bool bChanged = m_pBrightnessSlider->HasBeenModified() || m_pGammaSlider->HasBeenModified();

	m_pBrightnessSlider->ApplyChanges();
	m_pGammaSlider->ApplyChanges();

	ApplyVidSettings(bChanged);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVideo::GetVidSettings()
{
	// Get original settings
	CVidSettings *p = &m_OrigSettings;

	gameuifuncs->GetCurrentVideoMode( &p->w, &p->h, &p->bpp );
	gameuifuncs->GetCurrentRenderer( p->renderer, 128, &p->windowed );
	strlwr( p->renderer );

	m_CurrentSettings = m_OrigSettings;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVideo::RevertVidSettings()
{
	m_CurrentSettings = m_OrigSettings;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVideo::ApplyVidSettings(bool bForceRefresh)
{
	// Retrieve text from active controls and parse out strings
	if ( m_pMode )
	{
		char sz[256], colorDepth[256];
		m_pMode->GetText(sz, 256);
		m_pColorDepth->GetText(colorDepth, sizeof(colorDepth));

		int w, h;
		sscanf( sz, "%i x %i", &w, &h );
		m_CurrentSettings.w = w;
		m_CurrentSettings.h = h;
		if (strstr(colorDepth, "32"))
		{
			m_CurrentSettings.bpp = 32;
		}
		else
		{
			m_CurrentSettings.bpp = 16;
		}
	}

	if ( m_pRenderer )
	{
		char sz[ 256 ];
		m_pRenderer->GetText(sz, sizeof(sz));

		if ( !stricmp( sz, m_pszRenderNames[0] ) )
		{
			strcpy( m_CurrentSettings.renderer, "software" );
		}
		else if ( !stricmp( sz, m_pszRenderNames[1] ) )
		{
			strcpy( m_CurrentSettings.renderer, "gl" );
		}
		else if ( !stricmp( sz, m_pszRenderNames[2] ) )
		{
			strcpy( m_CurrentSettings.renderer, "d3d" );
		}
	}

	if ( m_pWindowed )
	{
		bool checked = m_pWindowed->IsSelected();
		m_CurrentSettings.windowed = checked ? 1 : 0;
	}

	if ( memcmp( &m_OrigSettings, &m_CurrentSettings, sizeof( CVidSettings ) ) == 0 && !bForceRefresh)
	{
		return;
	}

	CVidSettings *p = &m_CurrentSettings;

	char szCmd[ 256 ];
	// Set mode
	sprintf( szCmd, "_setvideomode %i %i %i\n", p->w, p->h, p->bpp );
	engine->ClientCmd( szCmd );

	// Set renderer
	sprintf( szCmd, "_setrenderer %s %s\n", p->renderer, p->windowed ? "windowed" : "fullscreen" ); // GoldSrc string
	//sprintf( szCmd, "_setrenderer %s\n", /*p->renderer, */p->windowed ? "windowed" : "fullscreen" ); // SRC string

	engine->ClientCmd( szCmd );

	// Force restart of entire engine
	engine->ClientCmd( "_restart\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVideo::OnButtonChecked(KeyValues *data)
{
	int state = data->GetInt("state");
	Panel* pPanel = (Panel*) data->GetPtr("panel", NULL);

	if (pPanel == m_pWindowed)
	{
		if (state != m_CurrentSettings.windowed)
		{
            OnDataChanged();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVideo::OnTextChanged(Panel *pPanel, const char *pszText)
{
	if (pPanel == m_pMode)
    {
		char sz[ 256 ];
		sprintf(sz, "%i x %i", m_CurrentSettings.w, m_CurrentSettings.h);

        if (strcmp(pszText, sz))
        {
            OnDataChanged();
        }
    }
    else if (pPanel == m_pRenderer)
    {
        if (strcmp(pszText, m_pszRenderNames[m_iStartRenderer]))
        {
            OnDataChanged();
        }
    }
	else if (pPanel == m_pAspectRatio )
    {
		if ( strcmp(pszText, m_pszAspectName[m_bStartWidescreen] ) )
		{
			m_bStartWidescreen = !m_bStartWidescreen;
			PrepareResolutionList();
		}
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVideo::OnDataChanged()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t COptionsSubVideo::m_MessageMap[] =
{
	MAP_MESSAGE_PTR_CONSTCHARPTR( COptionsSubVideo, "TextChanged", OnTextChanged, "panel", "text" ),
	//MAP_MESSAGE( COptionsSubVideo, "SliderMoved", OnDataChanged),
	MAP_MESSAGE_PARAMS( COptionsSubVideo, "CheckButtonChecked", OnButtonChecked),
	MAP_MESSAGE( COptionsSubVideo, "ControlModified", OnDataChanged),
};

IMPLEMENT_PANELMAP(COptionsSubVideo, BaseClass);
