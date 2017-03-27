//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CSTUDIORENDER_H
#define CSTUDIORENDER_H
#ifdef _WIN32
#pragma once
#endif

#include "istudiorender.h"
#include "studio.h"
#include "materialsystem/imaterialsystem.h" // for LightDesc_t
// wouldn't have to include these if it weren't for inlines.
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterial.h"
#include "mathlib.h"
#include "utllinkedlist.h"
#include "utlvector.h"
#include "flexrenderdata.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMaterialSystem;
class IMaterialSystemHardwareConfig;
class ITexture;
class CPixelWriter;
class CMeshBuilder;
class IMaterialVar;
struct mstudioeyeball_t;
struct eyeballstate_t;
struct lightpos_t;
struct dworldlight_t;
struct DecalClipState_t;
class CStudioRender;

extern CStudioRender g_StudioRender;

namespace OptimizedModel
{
	struct FileHeader_t;
	struct MeshHeader_t;
	struct StripGroupHeader_t;
	struct Vertex_t;
}


//-----------------------------------------------------------------------------
// Defines + structs
//-----------------------------------------------------------------------------
#define MAXLOCALLIGHTS 20

enum StudioModelLighting_t
{
	LIGHTING_HARDWARE = 0,
	LIGHTING_SOFTWARE,
	LIGHTING_MOUTH
};

struct lightpos_t
{
	Vector	delta;		// unit vector from vertex to light
	float	falloff;	// light distance falloff
	float	dot;		// light direction * delta;
};

struct eyeballstate_t
{
	const mstudioeyeball_t *peyeball;

	matrix3x4_t	mat;

	Vector	org;		// world center of eyeball
	Vector	forward;	
	Vector	right;
	Vector	up;
	
	Vector	cornea;		// world center of cornea
};


//-----------------------------------------------------------------------------
// Store decal vertex data here 
//-----------------------------------------------------------------------------
struct DecalVertex_t
{
	mstudiomesh_t *GetMesh( studiohdr_t *pHdr )
	{
		if ((m_Body == 0xFFFF) || (m_Model == 0xFFFF) || (m_Mesh == 0xFFFF))
		{
			return NULL;
		}

		mstudiobodyparts_t *pBody = pHdr->pBodypart( m_Body );
		mstudiomodel_t *pModel = pBody->pModel( m_Model );
		return pModel->pMesh( m_Mesh );
	}

	Vector m_Position;
	Vector m_Normal;
	Vector2D m_TexCoord;

	unsigned short	m_MeshVertexIndex;	// index into the mesh's vertex list
	unsigned short	m_Body;
	unsigned short	m_Model;
	unsigned short	m_Mesh;
};


//-----------------------------------------------------------------------------
// Implementation of IStudioRender
//-----------------------------------------------------------------------------
class CStudioRender : public IStudioRender
{
public:
	CStudioRender();
	virtual ~CStudioRender();

	virtual bool Init( CreateInterfaceFn materialSystemFactory, CreateInterfaceFn materialSystemHWConfigFactory );
	virtual void Shutdown( void );
	
	virtual void BeginFrame( void );
	virtual void EndFrame( void );

	virtual bool LoadModel( 
		studiohdr_t		*pStudioHdr, 	// read from the mdl file.
		void			*pVtxHdr, 		// read from the vtx file.(format OptimizedModel::FileHeader_t)
		studiohwdata_t	*pHardwareData 
		);
	
	// since everything inside of pHardwareData is allocated in studiorender.dll, must be freed there.
	virtual void UnloadModel( studiohwdata_t *pHardwareData );
	
	// This is needed to do eyeglint and calculate the correct texcoords for the eyes.
	virtual void SetEyeViewTarget( const Vector& worldPosition );
	
	virtual void UpdateConfig( const StudioRenderConfig_t& config );
	
	virtual int GetNumAmbientLightSamples();
	
	virtual const Vector *GetAmbientLightDirections();

	virtual void SetAmbientLightColors( const Vector *pAmbientOnlyColors, const Vector *pTotalColors );
	
	virtual void SetLocalLights( int numLights, const LightDesc_t *pLights );

	virtual void SetViewState( 	
		const Vector& viewOrigin,
		const Vector& viewRight,
		const Vector& viewUp,
		const Vector& viewPlaneNormal );
	
	virtual void SetFlexWeights( int numWeights, const float *pWeights );
	
	// fixme: these interfaces sucks. . use 'em to get this stuff working with the client dll
	// and then interate
	virtual matrix3x4_t* GetPoseToWorld(int i);

	virtual matrix3x4_t* GetBoneToWorld(int i);

	virtual matrix3x4_t* GetBoneToWorldArray(void);
	
	// LOD stuff
	virtual int GetNumLODs( const studiohwdata_t &hardwareData ) const;
	virtual float GetLODSwitchValue( const studiohwdata_t &hardwareData, int lod ) const;
	virtual void SetLODSwitchValue( studiohwdata_t &hardwareData, int lod, float switchValue );
	
	// Sets the color modulation
	virtual void SetColorModulation( const float* pColor );
	virtual void SetAlphaModulation( float alpha );

	// returns the number of triangles rendered.
	virtual int DrawModel(DrawModelInfo_t& info, const Vector& origin,
		int *pLodUsed, float *pMetric, int flags = STUDIORENDER_DRAW_ENTIRE_MODEL );

	virtual void ForcedMaterialOverride( IMaterial *newMaterial, OverrideType_t nOverrideType = OVERRIDE_NORMAL );

	// Create, destroy list of decals for a particular model
	virtual StudioDecalHandle_t CreateDecalList( studiohwdata_t *pHardwareData );
	virtual void DestroyDecalList( StudioDecalHandle_t handle );

	// Add decals to a decal list by doing a planar projection along the ray
	virtual void AddDecal( StudioDecalHandle_t handle, studiohdr_t *pStudioHdr, 
			const Ray_t & ray, const Vector& decalUp, IMaterial* pDecalMaterial, 
			float radius, int body, bool noPokethru, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );

	// Remove all the decals on a model
	virtual void RemoveAllDecals( StudioDecalHandle_t handle, studiohdr_t *pStudioHdr );

	// Compute the lighting at a point and normal
	virtual void ComputeLighting( const Vector* pAmbient, int lightCount,
		LightDesc_t* pLights, const Vector& pt, const Vector& normal, Vector& lighting );

	// Refresh the studiohdr since it was lost...
	virtual void RefreshStudioHdr( studiohdr_t* pStudioHdr, studiohwdata_t* pHardwareData );

	virtual void Mat_Stub( IMaterialSystem *pMatSys );

	// Shadow state (affects the models as they are rendered)
	virtual void AddShadow( IMaterial* pMaterial, void* pProxyData );
	virtual void ClearAllShadows();

	virtual int ComputeModelLod( studiohwdata_t* pHardwareData, float unitSphereSize );

	// Release/restore material system objects
	void PrecacheGlint();
	void UncacheGlint();

	// Get the config
	inline const StudioRenderConfig_t &GetConfig() { return m_Config; };
	inline bool GetSupportsHardwareLighting()	   { return m_bSupportsHardwareLighting; }
	inline bool GetSupportsVertexAndPixelShaders() { return m_bSupportsVertexAndPixelShaders; }
	inline bool GetSupportsOverbright()			   { return m_bSupportsOverbright; }

	void R_MouthComputeLightingValues( float& fIllum, Vector& forward );
	void R_MouthLighting( float fIllum, const Vector& normal, const Vector& forward, Vector& light );

	// Performs the lighting computation
	void R_ComputeLightAtPoint( const Vector &pos, const Vector &norm, Vector &color );
	void R_ComputeLightAtPoint3( const Vector &pos, const Vector &norm, Vector &color );

	void GetPerfStats( DrawModelInfo_t &info, CUtlBuffer *pSpewBuf = NULL ) const;
private:
	enum
	{
		DECAL_DYNAMIC = 0x1,
		DECAL_SECONDPASS = 0x2,
	};

	struct Decal_t
	{
		int m_IndexCount;
		int m_VertexCount;
		float m_FadeStartTime;
		float m_FadeDuration;
		int	m_Flags;
	};

	struct DecalHistory_t
	{
		unsigned short m_Material;
		unsigned short m_Decal;
	};

	typedef CUtlLinkedList<DecalVertex_t, unsigned short> DecalVertexList_t;
	typedef CUtlVector<unsigned short> DecalIndexList_t;
	typedef CUtlLinkedList<Decal_t, unsigned short> DecalList_t;
	typedef CUtlLinkedList<DecalHistory_t, unsigned short> DecalHistoryList_t;

	struct DecalMaterial_t
	{
		IMaterial*			m_pMaterial;
		DecalIndexList_t	m_Indices;
		DecalVertexList_t	m_Vertices;
		DecalList_t			m_Decals;
	};

	struct DecalLod_t
	{
		unsigned short m_FirstMaterial;
		DecalHistoryList_t	m_DecalHistory;
	};

	struct DecalModelList_t
	{
		studiohwdata_t* m_pHardwareData;
		DecalLod_t* m_pLod;
	};

	// A temporary structure used to figure out new decal verts
	struct DecalBuildVertexInfo_t
	{
		Vector2D		m_UV;
		unsigned short	m_VertexIndex;	// index into the DecalVertex_t list
		bool			m_FrontFacing;
		bool			m_InValidArea;
	};

	struct DecalBuildInfo_t
	{
		IMaterial**			m_ppMaterials;
		studiohdr_t*		m_pStudioHdr;
		mstudiomesh_t*		m_pMesh;
		studiomeshdata_t*	m_pMeshData;
		DecalMaterial_t*	m_pDecalMaterial;
		float				m_Radius;
		DecalBuildVertexInfo_t* m_pVertexInfo;
		int					m_Body;
		int					m_Model;
		int					m_Mesh;
		unsigned short		m_FirstVertex;
		unsigned short		m_VertexCount;
		bool				m_UseClipVert;
		bool				m_NoPokeThru;
		bool				m_SuppressTlucDecal;
	};

	struct ShadowState_t
	{
		IMaterial*			m_pMaterial;
		void*				m_pProxyData;
	};


private:
	int R_StudioRenderModel( int skin, int body, int hitboxset, void /*IClientEntity*/ *pEntity, 
		IMaterial **ppMaterials, int *pMaterialFlags, int flags, IMesh **ppColorMeshes = NULL );
	IMaterial* R_StudioSetupSkin( int index, IMaterial **ppMaterials, 
		void /*IClientEntity*/ *pClientEntity = NULL );
	int R_StudioDrawEyeball( mstudiomesh_t* pmesh,  studiomeshdata_t* pMeshData,
		StudioModelLighting_t lighting, IMaterial *pMaterial );
	int R_StudioDrawPoints( int skin, void /*IClientEntity*/ *pClientEntity, 
		IMaterial **ppMaterials, int *pMaterialFlags, IMesh **ppColorMeshes );
	int R_StudioDrawMesh( mstudiomesh_t* pmesh, studiomeshdata_t* pMeshData,
		  				   StudioModelLighting_t lighting, IMaterial *pMaterial, IMesh **ppColorMeshes );
	int R_StudioRenderFinal( 
		int skin, int body, int hitboxset, void /*IClientEntity*/ *pClientEntity,
		IMaterial **ppMaterials, int *pMaterialFlags, IMesh **ppColorMeshes = NULL );
	bool Mod_LoadStudioModelVertexData( studiohdr_t *pStudioHdr, void *pVtxBuffer,
									    int numStudioMeshes, 
										int	*numLODs,
										studioloddata_t	**ppLODs );
	void LoadMaterials( studiohdr_t *phdr, OptimizedModel::FileHeader_t *, studioloddata_t &lodData, int lodID );
	void UnloadMaterials( int numMaterials, IMaterial **ppMaterials );
	bool R_StudioCreateStaticMeshes(const char *pModelName, studiohdr_t *pStudioHdr, 
									OptimizedModel::FileHeader_t* pVtxHdr,
									int numStudioMeshes, studiomeshdata_t **ppStudioMeshes,
									int lodID );
	void R_StudioCreateSingleMesh(mstudiomesh_t* pMesh, 
		OptimizedModel::MeshHeader_t* pVtxMesh, int numBones, studiomeshdata_t* pMeshData,
		studiohdr_t *pStudioHdr );
	void R_StudioBuildMeshGroup( studiomeshgroup_t* pMeshGroup,
			OptimizedModel::StripGroupHeader_t *pStripGroup, mstudiomesh_t* pMesh,
			studiohdr_t *pStudioHdr );
	void R_StudioBuildMeshStrips( studiomeshgroup_t* pMeshGroup,
								OptimizedModel::StripGroupHeader_t *pStripGroup );
	bool R_AddVertexToMesh( CMeshBuilder& meshBuilder, 
		OptimizedModel::Vertex_t* pVertex, mstudiomesh_t* pMesh, bool hwSkin );
	void R_StudioDestroyStaticMeshes( int numStudioMeshes, studiomeshdata_t **ppStudioMeshes );
	int R_StudioSetupModel ( int bodypart, int entity_body, mstudiomodel_t **pSubModel, 
		studiohdr_t *pStudioHdr ) const;
	int R_StudioDrawStaticMesh( mstudiomesh_t* pmesh, 
		studiomeshgroup_t* pGroup, StudioModelLighting_t lighting, float r_blend, IMaterial* pMaterial,
		IMesh **ppColorMeshes );
	int R_StudioDrawDynamicMesh( mstudiomesh_t* pmesh, 
				studiomeshgroup_t* pGroup, StudioModelLighting_t lighting, 
				float r_blend, IMaterial* pMaterial );
	int R_StudioDrawGroupHWSkin( studiomeshgroup_t* pGroup, IMesh* pMesh, IMesh *pColorMesh = NULL );
	int R_StudioDrawGroupSWSkin( studiomeshgroup_t* pGroup, IMesh* pMesh );
	void R_StudioDrawHulls( int hitboxset, bool translucent );
	void R_StudioDrawBones (void);
	void R_StudioVertBuffer( void );
	void DrawNormal( const Vector& pos, float scale, const Vector& normal, const Vector& color );
	void BoneMatToMaterialMat( matrix3x4_t& boneMat, float materialMat[4][4] );

	// Various inner-loop methods
	void R_StudioSoftwareProcessMesh( mstudiomesh_t* pmesh, CMeshBuilder& meshBuilder, 
			int numVertices, unsigned short* pGroupToMesh, StudioModelLighting_t lighting, bool doFlex, float r_blend,
			bool bNeedsTangentSpace );

	void R_StudioSoftwareProcessMesh_Normals( mstudiomesh_t* pmesh, CMeshBuilder& meshBuilder, 
			int numVertices, unsigned short* pGroupToMesh, StudioModelLighting_t lighting, bool doFlex, float r_blend,
			bool bNeedsTangentSpace );
	void R_StudioProcessFlexedMesh( mstudiomesh_t* pmesh, CMeshBuilder& meshBuilder, 
								int numVertices, unsigned short* pGroupToMesh );
	void R_StudioRestoreMesh( mstudiomesh_t* pmesh, 
									studiomeshgroup_t* pMeshData );

	// Eye rendering using vertex shaders
	void SetEyeMaterialVars( IMaterial* pMaterial, mstudioeyeball_t* peyeball, 
		const Vector& eyeOrigin, const matrix3x4_t& irisTransform, const matrix3x4_t& glintTransform );

	void R_StudioEyeballPosition( const mstudioeyeball_t *peyeball, eyeballstate_t *pstate );

	// Computes the texture projection matrix for the glint texture
	void ComputeGlintTextureProjection( eyeballstate_t const* pState, 
		const Vector& vright, const Vector& vup, matrix3x4_t& mat );

	void R_StudioEyeballGlint( const eyeballstate_t *pstate, IMaterialVar *pGlintTextureVar, 
								const Vector& vright, const Vector& vup, const Vector& r_origin );
	void R_MouthSetupVertexShader( IMaterial* pMaterial );

	inline StudioModelLighting_t R_StudioComputeLighting( IMaterial *pMaterial, int materialFlags );
	inline void R_StudioTransform( Vector& in1, mstudioboneweight_t *pboneweight, Vector& out1 );
	inline void R_StudioRotate( Vector& in1, mstudioboneweight_t *pboneweight, Vector& out1 );
	inline void R_StudioRotate( Vector4D& in1, mstudioboneweight_t *pboneweight, Vector4D& out1 );
	inline void R_StudioEyeballNormal( mstudioeyeball_t const* peyeball, Vector& org, 
									Vector& pos, Vector& normal );
	void MaterialPlanerProjection( const matrix3x4_t& mat, int count, const Vector *psrcverts, Vector2D *pdesttexcoords );
	void R_SetPixel( CPixelWriter& pixelWriter, float* textureData, const Vector& color, float alpha );
	void R_BoxPixel( CPixelWriter& pixelWriter, float x, float y, const Vector& color );
	void AddGlint( CPixelWriter &pixelWriter, float x, float y, const Vector& color );

	// Methods associated with lighting
	inline void R_LightStrengthWorld( const Vector& vert, lightpos_t *light );
	void R_LightStrengthWorld( const Vector& vert, int lightcount, LightDesc_t* pLightDesc, lightpos_t *light );
	int R_LightGlintPosition( int index, const Vector& org, Vector& delta, Vector& intensity );
	void R_WorldLightDelta( const LightDesc_t *wl, const Vector& org, Vector& delta );
	void R_LightEffectsWorld( const lightpos_t *light, const Vector& normal, const Vector &src, Vector &dest );

public:
	// NJS: Messy, but needed for an externally optimized routine to set up the lighting.
	void R_InitLightEffectsWorld3();
	void (FASTCALL *R_LightEffectsWorld3)( const lightpos_t *light, const Vector& normal, const Vector &src, Vector &dest );
private:

	float ComputeGammaError( float f, float overbright );
	void R_ComputeFixedUpAmbientCube( Vector4D* lightBoxColor, const Vector* totalBoxColor );
	float FASTCALL R_WorldLightDistanceFalloff( const LightDesc_t *wl, const Vector& delta );
#ifdef NEW_SOFTWARE_LIGHTING
	inline float R_WorldLightAngle( const LightDesc_t *wl, const Vector& lnormal, const Vector& snormal, const Vector& delta );
#else
	float R_WorldLightAngle( const LightDesc_t *wl, const Vector& lnormal, const Vector& snormal, const Vector& delta );
#endif
	void R_LightAmbient( const Vector& normal, Vector &lv );
	void R_LightAmbient( const Vector& normal, Vector4D* pLightBoxColor, Vector &lv );

	void InitDebugMaterials( void );
	void ShutdownDebugMaterials( void );
	void R_StudioAbsBB ( int sequence, Vector& origin );
	bool PerformSoftwareLighting( ) const;
	int CalculateLOD( const Vector& modelOrigin, studiohwdata_t &hardwareData, float *pMetric );
	int SortMeshes( int* pIndices, IMaterial **ppMaterials, short* pskinref, const Vector& vforward, const Vector& r_origin );

	// Computes PoseToWorld from BoneToWorld
	void ComputePoseToWorld( studiohdr_t *pStudioHdr );
	
	// Generates a screen aligned 'billboard matrix' for the given bone index's poseToWorld transform.
	void ScreenAlignBone( studiohdr_t *pStudioHdr, int i );

	// Computes pose to decal space transforms for decal creation 
	// returns false if it can't for some reason.
	bool ComputePoseToDecal( Ray_t const& ray, const Vector& up );

	void AddDecalToModel( DecalBuildInfo_t& buildInfo );

	// returns true if we need to retire decals
	bool ShouldRetireDecal(DecalMaterial_t* pDecalMaterial, DecalHistoryList_t const& decalHistory );

	// Helper methods for decal projection, projects pose space vertex data
	// into decal space
	bool IsFrontFacing( const Vector& norm, mstudioboneweight_t *pboneweight );


	// Helper methods associated with creating decal geometry
	bool			TransformToDecalSpace( DecalBuildInfo_t& build, const Vector& pos, mstudioboneweight_t *pboneweight, Vector2D& uv );
	void			ProjectDecalOntoMesh( DecalBuildInfo_t& build );
	int				ComputeClipFlags( DecalBuildVertexInfo_t* pVertexInfo, int i );
	void			ConvertMeshVertexToDecalVertex( DecalBuildInfo_t& build, int meshIndex, DecalVertex_t& decalVertex );
	unsigned short	AddVertexToDecal( DecalBuildInfo_t& build, int meshIndex );
	unsigned short	AddVertexToDecal( DecalBuildInfo_t& build, DecalVertex_t& vert );
	void			AddClippedDecalToTriangle( DecalBuildInfo_t& build, DecalClipState_t& clipState );
	bool			ClipDecal( DecalBuildInfo_t& build, int i1, int i2, int i3, int *pClipFlags );
	void			AddTriangleToDecal( DecalBuildInfo_t& build, int i1, int i2, int i3 );
	void			AddDecalToMesh( DecalBuildInfo_t& build );
	int				GetDecalMaterial( DecalLod_t& decalLod, IMaterial* pDecalMaterial );
	int				AddDecalToMaterialList( DecalMaterial_t* pMaterial );

	// Removes a decal and associated vertices + indices from the history list
	void RetireDecal( DecalHistoryList_t& historyList );

	// Helper methods related to drawing decals
	void DrawSingleBoneDecals( CMeshBuilder& meshBuilder, DecalMaterial_t& decalMaterial );
	void DrawSingleBoneFlexedDecals( CMeshBuilder& meshBuilder, DecalMaterial_t& decalMaterial );
	void DrawMultiBoneDecals( CMeshBuilder& meshBuilder, DecalMaterial_t& decalMaterial, studiohdr_t *pStudioHdr );
	void DrawMultiBoneFlexedDecals( CMeshBuilder& meshBuilder, DecalMaterial_t& decalMaterial, studiohdr_t *pStudioHdr );
	void DrawDecalMaterial( DecalMaterial_t& decalMaterial, studiohdr_t *pStudioHdr );
	void DrawDecal( StudioDecalHandle_t handle, studiohdr_t *pStudioHdr, int lod, int body );
	
	// Draw shadows
	void DrawShadows( int body, int flags );

	// Helper methods related to flexing vertices
	void R_StudioFlexVerts( mstudiomesh_t *pmesh );

	// Avoid some warnings...
	CStudioRender( CStudioRender const& );

private:
	// Stores all decals for a particular material and lod
	CUtlLinkedList< DecalMaterial_t, unsigned short >	m_DecalMaterial;

	// Stores all decal lists that have been made
	CUtlLinkedList< DecalModelList_t, unsigned short >	m_DecalList;

	// Stores all shadows to be cast on the current object
	CUtlVector<ShadowState_t>	m_ShadowState;

	IMaterialSystem *m_pMaterialSystem;
	IMaterialSystemHardwareConfig *m_pMaterialSystemHardwareConfig;
	matrix3x4_t *m_BoneToWorld;	// bone transformation matrix
	matrix3x4_t *m_PoseToWorld;	// bone transformation matrix
	matrix3x4_t *m_PoseToDecal;	// bone transformation matrix

	studiohdr_t *m_pStudioHdr;
	mstudiomodel_t *m_pSubModel;
	studiomeshdata_t *m_pStudioMeshes;

	float m_MaterialPoseToWorld[MAXSTUDIOBONES][4][4];	
	Vector m_ViewTarget;
	float m_FlexWeights[MAXSTUDIOFLEXDESC];
	bool m_bDrawTranslucentSubModels;
	eyeballstate_t m_EyeballState[16]; // MAXSTUDIOEYEBALLS

	StudioRenderConfig_t m_Config;

	Vector m_ViewOrigin;
	Vector m_ViewRight;
	Vector m_ViewUp;
	Vector m_ViewPlaneNormal;

	Vector4D m_LightBoxColors[6];

	int m_NumLocalLights;
public:
	// Nasty hack, fix when finished with optimization:
	LightDesc_t m_LocalLights[MAXLOCALLIGHTS];
private:

	StudioRender_Printf_t Con_Printf;
	StudioRender_Printf_t Con_DPrintf;

	// debug materials
	IMaterial		*m_pMaterialMRMWireframe;
	IMaterial		*m_pMaterialMRMNormals;
	IMaterial		*m_pMaterialTranslucentModelHulls;
	IMaterialVar	*m_pMaterialTranslucentModelHulls_ColorVar;
	IMaterial		*m_pMaterialSolidModelHulls;
	IMaterialVar	*m_pMaterialSolidModelHulls_ColorVar;
	IMaterial		*m_pMaterialAdditiveVertexColorVertexAlpha;
	IMaterial		*m_pMaterialModelBones;
	IMaterial		*m_pMaterialWorldWireframe;
	IMaterial		*m_pMaterialModelEnvCubemap;

	float m_CachedGamma;
	float m_CachedTexGamma;
	float m_CachedBrightness;
	float m_CachedOverbright;

	// GLINT data
	ITexture* m_pGlintTexture;
	int m_GlintWidth;
	int m_GlintHeight;
	float* m_pGlintLinearScratch;

	float	m_ColorMod[3];
	float	m_AlphaMod;

	IMaterial	*m_pForcedMaterial;
	OverrideType_t m_nForcedMaterialType;

	// Flex data
	CCachedRenderData	m_VertexCache;

	friend class CGlintTextureRegenerator;

	// Cached variables:
	bool m_bSupportsHardwareLighting;
	bool m_bSupportsVertexAndPixelShaders;
	bool m_bSupportsOverbright;

};

//-----------------------------------------------------------------------------
// Converts matrices to a format material system wants
//-----------------------------------------------------------------------------

inline void CStudioRender::BoneMatToMaterialMat( matrix3x4_t& boneMat, float materialMat[4][4] )
{
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			materialMat[j][i] = boneMat[i][j];
		}
	}
	materialMat[0][3] = materialMat[1][3] = materialMat[2][3] = 0.0;
	materialMat[3][3] = 1.0;
}

/*
================
R_StudioTransform
================
*/
inline void CStudioRender::R_StudioTransform( Vector& in1, mstudioboneweight_t *pboneweight, Vector& out1 )
{
//	MEASURECODE( "R_StudioTransform" );

	Vector out2;
	switch( pboneweight->numbones )
	{
	case 1:
		VectorTransform( in1, m_PoseToWorld[pboneweight->bone[0]], out1 );
		break;
/*
	case 2:
		VectorTransform( in1, m_PoseToWorld[pboneweight->bone[0]], out1 );
		out1 *= pboneweight->weight[0];
		VectorTransform( in1, m_PoseToWorld[pboneweight->bone[1]], out2 );
		VectorMA( out1, pboneweight->weight[1], out2, out1 );
		break;

	case 3:
		VectorTransform( in1, m_PoseToWorld[pboneweight->bone[0]], out1 );
		out1 *= pboneweight->weight[0];
		VectorTransform( in1, m_PoseToWorld[pboneweight->bone[1]], out2 );
		VectorMA( out1, pboneweight->weight[1], out2, out1 );
		VectorTransform( in1, m_PoseToWorld[pboneweight->bone[2]], out2 );
		VectorMA( out1, pboneweight->weight[2], out2, out1 );
		break;
*/
	default:
		VectorFill( out1, 0 );
		for (int i = 0; i < pboneweight->numbones; i++)
		{
			VectorTransform( in1, m_PoseToWorld[pboneweight->bone[i]], out2 );
			VectorMA( out1, pboneweight->weight[i], out2, out1 );
		}
		break;
	}
}


/*
================
R_StudioRotate
================
*/
inline void CStudioRender::R_StudioRotate( Vector& in1, mstudioboneweight_t *pboneweight, Vector& out1 )
{
	// NOTE: This only works to rotate normals if there's no scale in the
	// pose to world transforms. If we ever add scale, we'll need to
	// multiply by the inverse transpose of the pose to world

	if (pboneweight->numbones == 1)
	{
		VectorRotate( in1, m_PoseToWorld[pboneweight->bone[0]], out1 );
	}
	else
	{
		Vector out2;

		VectorFill( out1, 0 );

		for (int i = 0; i < pboneweight->numbones; i++)
		{
			VectorRotate( in1, m_PoseToWorld[pboneweight->bone[i]], out2 );
			VectorMA( out1, pboneweight->weight[i], out2, out1 );
		}
		VectorNormalize( out1 );
	}
}

inline void CStudioRender::R_StudioRotate( Vector4D& realIn1, mstudioboneweight_t *pboneweight, Vector4D& realOut1 )
{
	// garymcthack - god this sucks.
	Vector in1( realIn1[0], realIn1[1], realIn1[2] );
	Vector out1;
	if (pboneweight->numbones == 1)
	{
		VectorRotate( in1, m_PoseToWorld[pboneweight->bone[0]], out1 );
	}
	else
	{
		Vector out2;

		VectorFill( out1, 0 );

		for (int i = 0; i < pboneweight->numbones; i++)
		{
			VectorRotate( in1, m_PoseToWorld[pboneweight->bone[i]], out2 );
			VectorMA( out1, pboneweight->weight[i], out2, out1 );
		}
		VectorNormalize( out1 );
	}
	realOut1.Init( out1[0], out1[1], out1[2], realIn1[3] );
}

//-----------------------------------------------------------------------------
// Figures out what kind of lighting we're gonna want
//-----------------------------------------------------------------------------

inline bool CStudioRender::PerformSoftwareLighting( ) const
{
	return m_Config.bSoftwareLighting ||
		(!m_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() &&
		 !m_pMaterialSystemHardwareConfig->SupportsHardwareLighting());
}

//-----------------------------------------------------------------------------
// Figures out what kind of lighting we're gonna want
//-----------------------------------------------------------------------------
inline StudioModelLighting_t CStudioRender::R_StudioComputeLighting( IMaterial *pMaterial, int materialFlags )
{
	// Here, we only do software lighting when the following conditions are met.
	// 1) The material is vertex lit and we don't have hardware lighting
	// 2) We're drawing an eyeball
	// 3) We're drawing mouth-lit stuff

	// FIXME: When we move software lighting into the material system, only need to
	// test if it's vertex lit
	assert( pMaterial );
	bool doMouthLighting = materialFlags && (m_pStudioHdr->nummouths >= 1);

	bool doSoftwareLighting = doMouthLighting ||
		(pMaterial->IsVertexLit() && ( PerformSoftwareLighting() || pMaterial->NeedsSoftwareLighting() ) );

	StudioModelLighting_t lighting = LIGHTING_HARDWARE;
	if (doMouthLighting)
		lighting = LIGHTING_MOUTH;
	else if (doSoftwareLighting)
		lighting = LIGHTING_SOFTWARE;

	return lighting;
}

//-----------------------------------------------------------------------------
// Lighting method
//-----------------------------------------------------------------------------

inline void CStudioRender::R_LightStrengthWorld( const Vector& vert, lightpos_t *light )
{
	R_LightStrengthWorld( vert, m_NumLocalLights, m_LocalLights, light );
}

//-----------------------------------------------------------------------------
// Compute the contribution of a light depending on it's angle
//-----------------------------------------------------------------------------
/*
  light_normal (lights normal translated to same space as other normals)
  surface_normal
  light_direction_normal | (light_pos - vertex_pos) |
*/
#ifdef NEW_SOFTWARE_LIGHTING

template< int nLightType >
class CWorldLightAngleWrapper
{
public:
	FORCEINLINE static float WorldLightAngle( const LightDesc_t *wl, const Vector& lnormal, const Vector& snormal, const Vector& delta )
	{
		float dot, dot2, ratio;

		switch (nLightType)	
		{
			case MATERIAL_LIGHT_POINT:
				dot = DotProduct( snormal, delta );
				if (dot < 0.f)
					return 0.f;
				return dot;

			case MATERIAL_LIGHT_SPOT:
				dot = DotProduct( snormal, delta );
				if (dot < 0.f)
					return 0.f;

				dot2 = -DotProduct (delta, lnormal);
				if (dot2 <= wl->m_PhiDot)
					return 0.f; // outside light cone

				ratio = dot;
				if (dot2 >= wl->m_ThetaDot)
					return ratio;	// inside inner cone

				if ((wl->m_Falloff == 1.f) || (wl->m_Falloff == 0.f))
				{
					ratio *= (dot2 - wl->m_PhiDot) / (wl->m_ThetaDot - wl->m_PhiDot);
				}
				else
				{
					ratio *= pow((dot2 - wl->m_PhiDot) / (wl->m_ThetaDot - wl->m_PhiDot), wl->m_Falloff );
				}
				return ratio;

			case MATERIAL_LIGHT_DIRECTIONAL:
				dot2 = -DotProduct( snormal, lnormal );
				if (dot2 < 0.f)
					return 0.f;
				return dot2;

			case MATERIAL_LIGHT_DISABLE:
				return 0.f;

			NO_DEFAULT;
		} 
	}
};

inline float CStudioRender::R_WorldLightAngle( const LightDesc_t *wl, const Vector& lnormal, const Vector& snormal, const Vector& delta )
{
	switch (wl->m_Type) 
	{
		case MATERIAL_LIGHT_DISABLE:		return CWorldLightAngleWrapper<MATERIAL_LIGHT_DISABLE>::WorldLightAngle( wl, lnormal, snormal, delta );
		case MATERIAL_LIGHT_POINT:			return CWorldLightAngleWrapper<MATERIAL_LIGHT_POINT>::WorldLightAngle( wl, lnormal, snormal, delta );
		case MATERIAL_LIGHT_DIRECTIONAL:	return CWorldLightAngleWrapper<MATERIAL_LIGHT_DIRECTIONAL>::WorldLightAngle( wl, lnormal, snormal, delta );
		case MATERIAL_LIGHT_SPOT:			return CWorldLightAngleWrapper<MATERIAL_LIGHT_SPOT>::WorldLightAngle( wl, lnormal, snormal, delta );
		NO_DEFAULT;
	}
}
#endif


//-----------------------------------------------------------------------------
// Computes the ambient term
//-----------------------------------------------------------------------------
inline void CStudioRender::R_LightAmbient( const Vector& normal, Vector &lv )
{
	R_LightAmbient( normal, m_LightBoxColors, lv );
}


//-----------------------------------------------------------------------------
// Draws eyeballs
//-----------------------------------------------------------------------------
inline void CStudioRender::R_StudioEyeballNormal( mstudioeyeball_t const* peyeball, Vector& org, 
									Vector& pos, Vector& normal )
{
	// inside of a flattened torus
	VectorSubtract( pos, org, normal );
	float flUpAmount =  DotProduct( normal, peyeball->up );
	VectorMA( normal, -0.5 * flUpAmount, peyeball->up, normal );
	VectorNormalize( normal );
}

#endif // CSTUDIORENDER_H
