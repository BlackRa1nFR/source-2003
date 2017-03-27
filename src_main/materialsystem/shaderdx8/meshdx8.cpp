//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// The dx8 implementation of the shader API
//=============================================================================

#include "locald3dtypes.h"
#include "IMeshDX8.h"
#include "ShaderAPIDX8_Global.h"
#include "tier0/vprof.h"

// fixme - stick this in a header file.
#ifdef _DEBUG
// define this if you want to range check all indices when drawing
#define CHECK_INDICES
#endif

#ifdef CHECK_INDICES
#define CHECK_INDICES_MAX_NUM_STREAMS 2
#endif

#include "DynamicIB.h"
#include "DynamicVB.h"
#include "UtlVector.h"
#include "ShaderAPI.h"
#include "IMaterialInternal.h"
#include "ShaderAPIDX8.h"
#include "IShaderUtil.h"
#include "CMaterialSystemStats.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "materialsystem/MaterialSystem_Config.h"
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Uncomment this to test buffered state
//-----------------------------------------------------------------------------

//#define DEBUG_BUFFERED_MESHES 1
//#define DRAW_SELECTION 1

//-----------------------------------------------------------------------------
// Important enumerations
//-----------------------------------------------------------------------------
enum
{
	VERTEX_BUFFER_SIZE = 32768,
	INDEX_BUFFER_SIZE = 32768,
};


//-----------------------------------------------------------------------------
// Standard vertex formats (must be sorted from smallest to largest)
//-----------------------------------------------------------------------------
static VertexFormat_t s_pStandardFormats[] =
{
	// 32 byte formats

	// position, color, two tex coordinates (32 bytes)
	VERTEX_POSITION | VERTEX_COLOR | 
			VERTEX_NUM_TEXCOORDS(2) |
			VERTEX_TEXCOORD_SIZE(0, 2) | VERTEX_TEXCOORD_SIZE(1, 2),

	// position, normal, one tex coordinate	(32 bytes)
	VERTEX_POSITION | VERTEX_NORMAL | 
			VERTEX_NUM_TEXCOORDS(1) | 
			VERTEX_TEXCOORD_SIZE(0, 2),

	// 48 byte formats

	// position, normal, color, specular, two tex coordinates (48 bytes)
	VERTEX_POSITION | VERTEX_COLOR | VERTEX_SPECULAR | VERTEX_NORMAL |
			VERTEX_NUM_TEXCOORDS(2) | 
			VERTEX_TEXCOORD_SIZE(0, 2) | VERTEX_TEXCOORD_SIZE(1, 2),

	// position, 2 bones, normal, color, one tex coord (48 bytes)
	VERTEX_POSITION | VERTEX_COLOR | VERTEX_NORMAL |
			VERTEX_BONEWEIGHT(2) | VERTEX_BONE_INDEX | 
			VERTEX_NUM_TEXCOORDS(1) | VERTEX_TEXCOORD_SIZE(0, 2),

	// 64 byte formats

	// position, normal, color, specular, four tex coordinates (64 bytes)
	VERTEX_POSITION | VERTEX_COLOR | VERTEX_SPECULAR | VERTEX_NORMAL |
			VERTEX_NUM_TEXCOORDS(4) | 
			VERTEX_TEXCOORD_SIZE(0, 2) | VERTEX_TEXCOORD_SIZE(1, 2) | 
			VERTEX_TEXCOORD_SIZE(2, 2) | VERTEX_TEXCOORD_SIZE(3, 2),

	// position, 1 bone, normal, color, specular, four tex coords (64 bytes)
	VERTEX_POSITION | VERTEX_COLOR | VERTEX_NORMAL |
			VERTEX_BONEWEIGHT(1) | 
			VERTEX_NUM_TEXCOORDS(4) | 
			VERTEX_TEXCOORD_SIZE(0, 2) | VERTEX_TEXCOORD_SIZE(1, 2) | 
			VERTEX_TEXCOORD_SIZE(2, 2) | VERTEX_TEXCOORD_SIZE(3, 2),

	// position, normal, color, four tex coordinates (64 bytes), 3d tex coord in stage 2
	VERTEX_POSITION | VERTEX_COLOR | VERTEX_NORMAL |
			VERTEX_NUM_TEXCOORDS(4) | 
			VERTEX_TEXCOORD_SIZE(0, 2) | VERTEX_TEXCOORD_SIZE(1, 2) | 
			VERTEX_TEXCOORD_SIZE(2, 3) | VERTEX_TEXCOORD_SIZE(3, 2),

	// position, color, tangentS, tangentT, three tex coordinates (64 bytes)
	VERTEX_POSITION | VERTEX_COLOR | VERTEX_TANGENT_S | VERTEX_TANGENT_T |  
			VERTEX_NUM_TEXCOORDS(3) | 
			VERTEX_TEXCOORD_SIZE(0, 2) | VERTEX_TEXCOORD_SIZE(1, 2) | 
			VERTEX_TEXCOORD_SIZE(2, 2),

	// 80 byte formats

	// Position, normal, color, specular, tangentS, tangentT, 3 2D tex coordinates (80 bytes)
	VERTEX_POSITION | VERTEX_NORMAL | VERTEX_COLOR | VERTEX_SPECULAR | VERTEX_TANGENT_S | VERTEX_TANGENT_T | 
			VERTEX_NUM_TEXCOORDS(3) | 
			VERTEX_TEXCOORD_SIZE(0, 2) | VERTEX_TEXCOORD_SIZE(1, 2) | 
			VERTEX_TEXCOORD_SIZE(2, 2),

	// Position, normal, tangentS, tangentT, 4 2D tex coordinates (80 bytes)
	VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T | 
			VERTEX_NUM_TEXCOORDS(4) | 
			VERTEX_TEXCOORD_SIZE(0, 2) | VERTEX_TEXCOORD_SIZE(1, 2) | 
			VERTEX_TEXCOORD_SIZE(2, 2) | VERTEX_TEXCOORD_SIZE(3, 2),

	// This must be last
	0
};


//-----------------------------------------------------------------------------
// Shared mesh methods
//-----------------------------------------------------------------------------
class CBaseMeshDX8 : public IMeshDX8
{
public:
	// constructor, destructor
	CBaseMeshDX8();
	virtual ~CBaseMeshDX8();

	// Locks mesh for modifying
	void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc );
	void ModifyEnd();

	// Sets/gets the vertex format
	virtual void SetVertexFormat( VertexFormat_t format );
	virtual VertexFormat_t GetVertexFormat() const;

	// Sets the material
	virtual void SetMaterial( IMaterial* pMaterial );

	// Uses pre-defined index buffers
	void GenerateSequentialIndexBuffer( unsigned short* pIndexBuffer, int numIndices, int firstVertex );
	void GenerateQuadIndexBuffer( unsigned short* pIndexBuffer, int numIndices, int firstVertex );
	void GeneratePolygonIndexBuffer( unsigned short* pIndexBuffer, int numIndices, int firstVertex );
	void GenerateLineStripIndexBuffer( unsigned short* pIndexMemory, int numIndices, int firstVertex );
	void GenerateLineLoopIndexBuffer( unsigned short* pIndexMemory, int numIndices, int firstVertex );

	// returns the # of vertices (static meshes only)
	int NumVertices() const { return 0; }

	void SetColorMesh( IMesh *pColorMesh )
	{
		Assert( 0 );
	}

	bool HasColorMesh( ) const { return false; }
	
	// Draws the mesh
	void DrawMesh( );

	// Begins a pass
	void BeginPass( );

	// Spews the mesh data
	void Spew( int numVerts, int numIndices, MeshDesc_t const& desc );

	// Call this in debug mode to make sure our data is good.
	virtual void ValidateData( int numVerts, int numIndices, MeshDesc_t const& desc );

	void Draw( CPrimList *pLists, int nLists );

	// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
	virtual void CopyToMeshBuilder( 
		int iStartVert,		// Which vertices to copy.
		int nVerts, 
		int iStartIndex,	// Which indices to copy.
		int nIndices, 
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder );

	// returns the primitive type
	virtual MaterialPrimitiveType_t GetPrimitiveType() const = 0;

	// Returns the number of indices in a mesh..
	virtual int NumIndices( ) const = 0;

	// returns a static vertex buffer...
	virtual CVertexBuffer*	GetVertexBuffer() { return 0; }
	virtual CIndexBuffer*	GetIndexBuffer() { return 0; }

	// Do I need to reset the vertex format?
	virtual bool NeedsVertexFormatReset( VertexFormat_t fmt ) const;

	// Do I have enough room?
	virtual bool HasEnoughRoom( int numVerts, int numIndices ) const;

	// Operation to do pre-lock
	virtual void PreLock() {}

	// Sets the software vertex shader
	virtual void SetSoftwareVertexShader( SoftwareVertexShader_t shader ) = 0;

protected:
	bool DebugTrace() const;
	
	// The vertex format we're using...
	VertexFormat_t m_VertexFormat;

#ifdef _DEBUG
	IMaterialInternal* m_pMaterial;
	bool m_IsDrawing;
#endif
};


//-----------------------------------------------------------------------------
// Implementation of the mesh
//-----------------------------------------------------------------------------
class CMeshDX8 : public CBaseMeshDX8
{
public:
	// constructor
	CMeshDX8( );
	virtual ~CMeshDX8();

	// Locks/unlocks the mesh
	void LockMesh( int numVerts, int numIndices, MeshDesc_t& desc );
	void UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc );

	// Locks mesh for modifying
	void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc );
	void ModifyEnd();

	// returns the # of vertices (static meshes only)
	int NumVertices() const;

	// returns the # of indices 
	int NumIndices( ) const;

	// Sets up the vertex and index buffers
	void UseIndexBuffer( CIndexBuffer* pBuffer );
	void UseVertexBuffer( CVertexBuffer* pBuffer );

	// returns a static vertex buffer...
	CVertexBuffer*	GetVertexBuffer() { return m_pVertexBuffer; }
	CIndexBuffer*	GetIndexBuffer() { return m_pIndexBuffer; }

	void SetColorMesh( IMesh *pColorMesh );
	bool HasColorMesh( ) const;
	
	// Draws the mesh
	void Draw( int firstIndex, int numIndices );
	void Draw( CPrimList *pLists, int nLists );

	// Draws a single pass
	void RenderPass();

	// Sets the primitive type
	void SetPrimitiveType( MaterialPrimitiveType_t type );
	MaterialPrimitiveType_t GetPrimitiveType() const;

	bool IsStatic() { return true; };

	void SetSoftwareVertexShader( SoftwareVertexShader_t shader );
	void CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder );

protected:
	// Sets the render state.
	bool SetRenderState( int firstVertexIdx = 0 );

	// Is the vertex format valid?
	bool IsValidVertexFormat();

	// Locks/ unlocks the vertex buffer
	void LockVertexBuffer( int numVerts, MeshDesc_t& desc );
	void UnlockVertexBuffer( int numVerts );

	// Locks/unlocks the index buffer
	// Pass in firstIndex=-1 to lock wherever the index buffer is. Pass in a value 
	// >= 0 to specify where to lock.
	int  LockIndexBuffer( int firstIndex, int numIndices, MeshDesc_t& pIndices );
	void UnlockIndexBuffer( int numIndices );

	// Computes the primitive mode
	D3DPRIMITIVETYPE ComputeMode( MaterialPrimitiveType_t mode );

	// computes how many primitives we've got
	int NumPrimitives( int numVerts, int numIndices ) const;

	// Debugging output...
	void SpewMaterialVerts( );

	// The vertex and index buffers
	CVertexBuffer* m_pVertexBuffer;
	CIndexBuffer* m_pIndexBuffer;

	CMeshDX8 *m_pColorMesh;
	
	// Primitive type
	MaterialPrimitiveType_t m_Type;

	// Primitive mode
	D3DPRIMITIVETYPE m_Mode;

	// Number of primitives
	unsigned short m_NumVertices;
	unsigned short m_NumIndices;

	// Is it locked?
	bool	m_IsVBLocked;
	bool	m_IsIBLocked;

	// Used in rendering sub-parts of the mesh
	static CPrimList	*s_pPrims;
	static int			s_nPrims;
	static unsigned int s_FirstVertex;
	static unsigned int s_NumVertices;
	int			m_FirstIndex;

#ifdef RECORDING
	int		m_LockVertexBufferSize;
	void*	m_LockVertexBuffer;
#endif

#if defined( RECORDING ) || defined( CHECK_INDICES )
	void*	m_LockIndexBuffer;
	int		m_LockIndexBufferSize;
#endif

};

//-----------------------------------------------------------------------------
// A little extra stuff for the dynamic version
//-----------------------------------------------------------------------------

class CDynamicMeshDX8 : public CMeshDX8
{
public:
	// constructor, destructor
	CDynamicMeshDX8();
	virtual ~CDynamicMeshDX8();

	// Sets the vertex format
	void SetVertexFormat( VertexFormat_t format );

	// Resets the state in case of a task switch
	void Reset();

	// Do I have enough room in the buffer?
	bool HasEnoughRoom( int numVerts, int numIndices ) const;

	// returns the # of indices
	int NumIndices( ) const;

	// Locks the mesh
	void LockMesh( int numVerts, int numIndices, MeshDesc_t& desc );

	// Unlocks the mesh
	void UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc );

	// Override vertex + index buffer
	void OverrideVertexBuffer( CVertexBuffer* pStaticVertexBuffer );
	void OverrideIndexBuffer( CIndexBuffer* pStaticIndexBuffer );

	// Do I need to reset the vertex format?
	bool NeedsVertexFormatReset(VertexFormat_t fmt) const;

	// Draws it						   
	void Draw( int firstIndex, int numIndices );

	// Simply draws what's been buffered up immediately, without state change 
	void DrawSinglePassImmediately();

	// Operation to do pre-lock
	void PreLock();

	bool IsStatic() { return false; };

	virtual void SetSoftwareVertexShader( SoftwareVertexShader_t shader );
	
	virtual void CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder );

private:
	// Resets buffering state
	void ResetVertexAndIndexCounts();

	// total queued vertices
	int m_TotalVertices;
	int	m_TotalIndices;

	// the first vertex and index since the last draw
	int m_FirstVertex;
	int m_FirstIndex;

	// Have we drawn since the last lock?
	bool m_HasDrawn;

	// Any overrides?
	bool m_VertexOverride;
	bool m_IndexOverride;

	SoftwareVertexShader_t m_SoftwareVertexShader;
};


//-----------------------------------------------------------------------------
// A mesh that stores temporary vertex data in the correct format (for modification)
//-----------------------------------------------------------------------------

class CTempMeshDX8 : public CBaseMeshDX8
{
public:
	// constructor, destructor
	CTempMeshDX8( bool isDynamic );
	virtual ~CTempMeshDX8();

	// Sets the material
	void SetVertexFormat( VertexFormat_t format );

	// Locks/unlocks the mesh
	void LockMesh( int numVerts, int numIndices, MeshDesc_t& desc );
	void UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc );

	// Locks mesh for modifying
	virtual void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc );
	virtual void ModifyEnd();

	// Number of indices + vertices
	int NumVertices() const;
	int NumIndices() const;

	// Sets the primitive type
	void SetPrimitiveType( MaterialPrimitiveType_t type );
	MaterialPrimitiveType_t GetPrimitiveType() const;

	// Begins a pass
	void BeginPass( );

	// Draws a single pass
	void RenderPass();

	// Draws the entire beast
	void Draw( int firstIndex, int numIndices );

	virtual void CopyToMeshBuilder( 
		int iStartVert,		// Which vertices to copy.
		int nVerts, 
		int iStartIndex,	// Which indices to copy.
		int nIndices, 
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder );

	virtual void SetSoftwareVertexShader( SoftwareVertexShader_t shader );
	
	virtual void CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder );

private:
	// Selection mode 
	void TestSelection( );
	void ClipTriangle( D3DXVECTOR3** ppVert, float zNear, D3DXMATRIX& proj );

	CDynamicMeshDX8* GetDynamicMesh();

	CUtlVector< unsigned char > m_VertexData;
	CUtlVector< unsigned short > m_IndexData;

	unsigned short m_VertexSize;
	MaterialPrimitiveType_t m_Type;
	int m_LockedVerts;
	int m_LockedIndices;
	bool m_IsDynamic;

	// Used in rendering sub-parts of the mesh
	static unsigned int s_NumIndices;
	static unsigned int s_FirstIndex;

#ifdef _DEBUG
	bool m_Locked;
	bool m_InPass;
#endif
};


//-----------------------------------------------------------------------------
// This is a version that buffers up vertex data so we can blast through it later
//-----------------------------------------------------------------------------

class CBufferedMeshDX8 : public CBaseMeshDX8
{
public:
	// constructor, destructor
	CBufferedMeshDX8();
	virtual ~CBufferedMeshDX8();

	// checks to see if it was rendered..
	void ResetRendered();
	bool WasNotRendered() const;

	// Sets the mesh we're really going to draw into
	void SetMesh( CBaseMeshDX8* pMesh );

	// Sets the vertex format
	void SetVertexFormat( VertexFormat_t format );
	VertexFormat_t GetVertexFormat() const;

	// Sets the material
	void SetMaterial( IMaterial* pMaterial );

	// returns the number of indices (should never be called!)
	int NumIndices() const { Assert(0); return 0; }

	// Locks the mesh
	void LockMesh( int numVerts, int numIndices, MeshDesc_t& desc );
	void UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc );

	// Sets the primitive type
	void SetPrimitiveType( MaterialPrimitiveType_t type );
	MaterialPrimitiveType_t GetPrimitiveType( ) const;

	// Draws it
	void Draw( int firstIndex, int numIndices );

	// Renders a pass
	void RenderPass();

	// Flushes queued data
	void Flush( );

	virtual void SetSoftwareVertexShader( SoftwareVertexShader_t shader );
	
	virtual void CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder );

private:
	// The actual mesh we need to render....
	CBaseMeshDX8* m_pMesh;
	
	// The index of the last vertex (for tristrip fixup)
	unsigned short m_LastIndex;

	// Extra padding indices for tristrips
	unsigned short m_ExtraIndices;

	// Am I currently flushing?
	bool m_IsFlushing;

	// has the dynamic mesh been rendered?
	bool m_WasRendered;

	// Do I need to flush?
	bool m_FlushNeeded;

#ifdef DEBUG_BUFFERED_MESHES
	// for debugging only
	bool m_BufferedStateSet;
	BufferedState_t m_BufferedState;
#endif
};


//-----------------------------------------------------------------------------
// Implementation of the mesh manager
//-----------------------------------------------------------------------------

class CMeshMgr : public IMeshMgr
{
public:
	// constructor, destructor
	CMeshMgr();
	virtual ~CMeshMgr();

	// Initialize, shutdown
	void Init();
	void Shutdown();

	// Task switch...
	void ReleaseBuffers();
	void RestoreBuffers();

	// Releases all dynamic vertex buffers
	void DestroyVertexBuffers();

	// Flushes the dynamic mesh
	void Flush();

	// Flushes the vertex buffers
	void DiscardVertexBuffers();

	// Creates, destroys static meshes
	IMesh*  CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh );
	IMesh*	CreateStaticMesh( VertexFormat_t vertexFormat, bool bSoftwareVertexShader );
	void	DestroyStaticMesh( IMesh* pMesh );

	// Gets at the dynamic mesh	(spoofs it though)
	IMesh*	GetDynamicMesh( IMaterial* pMaterial, bool buffered,
		IMesh* pVertexOverride, IMesh* pIndexOverride );

	// Gets at the *actual* dynamic mesh
	IMesh*	GetActualDynamicMesh( VertexFormat_t vertexFormat );

	// Computes a 'standard' format to use that's at least as large
	// as the passed-in format and is cached aligned
	VertexFormat_t ComputeVertexFormat( unsigned int flags, 
				int numTexCoords, int* pTexCoordDimensions, int numBoneWeights,
				int userDataSize ) const;

	// Computes one of our standard formats that can fit the requested one.
	VertexFormat_t ComputeStandardFormat( VertexFormat_t fmt ) const;

	// Returns a vertex buffer appropriate for the flags
	CVertexBuffer* FindOrCreateVertexBuffer( VertexFormat_t fmt );
	CIndexBuffer* GetDynamicIndexBuffer();

	// Is the mesh dynamic?
	bool IsDynamicMesh( IMesh* pMesh ) const;

    // Returns the vertex size 
	int VertexFormatSize( VertexFormat_t vertexFormat ) const;

	// Computes the vertex buffer pointers 
	void ComputeVertexDescription( unsigned char* pBuffer, 
		VertexFormat_t vertexFormat, MeshDesc_t& desc ) const;

	// Returns the number of buffers...
	int BufferCount() const
	{
#ifdef _DEBUG
		return CVertexBuffer::BufferCount() + CIndexBuffer::BufferCount();
#else
		return 0;
#endif
	}

private:
	struct VertexBufferLookup_t
	{
		CVertexBuffer*	m_pBuffer;
		int	m_VertexSize;
	};

	void CopyStaticMeshIndexBufferToTempMeshIndexBuffer( CTempMeshDX8 *pDstIndexMesh,
													     CMeshDX8 *pSrcIndexMesh );

	// Cleans up the class
	void CleanUp();

	// The dynamic index buffer
	CIndexBuffer* m_pDynamicIndexBuffer;

	// The dynamic vertex buffers
	CUtlVector< VertexBufferLookup_t >	m_DynamicVertexBuffers;

	// The buffered mesh
	CBufferedMeshDX8 m_BufferedMesh;

	// The current dynamic mesh
	CDynamicMeshDX8 m_DynamicMesh;

	// The dynamic mesh temp version (for shaders that modify vertex data)
	CTempMeshDX8 m_DynamicTempMesh;

	// Am I buffering or not?
	bool m_BufferedMode;
};


//-----------------------------------------------------------------------------
// Singleton...
//-----------------------------------------------------------------------------
static CMeshMgr g_MeshMgr;

IMeshMgr* MeshMgr()
{
	return &g_MeshMgr;
}


//-----------------------------------------------------------------------------
// Helpers with meshdescs...
//-----------------------------------------------------------------------------
inline D3DXVECTOR3& Position( MeshDesc_t const& desc, int vert )
{
	return *(D3DXVECTOR3*)((unsigned char*)desc.m_pPosition + vert * desc.m_VertexSize_Position );
}

inline D3DXVECTOR3& BoneWeight( MeshDesc_t const& desc, int vert )
{
	return *(D3DXVECTOR3*)((unsigned char*)desc.m_pBoneWeight + vert * desc.m_VertexSize_BoneWeight );
}

inline unsigned char *BoneIndex( MeshDesc_t const& desc, int vert )
{
	return desc.m_pBoneMatrixIndex + vert * desc.m_VertexSize_BoneMatrixIndex;
}

inline D3DXVECTOR3& Normal( MeshDesc_t const& desc, int vert )
{
	return *(D3DXVECTOR3*)((unsigned char*)desc.m_pNormal + vert * desc.m_VertexSize_Normal );
}

inline unsigned char* Color( MeshDesc_t const& desc, int vert )
{
	return desc.m_pColor + vert * desc.m_VertexSize_Color;
}

inline D3DXVECTOR2& TexCoord( MeshDesc_t const& desc, int vert, int stage )
{
	return *(D3DXVECTOR2*)((unsigned char*)desc.m_pTexCoord[stage] + vert * desc.m_VertexSize_TexCoord[stage] );
}

inline D3DXVECTOR3& TangentS( MeshDesc_t const& desc, int vert )
{
	return *(D3DXVECTOR3*)((unsigned char*)desc.m_pTangentS + vert * desc.m_VertexSize_TangentS );
}

inline D3DXVECTOR3& TangentT( MeshDesc_t const& desc, int vert )
{
	return *(D3DXVECTOR3*)((unsigned char*)desc.m_pTangentT + vert * desc.m_VertexSize_TangentT );
}


//-----------------------------------------------------------------------------
//
// Base mesh
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CBaseMeshDX8::CBaseMeshDX8() : m_VertexFormat(0)
{
#ifdef _DEBUG
	m_IsDrawing = false;
	m_pMaterial = 0;
#endif
}

CBaseMeshDX8::~CBaseMeshDX8()
{			   
}


//-----------------------------------------------------------------------------
// For debugging...
//-----------------------------------------------------------------------------
bool CBaseMeshDX8::DebugTrace() const
{
#ifdef _DEBUG
	if (m_pMaterial)
		return m_pMaterial->PerformDebugTrace();
#endif

	return false;
}

void CBaseMeshDX8::SetMaterial( IMaterial* pMaterial )
{
#ifdef _DEBUG
	m_pMaterial = static_cast<IMaterialInternal*>(pMaterial);
#endif
}


//-----------------------------------------------------------------------------
// Sets, gets the material
//-----------------------------------------------------------------------------
void CBaseMeshDX8::SetVertexFormat( VertexFormat_t format )
{
	m_VertexFormat = format;
}

VertexFormat_t CBaseMeshDX8::GetVertexFormat() const
{
	return m_VertexFormat;
}


//-----------------------------------------------------------------------------
// Do I need to reset the vertex format?
//-----------------------------------------------------------------------------
bool CBaseMeshDX8::NeedsVertexFormatReset( VertexFormat_t fmt ) const
{
	return m_VertexFormat != fmt;
}

//-----------------------------------------------------------------------------
// Do I have enough room?
//-----------------------------------------------------------------------------

bool CBaseMeshDX8::HasEnoughRoom( int numVerts, int numIndices ) const
{
	// by default, we do
	return true;
}


//-----------------------------------------------------------------------------
// Uses pre-defined index buffers
//-----------------------------------------------------------------------------
void CBaseMeshDX8::GenerateSequentialIndexBuffer( unsigned short* pIndices, int numIndices, int firstVertex )
{
	if( !pIndices )
	{
		// app probably isn't active
		return;
	}
	
	// Format the sequential buffer
	for ( int i = 0; i < numIndices; ++i)
	{
		pIndices[i] = i + firstVertex;
	}
}

void CBaseMeshDX8::GenerateQuadIndexBuffer( unsigned short* pIndices, int numIndices, int firstVertex )
{
	if( !pIndices )
	{
		// app probably isn't active
		return;
	}
	
	// Format the quad buffer
	int i;
	int numQuads = numIndices / 6;
	int baseVertex = firstVertex;
	for ( i = 0; i < numQuads; ++i)
	{
		// Triangle 1
		pIndices[0] = baseVertex;
		pIndices[1] = baseVertex + 1;
		pIndices[2] = baseVertex + 2;

		// Triangle 2
		pIndices[3] = baseVertex;
		pIndices[4] = baseVertex + 2;
		pIndices[5] = baseVertex + 3;

		baseVertex += 4;
		pIndices += 6;
	}
}

void CBaseMeshDX8::GeneratePolygonIndexBuffer( unsigned short* pIndices, int numIndices, int firstVertex )
{
	if( !pIndices )
	{
		// app probably isn't active
		return;
	}
	
	int i, baseVertex;
	int numPolygons = numIndices / 3;
	baseVertex = firstVertex;
	for ( i = 0; i < numPolygons; ++i)
	{
		// Triangle 1
		pIndices[0] = firstVertex;
		pIndices[1] = firstVertex + i + 1;
		pIndices[2] = firstVertex + i + 2;
		pIndices += 3;
	}
}

void CBaseMeshDX8::GenerateLineStripIndexBuffer( unsigned short* pIndices, 
												 int numIndices, int firstVertex )
{
	if( !pIndices )
	{
		// app probably isn't active
		return;
	}
	
	int i, baseVertex;
	int numLines = numIndices / 2;
	baseVertex = firstVertex;
	for ( i = 0; i < numLines; ++i)
	{
		pIndices[0] = firstVertex + i;
		pIndices[1] = firstVertex + i + 1;
		pIndices += 2;
	}
}

void CBaseMeshDX8::GenerateLineLoopIndexBuffer( unsigned short* pIndices, 
											 int numIndices, int firstVertex )
{
	if( !pIndices )
	{
		// app probably isn't active
		return;
	}
	
	int i, baseVertex;
	int numLines = numIndices / 2;
	baseVertex = firstVertex;

	pIndices[0] = firstVertex + numLines - 1;
	pIndices[1] = firstVertex;
	pIndices += 2;

	for ( i = 1; i < numLines; ++i)
	{
		pIndices[0] = firstVertex + i - 1;
		pIndices[1] = firstVertex + i;
		pIndices += 2;
	}
}


//-----------------------------------------------------------------------------
// Locks mesh for modifying
//-----------------------------------------------------------------------------
void CBaseMeshDX8::ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
{
	// for the time being, disallow for most cases
	Assert(0);
}

void CBaseMeshDX8::ModifyEnd()
{
	// for the time being, disallow for most cases
	Assert(0);
}


//-----------------------------------------------------------------------------
// Begins a pass
//-----------------------------------------------------------------------------
void CBaseMeshDX8::BeginPass( )
{
}


//-----------------------------------------------------------------------------
// Sets the render state and gets the drawing going
//-----------------------------------------------------------------------------
inline void CBaseMeshDX8::DrawMesh( )
{
	MEASURE_TIMED_STAT( MATERIAL_SYSTEM_STATS_DRAW_MESH );

#ifdef _DEBUG
	// Make sure we're not drawing...
	Assert( !m_IsDrawing );
	m_IsDrawing = true;
#endif

	// This is going to cause RenderPass to get called a bunch
	ShaderAPI()->DrawMesh( this );

#ifdef _DEBUG
	m_IsDrawing = false;
#endif
}


static void PrintVertexFormat( unsigned int vertexFormat )
{
	char buf[256];
	if( vertexFormat & VERTEX_POSITION )
	{
		OutputDebugString( "VERTEX_POSITION|" );
	}
	if( vertexFormat & VERTEX_NORMAL )
	{
		OutputDebugString( "VERTEX_NORMAL|" );
	}
	if( vertexFormat & VERTEX_COLOR )
	{
		OutputDebugString( "VERTEX_COLOR|" );
	}
	if( vertexFormat & VERTEX_SPECULAR )
	{
		OutputDebugString( "VERTEX_SPECULAR|" );
	}
	if( vertexFormat & VERTEX_TANGENT_S )
	{
		OutputDebugString( "VERTEX_TANGENT_S|" );
	}
	if( vertexFormat & VERTEX_TANGENT_T )
	{
		OutputDebugString( "VERTEX_TANGENT_T|" );
	}
	if( vertexFormat & VERTEX_BONE_INDEX )
	{
		OutputDebugString( "VERTEX_BONE_INDEX|" );
	}
	if( vertexFormat & VERTEX_FORMAT_VERTEX_SHADER )
	{
		OutputDebugString( "VERTEX_FORMAT_VERTEX_SHADER|" );
	}
	if( NumBoneWeights( vertexFormat ) > 0 )
	{
		sprintf( buf, "VERTEX_BONEWEIGHT(%d)|", NumBoneWeights( vertexFormat ) );
		OutputDebugString( buf );
	}
	if( UserDataSize( vertexFormat ) > 0 )
	{
		sprintf( buf, "VERTEX_USERDATA_SIZE(%d)|", UserDataSize( vertexFormat ) );
		OutputDebugString( buf );
	}
	if( NumTextureCoordinates( vertexFormat ) > 0 )
	{
		sprintf( buf, "VERTEX_NUM_TEXCOORDS(%d)|", NumTextureCoordinates( vertexFormat ) );
		OutputDebugString( buf );
	}
	int i;
	for( i = 0; i < NumTextureCoordinates( vertexFormat ); i++ )
	{
		sprintf( buf, "VERTEX_TEXCOORD_SIZE(%d,%d)", i, TexCoordSize( i, vertexFormat ) );
		OutputDebugString( buf );
	}
	OutputDebugString( "\n" );
}


//-----------------------------------------------------------------------------
// Spews the mesh data
//-----------------------------------------------------------------------------
void CBaseMeshDX8::Spew( int numVerts, int numIndices, MeshDesc_t const& spewDesc )
{
	int i;

#ifdef _DEBUG
	if( m_pMaterial )
	{
		OutputDebugString( ( const char * )m_pMaterial->GetName() );
		OutputDebugString( "\n" );
	}
#endif // _DEBUG
	
	// This is needed so buffering can just use this
	VertexFormat_t fmt = m_VertexFormat;

	// Set up the vertex descriptor
	MeshDesc_t desc;
	memcpy( &desc, &spewDesc, sizeof(MeshDesc_t) );

	char tempbuf[256];
	char* temp = tempbuf;
	sprintf( tempbuf,"\nVerts: (Vertex Format %x)\n", fmt);
	OutputDebugString(tempbuf);

	PrintVertexFormat( fmt );

	int numBoneWeights = NumBoneWeights( fmt );
	for ( i = 0; i < numVerts; ++i )
	{
		temp += sprintf( temp, "[%4d] ", i + desc.m_FirstVertex );
		if( fmt & VERTEX_POSITION )
		{
			D3DXVECTOR3& pos = Position( desc, i );
			temp += sprintf(temp, "P %8.2f %8.2f %8.2f ",
				pos[0], pos[1], pos[2]);
		}

		if (numBoneWeights > 0)
		{
			float* pWeight = BoneWeight( desc, i );
			temp += sprintf(temp, "BW ");
			for (int j = 0; j < numBoneWeights; ++j)
			{
				temp += sprintf(temp, "%1.2f ", pWeight[j]);
			}
			if( fmt & VERTEX_BONE_INDEX )
			{
				unsigned char *pIndex = BoneIndex( desc, i );
				temp += sprintf( temp, "BI %d %d %d %d ", ( int )pIndex[0], ( int )pIndex[1], ( int )pIndex[2], ( int )pIndex[3] );
				Assert( pIndex[0] >= 0 && pIndex[0] < 16 );
				Assert( pIndex[1] >= 0 && pIndex[1] < 16 );
				Assert( pIndex[2] >= 0 && pIndex[2] < 16 );
				Assert( pIndex[3] >= 0 && pIndex[3] < 16 );
			}
		}

		if( fmt & VERTEX_NORMAL )
		{
			D3DXVECTOR3& normal = Normal( desc, i );
			temp += sprintf(temp, "N %1.2f %1.2f %1.2f ",
				normal[0],	normal[1],	normal[2]);
		}
		
		if (fmt & VERTEX_COLOR)
		{
			unsigned char* pColor = Color( desc, i );
			temp += sprintf(temp, "C b %3d g %3d r %3d a %3d ",
				pColor[0], pColor[1], pColor[2], pColor[3]);
		}

		for (int j = 0; j < HardwareConfig()->GetNumTextureUnits(); ++j)
		{
			if( TexCoordSize( (TextureStage_t)j, fmt ) > 0)
			{
				D3DXVECTOR2& texcoord = TexCoord( desc, i, j );
				temp += sprintf(temp, "T%d %.2f %.2f ", j,texcoord[0], texcoord[1]);
			}
		}

		if (fmt & VERTEX_TANGENT_S)
		{
			D3DXVECTOR3& tangentS = TangentS( desc, i );
			temp += sprintf(temp, "S %1.2f %1.2f %1.2f ",
				tangentS[0], tangentS[1], tangentS[2]);
		}

		if (fmt & VERTEX_TANGENT_T)
		{
			D3DXVECTOR3& tangentT = TangentT( desc, i );
			temp += sprintf(temp, "T %1.2f %1.2f %1.2f ",
				tangentT[0], tangentT[1], tangentT[2]);
		}

		sprintf(temp,"\n");
		OutputDebugString(tempbuf);
		temp = tempbuf;
	}

	sprintf( tempbuf,"\nIndices: %d\n", numIndices );
	OutputDebugString(tempbuf);
	for ( i = 0; i < numIndices; ++i )
	{
		temp += sprintf(temp, "%d ", desc.m_pIndices[i] );
		if ((i & 0x0F) == 0x0F)
		{
			sprintf(temp,"\n");
			OutputDebugString(tempbuf);
			temp = tempbuf;
		}
	}
	sprintf(temp,"\n");
	OutputDebugString(temp);
}

void CBaseMeshDX8::ValidateData( int numVerts, int numIndices, MeshDesc_t const& spewDesc )
{
#ifdef BLAH_DEBUG
	int i;

	// This is needed so buffering can just use this
	VertexFormat_t fmt = m_VertexFormat;

	// Set up the vertex descriptor
	MeshDesc_t desc;
	memcpy( &desc, &spewDesc, sizeof(MeshDesc_t) );

	int numBoneWeights = NumBoneWeights( fmt );
	for ( i = 0; i < numVerts; ++i )
	{
		if (numBoneWeights > 0)
		{
			float* pWeight = BoneWeight( desc, i );
			for (int j = 0; j < numBoneWeights; ++j)
			{
				Assert( pWeight[j] >= 0.0f && pWeight[j] <= 1.0f );
			}
			if( fmt & VERTEX_BONE_INDEX )
			{
				unsigned char *pIndex = BoneIndex( desc, i );
				Assert( pIndex[0] >= 0 && pIndex[0] < 16 );
				Assert( pIndex[1] >= 0 && pIndex[1] < 16 );
				Assert( pIndex[2] >= 0 && pIndex[2] < 16 );
				Assert( pIndex[3] >= 0 && pIndex[3] < 16 );
			}
		}
		if( fmt & VERTEX_NORMAL )
		{
			D3DXVECTOR3& normal = Normal( desc, i );
			Assert( normal[0] >= -1.05f && normal[0] <= 1.05f );
			Assert( normal[1] >= -1.05f && normal[1] <= 1.05f );
			Assert( normal[2] >= -1.05f && normal[2] <= 1.05f );
		}
		
	}
#endif // _DEBUG
}

void CBaseMeshDX8::Draw( CPrimList *pLists, int nLists )
{
	Assert( !"CBaseMeshDX8::Draw(CPrimList, int): should never get here." );
}


// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
void CBaseMeshDX8::CopyToMeshBuilder( 
	int iStartVert,		// Which vertices to copy.
	int nVerts, 
	int iStartIndex,	// Which indices to copy.
	int nIndices, 
	int indexOffset,	// This is added to each index.
	CMeshBuilder &builder )
{
	Assert( false );
	Warning( "CopyToMeshBuilder called on something other than a temp mesh.\n" );
}


//-----------------------------------------------------------------------------
//
// static mesh
//
//-----------------------------------------------------------------------------

CPrimList *CMeshDX8::s_pPrims;
int CMeshDX8::s_nPrims;
unsigned int CMeshDX8::s_FirstVertex;
unsigned int CMeshDX8::s_NumVertices;


//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
CMeshDX8::CMeshDX8( ) : m_NumVertices(0), m_NumIndices(0), m_pVertexBuffer(0),
	m_pColorMesh( 0 ),
	m_pIndexBuffer(0), m_Type(MATERIAL_TRIANGLES), m_IsVBLocked(false),
	m_IsIBLocked(false)
{
	m_Mode = ComputeMode(m_Type);
}

CMeshDX8::~CMeshDX8()
{
	// Don't release the vertex buffer 
	if (!g_MeshMgr.IsDynamicMesh(this))
	{
		if (m_pVertexBuffer)
			delete m_pVertexBuffer;
		if (m_pIndexBuffer)
			delete m_pIndexBuffer;
	}
}
	
void CMeshDX8::SetColorMesh( IMesh *pColorMesh )
{
	m_pColorMesh = ( CMeshDX8 * )pColorMesh; // dangerous conversion! garymcthack
#ifdef _DEBUG
	if( pColorMesh )
	{
		int numVerts = NumVertices();
		int numVertsColorMesh = m_pColorMesh->NumVertices();
		Assert( numVerts == numVertsColorMesh );
	}
#endif
}

bool CMeshDX8::HasColorMesh( ) const
{
	return (m_pColorMesh != NULL);
}


//-----------------------------------------------------------------------------
// Locks/ unlocks the vertex buffer
//-----------------------------------------------------------------------------
void CMeshDX8::LockVertexBuffer( int numVerts, MeshDesc_t& desc )
{
	Assert( !m_IsVBLocked );

	// Just give the app crap buffers to fill up while we're suppressed...
	if (ShaderAPI()->IsDeactivated() || (numVerts == 0))
	{
		// Set up the vertex descriptor
		g_MeshMgr.ComputeVertexDescription( 0, 0, desc );
		desc.m_FirstVertex = 0;
		return;
	}

	MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_BUFFER_LOCK_TIME);
	MaterialSystemStats()->IncrementCountedStat( MATERIAL_SYSTEM_STATS_NUM_BUFFER_LOCK, 1 );

	// Static vertex buffer case
	if (!m_pVertexBuffer)
	{
		int size = g_MeshMgr.VertexFormatSize( m_VertexFormat );
		m_pVertexBuffer = new CVertexBuffer( D3DDevice(), 0, size, numVerts );
	}

	// Lock it baby
	Assert( numVerts <= VERTEX_BUFFER_SIZE );
	unsigned char* pVertexMemory = m_pVertexBuffer->Lock( numVerts, desc.m_FirstVertex );
	if( !pVertexMemory )
	{
		if( numVerts > VERTEX_BUFFER_SIZE )
		{
			Assert( 0 );
			Error( "Too many verts for a dynamic vertex buffer (%d>%d) Tell a programmer to up VERTEX_BUFFER_SIZE.\n", 
				( int )numVerts, ( int )VERTEX_BUFFER_SIZE );
		}
		else
		{
			Assert( 0 );
			Error( "failed to lock vertex buffer in CMeshDX8::LockVertexBuffer\n" );
		}
		g_MeshMgr.ComputeVertexDescription( 0, 0, desc );
		return;
	}

	// Set up the vertex descriptor
	g_MeshMgr.ComputeVertexDescription( pVertexMemory, m_VertexFormat, desc );
	m_IsVBLocked = true;

#ifdef RECORDING
	m_LockVertexBufferSize = numVerts * desc.m_ActualVertexSize;
	m_LockVertexBuffer = pVertexMemory;
#endif
}

void CMeshDX8::UnlockVertexBuffer( int numVerts )
{
	// NOTE: This can happen if another application finishes
	// initializing during the construction of a mesh
	if (!m_IsVBLocked)
		return;

	// This is recorded for debugging. . not sent to dx.
	RECORD_COMMAND( DX8_SET_VERTEX_BUFFER_FORMAT, 2 );
	RECORD_INT( m_pVertexBuffer->UID() );
	RECORD_INT( m_VertexFormat );
	
	RECORD_COMMAND( DX8_VERTEX_DATA, 3 );
	RECORD_INT( m_pVertexBuffer->UID() );
	RECORD_INT( m_LockVertexBufferSize );
	RECORD_STRUCT( m_LockVertexBuffer, m_LockVertexBufferSize );

	MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_BUFFER_UNLOCK_TIME);
	Assert(m_pVertexBuffer);
	m_pVertexBuffer->Unlock(numVerts);
	m_IsVBLocked = false;
}

//-----------------------------------------------------------------------------
// Locks/unlocks the index buffer
//-----------------------------------------------------------------------------

int CMeshDX8::LockIndexBuffer( int firstIndex, int numIndices, MeshDesc_t& desc )
{
	Assert( !m_IsIBLocked );

	// Just give the app crap buffers to fill up while we're suppressed...
	if (ShaderAPI()->IsDeactivated() || (numIndices == 0))
	{
		// Set up a bogus index descriptor
		desc.m_pIndices = NULL;
		return 0;
	}

	MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_BUFFER_LOCK_TIME);
	MaterialSystemStats()->IncrementCountedStat( MATERIAL_SYSTEM_STATS_NUM_BUFFER_LOCK, 1 );

	// Static vertex buffer case
	if (!m_pIndexBuffer)
		m_pIndexBuffer = new CIndexBuffer( D3DDevice(), numIndices );

	int startIndex;
	desc.m_pIndices = m_pIndexBuffer->Lock( numIndices, startIndex, firstIndex );
	m_IsIBLocked = true;

#if defined( RECORDING ) || defined( CHECK_INDICES )
	m_LockIndexBufferSize = numIndices * 2;
	m_LockIndexBuffer = desc.m_pIndices;
#endif

	return startIndex;
}

void CMeshDX8::UnlockIndexBuffer( int numIndices )
{
	// NOTE: This can happen if another application finishes
	// initializing during the construction of a mesh
	if (!m_IsIBLocked)
		return;

	RECORD_COMMAND( DX8_INDEX_DATA, 3 );
	RECORD_INT( m_pIndexBuffer->UID() );
	RECORD_INT( m_LockIndexBufferSize );
	RECORD_STRUCT( m_LockIndexBuffer, m_LockIndexBufferSize );

	MEASURE_TIMED_STAT(MATERIAL_SYSTEM_STATS_BUFFER_UNLOCK_TIME);
	Assert(m_pIndexBuffer);

#ifdef CHECK_INDICES
	m_pIndexBuffer->UpdateShadowIndices( ( unsigned short * )m_LockIndexBuffer );
#endif // CHECK_INDICES
	
	// Unlock, and indicate how many vertices we actually used
	m_pIndexBuffer->Unlock(numIndices);
	m_IsIBLocked = false;
}


//-----------------------------------------------------------------------------
// Locks/unlocks the entire mesh
//-----------------------------------------------------------------------------

void CMeshDX8::LockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
#ifdef MEASURE_STATS
	MaterialSystemStats()->BeginTimedStat(MATERIAL_SYSTEM_STATS_MESH_BUILD_TIME);
#endif

	LockVertexBuffer( numVerts, desc );
	if (m_Type != MATERIAL_POINTS)
		LockIndexBuffer( -1, numIndices, desc );
	else
		desc.m_pIndices = 0;
}

void CMeshDX8::UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	UnlockVertexBuffer(numVerts);
	if (m_Type != MATERIAL_POINTS)
		UnlockIndexBuffer(numIndices);
																	    
	// The actual # we wrote
	m_NumVertices = numVerts;
	m_NumIndices = numIndices;

#ifdef MEASURE_STATS
	MaterialSystemStats()->EndTimedStat(MATERIAL_SYSTEM_STATS_MESH_BUILD_TIME);
#endif
}

 
//-----------------------------------------------------------------------------
// Locks mesh for modifying
//-----------------------------------------------------------------------------

void CMeshDX8::ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
{
#ifdef MEASURE_STATS
	MaterialSystemStats()->BeginTimedStat(MATERIAL_SYSTEM_STATS_MESH_BUILD_TIME);
#endif

	// Just give the app crap buffers to fill up while we're suppressed...
	if (ShaderAPI()->IsDeactivated())
	{
		// Set up a bogus descriptor
		g_MeshMgr.ComputeVertexDescription( 0, 0, desc );
		desc.m_pIndices = 0;
		return;
	}

	Assert( m_pVertexBuffer );

	// Lock it baby
	unsigned char* pVertexMemory = m_pVertexBuffer->Modify( firstVertex, numVerts );
	if ( pVertexMemory )
	{
		m_IsVBLocked = true;
		g_MeshMgr.ComputeVertexDescription( pVertexMemory, m_VertexFormat, desc );

#ifdef RECORDING
		m_LockVertexBufferSize = numVerts * desc.m_ActualVertexSize;
		m_LockVertexBuffer = pVertexMemory;
#endif
	}

	desc.m_FirstVertex = firstVertex;

	LockIndexBuffer( firstIndex, numIndices, desc );
}

void CMeshDX8::ModifyEnd( )
{
	UnlockIndexBuffer(0);
	UnlockVertexBuffer(0);

#ifdef MEASURE_STATS
	MaterialSystemStats()->EndTimedStat(MATERIAL_SYSTEM_STATS_MESH_BUILD_TIME);
#endif
}

//-----------------------------------------------------------------------------
// returns the # of vertices (static meshes only)
//-----------------------------------------------------------------------------

int CMeshDX8::NumVertices() const
{
	return m_pVertexBuffer ? m_pVertexBuffer->VertexCount() : 0;
}

//-----------------------------------------------------------------------------
// returns the # of indices 
//-----------------------------------------------------------------------------

int CMeshDX8::NumIndices( ) const
{
	return m_pIndexBuffer ? m_pIndexBuffer->IndexCount() : 0;
}

//-----------------------------------------------------------------------------
// Sets up the vertex and index buffers
//-----------------------------------------------------------------------------

void CMeshDX8::UseIndexBuffer( CIndexBuffer* pBuffer )
{
	m_pIndexBuffer = pBuffer;
}

void CMeshDX8::UseVertexBuffer( CVertexBuffer* pBuffer )
{
	m_pVertexBuffer = pBuffer;
}


//-----------------------------------------------------------------------------
// Computes the mode
//-----------------------------------------------------------------------------

D3DPRIMITIVETYPE CMeshDX8::ComputeMode( MaterialPrimitiveType_t type )
{
	switch(type)
	{
	case MATERIAL_POINTS:
		return D3DPT_POINTLIST;
		
	case MATERIAL_LINES:
		return D3DPT_LINELIST;

	case MATERIAL_TRIANGLES:
		return D3DPT_TRIANGLELIST;

	case MATERIAL_TRIANGLE_STRIP:
		return D3DPT_TRIANGLESTRIP;

	// Here, we expect to have the type set later. only works for static meshes
	case MATERIAL_HETEROGENOUS:
		return (D3DPRIMITIVETYPE)-1;

	default:
		Assert(0);
		return (D3DPRIMITIVETYPE)-1;
	}
}

//-----------------------------------------------------------------------------
// Sets the primitive type
//-----------------------------------------------------------------------------

void CMeshDX8::SetPrimitiveType( MaterialPrimitiveType_t type )
{
	m_Type = type;
	m_Mode = ComputeMode( type );
}

MaterialPrimitiveType_t CMeshDX8::GetPrimitiveType( ) const
{
	return m_Type;
}


//-----------------------------------------------------------------------------
// Computes the number of primitives we're gonna draw
//-----------------------------------------------------------------------------

int CMeshDX8::NumPrimitives( int numVerts, int numIndices ) const
{
	switch(m_Mode)
	{
	case D3DPT_POINTLIST:
		return numVerts;
		
	case D3DPT_LINELIST:
		return numIndices / 2;

	case D3DPT_TRIANGLELIST:
		return numIndices / 3;

	case D3DPT_TRIANGLESTRIP:
		return numIndices - 2;

	default:
		// invalid, baby!
		Assert(0);
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Checks if it's a valid format
//-----------------------------------------------------------------------------

static void OutputVertexFormat( VertexFormat_t format )
{
	if( format & VERTEX_POSITION )
	{
		Warning( "VERTEX_POSITION|" );
	}
	if( format & VERTEX_NORMAL )
	{
		Warning( "VERTEX_NORMAL|" );
	}
	if( format & VERTEX_COLOR )
	{
		Warning( "VERTEX_COLOR|" );
	}
	if( format & VERTEX_SPECULAR )
	{
		Warning( "VERTEX_SPECULAR|" );
	}
	if( format & VERTEX_TANGENT_S )
	{
		Warning( "VERTEX_TANGENT_S|" );
	}
	if( format & VERTEX_TANGENT_T )
	{
		Warning( "VERTEX_TANGENT_T|" );
	}
	if( format & VERTEX_BONE_INDEX )
	{
		Warning( "VERTEX_BONE_INDEX|" );
	}
	if( format & VERTEX_FORMAT_VERTEX_SHADER )
	{
		Warning( "VERTEX_FORMAT_VERTEX_SHADER|" );
	}
	Warning( "Bone weights: %d\n", 
		( int )( ( format & VERTEX_BONE_WEIGHT_MASK ) >> VERTEX_BONE_WEIGHT_BIT ) );
	Warning( "user data size: %d\n", 
		( int )( ( format & USER_DATA_SIZE_MASK ) >> USER_DATA_SIZE_BIT ) );
	Warning( "num tex coords: %d\n", 
		( int )( ( format & NUM_TEX_COORD_MASK ) >> NUM_TEX_COORD_BIT ) );
	// NOTE: This doesn't print texcoord sizes.
}

bool CMeshDX8::IsValidVertexFormat()
{
	IMaterialInternal* pMaterial = ShaderAPI()->GetBoundMaterial();
	Assert( pMaterial );

	VertexFormat_t fmt = pMaterial->GetVertexFormat() & ~VERTEX_FORMAT_VERTEX_SHADER;
	bool isValid = (fmt == (m_VertexFormat  & ~VERTEX_FORMAT_VERTEX_SHADER));
#ifdef _DEBUG
	if( !isValid )
	{
		Warning( "material format:" );
		OutputVertexFormat( fmt );
		Warning( "mesh format:" );
		OutputVertexFormat( m_VertexFormat );
	}
#endif
	return isValid;
}


//-----------------------------------------------------------------------------
// Flushes queued data
//-----------------------------------------------------------------------------
static CIndexBuffer* g_pLastIndex = 0;
static CVertexBuffer* g_pLastVertex = 0;
static int g_LastVertexIdx = -1;
static CMeshDX8 *g_pLastColorMesh = 0;


//-----------------------------------------------------------------------------
// Makes sure that the render state is always set next time
//-----------------------------------------------------------------------------
static void ResetRenderState()
{
	g_pLastIndex = 0;
	g_pLastVertex = 0;
	g_LastVertexIdx = -1;
}

bool CMeshDX8::SetRenderState( int firstVertexIdx )
{
	MEASURE_TIMED_STAT( MATERIAL_SYSTEM_STATS_SET_RENDER_STATE );

	// Can't set the state if we're deactivated
	if (ShaderAPI()->IsDeactivated())
	{
		ResetRenderState();
		return false;
	}

	// make sure the vertex format is a superset of the current material's
	// vertex format...
	if (!IsValidVertexFormat())
	{
		Warning("Material %s is being applied to a model, you need $model=1 in the .vmt file!\n",
			ShaderAPI()->GetBoundMaterial()->GetName() );
		return false;
	}

	if( m_pColorMesh != g_pLastColorMesh )
	{
		HRESULT hr;
		if( m_pColorMesh )
		{
			RECORD_COMMAND( DX8_SET_STREAM_SOURCE, 3 );
			RECORD_INT( m_pColorMesh->GetVertexBuffer()->UID() );
			RECORD_INT( 1 );
			RECORD_INT( m_pColorMesh->GetVertexBuffer()->VertexSize() );

			hr = D3DDevice()->SetStreamSource( 1, 
				m_pColorMesh->GetVertexBuffer()->GetInterface(), 
				0, 
				m_pColorMesh->GetVertexBuffer()->VertexSize() );
		}
		else
		{
			RECORD_COMMAND( DX8_SET_STREAM_SOURCE, 3 );
			RECORD_INT( -1 );	// vertex buffer id
			RECORD_INT( 1 );	// stream
			RECORD_INT( 0 );	// vertex size

			hr = D3DDevice()->SetStreamSource( 1, 0, 0, 0 );
		}
		Assert( !FAILED(hr) );
		g_pLastColorMesh = m_pColorMesh;
	}

	if (g_pLastVertex != m_pVertexBuffer)
	{
		Assert( m_pVertexBuffer );

		RECORD_COMMAND( DX8_SET_STREAM_SOURCE, 3 );
		RECORD_INT( m_pVertexBuffer->UID() );
		RECORD_INT( 0 );
		RECORD_INT( m_pVertexBuffer->VertexSize() );

		HRESULT hr;
		hr = D3DDevice()->SetStreamSource( 0, 
			m_pVertexBuffer->GetInterface(), 0, m_pVertexBuffer->VertexSize() );
		Assert( !FAILED(hr) );

		MaterialSystemStats()->IncrementCountedStat(MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );

		g_pLastVertex = m_pVertexBuffer;
	}


	if ((g_pLastIndex != m_pIndexBuffer) || (firstVertexIdx != g_LastVertexIdx))
	{
		Assert( m_pIndexBuffer );

		RECORD_COMMAND( DX8_SET_INDICES, 2 );
		RECORD_INT( m_pIndexBuffer->UID() );
		RECORD_INT( firstVertexIdx );

		HRESULT hr;
		hr = D3DDevice()->SetIndices( m_pIndexBuffer->GetInterface() );
		m_FirstIndex = firstVertexIdx;
		Assert( !FAILED(hr) );

		MaterialSystemStats()->IncrementCountedStat(MATERIAL_SYSTEM_STATS_DYNAMIC_STATE, 1 );

		g_pLastIndex = m_pIndexBuffer;
		g_LastVertexIdx = firstVertexIdx;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Draws the static mesh
//-----------------------------------------------------------------------------

void CMeshDX8::Draw( int firstIndex, int numIndices )
{
	if ( ShaderUtil()->IsInStubMode() )
		return;

	CPrimList primList;
	if( firstIndex == -1 || numIndices == 0 )
	{
		primList.m_FirstIndex = 0;
		primList.m_NumIndices = m_NumIndices;
	}
	else
	{
		primList.m_FirstIndex = firstIndex;
		primList.m_NumIndices = numIndices;
	}
	Draw( &primList, 1 );
}


void CMeshDX8::Draw( CPrimList *pLists, int nLists )
{
	if ( ShaderUtil()->IsInStubMode() )
		return;

	// Make sure there's something to draw..
	int i;
	for( i=0; i < nLists; i++ )
	{
		if( pLists[i].m_NumIndices > 0 )
			break;
	}
	if( i == nLists )
		return;

	// can't do these in selection mode!
	Assert( !ShaderAPI()->IsInSelectionMode() );

	if (!SetRenderState(0))
		return;

	s_pPrims = pLists;
	s_nPrims = nLists;

#ifdef _DEBUG
	for ( i = 0; i < nLists; ++i)
	{
		Assert( pLists[i].m_NumIndices > 0 );
	}
#endif

	s_FirstVertex = 0;
	s_NumVertices = m_pVertexBuffer->VertexCount();

	DrawMesh();
}

//-----------------------------------------------------------------------------
// Actually does the dirty deed of rendering
//-----------------------------------------------------------------------------

void CMeshDX8::RenderPass()
{
	VPROF( "CMeshDX8::RenderPass" );
	MEASURE_TIMED_STAT( MATERIAL_SYSTEM_STATS_DRAW_INDEXED_PRIMITIVE );

	HRESULT hr;
	Assert( m_Type != MATERIAL_HETEROGENOUS );

	for( int iPrim=0; iPrim < s_nPrims; iPrim++ )
	{
		CPrimList *pPrim = &s_pPrims[iPrim];

		if (pPrim->m_NumIndices == 0)
			continue;

		if (m_Type == MATERIAL_POINTS)
		{
			RECORD_COMMAND( DX8_DRAW_PRIMITIVE, 3 );
			RECORD_INT( m_Mode );
			RECORD_INT(	s_FirstVertex );
			RECORD_INT( pPrim->m_NumIndices );

			// (For point lists, we don't actually fill in indices, but we treat it as
			// though there are indices for the list up until here).
			hr = D3DDevice()->DrawPrimitive( m_Mode, s_FirstVertex, pPrim->m_NumIndices );
			MaterialSystemStats()->IncrementCountedStat(MATERIAL_SYSTEM_STATS_NUM_PRIMITIVES, pPrim->m_NumIndices );
		}
		else
		{
			int numPrimitives = NumPrimitives( s_NumVertices, pPrim->m_NumIndices );

#ifdef CHECK_INDICES_NOT_RIGHT_NOW_NO_LINELIST_SUPPORT_WHICH_MAKES_THIS_PAINFUL_FOR_DEBUG_MODE
			// g_pLastVertex - this is the current vertex buffer
			// g_pLastColorMesh - this is the curent color mesh, if there is one.
			// g_pLastIndex - this is the current index buffer.
			// vertoffset : m_FirstIndex
			Assert( m_Mode == D3DPT_TRIANGLELIST || m_Mode == D3DPT_TRIANGLESTRIP );
			Assert( pPrim->m_FirstIndex >= 0 && pPrim->m_FirstIndex < g_pLastIndex->IndexCount() );
			int i;
			for( i = 0; i < 2; i++ )
			{
				CVertexBuffer *pMesh;
				if( i == 0 )
				{
					pMesh = g_pLastVertex;
					Assert( pMesh );
				}
				else
				{
					if( !g_pLastColorMesh )
					{
						continue;
					}
					pMesh = g_pLastColorMesh->m_pVertexBuffer;
					if( !pMesh )
					{
						continue;
					}
				}
				Assert( s_FirstVertex >= 0 && 
					s_FirstVertex + m_FirstIndex < pMesh->VertexCount() );
				int numIndices = 0;
				if( m_Mode == D3DPT_TRIANGLELIST )
				{
					numIndices = numPrimitives * 3;
				}
				else if( m_Mode == D3DPT_TRIANGLESTRIP )
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
					int index = g_pLastIndex->GetShadowIndex( j + pPrim->m_FirstIndex );
					Assert( index >= s_FirstVertex );
					Assert( index < s_FirstVertex + s_NumVertices );
				}
			}
#endif // CHECK_INDICES

			RECORD_COMMAND( DX8_DRAW_INDEXED_PRIMITIVE, 6 );
			RECORD_INT( m_Mode );
			RECORD_INT( s_FirstVertex );
			RECORD_INT( m_FirstIndex );
			RECORD_INT( s_NumVertices );
			RECORD_INT(	pPrim->m_FirstIndex );
			RECORD_INT( numPrimitives );

			MaterialSystemStats()->IncrementCountedStat(MATERIAL_SYSTEM_STATS_NUM_PRIMITIVES, numPrimitives );
		
			{
				VPROF( "D3DDevice()->DrawIndexedPrimitive" );
				hr = D3DDevice()->DrawIndexedPrimitive( m_Mode, m_FirstIndex, s_FirstVertex, 
					s_NumVertices, pPrim->m_FirstIndex, numPrimitives );
			}
		}

		Assert( !FAILED(hr) );
	}

	MaterialSystemStats()->IncrementCountedStat(MATERIAL_SYSTEM_STATS_NUM_INDEX_PRIMITIVE_CALLS, 1 );
}

void CMeshDX8::CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder )
{
}

void CMeshDX8::SetSoftwareVertexShader( SoftwareVertexShader_t shader )
{
	Assert( 0 );
}

//-----------------------------------------------------------------------------
//
// Dynamic mesh implementation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CDynamicMeshDX8::CDynamicMeshDX8()
{
	ResetVertexAndIndexCounts();
}

CDynamicMeshDX8::~CDynamicMeshDX8()
{
}


//-----------------------------------------------------------------------------
// Resets buffering state
//-----------------------------------------------------------------------------

void CDynamicMeshDX8::ResetVertexAndIndexCounts()
{
	m_TotalVertices = m_TotalIndices = 0;
	m_FirstIndex = m_FirstVertex = -1;
	m_HasDrawn = 0;
}


//-----------------------------------------------------------------------------
// Resets the state in case of a task switch
//-----------------------------------------------------------------------------

void CDynamicMeshDX8::Reset()
{
	m_VertexFormat = 0;
	m_pVertexBuffer = 0;
	m_pIndexBuffer = 0;
	ResetVertexAndIndexCounts();

	// Force the render state to be updated next time
	ResetRenderState();
}

//-----------------------------------------------------------------------------
// Sets the material associated with the dynamic mesh
//-----------------------------------------------------------------------------

void CDynamicMeshDX8::SetVertexFormat( VertexFormat_t format )
{
	if (ShaderAPI()->IsDeactivated())
		return;

	if ((format != m_VertexFormat) || m_VertexOverride || m_IndexOverride)
	{
		m_VertexFormat = format;
		UseVertexBuffer( g_MeshMgr.FindOrCreateVertexBuffer( format ) );
		UseIndexBuffer( g_MeshMgr.GetDynamicIndexBuffer() );
		m_VertexOverride = m_IndexOverride = false;
	}
}

void CDynamicMeshDX8::OverrideVertexBuffer( CVertexBuffer* pVertexBuffer )
{
	UseVertexBuffer( pVertexBuffer );
	m_VertexOverride = true;
}

void CDynamicMeshDX8::OverrideIndexBuffer( CIndexBuffer* pIndexBuffer )
{
	UseIndexBuffer( pIndexBuffer );
	m_IndexOverride = true;
}


//-----------------------------------------------------------------------------
// Do I need to reset the vertex format?
//-----------------------------------------------------------------------------

bool CDynamicMeshDX8::NeedsVertexFormatReset( VertexFormat_t fmt ) const
{
	return m_VertexOverride || m_IndexOverride || (m_VertexFormat != fmt);
}


//-----------------------------------------------------------------------------
// Locks/unlocks the entire mesh
//-----------------------------------------------------------------------------

bool CDynamicMeshDX8::HasEnoughRoom( int numVerts, int numIndices ) const
{
	if (ShaderAPI()->IsDeactivated())
		return false;

	// We need space in both the vertex and index buffer
	return m_pVertexBuffer->HasEnoughRoom( numVerts ) &&
		m_pIndexBuffer->HasEnoughRoom( numIndices );
}


//-----------------------------------------------------------------------------
// returns the number of indices in the mesh
//-----------------------------------------------------------------------------

int CDynamicMeshDX8::NumIndices( ) const
{
	return m_TotalIndices;
}


//-----------------------------------------------------------------------------
// Operation to do pre-lock	(only called for buffered meshes)
//-----------------------------------------------------------------------------

void CDynamicMeshDX8::PreLock()
{
	if (m_HasDrawn)
	{
		// Start again then
		ResetVertexAndIndexCounts();
	}
}

//-----------------------------------------------------------------------------
// Locks/unlocks the entire mesh
//-----------------------------------------------------------------------------

void CDynamicMeshDX8::LockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
#ifdef MEASURE_STATS
	MaterialSystemStats()->BeginTimedStat(MATERIAL_SYSTEM_STATS_MESH_BUILD_TIME);
#endif

	// Yes, this may well also be called from BufferedMesh but that's ok
	PreLock();

	if (m_VertexOverride)
		numVerts = 0;
	if (m_IndexOverride)
		numIndices = 0;

	LockVertexBuffer( numVerts, desc );
	if (m_FirstVertex < 0)
		m_FirstVertex = desc.m_FirstVertex;

	// When we're using a static index buffer, the indices assume vertices start at 0
	if (m_IndexOverride)
		desc.m_FirstVertex -= m_FirstVertex;

	// Don't add indices for points; DrawIndexedPrimitive not supported for them.
	if (m_Type != MATERIAL_POINTS)
	{
		int firstIndex = LockIndexBuffer( -1, numIndices, desc );
		if (m_FirstIndex < 0)
			m_FirstIndex = firstIndex;
	}
	else
	{
		desc.m_pIndices = 0;
	}
}


//-----------------------------------------------------------------------------
// Unlocks the mesh
//-----------------------------------------------------------------------------

void CDynamicMeshDX8::UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	m_TotalVertices += numVerts;
	m_TotalIndices += numIndices;

	if (DebugTrace())
		Spew( numVerts, numIndices, desc );

	CMeshDX8::UnlockMesh( numVerts, numIndices, desc );
}


//-----------------------------------------------------------------------------
// Draws it
//-----------------------------------------------------------------------------

void CDynamicMeshDX8::Draw( int firstIndex, int numIndices )
{
	VPROF( "CDynamicMeshDX8::Draw" );
	if ( ShaderUtil()->IsInStubMode() )
		return;

	m_HasDrawn = true;

	if (m_IndexOverride || m_VertexOverride || 
		((m_TotalVertices > 0) && (m_TotalIndices > 0)) )
	{
		Assert( !m_IsDrawing );

		// only have a non-zero first vertex when we are using static indices
		int firstVertex = m_VertexOverride ? 0 : m_FirstVertex;
		int actualFirstVertex = m_IndexOverride ? firstVertex : 0;
		int baseIndex = m_IndexOverride ? 0 : m_FirstIndex;
		if (!SetRenderState(actualFirstVertex))
			return;

		// Draws a portion of the mesh
		int numVertices = m_VertexOverride ? m_pVertexBuffer->VertexCount() : 
												m_TotalVertices;

		if ((firstIndex != -1) && (numIndices != 0))
		{
			firstIndex += baseIndex;
		}
		else
		{
			// by default we draw the whole thing
			firstIndex = baseIndex;
			if( m_IndexOverride )
			{
				numIndices = m_pIndexBuffer->IndexCount();
				Assert( numIndices != 0 );
			}
			else
			{
				numIndices = m_TotalIndices;
				Assert( numIndices != 0 );
			}
		}

		// Fix up firstVertex to indicate the first vertex used in the data
		actualFirstVertex = firstVertex - actualFirstVertex;
		
		s_FirstVertex = actualFirstVertex;
		s_NumVertices = numVertices;
		
		// Build a primlist with 1 element..
		CPrimList prim;
		prim.m_FirstIndex = firstIndex;
		prim.m_NumIndices = numIndices;
		Assert( numIndices != 0 );
		s_pPrims = &prim;
		s_nPrims = 1;

		DrawMesh();

		s_pPrims = NULL;
	}
}


//-----------------------------------------------------------------------------
// This is useful when we need to dynamically modify data; just set the
// render state and draw the pass immediately
//-----------------------------------------------------------------------------
void CDynamicMeshDX8::DrawSinglePassImmediately()
{
	if ((m_TotalVertices > 0) || (m_TotalIndices > 0))
	{
		Assert( !m_IsDrawing );

		// Set the render state
		if (SetRenderState(0))
		{
			s_FirstVertex = m_FirstVertex;
			s_NumVertices = m_TotalVertices;

			// Make a temporary PrimList to hold the indices.
			CPrimList prim( m_FirstIndex, m_TotalIndices );
			Assert( m_TotalIndices != 0 );
			s_pPrims = &prim;
			s_nPrims = 1;

			// Render it
			RenderPass();
		}

		// We're done with our data
		ResetVertexAndIndexCounts();
	}
}

void CDynamicMeshDX8::CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder )
{
	if( m_SoftwareVertexShader )
	{
		pMeshBuilder->Reset();
		IMaterialInternal *pMaterial = ShaderAPI()->GetBoundMaterial();
		Assert( pMaterial );
#ifdef _DEBUG
//		const char *matName = pMaterial->GetName();
#endif
		m_SoftwareVertexShader( *pMeshBuilder, pMaterial->GetShaderParams(), ShaderAPI() );
	}
}

void CDynamicMeshDX8::SetSoftwareVertexShader( SoftwareVertexShader_t shader )
{
	m_SoftwareVertexShader = shader;
}


//-----------------------------------------------------------------------------
//
// A mesh that stores temporary vertex data in the correct format (for modification)
//
//-----------------------------------------------------------------------------

// Used in rendering sub-parts of the mesh
unsigned int CTempMeshDX8::s_NumIndices;
unsigned int CTempMeshDX8::s_FirstIndex;

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CTempMeshDX8::CTempMeshDX8( bool isDynamic ) : m_VertexSize(0xFFFF), m_IsDynamic(isDynamic)
{
#ifdef _DEBUG
	m_Locked = false;
	m_InPass = false;
#endif
}

CTempMeshDX8::~CTempMeshDX8()
{
}


//-----------------------------------------------------------------------------
// Sets the material
//-----------------------------------------------------------------------------

void CTempMeshDX8::SetVertexFormat( VertexFormat_t format )
{
	CBaseMeshDX8::SetVertexFormat(format);
	m_VertexSize = g_MeshMgr.VertexFormatSize( format );
}


//-----------------------------------------------------------------------------
// returns the # of vertices (static meshes only)
//-----------------------------------------------------------------------------

int CTempMeshDX8::NumVertices() const
{
	return m_VertexSize ? m_VertexData.Size() / m_VertexSize : 0;
}

//-----------------------------------------------------------------------------
// returns the # of indices 
//-----------------------------------------------------------------------------

int CTempMeshDX8::NumIndices( ) const
{
	return m_IndexData.Size();
}


void CTempMeshDX8::ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
{
	Assert( !m_Locked );

	m_LockedVerts = numVerts;
	m_LockedIndices = numIndices;

	if( numVerts > 0 )
	{
		int vertexByteOffset = m_VertexSize * firstVertex;
		
		// Lock it baby
		unsigned char* pVertexMemory = &m_VertexData[vertexByteOffset];
		
		// Compute the vertex index..
		desc.m_FirstVertex = vertexByteOffset / m_VertexSize;
		
		// Set up the mesh descriptor
		g_MeshMgr.ComputeVertexDescription( pVertexMemory, m_VertexFormat, desc );
	}
	else
	{
		desc.m_FirstVertex = 0;
		// Set up the mesh descriptor
		g_MeshMgr.ComputeVertexDescription( 0, 0, desc );
	}

	if (m_Type != MATERIAL_POINTS && numIndices > 0 )
	{
		desc.m_pIndices = &m_IndexData[firstIndex];
	}
	else
	{
		desc.m_pIndices = 0;
	}

#ifdef _DEBUG
	m_Locked = true;
#endif
}


void CTempMeshDX8::ModifyEnd()
{
#ifdef _DEBUG
	Assert( m_Locked );
	m_Locked = false;
#endif
}


//-----------------------------------------------------------------------------
// Locks/unlocks the mesh
//-----------------------------------------------------------------------------

void CTempMeshDX8::LockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	Assert( !m_Locked );

	m_LockedVerts = numVerts;
	m_LockedIndices = numIndices;

	if( numVerts > 0 )
	{
		int vertexByteOffset = m_VertexData.AddMultipleToTail( m_VertexSize * numVerts );
		
		// Lock it baby
		unsigned char* pVertexMemory = &m_VertexData[vertexByteOffset];
		
		// Compute the vertex index..
		desc.m_FirstVertex = vertexByteOffset / m_VertexSize;
		
		// Set up the mesh descriptor
		g_MeshMgr.ComputeVertexDescription( pVertexMemory, m_VertexFormat, desc );
	}
	else
	{
		desc.m_FirstVertex = 0;
		// Set up the mesh descriptor
		g_MeshMgr.ComputeVertexDescription( 0, 0, desc );
	}

	if (m_Type != MATERIAL_POINTS && numIndices > 0 )
	{
		int firstIndex = m_IndexData.AddMultipleToTail( numIndices );
		desc.m_pIndices = &m_IndexData[firstIndex];
	}
	else
	{
		desc.m_pIndices = 0;
	}

#ifdef _DEBUG
	m_Locked = true;
#endif
}

void CTempMeshDX8::UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	Assert( m_Locked );

	// Remove unused vertices and indices
	int verticesToRemove = m_LockedVerts - numVerts;
	if( verticesToRemove != 0 )
	{
		m_VertexData.RemoveMultiple( m_VertexData.Size() - verticesToRemove - 1, verticesToRemove );
	}

	int indicesToRemove = m_LockedIndices - numIndices;
	if( indicesToRemove != 0 )
	{
		m_IndexData.RemoveMultiple( m_IndexData.Size() - indicesToRemove - 1, indicesToRemove );
	}

#ifdef _DEBUG
	m_Locked = false;
#endif
}


//-----------------------------------------------------------------------------
// Sets the primitive type
//-----------------------------------------------------------------------------

void CTempMeshDX8::SetPrimitiveType( MaterialPrimitiveType_t type )
{
	m_Type = type;
}

MaterialPrimitiveType_t CTempMeshDX8::GetPrimitiveType( ) const
{
	return m_Type;
}


//-----------------------------------------------------------------------------
// Gets the dynamic mesh
//-----------------------------------------------------------------------------

CDynamicMeshDX8* CTempMeshDX8::GetDynamicMesh( )
{
	return static_cast<CDynamicMeshDX8*>(g_MeshMgr.GetActualDynamicMesh( m_VertexFormat ));
}


//-----------------------------------------------------------------------------
// Draws the entire mesh
//-----------------------------------------------------------------------------

void CTempMeshDX8::Draw( int firstIndex, int numIndices )
{
	if ( ShaderUtil()->IsInStubMode() )
		return;

	if (m_VertexData.Size() > 0)
	{
		if (!ShaderAPI()->IsDeactivated())
		{
#ifdef DRAW_SELECTION
			if (0)
#else
			if (!ShaderAPI()->IsInSelectionMode())
#endif
			{
				s_FirstIndex = firstIndex;
				s_NumIndices = numIndices;

				DrawMesh( );

				// This assertion fails if a BeginPass() call was not matched by 
				// a RenderPass() call
				Assert(!m_InPass);
			}
			else
			{
				TestSelection();
			}
		}

		// Clear out the data if this temp mesh is a dynamic one...
		if (m_IsDynamic)
		{
			m_VertexData.RemoveAll();
			m_IndexData.RemoveAll();
		}
	}
}


void CTempMeshDX8::CopyToMeshBuilder( 
	int iStartVert,		// Which vertices to copy.
	int nVerts, 
	int iStartIndex,	// Which indices to copy.
	int nIndices, 
	int indexOffset,	// This is added to each index.
	CMeshBuilder &builder )
{
	int startOffset = iStartVert * m_VertexSize;
	int endOffset = (iStartVert + nVerts) * m_VertexSize;
	Assert( startOffset >= 0 && startOffset <= m_VertexData.Size() );
	Assert( endOffset >= 0 && endOffset <= m_VertexData.Size() && endOffset >= startOffset );
	if ( endOffset > startOffset )
	{
		memcpy( (void*)builder.Position(), &m_VertexData[startOffset], endOffset - startOffset );
		builder.AdvanceVertices( nVerts );
	}

	for ( int i = 0; i < nIndices; ++i )
	{
		builder.Index( m_IndexData[iStartIndex+i] + indexOffset );
		builder.AdvanceIndex();
	}		
}


//-----------------------------------------------------------------------------
// Selection mode helper functions
//-----------------------------------------------------------------------------

static void ComputeModelToView( D3DXMATRIX& modelToView )
{
	// Get the modelview matrix...
	D3DXMATRIX world, view;
	ShaderAPI()->GetMatrix( MATERIAL_MODEL, (float*)&world );
	ShaderAPI()->GetMatrix( MATERIAL_VIEW, (float*)&view );
	D3DXMatrixMultiply( &modelToView, &world, &view );
}

static float ComputeCullFactor( )
{
	D3DCULL cullMode = ShaderAPI()->GetCullMode();

	float cullFactor;
	switch(cullMode)
	{
	case D3DCULL_CCW:
		cullFactor = -1.0f;
		break;
		
	case D3DCULL_CW:
		cullFactor = 1.0f;
		break;

	default:
		cullFactor = 0.0f;
		break;
	};

	return cullFactor;
}


//-----------------------------------------------------------------------------
// Clip to viewport
//-----------------------------------------------------------------------------

static int g_NumClipVerts;
static D3DXVECTOR3 g_ClipVerts[16];

static bool PointInsidePlane( D3DXVECTOR3* pVert, int normalInd, float val, bool nearClip )
{
	if ((val > 0) || nearClip)
		return (val - (*pVert)[normalInd] >= 0);
	else
		return ((*pVert)[normalInd] - val >= 0);
}

static void IntersectPlane( D3DXVECTOR3* pStart, D3DXVECTOR3* pEnd, 
						    int normalInd, float val, D3DXVECTOR3* pOutVert )
{
	D3DXVECTOR3 dir;
	D3DXVec3Subtract( &dir, pEnd, pStart );
	Assert( dir[normalInd] != 0.0f );
	float t = (val - (*pStart)[normalInd]) / dir[normalInd];
	pOutVert->x = pStart->x + dir.x * t;
	pOutVert->y = pStart->y + dir.y * t;
	pOutVert->z = pStart->z + dir.z * t;

	// Avoid any precision problems.
	(*pOutVert)[normalInd] = val;
}

static int ClipTriangleAgainstPlane( D3DXVECTOR3** ppVert, int numVerts, 
			D3DXVECTOR3** ppOutVert, int normalInd, float val, bool nearClip = false )
{
	// Ye Olde Sutherland-Hodgman clipping algorithm
	int numOutVerts = 0;
	D3DXVECTOR3* pStart = ppVert[numVerts-1];
	bool startInside = PointInsidePlane( pStart, normalInd, val, nearClip );
	for (int i = 0; i < numVerts; ++i)
	{
		D3DXVECTOR3* pEnd = ppVert[i];
		bool endInside = PointInsidePlane( pEnd, normalInd, val, nearClip );
		if (endInside)
		{
			if (!startInside)
			{
				IntersectPlane( pStart, pEnd, normalInd, val, &g_ClipVerts[g_NumClipVerts] );
				ppOutVert[numOutVerts++] = &g_ClipVerts[g_NumClipVerts++];
			}
			ppOutVert[numOutVerts++] = pEnd;
		}
		else
		{
			if (startInside)
			{
				IntersectPlane( pStart, pEnd, normalInd, val, &g_ClipVerts[g_NumClipVerts] );
				ppOutVert[numOutVerts++] = &g_ClipVerts[g_NumClipVerts++];
			}
		}
		pStart = pEnd;
		startInside = endInside;
	}

	return numOutVerts;
}

void CTempMeshDX8::ClipTriangle( D3DXVECTOR3** ppVert, float zNear, D3DXMATRIX& projection )
{
	int i;
	int numVerts = 3;
	D3DXVECTOR3* ppClipVert1[10];
	D3DXVECTOR3* ppClipVert2[10];

	g_NumClipVerts = 0;

	// Clip against the near plane in view space to prevent negative w.
	// Clip against each plane
	numVerts = ClipTriangleAgainstPlane( ppVert, numVerts, ppClipVert1, 2, zNear, true );
	if (numVerts < 3)
		return;

	// Sucks that I have to do this, but I have to clip near plane in view space 
	// Clipping in projection space is screwy when w < 0
	// Transform the clipped points into projection space
	Assert( g_NumClipVerts <= 2 );
	for (i = 0; i < numVerts; ++i)
	{
		if (ppClipVert1[i] == &g_ClipVerts[0])
		{
			D3DXVec3TransformCoord( &g_ClipVerts[0], ppClipVert1[i], &projection ); 
		}
		else if (ppClipVert1[i] == &g_ClipVerts[1])
		{
			D3DXVec3TransformCoord( &g_ClipVerts[1], ppClipVert1[i], &projection ); 
		}
		else
		{
			D3DXVec3TransformCoord( &g_ClipVerts[g_NumClipVerts], ppClipVert1[i], &projection );
		    ppClipVert1[i] = &g_ClipVerts[g_NumClipVerts];
			++g_NumClipVerts;
		}
	}

	numVerts = ClipTriangleAgainstPlane( ppClipVert1, numVerts, ppClipVert2, 2, 1.0f );
	if (numVerts < 3)
		return;

	numVerts = ClipTriangleAgainstPlane( ppClipVert2, numVerts, ppClipVert1, 0, 1.0f );
	if (numVerts < 3)
		return;

	numVerts = ClipTriangleAgainstPlane( ppClipVert1, numVerts, ppClipVert2, 0, -1.0f );
	if (numVerts < 3)
		return;

	numVerts = ClipTriangleAgainstPlane( ppClipVert2, numVerts, ppClipVert1, 1, 1.0f );
	if (numVerts < 3)
		return;
	
	numVerts = ClipTriangleAgainstPlane( ppClipVert1, numVerts, ppClipVert2, 1, -1.0f );
	if (numVerts < 3)
		return;

#ifdef DRAW_SELECTION
	{
//	MeshDesc_t desc;
//	g_MeshMgr.ComputeVertexDescription( m_VertexData.Base(), 
//		m_pMaterial->GetVertexFormat(), desc );

	int r = rand() & 0xFF;
	int g = rand() & 0xFF;
	int b = rand() & 0xFF;

	CMeshBuilder* pMeshBuilder = ShaderAPI()->GetVertexModifyBuilder();
	IMesh* pMesh = GetDynamicMesh();
	pMeshBuilder->Begin( pMesh, MATERIAL_POLYGON, numVerts );

	D3DDevice()->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	
	for ( i = 0; i < numVerts; ++i)
	{
		pMeshBuilder->Position3fv( *ppClipVert2[i] );
//		pMeshBuilder->Color3ub( 255, 0, 0 );
		pMeshBuilder->Color3ub( r, g, b );
		pMeshBuilder->AdvanceVertex();
	}

	pMeshBuilder->End();
	pMesh->Draw();
	}
#endif

	// Compute closest and furthest verts
	float minz = ppClipVert2[0]->z;
	float maxz = ppClipVert2[0]->z;
	for ( i = 1; i < numVerts; ++i )
	{
		if (ppClipVert2[i]->z < minz)
			minz = ppClipVert2[i]->z;
		else if (ppClipVert2[i]->z > maxz)
			maxz = ppClipVert2[i]->z;
	}

	ShaderAPI()->RegisterSelectionHit( minz, maxz );
}


//-----------------------------------------------------------------------------
// Selection mode 
//-----------------------------------------------------------------------------

void CTempMeshDX8::TestSelection( )
{
	// Note that this doesn't take into account any vertex modification
	// done in a vertex shader. Also it doesn't take into account any clipping
	// done in hardware

	// Blow off points and lines; they don't matter
	if ((m_Type != MATERIAL_TRIANGLES) && (m_Type != MATERIAL_TRIANGLE_STRIP))
		return;

	D3DXMATRIX modelToView, projection;
	ComputeModelToView( modelToView );
	ShaderAPI()->GetMatrix( MATERIAL_PROJECTION, (float*)&projection );
	float zNear = -projection.m[3][2] / projection.m[2][2];

	D3DXVECTOR3* pPos[3];
	D3DXVECTOR3 edge[2];
	D3DXVECTOR3 normal;

	int numTriangles;
	if (m_Type == MATERIAL_TRIANGLES)
		numTriangles = m_IndexData.Size() / 3;
	else
		numTriangles = m_IndexData.Size() - 2;

	float cullFactor = ComputeCullFactor();

	// Makes the lovely loop simpler
	if (m_Type == MATERIAL_TRIANGLE_STRIP)
		cullFactor *= -1.0f;

	// We'll need some temporary memory to tell us if we're transformed the vert
	int numVerts = m_VertexData.Size() / m_VertexSize;
	static CUtlVector< unsigned char > transformedVert;
	int transformedVertSize = (numVerts + 7) >> 3;
	transformedVert.RemoveAll();
	transformedVert.EnsureCapacity( transformedVertSize );
	transformedVert.AddMultipleToTail( transformedVertSize );
	memset( transformedVert.Base(), 0, transformedVertSize );

#ifdef DRAW_SELECTION
	D3DXMATRIX ident;
	D3DXMatrixIdentity( &ident );
	D3DDevice()->SetTransform( D3DTS_WORLD, &ident );
	D3DDevice()->SetTransform( D3DTS_VIEW, &ident );
	D3DDevice()->SetTransform( D3DTS_PROJECTION, &ident );
#endif

	int indexPos;
	for (int i = 0; i < numTriangles; ++i)
	{
		// Get the three indices
		if (m_Type == MATERIAL_TRIANGLES)
		{
			indexPos = i * 3;
		}
		else
		{
			Assert( m_Type == MATERIAL_TRIANGLE_STRIP );
			cullFactor *= -1.0f;
			indexPos = i;
		}

		// BAH. Gotta clip to the near clip plane in view space to prevent
		// negative w coords; negative coords throw off the projection-space clipper.

		// Get the three positions in view space
		int inFrontIdx = -1;
		for (int j = 0; j < 3; ++j)
		{
			int index = m_IndexData[indexPos];
			D3DXVECTOR3* pPosition = (D3DXVECTOR3*)&m_VertexData[index * m_VertexSize];
			if ((transformedVert[index >> 3] & (1 << (index & 0x7))) == 0)
			{
				D3DXVec3TransformCoord( pPosition, pPosition, &modelToView );
				transformedVert[index >> 3] |= (1 << (index & 0x7));
			}

			pPos[j] = pPosition;
			if (pPos[j]->z < 0.0f)
				inFrontIdx = j;
			++indexPos;
		}

		// all points are behind the camera
		if (inFrontIdx < 0)
			continue;

		// backface cull....
		D3DXVec3Subtract( &edge[0], pPos[1], pPos[0] );
		D3DXVec3Subtract( &edge[1], pPos[2], pPos[0] );
		D3DXVec3Cross( &normal, &edge[0], &edge[1] );
		float dot = D3DXVec3Dot( &normal, pPos[inFrontIdx] );
		if (dot * cullFactor > 0.0f)
			continue;

		// Clip to viewport
		ClipTriangle( pPos, zNear, projection );
	}
}


//-----------------------------------------------------------------------------
// Begins a render pass
//-----------------------------------------------------------------------------
void CTempMeshDX8::BeginPass( )
{
	Assert( !m_InPass );

#ifdef _DEBUG
	m_InPass = true;
#endif

	CMeshBuilder* pMeshBuilder = ShaderAPI()->GetVertexModifyBuilder();

	CDynamicMeshDX8* pMesh = GetDynamicMesh( );

	int numIndices;
	int firstIndex;
	if ((s_FirstIndex == -1) && (s_NumIndices == 0))
	{
		numIndices = m_IndexData.Size();
		firstIndex = 0;
	}
	else
	{
		numIndices = s_NumIndices;
		firstIndex = s_FirstIndex;
	}
	
	int i;
	int numVerts = m_VertexData.Size() / m_VertexSize;
	pMeshBuilder->Begin( pMesh, m_Type, numVerts, numIndices );

	// Copy in the vertex data...
	// Note that since we pad the vertices, it's faster for us to simply
	// copy the fields we're using...
	Assert( pMeshBuilder->BaseVertexData() );

#if 0
	MeshDesc_t desc;
	g_MeshMgr.ComputeVertexDescription( m_VertexData.Base(), m_VertexFormat, desc );
	int vertexUsage = pMesh->GetVertexFormat();

//	int numTexCoords = NumTextureCoordinates( vertexUsage );
	int numBoneWeights = NumBoneWeights( vertexUsage );
	for ( i = 0; i < numVerts; ++i)
	{
		if( vertexUsage & VERTEX_POSITION )
		{
			pMeshBuilder->Position3fv( desc.m_pPosition );
			desc.m_pPosition = (float*)((unsigned char*)desc.m_pPosition + desc.m_VertexSize_Position );
		}

		if (numBoneWeights > 0)
		{
			for (int j = 0; j < numBoneWeights; ++j)
			{
				pMeshBuilder->BoneWeight( j, desc.m_pBoneWeight[j] );
			}
			desc.m_pBoneWeight = (float*)((unsigned char*)desc.m_pBoneWeight + desc.m_VertexSize_BoneWeight );
		}

		if( vertexUsage & VERTEX_NORMAL )
		{
			pMeshBuilder->Normal3fv( desc.m_pNormal );
			desc.m_pNormal = (float*)((unsigned char*)desc.m_pNormal + desc.m_VertexSize_Normal );
		}
		
		if (vertexUsage & VERTEX_COLOR)
		{
			// Arg.. gotta swizzle the color properly so Color4ub works...
			pMeshBuilder->Color4ub( desc.m_pColor[2], desc.m_pColor[1], 
									desc.m_pColor[0], desc.m_pColor[3] ); 
			desc.m_pColor += desc.m_VertexSize_Color;
		}

		int texCoordSize = TexCoordSize( SHADER_TEXTURE_STAGE0, vertexUsage );
		if (texCoordSize == 2)
		{
			pMeshBuilder->TexCoord2fv( 0, desc.m_pTexCoord[0] );
			desc.m_pTexCoord[0] = (float*)((unsigned char*)desc.m_pTexCoord[0] + desc.m_VertexSize_TexCoord[0] );
		}

		texCoordSize = TexCoordSize( SHADER_TEXTURE_STAGE1, vertexUsage );
		if (texCoordSize == 2)
		{
			pMeshBuilder->TexCoord2fv( 1, desc.m_pTexCoord[1] );
			desc.m_pTexCoord[1] = (float*)((unsigned char*)desc.m_pTexCoord[1] + desc.m_VertexSize_TexCoord[1] );
		}

		texCoordSize = TexCoordSize( SHADER_TEXTURE_STAGE2, vertexUsage );
		if (texCoordSize == 2)
		{
			pMeshBuilder->TexCoord2fv( 2, desc.m_pTexCoord[2] );
			desc.m_pTexCoord[2] = (float*)((unsigned char*)desc.m_pTexCoord[2] + desc.m_VertexSize_TexCoord[2] );
		}

		if (vertexUsage & VERTEX_TANGENT_S)
		{
			pMeshBuilder->TangentS3fv( desc.m_pTangentS ); 
			desc.m_pTangentS += desc.m_VertexSize_TangentS;
		}

		if (vertexUsage & VERTEX_TANGENT_T)
		{
			pMeshBuilder->TangentT3fv( desc.m_pTangentT ); 
			desc.m_pTangentT += desc.m_VertexSize_TangentT;
		}

		pMeshBuilder->AdvanceVertex();
	}
#else
	// Not sure which is faster...
	memcpy( pMeshBuilder->BaseVertexData(), m_VertexData.Base(), m_VertexData.Size() );
	pMeshBuilder->AdvanceVertices( m_VertexData.Count() / m_VertexSize );
#endif

	for ( i = 0; i < numIndices; ++i )
	{
		pMeshBuilder->Index( m_IndexData[firstIndex+i] );
		pMeshBuilder->AdvanceIndex();
	}

	// NOTE: The client is expected to modify the data after this call is made
	pMeshBuilder->Reset();
}


//-----------------------------------------------------------------------------
// Draws a single pass
//-----------------------------------------------------------------------------
void CTempMeshDX8::RenderPass( )
{
	Assert( m_InPass );

#ifdef _DEBUG
	m_InPass = false;
#endif

	// Have the shader API modify the vertex data as it needs
	// This vertex data is modified based on state set by the material
	ShaderAPI()->ModifyVertexData( );

	// Done building the mesh
	ShaderAPI()->GetVertexModifyBuilder()->End();

	// Have the dynamic mesh render a single pass...
	GetDynamicMesh()->DrawSinglePassImmediately();
}

void CTempMeshDX8::CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder )
{
//	Assert( 0 );
}

void CTempMeshDX8::SetSoftwareVertexShader( SoftwareVertexShader_t shader )
{
//	Assert( 0 );
}


//-----------------------------------------------------------------------------
//
// Buffered mesh implementation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CBufferedMeshDX8::CBufferedMeshDX8() : m_IsFlushing(false), m_WasRendered(true)
{
#ifdef DEBUG_BUFFERED_STATE
	m_BufferedStateSet = false;
#endif
}

CBufferedMeshDX8::~CBufferedMeshDX8()
{
}


//-----------------------------------------------------------------------------
// Sets the mesh
//-----------------------------------------------------------------------------
void CBufferedMeshDX8::SetMesh( CBaseMeshDX8* pMesh )
{
	if (m_pMesh != pMesh)
	{
		ShaderAPI()->FlushBufferedPrimitives();
		m_pMesh = pMesh;
	}
}


//-----------------------------------------------------------------------------
// Sets the material
//-----------------------------------------------------------------------------
void CBufferedMeshDX8::SetVertexFormat( VertexFormat_t format )
{
	Assert( m_pMesh );
	if (m_pMesh->NeedsVertexFormatReset(format))
	{
		ShaderAPI()->FlushBufferedPrimitives();
		m_pMesh->SetVertexFormat( format );
	}
}

VertexFormat_t CBufferedMeshDX8::GetVertexFormat( ) const
{
	Assert( m_pMesh );
	return m_pMesh->GetVertexFormat();
}

void CBufferedMeshDX8::SetMaterial( IMaterial* pMaterial )
{
#if _DEBUG
	Assert( m_pMesh );
	m_pMesh->SetMaterial( pMaterial );
#endif
}


//-----------------------------------------------------------------------------
// checks to see if it was rendered..
//-----------------------------------------------------------------------------
void CBufferedMeshDX8::ResetRendered()
{
	m_WasRendered = false;
}

bool CBufferedMeshDX8::WasNotRendered() const
{
	return !m_WasRendered;
}


//-----------------------------------------------------------------------------
// "Draws" it
//-----------------------------------------------------------------------------
void CBufferedMeshDX8::Draw( int firstIndex, int numIndices )
{
	if ( ShaderUtil()->IsInStubMode() )
		return;
	
	Assert( !m_IsFlushing && !m_WasRendered );

	// Gotta draw all of the buffered mesh
	Assert( (firstIndex == -1) && (numIndices == 0) );

	// No need to draw it more than once...
	m_WasRendered = true;

	// We've got something to flush
	m_FlushNeeded = true;

	// Less than 0 indices indicates we were using a standard buffer
	if (!ShaderUtil()->GetConfig().bBufferPrimitives)
		ShaderAPI()->FlushBufferedPrimitives();
}


//-----------------------------------------------------------------------------
// Sets the primitive mode
//-----------------------------------------------------------------------------

void CBufferedMeshDX8::SetPrimitiveType( MaterialPrimitiveType_t type )
{
	Assert( type != MATERIAL_HETEROGENOUS );

	if (type != GetPrimitiveType())
	{
		ShaderAPI()->FlushBufferedPrimitives();
		m_pMesh->SetPrimitiveType(type);
	}
}

MaterialPrimitiveType_t CBufferedMeshDX8::GetPrimitiveType( ) const
{
	return m_pMesh->GetPrimitiveType();
}


//-----------------------------------------------------------------------------
// Locks/unlocks the entire mesh
//-----------------------------------------------------------------------------
void CBufferedMeshDX8::LockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	Assert( m_pMesh );
	Assert( m_WasRendered );

	// Do some pre-lock processing
	m_pMesh->PreLock();

	// for tristrips, gotta make degenerate ones...
	m_ExtraIndices = 0;
	bool tristripFixup = (m_pMesh->NumIndices() != 0) && 
		(m_pMesh->GetPrimitiveType() == MATERIAL_TRIANGLE_STRIP);
	if (tristripFixup)
	{
		m_ExtraIndices = (m_pMesh->NumIndices() & 0x1) != 0 ? 3 : 2;
		numIndices += m_ExtraIndices;
	}

	// Flush if we gotta
	if (!m_pMesh->HasEnoughRoom(numVerts, numIndices))
		ShaderAPI()->FlushBufferedPrimitives();

	m_pMesh->LockMesh( numVerts, numIndices, desc );

	// Deal with fixing up the tristrip..
	if (tristripFixup && desc.m_pIndices)
	{
		char buf[32];
		if (DebugTrace())
		{
			if (m_ExtraIndices == 3)
				sprintf(buf,"Link Index: %d %d\n", m_LastIndex, m_LastIndex);
			else
				sprintf(buf,"Link Index: %d\n", m_LastIndex);
			OutputDebugString(buf);
		}
		*desc.m_pIndices++ = m_LastIndex;
		if (m_ExtraIndices == 3)
			*desc.m_pIndices++ = m_LastIndex;

		// Leave room for the last padding index
		++desc.m_pIndices;
	}

	m_WasRendered = false;

#ifdef DEBUG_BUFFERED_MESHES
	if (m_BufferedStateSet)
	{
		BufferedState_t compare;
		ShaderAPI()->GetBufferedState( compare );
		Assert( !memcmp( &compare, &m_BufferedState, sizeof(compare) ) );
	}
	else
	{
		ShaderAPI()->GetBufferedState( m_BufferedState );
		m_BufferedStateSet = true;
	}
#endif
}

//-----------------------------------------------------------------------------
// Locks/unlocks the entire mesh
//-----------------------------------------------------------------------------

void CBufferedMeshDX8::UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	Assert( m_pMesh );

	// Gotta fix up the first index to batch strips reasonably
	if ((m_pMesh->GetPrimitiveType() == MATERIAL_TRIANGLE_STRIP) && desc.m_pIndices)
	{
		if (m_ExtraIndices > 0)
		{
			*(desc.m_pIndices - 1) = *desc.m_pIndices;

			if (DebugTrace())
			{
				char buf[32];
				sprintf(buf,"Link Index: %d\n", *desc.m_pIndices);
				OutputDebugString(buf);
			}
		}
		
		// Remember the last index for next time 
		m_LastIndex = desc.m_pIndices[numIndices - 1];

		numIndices += m_ExtraIndices;
	}

	m_pMesh->UnlockMesh( numVerts, numIndices, desc );
}


//-----------------------------------------------------------------------------
// Renders a pass
//-----------------------------------------------------------------------------

void CBufferedMeshDX8::RenderPass()
{
	// this should never be called!
	Assert(0);
}

//-----------------------------------------------------------------------------
// Flushes queued data
//-----------------------------------------------------------------------------

void CBufferedMeshDX8::Flush( )
{
	VPROF( "CBufferedMeshDX8::Flush" );
	if (m_pMesh && (!m_IsFlushing) && m_FlushNeeded)
	{
#ifdef DEBUG_BUFFERED_MESHES
		if( m_BufferedStateSet )
		{
			BufferedState_t compare;
			ShaderAPI()->GetBufferedState( compare );
			Assert( !memcmp( &compare, &m_BufferedState, sizeof(compare) ) );
			m_BufferedStateSet = false;
		}
#endif

		m_IsFlushing = true;

		// Actually draws the data using the mesh's material
		static_cast<IMesh*>(m_pMesh)->Draw();

		m_IsFlushing = false;
		m_FlushNeeded = false;
	}
}

void CBufferedMeshDX8::CallSoftwareVertexShader( CMeshBuilder *pMeshBuilder )
{
	m_pMesh->CallSoftwareVertexShader( pMeshBuilder );
}

void CBufferedMeshDX8::SetSoftwareVertexShader( SoftwareVertexShader_t shader )
{
	m_pMesh->SetSoftwareVertexShader( shader );
}
//-----------------------------------------------------------------------------
//
// Mesh manager implementation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------

CMeshMgr::CMeshMgr() : m_pDynamicIndexBuffer(0), m_DynamicTempMesh(true)
{
}

CMeshMgr::~CMeshMgr()
{
}

//-----------------------------------------------------------------------------
// Initialize, shutdown
//-----------------------------------------------------------------------------

void CMeshMgr::Init()
{
	// The dynamic index buffer
	m_pDynamicIndexBuffer = new CIndexBuffer( D3DDevice(), INDEX_BUFFER_SIZE, true );
	m_BufferedMode = true;
}

void CMeshMgr::Shutdown()
{
	if( ShaderAPI()->IsDeactivated() )
	{
		return;
	}
	CleanUp();
}


//-----------------------------------------------------------------------------
// Task switch...
//-----------------------------------------------------------------------------
void CMeshMgr::ReleaseBuffers()
{
	if( ShaderAPI()->IsDeactivated() )
	{
		return;
	}
	
	CleanUp();
	m_DynamicMesh.Reset( );
}

void CMeshMgr::RestoreBuffers()
{
	Init();
}


//-----------------------------------------------------------------------------
// Cleans up vertex and index buffers
//-----------------------------------------------------------------------------
void CMeshMgr::CleanUp()
{
	// Necessary for cleanup
	RECORD_COMMAND( DX8_SET_STREAM_SOURCE, 3 );
	RECORD_INT( -1 );
	RECORD_INT( 0 );
	RECORD_INT( 0 );

	D3DDevice()->SetStreamSource( 0, 0, 0, 0 );

	RECORD_COMMAND( DX8_SET_STREAM_SOURCE, 3 );
	RECORD_INT( -1 );
	RECORD_INT( 1 );
	RECORD_INT( 0 );

	D3DDevice()->SetStreamSource( 1, 0, 0, 0 );

	if (m_pDynamicIndexBuffer)
	{
		delete m_pDynamicIndexBuffer;
		m_pDynamicIndexBuffer = 0;
	}

	DestroyVertexBuffers();
}


//-----------------------------------------------------------------------------
// Is the mesh dynamic?
//-----------------------------------------------------------------------------
bool CMeshMgr::IsDynamicMesh( IMesh* pMesh ) const
{
	IMesh const* pDynamicMesh = &m_DynamicMesh;
	return pMesh == pDynamicMesh;
}


//-----------------------------------------------------------------------------
// Discards the dynamic vertex and index buffer
//-----------------------------------------------------------------------------
void CMeshMgr::DiscardVertexBuffers()
{
	// This shouldn't be necessary, but it seems to be on GeForce 2
	// It helps when running WC and the engine simultaneously.
	ResetRenderState();

	if (!ShaderAPI()->IsDeactivated())
	{
		for (int i = m_DynamicVertexBuffers.Size(); --i >= 0; )
			m_DynamicVertexBuffers[i].m_pBuffer->FlushAtFrameStart();
		m_pDynamicIndexBuffer->FlushAtFrameStart();
	}
}


//-----------------------------------------------------------------------------
// Releases all dynamic vertex buffers
//-----------------------------------------------------------------------------
void CMeshMgr::DestroyVertexBuffers()
{
	for (int i = m_DynamicVertexBuffers.Count(); --i >= 0; )
	{
		if (m_DynamicVertexBuffers[i].m_pBuffer)
		{
			delete m_DynamicVertexBuffers[i].m_pBuffer;
		}
	}
	m_DynamicVertexBuffers.RemoveAll();
	m_DynamicMesh.Reset();
}


//-----------------------------------------------------------------------------
// Flushes the dynamic mesh
//-----------------------------------------------------------------------------
void CMeshMgr::Flush()
{
	m_BufferedMesh.Flush();
}


//-----------------------------------------------------------------------------
// Creates, destroys static meshes
//-----------------------------------------------------------------------------
IMesh*  CMeshMgr::CreateStaticMesh( IMaterial* pMaterial, bool bForceTempMesh )
{
	IMaterialInternal* pMatImp = static_cast<IMaterialInternal*>(pMaterial);
	CBaseMeshDX8* pMesh = (CBaseMeshDX8*)CreateStaticMesh( pMatImp->GetVertexFormat(), bForceTempMesh || pMaterial->UsesSoftwareVertexShader() );
	pMesh->SetMaterial( pMaterial );
	return pMesh;
}

IMesh*	CMeshMgr::CreateStaticMesh( VertexFormat_t format, bool bSoftwareVertexShader )
{
	if ( (format & VERTEX_PREPROCESS_DATA) || 
		bSoftwareVertexShader )
	{
		// FIXME: Use a fixed-size allocator
		CTempMeshDX8* pNewMesh = new CTempMeshDX8(false);
		pNewMesh->SetVertexFormat( format );
		return pNewMesh;
	}
	else
	{
		// FIXME: Use a fixed-size allocator
		CMeshDX8* pNewMesh = new CMeshDX8;
		pNewMesh->SetVertexFormat( format );
		return pNewMesh;
	}
}

void	CMeshMgr::DestroyStaticMesh( IMesh* pMesh )
{
	// Don't destroy the dynamic mesh!
	Assert( pMesh != &m_DynamicMesh );
	CBaseMeshDX8* pMeshImp = static_cast<CBaseMeshDX8*>(pMesh);
	if (pMeshImp)
		delete pMeshImp;
}

//-----------------------------------------------------------------------------
// Gets at the *real* dynamic mesh
//-----------------------------------------------------------------------------

IMesh* CMeshMgr::GetActualDynamicMesh( VertexFormat_t format )
{
	m_DynamicMesh.SetVertexFormat( format );
	return &m_DynamicMesh;
}

//-----------------------------------------------------------------------------
// Copy a static mesh index buffer to a dynamic mesh index buffer
//-----------------------------------------------------------------------------
void CMeshMgr::CopyStaticMeshIndexBufferToTempMeshIndexBuffer( CTempMeshDX8 *pDstIndexMesh,
															   CMeshDX8 *pSrcIndexMesh )
{
	Assert( pSrcIndexMesh->IsStatic() );
	int numIndices = pSrcIndexMesh->NumIndices();
	
	CMeshBuilder dstMeshBuilder;
	dstMeshBuilder.Begin( pDstIndexMesh, pSrcIndexMesh->GetPrimitiveType(), 0, numIndices );
	CIndexBuffer *srcIndexBuffer = pSrcIndexMesh->GetIndexBuffer();
	int dummy = 0;
	unsigned short *srcIndexArray = srcIndexBuffer->Lock( numIndices, dummy, 0 );
	int i;
	for( i = 0; i < numIndices; i++ )
	{
		dstMeshBuilder.Index( srcIndexArray[i] );
		dstMeshBuilder.AdvanceIndex();
	}
	srcIndexBuffer->Unlock( 0 );
	dstMeshBuilder.End();
}


//-----------------------------------------------------------------------------
// Gets at the dynamic mesh
//-----------------------------------------------------------------------------

IMesh*	CMeshMgr::GetDynamicMesh( IMaterial* pMaterial, bool buffered,
							IMesh* pVertexOverride, IMesh* pIndexOverride )
{
	// Can't be buffered if we're overriding the buffers
	if (pVertexOverride || pIndexOverride)
		buffered = false;

	// When going from buffered to unbuffered mode, need to flush..
	if ((m_BufferedMode != buffered) && m_BufferedMode)
	{
		m_BufferedMesh.SetMesh(0);
	}
	m_BufferedMode = buffered;

	IMaterialInternal* pMatInternal = static_cast<IMaterialInternal*>(pMaterial);

#ifdef DRAW_SELECTION
	bool needTempMesh = true;
#else
	bool needTempMesh = (pMatInternal->GetVertexUsage() & VERTEX_PREPROCESS_DATA) ||
		ShaderAPI()->IsInSelectionMode();
#endif

	CBaseMeshDX8* pMesh;
	if (needTempMesh)
	{
		// These haven't been implemented yet for temp meshes!
		// I'm not a hundred percent sure how to implement them; it would
		// involve a lock and a copy at least, which would stall the entire
		// rendering pipeline.
		Assert( !pVertexOverride );
		
		if( pIndexOverride )
		{
			CopyStaticMeshIndexBufferToTempMeshIndexBuffer( &m_DynamicTempMesh,
				( CMeshDX8 * )pIndexOverride );
		}
		pMesh = &m_DynamicTempMesh;
	}
	else
		pMesh = &m_DynamicMesh;

	if (m_BufferedMode)
	{
		Assert( !m_BufferedMesh.WasNotRendered() );
		m_BufferedMesh.SetMesh( pMesh );
		pMesh = &m_BufferedMesh;
	}

	pMesh->SetVertexFormat( pMatInternal->GetVertexFormat() );
	pMesh->SetMaterial( pMatInternal );

	// Note this works because we're guaranteed to not be using a buffered mesh
	// when we have overrides on
	// FIXME: Make work for temp meshes
	if (pMesh == &m_DynamicMesh)
	{
		CBaseMeshDX8* pBaseVertex = static_cast<CBaseMeshDX8*>(pVertexOverride);
		if (pBaseVertex)
			m_DynamicMesh.OverrideVertexBuffer( pBaseVertex->GetVertexBuffer() );

		CBaseMeshDX8* pBaseIndex = static_cast<CBaseMeshDX8*>(pIndexOverride);
		if (pBaseIndex)
			m_DynamicMesh.OverrideIndexBuffer( pBaseIndex->GetIndexBuffer() );
	}

	pMesh->SetSoftwareVertexShader( pMatInternal->GetSoftwareVertexShader() );

	return pMesh;
}


//-----------------------------------------------------------------------------
// Returns the vertex format size 
//-----------------------------------------------------------------------------

int CMeshMgr::VertexFormatSize( VertexFormat_t vertexFormat ) const
{
	// FIXME: We could make this much faster
	MeshDesc_t temp;
	ComputeVertexDescription( 0, vertexFormat, temp );
	return temp.m_ActualVertexSize;
}


//-----------------------------------------------------------------------------
// Used to construct vertex data
//-----------------------------------------------------------------------------
void CMeshMgr::ComputeVertexDescription( unsigned char* pBuffer, 
						VertexFormat_t vertexFormat, MeshDesc_t& desc ) const
{
	int i;
	int *pVertexSizesToSet[128];
	int nVertexSizesToSet = 0;
	static float dummyData[16]; // should be larger than any CMeshBuilder command can set.

	// We use fvf instead of flags here because we may pad out the fvf
	// vertex structure to optimize performance
	int offset = 0;
	if (vertexFormat & VERTEX_POSITION)
	{
		desc.m_pPosition = reinterpret_cast<float*>(pBuffer);
		offset += 3 * sizeof(float);
		pVertexSizesToSet[nVertexSizesToSet++] = &desc.m_VertexSize_Position;
	}
	else
	{
		desc.m_pPosition = dummyData;
		desc.m_VertexSize_Position = 0;
	}

	// Bone weights/matrix indices
	desc.m_NumBoneWeights = NumBoneWeights(vertexFormat);
	if (desc.m_NumBoneWeights > 0)
	{
		desc.m_pBoneWeight = reinterpret_cast<float*>(pBuffer + offset);
		offset += desc.m_NumBoneWeights * sizeof(float);
		pVertexSizesToSet[nVertexSizesToSet++] = &desc.m_VertexSize_BoneWeight;

		if (vertexFormat & VERTEX_BONE_INDEX)
		{
			desc.m_pBoneMatrixIndex = pBuffer + offset;
			offset += 4 * sizeof(unsigned char);
			pVertexSizesToSet[nVertexSizesToSet++] = &desc.m_VertexSize_BoneMatrixIndex;
		}
		else
		{
			desc.m_pBoneMatrixIndex = (unsigned char*)dummyData;
			desc.m_VertexSize_BoneMatrixIndex = 0;
		}
	}
	else
	{
		desc.m_pBoneWeight = dummyData;
		desc.m_VertexSize_BoneWeight = 0;

		desc.m_pBoneMatrixIndex = (unsigned char*)dummyData;
		desc.m_VertexSize_BoneMatrixIndex = 0;
	}

	if (vertexFormat & VERTEX_NORMAL)
	{
		desc.m_pNormal = reinterpret_cast<float*>(pBuffer + offset);
		offset += 3 * sizeof(float);
		pVertexSizesToSet[nVertexSizesToSet++] = &desc.m_VertexSize_Normal;
	}
	else
	{
		desc.m_pNormal = dummyData;
		desc.m_VertexSize_Normal = 0;
	}

	if (vertexFormat & VERTEX_COLOR)
	{
		desc.m_pColor = pBuffer + offset;
		offset += 4 * sizeof(unsigned char);
		pVertexSizesToSet[nVertexSizesToSet++] = &desc.m_VertexSize_Color;
	}
	else
	{
		desc.m_pColor = (unsigned char*)dummyData;
		desc.m_VertexSize_Color = 0;
	}

	if (vertexFormat & VERTEX_SPECULAR)
	{
		desc.m_pSpecular = pBuffer + offset;
		offset += 4 * sizeof(unsigned char);
		pVertexSizesToSet[nVertexSizesToSet++] = &desc.m_VertexSize_Specular;
	}
	else
	{
		desc.m_pSpecular = (unsigned char*)dummyData;
		desc.m_VertexSize_Specular = 0;
	}

	// Compute the number of texture coordinates
	int numTexCoords = NumTextureCoordinates(vertexFormat);
	for ( i = 0; i < numTexCoords; ++i)
	{
		int size = TexCoordSize( (TextureStage_t)i, vertexFormat );

		desc.m_pTexCoord[i] = reinterpret_cast<float*>(pBuffer + offset);
		offset += size * sizeof(float);
		pVertexSizesToSet[nVertexSizesToSet++] = &desc.m_VertexSize_TexCoord[i];
	}
	while (i < VERTEX_MAX_TEXTURE_COORDINATES)
	{
		desc.m_pTexCoord[i] = dummyData;
		desc.m_VertexSize_TexCoord[i] = 0;
		++i;
	}

	// Binormal + tangent...
	// Note we have to put these at the end so the vertex is FVF + stuff at end
	if (vertexFormat & VERTEX_TANGENT_S)
	{
		desc.m_pTangentS = reinterpret_cast<float*>(pBuffer + offset);
		offset += 3 * sizeof(float);
		pVertexSizesToSet[nVertexSizesToSet++] = &desc.m_VertexSize_TangentS;
	}
	else
	{
		desc.m_pTangentS = dummyData;
		desc.m_VertexSize_TangentS = 0;
	}

	if (vertexFormat & VERTEX_TANGENT_T)
	{
		desc.m_pTangentT = reinterpret_cast<float*>(pBuffer + offset);
		offset += 3 * sizeof(float);
		pVertexSizesToSet[nVertexSizesToSet++] = &desc.m_VertexSize_TangentT;
	}
	else
	{
		desc.m_pTangentT = dummyData;
		desc.m_VertexSize_TangentT = 0;
	}

	// User data..
	int userDataSize = UserDataSize( vertexFormat );
	if (userDataSize > 0)
	{
		desc.m_pUserData = reinterpret_cast<float*>(pBuffer + offset);
		offset += userDataSize * sizeof(float);
		pVertexSizesToSet[nVertexSizesToSet++] = &desc.m_VertexSize_UserData;
	}
	else
	{
		desc.m_pUserData = dummyData;
		desc.m_VertexSize_UserData = 0;
	}

	// We always use vertex sizes which are half-cache aligned (16 bytes)
	offset = (offset + 0xF) & (~0xF);

	desc.m_ActualVertexSize = offset;

	// Now set the m_VertexSize for all the members that were actually valid.
	Assert( nVertexSizesToSet < sizeof(pVertexSizesToSet)/sizeof(pVertexSizesToSet[0]) );
	for( int iElement=0; iElement < nVertexSizesToSet; iElement++ )
		*pVertexSizesToSet[iElement] = offset;
}


//-----------------------------------------------------------------------------
// Computes the vertex format
//-----------------------------------------------------------------------------
VertexFormat_t CMeshMgr::ComputeVertexFormat( unsigned int flags, 
			int numTexCoords, int* pTexCoordDimensions, int numBoneWeights,
			int userDataSize ) const
{
	// Construct a bitfield that makes sense and is unique from the
	// standard FVF formats
	VertexFormat_t fmt = flags;

	// This'll take 3 bits at most
	Assert( numBoneWeights <= 4 );
	fmt |= VERTEX_BONEWEIGHT(numBoneWeights);

	// Size is measured in # of floats
	Assert( userDataSize <= 4 );
	fmt |= VERTEX_USERDATA_SIZE(userDataSize);

	// FIXME: Is this assertion correct? Or should we allow 8?
	Assert( numTexCoords <= 4 );
	fmt |= VERTEX_NUM_TEXCOORDS(numTexCoords);

	for (int i = 0; i < numTexCoords; ++i)
	{
		if (pTexCoordDimensions)
			fmt |= VERTEX_TEXCOORD_SIZE( (TextureStage_t)i, pTexCoordDimensions[i] );
		else 
			fmt |= VERTEX_TEXCOORD_SIZE( (TextureStage_t)i, 2 );
	}

	return fmt;
}



//-----------------------------------------------------------------------------
// Converts draw flags into vertex format
//-----------------------------------------------------------------------------
VertexFormat_t CMeshMgr::ComputeStandardFormat( VertexFormat_t usage ) const
{
	unsigned int fVertexShader = (usage & VERTEX_FORMAT_VERTEX_SHADER);

	// Search the standard formats (from smallest to biggest) until a match is found
	int nTexCoordCount = NumTextureCoordinates(usage);
	unsigned int nVertexFlags = VertexFlags( usage ) & (~VERTEX_FORMAT_VERTEX_SHADER);
	for( int i = 0; s_pStandardFormats[i] != 0; ++i )
	{
		// Make sure it's a superset...
		if( (VertexFlags(s_pStandardFormats[i]) & nVertexFlags) != nVertexFlags )
			continue;

		if ( NumBoneWeights(s_pStandardFormats[i]) < NumBoneWeights(usage) )
			continue;

		if ( NumTextureCoordinates(s_pStandardFormats[i]) < nTexCoordCount )
			continue;

		int j = nTexCoordCount;
		while( --j >= 0 )
		{
			if( TexCoordSize(j, s_pStandardFormats[i]) < TexCoordSize(j, usage) )
				break;
		}

		if ( j >= 0 )
			continue;

		return s_pStandardFormats[i] | fVertexShader; 
	}

	// No match, just use the format as-is.
	// But report a warning
	static int s_WarningCount = 0;
	if ( s_WarningCount < 3 )
	{
		Warning("Encountered a non-standard vertex format (%X %d %d)\n",
			VertexFlags(usage), NumBoneWeights(usage), NumTextureCoordinates(usage) );
		++s_WarningCount;
	}

	return usage;
}

		 
//-----------------------------------------------------------------------------
// Returns a vertex buffer appropriate for the flags
//-----------------------------------------------------------------------------
CVertexBuffer* CMeshMgr::FindOrCreateVertexBuffer( VertexFormat_t vertexFormat )
{
	int i;
	int vertexSize = VertexFormatSize( vertexFormat );
	for (i = 0; i < m_DynamicVertexBuffers.Size(); ++i)
	{
		if (m_DynamicVertexBuffers[i].m_VertexSize == vertexSize)
		{
			return m_DynamicVertexBuffers[i].m_pBuffer;
		}
	}

	// Didn't find it, have to add it.
	i = m_DynamicVertexBuffers.AddToTail();
	m_DynamicVertexBuffers[i].m_VertexSize = vertexSize;
	m_DynamicVertexBuffers[i].m_pBuffer = new CVertexBuffer( D3DDevice(), 0, 
		vertexSize, VERTEX_BUFFER_SIZE, true ); 
	return m_DynamicVertexBuffers[i].m_pBuffer;
}

CIndexBuffer* CMeshMgr::GetDynamicIndexBuffer()
{
	return m_pDynamicIndexBuffer;
}
