//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef STUDIOMODEL_H
#define STUDIOMODEL_H
#ifdef _WIN32
#pragma once
#endif

#ifndef byte
typedef unsigned char byte;
#endif // byte

#include "worldcraft_mathlib.h"
#include "studio.h"
#include "UtlVector.h"


class StudioModel;
class CMaterial;
class IStudioRender;
class CRender3D;

typedef struct
{
	StudioModel *pModel;
	char *pszPath;
	int nRefCount;
} ModelCache_t;


//-----------------------------------------------------------------------------
// Purpose: Defines an interface to a cache of studio models.
//-----------------------------------------------------------------------------
class CStudioModelCache
{
	public:

		static StudioModel *CreateModel(const char *pszModelPath);
		static void AddRef(StudioModel *pModel);
		static void Release(StudioModel *pModel);
		static void AdvanceAnimation(float flInterval);

	protected:

		static BOOL AddModel(StudioModel *pModel, const char *pszModelPath);
		static void RemoveModel(StudioModel *pModel);

		static ModelCache_t m_Cache[1024];
		static int m_nItems;
};


class StudioModel
{
public:

	static bool				Initialize( void );
	static void				Shutdown( void ); // garymcthack - need to call this.
	static void				UpdateStudioRenderConfig( bool bFlat, bool bWireframe );
	static void				UpdateViewState( const Vector& viewOrigin,
											 const Vector& viewRight,
											 const Vector& viewUp,
											 const Vector& viewPlaneNormal );
	
	StudioModel(void);
	~StudioModel(void);

	void					FreeModel ();
	bool					LoadModel( const char *modelname );
	bool					PostLoadModel ( const char *modelname );
	void					DrawModel( CRender3D *pRender );
	void					AdvanceFrame( float dt );

	void					ExtractBbox( Vector &mins, Vector &maxs );
	void					ExtractClippingBbox( Vector &mins, Vector &maxs );
	void					ExtractMovementBbox( Vector &mins, Vector &maxs );
	void					RotateBbox( Vector &Mins, Vector &Maxs, const QAngle &Angles );

	int						SetSequence( int iSequence );
	int						GetSequence( void );
	int						GetSequenceCount( void );
	void					GetSequenceName( int nIndex, char *szName );
	void					GetSequenceInfo( float *pflFrameRate, float *pflGroundSpeed );

	int						SetBodygroup( int iGroup, int iValue );
	int						SetSkin( int iValue );
	void					SetOrigin( float x, float y, float z );
	void					SetAngles( QAngle& pfAngles );

	bool					IsTranslucent();

	static void				ReleaseStudioModels();
	static void				RestoreStudioModels();

private:
	// Release, restore static mesh in case of task switch
	void ReleaseStaticMesh();
	void RestoreStaticMesh();

	// entity settings
	Vector					m_origin;
	QAngle					m_angles;	
	int						m_sequence;			// sequence index
	float					m_cycle;			// pos in animation cycle
	int						m_bodynum;			// bodypart selection	
	int						m_skinnum;			// skin group selection
	byte					m_controller[4];	// bone controllers
	float					m_poseParameter[4];		// animation blending
	byte					m_mouth;			// mouth position
	char*					m_pModelName;		// model file name

	// internal data
	studiohdr_t				*m_pStudioHdr;
	mstudiomodel_t			*m_pModel;	
	studiohwdata_t			m_HardwareData;	

	// class data
	static IStudioRender	*m_pStudioRender;
	
	void					SetUpBones ( void );

	void					SetupLighting( void );

	void					SetupModel ( int bodypart );

	void					LoadStudioRender( void );

	// Used to release/restore static meshes
	static CUtlVector<StudioModel*>	s_StudioModels;
};


extern Vector g_vright;		// needs to be set to viewer's right in order for chrome to work
extern float g_lambert;		// modifier for pseudo-hemispherical lighting


#endif // STUDIOMODEL_H
