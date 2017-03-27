//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMaterial.h"
#include "tier0/dbg.h"
#include "vgui_controls/Panel.h"
#include "VMTEdit.h"
#include "vgui_controls/MenuBar.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Menu.h"
#include "KeyValues.h"
#include "MatEditPanel.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Maintains various important configuration state
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// main editor panel
//-----------------------------------------------------------------------------
class CVMTEditPanel : public vgui::Panel
{
public:
	CVMTEditPanel();

	// Resize this panel to match its parent
	virtual void PerformLayout();

private:
	vgui::MenuBar *m_pMenuBar;

	// FIXME: Is there a better way?
	// A panel that represents all area under the menu bar
	vgui::Panel *m_pClientArea;

	// Maintains various important configuration state
//	CUtlSymbol	m_RootPath;
//	CUtlSymbol	m_ModPath;
};

vgui::Panel *CreateVMTEditPanel()
{
	// add our main window
	vgui::Panel *pVMTEdit = new CVMTEditPanel;
	g_pVGuiSurface->CreatePopup(pVMTEdit->GetVPanel(), false);
	return pVMTEdit;
}


CVMTEditPanel::CVMTEditPanel()  : vgui::Panel(NULL, "VMTEdit")
{
	m_pMenuBar = new vgui::MenuBar( this, "Main Menu Bar" );
	m_pMenuBar->SetSize( 10, 28 );

	// Next create a menu
	Menu *pMenu = new Menu(NULL, "File Menu");
	pMenu->AddMenuItem("&New", new KeyValues ("OnNew"), this);
	pMenu->AddMenuItem("&Open", new KeyValues ("OnOpen"), this);
	pMenu->AddMenuItem("&Save", new KeyValues ("OnSave"), this);
	pMenu->AddMenuItem("Save &As", new KeyValues ("OnSaveAs"), this);
	pMenu->AddMenuItem("E&xit", new KeyValues ("OnExit"), this);

//	// Make the menu a popup window
//	pMenu->MakePopup();

	m_pMenuBar->AddMenu( "&File", pMenu );

	m_pClientArea = new vgui::Panel( this, "VMTEdit Client Area ");
	CreateMaterialEditorPanel( m_pClientArea );
}


//-----------------------------------------------------------------------------
// The editor panel should always fill the space...
//-----------------------------------------------------------------------------
void CVMTEditPanel::PerformLayout()
{
	// Make the editor panel fill the space
	int iWidth, iHeight;
	GetParent()->GetSize( iWidth, iHeight );
	SetSize( iWidth, iHeight );

	// Make the client area also fill the space not used by the menu bar
	int iTemp, iMenuHeight;
	m_pMenuBar->GetSize( iTemp, iMenuHeight );
	m_pClientArea->SetPos( 0, iMenuHeight );
	m_pClientArea->SetSize( iWidth, iHeight - iMenuHeight );
}



#if 0									  

MatEditPanel *g_MatEditPanel;

#define LABEL_WIDTH 200
#define EDIT_WIDTH 300
#define FIELD_HEIGHT 20
#define FIELD_SPACING 10
#define SHADER_CHOICE_HEIGHT 100
#define BROWSE_BUTTON_WIDTH 50

MatEditPanel::MatEditPanel( mxWindow *parent )
: mxWindow (parent, 0, 0, 0, 0, "MatEditPanel", mxWindow::Normal)
{
#if 0
	mxButton *button = new mxButton( this, 0, 0, 100, 50, "button", IDC_BUTTON );
	mxChoice *choice = new mxChoice( this, 100, 0, 100, 50, IDC_CHOICE );
	choice->add( "a" );
	choice->add( "b" );
	choice->add( "c" );
	mxGroupBox *group = new mxGroupBox( this, 200, 0, 100, 50, "group" );
	mxLabel *label = new mxLabel( this, 300, 0, 100, 50, "label" );
	mxLineEdit *lineEdit = new mxLineEdit( this, 400, 0, 100, 50, "lineEdit" );
	mxRadioButton *radioButton = new mxRadioButton( this, 100, 200, 100, 100, "radioButton", true );
	mxRadioButton *radioButton2 = new mxRadioButton( this, 200, 200, 100, 100, "radioButton2" );
	mxSlider *sliderHor = new mxSlider( this, 300, 200, 100, 100, 0, mxSlider::Horizontal );
	mxSlider *sliderVert = new mxSlider( this, 400, 200, 100, 100, 0, mxSlider::Vertical );
	mxTab *tab = new mxTab( this, 500, 200, 100, 100 );
	tab->add( new mxButton( this, 0, 0, 50, 50, "subbutton" ), "one" );
	tab->add( new mxButton( this, 0, 0, 50, 50, "subbutton2" ), "two" );
	mxToggleButton *toggle = new mxToggleButton( this, 0, 300, 100, 100, "toggle" );
	m_TreeView = new mxTreeView( this, 100, 300, 100, 100 );
	mxTreeViewItem *treeItem1 = m_TreeView->add( NULL, "root" );
	mxTreeViewItem *treeItem2 = m_TreeView->add( treeItem1, "child" );
	mxTreeViewItem *treeItem3 = m_TreeView->add( NULL, "root2" );
#endif
	m_NumShaderParamWidgets = 0;
//	m_MaterialTreeView = new mxTreeView( this, 0, 0, 300, 200 );
	MatSysWindow *matSysWindow = new MatSysWindow( this, 0, 0, 320, 240, "", mxWindow::Normal );
	m_ShaderParamsWindow = new mxWindow( this, 0, 240, 500, 500 );
	mxLabel *label = new mxLabel( m_ShaderParamsWindow, 0, 0, LABEL_WIDTH, FIELD_HEIGHT, "Shader" );
	m_ShaderChoice = new mxChoice( m_ShaderParamsWindow, LABEL_WIDTH, 0, EDIT_WIDTH, SHADER_CHOICE_HEIGHT, IDC_SHADERLISTBOX );

//	BuildMaterialTree();

	int numShaders = g_pMaterialSystem->GetNumShaders();

	int i;
	for( i = 0; i < numShaders; i++ )
	{
		m_ShaderChoice->add( g_pMaterialSystem->GetShaderName( i ) );
	}
	m_CurrentShaderID = 0;
	m_ShaderChoice->select( m_CurrentShaderID );
	UpdateShaderParamsWindow();

	g_MatEditPanel = this;
}

static void FixSlashes( char *str )
{
	while( *str )
	{
		if( *str == '\\' )
		{
			*str = '/';
		}
		str++;
	}
}

static mxTreeViewItem *TreeAddUnique( mxTreeView *pTree, mxTreeViewItem *parent, 
						   const char *item )
{
	mxTreeViewItem *child;
	child = pTree->getFirstChild( parent );
	while( child )
	{
		const char *label = pTree->getLabel( child );
		if( stricmp( label, item ) == 0 )
		{
			return child;
		}
		child = pTree->getNextChild( child );
	}
	// doh!  didn't find it, so add it.
	return pTree->add( parent, item );
}

static void TreeCreatePathedFile( char *pName, mxTreeView *pTree )
{
	char *buf;
	buf = ( char * )_alloca( strlen( pName ) + 1 );
	char *s;
	char *end;
	int len;
	mxTreeViewItem *parentTreeItem = NULL;
	
	strcpy( buf, pName );
	
	len = strlen( buf );
	end = buf + strlen( buf );
	s = buf;
	while( s < end )
    {
		if( *s == '/' )
        {
			*s = '\0';
//			parentTreeItem = pTree->add( parentTreeItem, buf );
			parentTreeItem = TreeAddUnique( pTree, parentTreeItem, buf );
			buf = s + 1;
        }
		s++;
    }
	TreeAddUnique( pTree, parentTreeItem, buf );
//	pTree->add( parentTreeItem, buf );
}

// FIXME: should subclass mxTree to know how to deal with directory hierarchies
// when things are added.
void MatEditPanel::AddToMaterialTree( const char *pName )
{
	char *tmpString = ( char * )_alloca( strlen( pName ) + 1 );
	strcpy( tmpString, pName );
	FixSlashes( tmpString );
//	TreeCreatePathedFile( tmpString, m_MaterialTreeView );
}

void MatEditPanel::BuildMaterialTree()
{
//	m_MaterialTreeView->removeAll();
//	m_MaterialTreeView->remove( 0 ); // hack: should fix mxtk
	MaterialHandle_t matHandle;
	matHandle = g_pMaterialSystem->FirstMaterial();
	IMaterial *pMat = g_pMaterialSystem->GetMaterial( matHandle );
	while( pMat )
	{
		AddToMaterialTree( pMat->GetName() );
		matHandle = g_pMaterialSystem->NextMaterial( matHandle );
		pMat = g_pMaterialSystem->GetMaterial( matHandle );
	}
}

static const char *ParamTypeToName( ShaderParamType_t type )
{
	switch( type )
	{
	case SHADER_PARAM_TYPE_TEXTURE:
		return "SHADER_PARAM_TYPE_TEXTURE";
	case SHADER_PARAM_TYPE_INTEGER:
		return "SHADER_PARAM_TYPE_INTEGER";
	case SHADER_PARAM_TYPE_COLOR:
		return "SHADER_PARAM_TYPE_COLOR";
	case SHADER_PARAM_TYPE_VEC3:
		return "SHADER_PARAM_TYPE_VEC3";
	case SHADER_PARAM_TYPE_ENVMAP:
		return "SHADER_PARAM_TYPE_ENVMAP";
	case SHADER_PARAM_TYPE_FLOAT:
		return "SHADER_PARAM_TYPE_FLOAT";
	default:
		return "UNKNOWN PARAM TYPE!";
	}
}

void MatEditPanel::UpdateShaderParamsWindow()
{
	int i;
	for( i = 0; i < m_NumShaderParamWidgets; i++ )
	{
		delete m_ShaderParamWidgets[i];
	}
	m_NumShaderParamWidgets = 0;
	int numParams;
	numParams = g_pMaterialSystem->GetNumShaderParams( m_CurrentShaderID );
	int offset = FIELD_HEIGHT + FIELD_SPACING;
	for( i = 0; i < numParams; i++ )
	{
		const char *paramName = g_pMaterialSystem->GetShaderParamName( m_CurrentShaderID, i );
		paramName++; // skip the $
		const char *paramDefault = g_pMaterialSystem->GetShaderParamDefault( m_CurrentShaderID, i );
		const char *paramHelp = g_pMaterialSystem->GetShaderParamHelp( m_CurrentShaderID, i );
		ShaderParamType_t paramType = g_pMaterialSystem->GetShaderParamType( m_CurrentShaderID, i );
		mxLabel *label = new mxLabel( m_ShaderParamsWindow, 0, offset, LABEL_WIDTH, FIELD_HEIGHT, paramName );
		m_ShaderParamWidgets[m_NumShaderParamWidgets++] = label;
		switch( paramType )
		{
		case SHADER_PARAM_TYPE_TEXTURE:
		case SHADER_PARAM_TYPE_ENVMAP:
#if 0
			{
				mxLineEdit *lineEdit = new mxLineEdit( m_ShaderParamsWindow, LABEL_WIDTH, offset, EDIT_WIDTH, FIELD_HEIGHT, paramDefault );
				m_ShaderParamWidgets[m_NumShaderParamWidgets++] = lineEdit;
				mxButton *button = new mxButton( m_ShaderParamsWindow, LABEL_WIDTH + EDIT_WIDTH, offset, BROWSE_BUTTON_WIDTH, FIELD_HEIGHT, "Browse", IDC_SHADERPARAM + i );
				m_ShaderParamWidgets[m_NumShaderParamWidgets++] = button;
				mxToolTip::add( lineEdit, paramHelp );
				char str[256];
				sprintf( str, "Browse for %s", paramName );
				mxToolTip::add( button, str );
			}
			break;
#endif
		case SHADER_PARAM_TYPE_INTEGER:
		case SHADER_PARAM_TYPE_COLOR:
		case SHADER_PARAM_TYPE_VEC3:
		case SHADER_PARAM_TYPE_FLOAT:
		default:
			{
				mxLineEdit *lineEdit = new mxLineEdit( m_ShaderParamsWindow, LABEL_WIDTH, offset, EDIT_WIDTH, FIELD_HEIGHT, paramDefault, IDC_SHADERPARAM + i );
				m_ShaderParamWidgets[m_NumShaderParamWidgets++] = lineEdit;
				mxToolTip::add( lineEdit, paramHelp );
#if 0
				mxSlider *slider = new mxSlider( m_ShaderParamsWindow, LABEL_WIDTH, offset, EDIT_WIDTH, FIELD_HEIGHT, 0, mxSlider::Horizontal );
				m_ShaderParamWidgets[m_NumShaderParamWidgets++] = slider;
#endif
			}
			break;
#if 0
		default:
			assert( 0 );
			break;
#endif
		}
		offset += FIELD_HEIGHT + FIELD_SPACING;
	}
}

MatEditPanel::~MatEditPanel()
{
}

int MatEditPanel::handleEvent (mxEvent *event)
{
	switch (event->action)
	{
	case IDC_SHADERLISTBOX:
		m_CurrentShaderID = ((mxChoice *)event->widget)->getSelectedIndex();
		UpdateShaderParamsWindow();
#if 0
		const char *selectedText = ((mxListBox *)event->widget)->getItemText( index );
		m_TreeView->add( NULL, selectedText );
#endif
		break;
	}

	if( event->action >= IDC_SHADERPARAM && event->action < IDC_SHADERPARAM + MAX_PARAMS )
	{
//		int r, g, b;
		// get the type of the param that we are editing.
		int paramID = event->action - IDC_SHADERPARAM;
		ShaderParamType_t paramType;
		paramType = g_pMaterialSystem->GetShaderParamType( m_CurrentShaderID, paramID );
//		OutputDebugString( "got one!\n" );
		mxLineEdit *lineEdit = ( ( mxLineEdit * )event->widget);
		if( lineEdit )
		{
#if 0
			char buf[500];
			int len;
			// fixme
			len = lineEdit->getText( buf, 500 );
			OutputDebugString( buf );
			OutputDebugString( "\n" );
#endif
		}

#if 0
		switch( paramType )
		{
		case SHADER_PARAM_TYPE_TEXTURE:
//			mxGetOpenFileName( m_ShaderParamsWindow, NULL, "*.tga" );
			break;
		case SHADER_PARAM_TYPE_COLOR:
//			mxChooseColor( m_ShaderParamsWindow, &r, &g, &b );
			break;
		default:
			break;
		}
#endif
	}

	return 1;
}


#endif