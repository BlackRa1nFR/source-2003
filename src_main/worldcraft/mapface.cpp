//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "stdafx.h"
#include "MapDefs.h"
#include "MapFace.h"
#include "MapDisp.h"
#include "MapWorld.h"
#include "KeyValues.h"
#include "GameData.h"
#include "GlobalFunctions.h"
#include "Render3D.h"
#include "SaveInfo.h"
#include "TextureSystem.h"
#include "MapDoc.h"
#include "materialsystem/IMesh.h"
#include "Material.h"
#include "UtlRBTree.h"
#include <Vector.h>


//#define DEBUGPTS


#define TEXTURE_AXIS_LENGTH				10			// Rendered texture axis length in world units.

//
// Components of the texture axes are rounded to integers within this tolerance. This tolerance corresponds
// to an angle of about 0.06 degrees.
//
#define TEXTURE_AXIS_ROUND_EPSILON		0.001		


//
// For passing into LoadKeyCallback. Collects key value data while loading.
//
struct LoadFace_t
{
	CMapFace *pFace;
	char szTexName[MAX_PATH];
};


BOOL CheckFace(Vector *Points, int nPoints, Vector *normal, float dist, CCheckFaceInfo *pInfo);
LPCTSTR GetDefaultTextureName();


#pragma warning(disable:4244)


//
// Static member data initialization.
//
bool CMapFace::m_bShowFaceSelection = true;
IEditorTexture *CMapFace::m_pLightmapGrid = NULL;


//-----------------------------------------------------------------------------
// Purpose: Determines whether a point is inside of a bounding box.
// Input  : pfPoint - The point.
//			pfMins - The box mins.
//			pfMaxs - The box maxes.
// Output : Returns TRUE if the point is inside the box, FALSE if not.
//-----------------------------------------------------------------------------
static bool PointInBox(Vector& pfPoint, Vector& pfMins, Vector& pfMaxs)
{
	if ((pfPoint[0] < pfMins[0]) || (pfPoint[0] > pfMaxs[0]))
	{
		return(false);
	}

	if ((pfPoint[1] < pfMins[1]) || (pfPoint[1] > pfMaxs[1]))
	{
		return(false);
	}

	if ((pfPoint[2] < pfMins[2]) || (pfPoint[2] > pfMaxs[2]))
	{
		return(false);
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor. Initializes data members and sets the texture to the
//			default texture.
//-----------------------------------------------------------------------------
CMapFace::CMapFace(void)
{
	memset(&texture, 0, sizeof(texture));
	memset(&plane, 0, sizeof(plane));

	m_pTexture = NULL;
	m_pTangentAxes = NULL;
    m_DispHandle = EDITDISPHANDLE_INVALID;

	Points = NULL;
	nPoints = 0;
	m_nFaceID = 0;
	m_pTextureCoords = NULL;
	m_pLightmapCoords = NULL;
	m_bIgnored = FALSE;
	m_uchAlpha = 255;

	texture.nLightmapScale = g_pGameConfig->GetDefaultLightmapScale();

	texture.scale[0] = g_pGameConfig->GetDefaultTextureScale();
	texture.scale[1] = g_pGameConfig->GetDefaultTextureScale();

	SetTexture(GetDefaultTextureName());
	
	if (m_pLightmapGrid == NULL)
	{
		m_pLightmapGrid = g_Textures.FindActiveTexture("Debug/debugluxelsnoalpha");
	}

	m_bIgnoreLighting = false;
	m_fSmoothingGroups = SMOOTHING_GROUP_DEFAULT;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Frees points and texture coordinates.
//-----------------------------------------------------------------------------
CMapFace::~CMapFace(void)
{
	delete [] Points;
	Points = NULL;

	delete [] m_pTextureCoords;
	m_pTextureCoords = NULL;

	delete [] m_pLightmapCoords;
	m_pLightmapCoords = NULL;

	FreeTangentSpaceAxes();

	if( HasDisp() )
	{
		IWorldEditDispMgr *pDispMgr = GetActiveWorldEditDispManager();
		if( !pDispMgr )
			return;

		pDispMgr->RemoveFromWorld( GetDisp() );
		EditDispMgr()->Destroy( GetDisp() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to fix this face. This is called by the check for problems
//			code when a face is reported as invalid.
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapFace::Fix(void)
{
	CalcPlane();
	CalcTextureCoords();

	return(CheckFace());
}


//-----------------------------------------------------------------------------
// Purpose: Returns the short texture name in 'pszName'. Places an empty string
//			in 'pszName' if the face has no texture.
//-----------------------------------------------------------------------------
void CMapFace::GetTextureName(char *pszName) const
{
	ASSERT(pszName != NULL);

	if (pszName != NULL)
	{
		if (m_pTexture != NULL)
		{
			m_pTexture->GetShortName(pszName);
		}
		else
		{
			pszName[0] = '\0';
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Populates this face with another face's information.
// Input  : pFrom - The face to copy.
// Output : CMapFace
//-----------------------------------------------------------------------------
CMapFace *CMapFace::CopyFrom(const CMapFace *pObject, DWORD dwFlags, bool bUpdateDependencies)
{
	const CMapFace *pFrom = dynamic_cast<const CMapFace *>(pObject);
	ASSERT(pFrom != NULL);

	if (pFrom != NULL)
	{
		//
		// Free our points first.
		//
		if (Points != NULL)
		{
			delete [] Points;
			Points = NULL;
		}

		if (m_pTextureCoords != NULL)
		{
			delete [] m_pTextureCoords;
			m_pTextureCoords = NULL;
		}

		if (m_pLightmapCoords != NULL)
		{
			delete [] m_pLightmapCoords;
			m_pLightmapCoords = NULL;
		}

		FreeTangentSpaceAxes();

		nPoints = 0;

		//
		// Copy the member data.
		//
		m_nFaceID = pFrom->m_nFaceID;
		m_bIgnored = pFrom->IsIgnored();
		m_eSelectionState = pFrom->GetSelectionState();
		texture = pFrom->texture;
		m_pTexture = pFrom->m_pTexture;

		//
		// Allocate points memory.
		//
		if (dwFlags & COPY_FACE_POINTS)
		{
			Points = NULL;
			nPoints = pFrom->nPoints;

			if (pFrom->Points && nPoints)
			{
				AllocatePoints(nPoints);
				AllocTangentSpaceAxes( nPoints );
				memcpy(Points, pFrom->Points, sizeof(Vector) * nPoints);
				memcpy(m_pTextureCoords, pFrom->m_pTextureCoords, sizeof(Vector2D) * nPoints);
				memcpy(m_pLightmapCoords, pFrom->m_pLightmapCoords, sizeof(Vector2D) * nPoints);
				memcpy(m_pTangentAxes, pFrom->m_pTangentAxes, sizeof(TangentSpaceAxes_t) * nPoints);
			}
		}
		else
		{
			Points = NULL;
			m_pTextureCoords = 0;
			m_pLightmapCoords = 0;
			m_pTangentAxes = 0;
			nPoints = 0;
		}

		//
		// Copy the plane. You shouldn't copy the points without copying the plane,
		// so we do it if either bit is set.
		//
		if ((dwFlags & COPY_FACE_POINTS) || (dwFlags & COPY_FACE_PLANE))
		{
			plane = pFrom->plane;
		}
		else
		{
			memset(&plane, 0, sizeof(plane));
		}

        //
        // copy the displacement info.
		//
		// If we do have displacement, then we'll never be asked to become a copy of
		// a face that does not have a displacement, because you cannot undo a Generate
		// Displacement operation.
        //
        if( pFrom->HasDisp() )
        {
			//
			// Allocate a new displacement info if we don't already have one.
			//
			if( !HasDisp() )
			{
				m_DispHandle = EditDispMgr()->Create();
			}

			CMapDisp *pDisp = EditDispMgr()->GetDisp( m_DispHandle );
			pDisp->SetParent( this );

			CMapDisp *pFromDisp = EditDispMgr()->GetDisp( pFrom->m_DispHandle );
			pDisp->CopyFrom( pFromDisp, bUpdateDependencies );
        }
		else
		{
			m_DispHandle = EDITDISPHANDLE_INVALID;
		}
	
		// Copy CMapAtom fields. dvs: this should be done in CMapAtom::CopyFrom!
		r = pFrom->r;
		g = pFrom->g;
		b = pFrom->b;

		m_uchAlpha = pFrom->m_uchAlpha;
		m_bIgnoreLighting = pFrom->m_bIgnoreLighting;

		// Copy the smoothing group data.
		m_fSmoothingGroups = pFrom->m_fSmoothingGroups;
	}

	return(this);
}


//-----------------------------------------------------------------------------
// Purpose: Creates a face from a list of points.
// Input  : pPoints - An array of points.
//			_nPoints - Number of points. If nPoints < 0, reverse points.
//-----------------------------------------------------------------------------
void CMapFace::CreateFace(Vector *pPoints, int _nPoints)
{
	if (_nPoints > 0)
	{
		AllocatePoints(_nPoints);
		ASSERT(nPoints > 0);
		if (nPoints > 0)
		{
			memcpy(Points, pPoints, nPoints * sizeof(Vector));
		}
	}
	else
	{
		AllocatePoints(-_nPoints);
		ASSERT(nPoints > 0);
		if (nPoints > 0)
		{
			int j = 0;
			for (int i = nPoints - 1; i >= 0; i--)
			{
				Points[j++] = pPoints[i];
			}
		}
	}

#ifdef DEBUGPTS
	DebugPoints();
#endif

	CalcPlaneFromFacePoints();
	CalcTextureCoords();

#if 0
    //
    // create the displacement map -- if need be
    //
    if( m_pMapDisp )
    {
		m_pMapDisp->InitSurfData( this, false );
        m_pMapDisp->Create();
    }
#endif
}


Vector FaceNormals[6] =
{
	Vector(0, 0, 1),		// floor
	Vector(0, 0, -1),		// ceiling
	Vector(0, -1, 0),		// north wall
	Vector(0, 1, 0),		// south wall
	Vector(-1, 0, 0),		// east wall
	Vector(1, 0, 0),		// west wall
};


Vector DownVectors[6] =
{
	Vector(0, -1, 0),		// floor
	Vector(0, -1, 0),		// ceiling
	Vector(0, 0, -1),		// north wall
	Vector(0, 0, -1),		// south wall
	Vector(0, 0, -1),		// east wall
	Vector(0, 0, -1),		// west wall
};


Vector RightVectors[6] =
{
	Vector(1, 0, 0),		// floor
	Vector(1, 0, 0),		// ceiling
	Vector(1, 0, 0),		// north wall
	Vector(1, 0, 0),		// south wall
	Vector(0, 1, 0),		// east wall
	Vector(0, 1, 0),		// west wall
};


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			downVect - 
//-----------------------------------------------------------------------------
void CMapFace::GetDownVector( int index, Vector& downVect )
{
    downVect = DownVectors[index];
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Center - 
//-----------------------------------------------------------------------------
void CMapFace::GetCenter(Vector& Center)
{
	ASSERT(nPoints > 0);

	ZeroVector(Center);

	if (nPoints != 0)
	{
		for (int i = 0; i < nPoints; i++)
		{
			Center[0] += Points[i][0];
			Center[1] += Points[i][1];
			Center[2] += Points[i][2];
		}

		Center[0] /= nPoints;
		Center[1] /= nPoints;
		Center[2] /= nPoints;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Determines the general orientation of a face based on its normal vector.
// Output : FaceOrientation_t
//-----------------------------------------------------------------------------
FaceOrientation_t CMapFace::GetOrientation(void) const
{
	// The normal must have a nonzero length!
	if ((plane.normal[0] == 0) && (plane.normal[1] == 0) && (plane.normal[2] == 0))
	{
		return(FACE_ORIENTATION_INVALID);
	}

	//
	// Find the axis that the surface normal has the greatest projection onto.
	//
	float fDot;
	float fMaxDot;
	Vector Normal;

	FaceOrientation_t eOrientation = FACE_ORIENTATION_INVALID;

	Normal = plane.normal;
	VectorNormalize(Normal);

	fMaxDot = 0;
	for (int i = 0; i < 6; i++)
	{
		fDot = DotProduct(Normal, FaceNormals[i]);

		if (fDot >= fMaxDot)
		{
			fMaxDot = fDot;
			eOrientation = (FaceOrientation_t)i;
		}
	}

	return(eOrientation);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eAlignment - 
//			dwFlags - 
//-----------------------------------------------------------------------------
void CMapFace::InitializeTextureAxes(TextureAlignment_t eAlignment, DWORD dwFlags)
{
	FaceOrientation_t eOrientation;

	//
	// If the texture axis information has been initialized, don't reinitialize unless
	// the FORCE flag is set.
	//
	if ((!(dwFlags & INIT_TEXTURE_FORCE)) && 
		((texture.UAxis[0] != 0) || (texture.UAxis[1] != 0) || (texture.UAxis[2] != 0) ||
		 (texture.VAxis[0] != 0) || (texture.VAxis[1] != 0) || (texture.VAxis[2] != 0)))
	{
		return;
	}

	if (dwFlags & INIT_TEXTURE_ROTATION)
	{
		texture.rotate = 0;
	}

	if (dwFlags & INIT_TEXTURE_SHIFT)
	{
		texture.UAxis[3] = 0;
		texture.VAxis[3] = 0;
	}

	if (dwFlags & INIT_TEXTURE_SCALE)
	{
		texture.scale[0] = g_pGameConfig->GetDefaultTextureScale();
		texture.scale[1] = g_pGameConfig->GetDefaultTextureScale();
	}

	if (dwFlags & INIT_TEXTURE_AXES)
	{
		ZeroVector(texture.UAxis);
		ZeroVector(texture.VAxis);

		// Determine the general orientation of this face (floor, ceiling, n wall, etc.)
		eOrientation = GetOrientation();
		if (eOrientation == FACE_ORIENTATION_INVALID)
		{
			CalcTextureCoords();
			return;
		}

		// Pick a world axis aligned V axis based on the face orientation.
		texture.VAxis.AsVector3D() = DownVectors[eOrientation];

		//
		// If we are using face aligned textures, calculate the texture axes.
		//
		if (eAlignment == TEXTURE_ALIGN_FACE)
		{
			// Using that axis-aligned V axis, calculate the true U axis
			CrossProduct(plane.normal, texture.VAxis.AsVector3D(), texture.UAxis.AsVector3D());
			VectorNormalize(texture.UAxis.AsVector3D());

			// Now use the true U axis to calculate the true V axis.
			CrossProduct(texture.UAxis.AsVector3D(), plane.normal, texture.VAxis.AsVector3D());
			VectorNormalize(texture.VAxis.AsVector3D());
		}
		//
		// If we are using world (or "natural") aligned textures, use the V axis as is
		// and pick the corresponding U axis from the table.
		//
		else if (eAlignment == TEXTURE_ALIGN_WORLD)
		{
			texture.UAxis.AsVector3D() = RightVectors[eOrientation];
		}
		//
		// Quake-style texture alignment used a different axis convention.
		//
		else
		{
			InitializeQuakeStyleTextureAxes(texture.UAxis, texture.VAxis);
		}
	
		if (texture.rotate != 0)
		{
			RotateTextureAxes(texture.rotate);
		}
	}

	CalcTextureCoords();
}


//-----------------------------------------------------------------------------
// Purpose: Checks for a texture axis perpendicular to the face.
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapFace::IsTextureAxisValid(void) const
{
	//
	// Generate the texture normal axis, which may be different from the
	// face normal, depending on texture alignment.
	//
	Vector TexNormalAxis;
	CrossProduct(texture.VAxis.AsVector3D(), texture.UAxis.AsVector3D(), TexNormalAxis);
	return(DotProduct(plane.normal, TexNormalAxis) != 0);
}


//-----------------------------------------------------------------------------
// Purpose: Normalize the U/V shift values to be less than the texture width/height.
//-----------------------------------------------------------------------------
void CMapFace::NormalizeTextureShifts(void)
{
	//
	// HACK: this should really be elsewhere, but it can live here for now.
	// Round all components of our texture axes within an epsilon.
	//
	for (int nDim = 0; nDim < 4; nDim++)
	{
		int nValue = rint(texture.UAxis[nDim]);
		if (fabs(texture.UAxis[nDim] - nValue) < TEXTURE_AXIS_ROUND_EPSILON)
		{
			texture.UAxis[nDim] = nValue;
		}

		nValue = rint(texture.VAxis[nDim]);
		if (fabs(texture.VAxis[nDim] - nValue) < TEXTURE_AXIS_ROUND_EPSILON)
		{
			texture.VAxis[nDim] = nValue;
		}
	}

	if (m_pTexture == NULL)
	{
		return;
	}

	if (m_pTexture->GetWidth() != 0)
	{
		texture.UAxis[3] = fmod(texture.UAxis[3], m_pTexture->GetWidth());
	}

	if (m_pTexture->GetHeight() != 0)
	{
		texture.VAxis[3] = fmod(texture.VAxis[3], m_pTexture->GetHeight());
	}
}


//-----------------------------------------------------------------------------
// Purpose: Determines the bounding box of a face in world space.
// Input  : pfMins - Receives the face X, Y, Z minima.
//			pfMaxs - Receives the face X, Y, Z maxima.
//-----------------------------------------------------------------------------
void CMapFace::GetFaceBounds(Vector& pfMins, Vector& pfMaxs) const
{
	for (int nPoint = 0; nPoint < nPoints; nPoint++)
	{
		if ((Points[nPoint][0] < pfMins[0]) || (nPoint == 0))
		{
			pfMins[0] = Points[nPoint][0];
		}

		if ((Points[nPoint][1] < pfMins[1]) || (nPoint == 0))
		{
			pfMins[1] = Points[nPoint][1];
		}

		if ((Points[nPoint][2] < pfMins[2]) || (nPoint == 0))
		{
			pfMins[2] = Points[nPoint][2];
		}

		if ((Points[nPoint][0] > pfMaxs[0]) || (nPoint == 0))
		{
			pfMaxs[0] = Points[nPoint][0];
		}

		if ((Points[nPoint][1] > pfMaxs[1]) || (nPoint == 0))
		{
			pfMaxs[1] = Points[nPoint][1];
		}

		if ((Points[nPoint][2] > pfMaxs[2]) || (nPoint == 0))
		{
			pfMaxs[2] = Points[nPoint][2];
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Finds the top left and bottom right points on the face in texture space.
//			These points are returned in texture space, not world space.
// Input  : TopLeft - 
//			BottomRight - 
//-----------------------------------------------------------------------------
void CMapFace::GetFaceTextureExtents(Vector2D & TopLeft, Vector2D & BottomRight) const
{
	BOOL bFirst = TRUE;

	for (int nPoint = 0; nPoint < nPoints; nPoint++)
	{
		Vector2D Test;

		Test[0] = DotProduct(Points[nPoint], texture.UAxis.AsVector3D()) / texture.scale[0];
		Test[1] = DotProduct(Points[nPoint], texture.VAxis.AsVector3D()) / texture.scale[1];

		if ((Test[0] < TopLeft[0]) || (bFirst))
		{
			TopLeft[0] = Test[0];
		}

		if ((Test[1] < TopLeft[1]) || (bFirst))
		{
			TopLeft[1] = Test[1];
		}

		if ((Test[0] > BottomRight[0]) || (bFirst))
		{
			BottomRight[0] = Test[0];
		}

		if ((Test[1] > BottomRight[1]) || (bFirst))
		{
			BottomRight[1] = Test[1];
		}

		bFirst = FALSE;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the distance along the face normal of a given point. The
//			distance will be negative if the point is behind the face, positive
//			if the point is in front of the face.
// Input  : fPoint - Point to calculate normal distance.
//-----------------------------------------------------------------------------
float CMapFace::GetNormalDistance(Vector& fPoint)
{
	float fDot = DotProduct(fPoint, plane.normal);
	return(fDot - plane.dist);
}


//-----------------------------------------------------------------------------
// Purpose: Determines the texture alignment(s) of this face. The alignments are
//			are returned as TextureAlignment_t values OR'ed together.
//
// Output : Returns an integer with any of the following flags set:
//
//			TEXTURE_ALIGN_FACE - the texture axes are face aligned.
//			TEXTURE_ALIGN_WORLD - the texture axes are world aligned.
//
//			If the returned value is zero (TEXTURE_ALIGN_NONE), the texture axes
//			are neither face aligned nor world aligned.
//-----------------------------------------------------------------------------
int CMapFace::GetTextureAlignment(void) const
{
	Vector TexNormalAxis;
	int nAlignment = TEXTURE_ALIGN_NONE;

	//
	// Generate the texture normal axis, which may be different from the
	// face normal, depending on texture alignment.
	//
	CrossProduct(texture.VAxis.AsVector3D(), texture.UAxis.AsVector3D(), TexNormalAxis);
	VectorNormalize(TexNormalAxis);

	//
	// Check for face alignment.
	//
	if (DotProduct(TexNormalAxis, plane.normal) > 0.9999)
	{
		nAlignment |= TEXTURE_ALIGN_FACE;
	}

	//
	// Check for world alignment.
	//
	FaceOrientation_t eOrientation = GetOrientation();
	if (eOrientation != FACE_ORIENTATION_INVALID)
	{
		Vector WorldTexNormal;

		CrossProduct(DownVectors[eOrientation], RightVectors[eOrientation], WorldTexNormal);
		if (DotProduct(TexNormalAxis, WorldTexNormal) > 0.9999)
		{
			nAlignment |= TEXTURE_ALIGN_WORLD;
		}
	}

	return(nAlignment);
}


//-----------------------------------------------------------------------------
// Purpose: Finds the top left and bottom right points of the given world extents
//			in texture space. These points are returned in texture space, not world space,
//			so a simple rectangle will suffice.
// Input  : Extents - 
//			TopLeft - 
//			BottomRight - 
//-----------------------------------------------------------------------------
void CMapFace::GetTextureExtents(Extents_t Extents, Vector2D & TopLeft, Vector2D & BottomRight) const
{
	BOOL bFirst = TRUE;

	for (int nPoint = 0; nPoint < NUM_EXTENTS_DIMS; nPoint++)
	{
		Vector2D Test;

		Test[0] = DotProduct(Extents[nPoint], texture.UAxis.AsVector3D()) / texture.scale[0];
		Test[1] = DotProduct(Extents[nPoint], texture.VAxis.AsVector3D()) / texture.scale[1];

		if ((Test[0] < TopLeft[0]) || (bFirst))
		{
			TopLeft[0] = Test[0];
		}

		if ((Test[1] < TopLeft[1]) || (bFirst))
		{
			TopLeft[1] = Test[1];
		}

		if ((Test[0] > BottomRight[0]) || (bFirst))
		{
			BottomRight[0] = Test[0];
		}

		if ((Test[1] > BottomRight[1]) || (bFirst))
		{
			BottomRight[1] = Test[1];
		}

		bFirst = FALSE;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Determines the world extents of the face. Different from a bounding
//			box in that each point in the returned extents is actually on the face.
// Input  : Extents - 
//-----------------------------------------------------------------------------
void CMapFace::GetFaceExtents(Extents_t Extents) const
{
	BOOL bFirst = TRUE;

	for (int nPoint = 0; nPoint < nPoints; nPoint++)
	{
		if ((Points[nPoint][0] < Extents[EXTENTS_XMIN][0]) || (bFirst))
		{
			Extents[EXTENTS_XMIN] = Points[nPoint];
		}

		if ((Points[nPoint][0] > Extents[EXTENTS_XMAX][0]) || (bFirst))
		{
			Extents[EXTENTS_XMAX] = Points[nPoint];
		}

		if ((Points[nPoint][1] < Extents[EXTENTS_YMIN][1]) || (bFirst))
		{
			Extents[EXTENTS_YMIN] = Points[nPoint];
		}

		if ((Points[nPoint][1] > Extents[EXTENTS_YMAX][1]) || (bFirst))
		{
			Extents[EXTENTS_YMAX] = Points[nPoint];
		}

		if ((Points[nPoint][2] < Extents[EXTENTS_ZMIN][2]) || (bFirst))
		{
			Extents[EXTENTS_ZMIN] = Points[nPoint];
		}

		if ((Points[nPoint][2] > Extents[EXTENTS_ZMAX][2]) || (bFirst))
		{
			Extents[EXTENTS_ZMAX] = Points[nPoint];
		}

		bFirst = FALSE;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eJustification - 
//			Extents - 
//-----------------------------------------------------------------------------
void CMapFace::JustifyTextureUsingExtents(TextureJustification_t eJustification, Extents_t Extents)
{
	Vector2D Center;

	if (!texture.scale[0])
	{
		texture.scale[0] = g_pGameConfig->GetDefaultTextureScale();
	}

	if (!texture.scale[1])
	{
		texture.scale[1] = g_pGameConfig->GetDefaultTextureScale();
	}

	// Skip all the mucking about for a justification of NONE.
	if (eJustification == TEXTURE_JUSTIFY_NONE)
	{
		texture.UAxis[3] = 0;
		texture.VAxis[3] = 0;
		CalcTextureCoords();
		return;
	}

	// For fit justification, use a scale of 1 for initial calculations.
	if (eJustification == TEXTURE_JUSTIFY_FIT)
	{
		texture.scale[0] = 1.0;
		texture.scale[1] = 1.0;
	}

	Vector2D TopLeft;
	Vector2D BottomRight;

	GetTextureExtents(Extents, TopLeft, BottomRight);

	// Find the face center in U/V space.
	Center[0] = (TopLeft[0] + BottomRight[0]) / 2;
	Center[1] = (TopLeft[1] + BottomRight[1]) / 2;

	//
	// Perform the justification.
	//
	switch (eJustification)
	{
		// Align the top left corner of the texture with the top left corner of the face.
		case TEXTURE_JUSTIFY_TOP:
		{
			texture.VAxis[3] = -TopLeft[1];
			break;
		}

		// Align the top left corner of the texture with the top left corner of the face.
		case TEXTURE_JUSTIFY_BOTTOM:
		{
			texture.VAxis[3] = -BottomRight[1] + m_pTexture->GetHeight();
			break;
		}

		// Align the left side of the texture with the left side of the face.
		case TEXTURE_JUSTIFY_LEFT:
		{
			texture.UAxis[3] = -TopLeft[0];
			break;
		}

		// Align the right side of the texture with the right side of the face.
		case TEXTURE_JUSTIFY_RIGHT:
		{
			texture.UAxis[3] = -BottomRight[0] + m_pTexture->GetWidth();
			break;
		}

		// Center the texture on the face.
		case TEXTURE_JUSTIFY_CENTER:
		{
			texture.UAxis[3] = -Center[0] + (m_pTexture->GetWidth() / 2);
			texture.VAxis[3] = -Center[1] + (m_pTexture->GetHeight() / 2);
			break;
		}

		// Scale the texture to exactly fit the face.
		case TEXTURE_JUSTIFY_FIT:
		{
			// Calculate the appropriate scale.
			if (m_pTexture && m_pTexture->GetWidth() && m_pTexture->GetHeight())
			{
				texture.scale[0] = (BottomRight[0] - TopLeft[0]) / m_pTexture->GetWidth();
				texture.scale[1] = (BottomRight[1] - TopLeft[1]) / m_pTexture->GetHeight();
			}
			else
			{
				texture.scale[0] = g_pGameConfig->GetDefaultTextureScale();
				texture.scale[1] = g_pGameConfig->GetDefaultTextureScale();
			}

			// Justify top left.
			JustifyTextureUsingExtents(TEXTURE_JUSTIFY_TOP, Extents);
			JustifyTextureUsingExtents(TEXTURE_JUSTIFY_LEFT, Extents);

			break;
		}
	}

	NormalizeTextureShifts();
	CalcTextureCoords();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eJustification - 
//-----------------------------------------------------------------------------
void CMapFace::JustifyTexture(TextureJustification_t eJustification)
{
	Extents_t Extents;
	GetFaceExtents(Extents);
	JustifyTextureUsingExtents(eJustification, Extents);
}


//-----------------------------------------------------------------------------
// Purpose: Offsets a texture due to texture locking when moving a face.
// Input  : Delta - The x, y, z translation that was applied to the face points.
//-----------------------------------------------------------------------------
void CMapFace::OffsetTexture(const Vector &Delta)
{
	//
	// Find the projection in U/V space of this movement
	// and shift the textures by that.
	//
	texture.UAxis[3] -= DotProduct(Delta, texture.UAxis.AsVector3D()) / texture.scale[0];
	texture.VAxis[3] -= DotProduct(Delta, texture.VAxis.AsVector3D()) / texture.scale[1];

	NormalizeTextureShifts();
}


//-----------------------------------------------------------------------------
// Purpose: Rotates the texture axes fDegrees counterclockwise around the
//			texture normal axis.
// Input  : fDegrees - Degrees to rotate the texture axes.
//-----------------------------------------------------------------------------
void CMapFace::RotateTextureAxes(float fDegrees)
{
	matrix4_t Matrix;
	Vector TexNormalAxis;
	Vector UAxis;
	Vector VAxis;
	
	// Generate the texture normal axis, which may be different from the
	// face normal, depending on texture alignment.
	CrossProduct(texture.VAxis.AsVector3D(), texture.UAxis.AsVector3D(), TexNormalAxis);

	// Rotate the texture axes around the texture normal.
	AxisAngleMatrix(Matrix, TexNormalAxis, fDegrees);
	MatrixMultiply(UAxis, texture.UAxis.AsVector3D(), Matrix);
	MatrixMultiply(VAxis, texture.VAxis.AsVector3D(), Matrix);

	texture.UAxis.AsVector3D() = UAxis;
	texture.VAxis.AsVector3D() = VAxis;
}


//-----------------------------------------------------------------------------
// Purpose: Rebuilds the plane normal and distance from the plane points.
//-----------------------------------------------------------------------------
void CMapFace::CalcPlane(void)
{
	//
	// Build the plane normal and distance from the three plane points.
	//
	Vector t1;
	Vector t2;
	Vector t3;

	for (int i = 0; i < 3; i++)
	{
		t1[i] = plane.planepts[0][i] - plane.planepts[1][i];
		t2[i] = plane.planepts[2][i] - plane.planepts[1][i];
		t3[i] = plane.planepts[1][i];
	}

	CrossProduct(t1, t2, plane.normal);
	VectorNormalize(plane.normal);
	plane.dist = DotProduct(t3, plane.normal);
}


//-----------------------------------------------------------------------------
// Purpose: Rebuilds the plane points from our face points.
//-----------------------------------------------------------------------------
void CMapFace::CalcPlaneFromFacePoints(void)
{
	if ((nPoints >= 3) && (Points != NULL))
	{
		//
		// Use the face points as a preliminary set of plane points.
		//
		memcpy(plane.planepts, Points, sizeof(Vector) * 3);

		//
		// Generate the plane normal and distance from the plane points.
		//
		CalcPlane();

		//
		// Now project large coordinates onto the plane to generate new
		// plane points that will be less prone to error creep.
		//
		// UNDONE: push out the points along the plane for better precision
	}
}


#ifdef DEBUGPTS
void CMapFace::DebugPoints(void)
{
	// check for dup points
	for(i = 0; i < nPoints; i++)
	{
		for(int j = 0; j < nPoints; j++)
		{
			if(j == i)
				continue;
			if(Points[j][0] == Points[i][0] &&
				Points[j][1] == Points[i][1] &&
				Points[j][2] == Points[i][2])
			{
				AfxMessageBox("Dup Points in CMapFace::Create(winding_t*)");
				break;
			}
		}
	}
}
#endif



//-----------------------------------------------------------------------------
// Purpose: Create the face from a winding type.
// Input  : w - Winding from which to create the face.
//-----------------------------------------------------------------------------
void CMapFace::CreateFace(winding_t *w, int nFlags)
{
	AllocatePoints(w->numpoints);
	for (int i = 0; i < nPoints; i++)
	{
		Points[i][0] = w->p[i][0];
		Points[i][1] = w->p[i][1];
		Points[i][2] = w->p[i][2];
	}

	if (!(nFlags & CREATE_FACE_PRESERVE_PLANE))
	{
		CalcPlaneFromFacePoints();
	}

    //
    // Create a new displacement surface is the clipped surfaces is a quad.
    //
	// This assumes it is being called by the clipper!!! (Bad assumption).
	//
    if( HasDisp() && ( nFlags & CREATE_FACE_CLIPPING ) )
    {
		if ( nPoints == 4 )
		{
			// Setup new displacement surface.
			EditDispHandle_t hClipDisp = EditDispMgr()->Create();
			CMapDisp *pClipDisp = EditDispMgr()->GetDisp( hClipDisp );
			
			// Get older displacement surface.
			EditDispHandle_t hDisp = GetDisp();
			CMapDisp *pDisp = EditDispMgr()->GetDisp( hDisp );
			
			// Init new displacement surface.
			pClipDisp->SetParent( this );
			SetDisp( hClipDisp );
			pClipDisp->InitData( pDisp->GetPower() );
			CalcTextureCoords();
			
			if ( !pDisp->Split( this, hClipDisp ) )
			{
				EditDispMgr()->Destroy( hClipDisp );
			}
		}
		else
		{
			EditDispMgr()->Destroy( GetDisp() );
			SetDisp( EDITDISPHANDLE_INVALID );
		}
    }
	else
	{
		CalcTextureCoords();
	}

#ifdef DEBUGPTS
	DebugPoints();
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Allocates space in Points array for nPoints worth of Vectors and
//			the corresponding texture and lightmap coordinates (Vector2D's). Frees
//			current points if there are any.
// Input  : _nPoints - number of points needed.
// Output : Total size of memory used by the points, texture, and lightmap coordinates.
//-----------------------------------------------------------------------------
size_t CMapFace::AllocatePoints(int _nPoints)
{
	//
	// If we have already allocated this many points, do nothing.
	//
	if ((Points != NULL) && (_nPoints == nPoints))
	{
		return(nPoints * (sizeof(Vector) + sizeof(Vector2D) + sizeof(Vector2D)));
	}

	//
	// If we have the wrong number of points allocated, free the memory.
	//
	if (Points != NULL)
	{
		delete [] Points;
		Points = NULL;

		delete [] m_pTextureCoords;
		m_pTextureCoords = NULL;

		delete [] m_pLightmapCoords;
		m_pLightmapCoords = NULL;
	}

	nPoints = _nPoints;

	if (!_nPoints)
	{
		return(0);
	}
	
	//
	// Allocate the correct number of points, texture coords, and lightmap coords.
	//
	Points = new Vector[nPoints];
	m_pTextureCoords = new Vector2D[nPoints];
	m_pLightmapCoords = new Vector2D[nPoints];

	// dvs: check for failure here and report an out of memory error
	ASSERT(Points != NULL);
	ASSERT(m_pTextureCoords != NULL);
	ASSERT(m_pLightmapCoords != NULL);

	return(nPoints * (sizeof(Vector) + sizeof(Vector2D) + sizeof(Vector2D)));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTexture - 
//-----------------------------------------------------------------------------
void CMapFace::SetTexture(IEditorTexture *pTexture)
{
	m_pTexture = pTexture;

	// Copy other things from m_pTexture.
	m_pTexture->GetShortName(texture.texture);
	texture.q2surface = m_pTexture->GetSurfaceAttributes();
	texture.q2contents = m_pTexture->GetSurfaceContents();

	BOOL bTexValid = FALSE;
	if (m_pTexture != NULL)
	{
		// Insure that the texture is loaded.
		m_pTexture->Load();

		bTexValid = !(
			m_pTexture->GetWidth() == 0 || 
			m_pTexture->GetHeight() == 0 ||
			m_pTexture->GetImageWidth() == 0 ||
			m_pTexture->GetImageHeight() == 0 || 
			!m_pTexture->HasData()
		);
	}
	
	if (bTexValid)
	{
		CalcTextureCoords();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets this face's texture by name.
// Input  : pszNewTex - Short name of texture to apply to this face.
//-----------------------------------------------------------------------------
void CMapFace::SetTexture(const char *pszNewTex)
{
	IEditorTexture *pTexture = g_Textures.FindActiveTexture(pszNewTex);
	SetTexture(pTexture);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapFace::CalcTextureCoordAtPoint( const Vector& pt, Vector2D &texCoord )
{
	// sanity check
	if( m_pTexture == NULL )
		return;

	//
	// projected s, t (u, v) texture coordinates
	//
	float s = DotProduct( texture.UAxis.AsVector3D(), pt ) / texture.scale[0] + texture.UAxis[3];
	float t = DotProduct( texture.VAxis.AsVector3D(), pt ) / texture.scale[1] + texture.VAxis[3];

	//
	// "normalize" the texture coordinates
	//
	if (m_pTexture->GetWidth())
		texCoord[0] = s / ( float )m_pTexture->GetWidth();
	else
		texCoord[0] = 0.0;
	
	if (m_pTexture->GetHeight())
		texCoord[1] = t / ( float )m_pTexture->GetHeight();
	else
		texCoord[1] = 0.0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapFace::CalcLightmapCoordAtPoint( const Vector& pt, Vector2D &lightCoord )
{
	lightCoord[0] = DotProduct( texture.UAxis.AsVector3D(), pt ) / texture.nLightmapScale + 0.5f;
	lightCoord[1] = DotProduct( texture.VAxis.AsVector3D(), pt ) / texture.nLightmapScale + 0.5f;
}


//-----------------------------------------------------------------------------
// Purpose: Calculates the U,V texture coordinates of all points on this face.
//-----------------------------------------------------------------------------
void CMapFace::CalcTextureCoords(void)
{
	float s, t;
	int i;

	if (m_pTexture == NULL)
	{
		return;
	}

	//
	// Make sure that scales are nonzero.
	//
	if (texture.scale[0] == 0)
	{
		texture.scale[0] = g_pGameConfig->GetDefaultTextureScale();
	}

	if (texture.scale[1] == 0)
	{
		texture.scale[1] = g_pGameConfig->GetDefaultTextureScale();
	}

	//
	// Recalculate U,V coordinates for all points.
	//
	for (i = 0; i < nPoints; i++)
	{
		//
		// Generate texture coordinates.
		//
		s = DotProduct(texture.UAxis.AsVector3D(), Points[i]) / texture.scale[0] + texture.UAxis[3];
		t = DotProduct(texture.VAxis.AsVector3D(), Points[i]) / texture.scale[1] + texture.VAxis[3];

		if (m_pTexture->GetWidth())
			m_pTextureCoords[i][0] = s / (float)m_pTexture->GetWidth();
		else
			m_pTextureCoords[i][0] = 0.0f;

		if (m_pTexture->GetHeight())
			m_pTextureCoords[i][1] = t / (float)m_pTexture->GetHeight();
 		else
			m_pTextureCoords[i][1] = 0.0f;

		//
		// Generate lightmap coordinates.
		//
		m_pLightmapCoords[i][0] = DotProduct(texture.UAxis.AsVector3D(), Points[i]) / texture.nLightmapScale + 0.5;
		m_pLightmapCoords[i][1] = DotProduct(texture.VAxis.AsVector3D(), Points[i]) / texture.nLightmapScale + 0.5;
 	}

	//
    // update the displacement map with new texture and lightmap coordinates
	//
	if( ( m_DispHandle != EDITDISPHANDLE_INVALID ) && nPoints == 4 )
    {
		CMapDisp *pDisp = EditDispMgr()->GetDisp( m_DispHandle );
		pDisp->InitDispSurfaceData( this, false );
		pDisp->Create();
    }

	// re-calculate the tangent space
	CalcTangentSpaceAxes();
}


//-----------------------------------------------------------------------------
// Returns the max lightmap size for this face
//-----------------------------------------------------------------------------
int CMapFace::MaxLightmapSize() const
{
	return HasDisp() ? MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER : MAX_BRUSH_LIGHTMAP_DIM_WITHOUT_BORDER;
}


//-----------------------------------------------------------------------------
// Purpose: Returns TRUE if the lightmap scale on this face is within the acceptable range.
//-----------------------------------------------------------------------------
BOOL CMapFace::ValidLightmapSize( void )
{
	Vector2D minCoord, maxCoord;

	const int nFaceMaxLightmapSize = MaxLightmapSize();

	//
	// initialize the minimum and maximum light coords
	//
	minCoord[0] = minCoord[1] = COORD_NOTINIT;
	maxCoord[0] = maxCoord[1] = -COORD_NOTINIT;

	for( int i = 0; i < nPoints; i++ )
	{
		if( m_pLightmapCoords[i][0] < minCoord[0] ) { minCoord[0] = m_pLightmapCoords[i][0]; }
		if( m_pLightmapCoords[i][1] < minCoord[1] ) { minCoord[1] = m_pLightmapCoords[i][1]; }

		if( m_pLightmapCoords[i][0] > maxCoord[0] ) { maxCoord[0] = m_pLightmapCoords[i][0]; }
		if( m_pLightmapCoords[i][1] > maxCoord[1] ) { maxCoord[1] = m_pLightmapCoords[i][1]; }
	}

	//
	// check for valid lightmap coord dimensions in u and v
	//
	for( i = 0; i < 2; i++ )
	{
		//
		// bloat points a little
		//
		minCoord[i] = ( float )floor( minCoord[i] );
		maxCoord[i] = ( float )ceil( maxCoord[i] );

		int size = ( int )( maxCoord[i] - minCoord[i] );
		if( size > nFaceMaxLightmapSize )
		{
			return FALSE;
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapFace::AdjustLightmapScale( void )
{
	Vector2D minCoord, maxCoord;
	const int nFaceMaxLightmapSize = MaxLightmapSize();

	//
	// initialize the minimum and maximum light coords
	//
	minCoord[0] = minCoord[1] = COORD_NOTINIT;
	maxCoord[0] = maxCoord[1] = -COORD_NOTINIT;

	for( int i = 0; i < nPoints; i++ )
	{
		if( m_pLightmapCoords[i][0] < minCoord[0] ) { minCoord[0] = m_pLightmapCoords[i][0]; }
		if( m_pLightmapCoords[i][1] < minCoord[1] ) { minCoord[1] = m_pLightmapCoords[i][1]; }

		if( m_pLightmapCoords[i][0] > maxCoord[0] ) { maxCoord[0] = m_pLightmapCoords[i][0]; }
		if( m_pLightmapCoords[i][1] > maxCoord[1] ) { maxCoord[1] = m_pLightmapCoords[i][1]; }
	}

	//
	// generate a valid lightmap scale
	//
	int nMaxIdx = -1;
	int nMaxWidth = -9999;
	for( i = 0; i < 2; i++ )
	{
		int nWidth = ceil( maxCoord[i] ) - floor( minCoord[i] );
		if( nWidth > nMaxWidth )
		{
			nMaxIdx = i;
			nMaxWidth = nWidth;
		}
	}

	if (nMaxWidth <= nFaceMaxLightmapSize)
		return;

	// Here's the equation
	// We want to find a *integer* scale s so that
	// ceil( xo / s ) - floor( x1 / s ) == m
	// where xo, x1 are the min + max lightmap coords (without scale),
	// and m is the max lightmap dimension

	// This is actually painful to solve directly owing to all of the various
	// truncations, etc. so we'll just brute force it by coming up with a good
	// lower bound and then iterating the scale up until we find something that works
	float flMax = texture.nLightmapScale * maxCoord[nMaxIdx];
	float flMin = texture.nLightmapScale * minCoord[nMaxIdx];

	int nMinScale = floor( (flMax - flMin) / nFaceMaxLightmapSize );
	while ( true ) 
	{
		int nTest = ceil( flMax / nMinScale ) - floor( flMin / nMinScale );	
		if ( nTest <= nFaceMaxLightmapSize ) 
			break;

		++nMinScale;
	}

	texture.nLightmapScale = nMinScale;

	// recalculate the texture coordinates
	CalcTextureCoords();
}


//-----------------------------------------------------------------------------
// Purpose: Checks the validity of this face.
// Input  : pInfo - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapFace::CheckFace(CCheckFaceInfo *pInfo)
{
	if (!::CheckFace(Points, nPoints, &plane.normal, plane.dist, pInfo))
	{
		return(FALSE);
	}

	//
	// Check for duplicate plane points. All three plane points must be unique
	// or it isn't a valid plane.
	//
	for (int nPlane = 0; nPlane < 3; nPlane++)
	{
		for (int nPlaneCheck = 0; nPlaneCheck < 3; nPlaneCheck++)
		{
			if (nPlane != nPlaneCheck)
			{
				if (VectorCompare(plane.planepts[nPlane], plane.planepts[nPlaneCheck]))
				{
					if (pInfo != NULL)
					{
						strcpy(pInfo->szDescription, "face has duplicate plane points");
					}
					return(FALSE);		
				}
			}
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Included for loading old (quake-style) maps. This sets up the texture axes
//			the same way QCSG and pre-2.2 Worldcraft did.
// Input  : UAxis - 
//			VAxis - 
//-----------------------------------------------------------------------------
void CMapFace::InitializeQuakeStyleTextureAxes(Vector4D& UAxis, Vector4D& VAxis)
{
	static Vector baseaxis[18] =
	{
		Vector(0,0,1), Vector(1,0,0), Vector(0,-1,0),			// floor
		Vector(0,0,-1), Vector(1,0,0), Vector(0,-1,0),		// ceiling
		Vector(1,0,0), Vector(0,1,0), Vector(0,0,-1),			// west wall
		Vector(-1,0,0), Vector(0,1,0), Vector(0,0,-1),		// east wall
		Vector(0,1,0), Vector(1,0,0), Vector(0,0,-1),			// south wall
		Vector(0,-1,0), Vector(1,0,0), Vector(0,0,-1)			// north wall
	};

	int		bestaxis;
	vec_t	dot,best;
	int		i;
	
	best = 0;
	bestaxis = 0;
	
	for (i=0 ; i<6 ; i++)
	{
		dot = DotProduct(plane.normal, baseaxis[i*3]);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}
	
	UAxis.AsVector3D() = baseaxis[bestaxis * 3 + 1];
	VAxis.AsVector3D() = baseaxis[bestaxis * 3 + 2];
}


//-----------------------------------------------------------------------------
// Should we render this lit or not
//-----------------------------------------------------------------------------
void CMapFace::RenderUnlit( bool enable )
{
	m_bIgnoreLighting = enable;
}


//-----------------------------------------------------------------------------
// Various different color types
//-----------------------------------------------------------------------------
inline void Color( unsigned char* pColor, unsigned char r, unsigned char g, unsigned char b )
{
	pColor[0] = r;
	pColor[1] = g;
	pColor[2] = b;
}

inline void Modulate( unsigned char* pColor, float f )
{
	pColor[0] *= f;
	pColor[1] *= f;
	pColor[2] *= f;
}


//-----------------------------------------------------------------------------
// Computes the color and texture to use
//-----------------------------------------------------------------------------

void CMapFace::ComputeColor( CRender3D* pRender, bool bRenderAsSelected,
							 SelectionState_t faceSelectionState,
							 bool ignoreLighting, unsigned char* pColor )
{
	RenderMode_t eCurrentRenderMode = pRender->GetCurrentRenderMode();
	
	// White w/alpha by default
	pColor[0] = pColor[1] = pColor[2] = 255;
	pColor[3] = m_uchAlpha;

	float fShade;
	if (!ignoreLighting)
		fShade = pRender->LightPlane(plane.normal);
	else
		fShade = 1.0;

	switch (eCurrentRenderMode)
	{
	case RENDER_MODE_TEXTURED:
		Modulate( pColor, fShade );
		break;

	case RENDER_MODE_SELECTION_OVERLAY:
		if( faceSelectionState == SELECT_MULTI_PARTIAL )
		{
			pColor[2] = 100;
			pColor[3] = 64;
		}
		else if( ( faceSelectionState == SELECT_NORMAL ) || bRenderAsSelected )
		{
			SelectFaceColor( pColor );
			pColor[3] = 64;
		}
		break;

	case RENDER_MODE_LIGHTMAP_GRID:
		if (bRenderAsSelected)
		{
			SelectFaceColor( pColor );
		}
		else if (texture.nLightmapScale != DEFAULT_LIGHTMAP_SCALE)
		{
			pColor[2] = 100;
		}
		pColor[3] = 255;

		Modulate( pColor, fShade );
		break;

	case RENDER_MODE_TRANSLUCENT_FLAT:
	case RENDER_MODE_FLAT:
		if (bRenderAsSelected)
			SelectFaceColor( pColor );
		else
			Color( pColor, r, g, b );
		Modulate( pColor, fShade );
		break;

	case RENDER_MODE_WIREFRAME:
		if (bRenderAsSelected)
			SelectEdgeColor( pColor );
		else
			Color( pColor, r, g, b );
		break;

	case RENDER_MODE_SMOOTHING_GROUP:
		{
			// Render the non-smoothing group faces in white, yellow for the others.
			CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
			if ( pDoc )
			{
				int iGroup = pDoc->GetSmoothingGroupVisual();
				if ( InSmoothingGroup( iGroup ) )
				{
					pColor[2] = 0;
				}
			}

			Modulate( pColor, fShade );
			break;
		}

	default:
		assert(0);
		break;
	}
}


//-----------------------------------------------------------------------------
// Draws the face using the material system material
//-----------------------------------------------------------------------------
void CMapFace::DrawFace( Vector& ViewPoint, unsigned char* pColor, RenderMode_t mode )
{
	// retrieve the coordinate frame to render into 
	// (most likely just the identity, unless we're animating)
	matrix4_t frame;
	bool hasParent = GetTransformMatrix( frame );

	// don't do this -- if you use the material system to rotate and/or translate
	// this will cull the locally spaced object!! -- need to pass around a flag!
#if 0 
	// A little culling....
	float fEyeDot = DotProduct(plane.normal, ViewPoint);
	if ((fEyeDot < plane.dist) && (mode != RENDER_MODE_WIREFRAME) && !hasParent && 
		(m_uchAlpha == 255))
	{
		return;
	}
#endif

	MaterialPrimitiveType_t type = (mode == RENDER_MODE_WIREFRAME) ? 
		MATERIAL_LINE_LOOP : MATERIAL_POLYGON; 

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh( );
	meshBuilder.Begin( pMesh, type, nPoints );
    
    for (int nPoint = 0; nPoint < nPoints; nPoint++)
    {
		if ((mode == RENDER_MODE_TEXTURED) || (mode == RENDER_MODE_LIGHTMAP_GRID))
		{
			meshBuilder.TexCoord2f( 0, m_pTextureCoords[nPoint][0], m_pTextureCoords[nPoint][1] );
			meshBuilder.TexCoord2f( 1, m_pLightmapCoords[nPoint][0], m_pLightmapCoords[nPoint][1]);
		}

		meshBuilder.Color4ubv( pColor );

        // transform into absolute space
        if ( hasParent )
        {
            Vector point;
            VectorTransform( Points[nPoint], frame, point );
            meshBuilder.Position3f(point[0], point[1], point[2]);
        }
        else
        {
            meshBuilder.Position3f(Points[nPoint][0], Points[nPoint][1], Points[nPoint][2]);
        }

		// FIXME: no smoothing group information
		meshBuilder.Normal3fv(plane.normal.Base());
		meshBuilder.TangentS3fv( m_pTangentAxes[nPoint].tangent.Base() );
		meshBuilder.TangentT3fv( m_pTangentAxes[nPoint].binormal.Base() );

		meshBuilder.AdvanceVertex();
    }
    
    meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Renders the grid on the face
//-----------------------------------------------------------------------------
void CMapFace::RenderGridIfCloseEnough( CRender3D* pRender, Vector& viewPoint )
{
	// If the 3D grid is enabled and we aren't picking, 
	// render the grid on this face.
	if ((pRender->IsEnabled(RENDER_GRID)) && (!pRender->IsPicking()))
	{
		Vector Maxs;
		Vector Mins;

		float fGridSize = pRender->GetGridDistance();

		GetFaceBounds(Mins, Maxs);

		for (int i = 0; i < 3; i++)
		{
			Mins[i] -= fGridSize;
			Maxs[i] += fGridSize;
		}

		// Only render the grid if the face is close enough to the camera.
		if (PointInBox(viewPoint, Mins, Maxs))
		{
			Render3DGrid(pRender);
		}
	}
}


//-----------------------------------------------------------------------------
// renders the texture axes
//-----------------------------------------------------------------------------
void CMapFace::RenderTextureAxes( CRender3D* pRender )
{
	Vector Center;
	GetCenter(Center);

	// Render the world axes.
	pRender->SetRenderMode( RENDER_MODE_WIREFRAME );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh( );
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 2 );

	meshBuilder.Color3ub(255, 255, 0);
	meshBuilder.Position3f(Center[0], Center[1], Center[2]);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(255, 255, 0);
	meshBuilder.Position3f(Center[0] + texture.UAxis[0] * TEXTURE_AXIS_LENGTH, 
		Center[1] + texture.UAxis[1] * TEXTURE_AXIS_LENGTH, 
		Center[2] + texture.UAxis[2] * TEXTURE_AXIS_LENGTH);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(0, 255, 0);
	meshBuilder.Position3f(Center[0], Center[1], Center[2]);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(0, 255, 0);
	meshBuilder.Position3f(Center[0] + texture.VAxis[0] * TEXTURE_AXIS_LENGTH, 
		Center[1] + texture.VAxis[1] * TEXTURE_AXIS_LENGTH, 
		Center[2] + texture.VAxis[2] * TEXTURE_AXIS_LENGTH);
	meshBuilder.AdvanceVertex();
	
	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// for sorting
//-----------------------------------------------------------------------------
bool CMapFace::ShouldRenderLast()
{
	if (!m_pTexture || !m_pTexture->GetMaterial())
		return false;

	return m_pTexture->GetMaterial()->IsTranslucent() || (m_uchAlpha != 255);
}


//-----------------------------------------------------------------------------
// Record rendered opaque faces so we can sort later....
//-----------------------------------------------------------------------------
struct MapFaceRender_t
{
	bool m_RenderSelected;
	RenderMode_t m_RenderMode;
	IEditorTexture* m_pTexture;
	CMapFace* m_pMapFace;
	SelectionState_t m_FaceSelectionState;
};


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mode - 
//-----------------------------------------------------------------------------
static int SortVal(RenderMode_t mode)
{
	if (mode == RENDER_MODE_WIREFRAME)
		return 2;
	if (mode == RENDER_MODE_SELECTION_OVERLAY)
		return 1;
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : s1 - 
//			s2 - 
// Output : 
//-----------------------------------------------------------------------------
static bool OpaqueFacesLessFunc( MapFaceRender_t const& s1, MapFaceRender_t const& s2 )
{
	// Render texture first, overlay second, wireframe 3rd
	if (SortVal(s1.m_RenderMode) < SortVal(s2.m_RenderMode))
		return true;
	if (SortVal(s1.m_RenderMode) > SortVal(s2.m_RenderMode))
		return false;
	return s1.m_pTexture < s2.m_pTexture;
}


static CUtlRBTree<MapFaceRender_t, int> g_OpaqueFaces(0, 0, OpaqueFacesLessFunc);


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pMapFace - 
//			pTexture - 
//			renderMode - 
//			selected - 
//			faceSelectionState - 
//-----------------------------------------------------------------------------
static void AddFaceToQueue( CMapFace* pMapFace, IEditorTexture* pTexture, 
	RenderMode_t renderMode, bool selected, SelectionState_t faceSelectionState )
{
	MapFaceRender_t newEntry;
	newEntry.m_RenderMode = renderMode;
	newEntry.m_pTexture = pTexture;
	newEntry.m_RenderSelected = selected;
	newEntry.m_pMapFace = pMapFace;
	newEntry.m_FaceSelectionState = faceSelectionState;
	g_OpaqueFaces.Insert( newEntry );
}


//-----------------------------------------------------------------------------
// draws a single face (displacement or normal)
//-----------------------------------------------------------------------------
void CMapFace::RenderFace( CRender3D* pRender, Vector& viewPoint, 
	RenderMode_t renderMode, bool renderSelected, SelectionState_t faceSelectionState )
{
	pRender->SetRenderMode( renderMode );
	if ( HasDisp() )
	{
		CMapDisp *pDisp = EditDispMgr()->GetDisp( m_DispHandle );
		pDisp->Render3D( pRender, renderSelected, faceSelectionState );
	}
	else
	{
		unsigned char color[4];
		ComputeColor( pRender, renderSelected, faceSelectionState, m_bIgnoreLighting, color );
		DrawFace( viewPoint, color, renderMode );
	}

	// Draw the texture axes
	if( renderMode == RENDER_MODE_WIREFRAME )
	{
		if (faceSelectionState != SELECT_NONE)
			RenderTextureAxes(pRender);

		// Draw the grid
		RenderGridIfCloseEnough( pRender, viewPoint );
	}
}


//-----------------------------------------------------------------------------
// renders queued up opaque faces, sorted by material
//-----------------------------------------------------------------------------
void CMapFace::RenderOpaqueFaces( CRender3D* pRender )
{
	Vector viewPoint;
	pRender->GetViewPoint(viewPoint);

	int i = g_OpaqueFaces.FirstInorder();
	while (i != g_OpaqueFaces.InvalidIndex())
	{
		MapFaceRender_t& mapFace = g_OpaqueFaces[i];
		if ( mapFace.m_RenderMode == RENDER_MODE_TEXTURED)
			pRender->BindTexture( mapFace.m_pTexture );

		mapFace.m_pMapFace->RenderFace( pRender, viewPoint, mapFace.m_RenderMode,
			mapFace.m_RenderSelected, mapFace.m_FaceSelectionState );

		i = g_OpaqueFaces.NextInorder(i);
	}

	g_OpaqueFaces.RemoveAll();

	// Restore the render mode
	pRender->SetRenderMode(RENDER_MODE_DEFAULT);
}


//-----------------------------------------------------------------------------
// Purpose: Renders this face using the given 3D renderer.
// Input  : pRender - Renderer to draw with. 
//-----------------------------------------------------------------------------
void CMapFace::Render3D(CRender3D *pRender)
{
	if (nPoints == 0)
	{
		return;
	}

	Vector viewPoint;
	pRender->GetViewPoint(viewPoint);

	//
	// Skip back faces unless rendering in wireframe.
	//
	RenderMode_t eCurrentRenderMode = pRender->GetCurrentRenderMode();

	SelectionState_t eFaceSelectionState = GetSelectionState();
	SelectionState_t eSolidSelectionState;
	if (m_pParent != NULL)
	{
		eSolidSelectionState = m_pParent->GetSelectionState();
	}
	else
	{
		eSolidSelectionState = eFaceSelectionState;
	}

	//
	// Draw the face.
	//
	bool renderSelected = ( eSolidSelectionState != SELECT_NONE );
	renderSelected = renderSelected || ( ( eFaceSelectionState != SELECT_NONE ) && (CMapFace::m_bShowFaceSelection) );

	if (pRender->DeferRendering())
	{
		AddFaceToQueue( this, m_pTexture, eCurrentRenderMode, renderSelected, eFaceSelectionState );
		if (renderSelected && pRender->NeedsOverlay())
		{
			AddFaceToQueue( this, m_pTexture, RENDER_MODE_SELECTION_OVERLAY, renderSelected, eFaceSelectionState );
		}
	}
	else
	{
		// Set up the texture to use
		pRender->BindTexture( m_pTexture );

		RenderFace( pRender, viewPoint, eCurrentRenderMode, renderSelected, eFaceSelectionState );
		if (renderSelected && pRender->NeedsOverlay())
		{
			RenderFace( pRender, viewPoint, RENDER_MODE_SELECTION_OVERLAY, renderSelected, eFaceSelectionState );
		}
    }

	// Restore the render mode
	pRender->SetRenderMode(eCurrentRenderMode);
}


//-----------------------------------------------------------------------------
// Purpose: Renders the world grid projected onto the given face.
// Input  : pFace - The face onto which the grid will be projected.
//-----------------------------------------------------------------------------
void CMapFace::Render3DGrid(CRender3D *pRender)
{
	//
	// Determine the extents of this face.
	//
	Extents_t Extents;
	float fDelta[3];
	float fGridSpacing = pRender->GetGridSize();

	GetFaceExtents(Extents);

	fDelta[0] = Extents[EXTENTS_XMAX][0] - Extents[EXTENTS_XMIN][0];
	fDelta[1] = Extents[EXTENTS_YMAX][1] - Extents[EXTENTS_YMIN][1];
	fDelta[2] = Extents[EXTENTS_ZMAX][2] - Extents[EXTENTS_ZMIN][2];

	//
	// Render the grid lines with wireframe material.
	//
	pRender->SetRenderMode( RENDER_MODE_WIREFRAME );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();

	//
	// For every dimension in which this face has a nonzero projection.
	//
	for (int nDim = 0; nDim < 3; nDim++)
	{
		if (fDelta[nDim] != 0)
		{
			Vector Normal;

			Normal[0] = (float)((nDim  % 3) == 0);
			Normal[1] = (float)((nDim  % 3) == 1);
			Normal[2] = (float)((nDim  % 3) == 2);

			float fMin = Extents[nDim * 2][nDim];
			float fMax = Extents[(nDim * 2) + 1][nDim];

			float fStart = (float)(floor(fMin / fGridSpacing) * fGridSpacing);
			float fEnd = (float)(ceil(fMax / fGridSpacing) * fGridSpacing);

			float fGridPoint = fStart;

			while (fGridPoint < fEnd)
			{
				int nPointsFound = 0;

				//
				// For every edge.
				//
				for (int nPoint = 0; nPoint < nPoints; nPoint++)
				{
					Vector PointFound[2];

					//
					// Get the start and end points of the edge.
					//
					Vector Point1 = Points[nPoint];

					Vector Point2;
					if (nPoint < nPoints - 1)
					{
						Point2 = Points[nPoint + 1];
					}
					else
					{
						Point2 = Points[0];
					}

					// 
					// If there is a projection of the normal vector along this edge.
					//
					if (Point2[nDim] != Point1[nDim])
					{
						//
						// Solve for the point along this edge that intersects the grid line
						// as a parameter from zero to one.
						//
						float fScale = (fGridPoint - Point1[nDim]) / (Point2[nDim] - Point1[nDim]);
						if ((fScale >= 0) && (fScale <= 1))
						{
							PointFound[nPointsFound][0] = Point1[0] + (Point2[0] - Point1[0]) * fScale;
							PointFound[nPointsFound][1] = Point1[1] + (Point2[1] - Point1[1]) * fScale;
							PointFound[nPointsFound][2] = Point1[2] + (Point2[2] - Point1[2]) * fScale;

							nPointsFound++;

							if (nPointsFound == 2)
							{
								Vector RenderPoint;

								meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

								VectorMA(PointFound[0], 0.2, plane.normal, RenderPoint);
								meshBuilder.Position3f(RenderPoint[0], RenderPoint[1], RenderPoint[2]);
								meshBuilder.Color3ub(Normal[0] * 255, Normal[1] * 255, Normal[2] * 255);
								meshBuilder.AdvanceVertex();

								VectorMA(PointFound[1], 0.2, plane.normal, RenderPoint);
								meshBuilder.Position3f(RenderPoint[0], RenderPoint[1], RenderPoint[2]);
								meshBuilder.Color3ub(Normal[0] * 255, Normal[1] * 255, Normal[2] * 255);
								meshBuilder.AdvanceVertex();

								meshBuilder.End();
								pMesh->Draw();

								nPointsFound = 0;
							}
						}
					}
				}
			
				fGridPoint += fGridSpacing;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pLoadInfo - 
//			pFace - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapFace::LoadDispInfoCallback(CChunkFile *pFile, CMapFace *pFace)
{
	// allocate a displacement (for the face)
	EditDispHandle_t dispHandle = EditDispMgr()->Create();
	CMapDisp *pDisp = EditDispMgr()->GetDisp( dispHandle );	

	//
	// load the displacement info and set relationships
	//
	ChunkFileResult_t eResult = pDisp->LoadVMF( pFile );
	if( eResult == ChunkFile_Ok )
	{
		pDisp->SetParent( pFace );
		pFace->SetDisp( dispHandle );

		CMapWorld *pWorld = GetActiveWorld();
		if( pWorld )
		{
			IWorldEditDispMgr *pDispMgr = pWorld->GetWorldEditDispManager();
			if( pDispMgr )
			{
				pDispMgr->AddToWorld( dispHandle );
			}
		}
	}

	return( eResult );
}


//-----------------------------------------------------------------------------
// Purpose: Handles key values when loading a VMF file.
// Input  : szKey - Key being handled.
//			szValue - Value of the key in the VMF file.
// Output : Returns ChunkFile_Ok or an error if there was a parsing error.
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapFace::LoadKeyCallback(const char *szKey, const char *szValue, LoadFace_t *pLoadFace)
{
	CMapFace *pFace = pLoadFace->pFace;

	if (!stricmp(szKey, "id"))
	{
		CChunkFile::ReadKeyValueInt(szValue, pFace->m_nFaceID);
	}
	else if (!stricmp(szKey, "rotation"))
	{
		pFace->texture.rotate = atof(szValue);
	}
	else if (!stricmp(szKey, "plane"))
	{
		int nRead = sscanf(szValue, "(%f %f %f) (%f %f %f) (%f %f %f)", 
			&pFace->plane.planepts[0][0], &pFace->plane.planepts[0][1], &pFace->plane.planepts[0][2],
			&pFace->plane.planepts[1][0], &pFace->plane.planepts[1][1], &pFace->plane.planepts[1][2],
			&pFace->plane.planepts[2][0], &pFace->plane.planepts[2][1], &pFace->plane.planepts[2][2]);

		if (nRead != 9)
		{
			// TODO: need specific error message
			return(ChunkFile_Fail);
		}
	}
	else if (!stricmp(szKey, "material"))
	{
		strcpy(pLoadFace->szTexName, szValue);
	}
	else if (!stricmp(szKey, "uaxis"))
	{
		int nRead = sscanf(szValue, "[%f %f %f %f] %f",
			&pFace->texture.UAxis[0], &pFace->texture.UAxis[1], &pFace->texture.UAxis[2], &pFace->texture.UAxis[3], &pFace->texture.scale[0]);

		if (nRead != 5)
		{
			// TODO: need specific error message
			return(ChunkFile_Fail);
		}
	}
	else if (!stricmp(szKey, "vaxis"))
	{
		int nRead = sscanf(szValue, "[%f %f %f %f] %f",
			&pFace->texture.VAxis[0], &pFace->texture.VAxis[1], &pFace->texture.VAxis[2], &pFace->texture.VAxis[3], &pFace->texture.scale[1]);

		if (nRead != 5)
		{
			// TODO: need specific error message
			return(ChunkFile_Fail);
		}
	}
	else if (!stricmp(szKey, "lightmapscale"))
	{
		pFace->texture.nLightmapScale = atoi(szValue);
	}
	else if (!stricmp(szKey, "smoothing_groups"))
	{
		pFace->m_fSmoothingGroups = atoi(szValue);
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: Loads a face chunk from the VMF file.
// Input  : pFile - Chunk file being loaded.
// Output : Returns ChunkFile_Ok or an error if there was a parsing error.
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapFace::LoadVMF(CChunkFile *pFile)
{
	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("dispinfo", (ChunkHandler_t)LoadDispInfoCallback, this);

	//
	// Read the keys and sub-chunks.
	//
	LoadFace_t LoadFace;
	memset(&LoadFace, 0, sizeof(LoadFace));
	LoadFace.pFace = this;

	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadKeyCallback, &LoadFace);
	pFile->PopHandlers();

	if (eResult == ChunkFile_Ok)
	{
		CalcPlane();
		SetTexture(LoadFace.szTexName);
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Called after this object is added to the world.
//
//			NOTE: This function is NOT called during serialization. Use PostloadWorld
//				  to do similar bookkeeping after map load.
//
// Input  : pWorld - The world that we have been added to.
//-----------------------------------------------------------------------------
void CMapFace::OnAddToWorld(CMapWorld *pWorld)
{
	if (HasDisp())
	{
		//
		// Add it to the world displacement list.
		//
		IWorldEditDispMgr *pDispMgr = GetActiveWorldEditDispManager();
		if (pDispMgr != NULL)
		{
			pDispMgr->AddToWorld( m_DispHandle );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called just after this object has been removed from the world so
//			that it can unlink itself from other objects in the world.
// Input  : pWorld - The world that we were just removed from.
//			bNotifyChildren - Whether we should forward notification to our children.
//-----------------------------------------------------------------------------
void CMapFace::OnRemoveFromWorld(void)
{
	if (HasDisp())
	{
		//
		// Add it to the world displacement list.
		//
		IWorldEditDispMgr *pDispMgr = GetActiveWorldEditDispManager();
		if (pDispMgr != NULL)
		{
			pDispMgr->RemoveFromWorld( m_DispHandle );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapFace::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	NormalizeTextureShifts();

	//
	// Check for duplicate plane points. All three plane points must be unique
	// or it isn't a valid plane. Try to fix it if it isn't valid.
	//
	if (!CheckFace())
	{
		Fix();
	}

	ChunkFileResult_t eResult = pFile->BeginChunk("side");

	char szBuf[512];

	//
	// Write our unique face ID.
	//
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("id", m_nFaceID);
	}

	//
	// Write the plane information.
	//
	if (eResult == ChunkFile_Ok)
	{
		sprintf(szBuf, "(%g %g %g) (%g %g %g) (%g %g %g)",
				(double)plane.planepts[0][0], (double)plane.planepts[0][1], (double)plane.planepts[0][2],
				(double)plane.planepts[1][0], (double)plane.planepts[1][1], (double)plane.planepts[1][2],
				(double)plane.planepts[2][0], (double)plane.planepts[2][1], (double)plane.planepts[2][2]);

		eResult = pFile->WriteKeyValue("plane", szBuf);
	}

	if (eResult == ChunkFile_Ok)
	{
		char szTexture[MAX_PATH];
		strcpy(szTexture, texture.texture);
		strupr(szTexture);

		eResult = pFile->WriteKeyValue("material", szTexture);
	}

	if (eResult == ChunkFile_Ok)
	{
		sprintf(szBuf, "[%g %g %g %g] %g", (double)texture.UAxis[0], (double)texture.UAxis[1], (double)texture.UAxis[2], (double)texture.UAxis[3], (double)texture.scale[0]);
		eResult = pFile->WriteKeyValue("uaxis", szBuf);
	}

	if (eResult == ChunkFile_Ok)
	{
		sprintf(szBuf, "[%g %g %g %g] %g", (double)texture.VAxis[0], (double)texture.VAxis[1], (double)texture.VAxis[2], (double)texture.VAxis[3], (double)texture.scale[1]);
		eResult = pFile->WriteKeyValue("vaxis", szBuf);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueFloat("rotation", texture.rotate);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueFloat("lightmapscale", texture.nLightmapScale);
	}

	// Save smoothing group data.
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("smoothing_groups", m_fSmoothingGroups );
	}

	//
	// Write the displacement chunk.
	//
	if ((eResult == ChunkFile_Ok) && (HasDisp()))
	{
		CMapDisp *pDisp = EditDispMgr()->GetDisp( m_DispHandle );
		eResult = pDisp->SaveVMF(pFile, pSaveInfo);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Enables or disables the special rendering of selected faces.
// Input  : bShowSelection - true to enable, false to disable.
//-----------------------------------------------------------------------------
void CMapFace::SetShowSelection(bool bShowSelection)
{
	CMapFace::m_bShowFaceSelection = bShowSelection;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nPoint - 
//			u - 
//			v - 
//-----------------------------------------------------------------------------
void CMapFace::SetTextureCoords(int nPoint, float u, float v)
{
	if (nPoint < nPoints)
	{
		m_pTextureCoords[nPoint][0] = u;
		m_pTextureCoords[nPoint][1] = v;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
size_t CMapFace::GetDataSize( void )
{
	// get base map class size
	size_t size = sizeof( CMapFace );
	
	//
	// better approximate by added in verts, texture coordinates, 
	// and lightmap coordinates
	//
	size += ( sizeof( Vector ) * nPoints );
	size += ( sizeof( Vector2D ) * ( nPoints * 2 ) );

	// add displacement size if necessary
	if( HasDisp() )
	{
		CMapDisp *pDisp = EditDispMgr()->GetDisp( m_DispHandle );
		size += pDisp->GetSize();
	}

	return size;
}


//-----------------------------------------------------------------------------
// Purpose: Returns our bounds for 2D rendering. These bounds do not consider
//			any displacement information.
// Input  : boundMin - 
//			boundMax - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMapFace::GetRender2DBox( Vector& boundMin, Vector& boundMax )
{
	// valid face?
	if( nPoints == 0 )
		return false;

	//
	// find min and maximum points on face
	//
	VectorFill( boundMin, COORD_NOTINIT );
	VectorFill( boundMax, -COORD_NOTINIT );
	for( int i = 0; i < nPoints; i++ )
	{
		if( Points[i][0] < boundMin[0] ) { boundMin[0] = Points[i][0]; }
		if( Points[i][1] < boundMin[1] ) { boundMin[1] = Points[i][1]; }
		if( Points[i][2] < boundMin[2] ) { boundMin[2] = Points[i][2]; }

		if( Points[i][0] > boundMax[0] ) { boundMax[0] = Points[i][0]; }
		if( Points[i][1] > boundMax[1] ) { boundMax[1] = Points[i][1]; }
		if( Points[i][2] > boundMax[2] ) { boundMax[2] = Points[i][2]; }
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns our bounds for frustum culling, including the bounds of
//			any displacement information.
// Input  : boundMin - 
//			boundMax - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMapFace::GetCullBox( Vector& boundMin, Vector& boundMax )
{
	// get the face bounds
	if( !GetRender2DBox( boundMin, boundMax ) )
		return false;

	//
	// add displacement to bounds
	//
	if( HasDisp() )
	{
		CMapDisp *pDisp = EditDispMgr()->GetDisp( m_DispHandle );

		Vector bbox[2];
		pDisp->GetBoundingBox( bbox[0], bbox[1] );

		for( int i = 0; i < 2; i++ )
		{
			if( bbox[i][0] < boundMin[0] ) { boundMin[0] = bbox[i][0]; }
			if( bbox[i][1] < boundMin[1] ) { boundMin[1] = bbox[i][1]; }
			if( bbox[i][2] < boundMin[2] ) { boundMin[2] = bbox[i][2]; }
			
			if( bbox[i][0] > boundMax[0] ) { boundMax[0] = bbox[i][0]; }
			if( bbox[i][1] > boundMax[1] ) { boundMax[1] = bbox[i][1]; }
			if( bbox[i][2] > boundMax[2] ) { boundMax[2] = bbox[i][2]; }
		}
	}

	return true;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : HitPos - 
//			Start - 
//			End - 
// Output : Returns true if the ray intersected the face, false if not.
//-----------------------------------------------------------------------------
bool CMapFace::TraceLine(Vector &HitPos, Vector &HitNormal, Vector const &Start, Vector const &End )
{
	if (HasDisp())
	{
		CMapDisp *pDisp = EditDispMgr()->GetDisp( m_DispHandle );
		return( pDisp->TraceLine( HitPos, HitNormal, Start, End ) );
	}

	//
	// Find the point of intersection of the ray with the given plane.
	//
	float t = Start.Dot(plane.normal) - plane.dist;
	t = t / -(End - Start).Dot(plane.normal);
	
	HitPos = Start + (t * (End - Start));
	HitNormal = plane.normal;
	return(true);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapFace::TraceLineInside( Vector &HitPos, Vector &HitNormal, 
							    Vector const &Start, Vector const &End, bool bNoDisp )
{
	// if the face is displaced -- collide with that
	if( HasDisp() && !bNoDisp )
	{
		CMapDisp *pDisp = EditDispMgr()->GetDisp( m_DispHandle );
		return( pDisp->TraceLine( HitPos, HitNormal, Start, End ) );
	}

	//
	// Find the point of intersection of the ray with the given plane.
	//
	float t = Start.Dot( plane.normal ) - plane.dist;
	t = t / -( End - Start ).Dot( plane.normal );	
	HitPos = Start + ( t * ( End - Start ) );

	//
	// determine if the plane point lies behind all of the polygon planes (edges)
	//
	for( int ndxEdge = 0; ndxEdge < nPoints; ndxEdge++ )
	{
		// create the edge and cross it with the face plane normal
		Vector edge;
		edge = Points[(ndxEdge+1)%nPoints] - Points[ndxEdge];

		PLANE edgePlane;
		edgePlane.normal = edge.Cross( plane.normal );
		VectorNormalize( edgePlane.normal );
		edgePlane.dist = edgePlane.normal.Dot( Points[ndxEdge] );

		// determine if the facing is correct
		float dist = edgePlane.normal.Dot( Points[(ndxEdge+2)%nPoints] ) - edgePlane.dist;
		if( dist > 0.0f )
		{
			// flip
			edgePlane.normal.Negate();
			edgePlane.dist = -edgePlane.dist;
		}
		
		// check to see if plane point lives behind the plane
		dist = edgePlane.normal.Dot( HitPos ) - edgePlane.dist;
		if( dist > 0.0f )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// NOTE: actually this could be calculated once for the face since only the
//       face normal is being used (no smoothing groups, etc.), but that may 
//       change????
//-----------------------------------------------------------------------------
void CMapFace::CalcTangentSpaceAxes( void )
{
	// destroy old axes if need be
	FreeTangentSpaceAxes();

	// allocate memory for tangent space axes
	if( !AllocTangentSpaceAxes( nPoints ) )
		return;

	//
	// get the texture space axes
	//
	Vector4D& uVect = texture.UAxis;
	Vector4D& vVect = texture.VAxis;

	//
	// calculate the tangent space per polygon point
	//

	for( int ptIndex = 0; ptIndex < nPoints; ptIndex++ )
	{
		// get the current tangent space axes
		TangentSpaceAxes_t *pAxis = &m_pTangentAxes[ptIndex];

		//
		// create the axes
		//
		pAxis->binormal = vVect.AsVector3D();
		VectorNormalize( pAxis->binormal );
		CrossProduct( plane.normal, pAxis->binormal, pAxis->tangent );
		VectorNormalize( pAxis->tangent );
		CrossProduct( pAxis->tangent, plane.normal, pAxis->binormal );
		VectorNormalize( pAxis->binormal );

		//
		// adjust tangent for "backwards" mapping if need be
		//
		Vector tmpVect;
		CrossProduct( uVect.AsVector3D(), vVect.AsVector3D(), tmpVect );
		if( DotProduct( plane.normal, tmpVect ) > 0.0f )
		{
			VectorScale( pAxis->tangent, -1.0f, pAxis->tangent );
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapFace::AllocTangentSpaceAxes( int count )
{
	if( count < 1 )
		return false;

	m_pTangentAxes = new TangentSpaceAxes_t[count];
	if( !m_pTangentAxes )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapFace::FreeTangentSpaceAxes( void )
{
	if( m_pTangentAxes )
	{
		delete [] m_pTangentAxes;
		m_pTangentAxes = NULL;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CMapFace::SmoothingGroupCount( void )
{
	int nCount = 0;
	for ( int iGroup = 0; iGroup < SMOOTHING_GROUP_MAX_COUNT; ++iGroup )
	{
		if ( ( m_fSmoothingGroups & ( 1 << iGroup ) ) != 0 )
		{
			nCount++;
		}
	}

	return nCount;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapFace::AddSmoothingGroup( int iGroup )
{
	Assert( iGroup >= 0 );
	Assert( iGroup < SMOOTHING_GROUP_MAX_COUNT );

	m_fSmoothingGroups |= ( 1 << ( iGroup - 1 ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMapFace::RemoveSmoothingGroup( int iGroup )
{
	Assert( iGroup >= 0 );
	Assert( iGroup < SMOOTHING_GROUP_MAX_COUNT );

	m_fSmoothingGroups &= ~( 1 << ( iGroup - 1 ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CMapFace::InSmoothingGroup( int iGroup )
{
	if ( ( m_fSmoothingGroups & ( 1 << ( iGroup - 1 ) ) ) != 0 )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Performs an intersection of this list with another.
// Input  : IntersectWith - the list to intersect with.
//			In - the list of items that were in both lists
//			Out - the list of items that were in one list but not the other.
//-----------------------------------------------------------------------------
void CMapFaceList::Intersect(CMapFaceList &IntersectWith, CMapFaceList &In, CMapFaceList &Out)
{
	//
	// See what we items have that are in the other list.
	//
	for (int i = 0; i < Count(); i++)
	{
		CMapFace *pFace = Element(i);

		if (IntersectWith.Find(pFace) != -1)
		{
			if (In.Find(pFace) == -1)
			{
				In.AddToTail(pFace);
			}
		}
		else
		{
			if (Out.Find(pFace) == -1)
			{
				Out.AddToTail(pFace);
			}
		}
	}

	//
	// Now go the other way.
	//
	for (i = 0; i < IntersectWith.Count(); i++)
	{
		CMapFace *pFace = IntersectWith.Element(i);

		if (Find(pFace) != -1)
		{
			if (In.Find(pFace) == -1)
			{
				In.AddToTail(pFace);
			}
		}
		else
		{
			if (Out.Find(pFace) == -1)
			{
				Out.AddToTail(pFace);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Performs an intersection of this list with another.
// Input  : IntersectWith - the list to intersect with.
//			In - the list of items that were in both lists
//			Out - the list of items that were in one list but not the other.
//-----------------------------------------------------------------------------
void CMapFaceIDList::Intersect(CMapFaceIDList &IntersectWith, CMapFaceIDList &In, CMapFaceIDList &Out)
{
	//
	// See what we items have that are in the other list.
	//
	for (int i = 0; i < Count(); i++)
	{
		int nFaceID = Element(i);
		if (IntersectWith.Find(nFaceID) != -1)
		{
			if (In.Find(nFaceID) == -1)
			{
				In.AddToTail(nFaceID);
			}
		}
		else
		{
			if (Out.Find(nFaceID) == -1)
			{
				Out.AddToTail(nFaceID);
			}
		}
	}

	//
	// Now go the other way.
	//
	for (i = 0; i < IntersectWith.Count(); i++)
	{
		int nFaceID = IntersectWith.Element(i);

		if (Find(nFaceID) != -1)
		{
			if (In.Find(nFaceID) == -1)
			{
				In.AddToTail(nFaceID);
			}
		}
		else
		{
			if (Out.Find(nFaceID) == -1)
			{
				Out.AddToTail(nFaceID);
			}
		}
	}
}

