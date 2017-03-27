#ifndef _TRANSFORM_H
#define _TRANSFORM_H

class CTransform
{
public:
	void TransformPoint(float * pt3);

	enum
	{
		rotate,
		shear,
		scale,
		move
	} Mode;

	float ptRef[3];
	float ptOrg[3];

	enum
	{
		scMinX = 0x01,
		scMaxX = 0x02,
		scMinY = 0x04,
		scMaxY = 0x08,
		scMinZ = 0x10,
		scMaxZ = 0x20
	};

	union
	{
		float fAngle;
		float fSlope;
		DWORD uTransOrg;
	}; 

	int axHorz, axVert, axThird;
}; 

#endif