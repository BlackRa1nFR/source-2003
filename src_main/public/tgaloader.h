//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef TGALOADER_H
#define TGALOADER_H
#pragma once

#include "imageloader.h"
#include "interface.h"
#include "utlmemory.h"

class CUtlBuffer;


namespace TGALoader
{

#ifndef TGALOADER_USE_FOPEN
bool SetFileSystem( CreateInterfaceFn fileSystemFactory );
#endif
bool GetInfo( const char *fileName, int *width, int *height, ImageFormat *imageFormat, float *sourceGamma );
bool GetInfo( CUtlBuffer &buf, int *width, int *height, ImageFormat *imageFormat, float *sourceGamma );
bool Load( unsigned char *imageData, const char *fileName, int width, int height, 
		   ImageFormat imageFormat, float targetGamma, bool mipmap );
bool Load( unsigned char *imageData, FILE *fp, int width, int height, 
			ImageFormat imageFormat, float targetGamma, bool mipmap );
bool Load( unsigned char *imageData, CUtlBuffer &buf, int width, int height, 
			ImageFormat imageFormat, float targetGamma, bool mipmap );

bool LoadRGBA8888( const char *pFileName, CUtlMemory<unsigned char> &outputData, int &outWidth, int &outHeight );

} // end namespace TGALoader

#endif // TGALOADER_H
