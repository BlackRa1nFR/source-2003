//
// CTransform implementation
//

#include "stdafx.h"
#include "Transform.h"

void CTransform::TransformPoint(float *pt3)
{
	if(Mode == rotate)
	{
		float & ptHorz = pt3[axHorz];
		float & ptVert = pt3[axVert];
		float fX, fY;

		// find rotate angle
		rotate_coords(&fX, &fY, ptHorz - ptRef[axHorz], 
			ptVert - ptRef[axVert], fAngle);
		ptHorz = fX + ptRef[axHorz];
		ptVert = fY + ptRef[axVert];

		return;
	}

	if(Mode == shear)
	{
		if(
	}
}