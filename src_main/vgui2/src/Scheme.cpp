//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>

#include <vgui/VGUI.h>
#include <vgui/IScheme.h>
#include <KeyValues.h>
#include <vgui/ISurface.h>

#include <UtlVector.h>
#include <UtlRBTree.h>
#include "vgui_border.h"
#include "vgui_internal.h"
#include "bitmap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;
#define FONT_ALIAS_NAME_LENGTH 64

//-----------------------------------------------------------------------------
// Purpose: Implementation of global scheme interface
//-----------------------------------------------------------------------------
class CScheme : public IScheme
{
public:
	CScheme();

	// gets a string from the default settings section
	virtual const char *GetResourceString(const char *stringName);

	// returns a pointer to an existing border
	virtual IBorder *GetBorder(const char *borderName);

	// returns a pointer to an existing font
	virtual HFont GetFont(const char *fontName, bool proportional);

	// colors
	virtual Color GetColor( const char *colorName, Color defaultColor);


	void Shutdown( bool full );
	void LoadFromFile( const char *filename, const char *tag, KeyValues *inKeys );

	// Gets at the scheme's name
	const char *GetName() { return tag; }
	const char *GetFileName() { return fileName; }

	char const *GetFontName( const HFont& font );

private:
	const char *GetMungedFontName( const char *fontName, const char *scheme, bool proportional);
	void LoadFonts();
	void LoadBorders();
	HFont FindFontInAliasList( const char *fontName );

	char fileName[256];
	char tag[64];

	KeyValues *data;
	KeyValues *baseSettings;
	KeyValues *colors;

	CUtlVector<IBorder*> _borderVec;
	IBorder  *_baseBorder;	// default border to use if others not found
	KeyValues *_borders;

	struct fontalias_t
	{
		char _fontName[FONT_ALIAS_NAME_LENGTH];
		char _trueFontName[FONT_ALIAS_NAME_LENGTH];
		HFont _font;
	};
	friend fontalias_t;

	CUtlVector<fontalias_t>	_fontAliases;
};


//-----------------------------------------------------------------------------
// Purpose: Implementation of global scheme interface
//-----------------------------------------------------------------------------
class CSchemeManager : public ISchemeManager
{
public:
	CSchemeManager();
	~CSchemeManager();

	// loads a scheme from a file
	// first scheme loaded becomes the default scheme, and all subsequent loaded scheme are derivitives of that
	// tag is friendly string representing the name of the loaded scheme
	virtual HScheme LoadSchemeFromFile(const char *fileName, const char *tag);

	// reloads the schemes from the file
	virtual void ReloadSchemes();

	// returns a handle to the default (first loaded) scheme
	virtual HScheme GetDefaultScheme();

	// returns a handle to the scheme identified by "tag"
	virtual HScheme GetScheme(const char *tag);

	// returns a pointer to an image
	virtual IImage *GetImage(const char *imageName, bool hardwareFiltered);
	virtual HTexture GetImageID(const char *imageName, bool hardwareFiltered);

	virtual IScheme *GetIScheme( HScheme scheme );

	virtual void Shutdown( bool full );

	// gets the proportional coordinates for doing screen-size independant panel layouts
	// use these for font, image and panel size scaling (they all use the pixel height of the display for scaling)
	virtual int GetProportionalScaledValue(int normalizedValue);
	virtual int GetProportionalNormalizedValue(int scaledValue);

private:
	// Search for already-loaded schemes
	HScheme FindLoadedScheme(const char *fileName);

	CUtlVector<CScheme *> _scheme;

	static const char *s_pszSearchString;
	struct CachedBitmapHandle_t
	{
		Bitmap *bitmap;
	};
	static bool BitmapHandleSearchFunc(const CachedBitmapHandle_t &, const CachedBitmapHandle_t &);
	CUtlRBTree<CachedBitmapHandle_t, int> m_Bitmaps;
};

const char *CSchemeManager::s_pszSearchString = NULL;

//-----------------------------------------------------------------------------
// Purpose: search function for stored bitmaps
//-----------------------------------------------------------------------------
bool CSchemeManager::BitmapHandleSearchFunc(const CachedBitmapHandle_t &lhs, const CachedBitmapHandle_t &rhs)
{
	// a NULL bitmap indicates to use the search string instead
	if (lhs.bitmap && rhs.bitmap)
	{
		return stricmp(lhs.bitmap->GetName(), rhs.bitmap->GetName()) > 0;
	}
	else if (lhs.bitmap)
	{
		return stricmp(lhs.bitmap->GetName(), s_pszSearchString) > 0;
	}
	return stricmp(s_pszSearchString, rhs.bitmap->GetName()) > 0;
}



CSchemeManager g_Scheme;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSchemeManager, ISchemeManager, VGUI_SCHEME_INTERFACE_VERSION, g_Scheme);


namespace vgui
{

// singleton accessor
ISchemeManager *scheme()
{
	return &g_Scheme;
}

} // namespace vgui
 
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSchemeManager::CSchemeManager()
{
	// 0th element is null, since that would be an invalid handle
	CScheme *nullScheme = new CScheme();
	_scheme.AddToTail(nullScheme);
	m_Bitmaps.SetLessFunc(&BitmapHandleSearchFunc);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSchemeManager::~CSchemeManager()
{
	for (int i = 0; i < m_Bitmaps.MaxElement(); i++)
	{
		if (m_Bitmaps.IsValidIndex(i))
		{
			delete m_Bitmaps[i].bitmap;
		}
	}
	m_Bitmaps.RemoveAll();

	Shutdown( true );
}

//-----------------------------------------------------------------------------
// Converts the handle into an interface
//-----------------------------------------------------------------------------
IScheme *CSchemeManager::GetIScheme( HScheme scheme )
{
	return _scheme[scheme];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSchemeManager::Shutdown( bool full )
{
	// Full shutdown kills the null scheme
	for( int i = full ? 0 : 1; i < _scheme.Count(); i++ )
	{
		_scheme[i]->Shutdown( full );
	}

	if ( full )
	{
		_scheme.RemoveAll();
	}
}

//-----------------------------------------------------------------------------
// Purpose: loads a scheme from disk
//-----------------------------------------------------------------------------
HScheme CSchemeManager::FindLoadedScheme(const char *fileName)
{
	// Find the scheme in the list of already loaded schemes
	for (int i = 1; i < _scheme.Count(); i++)
	{
		char const *schemeFileName = _scheme[i]->GetFileName();
		if (!stricmp(schemeFileName, fileName))
			return i;
	}

	return 0;
}

CScheme::CScheme()
{
	fileName[ 0 ] = 0;
	tag[ 0 ] = 0;

	data = NULL;
	baseSettings = NULL;
	colors = NULL;

	_baseBorder = NULL;	// default border to use if others not found
	_borders = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: loads a scheme from disk
//-----------------------------------------------------------------------------
HScheme CSchemeManager::LoadSchemeFromFile(const char *fileName, const char *tag)
{
	// Look to see if we've already got this scheme...
	HScheme hScheme = FindLoadedScheme(fileName);
	if (hScheme != 0)
		return hScheme;

	KeyValues *data;
	data = new KeyValues("Scheme");

	data->UsesEscapeSequences( true );	// VGUI uses this
	
	bool result = data->LoadFromFile((IBaseFileSystem*)filesystem(), fileName );
	if (!result)
	{
		data->deleteThis();
		return 0;
	}
	
	CScheme *newScheme = new CScheme();
	newScheme->LoadFromFile( fileName, tag, data );

	return _scheme.AddToTail(newScheme);
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScheme::LoadFromFile( const char *inFilename, const char *inTag, KeyValues *inKeys )
{

	strncpy(fileName, inFilename, sizeof(fileName) );
	fileName[ sizeof(fileName) - 1 ] = '\0';
	
	data = inKeys;
	baseSettings = data->FindKey("BaseSettings", true);
	colors = data->FindKey("Colors", true);

	// override the scheme name with the tag name
	KeyValues *name = data->FindKey("Name", true);
	name->SetString("Name", inTag);

	if ( inTag )
	{
		strncpy( tag, inTag, sizeof( tag ) );
	}
	else
	{
		Assert( "You need to name the scheme!" );
		strcpy( tag, "default" );
	}

	// need to copy tag before loading fonts
	LoadFonts();
	LoadBorders();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScheme::LoadFonts()
{

	// add our custom fonts
	{for (KeyValues *kv = data->FindKey("CustomFontFiles", true)->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		const char *fontFile = kv->GetString();
		if (fontFile && *fontFile)
		{
			surface()->AddCustomFontFile(fontFile);
		}
	}}


	// get our current resolution
	int screenWide, screenTall;
	surface()->GetScreenSize(screenWide, screenTall);

	for (KeyValues *kv = data->FindKey("Fonts", true)->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		for ( int i = 0; i < 2; i++ )
		{
			// create the base font
			bool proportionalFont = static_cast<bool>( i );
			const char *fontName = GetMungedFontName( kv->GetName(), tag, proportionalFont ); // first time it adds a normal font, and then a proportional one
			HFont font = surface()->CreateFont();

			// walk through creating add the glyph sets to the font
			for (KeyValues *fontdata = kv->GetFirstSubKey(); fontdata != NULL; fontdata = fontdata->GetNextKey())
			{
				// skip over fonts not meant for this resolution
				int fontYResMin = 0, fontYResMax = 0;
				sscanf(fontdata->GetString("yres", ""), "%d %d", &fontYResMin, &fontYResMax);
				if (fontYResMin)
				{
					if (!fontYResMax)
					{
						fontYResMax = fontYResMin;
					}
					// check the range
					if (screenTall < fontYResMin || screenTall > fontYResMax)
						continue;
				}

				int flags = 0;
				if (fontdata->GetInt("italic"))
				{
					flags |= ISurface::FONTFLAG_ITALIC;
				}
				if (fontdata->GetInt("underline"))
				{
					flags |= ISurface::FONTFLAG_UNDERLINE;
				}
				if (fontdata->GetInt("strikeout"))
				{
					flags |= ISurface::FONTFLAG_STRIKEOUT;
				}
				if (fontdata->GetInt("symbol"))
				{
					flags |= ISurface::FONTFLAG_SYMBOL;
				}
				if (fontdata->GetInt("antialias") && surface()->SupportsFeature(ISurface::ANTIALIASED_FONTS))
				{
					flags |= ISurface::FONTFLAG_ANTIALIAS;
				}
				if (fontdata->GetInt("dropshadow") && surface()->SupportsFeature(ISurface::DROPSHADOW_FONTS))
				{
					flags |= ISurface::FONTFLAG_DROPSHADOW;
				}
				if (fontdata->GetInt("outline") && surface()->SupportsFeature(ISurface::OUTLINE_FONTS))
				{
					flags |= ISurface::FONTFLAG_OUTLINE;
				}
				if (fontdata->GetInt( "rotary" ))
				{
					flags |= ISurface::FONTFLAG_ROTARY;
				}
				if (fontdata->GetInt( "additive" ))
				{
					flags |= ISurface::FONTFLAG_ADDITIVE;
				}



				int lowRange = 0, highRange = 0;
				sscanf(fontdata->GetString("range", ""), "%i %i", &lowRange, &highRange);
				int tall = fontdata->GetInt("tall");
				int blur = fontdata->GetInt("blur");
				int scanlines = fontdata->GetInt("scanlines");

				if ( (!fontYResMin && !fontYResMax) && proportionalFont ) // only grow this font if it doesn't have a resolution filter specified
				{
					tall = g_Scheme.GetProportionalScaledValue( tall );
					blur = g_Scheme.GetProportionalScaledValue( blur );
					scanlines = g_Scheme.GetProportionalScaledValue( scanlines );
				}

				// clip the font size so that fonts can't be too big
				if (tall > 128)
				{
					tall = 128;
				}

				surface()->AddGlyphSetToFont(
					font,
					fontdata->GetString("name"), 
					tall, 
					fontdata->GetInt("weight"), 
					blur,
					scanlines,
					flags,
					lowRange, 
					highRange);
				int alias = _fontAliases.AddToTail();
				strncpy( _fontAliases[alias]._fontName, fontName, FONT_ALIAS_NAME_LENGTH );
				strncpy( _fontAliases[alias]._trueFontName, kv->GetName(), FONT_ALIAS_NAME_LENGTH );
				_fontAliases[alias]._font = font;
			}
		}
	}
}

void CScheme::LoadBorders()
{
	_borders = data->FindKey("Borders", true);
	for ( KeyValues *kv = _borders->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
	{
		IBorder *border = new Border();
		border->SetName(kv->GetName());
		border->ApplySchemeSettings(this, kv); 
		_borderVec.AddToTail(border);
	}
	
	_baseBorder = GetBorder("BaseBorder");
}

//-----------------------------------------------------------------------------
// Purpose: reloads the scheme from the file
//-----------------------------------------------------------------------------
void CSchemeManager::ReloadSchemes()
{
	int count = _scheme.Count();
	Shutdown( false );
	
	// reload the scheme
	for (int i = 1; i < count; i++)
	{
		LoadSchemeFromFile(_scheme[i]->GetFileName(), _scheme[i]->GetName());
	}
}

//-----------------------------------------------------------------------------
// Purpose: kills all the schemes
//-----------------------------------------------------------------------------
void CScheme::Shutdown( bool full )
{
	for (int i = 0; i < _borderVec.Count(); i++)
	{
		// Cast back from IBorder to Border *
		Border *border = ( Border * )_borderVec[i];
		delete border;
	}

	_baseBorder = NULL;
	_borderVec.RemoveAll();
	_borders = NULL;

	if ( full )
	{
		if( data )
		{
			data->deleteThis();
			data = NULL;
		}

		delete this;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns a handle to the default (first loaded) scheme
//-----------------------------------------------------------------------------
HScheme CSchemeManager::GetDefaultScheme()
{
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: returns a handle to the scheme identified by "tag"
//-----------------------------------------------------------------------------
HScheme CSchemeManager::GetScheme(const char *tag)
{
	for (int i=1;i<_scheme.Count();i++)
	{
		if ( !stricmp(tag,_scheme[i]->GetName()) )
		{
			return i;
		}
	}
	return 1; // default scheme
}

//-----------------------------------------------------------------------------
// Purpose: converts a value into proportional mode
//-----------------------------------------------------------------------------
int CSchemeManager::GetProportionalScaledValue(int normalizedValue)
{
	int wide, tall;
	surface()->GetScreenSize( wide, tall );
	int proH, proW;
	surface()->GetProportionalBase( proW, proH );
	float scale = (float)tall / (float)proH;

	return (int)( normalizedValue * scale );
}

//-----------------------------------------------------------------------------
// Purpose: converts a value out of proportional mode
//-----------------------------------------------------------------------------
int CSchemeManager::GetProportionalNormalizedValue(int scaledValue)
{
	int wide, tall;
	surface()->GetScreenSize( wide, tall );
	int proH, proW;
	surface()->GetProportionalBase( proW, proH );
	float scale = (float)tall / (float)proH;

	return (int)( scaledValue / scale );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CScheme::GetResourceString(const char *stringName)
{
	return baseSettings->GetString(stringName);
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to an image
//-----------------------------------------------------------------------------
IImage *CSchemeManager::GetImage(const char *imageName, bool hardwareFiltered)
{
	if ( !imageName || strlen(imageName) <= 0 ) // frame icons and the like are in the scheme file and may not be defined, so if this is null then fail silently
	{
		return NULL; 
	}

	// set up to search for the bitmap
	CachedBitmapHandle_t searchBitmap;
	searchBitmap.bitmap = NULL;
	s_pszSearchString = imageName;
	int i = m_Bitmaps.Find(searchBitmap);
	if (m_Bitmaps.IsValidIndex(i))
	{
		return m_Bitmaps[i].bitmap;
	}

	// couldn't find the image, try and load it
	CachedBitmapHandle_t bitmap = { new Bitmap(imageName, hardwareFiltered) };
	m_Bitmaps.Insert(bitmap);
	return bitmap.bitmap;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
HTexture CSchemeManager::GetImageID(const char *imageName, bool hardwareFiltered)
{
	IImage *img = GetImage(imageName, hardwareFiltered);
	return ((Bitmap *)img)->GetID();
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to an existing border
//-----------------------------------------------------------------------------
IBorder *CScheme::GetBorder(const char *borderName)
{
	for (int i = 0; i < _borderVec.Count(); i++)
	{
		if (!stricmp(_borderVec[i]->GetName(), borderName))
		{
			return _borderVec[i];
		}
	}

	return _baseBorder;
}

//-----------------------------------------------------------------------------
// Finds a font in the alias list
//-----------------------------------------------------------------------------
HFont CScheme::FindFontInAliasList( const char *fontName )
{
	// FIXME: Slow!!!
	for (int i = _fontAliases.Count(); --i >= 0; )
	{
		const char *name = _fontAliases[i]._fontName;
		if (!strnicmp(fontName, _fontAliases[i]._fontName, FONT_ALIAS_NAME_LENGTH ))
			return _fontAliases[i]._font;
	}

	// No dice
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : font - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CScheme::GetFontName( const HFont& font )
{
	for (int i = _fontAliases.Count(); --i >= 0; )
	{
		HFont& fnt = _fontAliases[i]._font;
		if ( fnt == font )
			return _fontAliases[i]._trueFontName;
	}

	return "<Unknown font>";
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to an existing font, tall=0 means use default
//-----------------------------------------------------------------------------
HFont CScheme::GetFont(const char *fontName,bool proportional)
{
	// First look in the list of aliases...
	return FindFontInAliasList( GetMungedFontName( fontName, tag, proportional ) );
}

//-----------------------------------------------------------------------------
// Purpose: returns a char string of the munged name this font is stored as in the font manager
//-----------------------------------------------------------------------------
const char *CScheme::GetMungedFontName( const char *fontName, const char *scheme, bool proportional )
{
	static char mungeBuffer[ 64 ];
	if ( scheme )
	{
		_snprintf( mungeBuffer, sizeof( mungeBuffer ), "%s%s-%s", fontName, scheme, proportional ? "p" : "no" );
	}
	else
	{ 
		_snprintf( mungeBuffer, sizeof( mungeBuffer ), "%s-%s", fontName, proportional ? "p" : "no" ); // we don't want the "(null)" snprintf appends
	}
	return mungeBuffer;
}

//-----------------------------------------------------------------------------
// Purpose: Gets a color from the scheme file
//-----------------------------------------------------------------------------
Color CScheme::GetColor(const char *colorName, Color defaultColor)
{
	// check the color area first
	KeyValues *data = colors;
	const char *colStr = data->GetString(colorName, NULL);
	if (colStr)
	{
		int r, g, b, a;
		sscanf(colStr, "%d %d %d %d", &r, &g, &b, &a);
		return Color(r, g, b, a);
	}

	// check base settings
	data = baseSettings;
	colStr = data->GetString(colorName, NULL);
	if (colStr)
	{
		int r, g, b, a;
		int res = sscanf(colStr, "%d %d %d %d", &r, &g, &b, &a);
		if (res < 3)
		{
			// must be a color name, lookup color
			char colorName[64];
			strncpy(colorName, colStr, 63);
			colorName[63] = 0;
			return GetColor(colStr, defaultColor);
		}

		return Color(r, g, b, a);
	}

	// try parse out the color
	int r, g, b, a;
	int res = sscanf(colorName, "%d %d %d %d", &r, &g, &b, &a);
	if (res > 2)
	{
		return Color(r, g, b, a);
	}

	return defaultColor;
}

