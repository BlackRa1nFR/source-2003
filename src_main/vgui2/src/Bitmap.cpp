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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <vgui/ISurface.h>

#include "bitmap.h"
#include "vgui_internal.h"

#include <tier0/dbg.h>
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *filename - image file to load 
//-----------------------------------------------------------------------------
Bitmap::Bitmap(const char *filename, bool hardwareFiltered)
{
	_filtered = hardwareFiltered;
	// HACKHACK - force VGUI materials to be in the vgui/ directory
	//			 This needs to be revisited once GoldSRC is grandfathered off.
	_filename = (char *)malloc(strlen(filename) + 1 + strlen("vgui/"));
	Assert( _filename );
	_snprintf( _filename, strlen(filename) + 1 + strlen("vgui/"), "vgui/%s", filename );
	_id = 0;
	_uploaded = false;
	_color = Color(255, 255, 255, 255);
	_pos[0] = _pos[1] = 0;
	_valid = true;
	wide=0;
	tall=0;
	ForceUpload();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
Bitmap::~Bitmap()
{
	if (_filename)
	{
		free(_filename);
	}
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void Bitmap::GetSize(int &wide, int &tall)
{
	wide = 0;
	tall = 0;
	
	if (!_valid)
		return;

	surface()->DrawGetTextureSize(_id, wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: size of the bitmap
//-----------------------------------------------------------------------------
void Bitmap::GetContentSize(int &wide, int &tall)
{
	GetSize(wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: ignored
//-----------------------------------------------------------------------------
void Bitmap::SetSize(int x, int y)
{
	wide = x;
	tall = y;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void Bitmap::SetPos(int x, int y)
{
	_pos[0] = x;
	_pos[1] = y;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void Bitmap::SetColor(Color col)
{
	_color = col;
}


//-----------------------------------------------------------------------------
// Purpose: returns the file name of the bitmap
//-----------------------------------------------------------------------------
const char *Bitmap::GetName()
{
	return _filename;
}

//-----------------------------------------------------------------------------
// Purpose: Renders the loaded image, uploading it if necessary
//			Assumes a valid image is always returned from uploading
//-----------------------------------------------------------------------------
void Bitmap::Paint()
{
	if (!_valid)
		return;

	// if we don't have an _id then lets make one
	if (!_id)
	{
		_id = surface()->CreateNewTextureID();
	}
	
	// if we have not uploaded yet, lets go ahead and do so
	if (!_uploaded)
	{
		ForceUpload();
	}
	
	//set the texture current, set the color, and draw the biatch
	surface()->DrawSetTextureFile(_id, _filename, _filtered, false);
	surface()->DrawSetColor(_color[0], _color[1], _color[2], _color[3]);

	if (wide == 0)
	{
		GetSize(wide, tall);
	}
	surface()->DrawTexturedRect(_pos[0], _pos[1], _pos[0] + wide, _pos[1] + tall);
}

//-----------------------------------------------------------------------------
// Purpose: ensures the bitmap has been uploaded
//-----------------------------------------------------------------------------
void Bitmap::ForceUpload()
{
	if (!_valid)
		return;

	if (_uploaded)
		return;

	if (!_id)
	{
		_id = surface()->CreateNewTextureID();
	}

	surface()->DrawSetTextureFile(_id, _filename, _filtered, false);
	_uploaded = true;

	_valid = surface()->IsTextureIDValid(_id);
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
HCursor Bitmap::GetID()
{
	return _id;
}







