//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LOCALD3DTYPES_H
#define LOCALD3DTYPES_H
#ifdef _WIN32
#pragma once
#endif

#include <d3d9.h>
#include <d3dx9.h>
#include <d3dx9core.h>

struct IDirect3DTexture9;
struct IDirect3DBaseTexture9;
struct IDirect3DCubeTexture9;
struct IDirect3D9;
struct IDirect3DDevice9;
typedef IDirect3DTexture9 IDirect3DTexture;
typedef IDirect3DBaseTexture9 IDirect3DBaseTexture;
typedef IDirect3DCubeTexture9 IDirect3DCubeTexture;
typedef IDirect3DDevice9 IDirect3DDevice;
typedef IDirect3D9 IDirect3D;
typedef struct _D3DADAPTER_IDENTIFIER9 D3DADAPTER_IDENTIFIER9;
typedef D3DADAPTER_IDENTIFIER9 D3DADAPTER_IDENTIFIER;
typedef struct _D3DMATERIAL9 D3DMATERIAL9;
typedef D3DMATERIAL9 D3DMATERIAL;
typedef struct _D3DLIGHT9 D3DLIGHT9;
typedef D3DLIGHT9 D3DLIGHT;
struct IDirect3DSurface9;
typedef IDirect3DSurface9 IDirect3DSurface;
typedef struct _D3DCAPS9 D3DCAPS9;
typedef D3DCAPS9 D3DCAPS;
typedef struct _D3DVIEWPORT9 D3DVIEWPORT9;
typedef D3DVIEWPORT9 D3DVIEWPORT;
struct IDirect3DIndexBuffer9;
typedef IDirect3DIndexBuffer9 IDirect3DIndexBuffer;
struct IDirect3DVertexBuffer9;
typedef IDirect3DVertexBuffer9 IDirect3DVertexBuffer;
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;

typedef IDirect3DVertexShader9 *HardwareVertexShader_t;
typedef IDirect3DPixelShader9 *HardwarePixelShader_t;

//-----------------------------------------------------------------------------
// The vertex and pixel shader type
//-----------------------------------------------------------------------------
typedef unsigned int VertexShader_t;
typedef unsigned int PixelShader_t;	

#define INVALID_PIXEL_SHADER	( ( PixelShader_t )0xFFFFFFFF )
#define INVALID_VERTEX_SHADER	( ( VertexShader_t )0xFFFFFFFF )


//-----------------------------------------------------------------------------
// Bitpattern for an invalid shader
//-----------------------------------------------------------------------------
#define INVALID_HARDWARE_VERTEX_SHADER ( ( HardwareVertexShader_t )NULL )
#define	INVALID_HARDWARE_PIXEL_SHADER ( ( HardwarePixelShader_t )NULL )


typedef IDirect3DDevice *LPDIRECT3DDEVICE;
typedef IDirect3DIndexBuffer *LPDIRECT3DINDEXBUFFER;
typedef IDirect3DVertexBuffer *LPDIRECT3DVERTEXBUFFER;

typedef unsigned int PixelShader_t;
typedef unsigned int VertexShader_t;

#endif // LOCALD3DTYPES_H
