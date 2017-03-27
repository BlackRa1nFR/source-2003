
#include "iscratchpad3d.h"
#include "mathlib.h"


void ScratchPad_DrawLitCone( 
	IScratchPad3D *pPad,
	const Vector &vBaseCenter,
	const Vector &vTip,
	const Vector &vBrightColor,
	const Vector &vDarkColor,
	const Vector &vLightDir,
	float baseWidth,
	int nSegments )
{
	// Make orthogonal vectors.
	Vector vDir = vTip - vBaseCenter;
	VectorNormalize( vDir );

	Vector vRight, vUp;
	VectorVectors( vDir, vRight, vUp );
	vRight *= baseWidth;
	vUp *= baseWidth;

	// Setup the top and bottom caps.
	CSPVertList bottomCap, tri;
	bottomCap.m_Verts.SetSize( nSegments );
	tri.m_Verts.SetSize( 3 );

	float flDot = -vLightDir.Dot( vDir );
	Vector topColor, bottomColor;
	VectorLerp( vDarkColor, vBrightColor, RemapVal( -flDot, -1, 1, 0, 1 ), bottomColor );

	
	// Draw each quad.
	Vector vPrevBottom = vBaseCenter + vRight;
	
	for ( int i=0; i < nSegments; i++ )
	{
		float flAngle = (float)(i+1) * M_PI * 2.0 / nSegments;
		Vector vOffset = vRight * cos( flAngle ) + vUp * sin( flAngle );
		Vector vCurBottom = vBaseCenter + vOffset;

		const Vector &v1 = vTip;
		const Vector &v2 = vPrevBottom;
		const Vector &v3 = vCurBottom;
		Vector vFaceNormal = (v2 - v1).Cross( v3 - v1 );
		VectorNormalize( vFaceNormal );

		// Now light it.
		flDot = -vLightDir.Dot( vFaceNormal );
		Vector vColor;
		VectorLerp( vDarkColor, vBrightColor, RemapVal( flDot,  -1, 1, 0, 1 ), vColor );

		// Draw the quad.
		tri.m_Verts[0] = CSPVert( v1, vColor );
		tri.m_Verts[1] = CSPVert( v2, vColor );
		tri.m_Verts[2] = CSPVert( v3, vColor );
		pPad->DrawPolygon( tri );

		bottomCap.m_Verts[i] = CSPVert( vCurBottom, bottomColor );
	}

	pPad->DrawPolygon( bottomCap );
}


void ScratchPad_DrawLitCylinder( 
	IScratchPad3D *pPad,
	const Vector &v1,
	const Vector &v2,
	const Vector &vBrightColor,
	const Vector &vDarkColor,
	const Vector &vLightDir,
	float width,
	int nSegments )
{
	// Make orthogonal vectors.
	Vector vDir = v2 - v1;
	VectorNormalize( vDir );

	Vector vRight, vUp;
	VectorVectors( vDir, vRight, vUp );
	vRight *= width;
	vUp *= width;

	// Setup the top and bottom caps.
	CSPVertList topCap, bottomCap, quad;
	
	topCap.m_Verts.SetSize( nSegments );
	bottomCap.m_Verts.SetSize( nSegments );
	quad.m_Verts.SetSize( 4 );

	float flDot = -vLightDir.Dot( vDir );
	Vector topColor, bottomColor;

	VectorLerp( vDarkColor, vBrightColor, RemapVal( flDot,  -1, 1, 0, 1 ), topColor );
	VectorLerp( vDarkColor, vBrightColor, RemapVal( -flDot, -1, 1, 0, 1 ), bottomColor );

	
	// Draw each quad.
	Vector vPrevTop = v1 + vRight;
	Vector vPrevBottom = v2 + vRight;
	
	for ( int i=0; i < nSegments; i++ )
	{
		float flAngle = (float)(i+1) * M_PI * 2.0 / nSegments;
		Vector vOffset = vRight * cos( flAngle ) + vUp * sin( flAngle );
		Vector vCurTop = v1 + vOffset;
		Vector vCurBottom = v2 + vOffset;

		// Now light it.
		VectorNormalize( vOffset );
		flDot = -vLightDir.Dot( vOffset );
		Vector vColor;
		VectorLerp( vDarkColor, vBrightColor, RemapVal( flDot,  -1, 1, 0, 1 ), vColor );

		// Draw the quad.
		quad.m_Verts[0] = CSPVert( vPrevTop, vColor );
		quad.m_Verts[1] = CSPVert( vPrevBottom, vColor );
		quad.m_Verts[2] = CSPVert( vCurBottom, vColor );
		quad.m_Verts[3] = CSPVert( vCurTop, vColor );
		pPad->DrawPolygon( quad );

		topCap.m_Verts[i] = CSPVert( vCurTop, topColor );
		bottomCap.m_Verts[i] = CSPVert( vCurBottom, bottomColor );
	}

	pPad->DrawPolygon( topCap );
	pPad->DrawPolygon( bottomCap );
}


void ScratchPad_DrawArrow( 
	IScratchPad3D *pPad,
	const Vector &vPos, 
	const Vector &vDirection,
	const Vector &vColor, 
	float flLength=20, 
	float flLineWidth=3,
	float flHeadWidth=8,
	int nCylinderSegments=5,
	int nHeadSegments=8,
	float flArrowHeadPercentage = 0.3f	// How much of the line is the arrow head.
	)
{
	Vector vNormDir = vDirection;
	VectorNormalize( vNormDir );
	
	Vector vConeBase = vPos + vNormDir * (flLength * ( 1 - flArrowHeadPercentage ) );
	Vector vConeEnd = vPos + vNormDir * flLength;
	
	Vector vLightDir( -1, -1, -1 );
	VectorNormalize( vLightDir ); // could precalculate this

	pPad->SetRenderState( IScratchPad3D::RS_FillMode, IScratchPad3D::FillMode_Solid );
	pPad->SetRenderState( IScratchPad3D::RS_ZRead, true );

	ScratchPad_DrawLitCylinder( pPad, vPos, vConeBase, vColor, vColor*0.25f, vLightDir, flLineWidth, nCylinderSegments );
	ScratchPad_DrawLitCone( pPad, vConeBase, vConeEnd, vColor, vColor*0.25f, vLightDir, flHeadWidth, nHeadSegments );
}


void ScratchPad_DrawArrowSimple( 
	IScratchPad3D *pPad,
	const Vector &vPos, 
	const Vector &vDirection,
	const Vector &vColor, 
	float flLength )
{
	ScratchPad_DrawArrow(
		pPad, 
		vPos,
		vDirection,
		vColor,
		flLength,
		flLength * 1.0/15,
		flLength * 3.0/15,
		4,
		4 );
}


