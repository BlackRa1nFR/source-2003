//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// The main dx8 playback test app
//=============================================================================

#include "locald3dtypes.h"
#include <stdio.h>
#include "recording.h"
#include "UtlVector.h"
#include "UtlRBTree.h"
#include "ImageLoader.h"
#include "TGAWriter.h"
#include "mathlib.h"
#include "tier0/dbg.h"
//#include "vtuneapi.h"

#define CHECK_INDICES
#define MAX_NUM_STREAMS 2

#ifdef CHECK_INDICES
int g_StreamSourceID[MAX_NUM_STREAMS] = { -1, -1 };
#endif


// Set this to only do the first of a sequence of the same command.
//#define ONLY_FIRST_CMD
//#define ONLY_FIRST_CMD_CMD DX8_SET_VERTEX_SHADER_CONSTANT
//#define ONLY_FIRST_CMD_CMD 255

//#define SKIP_CMD
//#define SKIP_CMD_CMD DX8_SET_VERTEX_SHADER_CONSTANT
//#define SKIP_CMD_CMD 255

// Define this if you want to see verts in the listing files
//#define PRINT_VERTEX_BUFFER

// Define this to force drawindexprimitive calls to have at least N primitives.
// Will replicate previous indices to make more of them.
// 2 bytes per index
// 85 triangles/call seems to be the minimum for static props on a GF4.
//#define MIN_STATIC_INDEX_BUFFER_PRIMITIVES (100) // 9 ms
//#define MIN_STATIC_INDEX_BUFFER_PRIMITIVES (90) // 8.6ms
//#define MIN_STATIC_INDEX_BUFFER_PRIMITIVES (85) // 8.5ms
//#define MIN_STATIC_INDEX_BUFFER_PRIMITIVES (100) // 8.5ms
//#define MIN_STATIC_INDEX_BUFFER_BYTES ( MIN_STATIC_INDEX_BUFFER_PRIMITIVES * 6 )
//#define MAX_STATIC_INDEX_BUFFER_PRIMITIVES MIN_STATIC_INDEX_BUFFER_PRIMITIVES
//#define MAX_STATIC_INDEX_BUFFER_BYTES ( MIN_STATIC_INDEX_BUFFER_BYTES )

int g_ForcedStaticIndexBufferBytes = -1;
int g_ForcedStaticIndexBufferPrims = -1;

void DisplayError( char const* pError, ... );

// Define this if you want to use a stubbed d3d.
//#define STUBD3D

#ifdef STUBD3D
#define USE_FOPEN
#include "..\stubd3ddevice.h"
#endif

CUtlVector<double> g_FrameTime;
CUtlVector<double> g_HardwareFlushTime;

bool g_bAllowHWSync = true;

int g_FrameNum = 1;
int g_LockedTextureID = -1;

// This must match the definition in shaderapi.h!
enum ShaderRenderTarget_t
{
	SHADER_RENDERTARGET_BACKBUFFER = 0xffffffff,
	SHADER_RENDERTARGET_DEPTHBUFFER = 0xffffffff,
};

// This must match the definition in imaterial.h!
enum VertexFormatFlags_t
{
	VERTEX_POSITION	= 0x0001,
	VERTEX_NORMAL	= 0x0002,
	VERTEX_COLOR	= 0x0004,
	VERTEX_SPECULAR = 0x0008,

	VERTEX_TANGENT_S	= 0x0010,
	VERTEX_TANGENT_T	= 0x0020,

	VERTEX_TANGENT_SPACE= VERTEX_TANGENT_S | VERTEX_TANGENT_T,

	// Indicates we're using bone indices
	VERTEX_BONE_INDEX = 0x0080,

	// Indicates this is a vertex shader
	VERTEX_FORMAT_VERTEX_SHADER = 0x0100,

	// Update this if you add or remove bits...
	VERTEX_LAST_BIT = 8,

	VERTEX_BONE_WEIGHT_BIT = VERTEX_LAST_BIT + 1,
	USER_DATA_SIZE_BIT = VERTEX_LAST_BIT + 4,
	NUM_TEX_COORD_BIT = VERTEX_LAST_BIT + 7,
	TEX_COORD_SIZE_BIT = VERTEX_LAST_BIT + 10,
};

//-----------------------------------------------------------------------------
// The vertex format type
//-----------------------------------------------------------------------------

typedef unsigned int VertexFormat_t;

//-----------------------------------------------------------------------------
// Gets at various vertex format info...
//-----------------------------------------------------------------------------

inline int VertexFlags( VertexFormat_t vertexFormat )
{
	return vertexFormat & ( (1 << (VERTEX_LAST_BIT+1)) - 1 );
}

inline int NumBoneWeights( VertexFormat_t vertexFormat )
{
	return (vertexFormat >> VERTEX_BONE_WEIGHT_BIT) & 0x7;
}

inline int NumTextureCoordinates( VertexFormat_t vertexFormat )
{
	return (vertexFormat >> NUM_TEX_COORD_BIT) & 0x7;
}

inline int UserDataSize( VertexFormat_t vertexFormat )
{
	return (vertexFormat >> USER_DATA_SIZE_BIT) & 0x7;
}

inline int TexCoordSize( int stage, VertexFormat_t vertexFormat )
{
	return (vertexFormat >> (TEX_COORD_SIZE_BIT + 3*stage) ) & 0x7;
}

inline bool UsesVertexShader( VertexFormat_t vertexFormat )
{
	return (vertexFormat & VERTEX_FORMAT_VERTEX_SHADER) != 0;
}

//-----------------------------------------------------------------------------
// Command parsing..
//-----------------------------------------------------------------------------

class CUtlBuffer
{
public:
	CUtlBuffer();

	void Attach( void const* pBuffer, int size );

	void GetUnsignedChar( unsigned char& c );
	void GetInt( int& i );
	void GetFloat( float& f );
	void GetString( char* pString );
	void Get( void* pMem, int size );

	void const* PeekGet() const;
	void SeekGet( int offset )	{ m_Get = offset; }
	void const* Base() const	{ return m_pBuffer; }
	int Size() const			{ return m_Size; }

private:
	char const* m_pBuffer;
	int m_Size;
	int m_Get;
};

#define GET_TYPE( _type, _val )	\
	_val = *(_type *)PeekGet();	\
	m_Get += sizeof(_type);

CUtlBuffer::CUtlBuffer( )
{
	m_pBuffer = 0;
	m_Size = 0;
	m_Get = 0;
}

void CUtlBuffer::Attach( void const* pBuffer, int size )
{
	m_pBuffer = (char const*)pBuffer;
	m_Size = size;
	m_Get = 0;
}

void CUtlBuffer::Get( void* pMem, int size )
{
	Assert( m_Get + size <= m_Size );
	memcpy( pMem, &m_pBuffer[m_Get], size );
	m_Get += size;
}

void CUtlBuffer::GetUnsignedChar( unsigned char& c )
{
	GET_TYPE( unsigned char, c );
}

void CUtlBuffer::GetInt( int& i )
{
	GET_TYPE( int, i );
}

void CUtlBuffer::GetFloat( float& f )
{
	GET_TYPE( float, f );
}

void CUtlBuffer::GetString( char* pString )
{
	int len = strlen( &m_pBuffer[m_Get] );
	Get( pString, len + 1 );
}

void const* CUtlBuffer::PeekGet() const
{
	return &m_pBuffer[m_Get];
}

bool OpenRecordingFile( char const* pFileName );
bool ReadNextCommand( CUtlBuffer& buffer );
void CloseRecordingFile();


//-----------------------------------------------------------------------------
// Timing
//-----------------------------------------------------------------------------

static double g_ClockFreq;
static double s_MaxStall;
static double s_TotalStall;

void InitClock( void )
{
	DWORD t, start, end;

	t = GetTickCount() + 1;
	while (GetTickCount() < t) {}

	t = GetTickCount() + 1000;
	__asm
	{
		rdtsc
		mov [start], eax
	}
	while (GetTickCount() < t);
	__asm
	{
		rdtsc
		mov [end], eax
	}

	g_ClockFreq = (double)(end - start) / 1.0;
}

double GetTime( void )
{
	DWORD hi, lo;

	__asm
	{
		rdtsc
		mov [hi], edx
		mov [lo], eax
	}

	return ((double)hi * 4294967296.0 + (double)lo) / g_ClockFreq;
}



//-----------------------------------------------------------------------------
// structs..
//-----------------------------------------------------------------------------

enum TextureLookupFlags_t
{
	TEXTURE_LOOKUP_FLAGS_IS_DEPTH_STENCIL = 0x1,
};

struct TextureLookup_t
{
	int m_BindId;
	union
	{
		IDirect3DBaseTexture* m_pTexture;
		IDirect3DBaseTexture** m_ppTexture;
		IDirect3DSurface*		m_pDepthStencilSurface;
	};
	int m_NumCopies;

	unsigned char flags;
	bool operator==( TextureLookup_t const& src ) const
	{
		return m_BindId == src.m_BindId;
	}
	TextureLookup_t()
	{
		flags = 0;
		m_pTexture = NULL;
		m_pDepthStencilSurface = NULL;
		m_ppTexture = NULL;
	}
	~TextureLookup_t()
	{
		if( flags & TEXTURE_LOOKUP_FLAGS_IS_DEPTH_STENCIL )
		{
			if( m_pDepthStencilSurface )
			{
				m_pDepthStencilSurface->Release();
			}
		}
		else if (m_NumCopies == 1)
		{
			if (m_pTexture)
				m_pTexture->Release();
		}
		else
		{
			if (m_ppTexture)
			{
				for (int j = 0; j < m_NumCopies; ++j)
				{
					if (m_ppTexture[j])
						m_ppTexture[j]->Release();
				}
				delete[] m_ppTexture;
			}
		}
	}
};

struct VertexShaderLookup_t
{
	int m_Id;
	HardwareVertexShader_t m_VertexShader;

	bool operator==( VertexShaderLookup_t const& src ) const
	{
		return m_Id == src.m_Id;
	}
};

struct PixelShaderLookup_t
{
	int m_Id;
	HardwarePixelShader_t m_PixelShader;

	bool operator==( PixelShaderLookup_t const& src ) const
	{
		return m_Id == src.m_Id;
	}
};

struct VertexBufferLookup_t
{
	int m_Id;
	LPDIRECT3DVERTEXBUFFER m_pVB;
	void* m_pLockedMemory;
	int m_Dynamic;
#ifdef CHECK_INDICES
	UINT m_LengthInBytes;
	int m_VertexSize;
#endif

	bool operator==( VertexBufferLookup_t const& src ) const
	{
		return m_Id == src.m_Id;
	}
	VertexBufferLookup_t()
	{
		m_pVB = NULL;
	}
	~VertexBufferLookup_t()
	{
		if( m_pVB )
		{
			m_pVB->Release();
		}
	}
};

// This is only used for "-list"
struct VertexBufferFormatLookup_t
{
	int m_Id;
	unsigned int m_VertexFormat;

	bool operator==( VertexBufferFormatLookup_t const& src ) const
	{
		return m_Id == src.m_Id;
	}
};

struct IndexBufferLookup_t
{
	int m_Id;
	LPDIRECT3DINDEXBUFFER m_pIB;
	void* m_pLockedMemory;
	int m_Dynamic;
#ifdef CHECK_INDICES
	UINT m_LengthInBytes;
	unsigned short *m_pIndices;
	UINT m_LockedOffsetInBytes;
	UINT m_LockedSizeInBytes;
#endif

	bool operator==( IndexBufferLookup_t const& src ) const
	{
		return m_Id == src.m_Id;
	}
	IndexBufferLookup_t()
	{
		m_pIB = NULL;
#ifdef CHECK_INDICES
		m_pIndices = NULL;
		m_LengthInBytes = -1;
		m_LockedOffsetInBytes = 0;
		m_LockedSizeInBytes = 0;
#endif
	}
	~IndexBufferLookup_t()
	{
		if( m_pIB )
		{
			m_pIB->Release();
		}
	}
};

struct VertexDeclLookup_t
{
	int m_Id;
	IDirect3DVertexDeclaration9 *m_pDecl;

	bool operator==( VertexDeclLookup_t const& src ) const
	{
		return m_Id == src.m_Id;
	}
};

//-----------------------------------------------------------------------------
// globals..
//-----------------------------------------------------------------------------

static inline bool VertexBufferLess( const VertexBufferLookup_t& vb1, const VertexBufferLookup_t& vb2 )
{
	return vb1.m_Id < vb2.m_Id;
}

static inline bool IndexBufferLess( const IndexBufferLookup_t& ib1, const IndexBufferLookup_t& ib2 )
{
	return ib1.m_Id < ib2.m_Id;
}

static inline bool TextureLess( const TextureLookup_t& t1, const TextureLookup_t& t2 )
{
	return t1.m_BindId < t2.m_BindId;
}

static inline bool VertexDeclLess( const VertexDeclLookup_t& t1, const VertexDeclLookup_t& t2 )
{
	return t1.m_Id < t2.m_Id;
}

static char g_pCommandLine[1024];
static int g_ArgC = 0;
static char* g_ppArgV[32];
static HWND g_HWnd = 0;
static IDirect3D* g_pD3D = 0;
static IDirect3DDevice* g_pD3DDevice = 0;
static FILE* g_pRecordingFile = 0;
static CUtlRBTree<TextureLookup_t,int> g_Texture( 0, 256, TextureLess );
static CUtlVector<VertexShaderLookup_t> g_VertexShader; 
static CUtlVector<PixelShaderLookup_t> g_PixelShader;
static CUtlRBTree<VertexBufferLookup_t,int> g_VertexBuffer( 0, 256, VertexBufferLess );
static CUtlVector<VertexBufferFormatLookup_t> g_VertexBufferFormat;
static CUtlRBTree<IndexBufferLookup_t,int> g_IndexBuffer( 0, 256, IndexBufferLess );
static CUtlRBTree<VertexDeclLookup_t,int> g_VertexDeclBuffer( 0, 256, VertexDeclLess );
static char const* g_pOutputFile;
static bool g_SuppressOutputDynamicMesh = false;
static bool g_ValidStreamSource[MAX_NUM_STREAMS] = { false, false };
static int g_CurrentIndexSource = -1;
static bool g_ValidIndices = false;
static bool g_CrashWrite = false;
static CUtlVector<int> g_UsedVertexBuffers;
static CUtlVector<int> g_UsedIndexBuffers;
static bool g_InteractiveMode = false;
static bool g_Paused = false;
static int g_WindowMode = -1;
static bool g_Exiting = false;
static bool g_WhiteTextures = false;
static int g_KeyFrame = -1;
static bool g_StopNextFrame = false;
static int g_VertexProcessing = 0;
static bool g_ForceRefRast = false;
static bool g_ForceNoRefRast = false;
static IDirect3DSurface *g_pBackBufferSurface = NULL;
static IDirect3DSurface *g_pZBufferSurface = NULL;
static bool g_UsingTextureRenderTarget = false;
static bool g_FrameDump = false;
static int g_FrameDumpStart = -1;
static int g_FrameDumpEnd = -1;
static int g_CurrentCmd = 0;

bool IsSuppressingCommands()
{
	return g_KeyFrame >= 0;
}

int D3DFormatSize( D3DFORMAT format )
{
	switch(format)
	{
	case D3DFMT_R8G8B8:
		return 3;
	case D3DFMT_A8R8G8B8:
		return 4;
	case D3DFMT_X8R8G8B8:
		return 4;
	case D3DFMT_R5G6B5:
		return 2;
	case D3DFMT_X1R5G5B5:
		return 2;
	case D3DFMT_A4R4G4B4:
		return 2;
	case D3DFMT_L8:
		return 1;
	case D3DFMT_A8L8:
		return 2;
	case D3DFMT_P8:
		return 1;
	case D3DFMT_A8:
		return 1;
	}

	// Bogus baby!
	return 0;
}

//-----------------------------------------------------------------------------
// convert back and forth from D3D format to ImageFormat, regardless of
// whether it's supported or not
//-----------------------------------------------------------------------------

static ImageFormat D3DFormatToImageFormat( D3DFORMAT format )
{
	switch(format)
	{
	case D3DFMT_R8G8B8:
		return IMAGE_FORMAT_BGR888;
	case D3DFMT_A8R8G8B8:
		return IMAGE_FORMAT_BGRA8888;
	case D3DFMT_X8R8G8B8:
		return IMAGE_FORMAT_BGRX8888;
	case D3DFMT_R5G6B5:
		return IMAGE_FORMAT_BGR565;
	case D3DFMT_X1R5G5B5:
		return IMAGE_FORMAT_BGRX5551;
	case D3DFMT_A4R4G4B4:
		return IMAGE_FORMAT_BGRA4444;
	case D3DFMT_L8:
		return IMAGE_FORMAT_I8;
	case D3DFMT_A8L8:
		return IMAGE_FORMAT_IA88;
	case D3DFMT_P8:
		return IMAGE_FORMAT_P8;
	case D3DFMT_A8:
		return IMAGE_FORMAT_A8;
	case D3DFMT_DXT1:
		return IMAGE_FORMAT_DXT1;
	case D3DFMT_DXT3:
		return IMAGE_FORMAT_DXT3;
	case D3DFMT_DXT5:
		return IMAGE_FORMAT_DXT5;
	case D3DFMT_Q8W8V8U8:
		return IMAGE_FORMAT_UVWQ8888;
	}

	Assert( 0 );
	// Bogus baby!
	return (ImageFormat)-1;
}

static D3DFORMAT ImageFormatToD3DFormat( ImageFormat format )
{
	// This doesn't care whether it's supported or not
	switch(format)
	{
	case IMAGE_FORMAT_BGR888:
		return D3DFMT_R8G8B8;
	case IMAGE_FORMAT_BGRA8888:
		return D3DFMT_A8R8G8B8;
	case IMAGE_FORMAT_BGRX8888:
		return D3DFMT_X8R8G8B8;
	case IMAGE_FORMAT_BGR565:
		return D3DFMT_R5G6B5;
	case IMAGE_FORMAT_BGRX5551:
		return D3DFMT_X1R5G5B5;
	case IMAGE_FORMAT_BGRA5551:
		return D3DFMT_A1R5G5B5;
	case IMAGE_FORMAT_BGRA4444:
		return D3DFMT_A4R4G4B4;
	case IMAGE_FORMAT_I8:
		return D3DFMT_L8;
	case IMAGE_FORMAT_IA88:
		return D3DFMT_A8L8;
	case IMAGE_FORMAT_P8:
		return D3DFMT_P8;
	case IMAGE_FORMAT_A8:
		return D3DFMT_A8;
	case IMAGE_FORMAT_DXT1:
	case IMAGE_FORMAT_DXT1_ONEBITALPHA:
		return D3DFMT_DXT1;
	case IMAGE_FORMAT_DXT3:
		return D3DFMT_DXT3;
	case IMAGE_FORMAT_DXT5:
		return D3DFMT_DXT5;
	}

	Assert( 0 );
	return (D3DFORMAT)-1;
}

//-----------------------------------------------------------------------------
// Initializes with checkerboard
//-----------------------------------------------------------------------------

void InitCheckerboard( IDirect3DSurface* pSurface, int width, int height, D3DFORMAT format )
{
	if( format == D3DFMT_D16 )
	{
		return;
	}

	// choose two random colors....
	unsigned char c1[4];
	unsigned char c2[4];

	if (!g_WhiteTextures)
	{
		for (int c = 0; c < 4; ++c)
		{
			c1[c] = rand() & 0xFF;
			c2[c] = rand() & 0xFF;
		}
	}
	else
	{
		c1[0] = c1[1] = c1[2] = c1[3] = 128;
		c2[0] = c2[1] = c2[2] = c2[3] = 128;
	}

	unsigned char *pImage = new unsigned char[width*height*4];
	
	for (int i = 0; i < height; ++i)
	{
		unsigned char *pBits = pImage + i * width * 4;
		for (int j = 0; j < width; ++j)
		{
			pBits[0] = ((i&16)^(j&16)) ? c1[0] : c2[0];
			pBits[1] = ((i&16)^(j&16)) ? c1[1] : c2[1];
			pBits[2] = ((i&16)^(j&16)) ? c1[2] : c2[2];
			pBits[3] = ((i&16)^(j&16)) ? 128 : 255;
			pBits += 4;
		}
	}

	ImageFormat imageFormat = D3DFormatToImageFormat( format );
	
	int size = D3DFormatSize( format );

	D3DLOCKED_RECT rect;
	HRESULT hr = pSurface->LockRect( &rect, 0, D3DLOCK_NOSYSLOCK );
	Assert( !FAILED(hr) );

	ImageLoader::ConvertImageFormat( pImage, IMAGE_FORMAT_RGBA8888, 
		( unsigned char * )rect.pBits, imageFormat,
		width, height, 0, 0/*rect.Pitch*/ );

	pSurface->UnlockRect();

	delete [] pImage;
}


//-----------------------------------------------------------------------------
// Creates a texture.
//-----------------------------------------------------------------------------
IDirect3DBaseTexture* CreateTexture( int width, int height, D3DFORMAT format,
										int numLevels, int isCubeMap, int isRenderTarget, 
										int isManaged, int isDepthBuffer, int isDynamic )
{
#if 0
	// FIXME: Can't make checkerboard compressed textures 
	if ((format == D3DFMT_DXT1) || (format == D3DFMT_DXT3) || (format == D3DFMT_DXT5))
		format = D3DFMT_A8R8G8B8;
#endif

	DWORD usage = 0;
	if (isDepthBuffer)
	{
		usage |= D3DUSAGE_DEPTHSTENCIL;

		// FIXME: At the moment this is hardcoded this way in the engine
		// It won't always be that way
		format = D3DFMT_D16;
	}
	else if( isRenderTarget )
	{
		usage |= D3DUSAGE_RENDERTARGET;
	}
	if ( isDynamic )
	{
		usage |= D3DUSAGE_DYNAMIC;
	}

	HRESULT hr;
	if( isCubeMap )
	{
		IDirect3DCubeTexture* pD3DCubeTexture = 0;
		hr = g_pD3DDevice->CreateCubeTexture( 
			width, numLevels, usage, format,  isManaged ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT, 
			&pD3DCubeTexture
			,NULL 
			);
		Assert( !FAILED(hr) );

		if( !isRenderTarget )
		{
			// Initialize with checkerboards
			for (int i = 0; i < 6; ++i)
			{
				IDirect3DSurface* pSurfLevel;
				HRESULT hr = pD3DCubeTexture->GetCubeMapSurface( (D3DCUBEMAP_FACES)i, 0, &pSurfLevel );
				Assert( !FAILED(hr) );
				InitCheckerboard( pSurfLevel, width, height, format );
				pSurfLevel->Release();
			}
		}

		D3DXFilterCubeTexture( pD3DCubeTexture, 0, 0, D3DX_FILTER_LINEAR );
		
		return pD3DCubeTexture;
	}
	else
	{
		IDirect3DTexture* pD3DTexture = 0;
		hr = g_pD3DDevice->CreateTexture( width, height, numLevels, 
			usage, format,  isManaged ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT, 
			&pD3DTexture
			,NULL 
			);
		Assert( !FAILED(hr) );

		if( !isRenderTarget )
		{
			// Initialize with checkerboards
			IDirect3DSurface* pSurfLevel;
			hr = pD3DTexture->GetSurfaceLevel( 0, &pSurfLevel );
			Assert( !FAILED(hr) );
//			InitCheckerboard( pSurfLevel, width, height, format );
			pSurfLevel->Release();
		}

		D3DXFilterTexture( pD3DTexture, 0, 0, D3DX_FILTER_LINEAR );

		return pD3DTexture;
	}
}


//-----------------------------------------------------------------------------
// Pump windows messages
//-----------------------------------------------------------------------------

void PumpWindowsMessages ()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == TRUE) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	InvalidateRect(g_HWnd, NULL, false);
	UpdateWindow(g_HWnd);
}


//-----------------------------------------------------------------------------
// Resize the window based on the presentation parameters
//-----------------------------------------------------------------------------

void ResizeWindow( int width, int height )
{
    int     centerX, centerY, borderX, borderY;
	borderX = (GetSystemMetrics(SM_CXFIXEDFRAME) + 1) * 2;
	borderY = (GetSystemMetrics(SM_CYFIXEDFRAME) + 1) * 2 + GetSystemMetrics(SM_CYSIZE) + 1;
	centerX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	centerY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	centerX = (centerX < 0) ? 0 : centerX;
	centerY = (centerY < 0) ? 0 : centerY;

	SetWindowPos (g_HWnd, NULL, centerX, centerY, width + borderX, height + borderY,
				  SWP_NOZORDER | SWP_SHOWWINDOW );

	PumpWindowsMessages();
}

//-----------------------------------------------------------------------------
// Grab/release the render targets
//-----------------------------------------------------------------------------
void AcquireRenderTargets()
{
	if (!g_pBackBufferSurface)
	{
		g_pD3DDevice->GetRenderTarget( 0, &g_pBackBufferSurface );
		Assert( g_pBackBufferSurface );
	}

	if (!g_pZBufferSurface)
	{
		g_pD3DDevice->GetDepthStencilSurface( &g_pZBufferSurface );
		Assert( g_pZBufferSurface );
	}
}

void ReleaseRenderTargets()
{
	if (g_pBackBufferSurface)
	{
		g_pBackBufferSurface->Release();
		g_pBackBufferSurface = NULL;
	}

	if (g_pZBufferSurface)
	{
		g_pZBufferSurface->Release();
		g_pZBufferSurface = NULL;
	}
}

//-----------------------------------------------------------------------------
// Grab/release frame sync objects
//-----------------------------------------------------------------------------
IDirect3DQuery9 *g_pFrameSyncQueryObject = NULL;

void AllocFrameSyncObjects( void )
{
	HRESULT hr = g_pD3DDevice->CreateQuery( D3DQUERYTYPE_EVENT, &g_pFrameSyncQueryObject );
	if( hr == D3DERR_NOTAVAILABLE )
	{
		Warning( "D3DQUERYTYPE_EVENT not available on this driver\n" );
		Assert( g_pFrameSyncQueryObject == NULL );
	}
	else
	{
		Assert( hr == D3D_OK );
		Assert( g_pFrameSyncQueryObject );
		g_pFrameSyncQueryObject->Issue( D3DISSUE_END );
	}
}

void FreeFrameSyncObjects( void )
{
	if( !g_pFrameSyncQueryObject )
	{
		return;
	}
	g_pFrameSyncQueryObject->Release();
	g_pFrameSyncQueryObject = NULL;
}


//-----------------------------------------------------------------------------
// DX8 functions..
//-----------------------------------------------------------------------------

typedef void (*Dx8Func_t)( int numargs, CUtlBuffer& data );

void Dx8CreateDevice( int numargs, CUtlBuffer& data )
{
	if( numargs == 6 )
	{
		int displayAdapter, width, height, deviceCreationFlags;
		D3DDEVTYPE deviceType;
		D3DPRESENT_PARAMETERS presentParameters;

		data.GetInt( width );
		data.GetInt( height );
		data.GetInt( displayAdapter );
		data.GetInt( *(int*)&deviceType );
		data.GetInt( deviceCreationFlags );
		data.Get( &presentParameters, sizeof(presentParameters) );
		presentParameters.hDeviceWindow = g_HWnd;
		if( g_ForceRefRast )
		{
			deviceType = D3DDEVTYPE_REF;
		}
		if( g_ForceNoRefRast )
		{
			deviceType = D3DDEVTYPE_HAL;
		}

		// Resize the window based on the presentation parameters
		ResizeWindow( width, height );

		// Override window mode
		if (g_WindowMode >= 0)
		{
			presentParameters.Windowed = (g_WindowMode == 1);
			presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		}

        presentParameters.BackBufferWidth  = width;
        presentParameters.BackBufferHeight = height;

		presentParameters.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

		// command-line override
		if (g_VertexProcessing == 1)
		{
			deviceCreationFlags &= ~(D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE);
			deviceCreationFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		}
		else if (g_VertexProcessing == 2)
		{
			deviceCreationFlags &= ~(D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE);
			deviceCreationFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
		}
		else if (g_VertexProcessing == 3)
		{
			deviceCreationFlags &= ~D3DCREATE_SOFTWARE_VERTEXPROCESSING;
			deviceCreationFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
		}

		D3DPRESENT_PARAMETERS oldPresentParameters;
		memcpy( &oldPresentParameters, &presentParameters, sizeof(D3DPRESENT_PARAMETERS) );

		HRESULT hr = g_pD3D->CreateDevice( displayAdapter, deviceType,
			g_HWnd, deviceCreationFlags, &presentParameters, &g_pD3DDevice );
		Assert( !FAILED(hr) );

		bool bSupportsASyncQuery = false;

		// Check for asynch query support
		IDirect3DQuery9 *pQueryObject = NULL;

		// Detect whether query is supported by creating and releasing:
		hr = g_pD3DDevice->CreateQuery( D3DQUERYTYPE_EVENT, &pQueryObject );
		if( pQueryObject )
		{ 
			pQueryObject->Release();
		}

		// Re-create the device without a lockable backbuffer if asynch query is supported
		if( hr != D3DERR_NOTAVAILABLE )
		{
			g_pD3DDevice->Release();

			// Give the system a little time to recover:
			Sleep(1000);

			oldPresentParameters.Flags &= ~D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

			hr = g_pD3D->CreateDevice( displayAdapter, deviceType,
				g_HWnd, deviceCreationFlags, &oldPresentParameters, &g_pD3DDevice );
	 		Assert( !FAILED(hr) );

			bSupportsASyncQuery = true;
		}

#ifdef STUBD3D
		g_pD3DDevice = new CStubD3DDevice( g_pD3DDevice, NULL );
#endif
		
		AcquireRenderTargets();

		if ( bSupportsASyncQuery )
		{
			AllocFrameSyncObjects();
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8DestroyDevice( int numargs, CUtlBuffer& data )
{
	FreeFrameSyncObjects();
	ReleaseRenderTargets();

	if (g_pD3DDevice)
		g_pD3DDevice->Release();
}

void Dx8Reset( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		D3DPRESENT_PARAMETERS presentParamters;
		data.Get( &presentParamters, sizeof(presentParamters) );

		HRESULT hr = g_pD3DDevice->Reset( &presentParamters );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8ShowCursor( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		int show;
		data.GetInt( show );

		HRESULT hr = g_pD3DDevice->ShowCursor( show );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8BeginScene( int numargs, CUtlBuffer& data )
{	
	if (g_pD3DDevice && ( numargs == 0 ))
	{
		HRESULT hr = g_pD3DDevice->BeginScene( );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8EndScene( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 0 ))
	{
		HRESULT hr = g_pD3DDevice->EndScene( );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}



static void WriteFrameToTGA( const char *pFileName )
{
	HRESULT hr;
	IDirect3DSurface* pSurfaceBits = 0;

	// Get the back buffer
	IDirect3DSurface* pBackBuffer;
	hr = g_pD3DDevice->GetRenderTarget( 0, &pBackBuffer );
	Assert( !FAILED( hr ) );

	// Find about its size and format
	D3DSURFACE_DESC desc;
	hr = pBackBuffer->GetDesc( &desc );
	Assert( !FAILED( hr ) );

	// Create a buffer the same size and format
	hr = g_pD3DDevice->CreateOffscreenPlainSurface( desc.Width, desc.Height, 
		desc.Format, D3DPOOL_SYSTEMMEM, &pSurfaceBits, NULL );
	Assert( !FAILED( hr ) );

	// Blit from the back buffer to our scratch buffer
	hr = g_pD3DDevice->GetRenderTargetData( pBackBuffer, pSurfaceBits );
	if (FAILED(hr))
		return;
	Assert( !FAILED( hr ) );

	ImageFormat format = D3DFormatToImageFormat( desc.Format );
	
	int memRequired = ImageLoader::GetMemRequired( desc.Width, desc.Height, IMAGE_FORMAT_RGB888, false );
	unsigned char *pImage = new unsigned char[memRequired];
	
	D3DLOCKED_RECT lockedRect;
	hr = pSurfaceBits->LockRect( &lockedRect, NULL, D3DLOCK_READONLY );
	Assert( !FAILED( hr ) );

	ImageLoader::ConvertImageFormat( ( unsigned char * )lockedRect.pBits, format, pImage, IMAGE_FORMAT_RGB888, desc.Width, desc.Height, lockedRect.Pitch, 0 );

	TGAWriter::Write( pImage, pFileName, desc.Width, desc.Height, IMAGE_FORMAT_RGB888, IMAGE_FORMAT_RGB888 );

	delete [] pImage;
	
	pSurfaceBits->UnlockRect();
	Assert( !FAILED( hr ) );

	pSurfaceBits->Release();
	
	pBackBuffer->Release();
}

void Dx8Present( int numargs, CUtlBuffer& data )
{
	if( g_FrameDump && 
		( g_FrameDumpStart == -1 || g_FrameDumpEnd == -1 || 
		( g_FrameNum >= g_FrameDumpStart && g_FrameNum <= g_FrameDumpEnd ) ) )
	{
		char filename[128];
		sprintf( filename, "playback%04d.tga", g_FrameNum );
		WriteFrameToTGA( filename );
	}
	
	if (g_pD3DDevice && ( numargs == 0 ))
	{
		HRESULT hr = g_pD3DDevice->Present( 0, 0, 0, 0 );
		Assert( !FAILED(hr) );
	}
	else if (g_pD3DDevice && ( numargs == 2 ))
	{
		RECT srcRect, destRect;
		data.Get( &srcRect, sizeof(srcRect) );
		data.Get( &destRect, sizeof(destRect) );

		HRESULT hr = g_pD3DDevice->Present( &srcRect, &destRect, g_HWnd, 0 );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}

	g_FrameNum++;

	// We're in paused mode here if we're in interactive mode
	if (g_InteractiveMode)
		g_Paused = true;

	// Stop after the keyframe
	if (g_StopNextFrame)
		g_Exiting = true;

#if 0
	char buf[64];
	sprintf(buf, "max %.3f total %.3f\n", s_MaxStall, s_TotalStall );
	OutputDebugString( buf );
#endif
	s_MaxStall = 0;
	s_TotalStall = 0;

	// Pump windows messages too...
    PumpWindowsMessages();
}

void Dx8Keyframe( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 0 ))
	{
		if (g_KeyFrame >= 0)
		{
			--g_KeyFrame;
			if (g_KeyFrame < 0)
				g_StopNextFrame = true;
		}
		else
		{
//			g_Paused = true;
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8CreateTexture( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 7 ) || ( numargs == 10 ) || ( numargs == 11 ) )
	{
		TextureLookup_t texture;
		int width, height, numLevels, isCubeMap, numCopies;
		int isDepthBuffer, isRenderTarget, isManaged, isDynamic;
		D3DFORMAT format;

	    data.GetInt( texture.m_BindId );
	    data.GetInt( width );
	    data.GetInt( height );
	    data.GetInt( *(int*)&format );
	    data.GetInt( numLevels );
	    data.GetInt( isCubeMap );
	    data.GetInt( numCopies );
		switch( numargs )
		{
		case 11:
			data.GetInt( isRenderTarget );
			data.GetInt( isManaged );
			data.GetInt( isDepthBuffer );
			data.GetInt( isDynamic );
			break;

		case 10:
			data.GetInt( isRenderTarget );
			data.GetInt( isManaged );
			data.GetInt( isDepthBuffer );
			isDynamic = 0;
			break;

		case 7:
			isRenderTarget = 0;
			isManaged = 1;
			isDepthBuffer = 0;
			isDynamic = 0;
			break;
		}

		int i = g_Texture.Find( texture );
		if (i < 0)
			i = g_Texture.Insert( texture );

		g_Texture[i].m_NumCopies = numCopies;
		if (numCopies == 1)
		{
			g_Texture[i].m_pTexture = CreateTexture( width, height, format, 
				numLevels, isCubeMap, isRenderTarget, isManaged, isDepthBuffer, isDynamic );
		}
		else
		{
			g_Texture[i].m_ppTexture = new IDirect3DBaseTexture* [numCopies];
			for (int j = 0; j < numCopies; ++j)
				g_Texture[i].m_ppTexture[j] = CreateTexture( width, height,
				format, numLevels, isCubeMap, isRenderTarget, isManaged, isDepthBuffer, isDynamic );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8DestroyTexture( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		TextureLookup_t texture;
	    data.GetInt( texture.m_BindId );

		int i = g_Texture.Find( texture );
		if (i >= 0)
		{
			g_Texture.RemoveAt( i );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetTexture( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 3 ))
	{
		TextureLookup_t texture;
		int stage, copy;
		data.GetInt( stage );
	    data.GetInt( texture.m_BindId );
		data.GetInt( copy );

		int i = g_Texture.Find( texture );
		if (i >= 0)
		{
			HRESULT hr;
			if (g_Texture[i].m_NumCopies > 1)
				hr = g_pD3DDevice->SetTexture( stage, g_Texture[i].m_ppTexture[copy] );
			else				
				hr = g_pD3DDevice->SetTexture( stage, g_Texture[i].m_pTexture );
			Assert( !FAILED(hr) );
		}
		else
		{
			HRESULT hr = g_pD3DDevice->SetTexture( stage, 0 );
			Assert( !FAILED(hr) );
		}
	}
	else
	{
		Assert( 0 );
	}
}

static HRESULT GetSurfaceFromTexture( IDirect3DBaseTexture* pBaseTexture, UINT level, 
									  D3DCUBEMAP_FACES cubeFaceID, IDirect3DSurface** ppSurfLevel )
{
	HRESULT hr;

	switch( pBaseTexture->GetType() )
	{
	case D3DRTYPE_TEXTURE:
		hr = ( ( IDirect3DTexture * )pBaseTexture )->GetSurfaceLevel( level, ppSurfLevel );
		break;
	case D3DRTYPE_CUBETEXTURE:
		hr = ( ( IDirect3DCubeTexture * )pBaseTexture )->GetCubeMapSurface( cubeFaceID, level, ppSurfLevel );
		break;
	default:
		return ( HRESULT )-1;
		break;
	}
	return hr;
}


static RECT s_LockedSrcRect;
static D3DLOCKED_RECT s_LockedRect;
void Dx8LockTexture( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && (( numargs == 5 ) || ( numargs == 6 )) )
	{
		TextureLookup_t texture;
		int lockFlags, copy, level, cubeFace;

	    data.GetInt( texture.m_BindId );
		data.GetInt( copy );
		data.GetInt( level );
		data.GetInt( cubeFace );
		if (numargs == 6)
			data.Get( &s_LockedSrcRect, sizeof(s_LockedSrcRect) );
	    data.GetInt( lockFlags );

		g_LockedTextureID = texture.m_BindId;
		
		int i = g_Texture.Find( texture );
		if (i >= 0)
		{
#ifdef _DEBUG
			TextureLookup_t *pTextureLookup = &g_Texture[i];
#endif
			IDirect3DBaseTexture* pTex;
			if (g_Texture[i].m_NumCopies > 1)
				pTex = g_Texture[i].m_ppTexture[copy];
			else				
				pTex = g_Texture[i].m_pTexture;

			HRESULT hr;
			IDirect3DSurface* pSurface;
			hr = GetSurfaceFromTexture( pTex, level, (D3DCUBEMAP_FACES)cubeFace, &pSurface );
			Assert( !FAILED(hr) );

			if (numargs == 5)
				hr = pSurface->LockRect( &s_LockedRect, 0, lockFlags );
			else
				hr = pSurface->LockRect( &s_LockedRect, &s_LockedSrcRect, lockFlags );
			Assert( !FAILED(hr) );

			pSurface->Release();
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8UnlockTexture( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 4 ))
	{
		TextureLookup_t texture;
		int copy, level, cubeFace;

	    data.GetInt( texture.m_BindId );
		data.GetInt( copy );
		data.GetInt( level );
		data.GetInt( cubeFace );

		Assert( g_LockedTextureID == texture.m_BindId );
		g_LockedTextureID = -1;

		int i = g_Texture.Find( texture );
		if (i >= 0)
		{
			IDirect3DBaseTexture* pTex;
			if (g_Texture[i].m_NumCopies > 1)
				pTex = g_Texture[i].m_ppTexture[copy];
			else				
				pTex = g_Texture[i].m_pTexture;

			HRESULT hr;
			IDirect3DSurface* pSurface;
			hr = GetSurfaceFromTexture( pTex, level, (D3DCUBEMAP_FACES)cubeFace, &pSurface );
			Assert( !FAILED(hr) );

			hr = pSurface->UnlockRect(  );
			Assert( !FAILED(hr) );

			pSurface->Release();
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetTransform( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 2 ))
	{
		D3DTRANSFORMSTATETYPE transform;
		D3DXMATRIX matrix;

	    data.GetInt( *(int*)&transform );
		data.Get( &matrix, sizeof(matrix) );

		HRESULT hr = g_pD3DDevice->SetTransform( transform, &matrix );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8CreateVertexShader( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 2 ))
	{
		// This is the FVF version. . ie. no byte code
		VertexShaderLookup_t vertexShader;
		DWORD dwDecl[32];

	    data.GetInt( vertexShader.m_Id );
		data.Get( dwDecl, 32 * sizeof(DWORD) );

		int i = g_VertexShader.Find( vertexShader );
		if (i < 0)
		{
			i = g_VertexShader.AddToTail( vertexShader );
		}
		else
		{
			Assert( 0 );
		}

		HRESULT hr = g_pD3DDevice->CreateVertexShader( ( DWORD* )NULL, &g_VertexShader[i].m_VertexShader );
		Assert( !FAILED(hr) );
	}
	else if (g_pD3DDevice && ( numargs == 3 ))
	{
		// This is the vertex shader version, ie. we have byte code
		VertexShaderLookup_t vertexShader;
		int codeSize;

	    data.GetInt( vertexShader.m_Id );
		data.GetInt( codeSize );

		int i = g_VertexShader.Find( vertexShader );
		if (i < 0)
		{
			i = g_VertexShader.AddToTail( vertexShader );
		}
		else
		{
			Assert( 0 );
		}

		HRESULT hr = g_pD3DDevice->CreateVertexShader( ( DWORD* )data.PeekGet(), 
			&g_VertexShader[i].m_VertexShader );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8CreatePixelShader( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 3 ))
	{
		PixelShaderLookup_t pixelShader;
		int codeSize;

	    data.GetInt( pixelShader.m_Id );
		data.GetInt( codeSize );

		int i = g_PixelShader.Find( pixelShader );
		if (i < 0)
			i = g_PixelShader.AddToTail( pixelShader );

		HRESULT hr = g_pD3DDevice->CreatePixelShader( 
			( DWORD* )data.PeekGet(), &g_PixelShader[i].m_PixelShader );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8DestroyVertexShader( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		VertexShaderLookup_t vertexShader;
	    data.GetInt( vertexShader.m_Id );

		int i = g_VertexShader.Find( vertexShader );
		if (i >= 0)
		{
			g_VertexShader[i].m_VertexShader->Release();
			g_VertexShader.Remove( i );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8DestroyPixelShader( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		PixelShaderLookup_t pixelShader;
	    data.GetInt( pixelShader.m_Id );

		int i = g_PixelShader.Find( pixelShader );
		if (i >= 0)
		{
			pixelShader.m_PixelShader->Release();
			g_PixelShader.Remove( i );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetVertexShader( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		VertexShaderLookup_t vertexShader;
	    data.GetInt( vertexShader.m_Id );

		int i = g_VertexShader.Find( vertexShader );
		if (i >= 0)
		{
			g_pD3DDevice->SetVertexShader( g_VertexShader[i].m_VertexShader );
		}
		else
		{
			g_pD3DDevice->SetVertexShader( NULL );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetPixelShader( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		PixelShaderLookup_t pixelShader;
	    data.GetInt( pixelShader.m_Id );

		int i = g_PixelShader.Find( pixelShader );
		if (i >= 0)
		{
			g_pD3DDevice->SetPixelShader( g_PixelShader[i].m_PixelShader );
		}
		else
		{
			g_pD3DDevice->SetPixelShader( NULL );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetVertexShaderConstant( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 3 ))
	{
		int numVecs, var;
		data.GetInt( var );
		data.GetInt( numVecs );
		HRESULT hr = g_pD3DDevice->SetVertexShaderConstantF( var, ( const float * )data.PeekGet(), numVecs	);
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetPixelShaderConstant( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 3 ))
	{
		int numVecs, var;
		data.GetInt( var );
		data.GetInt( numVecs );
		HRESULT hr = g_pD3DDevice->SetPixelShaderConstantF( var, ( const float * )data.PeekGet(), numVecs );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetMaterial( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		D3DMATERIAL mat;
		data.Get( &mat, sizeof(mat) );
		HRESULT hr = g_pD3DDevice->SetMaterial( &mat );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8LightEnable( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 2 ))
	{
		int lightNum, enable;
		data.GetInt( lightNum );
		data.GetInt( enable );
		HRESULT hr = g_pD3DDevice->LightEnable( lightNum, enable );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetLight( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 2 ))
	{
		int lightNum;
		D3DLIGHT light;
		data.GetInt( lightNum );
		data.Get( &light, sizeof(light) );
		HRESULT hr = g_pD3DDevice->SetLight( lightNum, &light );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetViewport( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		D3DVIEWPORT viewport;
		data.Get( &viewport, sizeof(viewport) );
		HRESULT hr = g_pD3DDevice->SetViewport( &viewport );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8Clear( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 6 ))
	{
		DWORD numRect, flags, stencil;
		D3DRECT rect;
		D3DCOLOR color;
		float z;

		data.GetInt( *(int*)&numRect );
		data.Get( &rect, sizeof(rect) );
		data.GetInt( *(int*)&flags );
		data.GetInt( *(int*)&color );
		data.GetFloat( z );
		data.GetInt( *(int*)&stencil );

		HRESULT hr = g_pD3DDevice->Clear( numRect, &rect, flags, color, z, stencil );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8ValidateDevice( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 0 ))
	{
		DWORD numPasses;
		HRESULT hr = g_pD3DDevice->ValidateDevice( &numPasses );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetRenderState( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 2 ))
	{
		D3DRENDERSTATETYPE state;
		DWORD value;

		data.GetInt( *(int*)&state );
		data.GetInt( *(int*)&value );

		HRESULT hr = g_pD3DDevice->SetRenderState( state, value );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetSamplerState( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 3 ))
	{
		DWORD sampler;
		D3DSAMPLERSTATETYPE type;
		DWORD value;

		data.GetInt( *(int*)&sampler );
		data.GetInt( *(int*)&type );
		data.GetInt( *(int*)&value );

		HRESULT hr = g_pD3DDevice->SetSamplerState( sampler, type, value );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetTextureStageState( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 3 ))
	{
		int stage;
		D3DTEXTURESTAGESTATETYPE state;
		DWORD value;

		data.GetInt( stage );
		data.GetInt( *(int*)&state );
		data.GetInt( *(int*)&value );

		HRESULT hr = g_pD3DDevice->SetTextureStageState( stage, state, value );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8CreateVertexBuffer( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 6 ))
	{
		VertexBufferLookup_t vertexBuffer;
		UINT length;
		DWORD usage, fvf;
		D3DPOOL pool;
		int dynamic;

	    data.GetInt( vertexBuffer.m_Id );
		data.GetInt( *(int*)&length );
		data.GetInt( *(int*)&usage );
		data.GetInt( *(int*)&fvf );
		data.GetInt( *(int*)&pool );
		data.GetInt( dynamic );
#ifdef CHECK_INDICES
		vertexBuffer.m_LengthInBytes = length;
#endif

		int i = g_VertexBuffer.Find( vertexBuffer );
		if (i < 0)
		{
			i = g_VertexBuffer.Insert( vertexBuffer );
		}

		HRESULT hr = g_pD3DDevice->CreateVertexBuffer( length, usage, fvf, pool, &g_VertexBuffer[i].m_pVB
			,NULL
			);
		Assert( !FAILED(hr) );
		g_VertexBuffer[i].m_pLockedMemory = 0;
		g_VertexBuffer[i].m_Dynamic = dynamic;
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8DestroyVertexBuffer( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		VertexBufferLookup_t vertexBuffer;
	    data.GetInt( vertexBuffer.m_Id );

		int i = g_VertexBuffer.Find( vertexBuffer );
		if (i >= 0)
		{
			g_VertexBuffer.RemoveAt( i );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8LockVertexBuffer( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 4 ))
	{
		UINT offset, size;
		DWORD flags;
		VertexBufferLookup_t vertexBuffer;

	    data.GetInt( vertexBuffer.m_Id );
		data.GetInt( *(int*)&offset );
		data.GetInt( *(int*)&size );
		data.GetInt( *(int*)&flags );

		int i = g_VertexBuffer.Find( vertexBuffer );
		if (i >= 0)
		{
			if (IsSuppressingCommands() && g_VertexBuffer[i].m_Dynamic)
				return;

			if (g_VertexBuffer[i].m_pVB && (!g_VertexBuffer[i].m_pLockedMemory) )
			{
				HRESULT hr = g_VertexBuffer[i].m_pVB->Lock( offset, size,
					(void**)&g_VertexBuffer[i].m_pLockedMemory, flags );
				Assert( !FAILED(hr) );
			}
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8VertexData( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 3 ))
	{
		int size;
		VertexBufferLookup_t vertexBuffer;

	    data.GetInt( vertexBuffer.m_Id );
		data.GetInt( *(int*)&size );

		int i = g_VertexBuffer.Find( vertexBuffer );
		if (i >= 0)
		{
			if (IsSuppressingCommands() && g_VertexBuffer[i].m_Dynamic)
				return;

			if (g_VertexBuffer[i].m_pVB && g_VertexBuffer[i].m_pLockedMemory )
			{
				memcpy( g_VertexBuffer[i].m_pLockedMemory, data.PeekGet(), size );
			}
			else
			{
				Assert( 0 );
			}
		}
		else
		{
			Assert( 0 );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8UnlockVertexBuffer( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		VertexBufferLookup_t vertexBuffer;

	    data.GetInt( vertexBuffer.m_Id );

		int i = g_VertexBuffer.Find( vertexBuffer );
		if (i >= 0)
		{
			if (IsSuppressingCommands() && g_VertexBuffer[i].m_Dynamic)
				return;

			if (g_VertexBuffer[i].m_pVB && g_VertexBuffer[i].m_pLockedMemory )
			{
				HRESULT hr = g_VertexBuffer[i].m_pVB->Unlock( );
				Assert( !FAILED(hr) );
				g_VertexBuffer[i].m_pLockedMemory = 0;
			}
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8CreateIndexBuffer( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 6 ))
	{
		IndexBufferLookup_t indexBuffer;
		UINT lengthInBytes;
		DWORD usage;
		D3DFORMAT fmt;
		D3DPOOL pool;
		int dynamic;

	    data.GetInt( indexBuffer.m_Id );
		data.GetInt( *(int*)&lengthInBytes );
		data.GetInt( *(int*)&usage );
		data.GetInt( *(int*)&fmt );
		data.GetInt( *(int*)&pool );
		data.GetInt( dynamic );

		int i = g_IndexBuffer.Find( indexBuffer );
		if (i < 0)
		{
#ifdef CHECK_INDICES
			Assert( fmt == D3DFMT_INDEX16 );
			Assert( ( lengthInBytes & 1 ) == 0 );
			indexBuffer.m_pIndices = new unsigned short[lengthInBytes>>1];
			indexBuffer.m_LengthInBytes = lengthInBytes;
//			memset( indexBuffer.m_pIndices, 0xffff, lengthInBytes );
#endif
			i = g_IndexBuffer.Insert( indexBuffer );
		}
		else
		{
			Assert( 0 );
		}

		if( g_ForcedStaticIndexBufferBytes != -1 )
		{
			if( ( usage & D3DUSAGE_DYNAMIC ) == 0 )
			{
				if( lengthInBytes < ( UINT )g_ForcedStaticIndexBufferBytes )
				{
					lengthInBytes = ( UINT )g_ForcedStaticIndexBufferBytes;
				}
			}
		}
		
		HRESULT hr = g_pD3DDevice->CreateIndexBuffer( lengthInBytes, usage, fmt, pool, &g_IndexBuffer[i].m_pIB, NULL );
		Assert( !FAILED(hr) );
		g_IndexBuffer[i].m_pLockedMemory = 0;
		g_IndexBuffer[i].m_Dynamic = dynamic;
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8DestroyIndexBuffer( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		IndexBufferLookup_t indexBuffer;
	    data.GetInt( indexBuffer.m_Id );

		int i = g_IndexBuffer.Find( indexBuffer );
		if (i >= 0)
		{
#ifdef CHECK_INDICES
			delete [] g_IndexBuffer[i].m_pIndices;
			g_IndexBuffer[i].m_pIndices = NULL;
#endif
			g_IndexBuffer.RemoveAt( i );
		}
		else
		{
			Assert( 0 );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8LockIndexBuffer( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 4 ))
	{
		UINT offset, size;
		DWORD flags;
		IndexBufferLookup_t indexBuffer;

	    data.GetInt( indexBuffer.m_Id );
		data.GetInt( *(int*)&offset );
		data.GetInt( *(int*)&size );
		data.GetInt( *(int*)&flags );

		int i = g_IndexBuffer.Find( indexBuffer );
		if (i >= 0)
		{
#ifdef CHECK_INDICES
			IndexBufferLookup_t *pIndexBuffer = &g_IndexBuffer[i];
			Assert( pIndexBuffer->m_LengthInBytes >= offset + size );
			g_IndexBuffer[i].m_LockedOffsetInBytes = offset;
			g_IndexBuffer[i].m_LockedSizeInBytes = size;
#endif
			
			if (IsSuppressingCommands() && g_IndexBuffer[i].m_Dynamic)
				return;

			if( g_ForcedStaticIndexBufferBytes != -1 )
			{
				if( !g_IndexBuffer[i].m_Dynamic )
				{
					if( size < ( UINT )g_ForcedStaticIndexBufferBytes )
					{
						size = ( UINT )g_ForcedStaticIndexBufferBytes;
					}
				}
			}
			if (g_IndexBuffer[i].m_pIB && (!g_IndexBuffer[i].m_pLockedMemory) )
			{
				HRESULT hr = g_IndexBuffer[i].m_pIB->Lock( offset, size,
					(void**)&g_IndexBuffer[i].m_pLockedMemory, 
					flags );
				Assert( !FAILED(hr) );
			}
			else
			{
				Assert( 0 );
			}
		}
		else
		{
			Assert( 0 );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8IndexData( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 3 ))
	{
		int size;
		IndexBufferLookup_t indexBuffer;

	    data.GetInt( indexBuffer.m_Id );
		data.GetInt( *(int*)&size );

		int i = g_IndexBuffer.Find( indexBuffer );
		if (i >= 0)
		{
			if (IsSuppressingCommands() && g_IndexBuffer[i].m_Dynamic)
				return;

			if (g_IndexBuffer[i].m_pIB && g_IndexBuffer[i].m_pLockedMemory )
			{
				if( g_ForcedStaticIndexBufferBytes != -1 )
				{
					int srcSize = size;
					int dstSize = size;
					if( size < g_ForcedStaticIndexBufferBytes && !g_IndexBuffer[i].m_Dynamic )
					{
						dstSize = g_ForcedStaticIndexBufferBytes;
					}
					unsigned char *pSrc = ( unsigned char * )data.PeekGet();
					unsigned char *pDst = ( unsigned char * )g_IndexBuffer[i].m_pLockedMemory;
					unsigned char *pDstEnd = pDst + dstSize;
					int copySize;
					while( pDst < pDstEnd )
					{
						copySize = ( pDstEnd - pDst < srcSize ) ? ( pDstEnd - pDst ) : srcSize;
						memcpy( pDst, pSrc, copySize );
						pDst += srcSize;
					}
				}
				else
				{
#ifdef CHECK_INDICES
					IndexBufferLookup_t *pIndexBuffer = &g_IndexBuffer[i];
					Assert( (int)size == (int)pIndexBuffer->m_LockedSizeInBytes );
					memcpy( &pIndexBuffer->m_pIndices[pIndexBuffer->m_LockedOffsetInBytes>>1],
						data.PeekGet(), size );
#endif
					memcpy( g_IndexBuffer[i].m_pLockedMemory, data.PeekGet(), size );
				}
			}
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8UnlockIndexBuffer( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		IndexBufferLookup_t indexBuffer;

	    data.GetInt( indexBuffer.m_Id );

		int i = g_IndexBuffer.Find( indexBuffer );
		if (i >= 0)
		{
#ifdef CHECK_INDICES
			g_IndexBuffer[i].m_LockedOffsetInBytes = 0;
			g_IndexBuffer[i].m_LockedSizeInBytes = 0;
#endif
			if (IsSuppressingCommands() && g_IndexBuffer[i].m_Dynamic)
				return;

			if (g_IndexBuffer[i].m_pIB && g_IndexBuffer[i].m_pLockedMemory)
			{
				HRESULT hr = g_IndexBuffer[i].m_pIB->Unlock( );
				Assert( !FAILED(hr) );
				g_IndexBuffer[i].m_pLockedMemory = 0;
			}
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetStreamSource( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 3 ))
	{
		int stream;
		VertexBufferLookup_t vertexBuffer;
		int vertexSize;

	    data.GetInt( vertexBuffer.m_Id );
		data.GetInt( stream );
		data.GetInt( vertexSize );

#ifdef CHECK_INDICES
		Assert( stream >= 0 && stream < MAX_NUM_STREAMS );
		g_StreamSourceID[stream] = vertexBuffer.m_Id;
#endif
		
		g_ValidStreamSource[stream] = false;
		if( vertexBuffer.m_Id != -1 )
		{
			int i = g_VertexBuffer.Find( vertexBuffer );
			if (i >= 0)
			{
#ifdef CHECK_INDICES
				g_VertexBuffer[i].m_VertexSize = vertexSize;
#endif
				if ( g_VertexBuffer[i].m_pVB && (!g_VertexBuffer[i].m_pLockedMemory) )
				{
					HRESULT hr = g_pD3DDevice->SetStreamSource( stream, 
						g_VertexBuffer[i].m_pVB, 
						0, 
						vertexSize );
					Assert( !FAILED(hr) );
					g_ValidStreamSource[stream] = !FAILED(hr);
				}
			}
		}
		else
		{
			HRESULT hr = g_pD3DDevice->SetStreamSource( stream, NULL, 0, 0 );
			Assert( !FAILED(hr) );
			g_ValidStreamSource[stream] = !FAILED(hr);
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetIndices( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 2 ))
	{
		IndexBufferLookup_t indexBuffer;
		int baseVertexIndex;

	    data.GetInt( indexBuffer.m_Id );
		data.GetInt( baseVertexIndex );

		g_ValidIndices = false;
		int i = g_IndexBuffer.Find( indexBuffer );
		if (i >= 0)
		{
			if ( g_IndexBuffer[i].m_pIB && (!g_IndexBuffer[i].m_pLockedMemory) )
			{
				HRESULT hr = g_pD3DDevice->SetIndices( g_IndexBuffer[i].m_pIB ); 
				Assert( !FAILED(hr) );
				g_ValidIndices = !FAILED(hr);
				g_CurrentIndexSource = i;
			}
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8DrawPrimitive( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && g_ValidStreamSource[0] && g_ValidIndices && ( numargs == 3 ))
	{
		D3DPRIMITIVETYPE mode;
		int firstIndex, numPrimitives;

	    data.GetInt( *(int*)&mode );
	    data.GetInt( firstIndex );
		data.GetInt( numPrimitives );

		HRESULT hr = g_pD3DDevice->DrawPrimitive( mode, firstIndex, numPrimitives ); 
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8DrawIndexedPrimitive( int numargs, CUtlBuffer& data )
{
//	double t = GetTime();
	
	if (g_pD3DDevice && g_ValidStreamSource[0] && g_ValidIndices && 
		( numargs == 6 )
		)
	{
		D3DPRIMITIVETYPE mode;
		int firstVertex, numVertices;
		int firstIndex, numPrimitives;

	    data.GetInt( *(int*)&mode );
	    data.GetInt( firstVertex );
		int dx9FirstIndex;
		data.GetInt( dx9FirstIndex );
		data.GetInt( numVertices );
	    data.GetInt( firstIndex );
		data.GetInt( numPrimitives );

#ifdef CHECK_INDICES
		int vertexOffset;
		vertexOffset = dx9FirstIndex;
		Assert( mode == D3DPT_TRIANGLELIST || mode == D3DPT_TRIANGLESTRIP );

		IndexBufferLookup_t *pIndexBuffer = &g_IndexBuffer[g_CurrentIndexSource];
		Assert( (int)firstIndex >= 0 && (int)firstIndex < (int)( pIndexBuffer->m_LengthInBytes >> 1 ) );
		Assert( g_StreamSourceID[0] != -1 );
		int i;
		for( i = 0; i < MAX_NUM_STREAMS; i++ )
		{
			if( g_StreamSourceID[i] == -1 )
			{
				continue;
			}
			VertexBufferLookup_t vertexBufferLookup;
			vertexBufferLookup.m_Id = g_StreamSourceID[i];
			int vertexBufferID = g_VertexBuffer.Find( vertexBufferLookup );
			Assert( vertexBufferID >= 0 );
			
			VertexBufferLookup_t *pVertexBuffer = &g_VertexBuffer[vertexBufferID];
			Assert( firstVertex >= 0 && 
				(int)(firstVertex + vertexOffset) < (int)(pVertexBuffer->m_LengthInBytes / pVertexBuffer->m_VertexSize) );
			int numIndices;
			if( mode == D3DPT_TRIANGLELIST )
			{
				numIndices = numPrimitives * 3;
			}
			else if( mode == D3DPT_TRIANGLESTRIP )
			{
				numIndices = numPrimitives + 2;
			}
			else
			{
				Assert( 0 );
			}
			int j;
			for( j = 0; j < numIndices; j++ )
			{
				int index = pIndexBuffer->m_pIndices[j+firstIndex];
				Assert( index >= firstVertex );
				Assert( index < firstVertex + numVertices );
			}
		}

#endif // CHECK_INDICES
		
		if( g_ForcedStaticIndexBufferPrims != -1 )
		{
			if( !g_IndexBuffer[g_CurrentIndexSource].m_Dynamic )
			{
 				if( numPrimitives < g_ForcedStaticIndexBufferPrims )
				{
					numPrimitives = g_ForcedStaticIndexBufferPrims;
				}
				else if( numPrimitives > g_ForcedStaticIndexBufferPrims )
				{
					numPrimitives = g_ForcedStaticIndexBufferPrims;
				}
			}
		}
		HRESULT hr = g_pD3DDevice->DrawIndexedPrimitive( mode, 
			dx9FirstIndex,
			firstVertex, 
			numVertices, 
			firstIndex, numPrimitives ); 
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}

/*
	double dt = GetTime() - t;
	if (dt > s_MaxStall)
		s_MaxStall = dt;
	s_TotalStall += dt;
	*/
}

void Dx8SetTextureData( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 3 ) )
	{
		int srcWidthInBytes, srcHeight;
		data.GetInt( srcWidthInBytes );
		data.GetInt( srcHeight );
		unsigned char *pSrc = ( unsigned char * )data.PeekGet();
		unsigned char *pDst = ( unsigned char * )s_LockedRect.pBits;
		int i;
/*
		if( g_LockedTextureID >= 20000 && g_LockedTextureID < 20020 )
		{
			for( i = 0; i < srcHeight; i++ )
			{
				memset( pDst, 255, srcWidthInBytes );
				pDst += s_LockedRect.Pitch;
			}
		}
		else
*/
		{
			for( i = 0; i < srcHeight; i++ )
			{
				memcpy( pDst, pSrc, srcWidthInBytes );
				pSrc += srcWidthInBytes;
				pDst += s_LockedRect.Pitch;
			}
		}
	}
	else
	{
		Assert( 0 );
	}
}

//-----------------------------------------------------------------------------
// Compute texture size based on compression
//-----------------------------------------------------------------------------

static inline int DetermineGreaterPowerOfTwo( int val )
{
	int num = 1;
	while (val > num)
	{
		num <<= 1;
	}

	return num;
}

inline int DeterminePowerOfTwo( int val )
{
	int pow = 0;
	while ((val & 0x1) == 0x0)
	{
		val >>= 1;
		++pow;
	}

	return pow;
}

bool ComputeTempTextureSize( int dstFormat, int& texWidth, 
							 int& texHeight, int& lockLevel )
{
	texWidth = DetermineGreaterPowerOfTwo( texWidth );
	texHeight = DetermineGreaterPowerOfTwo( texHeight );

	// Clamp the size of the temp surface to deal with compressed textures
	if ((dstFormat == IMAGE_FORMAT_DXT1) || (dstFormat == IMAGE_FORMAT_DXT3) ||
		(dstFormat == IMAGE_FORMAT_DXT5) || (dstFormat == IMAGE_FORMAT_DXT1_ONEBITALPHA))
	{
		if (texWidth >= texHeight)
		{
			if (texHeight < 4)
			{
				int factor = 4 / texHeight;
				texWidth *= factor;
				texHeight = 4;
				lockLevel = DeterminePowerOfTwo(factor);
				return true;
			}
		}
		else
		{
			if (texWidth < 4)
			{
				int factor = 4 / texWidth;
				texHeight *= factor;
				texWidth = 4;
				lockLevel = DeterminePowerOfTwo(factor);
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// A faster blitter which works in many cases
//-----------------------------------------------------------------------------

// NOTE: IF YOU CHANGE THIS, CHANGE THE VERSION IN TEXTUREDX8.CPP!!!!
static void FastBlitTextureBits( int bindId, int copy, IDirect3DBaseTexture* pDstTexture, int level, 
							 D3DCUBEMAP_FACES cubeFaceID,
	                         int xOffset, int yOffset, 
	                         int width, int height, ImageFormat srcFormat, int srcStride,
	                         unsigned char *pSrcData, ImageFormat dstFormat )
{
	// Get the level of the texture we want to write into
	IDirect3DSurface* pTextureLevel;
	HRESULT hr = GetSurfaceFromTexture( pDstTexture, level, cubeFaceID, &pTextureLevel );
	if (!FAILED(hr))
	{
		RECT srcRect;
		D3DLOCKED_RECT lockedRect;
		srcRect.left = xOffset;
		srcRect.right = xOffset + width;
		srcRect.top = yOffset;
		srcRect.bottom = yOffset + height;

		if( FAILED( pTextureLevel->LockRect( &lockedRect, &srcRect, D3DLOCK_NOSYSLOCK ) ) )
		{
//			Warning( "CShaderAPIDX8::BlitTextureBits: couldn't lock texture rect\n" );
			Assert( 0 );
			pTextureLevel->Release();
			return;
		}

		// garymcthack : need to make a recording command for this.
		unsigned char *pImage = (unsigned char *)lockedRect.pBits;
		ImageLoader::ConvertImageFormat( (unsigned char *)pSrcData, srcFormat,
							pImage, dstFormat, width, height, srcStride, lockedRect.Pitch );

		if( FAILED( pTextureLevel->UnlockRect( ) ) ) 
		{
			Assert( 0 );
//			Warning( "CShaderAPIDX8::BlitTextureBits: couldn't unlock texture rect\n" );
			pTextureLevel->Release();
			return;
		}
		
		pTextureLevel->Release();
	}
}

//-----------------------------------------------------------------------------
// Blit in bits
//-----------------------------------------------------------------------------

// NOTE: IF YOU CHANGE THIS, CHANGE THE VERSION IN TEXTUREDX8.CPP!!!!
static void BlitTextureBits( int bindId, int copy, IDirect3DBaseTexture* pDstTexture, int level, 
							 D3DCUBEMAP_FACES cubeFaceID,
	                         int xOffset, int yOffset, 
	                         int width, int height, ImageFormat srcFormat, int srcStride,
	                         unsigned char *pSrcData, ImageFormat dstFormat )
{
	FastBlitTextureBits( bindId, copy, pDstTexture, level, cubeFaceID, xOffset, yOffset,
		width, height, srcFormat, srcStride, pSrcData, dstFormat );
}

void Dx8BlitTextureBits( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 13 ) )
	{
		int bindId, copy, level, xOffset, yOffset, width, height, srcStride, srcDataSize;
		ImageFormat srcFormat, dstFormat;
		D3DCUBEMAP_FACES cubeFaceID;

		data.GetInt( bindId );
		data.GetInt( copy );
		data.GetInt( level );
		data.GetInt( *( int * )&cubeFaceID );
		data.GetInt( xOffset );
		data.GetInt( yOffset );
		data.GetInt( width );
		data.GetInt( height );
		data.GetInt( *( int * )&srcFormat );
		data.GetInt( srcStride );
		data.GetInt( *( int * )&dstFormat );
		data.GetInt( srcDataSize );

		static CUtlVector< unsigned char > textureMemoryVec;
		if( textureMemoryVec.Size() < srcDataSize )
		{
			textureMemoryVec.AddMultipleToTail( srcDataSize - textureMemoryVec.Size() );
		}
		data.Get( textureMemoryVec.Base(), srcDataSize );

		TextureLookup_t texture;
		texture.m_BindId = bindId;
		int i = g_Texture.Find( texture );
		if( i < 0 )
		{
			Assert( 0 );
			return;
		}
		IDirect3DBaseTexture *pTexture;
		Assert( !( g_Texture[i].flags & TEXTURE_LOOKUP_FLAGS_IS_DEPTH_STENCIL ) );
		if( g_Texture[i].m_NumCopies == 1 )
		{
			pTexture = g_Texture[i].m_pTexture;
		}
		else
		{
			pTexture = g_Texture[i].m_ppTexture[copy];
		}

		BlitTextureBits( bindId, copy, pTexture, level, 
							 cubeFaceID,
	                         xOffset, yOffset, 
	                         width, height, srcFormat, srcStride,
	                         textureMemoryVec.Base(), dstFormat );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8GetDeviceCaps( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 0 ))
	{
		D3DCAPS caps;
		HRESULT hr = g_pD3DDevice->GetDeviceCaps( &caps );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8GetAdapterIdentifier( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 2 ))
	{
		D3DADAPTER_IDENTIFIER ident;
		
		UINT adapter;
		DWORD flags;

	    data.GetInt( *(int*)&adapter );
	    data.GetInt( *(int*)&flags );

		HRESULT hr = g_pD3D->GetAdapterIdentifier( adapter, flags, &ident );
		Assert( !FAILED(hr) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8HardwareSync( int numargs, CUtlBuffer& data )
{
	if( !g_bAllowHWSync )
	{
		return;
	}

	if( g_pFrameSyncQueryObject )
	{
		BOOL bDummy;
		HRESULT hr;
		
		// FIXME: Could install a callback into the materialsystem to do something while waiting for
		// the frame to finish (update sound, etc.)
		do
		{
			hr = g_pFrameSyncQueryObject->GetData( &bDummy, sizeof( bDummy ), D3DGETDATA_FLUSH );
		} while( hr == S_FALSE );

		// FIXME: Need to check for hr==D3DERR_DEVICELOST here.
		Assert( hr != D3DERR_DEVICELOST );
		Assert( hr == S_OK );
		g_pFrameSyncQueryObject->Issue( D3DISSUE_END );
	}
	else
	{
		HRESULT hr;
		IDirect3DSurface* pSurfaceBits = 0;
		
		// Get the back buffer
		IDirect3DSurface* pBackBuffer;
		hr = g_pD3DDevice->GetRenderTarget( 0, &pBackBuffer );
		if (FAILED(hr))
		{
			Warning("GetRenderTarget() failed");
			return;
		}

		// Find about its size and format
		D3DSURFACE_DESC desc;
		hr = pBackBuffer->GetDesc( &desc );
		if (FAILED(hr))
		{
			Warning("GetDesc() failed");
			goto CleanUp;
		}
		D3DLOCKED_RECT lockedRect;
		RECT sourceRect;
		sourceRect.bottom = 4;
		sourceRect.top = 0;
		sourceRect.left = 0;
		sourceRect.right = 4;
		hr = pBackBuffer->LockRect(&lockedRect,&sourceRect,D3DLOCK_READONLY);
		if (FAILED(hr))
		{
			Warning("LockRect() failed");
			goto CleanUp;
		}
		// Utterly lame, but prevents the optimizer from removing the read:
		volatile unsigned long a;
		memcpy((void *)&a,(unsigned char*)lockedRect.pBits,sizeof(a));

		hr = pBackBuffer->UnlockRect();
		if (FAILED(hr))
			Warning("UnlockRect() failed");

CleanUp:
		pBackBuffer->Release();
	}
}

void Dx8CopyRenderTargetToTexture( int numargs, CUtlBuffer& data )
{
	if (g_pD3DDevice && ( numargs == 1 ))
	{
		int bindID;
		data.GetInt( bindID );
		
		TextureLookup_t tmpTextureLookup;
		tmpTextureLookup.m_BindId = bindID;
		int texID = g_Texture.Find( tmpTextureLookup );
		if( texID < 0 )
		{
			Assert( 0 );
			return;
		}
		
		IDirect3DSurface* pSurfaceBits = 0;
		
		IDirect3DTexture *pD3DTexture = ( IDirect3DTexture * )g_Texture[texID].m_pTexture;

		// Get the back buffer
		IDirect3DSurface* pBackBuffer;
		HRESULT hr = g_pD3DDevice->GetRenderTarget( 0, &pBackBuffer );
		if (FAILED(hr))
		{
			Assert( 0 );
			return;
		}
		
		IDirect3DSurface *pDstSurf;
		hr = pD3DTexture->GetSurfaceLevel( 0, &pDstSurf );
		Assert( !FAILED( hr ) );
		if( FAILED( hr ) )
		{
			Assert( 0 );
			return;
		}

		hr = g_pD3DDevice->StretchRect( pBackBuffer, NULL, pDstSurf, NULL, D3DTEXF_NONE );
		Assert( !FAILED( hr ) );
		pDstSurf->Release();
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8DebugString( int numargs, CUtlBuffer& data )
{
	// do nothing . . this only used for listing purposes.
}

void Dx8CreateDepthTexture( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 5 ) )
	{
		int bindID, width, height;
		D3DFORMAT format;
		D3DMULTISAMPLE_TYPE multisampleType;
		data.GetInt( bindID );
		data.GetInt( width );
		data.GetInt( height );
		data.GetInt( *( int * )&format );
		data.GetInt( *( int * )&multisampleType );

		TextureLookup_t tmpTextureLookup;
		tmpTextureLookup.m_BindId = bindID;
		int i = g_Texture.Find( tmpTextureLookup );
		if (i < 0)
			i = g_Texture.Insert( tmpTextureLookup );
		
		g_Texture[i].flags |= TEXTURE_LOOKUP_FLAGS_IS_DEPTH_STENCIL;
		HRESULT hr = g_pD3DDevice->CreateDepthStencilSurface(
			width, height, format, multisampleType, 0, TRUE, &g_Texture[i].m_pDepthStencilSurface, NULL );
		Assert( !FAILED( hr ) );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8DestroyDepthTexture( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 1 ) )
	{
		TextureLookup_t texture;
	    data.GetInt( texture.m_BindId );

		int i = g_Texture.Find( texture );
		if (i >= 0)
		{
			g_Texture.RemoveAt( i );
		}
		else
		{
			Assert( 0 );
		}
	}
	else
	{
		Assert( 0 );
	}
}

//-----------------------------------------------------------------------------
// Gets the surface associated with a texture (refcount of surface is increased)
//-----------------------------------------------------------------------------
static IDirect3DSurface* GetTextureSurface( unsigned int tex )
{
	IDirect3DSurface* pSurface;

	TextureLookup_t texture;
	texture.m_BindId = tex;
	int i = g_Texture.Find( texture );
	if( i < 0 )
	{
		Assert( 0 );
		return NULL;
	}
	IDirect3DBaseTexture* pTexture;
	if( g_Texture[i].m_NumCopies == 1 )
	{
		pTexture = g_Texture[i].m_pTexture;
	}
	else
	{
		pTexture = g_Texture[i].m_ppTexture[0];
	}

	IDirect3DTexture* pTex = static_cast<IDirect3DTexture*>( pTexture );
	if( !pTex )
	{
		Assert( 0 );
		return NULL;
	}
	
	HRESULT hr = pTex->GetSurfaceLevel( 0, &pSurface );
	Assert( hr == D3D_OK );

	return pSurface;
}

//-----------------------------------------------------------------------------
// Gets the surface associated with a texture (refcount of surface is increased)
//-----------------------------------------------------------------------------
static IDirect3DSurface* GetDepthTextureSurface( unsigned int tex )
{
	TextureLookup_t texture;
	texture.m_BindId = tex;
	int i = g_Texture.Find( texture );
	if( i < 0 )
	{
		Assert( 0 );
		return NULL;
	}
	Assert( g_Texture[i].m_pDepthStencilSurface );
	return g_Texture[i].m_pDepthStencilSurface;
}

void Dx8SetRenderTarget( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 2 ) )
	{
		int colorTexture, depthTexture;
		data.GetInt( colorTexture );
		data.GetInt( depthTexture );
		
		IDirect3DSurface* pColorSurface;
		IDirect3DSurface* pZSurface;
		
		// NOTE!!!!  If this code changes, also change Dx8SetRenderTarget in playback.cpp
		bool usingTextureTarget = false;
		if( colorTexture == SHADER_RENDERTARGET_BACKBUFFER )
		{
			pColorSurface = g_pBackBufferSurface;
			
			// This is just to make the code a little simpler...
			// (simplifies the release logic)
			pColorSurface->AddRef();
		}
		else
		{
			pColorSurface = GetTextureSurface( colorTexture );
			if (!pColorSurface)
				return;
			
			usingTextureTarget = true;
		}
		
		if ( depthTexture == SHADER_RENDERTARGET_DEPTHBUFFER)
		{
			pZSurface = g_pZBufferSurface;
			
			// This is just to make the code a little simpler...
			// (simplifies the release logic)
			pZSurface->AddRef();
		}
		else
		{
			pZSurface = GetDepthTextureSurface( depthTexture );
			pZSurface->AddRef();
			if (!pZSurface)
			{
				// Refcount of color surface was increased above
				pColorSurface->Release();
				return;
			}
			usingTextureTarget = true;
		}
		
		g_UsingTextureRenderTarget = usingTextureTarget;
		
		// NOTE: The documentation says that SetRenderTarget increases the refcount
		// but it doesn't appear to in practice. If this somehow changes (perhaps
		// in a device-specific manner, we're in trouble).
		g_pD3DDevice->SetRenderTarget( 0, pColorSurface );
		g_pD3DDevice->SetDepthStencilSurface( pZSurface );
		int ref = pZSurface->Release();
		Assert( ref != 0 );
		ref = pColorSurface->Release();
		Assert( ref != 0 );
		
		if (g_UsingTextureRenderTarget)
		{
			D3DSURFACE_DESC  desc;
			HRESULT hr = pColorSurface->GetDesc( &desc );
			Assert( !FAILED(hr) );
		}
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8TestCooperativeLevel( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 0 ) )
	{
		g_pD3DDevice->TestCooperativeLevel();
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetVertexBufferFormat( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 2 ) )
	{
		// Don't do anything. . this is only for listing.
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetVertexDeclaration( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 1 ) )
	{
		VertexDeclLookup_t lookup;
		data.GetInt( lookup.m_Id );
		int id = g_VertexDeclBuffer.Find( lookup );
		Assert( id >= 0 );
		HRESULT hr = g_pD3DDevice->SetVertexDeclaration( g_VertexDeclBuffer[id].m_pDecl );
		Assert( hr == D3D_OK );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8CreateVertexDeclaration( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 2 ) )
	{
		VertexDeclLookup_t lookup;
		D3DVERTEXELEMENT9 decl[32];

		data.GetInt( lookup.m_Id );
		memcpy( decl, data.PeekGet(), sizeof( decl ) );
		Assert( g_VertexDeclBuffer.Find( lookup ) == -1 );
		HRESULT hr = g_pD3DDevice->CreateVertexDeclaration( decl, &lookup.m_pDecl );
		Assert( hr == D3D_OK );
		g_VertexDeclBuffer.Insert( lookup );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetFVF( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 1 ) )
	{
		DWORD fvf;
		data.GetInt( *( int * )&fvf );
		g_pD3DDevice->SetVertexShader( NULL );
		g_pD3DDevice->SetFVF( fvf );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SetClipPlane( int numargs, CUtlBuffer& data )
{
	if( g_pD3DDevice && ( numargs == 5 ) )
	{
		int index;
		float plane[4];
		data.GetInt( index );
		data.GetFloat( plane[0] );
		data.GetFloat( plane[1] );
		data.GetFloat( plane[2] );
		data.GetFloat( plane[3] );
		g_pD3DDevice->SetClipPlane( index, plane );
	}
	else
	{
		Assert( 0 );
	}
}

void Dx8SyncToken( int numargs, CUtlBuffer& data )
{
	int strMemSize = strlen( ( const char * )data.PeekGet() ) + 1;
	char *str = ( char * )_alloca( strMemSize );
	data.GetString( str );

	// Watch here for debug commands.
}

//-----------------------------------------------------------------------------
// Change this table when you change the RecordingCommands_t list...
//-----------------------------------------------------------------------------

static Dx8Func_t g_RecordingCommand[] = 
{
	Dx8CreateDevice,
	Dx8DestroyDevice,
	Dx8Reset,
	Dx8ShowCursor,
	Dx8BeginScene,
	Dx8EndScene,
	Dx8Present,
	Dx8CreateTexture,
	Dx8DestroyTexture,
	Dx8SetTexture,
	Dx8SetTransform,
	Dx8CreateVertexShader,
	Dx8CreatePixelShader,
	Dx8DestroyVertexShader,
	Dx8DestroyPixelShader,
	Dx8SetVertexShader,
	Dx8SetPixelShader,
	Dx8SetVertexShaderConstant,
	Dx8SetPixelShaderConstant,
	Dx8SetMaterial,
	Dx8LightEnable,
	Dx8SetLight,
	Dx8SetViewport,
	Dx8Clear,
	Dx8ValidateDevice,
	Dx8SetRenderState,
	Dx8SetTextureStageState,

	Dx8CreateVertexBuffer,
	Dx8DestroyVertexBuffer,
	Dx8LockVertexBuffer,
	Dx8VertexData,
	Dx8UnlockVertexBuffer,

	Dx8CreateIndexBuffer,
	Dx8DestroyIndexBuffer,
	Dx8LockIndexBuffer,
	Dx8IndexData,
	Dx8UnlockIndexBuffer,

	Dx8SetStreamSource,
	Dx8SetIndices,
	Dx8DrawPrimitive,
	Dx8DrawIndexedPrimitive,

	Dx8LockTexture,
	Dx8UnlockTexture,

	Dx8Keyframe,
	Dx8SetTextureData,
	Dx8BlitTextureBits,

	Dx8GetDeviceCaps,
	Dx8GetAdapterIdentifier,

	Dx8HardwareSync,
	
	Dx8CopyRenderTargetToTexture,
	Dx8DebugString,
	Dx8CreateDepthTexture,
	Dx8DestroyDepthTexture,
	Dx8SetRenderTarget,
	Dx8TestCooperativeLevel,
	Dx8SetVertexBufferFormat,
	Dx8SetSamplerState,
	Dx8SetVertexDeclaration,
	Dx8CreateVertexDeclaration,
	Dx8SetFVF,
	Dx8SetClipPlane,
	Dx8SyncToken
};

static const char *g_RecordingCommandNames[] = 
{
	"Dx8CreateDevice",
	"Dx8DestroyDevice",
	"Dx8Reset",
	"Dx8ShowCursor",
	"Dx8BeginScene",
	"Dx8EndScene",
	"Dx8Present",
	"Dx8CreateTexture",
	"Dx8DestroyTexture",
	"Dx8SetTexture",
	"Dx8SetTransform",
	"Dx8CreateVertexShader",
	"Dx8CreatePixelShader",
	"Dx8DestroyVertexShader",
	"Dx8DestroyPixelShader",
	"Dx8SetVertexShader",
	"Dx8SetPixelShader",
	"Dx8SetVertexShaderConstant",
	"Dx8SetPixelShaderConstant",
	"Dx8SetMaterial",
	"Dx8LightEnable",
	"Dx8SetLight",
	"Dx8SetViewport",
	"Dx8Clear",
	"Dx8ValidateDevice",
	"Dx8SetRenderState",
	"Dx8SetTextureStageState",

	"Dx8CreateVertexBuffer",
	"Dx8DestroyVertexBuffer",
	"Dx8LockVertexBuffer",
	"Dx8VertexData",
	"Dx8UnlockVertexBuffer",

	"Dx8CreateIndexBuffer",
	"Dx8DestroyIndexBuffer",
	"Dx8LockIndexBuffer",
	"Dx8IndexData",
	"Dx8UnlockIndexBuffer",

	"Dx8SetStreamSource",
	"Dx8SetIndices",
	"Dx8DrawPrimitive",
	"Dx8DrawIndexedPrimitive",

	"Dx8LockTexture",
	"Dx8UnlockTexture",

	"Dx8Keyframe",
	"Dx8SetTextureData",
	"Dx8BlitTextureBits",

	"Dx8GetDeviceCaps",
	"Dx8GetAdapterIdentifier",

	"Dx8HardwareSync",
	
	"Dx8CopyRenderTargetToTexture",
	"Dx8DebugString",
	"Dx8CreateDepthTexture",
	"Dx8DestroyDepthTexture",
	"Dx8SetRenderTarget",
	"Dx8TestCooperativeLevel",
	"Dx8SetVertexBufferFormat",

	"Dx8SetSamplerState",
	"Dx8SetVertexDeclaration",
	"Dx8CreateVertexDeclaration",
	"Dx8SetFVF",
	"Dx8SetClipPlane",
	"Dx8SyncToken"
};

typedef void ListFunc_t( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs );

void ListFunc_Default( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	fprintf( fp, "%s: %d args\n", g_RecordingCommandNames[cmd], ( int )numargs );
}

static const char *RenderStateToString( D3DRENDERSTATETYPE renderState )
{
	switch( renderState )
	{
    case D3DRS_ZENABLE:
		return "D3DRS_ZENABLE";
    case D3DRS_FILLMODE:
		return "D3DRS_FILLMODE";
    case D3DRS_SHADEMODE:
		return "D3DRS_SHADEMODE";
    case D3DRS_ZWRITEENABLE:
		return "D3DRS_ZWRITEENABLE";
    case D3DRS_ALPHATESTENABLE:
		return "D3DRS_ALPHATESTENABLE";
    case D3DRS_LASTPIXEL:
		return "D3DRS_LASTPIXEL";
    case D3DRS_SRCBLEND:
		return "D3DRS_SRCBLEND";
    case D3DRS_DESTBLEND:
		return "D3DRS_DESTBLEND";
    case D3DRS_CULLMODE:
		return "D3DRS_CULLMODE";
    case D3DRS_ZFUNC:
		return "D3DRS_ZFUNC";
    case D3DRS_ALPHAREF:
		return "D3DRS_ALPHAREF";
    case D3DRS_ALPHAFUNC:
		return "D3DRS_ALPHAFUNC";
    case D3DRS_DITHERENABLE:
		return "D3DRS_DITHERENABLE";
    case D3DRS_ALPHABLENDENABLE:
		return "D3DRS_ALPHABLENDENABLE";
    case D3DRS_FOGENABLE:
		return "D3DRS_FOGENABLE";
    case D3DRS_SPECULARENABLE:
		return "D3DRS_SPECULARENABLE";
    case D3DRS_FOGCOLOR:
		return "D3DRS_FOGCOLOR";
    case D3DRS_FOGTABLEMODE:
		return "D3DRS_FOGTABLEMODE";
    case D3DRS_FOGSTART:
		return "D3DRS_FOGSTART";
    case D3DRS_FOGEND:
		return "D3DRS_FOGEND";
    case D3DRS_FOGDENSITY:
		return "D3DRS_FOGDENSITY";
    case D3DRS_RANGEFOGENABLE:
		return "D3DRS_RANGEFOGENABLE";
    case D3DRS_STENCILENABLE:
		return "D3DRS_STENCILENABLE";
    case D3DRS_STENCILFAIL:
		return "D3DRS_STENCILFAIL";
    case D3DRS_STENCILZFAIL:
		return "D3DRS_STENCILZFAIL";
    case D3DRS_STENCILPASS:
		return "D3DRS_STENCILPASS";
    case D3DRS_STENCILFUNC:
		return "D3DRS_STENCILFUNC";
    case D3DRS_STENCILREF:
		return "D3DRS_STENCILREF";
    case D3DRS_STENCILMASK:
		return "D3DRS_STENCILMASK";
    case D3DRS_STENCILWRITEMASK:
		return "D3DRS_STENCILWRITEMASK";
    case D3DRS_TEXTUREFACTOR:
		return "D3DRS_TEXTUREFACTOR";
    case D3DRS_WRAP0:
		return "D3DRS_WRAP0";
    case D3DRS_WRAP1:
		return "D3DRS_WRAP1";
    case D3DRS_WRAP2:
		return "D3DRS_WRAP2";
    case D3DRS_WRAP3:
		return "D3DRS_WRAP3";
    case D3DRS_WRAP4:
		return "D3DRS_WRAP4";
    case D3DRS_WRAP5:
		return "D3DRS_WRAP5";
    case D3DRS_WRAP6:
		return "D3DRS_WRAP6";
    case D3DRS_WRAP7:
		return "D3DRS_WRAP7";
    case D3DRS_CLIPPING:
		return "D3DRS_CLIPPING";
    case D3DRS_LIGHTING:
		return "D3DRS_LIGHTING";
    case D3DRS_AMBIENT:
		return "D3DRS_AMBIENT";
    case D3DRS_FOGVERTEXMODE:
		return "D3DRS_FOGVERTEXMODE";
    case D3DRS_COLORVERTEX:
		return "D3DRS_COLORVERTEX";
    case D3DRS_LOCALVIEWER:
		return "D3DRS_LOCALVIEWER";
    case D3DRS_NORMALIZENORMALS:
		return "D3DRS_NORMALIZENORMALS";
    case D3DRS_DIFFUSEMATERIALSOURCE:
		return "D3DRS_DIFFUSEMATERIALSOURCE";
    case D3DRS_SPECULARMATERIALSOURCE:
		return "D3DRS_SPECULARMATERIALSOURCE";
    case D3DRS_AMBIENTMATERIALSOURCE:
		return "D3DRS_AMBIENTMATERIALSOURCE";
    case D3DRS_EMISSIVEMATERIALSOURCE:
		return "D3DRS_EMISSIVEMATERIALSOURCE";
    case D3DRS_VERTEXBLEND:
		return "D3DRS_VERTEXBLEND";
    case D3DRS_CLIPPLANEENABLE:
		return "D3DRS_CLIPPLANEENABLE";
    case D3DRS_POINTSIZE:
		return "D3DRS_POINTSIZE";
    case D3DRS_POINTSIZE_MIN:
		return "D3DRS_POINTSIZE_MIN";
    case D3DRS_POINTSPRITEENABLE:
		return "D3DRS_POINTSPRITEENABLE";
    case D3DRS_POINTSCALEENABLE:
		return "D3DRS_POINTSCALEENABLE";
    case D3DRS_POINTSCALE_A:
		return "D3DRS_POINTSCALE_A";
    case D3DRS_POINTSCALE_B:
		return "D3DRS_POINTSCALE_B";
    case D3DRS_POINTSCALE_C:
		return "D3DRS_POINTSCALE_C";
    case D3DRS_MULTISAMPLEANTIALIAS:
		return "D3DRS_MULTISAMPLEANTIALIAS";
    case D3DRS_MULTISAMPLEMASK:
		return "D3DRS_MULTISAMPLEMASK";
    case D3DRS_PATCHEDGESTYLE:
		return "D3DRS_PATCHEDGESTYLE";
    case D3DRS_DEBUGMONITORTOKEN:
		return "D3DRS_DEBUGMONITORTOKEN";
    case D3DRS_POINTSIZE_MAX:
		return "D3DRS_POINTSIZE_MAX";
    case D3DRS_INDEXEDVERTEXBLENDENABLE:
		return "D3DRS_INDEXEDVERTEXBLENDENABLE";
    case D3DRS_COLORWRITEENABLE:
		return "D3DRS_COLORWRITEENABLE";
    case D3DRS_TWEENFACTOR:
		return "D3DRS_TWEENFACTOR";
    case D3DRS_BLENDOP:
		return "D3DRS_BLENDOP";
	case D3DRS_SLOPESCALEDEPTHBIAS:
		return "D3DRS_SLOPESCALEDEPTHBIAS";
	case D3DRS_DEPTHBIAS:
		return "D3DRS_DEPTHBIAS";
	default:
		return "UNKNOWN!";
	}
}

static const char *BlendModeToString( int blendMode )
{
	switch( blendMode )
	{
	case D3DBLEND_ZERO:
		return "D3DBLEND_ZERO";
    case D3DBLEND_ONE:
		return "D3DBLEND_ONE";
    case D3DBLEND_SRCCOLOR:
		return "D3DBLEND_SRCCOLOR";
    case D3DBLEND_INVSRCCOLOR:
		return "D3DBLEND_INVSRCCOLOR";
    case D3DBLEND_SRCALPHA:
		return "D3DBLEND_SRCALPHA";
    case D3DBLEND_INVSRCALPHA:
		return "D3DBLEND_INVSRCALPHA";
    case D3DBLEND_DESTALPHA:
		return "D3DBLEND_DESTALPHA";
    case D3DBLEND_INVDESTALPHA:
		return "D3DBLEND_INVDESTALPHA";
    case D3DBLEND_DESTCOLOR:
		return "D3DBLEND_DESTCOLOR";
    case D3DBLEND_INVDESTCOLOR:
		return "D3DBLEND_INVDESTCOLOR";
    case D3DBLEND_SRCALPHASAT:
		return "D3DBLEND_SRCALPHASAT";
    case D3DBLEND_BOTHSRCALPHA:
		return "D3DBLEND_BOTHSRCALPHA";
    case D3DBLEND_BOTHINVSRCALPHA:
		return "D3DBLEND_BOTHINVSRCALPHA";
	default:
		Assert( 0 );
		return "<ERROR>";
	}
}

void PrintRenderStateValue( FILE *fp, D3DRENDERSTATETYPE state, DWORD value )
{
	switch( state )
	{
	case D3DRS_ZENABLE:                       /* D3DZBUFFERTYPE (or TRUE/FALSE for legacy) */
		switch( value )
		{
		case D3DZB_FALSE:
			fprintf( fp, "D3DZB_FALSE" );
			break;
		case D3DZB_TRUE:
			fprintf( fp, "D3DZB_TRUE" );
			break;
		case D3DZB_USEW:
			fprintf( fp, "D3DZB_USEW" );
			break;
		default:
			Assert( 0 );
			break;
		}
		break;
	case D3DRS_FILLMODE:                      /* D3DFILLMODE */
		switch( value )
		{
		case D3DFILL_POINT:
			fprintf( fp, "D3DFILL_POINT" );
			break;
		case D3DFILL_WIREFRAME:
			fprintf( fp, "D3DFILL_WIREFRAME" );
			break;
		case D3DFILL_SOLID:
			fprintf( fp, "D3DFILL_SOLID" );
			break;
		default:
			Assert( 0 );
			break;
		}
		break;
	case D3DRS_SHADEMODE:                     /* D3DSHADEMODE */
		switch( value )
		{
		case D3DSHADE_FLAT:
			fprintf( fp, "D3DSHADE_FLAT" );
			break;
		case D3DSHADE_GOURAUD:
			fprintf( fp, "D3DSHADE_GOURAUD" );
			break;
		case D3DSHADE_PHONG:
			fprintf( fp, "D3DSHADE_PHONG" );
			break;
		default:
			Assert( 0 );
			break;
		}
		break;
	case D3DRS_ZWRITEENABLE:                 /* TRUE to enable z writes */
	case D3DRS_ALPHATESTENABLE:              /* TRUE to enable alpha tests */
	case D3DRS_LASTPIXEL:                    /* TRUE for last-pixel on lines */
	case D3DRS_DITHERENABLE:                 /* TRUE to enable dithering */
	case D3DRS_ALPHABLENDENABLE:             /* TRUE to enable alpha blending */
	case D3DRS_FOGENABLE:                    /* TRUE to enable fog blending */
	case D3DRS_SPECULARENABLE:               /* TRUE to enable specular */
	case D3DRS_RANGEFOGENABLE:               /* Enables range-based fog */
	case D3DRS_STENCILENABLE:                /* BOOL enable/disable stenciling */
	case D3DRS_CLIPPING:                  
	case D3DRS_LIGHTING:                  
	case D3DRS_COLORVERTEX:               
	case D3DRS_LOCALVIEWER:               
	case D3DRS_NORMALIZENORMALS:          
	case D3DRS_POINTSPRITEENABLE:            /* BOOL point texture coord control */
	case D3DRS_POINTSCALEENABLE:             /* BOOL point size scale enable */
	case D3DRS_MULTISAMPLEANTIALIAS:        // BOOL - set to do FSAA with multisample buffer
	case D3DRS_INDEXEDVERTEXBLENDENABLE:  
		fprintf( fp, "%s", value ? "TRUE" : "FALSE" );
		break;
	case D3DRS_SRCBLEND:                     /* D3DBLEND */
	case D3DRS_DESTBLEND:                    /* D3DBLEND */
		fprintf( fp, "%s", BlendModeToString( value ) );
		break;
	case D3DRS_CULLMODE:                     /* D3DCULL */
		switch( value )
		{
		case D3DCULL_NONE:
			fprintf( fp, "D3DCULL_NONE" );
			break;
		case D3DCULL_CW:
			fprintf( fp, "D3DCULL_CW" );
			break;
		case D3DCULL_CCW:
			fprintf( fp, "D3DCULL_CCW" );
			break;
		default:
			Assert( 0 );
			break;
		}
		break;
	case D3DRS_ZFUNC:                        /* D3DCMPFUNC */
	case D3DRS_ALPHAFUNC:                    /* D3DCMPFUNC */
	case D3DRS_STENCILFUNC:                  /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
		switch( value )
		{
		case D3DCMP_NEVER:
			fprintf( fp, "D3DCMP_NEVER" );
			break;
		case D3DCMP_LESS:
			fprintf( fp, "D3DCMP_LESS" );
			break;
		case D3DCMP_EQUAL:
			fprintf( fp, "D3DCMP_EQUAL" );
			break;
		case D3DCMP_LESSEQUAL:
			fprintf( fp, "D3DCMP_LESSEQUAL" );
			break;
		case D3DCMP_GREATER:
			fprintf( fp, "D3DCMP_GREATER" );
			break;
		case D3DCMP_NOTEQUAL:
			fprintf( fp, "D3DCMP_NOTEQUAL" );
			break;
		case D3DCMP_GREATEREQUAL:
			fprintf( fp, "D3DCMP_GREATEREQUAL" );
			break;
		case D3DCMP_ALWAYS:
			fprintf( fp, "D3DCMP_ALWAYS" );
			break;
		default:
			Assert( 0 );
			break;
		}
		break;
	case D3DRS_ALPHAREF:                     /* D3DFIXED */
		fprintf( fp, "%d", ( int )value );
		break;
	case D3DRS_FOGCOLOR:                     /* D3DCOLOR */
	case D3DRS_TEXTUREFACTOR:                /* D3DCOLOR used for multi-texture blend */
	case D3DRS_AMBIENT:                   
		{
			unsigned char *p = ( unsigned char * )&value;
			fprintf( fp, "%d %d %d %d", ( int )p[0], ( int )p[1], ( int )p[2], ( int )p[3] );
			break;
		}
	case D3DRS_FOGTABLEMODE:                 /* D3DFOGMODE */
		switch( value )
		{
		case D3DFOG_NONE:
			fprintf( fp, "D3DFOG_NONE" );
			break;
		case D3DFOG_EXP:    
			fprintf( fp, "D3DFOG_EXP" );
			break;
		case D3DFOG_EXP2:   
			fprintf( fp, "D3DFOG_EXP2" );
			break;
		case D3DFOG_LINEAR: 
			fprintf( fp, "D3DFOG_LINEAR" );
			break;
		default:
			Assert( 0 );
			break;
		}
		break;
	case D3DRS_FOGSTART:                     /* Fog start (for both vertex and pixel fog) */
	case D3DRS_FOGEND:                       /* Fog end      */
	case D3DRS_FOGDENSITY:                   /* Fog density  */
	case D3DRS_SLOPESCALEDEPTHBIAS:
	case D3DRS_DEPTHBIAS:
		fprintf( fp, "%f", *( float * )&value );
		break;
	case D3DRS_STENCILFAIL:                  /* D3DSTENCILOP to do if stencil test fails */
	case D3DRS_STENCILZFAIL:                 /* D3DSTENCILOP to do if stencil test passes and Z test fails */
	case D3DRS_STENCILPASS:                  /* D3DSTENCILOP to do if both stencil and Z tests pass */
		switch( value )
		{
		case D3DSTENCILOP_KEEP:
			fprintf( fp, "D3DSTENCILOP_KEEP" );
			break;
		case D3DSTENCILOP_ZERO:
			fprintf( fp, "D3DSTENCILOP_ZERO" );
			break;
		case D3DSTENCILOP_REPLACE:
			fprintf( fp, "D3DSTENCILOP_REPLACE" );
			break;
		case D3DSTENCILOP_INCRSAT:
			fprintf( fp, "D3DSTENCILOP_INCRSAT" );
			break;
		case D3DSTENCILOP_DECRSAT:
			fprintf( fp, "D3DSTENCILOP_DECRSAT" );
			break;
		case D3DSTENCILOP_INVERT:
			fprintf( fp, "D3DSTENCILOP_INVERT" );
			break;
		case D3DSTENCILOP_INCR:
			fprintf( fp, "D3DSTENCILOP_INCR" );
			break;
		case D3DSTENCILOP_DECR:
			fprintf( fp, "D3DSTENCILOP_DECR" );
			break;
		default:
			Assert( 0 );
			break;
		}
		break;
		
	case D3DRS_STENCILREF:                   /* Reference value used in stencil test */
		fprintf( fp, "D3DRS_STENCILREF" );
		break;
	case D3DRS_STENCILMASK:                  /* Mask value used in stencil test */
		fprintf( fp, "D3DRS_STENCILMASK" );
		break;
	case D3DRS_STENCILWRITEMASK:             /* Write mask applied to values written to stencil buffer */
		fprintf( fp, "D3DRS_STENCILWRITEMASK" );
		break;
	case D3DRS_WRAP0:                       /* wrap for 1st texture coord. set */
		fprintf( fp, "D3DRS_WRAP1" );
		break;
	case D3DRS_WRAP1:                       /* wrap for 2nd texture coord. set */
		fprintf( fp, "D3DRS_WRAP1" );
		break;
	case D3DRS_WRAP2:                       /* wrap for 3rd texture coord. set */
		fprintf( fp, "D3DRS_WRAP2" );
		break;
	case D3DRS_WRAP3:                       /* wrap for 4th texture coord. set */
		fprintf( fp, "D3DRS_WRAP3" );
		break;
	case D3DRS_WRAP4:                       /* wrap for 5th texture coord. set */
		fprintf( fp, "D3DRS_WRAP4" );
		break;
	case D3DRS_WRAP5:                       /* wrap for 6th texture coord. set */
		fprintf( fp, "D3DRS_WRAP5" );
		break;
	case D3DRS_WRAP6:                       /* wrap for 7th texture coord. set */
		fprintf( fp, "D3DRS_WRAP6" );
		break;
	case D3DRS_WRAP7:                       /* wrap for 8th texture coord. set */
		fprintf( fp, "D3DRS_WRAP7" );
		break;
	case D3DRS_FOGVERTEXMODE:             
		switch( value )
		{
		case D3DFOG_NONE:
			fprintf( fp, "D3DFOG_NONE" );
			break;
		case D3DFOG_EXP:
			fprintf( fp, "D3DFOG_EXP" );
			break;
		case D3DFOG_EXP2:
			fprintf( fp, "D3DFOG_EXP2" );
			break;
		case D3DFOG_LINEAR:
			fprintf( fp, "D3DFOG_LINEAR" );
			break;
		default:
			Assert( 0 );
			break;
		}
		break;

	case D3DRS_DIFFUSEMATERIALSOURCE:     
	case D3DRS_SPECULARMATERIALSOURCE:    
	case D3DRS_AMBIENTMATERIALSOURCE:     
	case D3DRS_EMISSIVEMATERIALSOURCE:    
		switch( value )
		{
		case D3DMCS_MATERIAL:
			fprintf( fp, "D3DMCS_MATERIAL" );
			break;
		case D3DMCS_COLOR1:
			fprintf( fp, "D3DMCS_COLOR1" );
			break;
		case D3DMCS_COLOR2:
			fprintf( fp, "D3DMCS_COLOR2" );
			break;
		}
		break;
	
	case D3DRS_VERTEXBLEND:               
		switch( value )
		{
		case D3DVBF_DISABLE:
			fprintf( fp, "D3DVBF_DISABLE" );
			break;
		case D3DVBF_1WEIGHTS:
			fprintf( fp, "D3DVBF_1WEIGHTS" );
			break;
		case D3DVBF_2WEIGHTS:
			fprintf( fp, "D3DVBF_2WEIGHTS" );
			break;
		case D3DVBF_3WEIGHTS:
			fprintf( fp, "D3DVBF_3WEIGHTS" );
			break;
		case D3DVBF_TWEENING:
			fprintf( fp, "D3DVBF_TWEENING" );
			break;
		case D3DVBF_0WEIGHTS:
			fprintf( fp, "D3DVBF_0WEIGHTS" );
			break;
		default:
			break;
		}
		break;
	case D3DRS_CLIPPLANEENABLE:           
	case D3DRS_MULTISAMPLEMASK:             // DWORD - per-sample enable/disable
	case D3DRS_COLORWRITEENABLE:            // per-channel write enable
		fprintf( fp, "0x%08x", ( int )value );
		break;
	case D3DRS_POINTSIZE:                    /* float point size */
	case D3DRS_POINTSIZE_MIN:                /* float point size min threshold */
	case D3DRS_POINTSCALE_A:                 /* float point attenuation A value */
	case D3DRS_POINTSCALE_B:                 /* float point attenuation B value */
	case D3DRS_POINTSCALE_C:                 /* float point attenuation C value */
	case D3DRS_POINTSIZE_MAX:                /* float point size max threshold */
	case D3DRS_TWEENFACTOR:                  // float tween factor
		fprintf( fp, "%f", *( float * )&value );
		break;
	case D3DRS_PATCHEDGESTYLE:              // Sets whether patch edges will use float style tessellation
	case D3DRS_DEBUGMONITORTOKEN:           // DEBUG ONLY - token to debug monitor
		fprintf( fp, "D3DRS_DEBUGMONITORTOKEN" );
		break;
	case D3DRS_BLENDOP:                      // D3DBLENDOP setting
		switch( value )
		{
		case D3DBLENDOP_ADD:
			fprintf( fp, "D3DBLENDOP_ADD" );
			break;
		case D3DBLENDOP_SUBTRACT:
			fprintf( fp, "D3DBLENDOP_SUBTRACT" );
			break;
		case D3DBLENDOP_REVSUBTRACT:
			fprintf( fp, "D3DBLENDOP_REVSUBTRACT" );
			break;
		case D3DBLENDOP_MIN:
			fprintf( fp, "D3DBLENDOP_MIN" );
			break;
		case D3DBLENDOP_MAX:
			fprintf( fp, "D3DBLENDOP_MAX" );
			break;
		default:
			Assert( 0 );
			break;
		}
		break;
	}
}


void ListFunc_SetRenderState( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 2 )
	{
		D3DRENDERSTATETYPE state;
		DWORD value;

		cmdBuffer.GetInt( *(int*)&state );
		cmdBuffer.GetInt( *(int*)&value );
		fprintf( fp, "%s: %s ", g_RecordingCommandNames[cmd], RenderStateToString( state ) );
		PrintRenderStateValue( fp, state, value );
		fprintf( fp, "\n" );
	}
	else
	{
		Assert( 0 );
	}
}

const char *TextureStageStateTypeToString( D3DTEXTURESTAGESTATETYPE type )
{
	switch( type )
	{
    case D3DTSS_COLOROP:        /* D3DTEXTUREOP - per-stage blending controls for color channels */
		return "D3DTSS_COLOROP";
    case D3DTSS_COLORARG1:      /* D3DTA_* (texture arg) */
		return "D3DTSS_COLORARG1";
    case D3DTSS_COLORARG2:      /* D3DTA_* (texture arg) */
		return "D3DTSS_COLORARG2";
    case D3DTSS_ALPHAOP:        /* D3DTEXTUREOP - per-stage blending controls for alpha channel */
		return "D3DTSS_ALPHAOP";
    case D3DTSS_ALPHAARG1:      /* D3DTA_* (texture arg) */
		return "D3DTSS_ALPHAARG1";
    case D3DTSS_ALPHAARG2:      /* D3DTA_* (texture arg) */
		return "D3DTSS_ALPHAARG2";
    case D3DTSS_BUMPENVMAT00:   /* float (bump mapping matrix) */
		return "D3DTSS_BUMPENVMAT00";
    case D3DTSS_BUMPENVMAT01:   /* float (bump mapping matrix) */
		return "D3DTSS_BUMPENVMAT01";
    case D3DTSS_BUMPENVMAT10:   /* float (bump mapping matrix) */
		return "D3DTSS_BUMPENVMAT10";
    case D3DTSS_BUMPENVMAT11:   /* float (bump mapping matrix) */
		return "D3DTSS_BUMPENVMAT11";
    case D3DTSS_TEXCOORDINDEX:  /* identifies which set of texture coordinates index this texture */
		return "D3DTSS_TEXCOORDINDEX";
    case D3DTSS_BUMPENVLSCALE:  /* float scale for bump map luminance */
		return "D3DTSS_BUMPENVLSCALE";
    case D3DTSS_BUMPENVLOFFSET: /* float offset for bump map luminance */
		return "D3DTSS_BUMPENVLOFFSET";
    case D3DTSS_TEXTURETRANSFORMFLAGS: /* D3DTEXTURETRANSFORMFLAGS controls texture transform */
		return "D3DTSS_TEXTURETRANSFORMFLAGS";
    case D3DTSS_COLORARG0:      /* D3DTA_* third arg for triadic ops */
		return "D3DTSS_COLORARG0";
    case D3DTSS_ALPHAARG0:      /* D3DTA_* third arg for triadic ops */
		return "D3DTSS_ALPHAARG0";
    case D3DTSS_RESULTARG:      /* D3DTA_* arg for result (CURRENT or TEMP) */
		return "D3DTSS_RESULTARG";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

const char *TextureOpToString( D3DTEXTUREOP op )
{
	switch( op )
	{
	case D3DTOP_DISABLE:
		return "D3DTOP_DISABLE";
	case D3DTOP_SELECTARG1:
		return "D3DTOP_SELECTARG1";
	case D3DTOP_SELECTARG2:
		return "D3DTOP_SELECTARG2";
	case D3DTOP_MODULATE:
		return "D3DTOP_MODULATE";
	case D3DTOP_MODULATE2X:
		return "D3DTOP_MODULATE2X";
	case D3DTOP_MODULATE4X:
		return "D3DTOP_MODULATE4X";
	case D3DTOP_ADD:
		return "D3DTOP_ADD";
	case D3DTOP_ADDSIGNED:
		return "D3DTOP_ADDSIGNED";
	case D3DTOP_ADDSIGNED2X:
		return "D3DTOP_ADDSIGNED2X";
	case D3DTOP_SUBTRACT:
		return "D3DTOP_SUBTRACT";
	case D3DTOP_ADDSMOOTH:
		return "D3DTOP_ADDSMOOTH";
	case D3DTOP_BLENDDIFFUSEALPHA:
		return "D3DTOP_BLENDDIFFUSEALPHA";
	case D3DTOP_BLENDTEXTUREALPHA:
		return "D3DTOP_BLENDTEXTUREALPHA";
	case D3DTOP_BLENDFACTORALPHA:
		return "D3DTOP_BLENDFACTORALPHA";
	case D3DTOP_BLENDTEXTUREALPHAPM:
		return "D3DTOP_BLENDTEXTUREALPHAPM";
	case D3DTOP_BLENDCURRENTALPHA:
		return "D3DTOP_BLENDCURRENTALPHA";
	case D3DTOP_PREMODULATE:
		return "D3DTOP_PREMODULATE";
	case D3DTOP_MODULATEALPHA_ADDCOLOR:
		return "D3DTOP_MODULATEALPHA_ADDCOLOR";
	case D3DTOP_MODULATECOLOR_ADDALPHA:
		return "D3DTOP_MODULATECOLOR_ADDALPHA";
	case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
		return "D3DTOP_MODULATEINVALPHA_ADDCOLOR";
	case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
		return "D3DTOP_MODULATEINVCOLOR_ADDALPHA";
	case D3DTOP_BUMPENVMAP:
		return "D3DTOP_BUMPENVMAP";
	case D3DTOP_BUMPENVMAPLUMINANCE:
		return "D3DTOP_BUMPENVMAPLUMINANCE";
	case D3DTOP_DOTPRODUCT3:
		return "D3DTOP_DOTPRODUCT3";
	case D3DTOP_MULTIPLYADD:
		return "D3DTOP_MULTIPLYADD";
	case D3DTOP_LERP:
		return "D3DTOP_LERP";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

static const char *TextureArgToString( int arg )
{
	static char buf[128];
	switch( arg & D3DTA_SELECTMASK )
	{
	case D3DTA_DIFFUSE:
		strcpy( buf, "D3DTA_DIFFUSE" );
		break;
	case D3DTA_CURRENT:
		strcpy( buf, "D3DTA_CURRENT" );
		break;
	case D3DTA_TEXTURE:
		strcpy( buf, "D3DTA_TEXTURE" );
		break;
	case D3DTA_TFACTOR:
		strcpy( buf, "D3DTA_TFACTOR" );
		break;
	case D3DTA_SPECULAR:
		strcpy( buf, "D3DTA_SPECULAR" );
		break;
	case D3DTA_TEMP:
		strcpy( buf, "D3DTA_TEMP" );
		break;
	default:
		strcpy( buf, "<ERROR>" );
		break;
	}

	if( arg & D3DTA_COMPLEMENT )
	{
		strcat( buf, "|D3DTA_COMPLEMENT" );
	}
	if( arg & D3DTA_ALPHAREPLICATE )
	{
		strcat( buf, "|D3DTA_ALPHAREPLICATE" );
	}
	return buf;
}

const char *TextureAddressToString( DWORD val )
{
	switch( val )
	{
	case D3DTADDRESS_WRAP:
		return "D3DTADDRESS_WRAP";
	case D3DTADDRESS_MIRROR:
		return "D3DTADDRESS_MIRROR";
	case D3DTADDRESS_CLAMP:
		return "D3DTADDRESS_CLAMP";
	case D3DTADDRESS_BORDER:
		return "D3DTADDRESS_BORDER";
	case D3DTADDRESS_MIRRORONCE:
		return "D3DTADDRESS_MIRRORONCE";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

const char *TextureFilterToString( DWORD val )
{
	switch( val )
	{
	case D3DTEXF_NONE:
		return "D3DTEXF_NONE";
	case D3DTEXF_POINT:
		return "D3DTEXF_POINT";
	case D3DTEXF_LINEAR:
		return "D3DTEXF_LINEAR";
	case D3DTEXF_ANISOTROPIC:
		return "D3DTEXF_ANISOTROPIC";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

const char *TextureTransformFlagsToString( DWORD val )
{
	if( val & D3DTTFF_PROJECTED )
	{
		val &= ~D3DTTFF_PROJECTED;
		switch( val )
		{
		case D3DTTFF_DISABLE:
			return "D3DTTFF_PROJECTED|D3DTTFF_DISABLE";
		case D3DTTFF_COUNT1:
			return "D3DTTFF_PROJECTED|D3DTTFF_COUNT1";
		case D3DTTFF_COUNT2:
			return "D3DTTFF_PROJECTED|D3DTTFF_COUNT2";
		case D3DTTFF_COUNT3:
			return "D3DTTFF_PROJECTED|D3DTTFF_COUNT3";
		case D3DTTFF_COUNT4:
			return "D3DTTFF_PROJECTED|D3DTTFF_COUNT4";
		case D3DTTFF_PROJECTED:
			return "D3DTTFF_PROJECTED|D3DTTFF_PROJECTED";
		default:
			Assert( 0 );
			return "ERROR";
		}
	}
	else
	{
		switch( val )
		{
		case D3DTTFF_DISABLE:
			return "D3DTTFF_DISABLE";
		case D3DTTFF_COUNT1:
			return "D3DTTFF_COUNT1";
		case D3DTTFF_COUNT2:
			return "D3DTTFF_COUNT2";
		case D3DTTFF_COUNT3:
			return "D3DTTFF_COUNT3";
		case D3DTTFF_COUNT4:
			return "D3DTTFF_COUNT4";
		case D3DTTFF_PROJECTED:
			return "D3DTTFF_PROJECTED";
		default:
			Assert( 0 );
			return "ERROR";
		}
	}
}


void PrintTextureStageStateValue( FILE *fp, D3DTEXTURESTAGESTATETYPE state, DWORD value )
{
	switch( ( int )state )
	{
    case D3DTSS_COLOROP:        /* D3DTEXTUREOP - per-stage blending controls for color channels */
    case D3DTSS_ALPHAOP:        /* D3DTEXTUREOP - per-stage blending controls for alpha channel */
		fprintf( fp, "%s", TextureOpToString( ( D3DTEXTUREOP )value ) );
		break;
    case D3DTSS_COLORARG1:      /* D3DTA_* (texture arg) */
    case D3DTSS_COLORARG2:      /* D3DTA_* (texture arg) */
    case D3DTSS_ALPHAARG1:      /* D3DTA_* (texture arg) */
    case D3DTSS_ALPHAARG2:      /* D3DTA_* (texture arg) */
    case D3DTSS_COLORARG0:      /* D3DTA_* third arg for triadic ops */
    case D3DTSS_ALPHAARG0:      /* D3DTA_* third arg for triadic ops */
    case D3DTSS_RESULTARG:      /* D3DTA_* arg for result (CURRENT or TEMP) */
		fprintf( fp, "%s", TextureArgToString( value ) );
		break;
    case D3DTSS_BUMPENVMAT00:   /* float (bump mapping matrix) */
    case D3DTSS_BUMPENVMAT01:   /* float (bump mapping matrix) */
    case D3DTSS_BUMPENVMAT10:   /* float (bump mapping matrix) */
    case D3DTSS_BUMPENVMAT11:   /* float (bump mapping matrix) */
		fprintf( fp, "%f", *( float * )&value );
		break;
    case D3DTSS_TEXCOORDINDEX:  /* identifies which set of texture coordinates index this texture */
		fprintf( fp, "%d", ( int )value );
		break;
    case D3DTSS_BUMPENVLSCALE:  /* float scale for bump map luminance */
		fprintf( fp, "%f", *( float * )&value );
		break;
    case D3DTSS_BUMPENVLOFFSET: /* float offset for bump map luminance */
		fprintf( fp, "%f", *( float * )&value );
		break;
    case D3DTSS_TEXTURETRANSFORMFLAGS: /* D3DTEXTURETRANSFORMFLAGS controls texture transform */
		fprintf( fp, "%s", TextureTransformFlagsToString( value ) );
		break;
	default:
		Assert( 0 );
		break;
	}
}

void ListFunc_SetTextureStageState( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 3 )
	{
		int stage;
		D3DTEXTURESTAGESTATETYPE state;
		DWORD value;

		data.GetInt( stage );
		data.GetInt( *(int*)&state );
		data.GetInt( *(int*)&value );

		fprintf( fp, "%s: stage: %d state: %s, value: ", g_RecordingCommandNames[cmd],
			stage, TextureStageStateTypeToString( state ) );
		PrintTextureStageStateValue( fp, state, value );
		fprintf( fp, "\n" );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetVertexPixelShaderConstant( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 3 )
	{
		int numVecs, var;
		cmdBuffer.GetInt( var );
		cmdBuffer.GetInt( numVecs );
		fprintf( fp, "%s: reg: %d numvecs: %d val: ", g_RecordingCommandNames[cmd], var, numVecs );
		int i;
		for( i = 0; i < numVecs; i++ )
		{
			float f[4];
			cmdBuffer.GetFloat( f[0] );
			cmdBuffer.GetFloat( f[1] );
			cmdBuffer.GetFloat( f[2] );
			cmdBuffer.GetFloat( f[3] );
			fprintf( fp, "[%f %f %f %f]", f[0], f[1], f[2], f[3] );
		}
		fprintf( fp, "\n" );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetViewport( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 1 )
	{
		D3DVIEWPORT viewport;
		data.Get( &viewport, sizeof( viewport ) );
		fprintf( fp, "%s: X: %d Y: %d Width: %d Height: %d MinZ: %.1f MaxZ: %.1f\n", g_RecordingCommandNames[cmd], 
			( int )viewport.X, ( int )viewport.Y, ( int )viewport.Width, ( int )viewport.Height,
			( float )viewport.MinZ, ( float )viewport.MaxZ );
		
	}
	else
	{
		Assert( 0 );
	}
}

void PrintVertexFormat( FILE *fp, unsigned int vertexFormat )
{
	if( vertexFormat & VERTEX_POSITION )
	{
		fprintf( fp, "VERTEX_POSITION|" );
	}
	if( vertexFormat & VERTEX_NORMAL )
	{
		fprintf( fp, "VERTEX_NORMAL|" );
	}
	if( vertexFormat & VERTEX_COLOR )
	{
		fprintf( fp, "VERTEX_COLOR|" );
	}
	if( vertexFormat & VERTEX_SPECULAR )
	{
		fprintf( fp, "VERTEX_SPECULAR|" );
	}
	if( vertexFormat & VERTEX_TANGENT_S )
	{
		fprintf( fp, "VERTEX_TANGENT_S|" );
	}
	if( vertexFormat & VERTEX_TANGENT_T )
	{
		fprintf( fp, "VERTEX_TANGENT_T|" );
	}
	if( vertexFormat & VERTEX_BONE_INDEX )
	{
		fprintf( fp, "VERTEX_BONE_INDEX|" );
	}
	if( vertexFormat & VERTEX_FORMAT_VERTEX_SHADER )
	{
		fprintf( fp, "VERTEX_FORMAT_VERTEX_SHADER|" );
	}
	if( NumBoneWeights( vertexFormat ) > 0 )
	{
		fprintf( fp, "VERTEX_BONEWEIGHT(%d)|", NumBoneWeights( vertexFormat ) );
	}
	if( UserDataSize( vertexFormat ) > 0 )
	{
		fprintf( fp, "VERTEX_USERDATA_SIZE(%d)|", UserDataSize( vertexFormat ) );
	}
	if( NumTextureCoordinates( vertexFormat ) > 0 )
	{
		fprintf( fp, "VERTEX_NUM_TEXCOORDS(%d)|", NumTextureCoordinates( vertexFormat ) );
	}
	int i;
	for( i = 0; i < NumTextureCoordinates( vertexFormat ); i++ )
	{
		fprintf( fp, "VERTEX_TEXCOORD_SIZE(%d,%d)", i, TexCoordSize( i, vertexFormat ) );
	}
}

void PrintVertexBuffer( unsigned char *pVertexData, int bufferSize, unsigned int vertexFormat, FILE *fp )
{
#ifdef PRINT_VERTEX_BUFFER
	int offset = 0;
	int positionOffset = 0;
	bool hasPosition = vertexFormat & VERTEX_POSITION;	
	if( hasPosition )
	{
		positionOffset = offset;
		offset += sizeof( float ) * 3;
	}

	int numBoneWeights = NumBoneWeights( vertexFormat );
	int boneWeightOffset = 0;
	int boneIndexOffset;
	bool hasBoneWeights = numBoneWeights > 0;
	bool hasBoneIndices = false;
	if( hasBoneWeights )
	{
		boneWeightOffset = offset;
		offset += numBoneWeights * sizeof( float );

		if( vertexFormat & VERTEX_BONE_INDEX )
		{
			hasBoneIndices = true;
			boneIndexOffset = offset;
			offset += 4 * sizeof( unsigned char );
		}
	}

	bool hasNormal = ( vertexFormat & VERTEX_NORMAL ) != 0;
	int normalOffset = 0;
	if( hasNormal )
	{
		normalOffset = offset;
		offset += sizeof( float ) * 3;
	}

	bool hasColor = ( vertexFormat & VERTEX_COLOR ) != 0;
	int colorOffset = 0;
	if( hasColor )
	{
		colorOffset = offset;
		offset += 4 * sizeof( unsigned char );
	}

	bool hasSpecular = ( vertexFormat & VERTEX_SPECULAR ) != 0;
	int specularOffset = 0;
	if( hasSpecular )
	{
		specularOffset = offset;
		offset += 4 * sizeof( unsigned char );
	}


#define VERTEX_MAX_TEXTURE_COORDINATES 4
	
	bool hasTexCoords[VERTEX_MAX_TEXTURE_COORDINATES];
	int texCoordOffset[VERTEX_MAX_TEXTURE_COORDINATES];
	int texCoordSize[VERTEX_MAX_TEXTURE_COORDINATES];
	int i;
	for( i = 0; i < NumTextureCoordinates( vertexFormat ); i++ )
	{
		hasTexCoords[i] = true;
		texCoordSize[i] = TexCoordSize( i, vertexFormat );
		texCoordOffset[i] = offset;
		offset += texCoordSize[i] * sizeof( float );
	}
	for( ; i < VERTEX_MAX_TEXTURE_COORDINATES; i++ )
	{
		hasTexCoords[i] = false;
	}

	bool hasTangentS = ( vertexFormat & VERTEX_TANGENT_S ) != 0;
	int tangentSOffset = 0;
	if( hasTangentS )
	{
		tangentSOffset = offset;
		offset += 3 * sizeof( float );
	}

	bool hasTangentT = ( vertexFormat & VERTEX_TANGENT_T ) != 0;
	int tangentTOffset = 0;
	if( hasTangentT )
	{
		tangentTOffset = offset;
		offset += 3 * sizeof( float );
	}

	bool hasUserData = UserDataSize( vertexFormat ) > 0;
	int userDataOffset = 0;
	if( hasUserData )
	{
		userDataOffset = offset;
		offset += UserDataSize( vertexFormat ) * sizeof( float );
	}

	// We always use vertex sizes which are half-cache aligned (16 bytes)
	offset = (offset + 0xF) & (~0xF);

	int vertexSize = offset;

	Assert( bufferSize % vertexSize == 0 );

	int numVerts = bufferSize / vertexSize;
	for( i = 0; i < numVerts; i++ )
	{
		unsigned char *pVert = pVertexData + i * vertexSize;
		fprintf( fp, "vertex: %d ", i );
		if( hasPosition )
		{
			float *pPos = ( float * )( pVert + positionOffset );
			fprintf( fp, "pos: %f %f %f ", pPos[0], pPos[1], pPos[2] );
		}

		if( hasBoneWeights )
		{
			float *pBoneWeights = ( float * )( pVert + boneWeightOffset  );
			int j;
			fprintf( fp, "boneweights: " );
			for( j = 0; j < numBoneWeights; j++ )
			{
				fprintf( fp, "%f ", pBoneWeights[j] );
			}
		}
		if( hasBoneIndices )
		{
			unsigned char *pBoneIndices = pVert + boneIndexOffset;
			fprintf( fp, "boneindices: %d %d %d %d ", ( int )pBoneIndices[0], ( int )pBoneIndices[1], ( int )pBoneIndices[2], ( int )pBoneIndices[3] );
		}
		if( hasNormal )
		{
			float *pNormal = ( float * )( pVert + normalOffset );
			fprintf( fp, "normal: %f %f %f ", pNormal[0], pNormal[1], pNormal[2] );
		}
		if( hasColor )
		{
			unsigned char *pColor = pVert + colorOffset;
			fprintf( fp, "color: %d %d %d %d ", ( int )pColor[0], ( int )pColor[1], ( int )pColor[2], ( int )pColor[3] );
		}
		if( hasSpecular )
		{
			unsigned char *pSpecular = pVert + specularOffset;
			fprintf( fp, "specular: %d %d %d %d ", ( int )pSpecular[0], ( int )pSpecular[1], ( int )pSpecular[2], ( int )pSpecular[3] );
		}
		int j;
		for( j = 0; j < VERTEX_MAX_TEXTURE_COORDINATES; j++ )
		{
			if( hasTexCoords[j] )
			{
				fprintf( fp, "texcoords(%d): ", j );
				int k;
				float *pTexCoord = ( float * )( pVert + texCoordOffset[j] );
				for( k = 0; k < texCoordSize[j]; k++ )
				{
					fprintf( fp, "%f ", pTexCoord[k] );
				}
			}
		}
		if( hasTangentS )
		{
			float *pTangentS = ( float * )( pVert + tangentSOffset );
			fprintf( fp, "tangentS: %f %f %f ", pTangentS[0], pTangentS[1], pTangentS[2] );
		}
		if( hasTangentT )
		{
			float *pTangentT = ( float * )( pVert + tangentTOffset );
			fprintf( fp, "tangentT: %f %f %f ", pTangentT[0], pTangentT[1], pTangentT[2] );
		}
		if( hasUserData )
		{
			float *pUserData = ( float * )( pVert + userDataOffset );
			fprintf( fp, "UserData: " );
			int k;
			for( k = 0; k < UserDataSize( vertexFormat ); k++ )
			{
				fprintf( fp, "%f ", pUserData[k] );
			}
		}
		fprintf( fp, "\n" );
	}
#endif // PRINT_VERTEX_BUFFER
}

void ListFunc_VertexData( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 3 )
	{
		int size;
		VertexBufferFormatLookup_t vertexBufferFormat;
		cmdBuffer.GetInt( vertexBufferFormat.m_Id );
		cmdBuffer.GetInt( size );
		fprintf( fp, "%s: id: %d size: %d\n", g_RecordingCommandNames[cmd], vertexBufferFormat.m_Id, size );
		VertexFormat_t vertexFormat;
		int i = g_VertexBufferFormat.Find( vertexBufferFormat );
		if (i >= 0)
		{
			vertexFormat = g_VertexBufferFormat[i].m_VertexFormat;
			unsigned char *pVertexData = ( unsigned char * )cmdBuffer.PeekGet();
			PrintVertexBuffer( pVertexData, size, vertexFormat, fp );
		}
		else
		{
//			Assert( 0 );
			return;
		}
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_LockVertexBuffer( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 4 )
	{
		UINT offset, size;
		DWORD flags;
		VertexBufferLookup_t vertexBuffer;

	    data.GetInt( vertexBuffer.m_Id );
		data.GetInt( *(int*)&offset );
		data.GetInt( *(int*)&size );
		data.GetInt( *(int*)&flags );

		fprintf( fp, "%s: id: %d offset: %d size: %d flags: 0x%x\n",
			g_RecordingCommandNames[cmd],
			( int )vertexBuffer.m_Id,
			( int )offset,
			( int )size,
			( int )flags
			);
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_IndexData( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 3 )
	{
		int id, size;
		cmdBuffer.GetInt( id );
		cmdBuffer.GetInt( size );
		fprintf( fp, "%s: id: %d size: %d\n", g_RecordingCommandNames[cmd], id, size );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_LockIndexBuffer( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 4 )
	{
		UINT offset, size;
		DWORD flags;
		IndexBufferLookup_t indexBuffer;

	    data.GetInt( indexBuffer.m_Id );
		data.GetInt( *(int*)&offset );
		data.GetInt( *(int*)&size );
		data.GetInt( *(int*)&flags );

		fprintf( fp, "%s: id: %d offset: %d size: %d: flags: 0x%x\n", g_RecordingCommandNames[cmd], 
			indexBuffer.m_Id, offset, 
			size, flags );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_UnlockIndexBuffer( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 1 )
	{
		IndexBufferLookup_t indexBuffer;
	    data.GetInt( indexBuffer.m_Id );
		fprintf( fp, "%s: id: %d\n", g_RecordingCommandNames[cmd], indexBuffer.m_Id );
	}
	else
	{
		Assert( 0 );
	}
}

static void PrintBufferUsage( FILE *fp, DWORD usage )
{
	if( usage & D3DUSAGE_SOFTWAREPROCESSING )
	{
		fprintf( fp, "D3DUSAGE_SOFTWAREPROCESSING|" );
	}
	if( usage & D3DUSAGE_DONOTCLIP )
	{
		fprintf( fp, "D3DUSAGE_DONOTCLIP|" );
	}
	if( usage & D3DUSAGE_DYNAMIC )
	{
		fprintf( fp, "D3DUSAGE_DYNAMIC|" );
	}
	if( usage & D3DUSAGE_RTPATCHES )
	{
		fprintf( fp, "D3DUSAGE_RTPATCHES|" );
	}
	if( usage & D3DUSAGE_NPATCHES )
	{
		fprintf( fp, "D3DUSAGE_NPATCHES|" );
	}
	if( usage & D3DUSAGE_POINTS )
	{
		fprintf( fp, "D3DUSAGE_POINTS|" );
	}
	if( usage & D3DUSAGE_WRITEONLY )
	{
		fprintf( fp, "D3DUSAGE_WRITEONLY|") ;
	}
}

void ListFunc_CreateVertexBuffer( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 6 )
	{
		int id;
		UINT length;
		DWORD usage, fvf;
		D3DPOOL pool;
		int dynamic;

	    cmdBuffer.GetInt( id );
		cmdBuffer.GetInt( *(int*)&length );
		cmdBuffer.GetInt( *(int*)&usage );
		cmdBuffer.GetInt( *(int*)&fvf );
		cmdBuffer.GetInt( *(int*)&pool );
		cmdBuffer.GetInt( dynamic );

		fprintf( fp, "%s: id: %d length: %d usage: (", g_RecordingCommandNames[cmd], id, 
			( int )length );
		PrintBufferUsage( fp, usage );	
		fprintf( fp, ") fvf: %d pool: ", ( int )fvf );
		switch( pool )
		{
		case D3DPOOL_DEFAULT:
			fprintf( fp, "D3DPOOL_DEFAULT" );
			break;
		case D3DPOOL_MANAGED:
			fprintf( fp, "D3DPOOL_MANAGED" );
			break;
		case D3DPOOL_SYSTEMMEM:
			fprintf( fp, "D3DPOOL_SYSTEMMEM" );
			break;
		case D3DPOOL_SCRATCH:
			fprintf( fp, "D3DPOOL_SCRATCH" );
			break;
		}
		fprintf( fp, "\n" );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_DestroyVertexIndexBuffer( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 1 )
	{
		int id;
	    cmdBuffer.GetInt( id );
		fprintf( fp, "%s: id: %d\n", g_RecordingCommandNames[cmd], id );
	}
	else
	{
		Assert( 0 );
	}
}

static const char *D3DFormatToString( D3DFORMAT fmt )
{
	switch( fmt )
	{
    case D3DFMT_UNKNOWN:
		return "D3DFMT_UNKNOWN";
    case D3DFMT_R8G8B8:
		return "D3DFMT_R8G8B8";
    case D3DFMT_A8R8G8B8:
		return "D3DFMT_A8R8G8B8";
    case D3DFMT_X8R8G8B8:
		return "D3DFMT_X8R8G8B8";
    case D3DFMT_R5G6B5:
		return "D3DFMT_R5G6B5";
    case D3DFMT_X1R5G5B5:
		return "D3DFMT_X1R5G5B5";
    case D3DFMT_A1R5G5B5:
		return "D3DFMT_A1R5G5B5";
    case D3DFMT_A4R4G4B4:
		return "D3DFMT_A4R4G4B4";
    case D3DFMT_R3G3B2:
		return "D3DFMT_R3G3B2";
    case D3DFMT_A8:
		return "D3DFMT_A8";
    case D3DFMT_A8R3G3B2:
		return "D3DFMT_A8R3G3B2";
    case D3DFMT_X4R4G4B4:
		return "D3DFMT_X4R4G4B4";
    case D3DFMT_A2B10G10R10:
		return "D3DFMT_A2B10G10R10";
    case D3DFMT_G16R16:
		return "D3DFMT_G16R16";
    case D3DFMT_A8P8:
		return "D3DFMT_A8P8";
    case D3DFMT_P8:
		return "D3DFMT_P8";
    case D3DFMT_L8:
		return "D3DFMT_L8";
    case D3DFMT_A8L8:
		return "D3DFMT_A8L8";
    case D3DFMT_A4L4:
		return "D3DFMT_A4L4";
    case D3DFMT_V8U8:
		return "D3DFMT_V8U8";
    case D3DFMT_L6V5U5:
		return "D3DFMT_L6V5U5";
    case D3DFMT_X8L8V8U8:
		return "D3DFMT_X8L8V8U8";
    case D3DFMT_Q8W8V8U8:
		return "D3DFMT_Q8W8V8U8";
    case D3DFMT_V16U16:
		return "D3DFMT_V16U16";
    case D3DFMT_A2W10V10U10:
		return "D3DFMT_A2W10V10U10";
    case D3DFMT_UYVY:
		return "D3DFMT_UYVY";
    case D3DFMT_YUY2:
		return "D3DFMT_YUY2";
    case D3DFMT_DXT1:
		return "D3DFMT_DXT1";
    case D3DFMT_DXT2:
		return "D3DFMT_DXT2";
    case D3DFMT_DXT3:
		return "D3DFMT_DXT3";
    case D3DFMT_DXT4:
		return "D3DFMT_DXT4";
    case D3DFMT_DXT5:
		return "D3DFMT_DXT5";
    case D3DFMT_D16_LOCKABLE:
		return "D3DFMT_D16_LOCKABLE";
    case D3DFMT_D32:
		return "D3DFMT_D32";
    case D3DFMT_D15S1:
		return "D3DFMT_D15S1";
    case D3DFMT_D24S8:
		return "D3DFMT_D24S8";
    case D3DFMT_D16:
		return "D3DFMT_D16";
    case D3DFMT_D24X8:
		return "D3DFMT_D24X8";
    case D3DFMT_D24X4S4:
		return "D3DFMT_D24X4S4";
    case D3DFMT_VERTEXDATA:
		return "D3DFMT_VERTEXDATA";
    case D3DFMT_INDEX16:
		return "D3DFMT_INDEX16";
    case D3DFMT_INDEX32:
		return "D3DFMT_INDEX32";
	default:
		return "UNKNOWN!";
	}
}

void ListFunc_CreateIndexBuffer( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 6 )
	{
		int id;
		UINT length;
		DWORD usage;
		D3DFORMAT fmt;
		D3DPOOL pool;
		int dynamic;

	    cmdBuffer.GetInt( id );
		cmdBuffer.GetInt( *(int*)&length );
		cmdBuffer.GetInt( *(int*)&usage );
		cmdBuffer.GetInt( *(int*)&fmt );
		cmdBuffer.GetInt( *(int*)&pool );
		cmdBuffer.GetInt( dynamic );

		fprintf( fp, "%s: id: %d length: %d usage: (", g_RecordingCommandNames[cmd], id, 
			( int )length );
		PrintBufferUsage( fp, usage );	
		fprintf( fp, ") fmt: %s pool: ", D3DFormatToString( fmt ) );
		switch( pool )
		{
		case D3DPOOL_DEFAULT:
			fprintf( fp, "D3DPOOL_DEFAULT" );
			break;
		case D3DPOOL_MANAGED:
			fprintf( fp, "D3DPOOL_MANAGED" );
			break;
		case D3DPOOL_SYSTEMMEM:
			fprintf( fp, "D3DPOOL_SYSTEMMEM" );
			break;
		case D3DPOOL_SCRATCH:
			fprintf( fp, "D3DPOOL_SCRATCH" );
			break;
		}
		fprintf( fp, "\n" );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_Clear( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 6 )
	{
		DWORD numRect, flags, stencil;
		D3DRECT rect;
		D3DCOLOR color;
		float z;

		cmdBuffer.GetInt( *(int*)&numRect );
		cmdBuffer.Get( &rect, sizeof(rect) );
		cmdBuffer.GetInt( *(int*)&flags );
		cmdBuffer.GetInt( *(int*)&color );
		cmdBuffer.GetFloat( z );
		cmdBuffer.GetInt( *(int*)&stencil );

		fprintf( fp, "%s: numrect: %d rect: %d %d %d %d flags: (", g_RecordingCommandNames[cmd],
			( int )numRect, ( int )rect.x1, ( int )rect.y1, ( int )rect.x2, ( int )rect.y2 );
		if( flags & D3DCLEAR_STENCIL )
		{
			fprintf( fp, "D3DCLEAR_STENCIL|" );
		}
		if( flags & D3DCLEAR_STENCIL )
		{
			fprintf( fp, "D3DCLEAR_TARGET|" );
		}
		if( flags & D3DCLEAR_ZBUFFER )
		{
			fprintf( fp, "D3DCLEAR_ZBUFFER|" );
		}
		fprintf( fp, ") color: 0x%x depth: %f stencil: %d\n", ( int )color, ( float )z, ( int )stencil );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetStreamSource( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 3 )
	{
		int vertexBufferID, streamID, vertexSize;
		cmdBuffer.GetInt( vertexBufferID );
		cmdBuffer.GetInt( streamID );
		cmdBuffer.GetInt( vertexSize );

		fprintf( fp, "%s: vertexBufferID: %d streamID: %d vertexSize: %d\n", g_RecordingCommandNames[cmd],
			( int )vertexBufferID, ( int )streamID, ( int )vertexSize );
	}
	else
	{
		Assert( 0 );
	}
}

const char *DeviceTypeToString( D3DDEVTYPE devType )
{
	switch( devType )
	{
	case D3DDEVTYPE_HAL:
		return "D3DDEVTYPE_HAL";
	case D3DDEVTYPE_REF:
		return "D3DDEVTYPE_REF";
	case D3DDEVTYPE_SW:
		return "D3DDEVTYPE_SW";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

const char *MultisampleTypeToString( D3DMULTISAMPLE_TYPE type )
{
	switch( type )
	{
    case D3DMULTISAMPLE_NONE:
		return "D3DMULTISAMPLE_NONE";
    case D3DMULTISAMPLE_NONMASKABLE:
		return "D3DMULTISAMPLE_NONMASKABLE";
    case D3DMULTISAMPLE_2_SAMPLES:
		return "D3DMULTISAMPLE_2_SAMPLES";
    case D3DMULTISAMPLE_3_SAMPLES:
		return "D3DMULTISAMPLE_3_SAMPLES";
    case D3DMULTISAMPLE_4_SAMPLES:
		return "D3DMULTISAMPLE_4_SAMPLES";
    case D3DMULTISAMPLE_5_SAMPLES:
		return "D3DMULTISAMPLE_5_SAMPLES";
    case D3DMULTISAMPLE_6_SAMPLES:
		return "D3DMULTISAMPLE_6_SAMPLES";
    case D3DMULTISAMPLE_7_SAMPLES:
		return "D3DMULTISAMPLE_7_SAMPLES";
    case D3DMULTISAMPLE_8_SAMPLES:
		return "D3DMULTISAMPLE_8_SAMPLES";
    case D3DMULTISAMPLE_9_SAMPLES:
		return "D3DMULTISAMPLE_9_SAMPLES";
    case D3DMULTISAMPLE_10_SAMPLES:
		return "D3DMULTISAMPLE_10_SAMPLES";
    case D3DMULTISAMPLE_11_SAMPLES:
		return "D3DMULTISAMPLE_11_SAMPLES";
    case D3DMULTISAMPLE_12_SAMPLES:
		return "D3DMULTISAMPLE_12_SAMPLES";
    case D3DMULTISAMPLE_13_SAMPLES:
		return "D3DMULTISAMPLE_13_SAMPLES";
    case D3DMULTISAMPLE_14_SAMPLES:
		return "D3DMULTISAMPLE_14_SAMPLES";
    case D3DMULTISAMPLE_15_SAMPLES:
		return "D3DMULTISAMPLE_15_SAMPLES";
    case D3DMULTISAMPLE_16_SAMPLES:
		return "D3DMULTISAMPLE_16_SAMPLES";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

const char *SwapEffectToString( D3DSWAPEFFECT swapEffect )
{
	switch( swapEffect )
	{
	case D3DSWAPEFFECT_DISCARD:
		return "D3DSWAPEFFECT_DISCARD";
    case D3DSWAPEFFECT_FLIP:
		return "D3DSWAPEFFECT_FLIP";
    case D3DSWAPEFFECT_COPY:
		return "D3DSWAPEFFECT_COPY";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

void PrintPresentFlags( FILE *fp, DWORD flags )
{
	fprintf( fp, "flags: (" );
	if( flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER )
	{
		fprintf( fp, "D3DPRESENTFLAG_LOCKABLE_BACKBUFFER|" );
	}
	if( flags & D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL )
	{
		fprintf( fp, "D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL|" );
	}
	if( flags & D3DPRESENTFLAG_DEVICECLIP )
	{
		fprintf( fp, "D3DPRESENTFLAG_DEVICECLIP|" );
	}
	if( flags & D3DPRESENTFLAG_VIDEO )
	{
		fprintf( fp, "D3DPRESENTFLAG_VIDEO|" );
	}
	fprintf( fp, ") ");
}

void PrintPresentParams( FILE *fp, const D3DPRESENT_PARAMETERS &parm )
{
	fprintf( fp, "backbufferwidth: %d, backbufferheight: %d backbufferformat: %s, backbuffercount %d, multisampletype: %s, multisamplequality: %d swapeffect: %s windowed: %s autodepthstencil: %s fullscreen_refreshrate: %d, presentation_interval: %d ", 
		( int )parm.BackBufferWidth, ( int )parm.BackBufferHeight,
		D3DFormatToString( parm.BackBufferFormat ), ( int )parm.BackBufferCount,
		MultisampleTypeToString( parm.MultiSampleType ), ( int )parm.MultiSampleQuality,
		SwapEffectToString( parm.SwapEffect ), parm.Windowed ? "TRUE" : "FALSE",
		parm.EnableAutoDepthStencil ? "TRUE" : "FALSE",
		( int )parm.FullScreen_RefreshRateInHz,
		( int )parm.PresentationInterval );
	PrintPresentFlags( fp, parm.Flags );
	fprintf( fp, "\n" );
}

void PrintDeviceCreationFlags( int deviceCreationFlags )
{
	
}

void ListFunc_CreateDevice( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 6 )
	{
		int displayAdapter, width, height, deviceCreationFlags;
		D3DDEVTYPE deviceType;
		D3DPRESENT_PARAMETERS presentParameters;

		data.GetInt( width );
		data.GetInt( height );
		data.GetInt( displayAdapter );
		data.GetInt( *(int*)&deviceType );
		data.GetInt( deviceCreationFlags );
		data.Get( &presentParameters, sizeof(presentParameters) );
		fprintf( fp, "%s: width: %d height: %d displayAdapter: %d deviceType: %s ",
			g_RecordingCommandNames[cmd], width, height, ( int )displayAdapter,
			DeviceTypeToString( deviceType ) );
		PrintDeviceCreationFlags( deviceCreationFlags );
		PrintPresentParams( fp, presentParameters );
		fprintf( fp, "\n" );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_Present( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	ListFunc_Default( fp, data, cmd, numargs );
	fprintf( fp, "\n\n\n\n\n" );
}

void ListFunc_CreateTexture( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( ( numargs == 7 ) || ( numargs == 10 ) || ( numargs == 11 ) )
	{
		TextureLookup_t texture;
		int width, height, numLevels, isCubeMap, numCopies;
		int isDepthBuffer, isRenderTarget, isManaged, isDynamic;
		D3DFORMAT format;

	    data.GetInt( texture.m_BindId );
	    data.GetInt( width );
	    data.GetInt( height );
	    data.GetInt( *(int*)&format );
	    data.GetInt( numLevels );
	    data.GetInt( isCubeMap );
	    data.GetInt( numCopies );
		switch( numargs )
		{
		case 10:
			data.GetInt( isRenderTarget );
			data.GetInt( isManaged );
			data.GetInt( isDepthBuffer );
			isDynamic = 0;
			break;
		case 11:
			data.GetInt( isRenderTarget );
			data.GetInt( isManaged );
			data.GetInt( isDepthBuffer );
			data.GetInt( isDynamic );
			break;
		case 7:
			isRenderTarget = 0;
			isManaged = 1;
			isDepthBuffer = 0;
			isDynamic = 0;
			break;
		default:
			Assert( 0 );
		}
		fprintf( fp, "%s: id: %d width: %d height: %d format: %s numLevels: %d isCubeMap: %s numCopies: %d isRenderTarget: %s isManaged: %s isDepthBuffer: %s isDynamic: %s\n",
			g_RecordingCommandNames[cmd], ( int )texture.m_BindId,
			width, height, D3DFormatToString( format ), numLevels,
			isCubeMap ? "TRUE" : "FALSE", numCopies, isRenderTarget ? "TRUE" : "FALSE",
			isManaged ? "TRUE" : "FALSE", isDepthBuffer ? "TRUE" : "FALSE",
			isDynamic ? "TRUE" : "FALSE" );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetTexture( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 3 )
	{
		int stage, bindID, copyID;
		cmdBuffer.GetInt( stage );
		cmdBuffer.GetInt( bindID );
		cmdBuffer.GetInt( copyID );

		fprintf( fp, "%s: stage: %d bindID: %d copyID: %d\n", g_RecordingCommandNames[cmd],
			( int )stage, ( int )bindID, ( int )copyID );
	}
	else
	{
		Assert( 0 );
	}
}

void DisassembleShader( FILE *fp, DWORD *pByteCode, int lengthInBytes, int shaderID, bool isVertexShader )
{
	DWORD *pCur = pByteCode;
	DWORD opcode;
	while( 1 )
	{
		opcode = *pCur & 0x0000FFFF;
		if( opcode == D3DSIO_END )
		{
			break;
		}
		if( opcode == D3DSIO_COMMENT )
		{
			int commentSize = ( *pCur & D3DSI_COMMENTSIZE_MASK ) >> D3DSI_COMMENTSIZE_SHIFT;
			pCur++;
			if( strncmp( "LINE", ( char * )pCur, 4 ) != 0 )
			{
				if( isVertexShader )
				{
					fprintf( fp, "VertexShaderComment: id: %d \"%s\"\n", shaderID, ( char * )pCur );
				}
				else
				{
					fprintf( fp, "PixelShaderComment: id: %d \"%s\"\n", shaderID, ( char * )pCur );
				}
			}
			pCur += commentSize;
		}
		else
		{
			pCur++;
		}
		Assert( ( char * )pCur - ( char * )pByteCode < lengthInBytes );
	}
}

void ListFunc_CreateVertexShader( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 2 )
	{
		// This is the FVF version. . ie. no byte code
		VertexShaderLookup_t vertexShader;
		DWORD dwDecl[32];

	    data.GetInt( vertexShader.m_Id );
		data.Get( dwDecl, 32 * sizeof(DWORD) );
		fprintf( fp, "%s: id: %d dwDecl: ", g_RecordingCommandNames[cmd], 
			( int )vertexShader.m_Id );
	}
	else if ( numargs == 3 )
	{
		// This is the vertex shader version, ie. we have byte code
		VertexShaderLookup_t vertexShader;
		int codeSize;

		data.GetInt( vertexShader.m_Id );
		data.GetInt( codeSize );
		fprintf( fp, "%s: id: %d dwDecl: ", g_RecordingCommandNames[cmd], 
			( int )vertexShader.m_Id );
		DisassembleShader( fp, ( DWORD * )data.PeekGet(), codeSize, vertexShader.m_Id, true );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_CreatePixelShader( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 3 )
	{
		PixelShaderLookup_t pixelShader;
		int codeSize;

	    data.GetInt( pixelShader.m_Id );
		data.GetInt( codeSize );
		fprintf( fp, "%s: id: %d codeSize: %d\n", g_RecordingCommandNames[cmd], 
			( int )pixelShader.m_Id, ( int )codeSize );
		DisassembleShader( fp, ( DWORD * )data.PeekGet(), codeSize, pixelShader.m_Id, false );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_DestroyVertexOrPixelShader( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 1 )
	{
		int id;
		data.GetInt( id );
		fprintf( fp, "%s: id: %d\n", g_RecordingCommandNames[cmd], id );
	}
	else
	{
		Assert( 0 );
	}
}

static const char *D3DPrimitiveTypeToString( D3DPRIMITIVETYPE type )
{
	switch( type )
	{
	case D3DPT_POINTLIST:
		return "D3DPT_POINTLIST";
	case D3DPT_LINELIST:
		return "D3DPT_LINELIST";
	case D3DPT_LINESTRIP:
		return "D3DPT_LINESTRIP";
	case D3DPT_TRIANGLELIST:
		return "D3DPT_TRIANGLELIST";
	case D3DPT_TRIANGLESTRIP:
		return "D3DPT_TRIANGLESTRIP";
	case D3DPT_TRIANGLEFAN:
		return "D3DPT_TRIANGLEFAN";
	default:
		Assert( 0 );
		return "ERROR";
	}
}

void ListFunc_DrawIndexedPrimitive( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 6 )
	{
		D3DPRIMITIVETYPE mode;
		int firstVertex, numVertices;
		int firstIndex, numPrimitives;

	    cmdBuffer.GetInt( *(int*)&mode );
	    cmdBuffer.GetInt( firstVertex );
		int dx9FirstIndex;
		cmdBuffer.GetInt( dx9FirstIndex );
	    cmdBuffer.GetInt( numVertices );
	    cmdBuffer.GetInt( firstIndex );
		cmdBuffer.GetInt( numPrimitives );

		fprintf( fp, "%s: mode: %s firstVertex: %d dx9FirstIndex: %d numVertices: %d firstIndex: %d numPrimitives: %d\n", 
			g_RecordingCommandNames[cmd],
			D3DPrimitiveTypeToString( mode ), firstVertex, dx9FirstIndex, numVertices, firstIndex, numPrimitives );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_LockTexture( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( ( numargs == 5 ) || ( numargs == 6 ) )
	{
		TextureLookup_t texture;
		int lockFlags, copy, level, cubeFace;

	    data.GetInt( texture.m_BindId );
		data.GetInt( copy );
		data.GetInt( level );
		data.GetInt( cubeFace );
		if (numargs == 6)
			data.Get( &s_LockedSrcRect, sizeof(s_LockedSrcRect) );
	    data.GetInt( lockFlags );

		fprintf( fp, "%s: id: %d copy: %d level: %d cubeFace: %d ", g_RecordingCommandNames[cmd],
			texture.m_BindId, copy, level, cubeFace );
		if( numargs == 6 )
		{
			fprintf( fp, "lockedSrcRect: left %d right %d top %d bottom %d ",
				( int )s_LockedSrcRect.left, ( int )s_LockedSrcRect.right, 
				( int )s_LockedSrcRect.top, ( int )s_LockedSrcRect.bottom );
		}
		fprintf( fp, "lockFlags: %d\n", lockFlags );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_UnlockTexture( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 4 )
	{
		TextureLookup_t texture;
		int copy, level, cubeFace;

	    data.GetInt( texture.m_BindId );
		data.GetInt( copy );
		data.GetInt( level );
		data.GetInt( cubeFace );

		fprintf( fp, "%s: id: %d copy: %d level: %d cubeFace: %d\n", g_RecordingCommandNames[cmd],
			texture.m_BindId, copy, level, cubeFace );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetTextureData( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 3 )
	{
		int srcWidthInBytes, srcHeight;
		data.GetInt( srcWidthInBytes );
		data.GetInt( srcHeight );
		unsigned char *pSrc = ( unsigned char * )data.PeekGet();
		unsigned char *pDst = ( unsigned char * )s_LockedRect.pBits;

		fprintf( fp, "%s: srcWidthInBytes: %d srcHeight: %d\n", g_RecordingCommandNames[cmd], srcWidthInBytes, srcHeight );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetPixelShader( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 1 )
	{
		int pixelShader;
	    cmdBuffer.GetInt( pixelShader );
		fprintf( fp, "%s: pixelShader: %d\n", 
			g_RecordingCommandNames[cmd], pixelShader );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetVertexShader( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 1 )
	{
		int pixelShader;
	    cmdBuffer.GetInt( pixelShader );
		fprintf( fp, "%s: vertexShader: %d\n", 
			g_RecordingCommandNames[cmd], pixelShader );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetIndices( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 2 )
	{
		int indexBuffer;
		int baseVertexIndex;
	    cmdBuffer.GetInt( indexBuffer );
	    cmdBuffer.GetInt( baseVertexIndex );
		fprintf( fp, "%s: indexBuffer: %d baseVertexIndex: %d\n", 
			g_RecordingCommandNames[cmd], indexBuffer, baseVertexIndex );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_DebugString( FILE *fp, CUtlBuffer& cmdBuffer, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 1 )
	{
		int strMemSize = strlen( ( const char * )cmdBuffer.PeekGet() ) + 1;
		char *str = ( char * )_alloca( strMemSize );
		cmdBuffer.GetString( str );
		fprintf( fp, "%s: \"%s\"\n", 
			g_RecordingCommandNames[cmd], str );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetRenderTarget( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 2 )
	{
		int colorTexture, depthTexture;
		data.GetInt( colorTexture );
		data.GetInt( depthTexture );
		fprintf( fp, "%s: colorTexture: %d depthTexture: %d\n", 
			g_RecordingCommandNames[cmd], colorTexture, depthTexture );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_CreateDepthTexture(  FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 5 )
	{
		int bindID, width, height;
		D3DFORMAT format;
		D3DMULTISAMPLE_TYPE multisampleType;
		data.GetInt( bindID );
		data.GetInt( width );
		data.GetInt( height );
		data.GetInt( *( int * )&format );
		data.GetInt( *( int * )&multisampleType );

		fprintf( fp, "%s: bindID: %d width: %d height: %d format: %s multiSampleType: %d\n", 
			g_RecordingCommandNames[cmd], bindID, width, height, 
			D3DFormatToString( format ), ( int )multisampleType );
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetVertexBufferFormat( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 2 )
	{
		VertexFormat_t vertexFormat;
		VertexBufferFormatLookup_t lookup;
		data.GetInt( lookup.m_Id );
		data.GetInt( ( int & )vertexFormat );
		fprintf( fp, "%s: id: %d vertexFormat: 0x%x ",
			g_RecordingCommandNames[cmd],
			lookup.m_Id, vertexFormat );
		PrintVertexFormat( fp, vertexFormat );
		fprintf( fp, "\n" );
		int i = g_VertexBufferFormat.Find( lookup );
		if( i < 0 )
		{
			i = g_VertexBufferFormat.AddToTail( lookup );
		}
		g_VertexBufferFormat[i].m_VertexFormat = vertexFormat;
	}
	else
	{
		Assert( 0 );
	}
}

void ListFunc_SetClipPlane( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 5 )
	{
		int index;
		float plane[4];
		data.GetInt( index );
		data.GetFloat( plane[0] );
		data.GetFloat( plane[1] );
		data.GetFloat( plane[2] );
		data.GetFloat( plane[3] );
		fprintf( fp, "%s: index: %d plane: %f %f %f %f\n",
			g_RecordingCommandNames[cmd],
			index, plane[0], plane[1], plane[2], plane[3] );
	}
	else
	{
		Assert( 0 );
	}
}


void ListFunc_SyncToken( FILE *fp, CUtlBuffer& data, unsigned char cmd, unsigned char numargs )
{
	if( numargs == 1 )
	{
		int strMemSize = strlen( ( const char * )data.PeekGet() ) + 1;
		char *str = ( char * )_alloca( strMemSize );
		data.GetString( str );
		fprintf( fp, "%s: \"%s\"\n", 
			g_RecordingCommandNames[cmd], str );
	}
	else
	{
		Assert( 0 );
	}
}
 

static ListFunc_t *g_RecordingListFuncs[] = 
{
	ListFunc_CreateDevice, // Dx8CreateDevice
	ListFunc_Default, // Dx8DestroyDevice
	ListFunc_Default, // Dx8Reset
	ListFunc_Default, // Dx8ShowCursor
	ListFunc_Default, // Dx8BeginScene
	ListFunc_Default, // Dx8EndScene
	ListFunc_Present, // Dx8Present
	ListFunc_CreateTexture, // Dx8CreateTexture
	ListFunc_Default, // Dx8DestroyTexture
	ListFunc_SetTexture, // Dx8SetTexture
	ListFunc_Default, // Dx8SetTransform
	ListFunc_CreateVertexShader, // Dx8CreateVertexShader
	ListFunc_CreatePixelShader, // Dx8CreatePixelShader
	ListFunc_DestroyVertexOrPixelShader, // Dx8DestroyVertexShader
	ListFunc_DestroyVertexOrPixelShader, // Dx8DestroyPixelShader
	ListFunc_SetVertexShader, // Dx8SetVertexShader
	ListFunc_SetPixelShader, // Dx8SetPixelShader
	ListFunc_SetVertexPixelShaderConstant, // Dx8SetVertexShaderConstant
	ListFunc_SetVertexPixelShaderConstant, // Dx8SetPixelShaderConstant
	ListFunc_Default, // Dx8SetMaterial
	ListFunc_Default, // Dx8LightEnable
	ListFunc_Default, // Dx8SetLight
	ListFunc_SetViewport, // Dx8SetViewport
	ListFunc_Clear, // Dx8Clear
	ListFunc_Default, // Dx8ValidateDevice
	ListFunc_SetRenderState, // Dx8SetRenderState
	ListFunc_SetTextureStageState, // Dx8SetTextureStageState

	ListFunc_CreateVertexBuffer, // Dx8CreateVertexBuffer
	ListFunc_DestroyVertexIndexBuffer, // Dx8DestroyVertexBuffer
	ListFunc_LockVertexBuffer, // Dx8LockVertexBuffer
	ListFunc_VertexData, // Dx8VertexData
	ListFunc_Default, // Dx8UnlockVertexBuffer

	ListFunc_CreateIndexBuffer, // Dx8CreateIndexBuffer
	ListFunc_DestroyVertexIndexBuffer, // Dx8DestroyIndexBuffer
	ListFunc_LockIndexBuffer, // Dx8LockIndexBuffer
	ListFunc_IndexData, // Dx8IndexData
	ListFunc_UnlockIndexBuffer, // Dx8UnlockIndexBuffer

	ListFunc_SetStreamSource, // Dx8SetStreamSource
	ListFunc_SetIndices, // Dx8SetIndices
	ListFunc_Default, // Dx8DrawPrimitive
	ListFunc_DrawIndexedPrimitive, // Dx8DrawIndexedPrimitive

	ListFunc_LockTexture, // Dx8LockTexture
	ListFunc_UnlockTexture, // Dx8UnlockTexture

	ListFunc_Default, // Dx8Keyframe
	ListFunc_SetTextureData, // Dx8SetTextureData
	ListFunc_Default, // Dx8BlitTextureBits

	ListFunc_Default, // Dx8GetDeviceCaps
	ListFunc_Default, // Dx8GetAdapterIdentifier

	ListFunc_Default, // Dx8HardwareSync
	
	ListFunc_Default, // Dx8CopyRenderTargetToTexture
	ListFunc_DebugString, // Dx8DebugString
	ListFunc_CreateDepthTexture, // Dx8CreateDepthTexture
	ListFunc_Default, // Dx8DestroyDepthTexture
	ListFunc_SetRenderTarget, // Dx8SetRenderTarget
	ListFunc_Default, // Dx8TestCooperativeLevel
	ListFunc_SetVertexBufferFormat, // Dx8SetVertexBufferFormat
	ListFunc_Default, // Dx8SetSamplerState
	ListFunc_Default, // Dx8SetVertexDeclaration
	ListFunc_Default, // Dx8CreateVertexDeclaration
	ListFunc_Default, // Dx8SetFVF,
	ListFunc_SetClipPlane, // Dx8SetClipPlane
	ListFunc_SyncToken // DX8_SYNC_TOKEN
};



// Write all of the commands in a recording file to a listing file which is human readable.
void WriteListFile( const char *pListFileName, const char *pRecFileName, int startFrame, int endFrame )
{
	int curCmd = 0;
	int frameNum = 1;
	if ( OpenRecordingFile( pRecFileName ))
	{
		FILE *fp;
		if( fp = fopen( pListFileName, "w" ) )
		{
			CUtlBuffer buffer;
			while (ReadNextCommand( buffer ))
			{
				curCmd++;
				unsigned char cmd, numargs;
				buffer.SeekGet(0);
				buffer.GetUnsignedChar( cmd );
				buffer.GetUnsignedChar( numargs );
				if( cmd >= 0 && cmd < DX8_NUM_RECORDING_COMMANDS )
				{
					if( startFrame == -1 || endFrame == -1 ||
						( frameNum >= startFrame && frameNum <= endFrame ) )
					{
#ifdef ONLY_FIRST_CMD
						static unsigned char prevCmd = -1;
						if( ( frameNum > 1 ) && ( cmd == prevCmd ) && ( prevCmd == ONLY_FIRST_CMD_CMD ) )
						{
							continue;	
						}
						prevCmd = cmd;
#endif
#if defined SKIP_CMD && defined SKIP_CMD_CMD
						if( ( frameNum > 1 ) && ( cmd == SKIP_CMD_CMD ) )
						{
							continue;
						}
#endif
						fprintf( fp, "cmd: %d ", curCmd );
						(*g_RecordingListFuncs[cmd])( fp, buffer, cmd, numargs );
					}
					if( cmd == DX8_PRESENT )
					{
						frameNum++;
					}
				}
				else
				{
					Assert( 0 );
				}
			}
			CloseRecordingFile();
			fclose( fp );
		}
	}
}

// These indicate if we should be suppressing commands because
// we haven't hit the first keyframe yet...
static bool g_SuppressCommand[] = 
{
	false,	// Dx8CreateDevice,
	false,	// Dx8DestroyDevice,
	false,	// Dx8Reset,
	false,	// Dx8ShowCursor,
	false,	// Dx8BeginScene,
	false,	// Dx8EndScene,
	false,	// Dx8Present,
	false,	// Dx8CreateTexture,
	false,	// Dx8DestroyTexture,
	true,	// Dx8SetTexture,
	true,	// Dx8SetTransform,
	false,	// Dx8CreateVertexShader,
	false,	// Dx8CreatePixelShader,
	false,	// Dx8DestroyVertexShader,
	false,	// Dx8DestroyPixelShader,
	true,	// Dx8SetVertexShader,
	true,	// Dx8SetPixelShader,
	true,	// Dx8SetVertexShaderConstant,
	true,	// Dx8SetPixelShaderConstant,
	true,	// Dx8SetMaterial,
	true,	// Dx8LightEnable,
	true,	// Dx8SetLight,
	true,	// Dx8SetViewport,
	true,	// Dx8Clear,
	true,	// Dx8ValidateDevice,
	true,	// Dx8SetRenderState,
	true,	// Dx8SetTextureStageState,

	false,	// Dx8CreateVertexBuffer,
	false,	// Dx8DestroyVertexBuffer,
	false,	// Dx8LockVertexBuffer,
	false,	// Dx8VertexData,
	false,	// Dx8UnlockVertexBuffer,

	false,	// Dx8CreateIndexBuffer,
	false,	// Dx8DestroyIndexBuffer,
	false,	// Dx8LockIndexBuffer,
	false,	// Dx8IndexData,
	false,	// Dx8UnlockIndexBuffer,

	true,	// Dx8SetStreamSource,
	true,	// Dx8SetIndices,
	true,	// Dx8DrawPrimitive,
	true,	// Dx8DrawIndexedPrimitive,

	false,	// Dx8LockTexture,
	false,	// Dx8UnlockTexture,

	false,	// Dx8Keyframe,
	false,  // Dx8SetTextureData
	false,	// Dx8BlitTextureBits

	false,  // Dx8GetDeviceCaps
	false,	// Dx8GetAdapterIdentifier
	false,	// Dx8HardwareSync

	false,	// Dx8CopyRenderTargetToTexture
	false,  // Dx8DebugString,
	false,	// Dx8CreateDepthTexture,
	false,	// Dx8DestroyDepthTexture,
	false,	// Dx8SetRenderTarget,
	false,	// Dx8TestCooperativeLevel,
	false,	// Dx8SetVertexBufferFormat,
	false,	// Dx8SetSamplerState,
	false,	// Dx8SetVertexDeclaration,
	false,	// Dx8CreateVertexDeclaration,
	false,	// Dx8SetFVF,
	false,  // Dx8SetClipPlane
	false
};

//-----------------------------------------------------------------------------
// Error...
//-----------------------------------------------------------------------------

void DisplayError( char const* pError, ... )
{
	va_list		argptr;
	char		msg[1024];
	
	va_start( argptr, pError );
	vsprintf( msg, pError, argptr );
	va_end( argptr );

	MessageBox( 0, msg, 0, MB_OK );
}

//-----------------------------------------------------------------------------
// Command-line...
//-----------------------------------------------------------------------------

int FindCommand( const char *pCommand )
{
	for ( int i = 0; i < g_ArgC; i++ )
	{
		if ( !stricmp( pCommand, g_ppArgV[i] ) )
			return i;
	}
	return -1;
}

char const* CommandArgument( const char *pCommand )
{
	int cmd = FindCommand( pCommand );
	if ((cmd != -1) && (cmd < g_ArgC - 1))
	{
		return g_ppArgV[cmd+1];
	}
	return 0;
}

void SetupCommandLine( char const* pCommandLine )
{
	strcpy( g_pCommandLine, pCommandLine );

	g_ArgC = 0;

	char* pStr = g_pCommandLine;
	while ( *pStr )
	{
		pStr = strtok( pStr, " " );
		g_ppArgV[g_ArgC++] = pStr;
		pStr += strlen(pStr) + 1;
	}
}


//-----------------------------------------------------------------------------
// File read...
//-----------------------------------------------------------------------------

#define READ_CHUNK_SIZE (64 * 1024)

static bool g_BlockFileReadInitialized = false;

bool ReadNextFileData( void* pMem, int numBytes )
{
	// Batch up file reads to make it faster
	static unsigned char bufferMemory[READ_CHUNK_SIZE];
	static int readIndex;
	static int maxIndex;

	if (!g_BlockFileReadInitialized)
	{
		readIndex = maxIndex = READ_CHUNK_SIZE;
		g_BlockFileReadInitialized = true;
	}

	unsigned char* pMemory = (unsigned char*)pMem;

	// Don't read if we don't need to...
	int writeIndex = 0;
	while (readIndex + numBytes > maxIndex)
	{
		// Copy in the end of the buffer
		int bytesToCopy = maxIndex - readIndex;
		memcpy( &pMemory[writeIndex], &bufferMemory[readIndex], bytesToCopy );
		writeIndex += bytesToCopy;
		numBytes -= bytesToCopy;

		if ( maxIndex != READ_CHUNK_SIZE )
			return false;

		// Read the next chunk
		int sizeread = fread( bufferMemory, 1, READ_CHUNK_SIZE, g_pRecordingFile );
		maxIndex = sizeread;
		readIndex = 0;
	}
	
	memcpy( &pMemory[writeIndex], &bufferMemory[readIndex], numBytes );
	readIndex += numBytes;

	return true;
}

bool ReadNextCommand( CUtlBuffer& buffer )
{
	static CUtlVector< unsigned char > bufferMemoryVec;

	// read the size of the next command
	int size;
	if (!ReadNextFileData( &size, sizeof(int) ))
		return false;

	size -= sizeof(int);
	if (bufferMemoryVec.Size() < size)
		bufferMemoryVec.AddMultipleToTail( size - bufferMemoryVec.Size() );

	// Read the next command
	if (!ReadNextFileData( &bufferMemoryVec[0], size ))
		return false;

	buffer.Attach( &bufferMemoryVec[0], size );
	return true;
}

//-----------------------------------------------------------------------------
// Perform command...
//-----------------------------------------------------------------------------

void PerformCommand( CUtlBuffer& buffer )
{
	g_CurrentCmd++;
	unsigned char cmd, numargs;
	buffer.GetUnsignedChar( cmd );
//	OutputDebugString( g_RecordingCommandNames[cmd] );
//	OutputDebugString( "\n" );
	buffer.GetUnsignedChar( numargs );

	// Blow off this command if we're currently suppressing them
	if (IsSuppressingCommands() && g_SuppressCommand[cmd])
		return;

#ifdef ONLY_FIRST_CMD
	static unsigned char prevCmd = -1;
	if( ( g_FrameNum > 1 ) && ( cmd == prevCmd ) && ( prevCmd == ONLY_FIRST_CMD_CMD ) )
	{
		return;
	}
	prevCmd = cmd;
#endif
#if defined SKIP_CMD && defined SKIP_CMD_CMD
	if( ( g_FrameNum > 1 ) && ( cmd == SKIP_CMD_CMD ) )
	{
		return;
	}
#endif
	
	Assert( cmd >= 0 && cmd < DX8_NUM_RECORDING_COMMANDS );
	g_RecordingCommand[cmd]( numargs, buffer );
}

//-----------------------------------------------------------------------------
// Opens the output file
//-----------------------------------------------------------------------------

static FILE* OpenOutputFile( char const* pFileName )
{
	FILE* fp = 0;
	static bool g_CantOpenFile = false;
	static bool g_NeverOpened = true;
	if (!g_CantOpenFile)
	{
		fp = fopen( pFileName, g_NeverOpened ? "wb" : "ab" );
		if (!fp)
		{
			DisplayError("Unable to open output file \"%s\"!\n", pFileName);
			g_CantOpenFile = true;			
		}
		g_NeverOpened = false;
	}
	return fp;
}

//-----------------------------------------------------------------------------
// Write command...
//-----------------------------------------------------------------------------

#define OUTPUT_BUFFER_SIZE 32768

void WriteOutputFile( CUtlBuffer* pBuffer, bool force )
{
	static CUtlVector<unsigned char> recordingBuffer;

	// Append to output buffer
	if (pBuffer)
	{
		int i = recordingBuffer.AddMultipleToTail( pBuffer->Size() + sizeof(int) );
		*(int*)&recordingBuffer[i] = pBuffer->Size() + sizeof(int);
		memcpy( &recordingBuffer[i+4], pBuffer->Base(), pBuffer->Size() );
	}

	// When not crash recording, flush when buffer gets too big, 
	// or when Present() is called
	if ((recordingBuffer.Size() < OUTPUT_BUFFER_SIZE) && (!g_CrashWrite) && (!force))
		return;

	FILE* fp = OpenOutputFile( g_pOutputFile );
	if (fp)
	{
		// store the command size
		fwrite( recordingBuffer.Base(), 1, recordingBuffer.Size(), fp );
		fflush( fp );
		fclose( fp );
	}

	recordingBuffer.RemoveAll();
}

void WriteCommand( CUtlBuffer* pBuffer, bool force )
{
	if (!g_pOutputFile)
		return;

	// Suppress various types of data here
	if (pBuffer)
	{
		unsigned char cmd, numargs;
		pBuffer->SeekGet(0);
		pBuffer->GetUnsignedChar( cmd );
		pBuffer->GetUnsignedChar( numargs );

		// Blow off this command if we're currently suppressing them
		if (IsSuppressingCommands() && g_SuppressCommand[cmd])
			return;

		switch(cmd)
		{
		case DX8_CREATE_VERTEX_BUFFER:
			{
				VertexBufferLookup_t vertexBuffer;
				UINT length;
				DWORD usage, fvf;
				D3DPOOL pool;
				int dynamic;

				pBuffer->GetInt( vertexBuffer.m_Id );
				pBuffer->GetInt( *(int*)&length );
				pBuffer->GetInt( *(int*)&usage );
				pBuffer->GetInt( *(int*)&fvf );
				pBuffer->GetInt( *(int*)&pool );
				pBuffer->GetInt( dynamic );

				int i = g_VertexBuffer.Find( vertexBuffer );
				if (i < 0)
					i = g_VertexBuffer.Insert( vertexBuffer );

				g_VertexBuffer[i].m_Dynamic = dynamic;
				g_VertexBuffer[i].m_pLockedMemory = 0;
				g_VertexBuffer[i].m_pVB = 0;

				if (g_SuppressOutputDynamicMesh && dynamic)
					return;

				if ( g_UsedVertexBuffers.Find( vertexBuffer.m_Id ) < 0)
					return;
			}
			break;

		case DX8_DESTROY_VERTEX_BUFFER:
			{
				VertexBufferLookup_t vertexBuffer;
				pBuffer->GetInt( vertexBuffer.m_Id );
				bool dynamic = false;

				int i = g_VertexBuffer.Find( vertexBuffer );
				if (i >= 0)
				{
					dynamic = g_VertexBuffer[i].m_Dynamic != 0;
					g_VertexBuffer.RemoveAt( i );
				}

				if (g_SuppressOutputDynamicMesh && dynamic)
					return;

				if ( g_UsedVertexBuffers.Find( vertexBuffer.m_Id ) < 0)
					return;
			}
			break;

		case DX8_LOCK_VERTEX_BUFFER:
		case DX8_VERTEX_DATA:
		case DX8_UNLOCK_VERTEX_BUFFER:
			{
				VertexBufferLookup_t vertexBuffer;
				pBuffer->GetInt( vertexBuffer.m_Id );

				int i = g_VertexBuffer.Find( vertexBuffer );
				if (i >= 0)
				{
					if (g_SuppressOutputDynamicMesh)
					{
						if (g_VertexBuffer[i].m_Dynamic)
							return;
					}

					// Blow off this command if we're currently suppressing them
					if (IsSuppressingCommands() && g_VertexBuffer[i].m_Dynamic)
						return;
				}

				if ( g_UsedVertexBuffers.Find( vertexBuffer.m_Id ) < 0)
					return;
			}
			break;

		case DX8_CREATE_INDEX_BUFFER:
			{
				IndexBufferLookup_t indexBuffer;
				UINT length;
				DWORD usage;
				D3DFORMAT fmt;
				D3DPOOL pool;
				int dynamic;

				pBuffer->GetInt( indexBuffer.m_Id );
				pBuffer->GetInt( *(int*)&length );
				pBuffer->GetInt( *(int*)&usage );
				pBuffer->GetInt( *(int*)&fmt );
				pBuffer->GetInt( *(int*)&pool );
				pBuffer->GetInt( dynamic );

				int i = g_IndexBuffer.Find( indexBuffer );
				if (i < 0)
					i = g_IndexBuffer.Insert( indexBuffer );

				g_IndexBuffer[i].m_pIB = 0;
				g_IndexBuffer[i].m_pLockedMemory = 0;
				g_IndexBuffer[i].m_Dynamic = dynamic;

				if (g_SuppressOutputDynamicMesh && dynamic)
					return;

				if ( g_UsedIndexBuffers.Find( indexBuffer.m_Id ) < 0)
					return;
			}
			break;

		case DX8_DESTROY_INDEX_BUFFER:
			{
				IndexBufferLookup_t indexBuffer;
				pBuffer->GetInt( indexBuffer.m_Id );
				bool dynamic = false;

				int i = g_IndexBuffer.Find( indexBuffer );
				if (i >= 0)
				{
					dynamic = g_IndexBuffer[i].m_Dynamic != 0;
					g_IndexBuffer.RemoveAt( i );
				}

				if (g_SuppressOutputDynamicMesh && dynamic)
					return;

				if ( g_UsedIndexBuffers.Find( indexBuffer.m_Id ) < 0)
					return;
			}
			break;

		case DX8_LOCK_INDEX_BUFFER:
		case DX8_INDEX_DATA:
		case DX8_UNLOCK_INDEX_BUFFER:
			{
				IndexBufferLookup_t indexBuffer;
				pBuffer->GetInt( indexBuffer.m_Id );

				int i = g_IndexBuffer.Find( indexBuffer );
				if (i >= 0)
				{
					if (g_SuppressOutputDynamicMesh)
					{
						if (g_IndexBuffer[i].m_Dynamic)
							return;
					}

					// Blow off this command if we're currently suppressing them
					if (IsSuppressingCommands() && g_IndexBuffer[i].m_Dynamic)
						return;
				}

				if ( g_UsedIndexBuffers.Find( indexBuffer.m_Id ) < 0)
					return;
			}
			break;

		case DX8_KEYFRAME:
			if (g_KeyFrame >= 0)
			{
				--g_KeyFrame;
				if (g_KeyFrame < 0)
					g_StopNextFrame = true;
			}
			break;

		case DX8_PRESENT:
			// Stop after the keyframe
			if (g_StopNextFrame)
				g_Exiting = true;
			break;
		}
	}

	// If it got this far, we're writing it...
	WriteOutputFile( pBuffer, force );
}


//-----------------------------------------------------------------------------
// file open, close...
//-----------------------------------------------------------------------------

bool OpenRecordingFile( char const* pFileName )
{
	if (!pFileName)
	{
		DisplayError( "Recording file not specified, use -rec <filename>\n" );
		return false;
	}

	g_pRecordingFile = fopen( pFileName, "rb" );
	if (!g_pRecordingFile)
	{
		DisplayError( "Unable to open recording file \"%s\"\n", pFileName );
		return false;
	}

	g_BlockFileReadInitialized = false;

	return (g_pRecordingFile != 0);
}

void CloseRecordingFile()
{
	if (g_pRecordingFile)
		fclose(g_pRecordingFile); 
}


//-----------------------------------------------------------------------------
// Determine if we've got unused static index + vertex buffers
//-----------------------------------------------------------------------------

void DetermineUsedStaticMeshes( char const* pFileName )
{
	if ( OpenRecordingFile( pFileName ))
	{
		CUtlBuffer buffer;
		while (ReadNextCommand( buffer ))
		{
			unsigned char cmd, numargs;
			buffer.GetUnsignedChar( cmd );
			buffer.GetUnsignedChar( numargs );

			if (cmd == DX8_SET_STREAM_SOURCE)
			{
				int id;
				buffer.GetInt( id );
				if (g_UsedVertexBuffers.Find(id) < 0)
					g_UsedVertexBuffers.AddToTail( id );
			}
			else if (cmd == DX8_SET_INDICES)
			{
				int id;
				buffer.GetInt( id );
				if (g_UsedIndexBuffers.Find(id) < 0)
					g_UsedIndexBuffers.AddToTail( id );
			}
		}

		CloseRecordingFile();
	}
}


//-----------------------------------------------------------------------------
// Window create, destroy...
//-----------------------------------------------------------------------------

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
		// abort when ESC is hit
		case WM_CHAR:
			switch(wParam)
			{
			case VK_ESCAPE:
				SendMessage( g_HWnd, WM_CLOSE, 0, 0 );
				break;
			case VK_SPACE:
				if (g_InteractiveMode)
					g_Paused = false;
				else
					g_Paused = !g_Paused;
				break;
			case 'I':
			case 'i':
				g_InteractiveMode = (!g_InteractiveMode);
				if (!g_InteractiveMode)
					g_Paused = false;
				break;
			}
		break;

        case WM_DESTROY:
			g_Exiting = true;
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}

bool CreateAppWindow( char const* pAppName, int width, int height )
{
    // Register the window class
	WNDCLASSEX windowClass = 
	{ 
		sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        pAppName, NULL 
	};

    RegisterClassEx( &windowClass );

    // Create the application's window
    g_HWnd = CreateWindow( pAppName, pAppName,
		WS_OVERLAPPEDWINDOW, 0, 0, width, height,
		GetDesktopWindow(), NULL, windowClass.hInstance, NULL );
	
	return (g_HWnd != 0);
}

void DestroyAppWindow()
{
	if (g_HWnd)
		DestroyWindow( g_HWnd );
}

//-----------------------------------------------------------------------------
// Create, destroy DX8...
//-----------------------------------------------------------------------------

bool CreateDx8()
{
	g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    return (g_pD3D != 0);
}

void DestroyDx8()
{
	g_pD3D->Release();
}


//-----------------------------------------------------------------------------
// Cleanup temporary structures
//-----------------------------------------------------------------------------

void CleanUpTemporaryStructures()
{
	g_Texture.RemoveAll();
	g_VertexBuffer.RemoveAll();
	g_IndexBuffer.RemoveAll();

}

//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------

static SpewRetval_t MySpewOutputFunc( SpewType_t spewType, char const *pMsg )
{
	OutputDebugString( pMsg );
	switch( spewType )
	{
	case SPEW_MESSAGE:
	case SPEW_WARNING:
	case SPEW_LOG:
		return SPEW_CONTINUE;
	case SPEW_ASSERT:
	case SPEW_ERROR:
	default:
		return SPEW_DEBUGGER;
	}
}

bool Init( char const* pCommands )
{
	SpewOutputFunc( MySpewOutputFunc );
	MathLib_Init(  2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false );
	InitClock();

	// Store off the command line...
	SetupCommandLine( pCommands );

	if (CommandArgument("-i") == 0)
	{
		DisplayError("playback usage: playback -i <input> [-window|-fullscreen] [-o <output> -nodynamic -crash] -keyframe <keyframe> -white -softwaretl -hardwaretl -pure -ref -noref -framedump -framedumpstart framenum -framedumpend framenum -list list.txt -liststart framenum -listend framenum -nohwsync -forcestaticibprimcount N");
		return false;
	}

	if( FindCommand( "-nohwsync" ) != -1 )
	{
		g_bAllowHWSync = false;
	}
	
	// Write all of the commands ot a file if necessary
	if( CommandArgument( "-list" ) )
	{
		int startFrame = -1;
		int endFrame = -1;
		if( FindCommand( "-liststart" ) >= 0 )
		{
			startFrame = atoi( CommandArgument( "-liststart" ) );
		}
		if( FindCommand( "-listend" ) >= 0 )
		{
			endFrame = atoi( CommandArgument( "-listend" ) );
		}
		WriteListFile( CommandArgument( "-list" ), CommandArgument("-i"), startFrame, endFrame );
	}
	
	// Check if there's an output file
	g_pOutputFile = CommandArgument("-o");

	// Determine if we've got unused static index + vertex buffers
	if (g_pOutputFile)
		DetermineUsedStaticMeshes(CommandArgument("-i"));

	// Do we write out dynamic meshes?
	g_SuppressOutputDynamicMesh = (FindCommand("-nodynamic") >= 0);

	// Do we need to flush after every write?
	g_CrashWrite = (FindCommand("-crash") >= 0);

	// Explicitly in window/fullscreen mode?
	if (FindCommand("-window") >= 0)
		g_WindowMode = 1;
	else if (FindCommand("-fullscreen") >= 0)
		g_WindowMode = 0;

	g_WhiteTextures = (FindCommand("-white") >= 0);
	if (FindCommand("-softwaretl") >= 0)
		g_VertexProcessing = 1;
	else if (FindCommand("-hardwaretl") >= 0)
		g_VertexProcessing = 2;
	else if (FindCommand("-pure") >= 0)
		g_VertexProcessing = 3;

	if( FindCommand( "-framedump" ) >= 0 )
	{
		g_FrameDump = true;
		if( FindCommand( "-framedumpstart" ) >= 0 )
		{
			g_FrameDumpStart = atoi( CommandArgument( "-framedumpstart" ) );
		}
		if( FindCommand( "-framedumpend" ) >= 0 )
		{
			g_FrameDumpEnd = atoi( CommandArgument( "-framedumpend" ) );
		}
	}
	
	if( FindCommand( "-ref" ) >= 0 )
	{
		g_ForceRefRast = true;
	}

	if( FindCommand( "-noref" ) >= 0 )
	{
		g_ForceNoRefRast = true;
	}

	if( CommandArgument( "-forcestaticibprimcount" ) )
	{
		g_ForcedStaticIndexBufferPrims = atoi( CommandArgument( "-forcestaticibprimcount" ) );
		g_ForcedStaticIndexBufferBytes = 6 * g_ForcedStaticIndexBufferPrims;
	}
	
	// Should we skip up to a particular keyframe?
	if (CommandArgument("-keyframe"))
	{
		char* pTemp;
		g_KeyFrame = strtol(CommandArgument("-keyframe"), &pTemp, 10 ); 
	}
	else
	{
		g_KeyFrame = -1;
	}

	// Open the file...
	if (!OpenRecordingFile(CommandArgument("-i")))
		return false;

	// Create the window
	if (!CreateAppWindow( pCommands, 640, 480 ))
		return false;

	// Create DX8
	if (!CreateDx8())
		return false;

	return true;
}

volatile unsigned char g_VolatileUChar;

void TouchMemory( unsigned char *data, int size )
{
	int j;
	for( j = 0; j < 5; j++ )
	{
		int i;
		for( i = 0; i < size; i++ )
		{
			g_VolatileUChar = data[i];
		}
	}
}

void ProcessFrame( unsigned char *data, int size )
{	
	TouchMemory( data, size );
	
#if 0
	bool doVTune = g_FrameNum > 5 && g_FrameNum < 240;
	if( doVTune )
	{
		VTResume();
	}
#endif
	double t1 = GetTime();
	CUtlBuffer buffer;
	int cmdSize;
	unsigned char *scan = data;
	unsigned char *scanEnd = data + size;
	while( scan < scanEnd )
	{
		cmdSize = *( unsigned int * )scan;
		scan += sizeof( int );
		Assert( *( unsigned char * )scan < DX8_NUM_RECORDING_COMMANDS );
		buffer.Attach( scan, cmdSize );
		PerformCommand( buffer );
		scan += cmdSize;
	}
	double t2 = GetTime();
	double dt = t2 - t1;
	g_FrameTime.AddToTail( dt );
	CUtlBuffer dummyBuffer;
	Dx8HardwareSync( 0, dummyBuffer );
	double t3 = GetTime();
	double dt2 = t3 - t2;
	g_HardwareFlushTime.AddToTail( dt2 );
#if 0
	if( doVTune )
	{
		VTPause();
	}
#endif
}


//-----------------------------------------------------------------------------
// Update (play next recorded command)
//-----------------------------------------------------------------------------

bool Update()
{
	if (!g_Paused)
	{
#if 0
		// This version doesn't buffer a frame at a time.
		CUtlBuffer buffer;
		if (!ReadNextCommand( buffer ))
			return true;

		WriteCommand( &buffer, false );

		if (!g_pOutputFile)
		{
			PerformCommand( buffer );
		}
#else
		static CUtlVector<unsigned char> m_FrameData;
		int frameOffset = 0;
		
		bool doneWithFrame = false;
		bool doneWithEverything = false;

		// Process a frame at a time.
		while( !doneWithFrame )
		{
			CUtlBuffer buffer;
			if (ReadNextCommand( buffer ))
			{
				// append to the frame data.
				unsigned char cmd;
				buffer.GetUnsignedChar( cmd );
				Assert( cmd < DX8_NUM_RECORDING_COMMANDS );
				if( cmd == DX8_PRESENT )
				{
					doneWithFrame = true;
				}
				buffer.SeekGet( 0 );
				int requiredSize = frameOffset + buffer.Size() + sizeof( int );
				if( requiredSize > m_FrameData.Count() )
				{
					m_FrameData.AddMultipleToTail( requiredSize - m_FrameData.Count() );
				}
				*( int * )&m_FrameData[frameOffset] = buffer.Size();
				frameOffset += sizeof( int );
				memcpy( &m_FrameData[frameOffset], buffer.Base(), buffer.Size() );
				Assert( m_FrameData[frameOffset] < DX8_NUM_RECORDING_COMMANDS );
				frameOffset += buffer.Size();
			}
			else
			{
				doneWithFrame = true;
				doneWithEverything = true;
			}
		}
		ProcessFrame( m_FrameData.Base(), frameOffset );
		if( doneWithEverything )
		{
			return doneWithEverything;
		}
#endif
	}
	else
	{
		PumpWindowsMessages();
	}

	return g_Exiting;
}

//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------

void Shutdown()
{
	// Flush the output file
	WriteCommand( 0, true );

	// Cleanup temporary structures
	CleanUpTemporaryStructures();

	// Shutdown DX8
	DestroyDx8();

	// Close the window
	DestroyAppWindow();

	// Done with the file
	CloseRecordingFile();
}
			  
//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR pCommands, INT )
{
	COMPILE_TIME_ASSERT( sizeof( g_RecordingCommand ) / sizeof( g_RecordingCommand[0] ) ==
		DX8_NUM_RECORDING_COMMANDS );
	COMPILE_TIME_ASSERT( sizeof( g_RecordingCommandNames ) / sizeof( g_RecordingCommandNames[0] ) ==
		DX8_NUM_RECORDING_COMMANDS );
	COMPILE_TIME_ASSERT( sizeof( g_RecordingListFuncs ) / sizeof( g_RecordingListFuncs[0] ) ==
		DX8_NUM_RECORDING_COMMANDS );
	COMPILE_TIME_ASSERT( sizeof( g_SuppressCommand ) / sizeof( g_SuppressCommand[0] ) ==
		DX8_NUM_RECORDING_COMMANDS );
	// Start up
	if (!Init( pCommands ))
		return false;

	bool done = false;
	while (!done)
	{
		done = Update();
	}

	FILE *fp;
	fp = fopen( "times.txt", "w" );
	if( fp )
	{
		Assert( g_FrameTime.Count() == g_HardwareFlushTime.Count() );
		int i;
		for( i = 0; i < g_FrameTime.Count(); i++ )
		{
			fprintf( fp, "%lf, %lf\n", ( double )( 1000.0 * g_FrameTime[i] ),
				( double )( 1000.0 * g_HardwareFlushTime[i] ) );
		}
		fclose( fp );
	}
	
	Shutdown();

#ifdef STUBD3D
	if( g_pD3DDevice )
	{
		delete ( CStubD3DDevice * )g_pD3DDevice;
		g_pD3DDevice = NULL;
	}
#endif
    return 0;
}

