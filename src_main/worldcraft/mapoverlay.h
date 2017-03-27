//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPOVERLAY_H
#define MAPOVERLAY_H
#pragma once

#include <afxwin.h>
#include <assert.h>
#include "UtlVector.h"
#include "MapSideList.h"

class CHelperInfo;
class CMapFace;
class CRender3D;
class IEditorTexture;

#define OVERLAY_HANDLES_COUNT	4

#define NUM_CLIPFACE_TEXCOORDS	2


//=============================================================================
//
// Class Map Overlay
//
class CMapOverlay : public CMapSideList
{
public:

	DECLARE_MAPCLASS( CMapOverlay );

	//=========================================================================
	//
	// Construction/Deconstruction
	//
	CMapOverlay();
	~CMapOverlay();

	// Factory for building from a list of string parameters.
	static CMapClass *CreateMapOverlay( CHelperInfo *pInfo, CMapEntity *pParent );

	virtual void OnAddToWorld( CMapWorld *pWorld );
	virtual void OnRemoveFromWorld( CMapWorld *pWorld, bool bNotifyChildren );
	virtual void PostloadWorld( CMapWorld *pWorld );

	//=========================================================================
	//
	// Operations
	//
	virtual CMapClass *Copy( bool bUpdateDependencies );
	virtual CMapClass *CopyFrom( CMapClass *pObject, bool bUpdateDependencies );

	void CalcBounds( BOOL bFullUpdate = FALSE );

	virtual void OnParentKeyChanged( LPCSTR szKey, LPCSTR szValue );
	virtual void OnNotifyDependent( CMapClass *pObject, Notify_Dependent_t eNotifyType );

	void DoTransform( Box3D *t );
	void DoTransMove( const Vector &Delta );
	void DoTransRotate( const Vector *pRefPoint, const QAngle &Angles );
	void DoTransScale( const Vector &RefPoint, const Vector &Scale );
	void DoTransFlip( const Vector &RefPoint );

	void OnPaste( CMapClass *pCopy, CMapWorld *pSourceWorld, CMapWorld *pDestWorld, 
				  CMapObjectList &OriginalList, CMapObjectList &NewList);
	void OnClone( CMapClass *pClone, CMapWorld *pWorld, 
				  CMapObjectList &OriginalList, CMapObjectList &NewList );
	void OnUndoRedo( void );

	void Render3D( CRender3D *pRender );

	void HandlesReset( void );
	bool HandlesHitTest( CPoint const &pt );
	void HandlesDragTo( Vector &vecImpact, CMapFace *pFace );
	
	void DoClip( void );
	void ReBuild( void );

	void GetHandlePos( int iHandle, Vector &vecPos );
	void GetPlane( cplane_t &plane );

	int	GetFaceCount( void ) { return m_Faces.Count(); }
	CMapFace *GetFace( int iFace ) { return m_Faces.Element( iFace ); }
	int *SetFace( CMapFace *pFace );

	void AddInitialFaceToSideList( CMapFace *pFace );

	//=========================================================================
	//
	// Attributes
	//
	inline virtual bool IsVisualElement( void ) { return true; }
	inline virtual bool ShouldRenderLast( void ) { return true; }
	inline LPCTSTR GetDescription() { return ( "Overlay" ); }

private:

	//=========================================================================
	//
	// Basis Data
	//
	struct Basis_t
	{
		CMapFace	*m_pFace;					// Index to the face the basis were derived from	
		Vector		m_vecOrigin;				// Origin of basis vectors (in plane)
		Vector		m_vecAxes[3];				// Basis vectors	
		Vector		m_vecCacheAxes[3];			// Possible rotated basis
	};

	void BasisInit( void );
	void BasisBuild( CMapFace *pFace );
	void BasisBuildFromSideList( void );
	void BasisReBuild( void );
	void BasisSetOrigin( void );
	void BasisSetInitialUAxis( Vector const &vecNormal );
	void BasisBuildCached( void );
	void BasisCopy( Basis_t *pSrc, Basis_t *pDst );

	//=========================================================================
	//
	// Handle Data
	//		
	struct Handles_t
	{
		int			m_iHit;									 // Index of the selected handle
		Vector2D	m_vecBasisCoords[OVERLAY_HANDLES_COUNT]; // U,V coordinates of the 4 corners in the editable plane (use basis)
		Vector		m_vec3D[OVERLAY_HANDLES_COUNT];			 // World space handles for snap testing
		Vector2D	m_vec2D[OVERLAY_HANDLES_COUNT];			 // Screen space handles for hit testing
	};
		
	void HandlesInit( void );
	void HandlesBuild( void );
	void HandlesBuild2D( CRender3D *pRender );
	void HandlesRender( CRender3D *pRender );
	void HandlesRender2D( CRender3D *pRender, Vector2D const &vScreenPt, unsigned char *pColor );
	void HandlesRotate( QAngle const &vAngles );
	void HandlesFlip( Vector const &vRefPoint );	
	void HandlesSurfToOverlayPlane( CMapFace *pFace, Vector const &vSurf, Vector &vPoint );
	void HandlesOverlayPlaneToSurf( CMapFace *pFace, Vector const &vPoint, Vector &vSurf );
	void HandlesUpdatePropertyBox( void );
	void HandlesCopy( Handles_t *pSrc, Handles_t *pDst );
	void HandlesBounds( Vector &vecMin, Vector &vecMax );

	//=========================================================================
	//
	// Clip Face Data
	//		
	struct BlendData_t
	{
		int		m_nType;			// type of blend (point, edge, barycentric)
		short	m_iPoints[3];		// displacement point indices
		float	m_flBlends[3];		// blending values
	};

	struct ClipFace_t
	{
		CMapFace					*m_pBuildFace;
		int							m_nPointCount;
		CUtlVector<Vector>			m_aPoints;
		CUtlVector<Vector>			m_aDispPointUVs;		// z is always 0 (need to be this way to share functions!)
		CUtlVector<Vector2D>		m_aTexCoords[NUM_CLIPFACE_TEXCOORDS];
		CUtlVector<BlendData_t>		m_aBlends;

		~ClipFace_t()				{ int blah = 3; };
	};

	typedef CUtlVector<ClipFace_t*> ClipFaces_t;
	
	ClipFace_t *ClipFaceCreate( int nSize );
	void ClipFaceDestroy( ClipFace_t **ppClipFace );
	void ClipFaceGetBounds( ClipFace_t *pClipFace, Vector &vecMin, Vector &vecMax );
	void ClipFaceClip( ClipFace_t *pClipFace, cplane_t *pClipPlane, float flEpsilon, ClipFace_t **ppFront, ClipFace_t **ppBack );
	void ClipFaceClipBarycentric( ClipFace_t *pClipFace, cplane_t *pClipPlane, float flEpsilon, int iClip, CMapDisp *pDisp, ClipFace_t **ppFront, ClipFace_t **ppBack );
	void ClipFacePreClipDisp( ClipFace_t *pClipFace, CMapDisp *pDisp );
	bool ClipFaceCalcBarycentricCooefs( CMapDisp *pDisp, Vector2D *pVertsUV, const Vector2D &vecPointUV, float *pCoefs );
	void ResolveBarycentricClip( CMapDisp *pDisp, ClipFace_t *pClipFace, int iClipFacePoint, const Vector2D &vecPointUV, float *pCoefs, int *pTris, Vector2D *pVertsUV );
	void ClipFacePostClipDisp( void );
	void ClipFaceBuildBlend( ClipFace_t *pClipFace, CMapDisp *pDisp, cplane_t *pClipPlane, int iClip, const Vector &vecUV, const Vector &vecPoint );
	int  ClipFaceGetAxisType( cplane_t *pClipPlane );
	void ClipFaceCopyBlendFrom( ClipFace_t *pClipFace, BlendData_t *pBlendFrom );
	void ClipFaceBuildFacesFromBlendedData( ClipFace_t *pClipFace );
	ClipFace_t *ClipFaceCopy( ClipFace_t *pSrc );

	void GetTriVerts( CMapDisp *pDisp, const Vector2D &vecSurfUV, int *pTris, Vector2D *pVertsUV );

	void ClipFacesDestroy( ClipFaces_t &aClipFaces );

	//=========================================================================
	//
	// Material Functions
	//
	struct Material_t
	{
		IEditorTexture				*m_pTexture;		// material
		Vector2D					m_vecTexCoords[4];	// texture coordinates 
		Vector2D					m_vecTextureU;		// material starting and ending U
		Vector2D					m_vecTextureV;		// material starting and ending V
	};

	void MaterialInit( void );
	void MaterialUpdateTexCoordsFromUVs( void );
	void MaterialCopy( Material_t *pSrc, Material_t *pDst );

	//=========================================================================
	//
	//
	//
	bool BuildEdgePlanes( Vector const *pPoints, int pointCount, cplane_t *pEdgePlanes, int edgePlaneCount );
	void UpdateParentEntityOrigin( void );

	// Clipping
	void PreClip( void );
	void PostClip( void );
	void DoClipFace( CMapFace *pFace );
	void DoClipDisp( CMapFace *pFace, ClipFace_t *pClippedFace );
	void DoClipDispInV( CMapDisp *pDisp, ClipFaces_t &aCurrentFaces );
	void DoClipDispInU( CMapDisp *pDisp, ClipFaces_t &aCurrentFaces );
	void DoClipDispInUVFromTLToBR( CMapDisp *pDisp, ClipFaces_t &aCurrentFaces );
	void DoClipDispInUVFromBLToTR( CMapDisp *pDisp, ClipFaces_t &aCurrentFaces );

	void Disp_ClipFragments( CMapDisp *pDisp, ClipFaces_t &aDispFragments );
	void Disp_DoClip( CMapDisp *pDisp, ClipFaces_t &aDispFragments, cplane_t &clipPlane, float clipDistStart, int nInterval, int nLoopStart, int nLoopEnd, int nLoopInc );


	// Utility
	void OverlayUVToOverlayPlane( Vector2D const &vecUV, Vector &vecPoint );
	void OverlayPlaneToOverlayUV( Vector const &vecPoint, Vector2D &vecUV );
	void WorldToOverlayPlane( Vector const &vecWorld, Vector &vecPoint );
	void OverlayPlaneToBaseFacePlane( CMapFace *pFace, Vector const &vecPoint, Vector &vecBasePoint );
	void OverlayPlaneToWorld( CMapFace *pFace, const Vector &vecPlane, Vector &vecWorld );

	void PostModified( void );

private:

	Basis_t			m_Basis;			// Overlay Basis Data
	Handles_t		m_Handles;			// Overlay Handle Data

	Material_t		m_Material;			// Overlay Material

	ClipFace_t		*m_pOverlayFace;	// Primary Overlay
	ClipFaces_t		m_aRenderFaces;		// Clipped Face Cache (Render Faces)
};


#endif // MAPOVERLAY_H
