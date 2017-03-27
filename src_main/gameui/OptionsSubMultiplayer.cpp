//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <windows.h> // SRC only!!

#include "OptionsSubMultiplayer.h"
#include "MultiplayerAdvancedDialog.h"
#include <stdio.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <KeyValues.h>
#include <vgui_controls/Label.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui/IVgui.h>


#include "CvarTextEntry.h"
#include "CvarToggleCheckButton.h"
#include "CvarSlider.h"
#include "LabeledCommandComboBox.h"
#include "FileSystem.h"
#include "EngineInterface.h"
#include "BitmapImagePanel.h"
#include "UtlBuffer.h"
#include "ModInfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;


#define DEFAULT_SUIT_HUE 30
#define DEFAULT_PLATE_HUE 6

void UpdateLogoWAD( void *hdib, int r, int g, int b );

struct ColorItem_t
{
	char		*name;
	int			r, g, b;
};

static ColorItem_t itemlist[]=
{
	{ "Orange", 255, 120, 24 },
	{ "Yellow", 225, 180, 24 },
	{ "Blue", 0, 60, 255 },
	{ "Ltblue", 0, 167, 255 },
	{ "Green", 0, 167, 0 },
	{ "Red", 255, 43, 0 },
	{ "Brown", 123, 73, 0 },
	{ "Ltgray", 100, 100, 100 },
	{ "Dkgray", 36, 36, 36 },
};

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
COptionsSubMultiplayer::COptionsSubMultiplayer(vgui::Panel *parent) : vgui::PropertyPage(parent, "OptionsSubMultiplayer") 
{
	Button *cancel = new Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	Button *ok = new Button( this, "OK", "#GameUI_OK" );
	ok->SetCommand( "Ok" );

	Button *apply = new Button( this, "Apply", "#GameUI_Apply" );
	apply->SetCommand( "Apply" );

	Button *advanced = new Button( this, "Advanced", "#GameUI_AdvancedEllipsis" );
	advanced->SetCommand( "Advanced" );

	new Label( this, "NameLabel", "#GameUI_PlayerName" );
	m_pNameTextEntry = new CCvarTextEntry( this, "NameEntry", "name" );

	m_pPrimaryColorSlider = new CCvarSlider( this, "Primary Color Slider", "#GameUI_PrimaryColor",
		0.0f, 255.0f, "topcolor" );

	m_pSecondaryColorSlider = new CCvarSlider( this, "Secondary Color Slider", "#GameUI_SecondaryColor",
		0.0f, 255.0f, "bottomcolor" );

	m_pHighQualityModelCheckBox = new CCvarToggleCheckButton( this, "High Quality Models", "#GameUI_HighModels", "cl_himodels" );

	m_pModelList = new CLabeledCommandComboBox( this, "Player model" );
	InitModelList( m_pModelList );

	m_pLogoList = new CLabeledCommandComboBox( this, "SpraypaintList" );
    m_LogoName[0] = 0;
	InitLogoList( m_pLogoList );

	m_pColorList = new CLabeledCommandComboBox( this, "SpraypaintColor" );

//	char const *currentcolor = engine->pfnGetCvarString( "cl_logocolor" );
	char const *currentcolor = NULL;
	ConVar *var = (ConVar *)cvar->FindVar( "cl_logocolor" );
	if ( var )
	{
		currentcolor = var->GetString();
	}

	int count = sizeof( itemlist ) / sizeof( itemlist[0] );
	int selected = 0;
	for ( int i = 0; i < count; i++ )
	{
		if ( currentcolor && !stricmp( currentcolor, itemlist[ i ].name ) )
		{
			selected = i;
		}
		char command[ 256 ];
		sprintf( command, "cl_logocolor %s\n", itemlist[ i ].name );
		m_pColorList->AddItem( itemlist[ i ].name, command );
	}
	m_pColorList->SetInitialItem( selected );
	m_pColorList->AddActionSignalTarget( this );

	m_pModelImage = new CBitmapImagePanel( this, "ModelImage", NULL );
	m_pModelImage->AddActionSignalTarget( this );

	m_pLogoImage = new CBitmapImagePanel( this, "LogoImage", NULL );
	m_pLogoImage->AddActionSignalTarget( this );

	m_nTopColor = DEFAULT_SUIT_HUE;
	m_nBottomColor = DEFAULT_PLATE_HUE;
	
	m_nLogoR = 255;
	m_nLogoG = 255;
	m_nLogoB = 255;

	LoadControlSettings("Resource/OptionsSubMultiplayer.res");
	
	// turn off model selection stuff if the mod specifies "nomodels" in the liblist.gam file
	if ( ModInfo().NoModels() )
	{
		Panel *pTempPanel = NULL;

		if ( m_pModelImage )
		{
			m_pModelImage->SetVisible( false );
		}

		if ( m_pModelList )
		{
			m_pModelList->SetVisible( false );
		}

		if ( m_pPrimaryColorSlider )
		{
			m_pPrimaryColorSlider->SetVisible( false );
		}

		if ( m_pSecondaryColorSlider )
		{
			m_pSecondaryColorSlider->SetVisible( false );
		}

		// #GameUI_PlayerModel (from "Resource/OptionsSubMultiplayer.res")
		pTempPanel = FindChildByName( "Label1" );

		if ( pTempPanel )
		{
			pTempPanel->SetVisible( false );
		}

		// #GameUI_ColorSliders (from "Resource/OptionsSubMultiplayer.res")
		pTempPanel = FindChildByName( "Colors" );

		if ( pTempPanel )
		{
			pTempPanel->SetVisible( false );
		}
	}

	// turn off the himodel stuff if the mod specifies "nohimodel" in the liblist.gam file
	if ( ModInfo().NoHiModel() )
	{
		if ( m_pHighQualityModelCheckBox )
		{
			m_pHighQualityModelCheckBox->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubMultiplayer::~COptionsSubMultiplayer()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnCommand( const char *command )
{
	if ( !stricmp( command, "Advanced" ) )
	{
		if (!m_hMultiplayerAdvancedDialog.Get())
		{
			m_hMultiplayerAdvancedDialog = new CMultiplayerAdvancedDialog(this);
		}
		m_hMultiplayerAdvancedDialog->Activate();
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: Builds the list of logos
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::InitLogoList( CLabeledCommandComboBox *cb )
{
/*	// cleanup old remap files
	filesystem()->RemoveFile( "logos/remapped.bmp", NULL );

	// Find out images
	FileFindHandle_t fh;
	char directory[ 512 ];

	const char *logofile = engine->pfnGetCvarString("cl_logofile");
	sprintf( directory, "logos/*.bmp" );
	const char *fn = filesystem()->FindFirst( directory, &fh );
	int i = 0, initialItem = 0; 
	while (fn)
	{
		if (stricmp(fn, "remapped.bmp"))
		{
			if (fn[0] && fn[0] != '.')
			{
				// Strip the .bmp
				char filename[ 512 ];
				strcpy( filename, fn );
				if ( strlen( filename ) >= 4 )
				{
					filename[ strlen( filename ) - 4 ] = 0;
				}

				// check to see if this is the one we have set
				if (!stricmp(filename, logofile))
				{
					initialItem = i;
				}

				cb->AddItem( filename, "" );

				// get the first filename
				if ( m_LogoName[0] == 0)
				{
					strcpy( m_LogoName, filename );
				}
			}
			i++;
		}
		fn = filesystem()->FindNext( fh );
	}

	filesystem()->FindClose( fh );
	cb->SetInitialItem(initialItem);
	*/
		
	// Find out images
	FileFindHandle_t fh;
	char directory[ 512 ];

	sprintf( directory, "logos/*.bmp" );

	char const *fn = filesystem()->FindFirst( directory, &fh );
	bool AddedItem = false;

	while ( fn )
	{
		//
		if ( fn[ 0 ] && fn[0] != '.' )
		{
			// Strip the .bmp
			char filename[ 512 ];
			strcpy( filename, fn );
			if ( strlen( filename ) >= 4 )
			{
				filename[ strlen( filename ) - 4 ] = 0;
			}

			cb->AddItem( filename, "" );
			AddedItem = true;
		}
		fn = filesystem()->FindNext( fh );
	}

	filesystem()->FindClose( fh );

	// FIXME:  Retrieve from somewhere
	if ( AddedItem )
	{
		cb->ActivateItem( 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Builds model list
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::InitModelList( CLabeledCommandComboBox *cb )
{
	// cleanup old remap files
	filesystem()->RemoveFile( "models/player/remapped.bmp", NULL );

	// Find out images
	FileFindHandle_t fh;
	char directory[ 512 ];
	char cmdstring[ 256 ];
	char currentmodel[ 64 ];

	sprintf( directory, "models/player/*.mdl" );

	char const *fn = filesystem()->FindFirst( directory, &fh );

	int c = 0;
	int selected = 0;

//	char *p = engine->pfnGetCvarString( "model" );
	char const *p = NULL;
	ConVar *var = (ConVar *)cvar->FindVar( "model" );
	if ( var )
	{
		p = var->GetString();
	}

	currentmodel[ 0 ] = 0;
	if ( p )
	{
		strcpy( currentmodel, p );
	}

	while ( fn )
	{
		//
		if ( fn[ 0 ] && fn[0] != '.' )
		{
			char modelname[ 256 ];
			_snprintf( modelname, sizeof( modelname ), "%s", fn );

			char *spot = strstr( modelname, ".mdl" );
			if ( spot )
			{
				*spot = 0;
			}

			_snprintf( cmdstring, sizeof( cmdstring ), "model %s\n", modelname );


			if ( !stricmp( currentmodel, modelname ) )
			{
				selected = c;
			}

			cb->AddItem( modelname, cmdstring );
			c++;
		}
		fn = filesystem()->FindNext( fh );
	}

	filesystem()->FindClose( fh );

	cb->SetInitialItem( selected );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::RemapLogo()
{
	char logoname[256];
	
	m_pLogoList->GetText( logoname, sizeof( logoname ) );
	if( !logoname[ 0 ] )
		return;

	char texture[ 256 ];
	sprintf( texture, "logos/remapped" );

	int r, g, b;
	const char *colorname = m_pColorList->GetActiveItemCommand();
	if ( !colorname || !colorname[ 0 ] )
		return;
	colorname += strlen("cl_logocolor ");

	ColorForName( colorname, r, g, b );
	RemapLogoPalette( logoname, r, g, b );

	m_pLogoImage->setTexture( texture );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::RemapModel()
{
	char modelname[256];
	
	m_pModelList->GetText( modelname, 256 );
	if( !modelname[ 0 ] )
		return;

	char texture[ 256 ];
	sprintf( texture, "models/player/remapped" );

	RemapPalette( modelname, m_nTopColor, m_nBottomColor );

	m_pModelImage->setTexture( texture );
}


//-----------------------------------------------------------------------------
// Purpose: Called whenever model name changes
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnTextChanged(vgui::Panel *panel)
{
	if (panel == m_pNameTextEntry)
    {
        return; // we don't need to remap model or logo if name got changes
    }

	RemapModel();
	RemapLogo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnSliderMoved(KeyValues *data)
{
    m_nTopColor = (int) m_pPrimaryColorSlider->GetSliderValue();
    m_nBottomColor = (int) m_pSecondaryColorSlider->GetSliderValue();

	RemapModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnApplyButtonEnable()
{
	PostMessage(GetParent(), new KeyValues("ApplyButtonEnable"));
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
MessageMapItem_t COptionsSubMultiplayer::m_MessageMap[] =
{
	MAP_MESSAGE_PTR( COptionsSubMultiplayer, "TextChanged", OnTextChanged, "panel" ),
	MAP_MESSAGE_PARAMS( COptionsSubMultiplayer, "SliderMoved", OnSliderMoved ),
	MAP_MESSAGE( COptionsSubMultiplayer, "ControlModified", OnApplyButtonEnable ),
};
IMPLEMENT_PANELMAP( COptionsSubMultiplayer, BaseClass );


//#include <windows.h>

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')
#define SUIT_HUE_START 192
#define SUIT_HUE_END 223
#define PLATE_HUE_START 160
#define PLATE_HUE_END 191

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void PaletteHueReplace( RGBQUAD *palSrc, int newHue, int Start, int end )
{
	int i;
	float r, b, g;
	float maxcol, mincol;
	float hue, val, sat;

	hue = (float)(newHue * (360.0 / 255));

	for (i = Start; i <= end; i++)
	{
		b = palSrc[ i ].rgbBlue;
		g = palSrc[ i ].rgbGreen;
		r = palSrc[ i ].rgbRed;
		
		maxcol = max( max( r, g ), b ) / 255.0f;
		mincol = min( min( r, g ), b ) / 255.0f;
		
		val = maxcol;
		sat = (maxcol - mincol) / maxcol;

		mincol = val * (1.0f - sat);

		if (hue <= 120)
		{
			b = mincol;
			if (hue < 60)
			{
				r = val;
				g = mincol + hue * (val - mincol)/(120 - hue);
			}
			else
			{
				g = val;
				r = mincol + (120 - hue)*(val-mincol)/hue;
			}
		}
		else if (hue <= 240)
		{
			r = mincol;
			if (hue < 180)
			{
				g = val;
				b = mincol + (hue - 120)*(val-mincol)/(240 - hue);
			}
			else
			{
				b = val;
				g = mincol + (240 - hue)*(val-mincol)/(hue - 120);
			}
		}
		else
		{
			g = mincol;
			if (hue < 300)
			{
				b = val;
				r = mincol + (hue - 240)*(val-mincol)/(360 - hue);
			}
			else
			{
				r = val;
				b = mincol + (360 - hue)*(val-mincol)/(hue - 240);
			}
		}

		palSrc[ i ].rgbBlue = (unsigned char)(b * 255);
		palSrc[ i ].rgbGreen = (unsigned char)(g * 255);
		palSrc[ i ].rgbRed = (unsigned char)(r * 255);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::RemapPalette( char *filename, int topcolor, int bottomcolor )
{
	char infile[ 256 ];
	char outfile[ 256 ];

	FileHandle_t file;
	CUtlBuffer outbuffer( 16384, 16384, false );

	sprintf( infile, "models/player/%s/%s.bmp", filename, filename );
	sprintf( outfile, "models/player/remapped.bmp" );

	file = filesystem()->Open( infile, "rb" );
	if ( file == FILESYSTEM_INVALID_HANDLE )
		return;

	// Parse bitmap
	BITMAPFILEHEADER bmfHeader;
	DWORD dwBitsSize, dwFileSize;
	LPBITMAPINFO lpbmi;

	dwFileSize = filesystem()->Size( file );

	filesystem()->Read( &bmfHeader, sizeof(bmfHeader), file );
	
	outbuffer.Put( &bmfHeader, sizeof( bmfHeader ) );
	
	if (bmfHeader.bfType == DIB_HEADER_MARKER)
	{
		dwBitsSize = dwFileSize - sizeof(bmfHeader);

		HGLOBAL hDIB = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, dwBitsSize );
		char *pDIB = (LPSTR)GlobalLock((HGLOBAL)hDIB);
		{
			filesystem()->Read(pDIB, dwBitsSize, file );

			lpbmi = (LPBITMAPINFO)pDIB;

			// Remap palette
			PaletteHueReplace( lpbmi->bmiColors, topcolor, SUIT_HUE_START, SUIT_HUE_END );
			PaletteHueReplace( lpbmi->bmiColors, bottomcolor, PLATE_HUE_START, PLATE_HUE_END );

			outbuffer.Put( pDIB, dwBitsSize );
		}	

		GlobalUnlock( hDIB);
		GlobalFree((HGLOBAL) hDIB);
	}

	filesystem()->Close(file);

	filesystem()->RemoveFile( outfile, NULL );

	filesystem()->CreateDirHierarchy("models/player", NULL);
	file = filesystem()->Open( outfile, "wb" );
	if ( file != FILESYSTEM_INVALID_HANDLE )
	{
		filesystem()->Write( outbuffer.Base(), outbuffer.TellPut(), file );
		filesystem()->Close( file );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::ColorForName( char const *pszColorName, int&r, int&g, int&b )
{
	r = g = b = 0;
	
	int count = sizeof( itemlist ) / sizeof( itemlist[0] );

	for ( int i = 0; i < count; i++ )
	{
		if (!strnicmp(pszColorName, itemlist[ i ].name, strlen(itemlist[ i ].name)))
		{
			r = itemlist[ i ].r;
			g = itemlist[ i ].g;
			b = itemlist[ i ].b;
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::RemapLogoPalette( char *filename, int r, int g, int b )
{
	char infile[ 256 ];
	char outfile[ 256 ];

	FileHandle_t file;
	CUtlBuffer outbuffer( 16384, 16384, false );

	sprintf(infile, "logos/%s.bmp", filename);
	sprintf(outfile, "logos/remapped.bmp");
	file = filesystem()->Open(infile, "rb");
	if (file == FILESYSTEM_INVALID_HANDLE)
		return;

	// Parse bitmap
	BITMAPFILEHEADER bmfHeader;
	DWORD dwBitsSize, dwFileSize;
	LPBITMAPINFO lpbmi;
	LPBITMAPCOREINFO lpbmc;  // pointer to BITMAPCOREINFO structure (old)

	dwFileSize = filesystem()->Size( file );

	filesystem()->Read( &bmfHeader, sizeof(bmfHeader), file );
	
	outbuffer.Put( &bmfHeader, sizeof( bmfHeader ) );
	
	if (bmfHeader.bfType == DIB_HEADER_MARKER)
	{
		dwBitsSize = dwFileSize - sizeof(bmfHeader);

		HGLOBAL hDIB = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, dwBitsSize );
		char *pDIB = (LPSTR)GlobalLock((HGLOBAL)hDIB);
		{
			filesystem()->Read(pDIB, dwBitsSize, file );

			lpbmi = (LPBITMAPINFO)pDIB;

			/* get pointer to BITMAPCOREINFO (old 1.x) */
			lpbmc = (LPBITMAPCOREINFO)pDIB;


			/* is this a Win 3.0 DIB? */
			bool bWinStyleDIB = true; // IS_WIN30_DIB(lpbi);

			float f = 0;

			for (int i = 0; i < 256; i++)
			{
				float t = f/256.0f;
				if (bWinStyleDIB)
				{
					lpbmi->bmiColors[i].rgbRed = (unsigned char)( r * t );
					lpbmi->bmiColors[i].rgbGreen = (unsigned char)(g * t);
					lpbmi->bmiColors[i].rgbBlue = (unsigned char)(b * t);
				}
				else
				{
					lpbmc->bmciColors[i].rgbtRed = (unsigned char)(r * t);
					lpbmc->bmciColors[i].rgbtGreen = (unsigned char)(g * t);
					lpbmc->bmciColors[i].rgbtBlue  = (unsigned char)(b * t);
				}
				f++;
			}

			outbuffer.Put( pDIB, dwBitsSize );
		}	

		GlobalUnlock( hDIB);
		GlobalFree((HGLOBAL) hDIB);
	}

	filesystem()->Close(file);

	filesystem()->RemoveFile( outfile, NULL );

	filesystem()->CreateDirHierarchy("logos", NULL);
	file = filesystem()->Open( outfile, "wb" );
	if ( file != FILESYSTEM_INVALID_HANDLE )
	{
		filesystem()->Write( outbuffer.Base(), outbuffer.TellPut(), file );
		filesystem()->Close( file );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnResetData()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnApplyChanges()
{
	m_pPrimaryColorSlider->ApplyChanges();
	m_pSecondaryColorSlider->ApplyChanges();
	m_pModelList->ApplyChanges();
	m_pLogoList->ApplyChanges();
    m_pLogoList->GetText(m_LogoName, sizeof(m_LogoName));
	m_pColorList->ApplyChanges();	
	m_pHighQualityModelCheckBox->ApplyChanges();
	m_pNameTextEntry->ApplyChanges();

	// hack, get command and skip over cvarname
	const char *colorname = m_pColorList->GetActiveItemCommand();
	if ( !colorname || !colorname[ 0 ] )
		return;
	colorname += strlen("cl_logocolor ");

	// save the logo name
	char cmd[512];
	_snprintf(cmd, sizeof(cmd) - 1, "cl_logofile %s\n", m_LogoName);
	engine->ClientCmd(cmd);

	int r, g, b;
	ColorForName( colorname, r, g, b );
	
	// Create a pldecal.wad file from the logo/remapped
	char infile[ 256 ];
	FileHandle_t file;
	sprintf( infile, "logos/remapped.bmp" );
	file = filesystem()->Open(infile, "rb");
	if ( file == FILESYSTEM_INVALID_HANDLE )
		return;

	// Parse bitmap
	BITMAPFILEHEADER bmfHeader;
	DWORD dwBitsSize, dwFileSize;

	dwFileSize = filesystem()->Size( file );

	filesystem()->Read( &bmfHeader, sizeof(bmfHeader), file );
	if (bmfHeader.bfType == DIB_HEADER_MARKER)
	{
		dwBitsSize = dwFileSize - sizeof(bmfHeader);

		HGLOBAL hDIB = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, dwBitsSize );

		char *pDIB = (LPSTR)GlobalLock((HGLOBAL)hDIB);
		{
			filesystem()->Read(pDIB, dwBitsSize, file );
		}
		GlobalUnlock( (HGLOBAL)hDIB );

		// Create .wad from the raw data
		UpdateLogoWAD( (void *)hDIB, r, g, b );

		GlobalFree( (HGLOBAL)hDIB );
	}

	filesystem()->Close( file );
}