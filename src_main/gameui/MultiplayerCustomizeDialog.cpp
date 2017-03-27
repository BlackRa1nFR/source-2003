//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <windows.h>
#include "MultiplayerCustomizeDialog.h"
#include <stdio.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <KeyValues.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Controls.h>
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
#include "GameUIPanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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
	{ "Orange", 255, 180, 24 },
	{ "Blue", 0, 60, 255 },
	{ "Ltblue", 0, 167, 255 },
	{ "Green", 0, 167, 0 },
	{ "Red", 255, 73, 0 },
	{ "Brown", 123, 73, 0 },
	{ "Ltgray", 100, 100, 100 },
	{ "Dkgray", 36, 36, 36 },
};

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CMultiplayerCustomizeDialog::CMultiplayerCustomizeDialog( Panel *menuTarget ) 
: Frame(NULL, "MultiplayerCustomizeDialog"), m_pMenuTarget( menuTarget )
{
	SetBounds(0, 0, 372, 160);
	SetSizeable( false );

	SetParent( GetGameUIRootPanel() );
	surface()->CreatePopup( GetVPanel(), false );

	SetTitle("#GameUI_MultiplayerCustomize", true);

	Button *cancel = new Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	Button *ok = new Button( this, "OK", "#GameUI_OK" );
	ok->SetCommand( "Ok" );

	Button *advanced = new Button( this, "Advanced", "#GameUI_Advanced" );
	advanced->SetCommand( "Advanced" );

	new Label( this, "NameLabel", "#GameUI_PlayerName" );
	new CCvarTextEntry( this, "NameEntry", "name" );

	new CCvarSlider( this, "Primary Color Slider", "#GameUI_PrimaryColor",
		0.0f, 255.0f, "topcolor" );

	new CCvarSlider( this, "Secondary Color Slider", "#GameUI_SecondaryColor",
		0.0f, 255.0f, "bottomcolor" );

	new CCvarToggleCheckButton( this, "High Quality Models", "#GameUI_HighModels", "cl_himodels" );

	m_pModelList = new CLabeledCommandComboBox( this, "Player model", "#GameUI_PlayerModel" );
	InitModelList( m_pModelList );

	m_pLogoList = new CLabeledCommandComboBox( this, "SpraypaintList", "#GameUI_SpraypaintImage" );
	InitLogoList( m_pLogoList );

	m_pColorList = new CLabeledCommandComboBox( this, "SpraypaintColor", "" );
	m_pColorList->showLabel( false );

	
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
	m_pColorList->ActivateItem( selected );
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

	LoadControlSettings("Resource\\MultiplayerCustomizeDialog.res");

	ivgui()->AddTickSignal(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMultiplayerCustomizeDialog::~CMultiplayerCustomizeDialog()
{
	filesystem()->RemoveFile( "models/player/remapped.bmp", NULL );
	filesystem()->RemoveFile( "logos/remapped.bmp", NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CMultiplayerCustomizeDialog::OnCommand( const char *command )
{
	if ( !stricmp( command, "Ok" ) )
	{
		OnApplyChanges();
		OnClose();
		return;
	}
	else if ( !stricmp( command, "Advanced" ) )
	{
		if ( m_pMenuTarget )
		{
			PostMessage( m_pMenuTarget->GetVPanel(),
				new KeyValues( "Command", "command", "OpenMultiplayerAdvancedDialog" ) );
		}
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiplayerCustomizeDialog::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}

void CMultiplayerCustomizeDialog::InitLogoList( CLabeledCommandComboBox *cb )
{
	// Find out images
	FileFindHandle_t fh;
	char directory[ 512 ];

	sprintf( directory, "logos/*.bmp" );

	char const *fn = filesystem()->FindFirst( directory, &fh );

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
		}
		fn = filesystem()->FindNext( fh );
	}

	filesystem()->FindClose( fh );

	// FIXME:  Retrieve from somewhere
	cb->ActivateItem( 0 );
}

void CMultiplayerCustomizeDialog::InitModelList( CLabeledCommandComboBox *cb )
{
	// Find out images
	FileFindHandle_t fh;
	char directory[ 512 ];
	char cmdstring[ 256 ];
	char currentmodel[ 64 ];

	sprintf( directory, "models/player/*.mdl" );

	char const *fn = filesystem()->FindFirst( directory, &fh );

	int c = 0;
	int selected = 0;

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
			Q_snprintf( modelname, sizeof( modelname ), "%s", fn );

			char *spot = Q_strstr( modelname, ".mdl" );
			if ( spot )
			{
				*spot = 0;
			}

			Q_snprintf( cmdstring, sizeof( cmdstring ), "model %s\n", modelname );


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

	cb->ActivateItem( selected );

}

void CMultiplayerCustomizeDialog::RemapLogo()
{
	char logoname[256];
	
	m_pLogoList->GetText( logoname, sizeof( logoname ) );
	if( !logoname[ 0 ] )
		return;

	char texture[ 256 ];
	sprintf( texture, "logos/remapped" );

	int r, g, b;
	char colorname[ 32 ];

	m_pColorList->GetText( colorname, sizeof( colorname ) );

	ColorForName( colorname, r, g, b );
	RemapLogoPalette( logoname, r, g, b );

	m_pLogoImage->setTexture( texture );
}

void CMultiplayerCustomizeDialog::RemapModel()
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
void CMultiplayerCustomizeDialog::OnTextChanged()
{
	RemapModel();
	RemapLogo();
}

void CMultiplayerCustomizeDialog::OnTick()
{
	int topcolor	= m_nTopColor;
	ConVar *var = (ConVar *)cvar->FindVar( "topcolor" );
	if ( var )
	{
		topcolor = var->GetInt();
	}
		
	int bottomcolor = m_nBottomColor;
	var = (ConVar *)cvar->FindVar( "bottomcolor" );
	if ( var )
	{
		bottomcolor = var->GetInt();
	}

	// Check if values changed
	if ( m_nTopColor != topcolor ||
		 m_nBottomColor != bottomcolor )
	{
		m_nTopColor = topcolor;
		m_nBottomColor = bottomcolor;
		
		RemapModel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
MessageMapItem_t CMultiplayerCustomizeDialog::m_MessageMap[] =
{
	MAP_MESSAGE( CMultiplayerCustomizeDialog, "TextChanged", OnTextChanged ),
};
IMPLEMENT_PANELMAP( CMultiplayerCustomizeDialog, BaseClass );



#include <windows.h>

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')
#define SUIT_HUE_START 192
#define SUIT_HUE_END 223
#define PLATE_HUE_START 160
#define PLATE_HUE_END 191

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

void CMultiplayerCustomizeDialog::RemapPalette( char *filename, int topcolor, int bottomcolor )
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

	file = filesystem()->Open( outfile, "wb" );
	if ( file != FILESYSTEM_INVALID_HANDLE )
	{
		filesystem()->Write( outbuffer.Base(), outbuffer.TellPut(), file );
		filesystem()->Close( file );
	}
}

void CMultiplayerCustomizeDialog::ColorForName( char const *pszColorName, int&r, int&g, int&b )
{
	r = g = b = 0;
	
	int count = sizeof( itemlist ) / sizeof( itemlist[0] );

	for ( int i = 0; i < count; i++ )
	{
		if (!stricmp(pszColorName, itemlist[ i ].name ))
		{
			r = itemlist[ i ].r;
			g = itemlist[ i ].g;
			b = itemlist[ i ].b;
			return;
		}
	}
}

void CMultiplayerCustomizeDialog::RemapLogoPalette( char *filename, int r, int g, int b )
{
	char infile[ 256 ];
	char outfile[ 256 ];

	FileHandle_t file;
	CUtlBuffer outbuffer( 16384, 16384, false );

	sprintf( infile, "logos/%s.bmp", filename );
	sprintf( outfile, "logos/remapped.bmp" );

	file = filesystem()->Open( infile, "rb" );
	if ( file == FILESYSTEM_INVALID_HANDLE )
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

	file = filesystem()->Open( outfile, "wb" );
	if ( file != FILESYSTEM_INVALID_HANDLE )
	{
		filesystem()->Write( outbuffer.Base(), outbuffer.TellPut(), file );
		filesystem()->Close( file );
	}
}

void CMultiplayerCustomizeDialog::OnApplyChanges()
{
	ConVar *var = (ConVar *)cvar->FindVar( "cl_logocolor" );
	if ( !var )
		return;

	char const *colorname = var->GetString();
	if ( !colorname || !colorname[ 0 ] )
		return;

	int r, g, b;
	ColorForName( colorname, r, g, b );
	
	// Create a pldecal.wad file from the logo/remapped
	char infile[ 256 ];
	FileHandle_t file;
	sprintf( infile, "logos/remapped.bmp" );
	file = filesystem()->Open( infile, "rb" );
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