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

#include "ShaderGL.h"
#include <assert.h>

// code from nvidia bump demo

extern "C" {
/* ARB_multitexture command function pointers */
PFNGLMULTITEXCOORD2IARBPROC glMultiTexCoord2iARB;
PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord3fARB;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB;
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;

/* NV_register_combiners command function pointers */
PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
PFNGLCOMBINERPARAMETERIVNVPROC glCombinerParameterivNV;
PFNGLCOMBINERPARAMETERFNVPROC glCombinerParameterfNV;
PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glGetCombinerInputParameterfvNV;
PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glGetCombinerInputParameterivNV;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glGetCombinerOutputParameterfvNV;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glGetCombinerOutputParameterivNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glGetFinalCombinerInputfvNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glGetFinalCombinerInputivNV;

/* WGL_EXT_swap_control command function pointers */
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

/* GL_EXT_secondary_color */
PFNGLSECONDARYCOLOR3FEXTPROC glSecondaryColor3fEXT;
}

/*** OPENGL INITIALIZATION AND CHECKS ***/

/* Check if required extensions exist. */
// undone: check which extensions exist.
static void InitGLExtensions(void)
{
	/* Retrieve some ARB_multitexture routines. */
	glMultiTexCoord2iARB =
		(PFNGLMULTITEXCOORD2IARBPROC)
		wglGetProcAddress("glMultiTexCoord2iARB");
	glMultiTexCoord3fARB =
		(PFNGLMULTITEXCOORD3FARBPROC)
		wglGetProcAddress("glMultiTexCoord3fARB");
	glMultiTexCoord3fvARB =
		(PFNGLMULTITEXCOORD3FVARBPROC)
		wglGetProcAddress("glMultiTexCoord3fvARB");
	glMultiTexCoord2fARB = 
		(PFNGLMULTITEXCOORD2FARBPROC) 
		wglGetProcAddress( "glMultiTexCoord2fARB" );
	glActiveTextureARB =
		(PFNGLACTIVETEXTUREARBPROC)
		wglGetProcAddress("glActiveTextureARB");
	
	/* Retrieve all NV_register_combiners routines. */
	glCombinerParameterfvNV =
		(PFNGLCOMBINERPARAMETERFVNVPROC)
		wglGetProcAddress("glCombinerParameterfvNV");
	glCombinerParameterivNV =
		(PFNGLCOMBINERPARAMETERIVNVPROC)
		wglGetProcAddress("glCombinerParameterivNV");
	glCombinerParameterfNV =
		(PFNGLCOMBINERPARAMETERFNVPROC)
		wglGetProcAddress("glCombinerParameterfNV");
	glCombinerParameteriNV =
		(PFNGLCOMBINERPARAMETERINVPROC)
		wglGetProcAddress("glCombinerParameteriNV");
	glCombinerInputNV =
		(PFNGLCOMBINERINPUTNVPROC)
		wglGetProcAddress("glCombinerInputNV");
	glCombinerOutputNV =
		(PFNGLCOMBINEROUTPUTNVPROC)
		wglGetProcAddress("glCombinerOutputNV");
	glFinalCombinerInputNV =
		(PFNGLFINALCOMBINERINPUTNVPROC)
		wglGetProcAddress("glFinalCombinerInputNV");
	glGetCombinerInputParameterfvNV =
		(PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC)
		wglGetProcAddress("glGetCombinerInputParameterfvNV");
	glGetCombinerInputParameterivNV =
		(PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC)
		wglGetProcAddress("glGetCombinerInputParameterivNV");
	glGetCombinerOutputParameterfvNV =
		(PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC)
		wglGetProcAddress("glGetCombinerOutputParameterfvNV");
	glGetCombinerOutputParameterivNV =
		(PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC)
		wglGetProcAddress("glGetCombinerOutputParameterivNV");
	glGetFinalCombinerInputfvNV =
		(PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC)
		wglGetProcAddress("glGetFinalCombinerInputfvNV");
	glGetFinalCombinerInputivNV =
		(PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC)
		wglGetProcAddress("glGetFinalCombinerInputivNV");
	
	wglSwapIntervalEXT = 
		(PFNWGLSWAPINTERVALEXTPROC)
		wglGetProcAddress("wglSwapIntervalEXT");
	wglGetSwapIntervalEXT = 
		(PFNWGLGETSWAPINTERVALEXTPROC)
		wglGetProcAddress("wglGetSwapIntervalEXT");

	/* GL_EXT_secondary_color */
#if 0
	glSecondaryColor3fEXT = (PFNGLSECONDARYCOLOR3FEXTPROC)
		wglGetProcAddress( "glSecondaryColor3fEXT" );
	assert( glSecondaryColor3fEXT );
#endif
}

static void CheckDestinationAlphaSupport( void )
{
	GLint alphaBits;
	
	glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
	if (alphaBits > 0) 
	{
//		supportSpecular = 1;
	} 
	else 
	{
		OutputDebugString("\nnpeturb: Specular effects disabled due to lack of alpha buffer.\n");
		OutputDebugString("         Try switching to 32-bit True Color mode and restart.\n");
		assert( 0 ); // hack
	}
}

void InitGL( void )
{
	InitGLExtensions();
	CheckDestinationAlphaSupport();
}
