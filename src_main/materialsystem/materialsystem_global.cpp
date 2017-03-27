//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "materialsystem_global.h"
#include "shaderapi.h"
#include "shadersystem.h"
#include <malloc.h>
#include "filesystem.h"

float g_ooOverbright;														  
int g_FrameNum;
IShaderAPI *g_pShaderAPI = 0;
IShaderShadow* g_pShaderShadow = 0;
IFileSystem *g_pFileSystem = 0;
