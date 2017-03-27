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

#ifndef SHADERGL_H
#define SHADERGL_H
#pragma once

#include <windows.h>
#include "gl/gl.h"

/* Include stuff that should be in <GL/gl.h> that might not be in your version. */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* EXT_bgra defines from <GL/gl.h> */
#ifndef GL_EXT_bgra
#define GL_BGR_EXT                          0x80E0
#define GL_BGRA_EXT                         0x80E1
#endif

/* ARB_multitexture defines and prototypes from <GL/gl.h> */
#ifndef GL_ARB_multitexture
#define GL_ACTIVE_TEXTURE_ARB               0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB        0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB            0x84E2
#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DARBPROC) (GLenum target, GLdouble s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FARBPROC) (GLenum target, GLfloat s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IARBPROC) (GLenum target, GLint s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SARBPROC) (GLenum target, GLshort s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DARBPROC) (GLenum target, GLdouble s, GLdouble t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FARBPROC) (GLenum target, GLfloat s, GLfloat t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IARBPROC) (GLenum target, GLint s, GLint t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SARBPROC) (GLenum target, GLshort s, GLshort t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DARBPROC) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FARBPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IARBPROC) (GLenum target, GLint s, GLint t, GLint r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SARBPROC) (GLenum target, GLshort s, GLshort t, GLshort r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DARBPROC) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DVARBPROC) (GLenum target, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FARBPROC) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FVARBPROC) (GLenum target, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IARBPROC) (GLenum target, GLint s, GLint t, GLint r, GLint q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IVARBPROC) (GLenum target, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SARBPROC) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SVARBPROC) (GLenum target, const GLshort *v);
typedef void (APIENTRY * PFNGLACTIVETEXTUREARBPROC) (GLenum target);
typedef void (APIENTRY * PFNGLCLIENTACTIVETEXTUREARBPROC) (GLenum target);
#endif

/* EXT_texture_cube_map defines from <GL/gl.h> */
#ifndef GL_EXT_texture_cube_map
#define GL_NORMAL_MAP_EXT                   0x8511
#define GL_REFLECTION_MAP_EXT               0x8512
#define GL_TEXTURE_CUBE_MAP_EXT             0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_EXT     0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT  0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT  0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT  0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT  0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT  0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT  0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_EXT       0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT    0x851C
#endif

/* NV_register_combiners defines and prototypes from <GL/gl.h> */
#ifndef GL_NV_register_combiners
#define GL_REGISTER_COMBINERS_NV            0x8522
#define GL_COMBINER0_NV                     0x8550
#define GL_COMBINER1_NV                     0x8551
#define GL_COMBINER2_NV                     0x8552
#define GL_COMBINER3_NV                     0x8553
#define GL_COMBINER4_NV                     0x8554
#define GL_COMBINER5_NV                     0x8555
#define GL_COMBINER6_NV                     0x8556
#define GL_COMBINER7_NV                     0x8557
#define GL_VARIABLE_A_NV                    0x8523
#define GL_VARIABLE_B_NV                    0x8524
#define GL_VARIABLE_C_NV                    0x8525
#define GL_VARIABLE_D_NV                    0x8526
#define GL_VARIABLE_E_NV                    0x8527
#define GL_VARIABLE_F_NV                    0x8528
#define GL_VARIABLE_G_NV                    0x8529
/*      GL_ZERO */
#define GL_CONSTANT_COLOR0_NV               0x852A
#define GL_CONSTANT_COLOR1_NV               0x852B
/*      GL_FOG */
#define GL_PRIMARY_COLOR_NV                 0x852C
#define GL_SECONDARY_COLOR_NV               0x852D
#define GL_SPARE0_NV                        0x852E
#define GL_SPARE1_NV                        0x852F
/*      GL_TEXTURE0_ARB */
/*      GL_TEXTURE1_ARB */
#define GL_UNSIGNED_IDENTITY_NV             0x8536
#define GL_UNSIGNED_INVERT_NV               0x8537
#define GL_EXPAND_NORMAL_NV                 0x8538
#define GL_EXPAND_NEGATE_NV                 0x8539
#define GL_HALF_BIAS_NORMAL_NV              0x853A
#define GL_HALF_BIAS_NEGATE_NV              0x853B
#define GL_SIGNED_IDENTITY_NV               0x853C
#define GL_SIGNED_NEGATE_NV                 0x853D
#define GL_E_TIMES_F_NV                     0x8531
#define GL_SPARE0_PLUS_SECONDARY_COLOR_NV   0x8532
/*      GL_NONE */
#define GL_SCALE_BY_TWO_NV                  0x853E
#define GL_SCALE_BY_FOUR_NV                 0x853F
#define GL_SCALE_BY_ONE_HALF_NV             0x8540
#define GL_BIAS_BY_NEGATIVE_ONE_HALF_NV     0x8541
#define GL_DISCARD_NV                       0x8530
#define GL_COMBINER_INPUT_NV                0x8542
#define GL_COMBINER_MAPPING_NV              0x8543
#define GL_COMBINER_COMPONENT_USAGE_NV      0x8544
#define GL_COMBINER_AB_DOT_PRODUCT_NV       0x8545
#define GL_COMBINER_CD_DOT_PRODUCT_NV       0x8546
#define GL_COMBINER_MUX_SUM_NV              0x8547
#define GL_COMBINER_SCALE_NV                0x8548
#define GL_COMBINER_BIAS_NV                 0x8549
#define GL_COMBINER_AB_OUTPUT_NV            0x854a
#define GL_COMBINER_CD_OUTPUT_NV            0x854b
#define GL_COMBINER_SUM_OUTPUT_NV           0x854c
#define GL_MAX_GENERAL_COMBINERS_NV         0x854d
#define GL_NUM_GENERAL_COMBINERS_NV         0x854e
#define GL_COLOR_SUM_CLAMP_NV               0x854f
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERFVNVPROC) (GLenum pname, const GLfloat *params);
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERFNVPROC) (GLenum pname, GLfloat param);
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERIVNVPROC) (GLenum pname, const GLint *params);
typedef void (APIENTRY * PFNGLCOMBINERPARAMETERINVPROC) (GLenum pname, GLint param);
typedef void (APIENTRY * PFNGLCOMBINERINPUTNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY * PFNGLCOMBINEROUTPUTNVPROC) (GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
typedef void (APIENTRY * PFNGLFINALCOMBINERINPUTNVPROC) (GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
typedef void (APIENTRY * PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC) (GLenum stage, GLenum portion, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC) (GLenum stage, GLenum portion, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC) (GLenum variable, GLenum pname, GLfloat *params);
typedef void (APIENTRY * PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC) (GLenum variable, GLenum pname, GLint *params);
#endif

/* WGL_EXT_swap_control defines and prototypes from <GL/gl.h> */
#ifndef WGL_EXT_swap_control
typedef int (APIENTRY * PFNWGLSWAPINTERVALEXTPROC) (int);
typedef int (APIENTRY * PFNWGLGETSWAPINTERVALEXTPROC) (void);
#endif

/* OpenGL 1.2 defines and prototypes from <GL/gl.h> */
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE                    0x812F
#endif

/* GL_EXT_secondary_color stuff */
typedef void (APIENTRY * PFNGLSECONDARYCOLOR3FEXTPROC)( float r, float g, float b );

/* ARB_multitexture command function pointers */
extern PFNGLMULTITEXCOORD2IARBPROC glMultiTexCoord2iARB;
extern PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord3fARB;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB;
extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;

/* NV_register_combiners command function pointers */
extern PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
extern PFNGLCOMBINERPARAMETERIVNVPROC glCombinerParameterivNV;
extern PFNGLCOMBINERPARAMETERFNVPROC glCombinerParameterfNV;
extern PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
extern PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
extern PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
extern PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
extern PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glGetCombinerInputParameterfvNV;
extern PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glGetCombinerInputParameterivNV;
extern PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glGetCombinerOutputParameterfvNV;
extern PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glGetCombinerOutputParameterivNV;
extern PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glGetFinalCombinerInputfvNV;
extern PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glGetFinalCombinerInputivNV;

/* WGL_EXT_swap_control command function pointers */
extern PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
extern PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

/* GL_EXT_secondary_color */
extern PFNGLSECONDARYCOLOR3FEXTPROC glSecondaryColor3fEXT;

#ifdef __cplusplus
}
#endif // __cplusplus

void InitGL( void );

#endif // SHADERGL_H
