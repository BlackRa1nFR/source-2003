//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "MatEditPanel.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMaterial.h"
#include "tier0/dbg.h"
#include "vgui_controls/Panel.h"
#include "VMTEdit.h"
#include "vgui/ISurface.h"
#include "KeyValues.h"
#include "vgui_controls/ScrollBar.h"
#include "materialsystem/IMesh.h"
#include "vguimatsurface/IMatSystemSurface.h"
#include "vgui_controls/Controls.h"
#include "vgui/IScheme.h"
#include "UtlSymbol.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/PanelListPanel.h"
#include "materialsystem/IMaterialVar.h"


using namespace vgui;

enum 
{
	SCROLLBAR_SIZE=18,  // the width of a scrollbar
	WINDOW_BORDER_WIDTH=2 // the width of the window's border
};

//-----------------------------------------------------------------------------
// Material Viewer Panel
//-----------------------------------------------------------------------------
class CMaterialViewPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
public:
	// constructor, destructor
	CMaterialViewPanel( vgui::Panel *pParent, const char *pName );
	virtual ~CMaterialViewPanel();

	// Set the material to draw
	void SetMaterial( IMaterial *pMaterial );

	// Set rendering mode (stretch to full screen, or use actual size)
	void RenderUsingActualSize( bool bEnable );

	// performs the layout
	virtual void PerformLayout();

	// paint it!
	virtual void Paint();

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );

private:
	// paint it stretched to the window size
	void DrawStretchedToPanel( CMeshBuilder &meshBuilder );

	// paint it actual size
	void DrawActualSize( CMeshBuilder &meshBuilder );

private:
	// The material to draw
	IMaterial *m_pMaterial;

	// Are we using actual size or not?
	bool m_bUseActualSize;

	// Scroll bars
	vgui::ScrollBar *m_pHorizontalBar;
	vgui::ScrollBar *m_pVerticalBar;

	// The viewable size
	int	m_iViewableWidth;
	int m_iViewableHeight;
};


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CMaterialViewPanel::CMaterialViewPanel( vgui::Panel *pParent, const char *pName ) :
	vgui::Panel( pParent, pName )
{
	m_bUseActualSize = true;
	m_pMaterial = NULL;

	m_pHorizontalBar = new ScrollBar(this, "HorizScrollBar", false);
	m_pHorizontalBar->AddActionSignalTarget(this);
	m_pHorizontalBar->SetVisible(true);

	m_pVerticalBar = new ScrollBar(this, "VertScrollBar", true);
	m_pVerticalBar->AddActionSignalTarget(this);
	m_pVerticalBar->SetVisible(true);

}

void CMaterialViewPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBorder( pScheme->GetBorder( "MenuBorder") );
}

CMaterialViewPanel::~CMaterialViewPanel()
{
	if (m_pMaterial)
	{
		m_pMaterial->DecrementReferenceCount();
	}
}


//-----------------------------------------------------------------------------
// Set the material to draw
//-----------------------------------------------------------------------------
void CMaterialViewPanel::SetMaterial( IMaterial *pMaterial )
{
	if (m_pMaterial)
	{
		m_pMaterial->DecrementReferenceCount();
	}
	m_pMaterial = pMaterial;
	if (m_pMaterial)
	{
		m_pMaterial->IncrementReferenceCount();
	}
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Set rendering mode (stretch to full screen, or use actual size)
//-----------------------------------------------------------------------------
void CMaterialViewPanel::RenderUsingActualSize( bool bEnable )
{
	m_bUseActualSize = bEnable;
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: relayouts out the panel after any internal changes
//-----------------------------------------------------------------------------
void CMaterialViewPanel::PerformLayout()
{
	// Get the current size, see if it's big enough to view the entire thing
	int iWidth, iHeight;
	GetSize( iWidth, iHeight );

	// In the case of stretching, just stretch to the size and blow off
	// the scrollbars. Same holds true if there's no material
	if (!m_bUseActualSize || !m_pMaterial)
	{
		m_iViewableWidth = iWidth;
		m_iViewableHeight = iHeight;
		m_pHorizontalBar->SetVisible(false);
		m_pVerticalBar->SetVisible(false);
		return;
	}

	// Check the size of the material...
	int iMaterialWidth = m_pMaterial->GetMappingWidth();
	int iMaterialHeight = m_pMaterial->GetMappingHeight();

	// Check if the scroll bars are visible
	bool bHorizScrollVisible = (iMaterialWidth > iWidth);
	bool bVertScrollVisible = (iMaterialHeight > iHeight);

	m_pHorizontalBar->SetVisible(bHorizScrollVisible);
	m_pVerticalBar->SetVisible(bVertScrollVisible);

	// Shrink the bars if both are visible
	m_iViewableWidth = bVertScrollVisible ? iWidth - SCROLLBAR_SIZE - WINDOW_BORDER_WIDTH : iWidth; 
	m_iViewableHeight = bHorizScrollVisible ? iHeight - SCROLLBAR_SIZE - WINDOW_BORDER_WIDTH : iHeight; 

	// Set the position of the horizontal bar...
	if (bHorizScrollVisible)
	{
		m_pHorizontalBar->SetPos(0, iHeight - SCROLLBAR_SIZE);
		m_pHorizontalBar->SetSize( m_iViewableWidth, SCROLLBAR_SIZE );

		m_pHorizontalBar->SetRangeWindow( m_iViewableWidth );
		m_pHorizontalBar->SetRange( 0, iMaterialWidth );	

		// FIXME: Change scroll amount based on how much is not visible?
		m_pHorizontalBar->SetButtonPressedScrollValue( 5 );
	}

	// Set the position of the vertical bar...
	if (bVertScrollVisible)
	{
		m_pVerticalBar->SetPos(iWidth - SCROLLBAR_SIZE, 0);
		m_pVerticalBar->SetSize(SCROLLBAR_SIZE, m_iViewableHeight);

		m_pVerticalBar->SetRangeWindow( m_iViewableHeight );
		m_pVerticalBar->SetRange( 0, iMaterialHeight);	
		m_pVerticalBar->SetButtonPressedScrollValue( 5 );
	}
}


//-----------------------------------------------------------------------------
// paint it stretched to the window size
//-----------------------------------------------------------------------------
void CMaterialViewPanel::DrawStretchedToPanel( CMeshBuilder &meshBuilder )
{
	// Draw a polygon the size of the panel
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( 0, 0, 0 );
	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( 0, m_iViewableHeight, 0 );
	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( m_iViewableWidth, m_iViewableHeight, 0 );
	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( m_iViewableWidth, 0, 0 );
	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.AdvanceVertex();
}


//-----------------------------------------------------------------------------
// paint it actual size
//-----------------------------------------------------------------------------
void CMaterialViewPanel::DrawActualSize( CMeshBuilder &meshBuilder )
{
	// Check the size of the material...
	int iMaterialWidth = m_pMaterial->GetMappingWidth();
	int iMaterialHeight = m_pMaterial->GetMappingHeight();

	Vector2D ul;
	Vector2D lr;
	Vector2D tul;
	Vector2D tlr;

	if (m_iViewableWidth >= iMaterialWidth)
	{
		// Center the material if we've got enough horizontal space
		ul.x = (m_iViewableWidth - iMaterialWidth) * 0.5f;
		lr.x = ul.x + iMaterialWidth;
		tul.x = 0.0f; tlr.x = 1.0f;
	}
	else
	{
		// Use the scrollbars here...
		int val = m_pHorizontalBar->GetValue();
		tul.x = (float)val / (float)iMaterialWidth;
		tlr.x = tul.x + (float)m_iViewableWidth / (float)iMaterialWidth;

		ul.x = 0;
		lr.x = m_iViewableWidth;
	}

	if (m_iViewableHeight >= iMaterialHeight)
	{
		// Center the material if we've got enough vertical space
		ul.y = (m_iViewableHeight - iMaterialHeight) * 0.5f;
		lr.y = ul.y + iMaterialHeight;
		tul.y = 0.0f; tlr.y = 1.0f;
	}
	else
	{
		// Use the scrollbars here...
		int val = m_pVerticalBar->GetValue();

		tul.y = (float)val / (float)iMaterialHeight;
		tlr.y = tul.y + (float)m_iViewableHeight / (float)iMaterialHeight;

		ul.y = 0;
		lr.y = m_iViewableHeight;
	}

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( ul.x, ul.y, 0 );
	meshBuilder.TexCoord2f( 0, tul.x, tul.y );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( lr.x, ul.y, 0 );
	meshBuilder.TexCoord2f( 0, tlr.x, tul.y );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( lr.x, lr.y, 0 );
	meshBuilder.TexCoord2f( 0, tlr.x, tlr.y );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.Position3f( ul.x, lr.y, 0 );
	meshBuilder.TexCoord2f( 0, tul.x, tlr.y );
	meshBuilder.AdvanceVertex();
}


//-----------------------------------------------------------------------------
// paint it!
//-----------------------------------------------------------------------------
void CMaterialViewPanel::Paint()
{
	// Draw a background (translucent objects will appear that way)

	// FIXME: Draw the outline of this panel?
	if (!m_pMaterial)
		return;

	g_pMatSystemSurface->Begin3DPaint( 0, 0, m_iViewableWidth, m_iViewableHeight );
	g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	g_pMaterialSystem->LoadIdentity();
	g_pMaterialSystem->Scale( 1, -1, 1 );
	g_pMaterialSystem->Ortho( 0, 0, m_iViewableWidth, m_iViewableHeight, 0, 1 );

	g_pMaterialSystem->Bind( m_pMaterial );
	IMesh *pMesh = g_pMaterialSystem->GetDynamicMesh();
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	if (!m_bUseActualSize)
	{
		DrawStretchedToPanel( meshBuilder );
	}
	else
	{
		DrawActualSize( meshBuilder );
	}

	meshBuilder.End();
	pMesh->Draw();

	g_pMatSystemSurface->End3DPaint( );
}
	    


class CMaterialEditorPanel;

//-----------------------------------------------------------------------------
// Mini combobox wrapper...
//-----------------------------------------------------------------------------
class CShaderComboBox : public vgui::ComboBox
{
	typedef vgui::ComboBox BaseClass;

public:
	CShaderComboBox(CMaterialEditorPanel *pPanel, const char *panelName, int numLines, bool allowEdit);
	virtual void OnCommand( const char *command );

private:
	CMaterialEditorPanel *m_pPanel;
};


//-----------------------------------------------------------------------------
// main editor panel
//-----------------------------------------------------------------------------
class CMaterialEditorPanel : public vgui::Panel
{
public:
	CMaterialEditorPanel( vgui::Panel *pParent );

	// performs the layout
	virtual void PerformLayout();

	// Creates a new material to edit
	void NewMaterial( );

	// Opens a particular material...
	void OpenMaterial( const char *pMaterialName );

private:
	// Sets up the shader listbox
	void PopulateShaderList();

	// Sets up the shader param editbox
	void PopulateShaderParameters();

	// Begin editing a particular material
	void BeginEditingMaterial( const char *pMaterialName, IMaterial *pMaterial );

	// Used to save dirty materials, returns false
	// if we should abort the operation that forced us to want to save
	// (like creating a new material, etc.)
	bool SaveDirtyMaterial();

	// Changes the shader associated with the current material
	void OnShaderSelected( const char *pShaderName );

private:
	// Child panels
	CMaterialViewPanel *m_pMaterialViewer;
	vgui::Label *m_pTitleLabel;
	vgui::ComboBox *m_pShaderList;
	vgui::PanelListPanel *m_pEditorPanel;

	// Name of the material being edited...
	CUtlSymbol m_EditedMaterial;

	// The shader index
	int	m_nShaderIndex;

	// Pointer to the material being edited...
	IMaterial *m_pMaterial;

	// Has the material been changed?
	bool m_bMaterialDirty;

	DECLARE_PANELMAP();
};


MessageMapItem_t CMaterialEditorPanel::m_MessageMap[] =
{
	MAP_MESSAGE_CONSTCHARPTR( CMaterialEditorPanel, "TextChanged", OnShaderSelected, "text" ), 
};

IMPLEMENT_PANELMAP( CMaterialEditorPanel, Panel );


void CreateMaterialEditorPanel( vgui::Panel *pParent )
{
	// add our main window
	vgui::Panel *pMaterialEditor = new CMaterialEditorPanel(pParent);
}


CShaderComboBox::CShaderComboBox(CMaterialEditorPanel *pPanel, const char *panelName, int numLines, bool allowEdit) :
	vgui::ComboBox( pPanel, panelName, numLines, allowEdit), m_pPanel(pPanel)
{
}

void CShaderComboBox::OnCommand( const char *command )
{
	BaseClass::OnCommand(command);
//	m_pPanel->OnShaderSelected( command );
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CMaterialEditorPanel::CMaterialEditorPanel(vgui::Panel *pParent) : 
	vgui::Panel(pParent, "Material Editor")
{
	m_bMaterialDirty = false;
	m_pMaterial = NULL;
	m_nShaderIndex = -1;

	m_pTitleLabel = new vgui::Label( this, "Material Viewer Title", "Untitled" );
	m_pTitleLabel->SetContentAlignment( Label::a_west );
	m_pTitleLabel->SetSize( 288, 16 );
	m_pTitleLabel->SetPos( 16, 2 );

	m_pMaterialViewer = new CMaterialViewPanel(this, "Material Viewer");
	m_pMaterialViewer->SetSize( 256, 256 );
	m_pMaterialViewer->SetPos( 16, 20 );

	PopulateShaderList();
	m_pShaderList->SetWide( 256 );	// Height is set by the constructor
	m_pShaderList->SetPos( 16, 278 );
	m_pShaderList->AddActionSignalTarget(this);

	m_pEditorPanel = new vgui::PanelListPanel( this, "Shader Parameter Editor" );
	m_pEditorPanel->SetSize( 256, 360 );
	m_pEditorPanel->SetPos( 16, 308 );
	m_pEditorPanel->AddActionSignalTarget(this);

	NewMaterial();
}


//-----------------------------------------------------------------------------
// Sets up the shader listbox
//-----------------------------------------------------------------------------
void CMaterialEditorPanel::PopulateShaderList()
{
	int iShaderCount = g_pMaterialSystem->GetNumShaders();
	m_pShaderList = new vgui::ComboBox( this, "Shader List", 12, false );

	for (int i = 0; i < iShaderCount; ++i )
	{
		m_pShaderList->AddItem( g_pMaterialSystem->GetShaderName(i), NULL ); 
	}

	m_pShaderList->SortItems();
	m_nShaderIndex = 0;
}


//-----------------------------------------------------------------------------
// Sets up the shader param editbox
//-----------------------------------------------------------------------------
void CMaterialEditorPanel::PopulateShaderParameters()
{
	Assert( m_nShaderIndex >= 0 );
	m_pEditorPanel->RemoveAll();
	int nCount = g_pMaterialSystem->GetNumShaderParams(m_nShaderIndex);
	for (int i = 0; i < nCount; ++i)
	{
		const char *pParamName = g_pMaterialSystem->GetShaderParamName(m_nShaderIndex, i);

		char tempBuf[512];
		Q_strncpy( tempBuf, pParamName, 512 );
		Q_strnlwr( tempBuf, 512 );

		// build a control & label (strip off '$' prefix)
		Label *pLabel = new Label( this, NULL, tempBuf + 1 ); 
		pLabel->SetSize(128, 24);
		
		// Set up the tooltip for this puppy...
		pLabel->SetTooltipText( g_pMaterialSystem->GetShaderParamHelp(m_nShaderIndex, i) );

		// Get at the material var
		bool bFound;
		IMaterialVar *pMaterialVar = m_pMaterial->FindVar( pParamName, &bFound );
		Assert( bFound );

		Panel *pEditPanel;
		switch( g_pMaterialSystem->GetShaderParamType(m_nShaderIndex, i) )
		{
		case SHADER_PARAM_TYPE_TEXTURE:
			pEditPanel = new Label( this, NULL, "texture" ); 
			break;

		case SHADER_PARAM_TYPE_INTEGER:
			{
				TextEntry *pTextEntry = new TextEntry(this, "Shader Param Integer");
				Q_snprintf( tempBuf, 512, "%d", pMaterialVar->GetIntValue() );
				pTextEntry->SetText( tempBuf );
				pTextEntry->SetEditable( true );

				pEditPanel = pTextEntry;
			}
			break;

		case SHADER_PARAM_TYPE_COLOR:
			pEditPanel = new Label( this, NULL, "color" ); 
			break;
		case SHADER_PARAM_TYPE_VEC2:
			pEditPanel = new Label( this, NULL, "vec2" ); 
			break;
		case SHADER_PARAM_TYPE_VEC3:
			pEditPanel = new Label( this, NULL, "vec3" ); 
			break;

		case SHADER_PARAM_TYPE_FLOAT:
			{
				TextEntry *pTextEntry = new TextEntry(this, "Shader Param Float");
				Q_snprintf( tempBuf, 512, "%f", pMaterialVar->GetIntValue() );
				pTextEntry->SetText( tempBuf );
				pTextEntry->SetEditable( true );

				pEditPanel = pTextEntry;
			}
			break;

		default:
			pEditPanel = new Label( this, NULL, "other" ); 
			break;
		}

		pEditPanel->SetSize(128, 24);
//		pEditPanel->SetContentAlignment( Label::a_east );
		m_pEditorPanel->AddItem( pLabel, pEditPanel );
	}
}


//-----------------------------------------------------------------------------
// Begin editing a particular material
//-----------------------------------------------------------------------------
void CMaterialEditorPanel::BeginEditingMaterial( const char *pMaterialName, IMaterial *pMaterial )
{
	m_bMaterialDirty = false;
	m_EditedMaterial = pMaterialName;
	m_pTitleLabel->SetText( m_EditedMaterial.String() );
	m_pMaterialViewer->SetMaterial(pMaterial);
	m_pMaterial = pMaterial;
}


//-----------------------------------------------------------------------------
// Creates a new material to edit
//-----------------------------------------------------------------------------
void CMaterialEditorPanel::NewMaterial( )
{
	if (!SaveDirtyMaterial())
		return;

	IMaterial *pMaterial = g_pMaterialSystem->CreateMaterial();
	BeginEditingMaterial( "Untitled", pMaterial );
}


//-----------------------------------------------------------------------------
// Opens a particular material...
//-----------------------------------------------------------------------------
void CMaterialEditorPanel::OpenMaterial( const char *pMaterialFileName )
{
	if (!SaveDirtyMaterial())
		return;

	// FIXME: Should we really be using 'FindMaterial' here?
	// Or just directly loading the material from the directory?
	bool bFound;
	IMaterial *pMaterial = g_pMaterialSystem->FindMaterial( pMaterialFileName, &bFound, false );
	if (!bFound)
	{
		// FIXME: Add a modal error message indicating the error

		return;
	}

	BeginEditingMaterial( pMaterialFileName, pMaterial );
}


//-----------------------------------------------------------------------------
// Used to save dirty materials, returns false
//-----------------------------------------------------------------------------
bool CMaterialEditorPanel::SaveDirtyMaterial()
{
	return true;
}


//-----------------------------------------------------------------------------
// Changes the shader associated with the current material
//-----------------------------------------------------------------------------
void CMaterialEditorPanel::OnShaderSelected( const char *pShaderName )
{
	Assert( m_pMaterial );
	m_pMaterial->SetShader( pShaderName );

	// Compute the shader number
	m_nShaderIndex = -1;
	for (int i = g_pMaterialSystem->GetNumShaders(); --i >= 0; )
	{
		if (!Q_strnicmp( pShaderName, g_pMaterialSystem->GetShaderName(i), 128 ))
		{
			m_nShaderIndex = i;
			break;
		}
	}
	Assert( m_nShaderIndex >= 0 );

	PopulateShaderParameters();

	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// performs the layout
//-----------------------------------------------------------------------------
void CMaterialEditorPanel::PerformLayout()
{
	// Make the editor panel fill the space
	int iWidth, iHeight;
	GetParent()->GetSize( iWidth, iHeight );
	SetSize( 288, iHeight );
}


