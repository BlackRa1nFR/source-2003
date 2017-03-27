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
//=============================================================================
#include "cbase.h"
#include "smoke_fog_overlay.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMesh.h"

static IMaterial *g_pSmokeFogMaterial = NULL;


float		g_SmokeFogOverlayAlpha;
Vector		g_SmokeFogOverlayColor;


void InitSmokeFogOverlay()
{
	TermSmokeFogOverlay();
	
	g_SmokeFogOverlayAlpha = 0;

	if(materials)
	{
		g_pSmokeFogMaterial = materials->FindMaterial("particle/screenspace_fog", 0, 0);
		if(g_pSmokeFogMaterial)
			g_pSmokeFogMaterial->IncrementReferenceCount();
	}
}


void TermSmokeFogOverlay()
{
	if(g_pSmokeFogMaterial)
	{
		g_pSmokeFogMaterial->DecrementReferenceCount();
		g_pSmokeFogMaterial = NULL;
	}
}


void DrawSmokeFogOverlay()
{
	if(g_SmokeFogOverlayAlpha == 0 || !g_pSmokeFogMaterial || !materials)
		return;

	materials->MatrixMode(MATERIAL_MODEL);
	materials->LoadIdentity();

	materials->MatrixMode(MATERIAL_VIEW);
	materials->LoadIdentity();

	IMesh* pMesh = materials->GetDynamicMesh( true, NULL, NULL, g_pSmokeFogMaterial );
	CMeshBuilder meshBuilder;

	static float dist = 10;

	Vector vColor = g_SmokeFogOverlayColor;
	vColor.x = min(max(vColor.x, 0), 1);
	vColor.y = min(max(vColor.y, 0), 1);
	vColor.z = min(max(vColor.z, 0), 1);
	float alpha = min(max(g_SmokeFogOverlayAlpha, 0), 1);

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( -dist, -dist, -dist );
	meshBuilder.Color4f( vColor.x, vColor.y, vColor.z, alpha );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( -dist,  dist, -dist );
	meshBuilder.Color4f( vColor.x, vColor.y, vColor.z, alpha );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(  dist,  dist, -dist );
	meshBuilder.Color4f( vColor.x, vColor.y, vColor.z, alpha );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(  dist, -dist, -dist );
	meshBuilder.Color4f( vColor.x, vColor.y, vColor.z, alpha );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}
