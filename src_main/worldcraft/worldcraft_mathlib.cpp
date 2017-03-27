//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements math functions specific to Worldcraft.
//
// $NoKeywords: $
//=============================================================================

#include "worldcraft_mathlib.h"
#include <assert.h>
#include <string.h>
#include <Windows.h>

// provide implementation for mathlib Sys_Error()
extern void Error(char* fmt, ...);
extern "C" void Sys_Error( char *error, ... )
{
	Error( "%s", error );
}

void polyMake( float x1, float  y1, float x2, float y2, int npoints, float start_ang, Vector *pmPoints )
{
    int		point;
    double  angle = start_ang, angle_delta = 360.0 / (double) npoints;
    double  xrad = (x2-x1) / 2, yrad = (y2-y1) / 2;

	// make centerpoint for polygon:
    float xCenter = x1 + xrad;
    float yCenter = y1 + yrad;

    for( point = 0; point < npoints; point++, angle += angle_delta )
    {
        if( angle > 360 )
            angle -= 360;

        pmPoints[point][0] = rint(xCenter + (sin(DEG2RAD(angle)) * (float)xrad));
        pmPoints[point][1] = rint(yCenter + (cos(DEG2RAD(angle)) * (float)yrad));
    }

    pmPoints[point][0] = pmPoints[0][0];
    pmPoints[point][1] = pmPoints[0][1];
}


float fixang(float a)
{
	if(a < 0.0)
		return a+360.0;
	if(a > 359.9)
		return a-360.0;

	return a;
}


float lineangle(float x1, float y1, float x2, float y2)
{
    float x, y;
	float rvl;

    x = x2 - x1;
    y = y2 - y1;

    if(!x && !y)
        return 0.0;

    rvl = RAD2DEG(atan2( y, x ));

    return (rvl);
}


float rint(float f)
{
	if (f > 0.0f) {
		return (float) floor(f + 0.5f);
	} else if (f < 0.0f) {
		return (float) ceil(f - 0.5f);
	} else
		return 0.0f;
}


void rotate_coords(float *dx, float *dy, float sx, float sy, double angle)
{
    double r, theta;

    if( !sx && !sy )
    {
        *dx = 0.0;
        *dy = 0.0;
        return;
    }

	// special cases
	if(angle == 0.0 || angle == 360.0)
	{
		dx[0] = sx;
		dy[0] = sy;
		return;
	}
	else if(angle == 90.0)
	{
		dx[0] = -sy;
		dy[0] = sx;
		return;
	}
	else if(angle == 180.0)
	{
		dx[0] = -sx;
		dy[0] = -sy;
		return;
	}
	else if(angle == 270.0)
	{
		dx[0] = sy;
		dy[0] = -sx;
		return;
	}

    angle *= 0.0174533;

    r = hypot( sx, sy );
    theta = atan2( sy, sx );
    *dx = r * cos( theta + angle );
    *dy = r * sin( theta + angle );
}


//-----------------------------------------------------------------------------
// Purpose: Builds the matrix for a counterclockwise rotation about an arbitrary axis.
//
//		   | ax2 + (1 - ax2)cosQ		axay(1 - cosQ) - azsinQ		azax(1 - cosQ) + aysinQ |
// Ra(Q) = | axay(1 - cosQ) + azsinQ	ay2 + (1 - ay2)cosQ			ayaz(1 - cosQ) - axsinQ |
//		   | azax(1 - cosQ) - aysinQ	ayaz(1 - cosQ) + axsinQ		az2 + (1 - az2)cosQ     |
//          
// Input  : Matrix - 
//			Axis - 
//			fAngle - 
//-----------------------------------------------------------------------------
void AxisAngleMatrix(matrix4_t& Matrix, const Vector& Axis, float fAngle)
{
	float fRadians;
	float fAxisXSquared;
	float fAxisYSquared;
	float fAxisZSquared;
	float fSin;
	float fCos;

	fRadians = fAngle * M_PI / 180.0;

	fSin = sin(fRadians);
	fCos = cos(fRadians);

	fAxisXSquared = Axis[0] * Axis[0];
	fAxisYSquared = Axis[1] * Axis[1];
	fAxisZSquared = Axis[2] * Axis[2];

	// Column 0:
	Matrix[0][0] = fAxisXSquared + (1 - fAxisXSquared) * fCos;
	Matrix[1][0] = Axis[0] * Axis[1] * (1 - fCos) + Axis[2] * fSin;
	Matrix[2][0] = Axis[2] * Axis[0] * (1 - fCos) - Axis[1] * fSin;
	Matrix[3][0] = 0;

	// Column 1:
	Matrix[0][1] = Axis[0] * Axis[1] * (1 - fCos) - Axis[2] * fSin;
	Matrix[1][1] = fAxisYSquared + (1 - fAxisYSquared) * fCos;
	Matrix[2][1] = Axis[1] * Axis[2] * (1 - fCos) + Axis[0] * fSin;
	Matrix[3][1] = 0;

	// Column 2:
	Matrix[0][2] = Axis[2] * Axis[0] * (1 - fCos) + Axis[1] * fSin;
	Matrix[1][2] = Axis[1] * Axis[2] * (1 - fCos) - Axis[0] * fSin;
	Matrix[2][2] = fAxisZSquared + (1 - fAxisZSquared) * fCos;
	Matrix[3][2] = 0;

	// Column 3:
	Matrix[0][3] = 0;
	Matrix[1][3] = 0;
	Matrix[2][3] = 0;
	Matrix[3][3] = 1;
}


//-----------------------------------------------------------------------------
// Invert a matrix
//-----------------------------------------------------------------------------

int  LU_Decompose  (matrix4_t& matrix, int index[4])
{
    int    i, j, k;		/* Loop Indices */
	int    imax;		/* Maximum i Index */
	float big;		/* Non-Zero Largest Element */
	float sum;		/* Accumulator */
	float temp;		/* Scratch Variable */
	float vv[4];		/* Implicit Scaling of Each Row */
	
    for (i=0;  i < 4;  ++i)
    {   
		big = 0;
		for (j=0;  j < 4;  ++j)
		{
			if ((temp = fabs (matrix[i][j])) > big)
				big = temp;
		}
		if (big == 0.0)
			return 0;
		vv[i] = 1 / big;
    }
	
    for (j=0;  j < 4;  ++j)
    {   
		for (i=0;  i < j;  ++i)
		{   
			sum = matrix[i][j];
			for (k=0;  k < i;  ++k)
				sum -= matrix[i][k] * matrix[k][j];
			matrix[i][j] = sum;
		}
		
		big = 0;
		for (i=j;  i < 4;  ++i)
		{   
			sum = matrix[i][j];
			for (k=0;  k < j;  ++k)
				sum -= matrix[i][k] * matrix[k][j];
			matrix[i][j] = sum;
			temp = vv[i] * fabs(sum);
			if (temp >= big)
			{   
				big = temp;
				imax = i;
			}
		}
		
		if (j != imax)
		{   
			for (k=0;  k < 4;  ++k)
			{   
				temp = matrix[imax][k];
				matrix[imax][k] = matrix[j][k];
				matrix[j][k] = temp;
			}
			vv[imax] = vv[j];
		}
		
		index[j] = imax;
		if (matrix[j][j] == 0.0)
			return 0;
		
		if (j != (4-1))
		{   
			temp = 1 / matrix[j][j];
			for (i=j+1;  i < 4;  ++i)
				matrix[i][j] *= temp;
		}
    }
	
    return 1;
}



/****************************************************************************/

void  LU_Backsub  (matrix4_t& matrix, int index[4], float B[4])
{
	int    i,j;
	int    ii;
	int    ip;
	float sum;
	
    ii = -1;
	
    for (i=0;  i < 4;  ++i)
    {   
		ip = index[i];
		sum = B[ip];
		B[ip] = B[i];
		if (ii >= 0)
		{
			for (j=ii;  j <= i-1;  ++j)
				sum -= matrix[i][j] * B[j];
		}
		else if (sum)
			ii = i;
		B[i] = sum;
    }
	
    for (i=(4-1);  i >= 0;  --i)
    {   
		sum = B[i];
		for (j=i+1;  j < 4;  ++j)
			sum -= matrix[i][j] * B[j];
		B[i] = sum / matrix[i][i];
    }
}


// General, 4x4 matrix inversion
void  MatrixInvert(matrix4_t& matrix)
{
	int         i,j;
	int         index[4];
	float      column[4];
	matrix4_t inverse;
	
    if (!LU_Decompose (matrix, index))
    {
		// singular matrix
		assert(0);
		return;
    }
	
    for (j=0;  j < 4;  ++j)
    {   
		for (i=0;  i < 4;  ++i)
		{
			column[i] = 0;
		}
		column[j] = 1;
		LU_Backsub (matrix, index, column);
		for (i=0;  i < 4;  ++i)
		{
			inverse[i][j] = column[i];
		}
    }
	
    memcpy (matrix.Base(), inverse.Base(), sizeof(matrix4_t));
}


//
// Multiplies a vector by a matrix.
//
void MatrixMultiply(Vector &Dest, const Vector &Source, const matrix4_t& Matrix)
{
//	assert( Dest != Source );
	Dest[0] = Source[0] * Matrix[0][0] + Source[1] * Matrix[0][1] + Source[2] * Matrix[0][2];
	Dest[1] = Source[0] * Matrix[1][0] + Source[1] * Matrix[1][1] + Source[2] * Matrix[1][2];
	Dest[2] = Source[0] * Matrix[2][0] + Source[1] * Matrix[2][1] + Source[2] * Matrix[2][2];
}

void MatrixMultiplyPoint(Vector& Dest, const Vector& Source, const matrix4_t& Matrix)
{
//	assert( &Dest != &Source );
	Dest[0] = Source[0] * Matrix[0][0] + Source[1] * Matrix[0][1] + Source[2] * Matrix[0][2] + Matrix[0][3];
	Dest[1] = Source[0] * Matrix[1][0] + Source[1] * Matrix[1][1] + Source[2] * Matrix[1][2] + Matrix[1][3];
	Dest[2] = Source[0] * Matrix[2][0] + Source[1] * Matrix[2][1] + Source[2] * Matrix[2][2] + Matrix[2][3];
}


void ProjectPoint(Vector &Dest, const Vector &Source, const matrix4_t& Matrix)
{
	float w;

	Dest[0] = Source[0] * Matrix[0][0] + Source[1] * Matrix[0][1] + Source[2] * Matrix[0][2] + Matrix[0][3];
	Dest[1] = Source[0] * Matrix[1][0] + Source[1] * Matrix[1][1] + Source[2] * Matrix[1][2] + Matrix[1][3];
	Dest[2] = Source[0] * Matrix[2][0] + Source[1] * Matrix[2][1] + Source[2] * Matrix[2][2] + Matrix[2][3];
	w		= Source[0] * Matrix[3][0] + Source[1] * Matrix[3][1] + Source[2] * Matrix[3][2] + Matrix[3][3];
	if (w != 0)
	{
		w = 1.0 / w;
		Dest[0] *= w;
		Dest[1] *= w;
		Dest[2] *= w;
	}
}


void RotateX(matrix4_t& Matrix, float fDegrees)
{
	float Temp01, Temp11, Temp21, Temp31;
	float Temp02, Temp12, Temp22, Temp32;

	float fRadians = DEG2RAD(fDegrees);

	float fSin = (float)sin(fRadians);
	float fCos = (float)cos(fRadians);

	Temp01 = Matrix[0][1] * fCos + Matrix[0][2] * fSin;
	Temp11 = Matrix[1][1] * fCos + Matrix[1][2] * fSin;
	Temp21 = Matrix[2][1] * fCos + Matrix[2][2] * fSin;
	Temp31 = Matrix[3][1] * fCos + Matrix[3][2] * fSin;

	Temp02 = Matrix[0][1] * -fSin + Matrix[0][2] * fCos;
	Temp12 = Matrix[1][1] * -fSin + Matrix[1][2] * fCos;
	Temp22 = Matrix[2][1] * -fSin + Matrix[2][2] * fCos;
	Temp32 = Matrix[3][1] * -fSin + Matrix[3][2] * fCos;

	Matrix[0][1] = Temp01;
	Matrix[1][1] = Temp11;
	Matrix[2][1] = Temp21;
	Matrix[3][1] = Temp31;

	Matrix[0][2] = Temp02;
	Matrix[1][2] = Temp12;
	Matrix[2][2] = Temp22;
	Matrix[3][2] = Temp32;
}


void RotateY(matrix4_t& Matrix, float fDegrees)
{
	float Temp00, Temp10, Temp20, Temp30;
	float Temp02, Temp12, Temp22, Temp32;

	float fRadians = DEG2RAD(fDegrees);

	float fSin = (float)sin(fRadians);
	float fCos = (float)cos(fRadians);

	Temp00 = Matrix[0][0] * fCos + Matrix[0][2] * -fSin;
	Temp10 = Matrix[1][0] * fCos + Matrix[1][2] * -fSin;
	Temp20 = Matrix[2][0] * fCos + Matrix[2][2] * -fSin;
	Temp30 = Matrix[3][0] * fCos + Matrix[3][2] * -fSin;

	Temp02 = Matrix[0][0] * fSin + Matrix[0][2] * fCos;
	Temp12 = Matrix[1][0] * fSin + Matrix[1][2] * fCos;
	Temp22 = Matrix[2][0] * fSin + Matrix[2][2] * fCos;
	Temp32 = Matrix[3][0] * fSin + Matrix[3][2] * fCos;

	Matrix[0][0] = Temp00;
	Matrix[1][0] = Temp10;
	Matrix[2][0] = Temp20;
	Matrix[3][0] = Temp30;

	Matrix[0][2] = Temp02;
	Matrix[1][2] = Temp12;
	Matrix[2][2] = Temp22;
	Matrix[3][2] = Temp32;
}


void RotateZ(matrix4_t& Matrix, float fDegrees)
{
	float Temp00, Temp10, Temp20, Temp30;
	float Temp01, Temp11, Temp21, Temp31;

	float fRadians = DEG2RAD(fDegrees);

	float fSin = (float)sin(fRadians);
	float fCos = (float)cos(fRadians);

	Temp00 = Matrix[0][0] * fCos + Matrix[0][1] * fSin;
	Temp10 = Matrix[1][0] * fCos + Matrix[1][1] * fSin;
	Temp20 = Matrix[2][0] * fCos + Matrix[2][1] * fSin;
	Temp30 = Matrix[3][0] * fCos + Matrix[3][1] * fSin;

	Temp01 = Matrix[0][0] * -fSin + Matrix[0][1] * fCos;
	Temp11 = Matrix[1][0] * -fSin + Matrix[1][1] * fCos;
	Temp21 = Matrix[2][0] * -fSin + Matrix[2][1] * fCos;
	Temp31 = Matrix[3][0] * -fSin + Matrix[3][1] * fCos;

	Matrix[0][0] = Temp00;
	Matrix[1][0] = Temp10;
	Matrix[2][0] = Temp20;
	Matrix[3][0] = Temp30;

	Matrix[0][1] = Temp01;
	Matrix[1][1] = Temp11;
	Matrix[2][1] = Temp21;
	Matrix[3][1] = Temp31;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Matrix - 
//-----------------------------------------------------------------------------
void ZeroMatrix(matrix4_t& Matrix)
{
	Matrix[0][0] = 0;
	Matrix[1][0] = 0;
	Matrix[2][0] = 0;
	Matrix[3][0] = 0;

	Matrix[0][1] = 0;
	Matrix[1][1] = 0;
	Matrix[2][1] = 0;
	Matrix[3][1] = 0;

	Matrix[0][2] = 0;
	Matrix[1][2] = 0;
	Matrix[2][2] = 0;
	Matrix[3][2] = 0;

	Matrix[0][3] = 0;
	Matrix[1][3] = 0;
	Matrix[2][3] = 0;
	Matrix[3][3] = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Loads the given matrix with an identity matrix.
// Input  : Matrix - 4 x 4 matrix to be loaded with the identity matrix.
//-----------------------------------------------------------------------------
void IdentityMatrix(matrix4_t& Matrix)
{
	Matrix[0][0] = 1;
	Matrix[1][0] = 0;
	Matrix[2][0] = 0;
	Matrix[3][0] = 0;

	Matrix[0][1] = 0;
	Matrix[1][1] = 1;
	Matrix[2][1] = 0;
	Matrix[3][1] = 0;

	Matrix[0][2] = 0;
	Matrix[1][2] = 0;
	Matrix[2][2] = 1;
	Matrix[3][2] = 0;

	Matrix[0][3] = 0;
	Matrix[1][3] = 0;
	Matrix[2][3] = 0;
	Matrix[3][3] = 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Matrix - 
//-----------------------------------------------------------------------------
void TransposeMatrix(matrix4_t& Matrix)
{
	float fTemp;

	fTemp = Matrix[0][1];
	Matrix[0][1] = Matrix[1][0];
	Matrix[1][0] = fTemp;

	fTemp = Matrix[0][2];
	Matrix[0][2] = Matrix[2][0];
	Matrix[2][0] = fTemp;

	fTemp = Matrix[0][3];
	Matrix[0][3] = Matrix[3][0];
	Matrix[3][0] = fTemp;

	fTemp = Matrix[1][2];
	Matrix[1][2] = Matrix[2][1];
	Matrix[2][1] = fTemp;

	fTemp = Matrix[1][3];
	Matrix[1][3] = Matrix[3][1];
	Matrix[3][1] = fTemp;

	fTemp = Matrix[2][3];
	Matrix[2][3] = Matrix[3][2];
	Matrix[3][2] = fTemp;
}


//-----------------------------------------------------------------------------
// Purpose: Computes MatrixIn1 * MatrixIn2, placing the result in MatrixOut.
// Input  : MatrixIn1 - 
//			MatrixIn2 - 
//			MatrixOut - 
//-----------------------------------------------------------------------------
void MultiplyMatrix(const matrix4_t& MatrixIn1, const matrix4_t& MatrixIn2, matrix4_t& MatrixOut)
{
	int nRow;
	int nCol;
	int i;

	for (nRow = 0; nRow < 4; nRow++)
	{
		for (nCol = 0; nCol < 4; nCol++)
		{
			MatrixOut[nRow][nCol] = 0;

			for (i = 0; i < 4; i++)
			{
				MatrixOut[nRow][nCol] += MatrixIn1[nRow][i] * MatrixIn2[i][nCol];
			}
		}
	}
}


