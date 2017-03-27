//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Render2D.h"
#include "RenderUtils.h"


//-----------------------------------------------------------------------------
// Purpose: Draws the measurements of a brush in the 2D view.
// Input  : pRender -
//			Mins - 
//			Maxs -
//			nFlags - 
//-----------------------------------------------------------------------------
void DrawBoundsText(CRender2D *pRender, const Vector &Mins, const Vector &Maxs, int nFlags)
{
	int axHorz = pRender->GetAxisHorz();
	int axVert = pRender->GetAxisVert();

	//
	// Calculate the solid's extents along our 2D view axes.
	//
	Vector Extents;
	Extents[axHorz] = fabs(Maxs[axHorz] - Mins[axHorz]);
	Extents[axVert] = fabs(Maxs[axVert] - Mins[axVert]);

	//
	// Transform the solids mins and maxs to 2D view space. These are used
	// for placing the text in the view.
	//
    Vector2D projMins, projMaxs;
    pRender->TransformPoint3D( projMins, Mins );
    pRender->TransformPoint3D( projMaxs, Maxs );

    if( projMins[0] > projMaxs[0] )
    {
        int temp = projMins[0];
        projMins[0] = projMaxs[0];
        projMaxs[0] = temp;
    }

    if( projMins[1] > projMaxs[1] )
    {
        int temp = projMins[1];
        projMins[1] = projMaxs[1];
        projMaxs[1] = temp;
    }

    //
    // display the extents of this brush
    //
    char extentText[30];
    CPoint textPos;
    pRender->SetTextColor( 255, 255, 255, 0, 0, 0 );

    // horz
    sprintf( extentText, "%.2f", Extents[axHorz] );
    textPos.x = projMins[0];
    int textWidth = projMaxs[0] - projMins[0];
	textPos.y = ( nFlags & DBT_TOP ) ? ( projMins[1] - 30 ) : ( projMaxs[1] + 5 );
    pRender->DrawText( extentText, textPos, textWidth, 0, 
                       CRender2D::TEXT_JUSTIFY_HORZ_CENTER | CRender2D::TEXT_SINGLELINE );

    // vert
    sprintf( extentText, "%.2f", Extents[axVert] );
    textPos.x = ( nFlags & DBT_LEFT ) ? ( projMins[0] - 80 ) : ( projMaxs[0] + 5 );
    textWidth = ( nFlags & DBT_LEFT ) ? ( projMins[0] - 5 ) : ( projMaxs[0] + 80 );
    textWidth -= textPos.x;
    textPos.y = projMins[1];
    int textHeight = projMaxs[1] - projMins[1];
    pRender->DrawText( extentText, textPos, textWidth, textHeight,
                       CRender2D::TEXT_JUSTIFY_LEFT | CRender2D::TEXT_JUSTIFY_VERT_CENTER |
                       CRender2D::TEXT_SINGLELINE );
}


