
#include "helpers.h"


float g_FastBiasPrecalc=0.0f;


bool LineSphereIntersection(
	const Vector &vSphereCenter,
	const float fSphereRadius,
	const Vector &vLinePt,
	const Vector &vLineDir,
	float &fIntersection1,
	float &fIntersection2)
{
	// Line = P + Vt
	// Sphere = r (assume we've translated to origin)
	// (P + Vt)^2 = r^2
	// VVt^2 + 2PVt + (PP - r^2)
	// Solve as quadratic:  (-b  +/-  sqrt(b^2 - 4ac)) / 2a
	// If (b^2 - 4ac) is < 0 there is no solution.
	// If (b^2 - 4ac) is = 0 there is one solution (a case this function doesn't support).
	// If (b^2 - 4ac) is > 0 there are two solutions.
	Vector P;
	float a, b, c, sqr, insideSqr;


	// Translate sphere to origin.
	P = vLinePt - vSphereCenter;
	a = vLineDir.Dot(vLineDir);
	b = 2.0f * P.Dot(vLineDir);
	c = P.Dot(P) - (fSphereRadius * fSphereRadius);

	insideSqr = b*b - 4*a*c;
	if(insideSqr <= 0.000001f)
		return false;

	// Ok, two solutions.
	sqr = (float)sqrt(insideSqr);
	
	fIntersection1 = (-b + sqr) / (2.0f * a);
	fIntersection2 = (-b - sqr) / (2.0f * a);

	return true;
}


