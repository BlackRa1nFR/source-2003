
#ifndef __HELPERS_H__
#define __HELPERS_H__


	#include <stdlib.h>
	#include <math.h>
	#include "vector.h"


	#ifndef DEG2RAD
		#define DEG2RAD(x)		((x) * (3.141592653589793f / 180.0f))
	#endif

	#ifndef RAD2DEG
		#define RAD2DEG(x)		((x) * (180.0f / 3.141592653589793f))
	#endif

	extern float g_FastBiasPrecalc;


	inline int RoundFloatToInt(float f)
	{
		int nResult;

		__asm
		{
			fld f
			fistp nResult
		}
		return nResult;
	}

	inline float CosDegrees(float angle)
	{
		return (float)cos(DEG2RAD(angle));
	}

	inline float SinDegrees(float angle)
	{
		return (float)sin(DEG2RAD(angle));
	}

	inline float FSqr(float a)
	{
		return a * a;
	}

	// Swap a and b.
	inline void FSwap(float &a, float &b)
	{
		float temp;
		temp = a;
		a = b;
		b = temp;
	}
	
	// Returns -1 for negative numbers, 1 for positive numbers.
	inline float FSign(float val)
	{
		return val > 0.0f ? 1.0f : -1.0f;
	}

	inline float FClamp(float val, float min, float max)
	{
		return (val < min) ? min : (val > max ? max : val);
	}

	inline float FMin(float val1, float val2)
	{
		return (val1 < val2) ? val1 : val2;
	}

	inline float FMax(float val1, float val2)
	{
		return (val1 > val2) ? val1 : val2;
	}

	inline void SetupFastBias(float bias)
	{
		g_FastBiasPrecalc = (float)(log(bias) / log(0.5f));
	}

	inline float FastBias(float x)
	{
		return (float)pow(x, g_FastBiasPrecalc);
	}


	// Gain functions.
	inline void SetupFastGain(float gain)
	{
		SetupFastBias(1.0f - gain);
	}

	inline float FastGain(float x)
	{
		if(x < 0.5f)
		{
			return FastBias(2.0f*x) * 0.5f;
		}
		else
		{
			return FastBias(2.0f - 2.0f*x) * 0.5f;
		}
	}


	// Get the intersections of a ray and a sphere (if they exist).
	bool LineSphereIntersection(
		const Vector &vSphereCenter,
		const float fSphereRadius,
		const Vector &vLinePt,
		const Vector &vLineDir,
		float &fIntersection1,
		float &fIntersection2);

#endif

