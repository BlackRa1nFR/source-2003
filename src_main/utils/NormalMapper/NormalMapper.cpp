//=============================================================================
// NormalMapper.cpp -- Program that converts a low and high res model into
//                     a normal map, lots of options see main() for details.
//=============================================================================
// $File: //depot/3darg/Tools/NormalMapper/NormalMapper.cpp $ $Revision: #35 $ $Author: gosselin $
//=============================================================================
// (C) 2002 ATI Research, Inc., All rights reserved.
//=============================================================================

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include "NmFileIO.h"
#include "TGAIO.h"
#include "ArgFileIO.h"
#include "SMDFileIO.h"

#define PACKINTOBYTE_MINUS1TO1(X)  ((BYTE)((X)*127.5+127.5))
#define UNPACKBYTE_MINUS1TO1(x)    ((((float)(x)-127.5)/127.5))
#define PACKINTOBYTE_0TO1(x)       ((BYTE)((x)*255))
#define UNPACKBYTE_0TO1(x)         (((float)(x)/255.0f))

#define PACKINTOSHORT_0TO1(x)      ((unsigned short)((x)*65535))
#define UNPACKSHORT_0TO1(x)        (((float)(x)/65535.0f))
#define PACKINTOSHORT_MINUS1TO1(X)  ((short)((X)*32767.5+32767.5))
#define UNPACKSHORT_MINUS1TO1(x)    ((((float)(x)-32767.5)/32767.5))
#define PACKINTOSHORT_SMINUS1TO1(x) ((short)((x)*32767.5))
#define UNPACKSHORT_SMINUS1TO1(x)   (((float)(x))/32767.5)

#define VEC_Subtract(a, b, c) ((c)[0] = (a)[0] - (b)[0], \
                               (c)[1] = (a)[1] - (b)[1], \
                               (c)[2] = (a)[2] - (b)[2])
#define VEC_Add(a, b, c) ((c)[0] = (a)[0] + (b)[0], \
                          (c)[1] = (a)[1] + (b)[1], \
                          (c)[2] = (a)[2] + (b)[2])
#define VEC_Cross(a, b, c) ((c)[0] = (a)[1] * (b)[2] - (a)[2] * (b)[1], \
                            (c)[1] = (a)[2] * (b)[0] - (a)[0] * (b)[2], \
                            (c)[2] = (a)[0] * (b)[1] - (a)[1] * (b)[0])
#define VEC_DotProduct(a, b) ((a)[0] * (b)[0] + \
                              (a)[1] * (b)[1] + \
                              (a)[2] * (b)[2])
#define INT_ROUND_TEXCOORD_U(X)  (int)(((X)*(float)(gWidth-1))+0.5f)
#define INT_ROUND_TEXCOORD_V(X)  (int)(((X)*(float)(gHeight-1))+0.5f)
#define INT_TEXCOORD_U(X)  (int)((X)*(float)(gWidth-1))
#define INT_TEXCOORD_V(X)  (int)((X)*(float)(gHeight-1))

// Value that's close enough to be called 0.0
#define EPSILON 1.0e-7

// Edge structure.
typedef struct
{
   int idx0;
   int idx1;

   // Min/max info
   int yMin;
   int yMax;
   int x;
   int x1;
   int increment;
   int numerator;
   int denominator;
} NmEdge;

// Tangent space structure.
typedef struct
{
   double m[3][9];
} NmTangentMatrix;

// High res "visibility"
typedef struct
{
   int numTris;
   unsigned int* index;
} NmVisible;

// Experimental pixel format.      
typedef union
{
   struct { BYTE r, g, b, a; };
   struct { BYTE v[4]; };
} JohnPixel;

// Local print routines.
void NmErrorLog (const char *szFmt, ...);
void NmErrorLog (char *szFmt);
#define NmPrint NmErrorLog

// Width and height of the resultant texture
int gWidth;
int gHeight;

// Supersampling variables.
int gNumSamples = 1;
typedef union
{
   struct { float x, y; };
   struct { float v[2]; };
} Sample;

// Rules for figuring out which normal is better.
enum
{
   NORM_RULE_CLOSEST = 0,
   NORM_RULE_BEST_CLOSEST,
   NORM_RULE_BEST_FARTHEST,
   NORM_RULE_FARTHEST,
   NORM_RULE_MIXED,
   NORM_RULE_BEST_MIXED,
   NORM_RULE_BEST,
   NORM_RULE_FRONT_FURTHEST,
   NORM_RULE_FRONT_BEST_FURTHEST,
   NORM_RULE_FRONT_CLOSEST,
   NORM_RULE_FRONT_BEST_CLOSEST
};
int gNormalRules = NORM_RULE_BEST_CLOSEST;

// The output format
enum
{
   NORM_OUTPUT_8_8_8_TGA = 0,
   NORM_OUTPUT_JOHN_TGA,
   NORM_OUTPUT_16_16_ARG,
   NORM_OUTPUT_16_16_16_16_ARG,
   NORM_OUTPUT_10_10_10_2_ARG,
   NORM_OUTPUT_10_10_10_2_ARG_MS,
   NORM_OUTPUT_11_11_10_ARG_MS,
};
int gOutput = NORM_OUTPUT_8_8_8_TGA;


// How to generate mip levels
enum
{
   MIP_NONE = 0,
   MIP_RECOMPUTE,
   MIP_BOX
};
int gComputeMipLevels = MIP_NONE;


double gEpsilon = 0.0;       // Tolerance value
double gDistance = FLT_MAX;  // Maximum distance that a normal is considered
bool gInTangentSpace = true; // Are we putting the normals into tangent space
bool gExpandTexels = true;   // Expand the border texels so we don't get crud
                             // when bi/trilinear filtering
bool gBoxFilter = false;     // Perform a post-box filter on normal map?

//////////////////////////////////////////////////////////////////////////////
// Get the sample offsets for supersampling.
//////////////////////////////////////////////////////////////////////////////
Sample*
GetSamples (int numSamples)
{
   static Sample s1[] = { {0.0f, 0.0f} };
   switch (numSamples)
   {
      case 1:
         {
            return s1;
         }
      case 2:
         {
            static Sample s2[2] = { {0.5f, 0.5f},
                                    {-0.5f, -0.5f} };
            return s2;
         }
      case 3:
         {
            static Sample s3[3] = { {0.5f, 0.5f},
                                    {0.0f, 0.0f},
                                    {-0.5f, -0.5f} };
            return s3;
         }
      case 4:
         {
            static Sample s4[4] = { {0.5f, 0.5f}, 
                                    {-0.5f, 0.5f},
                                    {0.5f, -0.5f},
                                    {-0.5f, -0.5f} };
            return s4;
         }
      case 5:
         {
            static Sample s5[5] = { {0.5f, 0.5f},
                                    {-0.5f, 0.5f},
                                    {0.0f, 0.0f},
                                    {0.5f, -0.5f},
                                    {-0.5f, -0.5f} };
            return s5;
         }
      case 9:
         {
            static Sample s9[9] = { {0.5f, 0.5f},
                                    {0.0f, 0.5f},
                                    {-0.5f, 0.5f},
                                    {-0.5f, 0.0f},
                                    {0.0f, 0.0f},
                                    {0.5f, 0.0f},
                                    {0.5f, -0.5f},
                                    {0.0f, -0.5f},
                                    {-0.5f, -0.5f} };
            return s9;
         }
      case 13:
         {
            static Sample s13[13] = { { 0.33f, 0.33f},
                                      { 0.0f,  0.33f},
                                      {-0.33f, 0.33f},
                                      {-0.33f, 0.0f},
                                      { 0.0f,  0.0f},
                                      { 0.33f, 0.0f},
                                      { 0.33f,-0.33f},
                                      { 0.0f, -0.33f},
                                      {-0.33f,-0.33f},
                                      { 0.66f, 0.00f},
                                      {-0.66f, 0.00f},
                                      { 0.00f, 0.66f},
                                      { 0.00f, 0.66f}};
            return s13;
         }
      case 21:
         {
            static Sample s21[21] = { { 0.33f, 0.33f},
                                      { 0.0f,  0.33f},
                                      {-0.33f, 0.33f},
                                      {-0.33f, 0.0f},
                                      { 0.0f,  0.0f},
                                      { 0.33f, 0.0f},
                                      { 0.33f,-0.33f},
                                      { 0.0f, -0.33f},
                                      {-0.33f,-0.33f},
                                      { 0.66f, 0.00f},
                                      { 0.66f, 0.33f},
                                      { 0.66f,-0.33f},
                                      {-0.66f, 0.00f},
                                      {-0.66f, 0.33f},
                                      {-0.66f,-0.33f},
                                      { 0.00f, 0.66f},
                                      { 0.33f, 0.66f},
                                      {-0.33f, 0.66f},
                                      { 0.33f,-0.66f},
                                      {-0.33f,-0.66f},
                                      { 0.00f,-0.66f}};
            return s21;
         }
      case 37:
         {
            static Sample s37[37] = { { 0.25f, 0.25f},
                                      { 0.0f,  0.25f},
                                      {-0.25f, 0.25f},
                                      {-0.25f, 0.0f},
                                      { 0.0f,  0.0f},
                                      { 0.25f, 0.0f},
                                      { 0.25f,-0.25f},
                                      { 0.0f, -0.25f},
                                      {-0.25f,-0.25f},
                                      { 0.50f, 0.00f},
                                      { 0.50f, 0.25f},
                                      { 0.50f,-0.25f},
                                      {-0.50f, 0.00f},
                                      {-0.50f, 0.25f},
                                      {-0.50f,-0.25f},
                                      { 0.00f, 0.50f},
                                      { 0.25f, 0.50f},
                                      {-0.25f, 0.50f},
                                      { 0.25f,-0.50f},
                                      {-0.25f,-0.50f},
                                      { 0.00f,-0.50f},
                                      { 0.75f, 0.25f},
                                      { 0.75f, 0.00f},
                                      { 0.75f,-0.25f},
                                      { 0.50f, 0.50f},
                                      { 0.50f,-0.50f},
                                      { 0.25f, 0.75f},
                                      { 0.25f,-0.75f},
                                      { 0.00f, 0.75f},
                                      { 0.00f,-0.75f},
                                      {-0.25f, 0.75f},
                                      {-0.25f,-0.75f},
                                      {-0.50f, 0.50f},
                                      {-0.50f,-0.50f},
                                      {-0.75f, 0.25f},
                                      {-0.75f, 0.00f},
                                      {-0.75f,-0.25f}};
            return s37;
         }
      case 57:
         {
            static Sample s57[57] = { {-0.8f, 0.2f},
                                      {-0.8f, 0.0f},
                                      {-0.8f,-0.2f},
                                      {-0.6f, 0.4f},
                                      {-0.6f, 0.2f},
                                      {-0.6f, 0.0f},
                                      {-0.6f,-0.2f},
                                      {-0.6f,-0.4f},
                                      {-0.4f, 0.6f},
                                      {-0.4f, 0.4f},
                                      {-0.4f, 0.2f},
                                      {-0.4f, 0.0f},
                                      {-0.4f,-0.2f},
                                      {-0.4f,-0.4f},
                                      {-0.4f,-0.6f},
                                      {-0.2f, 0.8f},
                                      {-0.2f, 0.6f},
                                      {-0.2f, 0.4f},
                                      {-0.2f, 0.2f},
                                      {-0.2f, 0.0f},
                                      {-0.2f,-0.2f},
                                      {-0.2f,-0.4f},
                                      {-0.2f,-0.6f},
                                      {-0.2f,-0.8f},
                                      {-0.0f, 0.8f},
                                      {-0.0f, 0.6f},
                                      {-0.0f, 0.4f},
                                      {-0.0f, 0.2f},
                                      {-0.0f, 0.0f},
                                      {-0.0f,-0.2f},
                                      {-0.0f,-0.4f},
                                      {-0.0f,-0.6f},
                                      {-0.0f,-0.8f},
                                      { 0.2f, 0.8f},
                                      { 0.2f, 0.6f},
                                      { 0.2f, 0.4f},
                                      { 0.2f, 0.2f},
                                      { 0.2f, 0.0f},
                                      { 0.2f,-0.2f},
                                      { 0.2f,-0.4f},
                                      { 0.2f,-0.6f},
                                      { 0.2f,-0.8f},
                                      { 0.4f, 0.6f},
                                      { 0.4f, 0.4f},
                                      { 0.4f, 0.2f},
                                      { 0.4f, 0.0f},
                                      { 0.4f,-0.2f},
                                      { 0.4f,-0.4f},
                                      { 0.4f,-0.6f},
                                      { 0.6f, 0.4f},
                                      { 0.6f, 0.2f},
                                      { 0.6f, 0.0f},
                                      { 0.6f,-0.2f},
                                      { 0.6f,-0.4f},
                                      { 0.8f, 0.2f},
                                      { 0.8f, 0.0f},
                                      { 0.8f,-0.2f}};
            return s57;
         }
   }
   return s1;
}

//////////////////////////////////////////////////////////////////////////
// Test if the new normal is a better fit than the last one.
//////////////////////////////////////////////////////////////////////////
inline bool
IntersectionIsBetter (int rule, NmRawPointD* norm,
                      double nNorm[3], NmRawPointD* nIntersect,
                      double lNorm[3], NmRawPointD* lIntersect)
{
   // First see if the normal is roughly in the same direction as the low
   // resoultion normal.
   if (VEC_DotProduct (nNorm, norm->v) > 0.1)
   {
      // If this is the first intersection we've found.
      if ((lNorm[0] == 0.0) && (lNorm[1] == 0.0) && (lNorm[2] == 0.0))
      {
         return true;
      }
     
      // Which ruleset to use.
      switch (rule)
      {
         default:
            NmPrint ("Error: Unknown rules set (%d)!\n", rule);
            exit (-1);
               
         case NORM_RULE_CLOSEST:
            // Pick the closest
             if ( (fabs (nIntersect->x) < gDistance) &&
                 (fabs(nIntersect->x) < fabs(lIntersect->x)) )
            {
               return true;
            }
            return false;

        case NORM_RULE_FRONT_CLOSEST:
            // Pick the closest in front of the low res.
            if ( (nIntersect->x >= -gEpsilon) && 
                 (nIntersect->x < gDistance) &&
                 (nIntersect->x < lIntersect->x) )
            {
               return true;
            }
            return false;


         case NORM_RULE_BEST_CLOSEST:
            // Pick the closest, if equal pick the one closer to low res norm
            if ( (fabs(nIntersect->x) < gDistance) &&
                 (fabs(nIntersect->x) < fabs(lIntersect->x)) )
            {
               return true;
            }
            else if ( (fabs(nIntersect->x) < gDistance) &&
                      (fabs(nIntersect->x) == fabs(lIntersect->x)) &&
                      (VEC_DotProduct (nNorm, norm->v) >
                       VEC_DotProduct (lNorm, norm->v)) )
            {
               return true;
            }
            return false;

        case NORM_RULE_FRONT_BEST_CLOSEST:
            // Pick the closest in front of low res,
            // if equal pick the one closer to low res norm
            if ( (nIntersect->x >= -gEpsilon) && 
                 (nIntersect->x < gDistance) &&
                 (nIntersect->x < lIntersect->x) )
            {
               return true;
            }
            else if ( (nIntersect->x < gDistance) &&
                      (nIntersect->x == lIntersect->x) &&
                      (VEC_DotProduct (nNorm, norm->v) >
                       VEC_DotProduct (lNorm, norm->v)))
            {
               return true;
            }
            return false;

         case NORM_RULE_FARTHEST:
            // Pick the furthest
            if ( (fabs(nIntersect->x) < gDistance) &&
                 (fabs(nIntersect->x) > fabs(lIntersect->x)) )
            {
               return true;
            }
            return false;

         case NORM_RULE_FRONT_FURTHEST:
            // Pick the furthest in front of low res
            if ( (nIntersect->x >= -gEpsilon) && 
                 (nIntersect->x < gDistance) &&
                 (nIntersect->x > lIntersect->x))
            {
               return true;
            }
            return false;

         case NORM_RULE_BEST_FARTHEST:
            // Pick the furthest, if equal pick the one closer to low res norm
            if ( (fabs (nIntersect->x) < gDistance) &&
                 (fabs(nIntersect->x) > fabs(lIntersect->x)) )
            {
               return true;
            }
            else if ( (fabs (nIntersect->x) < gDistance) && 
                      (fabs(nIntersect->x) == fabs(lIntersect->x)) &&
                      (VEC_DotProduct (nNorm, norm->v) >
                       VEC_DotProduct (lNorm, norm->v)) )
            {
               return true;
            }
            return false;

        case NORM_RULE_FRONT_BEST_FURTHEST:
            // Pick the furthest in front of low res,
            // if equal pick the one closer to low res norm
            if ( (nIntersect->x >= -gEpsilon) && 
                 (nIntersect->x < gDistance) && 
                 (nIntersect->x > lIntersect->x) )
            {
               return true;
            }
            else if ( (nIntersect->x < gDistance) &&  
                      (fabs(nIntersect->x) == fabs(lIntersect->x)) &&
                      (VEC_DotProduct (nNorm, norm->v) >
                       VEC_DotProduct (lNorm, norm->v)) )
            {
               return true;
            }
            return false;

         case NORM_RULE_MIXED:
            // See if this is an intersection behind low res face
            if (nIntersect->x <= 0)
            {
               // We prefer normals that are in front of the low res face, if
               // the last intersection was in front ignore this one.
               if (lIntersect->x > 0)
               {
                  return false;
               }
               
               // Pick the closer of the two since this intersection is behind
               // the low res face.
               if (  (fabs (nIntersect->x) < gDistance) &&  
                     (fabs(nIntersect->x) < fabs(lIntersect->x)) )
               {
                  return true;
               }
            }
            else // Intersection in front of low res face
            {
               // Pick the furthest intersection since the intersection is in
               // front of the low res face.
               if ( (fabs (nIntersect->x) < gDistance) &&  
                    (fabs(nIntersect->x) > fabs(lIntersect->x)) )
               {
                  return true;
               }
            }
            return false;

         case NORM_RULE_BEST_MIXED:
            // See if this is an intersection behind low res face
            if (nIntersect->x <= 0)
            {
               // We prefer normals that are in front of the low res face, if
               // the last intersection was in front ignore this one.
               if (lIntersect->x > 0)
               {
                  return false;
               }
               
               // Pick the closer of the two since this intersection is behind
               // the low res face. If the two intersections are equidistant
               // pick the normal that more closely matches the low res normal
               if ( (fabs (nIntersect->x) < gDistance) &&  
                    (fabs(nIntersect->x) < fabs(lIntersect->x)) )
               {
                  return true;
               }
               else if ( (fabs (nIntersect->x) < gDistance) &&  
                         (fabs(nIntersect->x) == fabs(lIntersect->x)) &&
                         (VEC_DotProduct (nNorm, norm->v) >
                          VEC_DotProduct (lNorm, norm->v)) )
               {
                  return true;
               }
            }
            else // Intersection in front of low res face
            {
               // Pick the furthest intersection since the intersection is in
               // front of the low res face. If they are equidistant pick the
               // normal that most closely matches the low res normal.
               if (fabs(nIntersect->x) > fabs(lIntersect->x))
               {
                  return true;
               }
               else if ( (fabs (nIntersect->x) < gDistance) &&  
                         (fabs(nIntersect->x) == fabs(lIntersect->x)) &&
                         (VEC_DotProduct (nNorm, norm->v) >
                          VEC_DotProduct (lNorm, norm->v)) )
               {
                  return true;
               }
            }
            return false;

         case NORM_RULE_BEST:
            // Pick the one closer to low res norm
            if ( (fabs (nIntersect->x) < gDistance) &&  
                 (VEC_DotProduct (nNorm, norm->v) >
                  VEC_DotProduct (lNorm, norm->v)) )
            {
               return true;
            }
            return false;
      }
   }
   return false;
}

//////////////////////////////////////////////////////////////////////////////
// Normalize a vector
//////////////////////////////////////////////////////////////////////////////
void
Normalize(double v[3])
{
   double len = sqrt((v[0]*v[0])+(v[1]*v[1])+(v[2]*v[2]));
   if (len < EPSILON)
   {
      v[0] = 1.0f;
      v[1] = 0.0f;
      v[2] = 0.0f;
   }
   else
   {
      v[0] = v[0]/len;
      v[1] = v[1]/len;
      v[2] = v[2]/len;
   }
}

//////////////////////////////////////////////////////////////////////////
// See if the ray intersects the triangle.
// Drew's adaption from Real Time Rendering
//////////////////////////////////////////////////////////////////////////
bool
IntersectTriangle(double *orig, double *dir,
                  float *v1, float *v2, float *v3,
                  double *t, double *u, double *v)
{
   double edge1[3], edge2[3], tvec[3], pvec[3], qvec[3];
   double det,inv_det;

   // find vectors for two edges sharing vert0
   VEC_Subtract(v2, v1, edge1);
   VEC_Subtract(v3, v1, edge2);
   
   // begin calculating determinant - also used to calculate U parameter
   VEC_Cross(dir, edge2, pvec);

   // if determinant is near zero, ray lies in plane of triangle
   det = VEC_DotProduct(edge1, pvec);
   //   if (det > -gEpsilon && det < gEpsilon)
   if (det == 0.0)
   {
      return false;
   }
   inv_det = 1.0 / det;

   // calculate distance from vert0 to ray origin
   VEC_Subtract(orig, v1, tvec);

   // calculate U parameter and test bounds
   *u = VEC_DotProduct(tvec, pvec) * inv_det;
   //if (*u < 0.0 || *u > 1.0)
   //if (*u < -0.1 || *u > 1.1)
   //if ((*u < -EPSILON) || (*u  > (1.0 + EPSILON)))
   if (*u < -gEpsilon || *u > (1.0 + gEpsilon))
   {
     return false;
   }

   // prepare to test V parameter
   VEC_Cross(tvec, edge1, qvec);

   // calculate V parameter and test bounds
   *v = VEC_DotProduct(dir, qvec) * inv_det;
   //if ((*v < 0.0) || ((*u + *v) > 1.0))
   //if ((*v < -0.1) || ((*u + *v) > 1.1))
   //if ((*v < -EPSILON) || ((*u + *v) > (1.0 + EPSILON)))
   if ((*v < -gEpsilon) || ((*u + *v) > (1.0 + gEpsilon)))
   {
      return false;
   }

   // calculate t, ray intersects triangle
   *t = VEC_DotProduct(edge2, qvec) * inv_det;

   return true;
}

/////////////////////////////////////
// Get edge info
/////////////////////////////////////
inline void
GetEdge (NmEdge* edge, NmRawTriangle* tri, int idx0, int idx1)
{
   // Based on the Y/v coordinate fill in the structure.
   if (tri->texCoord[idx0].v <= tri->texCoord[idx1].v)
   {
      edge->idx0 = idx0;
      edge->idx1 = idx1;
      edge->yMin = INT_ROUND_TEXCOORD_V (tri->texCoord[idx0].v);
      edge->yMax = INT_ROUND_TEXCOORD_V (tri->texCoord[idx1].v);
      edge->x = INT_ROUND_TEXCOORD_U (tri->texCoord[idx0].u);
      edge->x1 = INT_ROUND_TEXCOORD_U (tri->texCoord[idx1].u);
      edge->increment = edge->denominator = edge->yMax - edge->yMin;
      edge->numerator = edge->x1 - edge->x;
   }
   else
   {
      edge->idx0 = idx1;
      edge->idx1 = idx0;
      edge->yMin = INT_ROUND_TEXCOORD_V (tri->texCoord[idx1].v);
      edge->yMax = INT_ROUND_TEXCOORD_V (tri->texCoord[idx0].v);
      edge->x = INT_ROUND_TEXCOORD_U (tri->texCoord[idx1].u);
      edge->x1 = INT_ROUND_TEXCOORD_U (tri->texCoord[idx0].u);
      edge->increment = edge->denominator = edge->yMax - edge->yMin;
      edge->numerator = edge->x1 - edge->x;
   }
} // GetEdge

///////////////////////////////////////////////////////////////////////////////
// Simple spinner display for progress indication
///////////////////////////////////////////////////////////////////////////////
int gSpinner = 0;
void
ShowSpinner (char* header = NULL)
{
   char* lineStart = header;
   if (header == NULL)
   {
      static char empty[] = "";
      lineStart = empty;
   }
   switch (gSpinner)
   {
      case 0:
         printf ("\r%s\\", lineStart);
         gSpinner++;
         break;
      case 1:
         printf ("\r%s|", lineStart);
         gSpinner++;
         break;
      case 2:
         printf ("\r%s/", lineStart);
         gSpinner++;
         break;
      default:
      case 3:
         printf ("\r%s-", lineStart);
         gSpinner = 0;
         break;
   }
}

////////////////////////////////////////////////////////////////////
// Multiply a 3x3 matrix with a 3 space vector (assuming w = 1), ignoring
// the last row of the matrix
////////////////////////////////////////////////////////////////////
void
ConvertFromTangentSpace (double* m, double *vec, double *result)
{
   double tmp[3];

   if ((result == NULL) || (vec == NULL))
      return;

   tmp[0] = vec[0]*m[0] + vec[1]*m[1] + vec[2]*m[2];
   tmp[1] = vec[0]*m[3] + vec[1]*m[4] + vec[2]*m[5];
   tmp[2] = vec[0]*m[6] + vec[1]*m[7] + vec[2]*m[8];

   result[0] = tmp[0];
   result[1] = tmp[1];
   result[2] = tmp[2];
}

////////////////////////////////////////////////////////////////////
// Multiply a 3x3 matrix with a 3 space vector (assuming w = 1), ignoring
// the last row of the matrix
////////////////////////////////////////////////////////////////////
void
ConvertToTangentSpace (double* m, double *vec, double *result)
{
   double tmp[3];

   if ((result == NULL) || (vec == NULL))
      return;

   tmp[0] = vec[0]*m[0] + vec[1]*m[3] + vec[2]*m[6];
   tmp[1] = vec[0]*m[1] + vec[1]*m[4] + vec[2]*m[7];
   tmp[2] = vec[0]*m[2] + vec[1]*m[5] + vec[2]*m[8];

   result[0] = tmp[0];
   result[1] = tmp[1];
   result[2] = tmp[2];
}

//////////////////////////////////////////////////////////////////////////
// Compute plane equation
//////////////////////////////////////////////////////////////////////////
void
ComputePlaneEqn (float v0[3], float v1[3], float norm[3], double plane[4])
{
   // Find A, B, C - normal to plane
   double v[3];
   VEC_Subtract (v1, v0, v);
   VEC_Cross (v, norm, plane);
   double tmp = sqrt (plane[0]*plane[0] + plane[1]*plane[1]+plane[2]*plane[2]);
   if (tmp < EPSILON)
   {
      plane[0] = 0;
      plane[1] = 0;
      plane[2] = 1;
   }
   else
   {
      plane[0] = plane[0]/tmp;
      plane[1] = plane[1]/tmp;
      plane[2] = plane[2]/tmp;
   }

   // Find D (A*x + B*y + C*z = D)
   plane[3] = VEC_DotProduct (plane, v0);
}

//////////////////////////////////////////////////////////////////////////
// Is the triangle "inside" the plane - if any of the three points lie on
// or above the given plane then the triangle is "inside". Given the
// plane equation above if the dot product with ABC is >= D it's in!
//////////////////////////////////////////////////////////////////////////
inline bool
IsInside (NmRawTriangle* tri, double plane[4])
{
   if (VEC_DotProduct (tri->vert[0].v, plane) >= plane[3])
   {
      return true;
   }
   if (VEC_DotProduct (tri->vert[1].v, plane) >= plane[3])
   {
      return true;
   }
   if (VEC_DotProduct (tri->vert[2].v, plane) >= plane[3])
   {
      return true;
   }
   return false;
}

//////////////////////////////////////////////////////////////////////////
// Convert to John style experimental pixel format
//////////////////////////////////////////////////////////////////////////
void
ConvertToJohn (double x, double y, double z, JohnPixel* jp)
{
   // Compute one minus z
   double omz = 1.0 - z;
   double ax = fabs (x);
   double ay = fabs (y);
   double aomz = fabs (omz);

   // Find max.
   double max;
   if (ax > ay)
   {
      if (ax > aomz)
      {
         max = ax;
      }
      else
      {
         max = aomz;
      }
   }
   else
   {
      if (ay > aomz)
      {
         max = ay;
      }
      else
      {
         max = aomz;
      }
   }

   // Now compute values.
   jp->r = PACKINTOBYTE_MINUS1TO1(x/max);
   jp->g = PACKINTOBYTE_MINUS1TO1(y/max);
   jp->b = PACKINTOBYTE_MINUS1TO1(omz/max);
   jp->a = PACKINTOBYTE_0TO1(max);
}

//////////////////////////////////////////////////////////////////////////
// Fetch from the normal map given the uv.
//////////////////////////////////////////////////////////////////////////
void
Fetch (float* map, int width, int height, double u, double v, double bumpN[3])
{
   // Get coordinates in 0-1 range
   if ((u < 0.0) || (u > 1.0))
   {
      u -= floor(u);
   }
   if ((v < 0.0) || (v > 1.0))
   {
      v -= floor(v);
   }

   // Now figure out the texel information for u coordinate
   double up = (double)(width-1) * u;
   double umin = floor (up);
   double umax = ceil (up);
   double ufrac = up - umin;

   // Now figure out the texel information for v coordinate
   double vp = (double)(height-1) * v;
   double vmin = floor (vp);
   double vmax = ceil (vp);
   double vfrac = vp - vmin;

   // First term umin/vmin
   int idx = (int)(vmin)*width*3 + (int)(umin)*3;
   bumpN[0] = ((1.0-ufrac)*(1.0-vfrac)*(double)(map[idx]));
   bumpN[1] = ((1.0-ufrac)*(1.0-vfrac)*(double)(map[idx+1]));
   bumpN[2] = ((1.0-ufrac)*(1.0-vfrac)*(double)(map[idx+2]));

   // Second term umax/vmin
   idx = (int)(vmin)*width*3 + (int)(umax)*3;
   bumpN[0] += (ufrac*(1.0-vfrac)*(double)(map[idx]));
   bumpN[1] += (ufrac*(1.0-vfrac)*(double)(map[idx+1]));
   bumpN[2] += (ufrac*(1.0-vfrac)*(double)(map[idx+2]));

   // Third term umin/vmax
   idx = (int)(vmax)*width*3 + (int)(umin)*3;
   bumpN[0] += ((1.0-ufrac)*vfrac*(double)(map[idx]));
   bumpN[1] += ((1.0-ufrac)*vfrac*(double)(map[idx+1]));
   bumpN[2] += ((1.0-ufrac)*vfrac*(double)(map[idx+2]));

   // Fourth term umax/vmax
   idx = (int)(vmax)*width*3 + (int)(umax)*3;
   bumpN[0] += (ufrac*vfrac*(double)(map[idx]));
   bumpN[1] += (ufrac*vfrac*(double)(map[idx+1]));
   bumpN[2] += (ufrac*vfrac*(double)(map[idx+2]));
}

//////////////////////////////////////////////////////////////////////////
// Get a pixel from the image.
//////////////////////////////////////////////////////////////////////////
inline void 
ReadPixel (BYTE* image, int width, int off, pixel* pix, int x, int y)
{
   int idx = y*width*off + x*off;
   if (off > 0)
   {
      pix->red = image[idx];
   }
   if (off > 1)
   {
      pix->blue = image[idx + 1];
   }
   if (off > 2)
   {
      pix->green = image[idx + 2];
   }
}

//////////////////////////////////////////////////////////////////////////
// Reads a height field file from disk and converts it into a normal for
// use in perturbing the normals generated from the high res model
//////////////////////////////////////////////////////////////////////////
void
GetBumpMapFromHeightMap (char* bumpName, int* bumpWidth,  int* bumpHeight,
                         float** bumpMap, float scale)
{
   // No height field
   if (bumpName == NULL)
   {
      (*bumpWidth) = 0;
      (*bumpHeight) = 0;
      (*bumpMap) = NULL;
      return;
   }

   // First read in the heightmap.
   FILE* fp = fopen (bumpName, "rb");
   if (fp == NULL)
   {
      NmPrint ("ERROR: Unable to open %s\n", bumpName);
      exit (-1);
   }
   BYTE* image;
   int bpp;
   if (!TGAReadImage (fp, bumpWidth, bumpHeight, &bpp, &image))
   {
      NmPrint ("ERROR: Unable to read %s\n", bumpName);
      exit (-1);
   }
   fclose (fp);

   // Allocate normal image.
   (*bumpMap) = new float [(*bumpWidth)*(*bumpHeight)*3];
   if ((*bumpMap) == NULL)
   {
      NmPrint ("ERROR: Unable to allocate normal map memory!");
      exit (-1);
   }
   
   // Get offset
   int off = 0;
   switch (bpp)
   {
      case 8:
         off = 1;
         break;
      case 16:
         off = 2;
         break;
      case 24:
         off = 3;
         break;
      case 32:
         off = 4;
         break;
      default:
         NmPrint ("ERROR: Unhandled bit depth for bump map!\n");
         exit (-1);
   }

   // Sobel the image to get normals.
   float dX, dY, nX, nY, nZ, oolen;
   pixel pix;
   for (int y = 0; y < (*bumpHeight); y++)
   {
      for (int x = 0; x < (*bumpWidth); x++)
      {
         // Do Y Sobel filter
         ReadPixel (image, (*bumpWidth), off,
                    &pix, (x-1+(*bumpWidth)) % (*bumpWidth), (y+1) % (*bumpHeight));
         dY  = ((((float) pix.red) / 255.0f)*scale) * -1.0f;
            
         ReadPixel(image, (*bumpWidth), off,
                   &pix,   x   % (*bumpWidth), (y+1) % (*bumpHeight));
         dY += ((((float) pix.red) / 255.0f)*scale) * -2.0f;
            
         ReadPixel(image, (*bumpWidth), off,
                   &pix, (x+1) % (*bumpWidth), (y+1) % (*bumpHeight));
         dY += ((((float) pix.red) / 255.0f)*scale) * -1.0f;
            
         ReadPixel(image, (*bumpWidth), off,
                   &pix, (x-1+(*bumpWidth)) % (*bumpWidth), (y-1+(*bumpHeight)) % (*bumpHeight));
         dY += ((((float) pix.red) / 255.0f)*scale) *  1.0f;
            
         ReadPixel(image, (*bumpWidth), off,
                   &pix,   x   % (*bumpWidth), (y-1+(*bumpHeight)) % (*bumpHeight));
         dY += ((((float) pix.red) / 255.0f)*scale) *  2.0f;
            
         ReadPixel(image, (*bumpWidth), off,
                   &pix, (x+1) % (*bumpWidth), (y-1+(*bumpHeight)) % (*bumpHeight));
         dY += ((((float) pix.red) / 255.0f)*scale) *  1.0f;
            
         // Do X Sobel filter
         ReadPixel(image, (*bumpWidth), off,
                   &pix, (x-1+(*bumpWidth)) % (*bumpWidth), (y-1+(*bumpHeight)) % (*bumpHeight));
         dX  = ((((float) pix.red) / 255.0f)*scale) * -1.0f;
            
         ReadPixel(image, (*bumpWidth), off,
                   &pix, (x-1+(*bumpWidth)) % (*bumpWidth),   y   % (*bumpHeight));
         dX += ((((float) pix.red) / 255.0f)*scale) * -2.0f;
            
         ReadPixel(image, (*bumpWidth), off,
                   &pix, (x-1+(*bumpWidth)) % (*bumpWidth), (y+1) % (*bumpHeight));
         dX += ((((float) pix.red) / 255.0f)*scale) * -1.0f;
            
         ReadPixel(image, (*bumpWidth), off,
                   &pix, (x+1) % (*bumpWidth), (y-1+(*bumpHeight)) % (*bumpHeight));
         dX += ((((float) pix.red) / 255.0f)*scale) *  1.0f;
            
         ReadPixel(image, (*bumpWidth), off,
                   &pix, (x+1) % (*bumpWidth),   y   % (*bumpHeight));
         dX += ((((float) pix.red) / 255.0f)*scale) *  2.0f;
            
         ReadPixel(image, (*bumpWidth), off,
                   &pix, (x+1) % (*bumpWidth), (y+1) % (*bumpHeight));
         dX += ((((float) pix.red) / 255.0f)*scale) *  1.0f;
            
            
         // Cross Product of components of gradient reduces to
         nX = -dX;
         nY = -dY;
         nZ = 1;
            
         // Normalize
         oolen = 1.0f/((float) sqrt(nX*nX + nY*nY + nZ*nZ));
         nX *= oolen;
         nY *= oolen;
         nZ *= oolen;

         int idx = y*(*bumpWidth)*3 + x*3;
         (*bumpMap)[idx] = nX;
         (*bumpMap)[idx+1] = nY;
         (*bumpMap)[idx+2] = nZ;
      }
   }
}

//////////////////////////////////////////////////////////////////////////
// Check argument flags and set the appropriate values.
//////////////////////////////////////////////////////////////////////////
void
TestArgs (char* args)
{
   // Print out flags
   NmPrint ("Flags: %s\n", args);

   // Super-sample number
   if (strstr (args, "1") != NULL)
   {
      gNumSamples = 1;
   }
   if (strstr (args, "2") != NULL)
   {
      gNumSamples = 2;
   }
   if (strstr (args, "3") != NULL)
   {
      gNumSamples = 3;
   }
   if (strstr (args, "4") != NULL)
   {
      gNumSamples = 4;
   }
   if (strstr (args, "5") != NULL)
   {
      gNumSamples = 5;
   }
   if (strstr (args, "6") != NULL)
   {
      gNumSamples = 9;
   }
   if (strstr (args, "7") != NULL)
   {
      gNumSamples = 13;
   }
   if (strstr (args, "8") != NULL)
   {
      gNumSamples = 21;
   }
   if (strstr (args, "9") != NULL)
   {
      gNumSamples = 37;
   }
   if (strstr (args, "0") != NULL)
   {
      gNumSamples = 57;
   }

   // Rulesets for determining best normals
   if (strstr (args, "c") != NULL)
   {
      gNormalRules = NORM_RULE_CLOSEST;
      if (strstr (args, "X") != NULL)
      {
         gNormalRules = NORM_RULE_FRONT_CLOSEST;
      }
   }
   if (strstr (args, "C") != NULL)
   {
      gNormalRules = NORM_RULE_BEST_CLOSEST;
      if (strstr (args, "X") != NULL)
      {
         gNormalRules = NORM_RULE_FRONT_BEST_CLOSEST;
      }
   }
   if (strstr (args, "f") != NULL)
   {
      gNormalRules = NORM_RULE_FARTHEST;
      if (strstr (args, "X") != NULL)
      {
         gNormalRules = NORM_RULE_FRONT_FURTHEST;
      }
   }
   if (strstr (args, "F") != NULL)
   {
      gNormalRules = NORM_RULE_BEST_FARTHEST;
      if (strstr (args, "X") != NULL)
      {
         gNormalRules = NORM_RULE_FRONT_BEST_FURTHEST;
      }
   }
   if (strstr (args, "d") != NULL)
   {
      gNormalRules = NORM_RULE_MIXED;
   }
   if (strstr (args, "D") != NULL)
   {
      gNormalRules = NORM_RULE_BEST_MIXED;
   }
   if (strstr (args, "b") != NULL)
   {
      gNormalRules = NORM_RULE_BEST;
   }

   // Misc flags
   if (strstr (args, "m") != NULL)
   {
      gComputeMipLevels = MIP_RECOMPUTE;
   }
   if (strstr (args, "M") != NULL)
   {
      gComputeMipLevels = MIP_BOX;
   }
   if ((strstr (args, "w") != NULL) ||
       (strstr (args, "W") != NULL))
   {
      gInTangentSpace = false;
   }
   if ((strstr (args, "e") != NULL) ||
       (strstr (args, "E") != NULL))
   {
      gExpandTexels = false;
   }
   if (strstr (args, "t") != NULL)
   {
      gEpsilon = EPSILON;
   }
   if (strstr (args, "T") != NULL)
   {
      gEpsilon = 0.0;
   }
   if (strstr (args, "h") != NULL)
   {
      gOutput = NORM_OUTPUT_16_16_ARG;
   }
   if (strstr (args, "H") != NULL)
   {
      gOutput = NORM_OUTPUT_16_16_16_16_ARG;
   }
   if (strstr (args, "i") != NULL)
   {
      gOutput = NORM_OUTPUT_10_10_10_2_ARG;
   }
   if (strstr (args, "s") != NULL)
   {
      gOutput = NORM_OUTPUT_10_10_10_2_ARG_MS;
   }
   if (strstr (args, "S") != NULL)
   {
      gOutput = NORM_OUTPUT_11_11_10_ARG_MS;
   }
   if (strstr (args, "B") != NULL)
   {
      gBoxFilter = true;
   }
}

//////////////////////////////////////////////////////////////////////////
// Entry point
//////////////////////////////////////////////////////////////////////////
void 
main (int argc, char **argv)
{
   NmPrint ("NormalMapper v01.05.01\n");

   // Get the arguments.
   char* lowName;
   char* highName;
   char* bumpName = NULL;
   double bumpScale;
   char* outName;

   // Print out verbose message if 0 or 1 arguments given.
   if ((argc == 1) || (argc == 2))
   {
      NmPrint ("Usage: NormalMapper [-12345bBcCdDefFhHimsStTvwX] lowres highres Width Height [value] [heightmap heightscale] outputname\n");
      NmPrint ("                     h  - Output 16x16 .arg texture\n");
      NmPrint ("                     H  - Output 16x16x16x16 .arg texture\n");
      NmPrint ("                     i  - Output 10x10x10x2 .arg texture\n");
      NmPrint ("                     w  - Use worldspace normals\n");
      NmPrint ("                     m  - Create mip chain (remap)\n");
      NmPrint ("                     M  - Create mip chain (box filter)\n");
      NmPrint ("                     B  - Box filter final image\n");
      NmPrint ("                     e  - Don't expand border texels\n");
      NmPrint ("                     t  - smaller tolerance on compare\n");
      NmPrint ("                     T  - no tolerance on compare\n");
      NmPrint ("                     X  - only normals in front\n");
      NmPrint ("                     v  - value is the maximum distance\n");
      NmPrint ("                     1  - Single sample per texel\n");
      NmPrint ("                     2  - Two samples per texel\n");
      NmPrint ("                     3  - Three samples per texel\n");
      NmPrint ("                     4  - Four samples per texel\n");
      NmPrint ("                     5  - Five samples per texel\n");
      NmPrint ("                     6  - Nine samples per texel\n");
      NmPrint ("                     7  - Thirteen samples per texel\n");
      NmPrint ("                     8  - Twenty one samples per texel\n");
      NmPrint ("                     9  - Thrity seven samples per texel\n");
      NmPrint ("                     0  - Fifty seven samples per texel\n");
      NmPrint ("                     c  - Pick closest intersection,\n");
      NmPrint ("                          first if equidistant\n");
      NmPrint ("                     C  - Pick closest intersection,\n");
      NmPrint ("                          normal closer to low res if equidistant\n");
      NmPrint ("                     f  - Pick furthest intersection,\n");
      NmPrint ("                          first if equidistant\n");
      NmPrint ("                     F  - Pick furthest intersection,\n");
      NmPrint ("                          normal closer to low res if equidistant\n");
      NmPrint ("                     d  - Pick furthest intersection in front,\n");
      NmPrint ("                          closest intersection behind,\n");
      NmPrint ("                          first if equidistant\n");
      NmPrint ("                     D  - Pick Pick furthest intersection in front,\n");
      NmPrint ("                          normal closer to low res if equidistant,\n");
      NmPrint ("                          closest intersection behind,\n");
      NmPrint ("                     b  - normal closest to low res\n");
      NmPrint ("                     s  - Microsoft 10x10x10x2 test (16x16x16x16\n");
      NmPrint ("                     S  - Microsoft 11x11x10 test (16x16x16x16\n");
      exit (-1);
   }

   // Make sure the right number of arguments are present.
   if ((argc != 6) && (argc != 7) && (argc != 8) && (argc != 9) && (argc != 10))
   {
      NmPrint ("Usage: NormalMapper [-12345bcCdDfFmtTvwXhH] lowres highres Width Height [value] [heightmap heightscale] outputname\n");
      exit (-1);
   }

   // No heightmap, no arguments.
   gEpsilon = 0.1;
   if (argc == 6)
   {
      lowName = argv[1];
      highName = argv[2];
      gWidth = atoi (argv[3]);
      gHeight = atoi (argv[4]);
      outName = argv[5];
   }

   // Command line arguments
   if (argc == 7)
   {
      lowName = argv[2];
      highName = argv[3];
      gWidth = atoi (argv[4]);
      gHeight = atoi (argv[5]);
      outName = argv[6];
      TestArgs (argv[1]);
   }

   // Height map no command line args, or distance with arguments.
   if (argc == 8)
   {

      if (strstr (argv[1], "-") != NULL)
      {
         if (strstr (argv[1], "v") == NULL)
         {
            NmPrint ("Usage: NormalMapper [-12345bcCdDfFmtTvwXhH] lowres highres Width Height [value] [heightmap heightscale] outputname\n");
            exit (-1);
         }
         lowName = argv[2];
         highName = argv[3];
         gWidth = atoi (argv[4]);
         gHeight = atoi (argv[5]);
         gDistance = atof (argv[6]);
         outName = argv[7];
         TestArgs (argv[1]);
      }
      else
      {
         gNormalRules = NORM_RULE_CLOSEST;
         lowName = argv[1];
         highName = argv[2];
         gWidth = atoi (argv[3]);
         gHeight = atoi (argv[4]);
         bumpName = argv[5];
         bumpScale = atof (argv[6]);
         outName = argv[7];
      }
   }

   // Commandline arguments and heightmap
   if (argc == 9)
   {
         gNormalRules = NORM_RULE_CLOSEST;
         lowName = argv[2];
         highName = argv[3];
         gWidth = atoi (argv[4]);
         gHeight = atoi (argv[5]);
         bumpName = argv[6];
         bumpScale = atof (argv[7]);
         outName = argv[8];
         TestArgs (argv[1]);
   }

   // Commandline arguments, heightmap, and distance
   if (argc == 10)
   {
         if (strstr (argv[1], "v") == NULL)
         {
            NmPrint ("Usage: NormalMapper [-12345bcCdDfFmtTvwXhH] lowres highres Width Height [value] [heightmap heightscale] outputname\n");
            exit (-1);
         }

         gNormalRules = NORM_RULE_CLOSEST;
         lowName = argv[2];
         highName = argv[3];
         gWidth = atoi (argv[4]);
         gHeight = atoi (argv[5]);
         gDistance = atof (argv[6]);
         bumpName = argv[7];
         bumpScale = atof (argv[8]);
         outName = argv[9];
         TestArgs (argv[1]);
   }

   // Check width and height
   if ((gWidth % 2) != 0)
   {
      NmPrint ("ERROR: Width must be a power of two!\n");
      exit (-1);
   }
   if ((gHeight % 2) != 0)
   {
      NmPrint ("ERROR: Height must be a power of two!\n");
      exit (-1);
   }
   if (gWidth < 1)
   {
      NmPrint ("ERROR: Width must be greater than 0\n");
      exit (-1);
   }
   if (gHeight < 1)
   {
      NmPrint ("ERROR: Height must be greater than 0\n");
      exit (-1);
   }

   // Print out options
   NmPrint ("Width: %d Height: %d\n", gWidth, gHeight);
   NmPrint ("%d samples per texel\n", gNumSamples);
   switch (gNormalRules)
   {
      case NORM_RULE_CLOSEST:
         NmPrint ("Normal Rule: Closest\n");
         break;
      case NORM_RULE_BEST_CLOSEST:
         NmPrint ("Normal Rule: Best Closest\n");
         break;
      case NORM_RULE_BEST_FARTHEST:
         NmPrint ("Normal Rule: Best Furthest\n");
         break;
      case NORM_RULE_FARTHEST:
         NmPrint ("Normal Rule: Furthest\n");
         break;
      case NORM_RULE_MIXED:
         NmPrint ("Normal Rule: Mixed\n");
         break;
      case NORM_RULE_BEST_MIXED:
         NmPrint ("Normal Rule: Best Mixed\n");
         break;
      case NORM_RULE_BEST:
         NmPrint ("Normal Rule: Best\n");
         break;
      case NORM_RULE_FRONT_FURTHEST:
         NmPrint ("Normal Rule: Front Furthest\n");
         break;
      case NORM_RULE_FRONT_BEST_FURTHEST:
         NmPrint ("Normal Rule: Front Best Furthest\n");
         break;
      case NORM_RULE_FRONT_CLOSEST:
         NmPrint ("Normal Rule: Front Closest\n");
         break;
      case NORM_RULE_FRONT_BEST_CLOSEST:
         NmPrint ("Normal Rule: Front Best Closest\n");
         break;
      default:
         NmPrint ("Normal Rule: UKNOWN!\n");
         break;
   }
   if (gDistance != FLT_MAX)
   {
      NmPrint ("Max distance: %4.12\n", gDistance);
   }
   switch (gComputeMipLevels)
   {
      case MIP_NONE:
         NmPrint ("No mipmap generation\n");
         break;
      case MIP_RECOMPUTE:
         NmPrint ("Re-cast mipmap generation.\n");
         break;
      case MIP_BOX:
         NmPrint ("Box filter mip map generation.\n");
         break;
      default:
         NmPrint ("Unknown mip map generation\n");
         break;
   }
   NmPrint ("Epsilon: %1.10f\n", gEpsilon);
   if (gInTangentSpace)
   {
      NmPrint ("Normals in tangent space\n");
   }
   else
   {
      NmPrint ("Normals in world space\n");
   }
   if (gExpandTexels)
   {
      NmPrint ("Expand border texels\n");
   }
   else
   {
      NmPrint ("Don't Expand border texels\n");
   }
   if (gBoxFilter)
   {
      NmPrint ("Postprocess box filter\n");
   }
   else
   {
      NmPrint ("No postprocess filter\n");
   }

   // Get bump map from height map.
   float* bumpMap = NULL;
   int bumpWidth = 0;
   int bumpHeight = 0;
   if (bumpName != NULL)
   {
      NmPrint ("Bump Scale: %2.4f\n", bumpScale);
      NmPrint ("Reading in height map: %s\n", bumpName);
      GetBumpMapFromHeightMap (bumpName, &bumpWidth,  &bumpHeight, &bumpMap,
                               (float)bumpScale);
   }

   // Read in the low res model
//   FILE* fp = fopen (lowName, "rb");
   FILE* fp = fopen (lowName, "r");
   if (fp == NULL)
   {
      NmPrint ("ERROR: Unable to open %s\n", lowName);
      exit (-1);
   }
   int lowNumTris;
   NmRawTriangle* lowTris;
//   if (NmReadTriangles (fp, &lowNumTris, &lowTris) == false)
   if (SMDReadTriangles (fp, &lowNumTris, &lowTris) == false)
   {
      NmPrint ("ERROR: Unable to read %s\n", lowName);
      fclose (fp);
      exit (-1);
   }
   fclose (fp);
   NmPrint ("Found %d triangles in low res model\n", lowNumTris);

   // Check low res model to make sure it's textures are in range
   double loBbox[6];
   loBbox[0] = FLT_MAX; // X min
   loBbox[1] = FLT_MAX; // Y min
   loBbox[2] = FLT_MAX; // Z min
   loBbox[3] = -FLT_MAX; // X max
   loBbox[4] = -FLT_MAX; // Y max
   loBbox[5] = -FLT_MAX; // Z max
   for (int i = 0; i < lowNumTris; i++)
   {
      for (int j = 0; j < 3; j++)
      {
         // Find bounding box.
         for (int k = 0; k < 3; k++)
         {
            if (lowTris[i].vert[j].v[k] < loBbox[k])
            {
               loBbox[k] = lowTris[i].vert[j].v[k];
            }
            if (lowTris[i].vert[j].v[k] > loBbox[k + 3])
            {
               loBbox[k + 3] = lowTris[i].vert[j].v[k];
            }
         }

         // Check texture coordinates
         if ((lowTris[i].texCoord[j].u < 0.0) ||
             (lowTris[i].texCoord[j].u > 1.0))
         {
            if (fabs(lowTris[i].texCoord[j].u) > EPSILON)
            {
               NmPrint ("ERROR: Texture coordinates must lie in the 0.0 - 1.0 range for the low res model (u: %f)!\n", lowTris[i].texCoord[j].u);
               exit (-1);
            }
            else
            {
               lowTris[i].texCoord[j].u = 0.0;
            }
         }
         if ((lowTris[i].texCoord[j].v < 0.0) ||
             (lowTris[i].texCoord[j].v > 1.0))
         {
            if (fabs(lowTris[i].texCoord[j].v) > EPSILON)
            {
               NmPrint ("ERROR: Texture coordinates must lie in the 0.0 - 1.0 range for the low res model! (v %f)\n", lowTris[i].texCoord[j].v);
               exit (-1);
            }
            else
            {
               lowTris[i].texCoord[j].v = 0.0;
            }
            
         }
      }
   }
   NmPrint ("Low poly bounding box: (%10.3f %10.3f %10.3f)\n", loBbox[0],
           loBbox[1], loBbox[2]);
   NmPrint ("                       (%10.3f %10.3f %10.3f)\n", loBbox[3],
            loBbox[4], loBbox[5]);

   // If the flag is set compute tangent space for all the low res triangles.
   NmTangentMatrix* tangentSpace = NULL;
   if (gInTangentSpace == true)
   {
      // Get tangent space.
      NmPrint ("Computing tangents\n");
      NmRawTangentSpaceD* tan = NULL;
      if (NmComputeTangentsD (lowNumTris, lowTris, &tan) == false)
      {
         NmPrint ("ERROR: Unable to compute tangent space!\n");
         exit (-1);
      }
      
      // Create tangent matrices
      tangentSpace = new NmTangentMatrix [lowNumTris];
      for (int j = 0; j < lowNumTris; j++)
      {
         for (int k = 0; k < 3; k++)
         {
            tangentSpace[j].m[k][0] = tan[j].tangent[k].x;
            tangentSpace[j].m[k][3] = tan[j].tangent[k].y;
            tangentSpace[j].m[k][6] = tan[j].tangent[k].z;

            tangentSpace[j].m[k][1] = tan[j].binormal[k].x;
            tangentSpace[j].m[k][4] = tan[j].binormal[k].y;
            tangentSpace[j].m[k][7] = tan[j].binormal[k].z;

            tangentSpace[j].m[k][2] = lowTris[j].norm[k].x;
            tangentSpace[j].m[k][5] = lowTris[j].norm[k].y;
            tangentSpace[j].m[k][8] = lowTris[j].norm[k].z;
         }
      }
      delete [] tan;
   } // end if we need to deal with tangent space

   // Read in the high res model
//   fp = fopen (highName, "rb");
   fp = fopen (highName, "r");
   if (fp == NULL)
   {
      NmPrint ("ERROR: Unable to open %s\n", highName);
      exit (-1);
   }
   int highNumTris;
   NmRawTriangle* highTris;
//   if (NmReadTriangles (fp, &highNumTris, &highTris) == false)
   if (SMDReadTriangles (fp, &highNumTris, &highTris) == false)
   {
      NmPrint ("ERROR: Unable to read %s\n", highName);
      fclose (fp);
      exit (-1);
   }
   fclose (fp);
   NmPrint ("Found %d triangles in high res model\n", highNumTris);

   // Get high res bounding box.
   double hiBbox[6];
   hiBbox[0] = FLT_MAX; // X min
   hiBbox[1] = FLT_MAX; // Y min
   hiBbox[2] = FLT_MAX; // Z min
   hiBbox[3] = -FLT_MAX; // X max
   hiBbox[4] = -FLT_MAX; // Y max
   hiBbox[5] = -FLT_MAX; // Z max
   for (int h = 0; h < highNumTris; h++)
   {
      for (int j = 0; j < 3; j++)
      {
         for (int k = 0; k < 3; k++)
         {
            if (highTris[h].vert[j].v[k] < hiBbox[k])
            {
               hiBbox[k] = highTris[h].vert[j].v[k];
            }
            if (highTris[h].vert[j].v[k] > hiBbox[k + 3])
            {
               hiBbox[k + 3] = highTris[h].vert[j].v[k];
            }
         }
      }
   }
   NmPrint ("High poly bounding box: (%10.3f %10.3f %10.3f)\n", hiBbox[0],
            hiBbox[1], hiBbox[2]);
   NmPrint ("                        (%10.3f %10.3f %10.3f)\n", hiBbox[3],
            hiBbox[4], hiBbox[5]);

   // Tangents for high res if we need them.
   NmTangentMatrix* hTangentSpace = NULL;
   if (bumpName != NULL)
   {
      // Get tangent space.
      NmPrint ("Computing tangents\n");
      NmRawTangentSpaceD* tan = NULL;
      if (NmComputeTangentsD (highNumTris, highTris, &tan) == false)
      {
         NmPrint ("ERROR: Unable to compute tangent space!\n");
         exit (-1);
      }
      
      // Create tangent matrices
      hTangentSpace = new NmTangentMatrix [highNumTris];
      for (int j = 0; j < highNumTris; j++)
      {
         for (int k = 0; k < 3; k++)
         {
            hTangentSpace[j].m[k][0] = tan[j].tangent[k].x;
            hTangentSpace[j].m[k][3] = tan[j].tangent[k].y;
            hTangentSpace[j].m[k][6] = tan[j].tangent[k].z;

            hTangentSpace[j].m[k][1] = tan[j].binormal[k].x;
            hTangentSpace[j].m[k][4] = tan[j].binormal[k].y;
            hTangentSpace[j].m[k][7] = tan[j].binormal[k].z;

            hTangentSpace[j].m[k][2] = highTris[j].norm[k].x;
            hTangentSpace[j].m[k][5] = highTris[j].norm[k].y;
            hTangentSpace[j].m[k][8] = highTris[j].norm[k].z;
         }
      }
      delete [] tan;
   }
   
   // Loop over low res triangles and figure out which triangles in the
   // high res model are relevant for normal finding.
   NmVisible* visible = new NmVisible [lowNumTris];
   if (visible == NULL)
   {
      NmPrint ("ERROR: Unable to allocate visibility sets\n");
      exit (-1);
   }
   NmPrint ("Finding visible triangle sets\n");
   printf ("  0%%");
   for (int t = 0; t < lowNumTris; t++)
   {
      // Show some progress
      int progress = (int)(((float)(t)/(float)(lowNumTris))*100.0);
      char prog[24];
      sprintf (prog, "%3d%% ", progress);
      printf ("\r%s", prog);

      // Save off a pointer to the triangle and create
      NmRawTriangle* lTri = &lowTris[t];

      // Clear out the counts and allocate a new index
      visible[t].numTris = 0;
      visible[t].index = new unsigned int [highNumTris];
      if (visible[t].index == NULL)
      {
         NmPrint ("ERROR: Unable to allocate visibilty index.\n");
         exit (-1);
      }

      // Plane equation constants
      double plane[6][4];
      ComputePlaneEqn (lTri->vert[1].v, lTri->vert[0].v, lTri->norm[0].v,
                       plane[0]);
      ComputePlaneEqn (lTri->vert[1].v, lTri->vert[0].v, lTri->norm[1].v,
                       plane[1]);
      ComputePlaneEqn (lTri->vert[2].v, lTri->vert[1].v, lTri->norm[1].v,
                       plane[2]);
      ComputePlaneEqn (lTri->vert[2].v, lTri->vert[1].v, lTri->norm[2].v,
                       plane[3]);
      ComputePlaneEqn (lTri->vert[0].v, lTri->vert[2].v, lTri->norm[2].v,
                       plane[4]);
      ComputePlaneEqn (lTri->vert[0].v, lTri->vert[2].v, lTri->norm[0].v,
                       plane[5]);
      
      // Loop through the high res triangles to see if they are "visible"
      for (int h = 0; h < highNumTris; h++)
      {
         // Save off triangle pointer
         NmRawTriangle* hTri = &highTris[h];
         // Check to see if this triangle is in the planes defined by the
         // planes computed for the low res triangle
         if ( (IsInside (hTri, plane[0]) || IsInside (hTri, plane[1])) &&
              (IsInside (hTri, plane[2]) || IsInside (hTri, plane[3])) &&
              (IsInside (hTri, plane[4]) || IsInside (hTri, plane[5])) )
         {
            visible[t].index[visible[t].numTris] = (unsigned)h;
            visible[t].numTris++;
         }
      } // end for high res triangles
   } // end for low res triangles
   printf ("\r100%%\r");
   
   // Print some stats
   float sum = 0;
   for (int lt = 0; lt < lowNumTris; lt++)
   {
      sum += (float)(visible[lt].numTris);
   }
   if ((int)sum < 1)
   {
      NmPrint ("ERROR: No visible triangles found!\n");
      exit (-1);
   }
   NmPrint ("%6.1f avg visible/triangle\n", (float)(sum)/(float)(lowNumTris));
   
   // Create image and clear to a biased 0 so any untouched pixels won't
   // have a normal associated with them.
   float* img = new float[gWidth*gHeight*4];
   if (img == NULL)
   {
      NmPrint ("ERROR: Unable to allocate texture\n");
      exit (-1);
   }
   float* img2 = new float[gWidth*gHeight*4];
   if (img2 == NULL)
   {
      NmPrint ("ERROR: Unable to allocate texture\n");
      exit (-1);
   }

   // Get samples for supersampling
   Sample* samples = GetSamples (gNumSamples);

   // loop over mip levels
   int mipCount = 0;
   int lastWidth = gWidth;
   int lastHeight = gHeight;
   while ((gWidth > 0) && (gHeight > 0))
   {
      // A little info
      NmPrint ("Output normal map %d by %d\n", gWidth, gHeight);

      // If this is the first time, or we are recomputing each mip level
      // find the normals by ray casting.
      if ((gComputeMipLevels == MIP_RECOMPUTE) || (mipCount == 0))
      {
         // Zero out memory
         memset (img, 0, sizeof (float)*gWidth*gHeight*3);
         
         // Loop over the triangles in the low res model and rasterize them
         // into the normal texture based on the texture coordinates.
         NmPrint ("Computing normals\n");
         printf ("  0%%");
         NmEdge edge[3];
         NmEdge tmp;
         for (int l = 0; l < lowNumTris; l++)
         {
            // Show some progress
            int progress = (int)(((float)(l)/(float)(lowNumTris))*100.0);
            char prog[24];
            sprintf (prog, "%3d%% ", progress);
            printf ("\r%s", prog);
            
            // Save off a pointer to the triangle and create
            NmRawTriangle* lTri = &lowTris[l];
            
            // Find edges and sort by minimum Y
            GetEdge (&edge[0], lTri, 0, 1);
            GetEdge (&edge[1], lTri, 0, 2);
            GetEdge (&edge[2], lTri, 1, 2);
            if (edge[2].yMin < edge[1].yMin)
            {
               memcpy (&tmp, &edge[1], sizeof (NmEdge));
               memcpy (&edge[1], &edge[2], sizeof (NmEdge));
               memcpy (&edge[2], &tmp, sizeof (NmEdge));
            }
            if (edge[1].yMin < edge[0].yMin)
            {
               memcpy (&tmp, &edge[0], sizeof (NmEdge));
               memcpy (&edge[0], &edge[1], sizeof (NmEdge));
               memcpy (&edge[1], &tmp, sizeof (NmEdge));
            }
            
            // Find initial Barycentric parameter. Algorithm described at: 
            // http://research.microsoft.com/~hollasch/cgindex/math/barycentric.html
#if 0
            double x1 = lTri->texCoord[0].u * (gWidth-1);
            double x2 = lTri->texCoord[1].u * (gWidth-1);
            double x3 = lTri->texCoord[2].u * (gWidth-1);
            double y1 = lTri->texCoord[0].v * (gHeight-1);
            double y2 = lTri->texCoord[1].v * (gHeight-1);
            double y3 = lTri->texCoord[2].v * (gHeight-1);
#else
            double x1 = (float)INT_ROUND_TEXCOORD_U(lTri->texCoord[0].u);
            double x2 = (float)INT_ROUND_TEXCOORD_U(lTri->texCoord[1].u);
            double x3 = (float)INT_ROUND_TEXCOORD_U(lTri->texCoord[2].u);
            double y1 = (float)INT_ROUND_TEXCOORD_V(lTri->texCoord[0].v);
            double y2 = (float)INT_ROUND_TEXCOORD_V(lTri->texCoord[1].v);
            double y3 = (float)INT_ROUND_TEXCOORD_V(lTri->texCoord[2].v);
#endif
            double b0 = ((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)); 
            
            // Find min/max Y
            int minY = edge[0].yMin;
            int maxY = edge[0].yMax;
            if (edge[1].yMax > maxY)
            {
               maxY = edge[1].yMax;
            }
            if (edge[2].yMax > maxY)
            {
               maxY = edge[2].yMax;
            }
            
            // Now loop over Ys doing each "scanline"
            for (int y = minY; y <= maxY; y++)
            {
               // Find min and max X for this scanline. 
               // I'm sure there's a smarter way to do this.
               int minX = 32767;
               int maxX = 0;
               for (int e = 0; e < 3; e++)
               {
                  // See if this edge is active.
                  if ((edge[e].yMin <= y) && (edge[e].yMax >= y))
                  {
                     // Check it's X values to see if they are min or max.
                     if (edge[e].x < minX)
                     {
                        minX = edge[e].x;
                     }
                     if (edge[e].x > maxX)
                     {
                        maxX = edge[e].x;
                     }
                     
                     // update x for next scanline
                     edge[e].increment += edge[e].numerator;
                     if (edge[e].denominator != 0)
                     {
                        if (edge[e].numerator < 0)
                        {
                           while (edge[e].increment <= 0)
                           {
                              edge[e].x--;
                              edge[e].increment += edge[e].denominator;
                           }
                        }
                        else
                        {
                           while (edge[e].increment > edge[e].denominator)
                           {
                              edge[e].x++;
                              edge[e].increment -= edge[e].denominator;
                           }
                        }
                     }
                  } // end if edge is active
               } // end for number of edges
               
               // Loop over the Xs filling in each pixel of the normal map
               for (int x = minX; x <= maxX; x++)
               {
//#define DEBUG_RASTER
#ifndef DEBUG_RASTER
                  // Find Barycentric coordinates.
                  double b1 = ((x2-x) * (y3-y) - (x3-x) * (y2-y)) / b0;
                  double b2 = ((x3-x) * (y1-y) - (x1-x) * (y3-y)) / b0;
                  double b3 = ((x1-x) * (y2-y) - (x2-x) * (y1-y)) / b0;
                  
                  // Interpolate tangent space.
                  double ts[9];
                  if (gInTangentSpace == true)
                  {
                     for (int t = 0; t < 9; t++)
                     {
                        ts[t] = (tangentSpace[l].m[0][t] * b1) + 
                           (tangentSpace[l].m[1][t] * b2) +
                           (tangentSpace[l].m[2][t] * b3);
                     }
                  }
                  
                  // For each sample accumulate the normal
                  double sNorm[3];
                  sNorm[0] = 0.0;
                  sNorm[1] = 0.0;
                  sNorm[2] = 0.0;
                  int sFound = 0;
                  for (int s = 0; s < gNumSamples; s++)
                  {
                     // Compute new x & y
                     double sX = (float)x + samples[s].x;
                     double sY = (float)y + samples[s].y;
                     
                     // Find Barycentric coordinates.
                     double b1 = ((x2-sX) * (y3-sY) - (x3-sX) * (y2-sY)) / b0;
                     double b2 = ((x3-sX) * (y1-sY) - (x1-sX) * (y3-sY)) / b0;
                     double b3 = ((x1-sX) * (y2-sY) - (x2-sX) * (y1-sY)) / b0;
                     
                     // Compute position
                     NmRawPointD pos;
                     pos.x = (lTri->vert[0].x * b1) + (lTri->vert[1].x * b2) +
                        (lTri->vert[2].x * b3);
                     pos.y = (lTri->vert[0].y * b1) + (lTri->vert[1].y * b2) +
                        (lTri->vert[2].y * b3);
                     pos.z = (lTri->vert[0].z * b1) + (lTri->vert[1].z * b2) +
                        (lTri->vert[2].z * b3);
                     
                     // Compute normal
                     NmRawPointD norm;
                     norm.x = (lTri->norm[0].x * b1) + (lTri->norm[1].x * b2) +
                        (lTri->norm[2].x * b3);
                     norm.y = (lTri->norm[0].y * b1) + (lTri->norm[1].y * b2) +
                        (lTri->norm[2].y * b3);
                     norm.z = (lTri->norm[0].z * b1) + (lTri->norm[1].z * b2) +
                        (lTri->norm[2].z * b3);
                     
                     // Find intersection with high poly model.
                     NmRawPointD intersect;
                     NmRawPointD lastIntersect;
                     double newNorm[3];
                     newNorm[0] = 0.0;
                     newNorm[1] = 0.0;
                     newNorm[2] = 0.0;
//#define SHOW_INTERSECTION_SELECTION
#ifdef SHOW_INTERSECTION_SELECTION
                     NmPrint ("-------------------------------------------\n");
                     NmPrint ("x: %d  y: %d\n", x, y);
                     NmPrint ("norm: %8.4f %8.4f %8.4f\n", norm.v[0], norm.v[1],
                              norm.v[2]);
#endif
                     for (int h = 0; h < visible[l].numTris; h++)
                     {
                        // Find closest intersection
                        NmRawTriangle* hTri = &highTris[visible[l].index[h]];
                        if (IntersectTriangle (pos.v, norm.v, hTri->vert[0].v,
                                               hTri->vert[1].v, hTri->vert[2].v,
                                               &intersect.x, &intersect.y,
                                               &intersect.z) == true)
                        {
                           // Figure out new normal so we can test if it's
                           //  facing away from the current normal
                           double b0 = 1.0 - intersect.y - intersect.z;
                           double nn[3];
                           nn[0] = (hTri->norm[0].x * b0) +
                              (hTri->norm[1].x * intersect.y) +
                              (hTri->norm[2].x * intersect.z);
                           nn[1] = (hTri->norm[0].y * b0) +
                              (hTri->norm[1].y * intersect.y)+
                              (hTri->norm[2].y * intersect.z);
                           nn[2] = (hTri->norm[0].z * b0) +
                              (hTri->norm[1].z * intersect.y) +
                              (hTri->norm[2].z * intersect.z);
                           
                           // See if this should be the normal for the map.
#ifdef SHOW_INTERSECTION_SELECTION
                           NmPrint ("   %8.4f %8.4f %8.4f - %8.4f: ",
                                    nn[0], nn[1], nn[2], intersect.x);
#endif
                           if (IntersectionIsBetter (gNormalRules, &norm,
                                                     nn, &intersect,
                                                     newNorm, &lastIntersect))
                           {
                              // Perturb by bump map
                              if (bumpMap != NULL)
                              {
                                 // Figure out texture coordinates.
                                 double u = (hTri->texCoord[0].u * b0) +
                                    (hTri->texCoord[1].u * intersect.y) +
                                    (hTri->texCoord[2].u * intersect.z);
                                 double v = (hTri->texCoord[0].v * b0) +
                                    (hTri->texCoord[1].v * intersect.y) +
                                    (hTri->texCoord[2].v * intersect.z);
                                 
                                 // Fetch from the bump map.
                                 double bumpN[3];
                                 Fetch (bumpMap,bumpWidth,bumpHeight,u,v, bumpN);
                                 
                                 // Interpolate tangent space on high res model
                                 double ts[9];
                                 for (int t = 0; t < 9; t++)
                                 {
                                    ts[t] = (hTangentSpace[visible[l].index[h]].m[0][t] * b0) + 
                                       (hTangentSpace[visible[l].index[h]].m[1][t] * intersect.y) +
                                       (hTangentSpace[visible[l].index[h]].m[2][t] * intersect.z);
                                 }
                                 // Convert normal into object space.
                                 ConvertFromTangentSpace (ts, bumpN, bumpN);
                                 double ident[3] = {0,0,1};
                                 ConvertFromTangentSpace (ts, ident, ident);
                                 
                                 // Perturb or just plain fetch.
                                 nn[0] = bumpN[0];
                                 nn[1] = bumpN[1];
                                 nn[2] = bumpN[2];
                              } // end if we've got a bump map
                              
                              // Copy over values
                              memcpy (newNorm, nn, sizeof (double)*3);
                              memcpy (&lastIntersect, &intersect,
                                      sizeof (NmRawPointD));
#ifdef SHOW_INTERSECTION_SELECTION
                              NmPrint ("Better!\n");
#endif
                           } // end if this intersection is better
#ifdef SHOW_INTERSECTION_SELECTION
                           else
                           {
                              NmPrint ("reject\n");
                           }
#endif
                        }
                     } // end for triangles in the high res model
                     
                     // Add this normal into our normal
                     if ((newNorm[0] != 0.0) ||
                         (newNorm[1] != 0.0) ||
                         (newNorm[2] != 0.0) )
                     {
                        sNorm[0] += newNorm[0];
                        sNorm[1] += newNorm[1];
                        sNorm[2] += newNorm[2];
                        sFound++;
                     }
                  } // end for number of samples
                  
                  if (sFound > 0)
                  {
                     // Convert to tangent space
                     if (gInTangentSpace == true)
                     {
                        ConvertToTangentSpace (ts, sNorm, sNorm);
                     }
                     Normalize (sNorm);
                     
                     // Save into output image.
                     int idx = y*gWidth*3 + x*3;
                     img[idx + 0] = (float)sNorm[0];
                     img[idx + 1] = (float)sNorm[1];
                     img[idx + 2] = (float)sNorm[2];
                  }
#ifdef SHOW_INTERSECTION_SELECTION
                  else
                  {
                     NmPrint ("Unable to find an intersection!\n");
                  }
#endif
                  
#else // DEBUG_RASTER
                  int idx = y*gWidth*3 + x*3;
                  img[idx] = 0.0f;
                  img[idx + 1] = 0.0f;
                  img[idx + 2] = 1.0f;
#endif
                  ShowSpinner (prog);
               } // end for x
            } // end for y
         } // end for low res triangles
         printf ("\r100%%\r");
         
         // Fill unused areas based on surrounding colors to prevent artifacts.
         if (gExpandTexels)
         {
            NmPrint ("Filling borders\n");
            for (int f = 0; f < 10; f++)
            {
               // Loop over the old image and create a new one.
               for (int y = 0; y < gHeight; y++)
               {
                  ShowSpinner ();
                  for (int x = 0; x < gWidth; x++)
                  {
                     // See if this pixel is empty
                     int idx = y*gWidth*3 + x*3;
                     if ((fabs(img[idx]) <= EPSILON) &&
                         (fabs(img[idx + 1]) <= EPSILON) && 
                         (fabs(img[idx + 2]) <= EPSILON))
                     {
                        // Accumulate the neary filled pixels
                        double nn[3] = {0.0, 0.0, 0.0};
                        int ncount = 0;
                        int yv;
                        int xv;
                        int nidx;
                        
                        // -Y
                        yv = (y - 1)%gHeight;
                        if (yv < 0) 
                        {
                           yv = gHeight -1;
                        }
                        xv = (x)%gWidth;
                        if (xv < 0) 
                        {
                           xv = gWidth -1;
                        }
                        nidx = yv*gWidth*3 + xv*3;
                        if ((fabs(img[nidx]) > EPSILON) ||
                            (fabs(img[nidx + 1]) > EPSILON) ||
                            (fabs(img[nidx + 2]) > EPSILON))
                        {
                           nn[0] += img[nidx];
                           nn[1] += img[nidx+1];
                           nn[2] += img[nidx+2];
                           ncount++;
                        }
                        
                        // +Y
                        yv = (y + 1)%gHeight;
                        if (yv < 0) 
                        {
                           yv = gHeight -1;
                        }
                        xv = (x)%gWidth;
                        if (xv < 0) 
                        {
                           xv = gWidth -1;
                        }
                        nidx = yv*gWidth*3 + xv*3;
                        if ((fabs(img[nidx]) > EPSILON) ||
                            (fabs(img[nidx + 1]) > EPSILON) ||
                            (fabs(img[nidx + 2]) > EPSILON))
                        {
                           nn[0] += img[nidx];
                           nn[1] += img[nidx+1];
                           nn[2] += img[nidx+2];
                           ncount++;
                        }
                        
                        // -X
                        yv = (y)%gHeight;
                        if (yv < 0) 
                        {
                           yv = gHeight -1;
                        }
                        xv = (x-1)%gWidth;
                        if (xv < 0) 
                        {
                           xv = gWidth -1;
                        }
                        nidx = yv*gWidth*3 + xv*3;
                        if ((fabs(img[nidx]) > EPSILON) ||
                            (fabs(img[nidx + 1]) > EPSILON) ||
                            (fabs(img[nidx + 2]) > EPSILON))
                        {
                           nn[0] += img[nidx];
                           nn[1] += img[nidx+1];
                           nn[2] += img[nidx+2];
                           ncount++;
                        }
                        
                        // -X
                        yv = (y)%gHeight;
                        if (yv < 0) 
                        {
                           yv = gHeight -1;
                        }
                        xv = (x+1)%gWidth;
                        if (xv < 0) 
                        {
                           xv = gWidth -1;
                        }
                        nidx = yv*gWidth*3 + xv*3;
                        if ((fabs(img[nidx]) > EPSILON) ||
                            (fabs(img[nidx + 1]) > EPSILON) ||
                            (fabs(img[nidx + 2]) > EPSILON))
                        {
                           nn[0] += img[nidx];
                           nn[1] += img[nidx+1];
                           nn[2] += img[nidx+2];
                           ncount++;
                        }
                        
                        // If we found some neighbors that were filled, fill
                        // this one with the average, otherwise copy
                        if (ncount > 0)
                        {
                           Normalize (nn);
                           img2[idx] = (float)nn[0];
                           img2[idx + 1] = (float)nn[1];
                           img2[idx + 2] = (float)nn[2];
                        }
                        else
                        {
                           img2[idx] = img[idx];
                           img2[idx + 1] = img[idx  + 1];
                           img2[idx + 2] = img[idx + 2];
                        }
                     } // end if this pixel is empty
                     else
                     {
                        img2[idx] = img[idx];
                        img2[idx + 1] = img[idx + 1];
                        img2[idx + 2] = img[idx + 2];
                     }
                  } // end for x
               } // end for y
               
               // Swap images
               float* i1 = img2;
               img2 = img;
               img = i1;
            } // end for f
            printf ("\r");
         } // end if we are expanding border texels

         // Box filter image
         if (gBoxFilter)
         {
            NmPrint ("Box filtering\n");
            for (int y = 0; y < (gHeight-1); y++)
            {
               ShowSpinner ();
               for (int x = 0; x < (gWidth-1); x++)
               {
                  float norm[3] = {0.0f, 0.0f, 0.0f};
                  int oldIdx = y*gWidth*3 + x*3;
                  norm[0] += img[oldIdx];
                  norm[1] += img[oldIdx+1];
                  norm[2] += img[oldIdx+2];
                  oldIdx = (y+1)*gWidth*3 + x*3;
                  norm[0] += img[oldIdx];
                  norm[1] += img[oldIdx+1];
                  norm[2] += img[oldIdx+2];
                  oldIdx = (y+1)*gWidth*3 + (x+1)*3;
                  norm[0] += img[oldIdx];
                  norm[1] += img[oldIdx+1];
                  norm[2] += img[oldIdx+2];
                  oldIdx = y*gWidth*3 + (x+1)*3;
                  norm[0] += img[oldIdx];
                  norm[1] += img[oldIdx+1];
                  norm[2] += img[oldIdx+2];
                  int idx = y*gWidth*3 + x*3;
                  double len = sqrt((norm[0]*norm[0])+(norm[1]*norm[1])+(norm[2]*norm[2]));
                  if (len < EPSILON)
                  {
                     img2[idx] = 0.0;
                     img2[idx + 1] = 0.0;
                     img2[idx + 2] = 0.0;
                  }
                  else
                  {
                     img2[idx] = (float)(norm[0]/len);
                     img2[idx + 1] = (float)(norm[1]/len);
                     img2[idx + 2] = (float)(norm[2]/len);
                  }
               }
            }

            // Swap images
            float* i1 = img2;
            img2 = img;
            img = i1;
            printf ("\r");
         } // end if we are box filtering.
      } // end if we are recomputing mip chains or this is the first time.
      else // Could do compute mip == box here, but it's our only option ATM.
      {
         // Box filter old image down.
         NmPrint ("Box filtering\n");
         for (int y = 0; y < gHeight; y++)
         {
            ShowSpinner ();
            for (int x = 0; x < gWidth; x++)
            {
               float norm[3] = {0.0f, 0.0f, 0.0f};
               int oldIdx = (y*2)*lastWidth*3 + (x*2)*3;
               norm[0] += img[oldIdx];
               norm[1] += img[oldIdx+1];
               norm[2] += img[oldIdx+2];
               oldIdx = ((y*2)+1)*lastWidth*3 + (x*2)*3;
               norm[0] += img[oldIdx];
               norm[1] += img[oldIdx+1];
               norm[2] += img[oldIdx+2];
               oldIdx = ((y*2)+1)*lastWidth*3 + ((x*2)+1)*3;
               norm[0] += img[oldIdx];
               norm[1] += img[oldIdx+1];
               norm[2] += img[oldIdx+2];
               oldIdx = (y*2)*lastWidth*3 + ((x*2)+1)*3;
               norm[0] += img[oldIdx];
               norm[1] += img[oldIdx+1];
               norm[2] += img[oldIdx+2];
               int idx = y*gWidth*3 + x*3;
               double len = sqrt((norm[0]*norm[0])+(norm[1]*norm[1])+(norm[2]*norm[2]));
               if (len < EPSILON)
               {
                  img2[idx] = 0.0;
                  img2[idx + 1] = 0.0;
                  img2[idx + 2] = 0.0;
               }
               else
               {
                  img2[idx] = (float)(norm[0]/len);
                  img2[idx + 1] = (float)(norm[1]/len);
                  img2[idx + 2] = (float)(norm[2]/len);
               }
            }
         }
         printf ("\r");

         // Swap images
         float* i1 = img2;
         img2 = img;
         img = i1;
      } // end box filter old image down for mip levels
      
      // Tack on mip chain number if needed
      char fName[256];
      strcpy (fName, outName);
      if (gComputeMipLevels > 0)
      {
         char* dot = strrchr (fName, '.');
         char ext[24];
         strcpy (ext, dot);
         if (dot != NULL)
         {
            sprintf (dot, "%02d%s", mipCount, ext);
         }
         else
         {
            sprintf (ext, "%02d.tga", mipCount);
            strcat (fName, ext);
         }
      }
      
      // Convert from float and output image and write
      switch (gOutput)
      {
         case NORM_OUTPUT_8_8_8_TGA:
            {
               // Convert image
               BYTE* outImg = new BYTE[gWidth*gHeight*3];
               NmPrint ("\rConverting to 8x8x8 Targa\n");
               for (int y = 0; y < gHeight; y++)
               {
                  ShowSpinner ();
                  for (int x = 0; x < gWidth; x++)
                  {
                     int idx = y*gWidth*3 + x*3;
                     outImg[idx + 0] = PACKINTOBYTE_MINUS1TO1(img[idx]);
                     outImg[idx + 1] = PACKINTOBYTE_MINUS1TO1(img[idx+1]);
                     outImg[idx + 2] = PACKINTOBYTE_MINUS1TO1(img[idx+2]);
                  }
               }
               printf ("\r");
               
               // Write out image
               fp = fopen (fName, "wb");
               if (fp == NULL)
               {
                  NmPrint ("ERROR: Unable to open %s\n", fName);
                  exit (-1);
               }
               if (TGAWriteImage (fp, gWidth, gHeight, 24, outImg) != true)
               {
                  NmPrint ("ERROR: Unable to write %s\n", fName);
                  fclose (fp);
                  exit (-1);
               }
               fclose (fp);
               NmPrint ("Wrote %s\n", fName);
               delete [] outImg;
            }
            break;
            
         case NORM_OUTPUT_JOHN_TGA:
            {
               // Convert image
               BYTE* outImg = new BYTE[gWidth*gHeight*4];
               JohnPixel jp;
               NmPrint ("\rConverting to John format\n");
               for (int y = 0; y < gHeight; y++)
               {
                  ShowSpinner ();
                  for (int x = 0; x < gWidth; x++)
                  {
                     int fIdx = y*gWidth*3 + x*3;
                     ConvertToJohn (img[fIdx], img[fIdx+1], img[fIdx+2], &jp);
                     int idx = y*gWidth*4 + x*4;
                     outImg[idx + 0] = jp.r;
                     outImg[idx + 1] = jp.g;
                     outImg[idx + 2] = jp.b;
                     outImg[idx + 3] = jp.a;
                  }
               }
               printf ("\r");
               
               // Write out image
               fp = fopen (fName, "wb");
               if (fp == NULL)
               {
                  NmPrint ("ERROR: Unable to open %s\n", fName);
                  exit (-1);
               }
               if (TGAWriteImage (fp, gWidth, gHeight, 32, outImg) != true)
               {
                  NmPrint ("ERROR: Unable to write %s\n", fName);
                  fclose (fp);
                  exit (-1);
               }
               fclose (fp);
               NmPrint ("Wrote %s\n", fName);
               delete [] outImg;
            }
            break;
            
         case NORM_OUTPUT_16_16_ARG:
            {
               // Convert
               short* outImg = new short[gWidth*gHeight*2];
               NmPrint ("\rConverting to 16x16 signed format\n");
               for (int y = 0; y < gHeight; y++)
               {
                  ShowSpinner ();
                  for (int x = 0; x < gWidth; x++)
                  {
                     int fIdx = y*gWidth*3 + x*3;
                     int idx = y*gWidth*2 + x*2;
                     outImg[idx + 0] = PACKINTOSHORT_SMINUS1TO1(img[fIdx]);
                     outImg[idx + 1] = PACKINTOSHORT_SMINUS1TO1(img[fIdx+1]);
                  }
               }
               printf ("\r");
               
               // Now write the image.
               AtiBitmap atiOut;
               atiOut.width = gWidth;
               atiOut.height = gHeight;
               atiOut.bitDepth = 32;
               atiOut.format = ATI_BITMAP_FORMAT_S1616;
               atiOut.pixels = (unsigned char*)(outImg);
               if (!AtiWrite3DARGImageFile (&atiOut, fName))
               {
                  NmPrint ("ERROR: Unable to write %s\n", fName);
                  exit (-1);
               }
               delete [] outImg;
               NmPrint ("Wrote %s\n", fName);
            }
            break;
            
         case NORM_OUTPUT_16_16_16_16_ARG:
            {
               // Convert image
               short* outImg = new short[gWidth*gHeight*4];
               NmPrint ("\rConverting to 16x16x16x16 signed format\n");
               for (int y = 0; y < gHeight; y++)
               {
                  ShowSpinner ();
                  for (int x = 0; x < gWidth; x++)
                  {
                     int fIdx = y*gWidth*3 + x*3;
                     int idx = y*gWidth*4 + x*4;
                     outImg[idx + 0] = PACKINTOSHORT_SMINUS1TO1(img[fIdx]);
                     outImg[idx + 1] = PACKINTOSHORT_SMINUS1TO1(img[fIdx+1]);
                     outImg[idx + 2] = PACKINTOSHORT_SMINUS1TO1(img[fIdx+2]);
                     outImg[idx + 3] = PACKINTOSHORT_SMINUS1TO1(0);
                  }
               }
               printf ("\r");

               // Now write the image.
               AtiBitmap atiOut;
               atiOut.width = gWidth;
               atiOut.height = gHeight;
               atiOut.bitDepth = 64;
               atiOut.format = ATI_BITMAP_FORMAT_S16161616;
               atiOut.pixels = (unsigned char*)(outImg);
               if (!AtiWrite3DARGImageFile (&atiOut, fName))
               {
                  NmPrint ("ERROR: Unable to write %s\n", fName);
                  exit (-1);
               }
               delete [] outImg;
               NmPrint ("Wrote %s\n", fName);
            }
            break;
            
        case NORM_OUTPUT_10_10_10_2_ARG:
            {
               // Convert image
               long* outImg = new long[gWidth*gHeight];
               NmPrint ("\rConverting to 10x10x10x2 format\n");
               for (int y = 0; y < gHeight; y++)
               {
                  ShowSpinner ();
                  for (int x = 0; x < gWidth; x++)
                  {
                     int fIdx = y*gWidth*3 + x*3;
                     int idx = y*gWidth + x;
#define MASK9  0x1ff
#define MASK10 0x3ff
#define MASK11 0x7ff
                     outImg[idx] = ((((unsigned long)(img[fIdx  ]*MASK9) + MASK9)&MASK10)<<22) |
                                   ((((unsigned long)(img[fIdx+1]*MASK9) + MASK9)&MASK10)<<12) |
                                   ((((unsigned long)(img[fIdx+2]*MASK9) + MASK9)&MASK10)<<2);
                  }
               }
               printf ("\r");

               // Now write the image.
               AtiBitmap atiOut;
               atiOut.width = gWidth;
               atiOut.height = gHeight;
               atiOut.bitDepth = 32;
               atiOut.format = ATI_BITMAP_FORMAT_1010102;
               atiOut.pixels = (unsigned char*)(outImg);
               if (!AtiWrite3DARGImageFile (&atiOut, fName))
               {
                  NmPrint ("ERROR: Unable to write %s\n", fName);
                  exit (-1);
               }
               delete [] outImg;
               NmPrint ("Wrote %s\n", fName);
            }
            break;

         case NORM_OUTPUT_10_10_10_2_ARG_MS:
            {
               // Convert image
               short* outImg = new short[gWidth*gHeight*4];
               NmPrint ("\rConverting to 10x10x10x2 test format (bit chopped 16bpc)\n");
               for (int y = 0; y < gHeight; y++)
               {
                  ShowSpinner ();
                  for (int x = 0; x < gWidth; x++)
                  {
#define MASK_10BITS 0xffc0
                     int fIdx = y*gWidth*3 + x*3;
                     int idx = y*gWidth*4 + x*4;
                     outImg[idx + 0] = (PACKINTOSHORT_MINUS1TO1(img[fIdx]))&MASK_10BITS;
                     outImg[idx + 1] = (PACKINTOSHORT_MINUS1TO1(img[fIdx+1]))&MASK_10BITS;
                     outImg[idx + 2] = (PACKINTOSHORT_MINUS1TO1(img[fIdx+2]))&MASK_10BITS;
                     outImg[idx + 3] = PACKINTOSHORT_MINUS1TO1(0);
                  }
               }
               printf ("\r");

               // Now write the image.
               AtiBitmap atiOut;
               atiOut.width = gWidth;
               atiOut.height = gHeight;
               atiOut.bitDepth = 64;
               atiOut.format = ATI_BITMAP_FORMAT_16161616;
               atiOut.pixels = (unsigned char*)(outImg);
               if (!AtiWrite3DARGImageFile (&atiOut, fName))
               {
                  NmPrint ("ERROR: Unable to write %s\n", fName);
                  exit (-1);
               }
               delete [] outImg;
               NmPrint ("Wrote %s\n", fName);
            }
            break;

         case NORM_OUTPUT_11_11_10_ARG_MS:
            {
               // Convert image
               short* outImg = new short[gWidth*gHeight*4];
               NmPrint ("\rConverting to 11x11x10 test format (bit chopped 16bpc)\n");
               for (int y = 0; y < gHeight; y++)
               {
                  ShowSpinner ();
                  for (int x = 0; x < gWidth; x++)
                  {
#define MASK_11BITS 0xffe0
                     int fIdx = y*gWidth*3 + x*3;
                     int idx = y*gWidth*4 + x*4;
                     outImg[idx + 0] = (PACKINTOSHORT_MINUS1TO1(img[fIdx]))&MASK_11BITS;
                     outImg[idx + 1] = (PACKINTOSHORT_MINUS1TO1(img[fIdx+1]))&MASK_11BITS;
                     outImg[idx + 2] = (PACKINTOSHORT_MINUS1TO1(img[fIdx+2]))&MASK_10BITS;
                     outImg[idx + 3] = PACKINTOSHORT_MINUS1TO1(0);
                  }
               }
               printf ("\r");

               // Now write the image.
               AtiBitmap atiOut;
               atiOut.width = gWidth;
               atiOut.height = gHeight;
               atiOut.bitDepth = 64;
               atiOut.format = ATI_BITMAP_FORMAT_16161616;
               atiOut.pixels = (unsigned char*)(outImg);
               if (!AtiWrite3DARGImageFile (&atiOut, fName))
               {
                  NmPrint ("ERROR: Unable to write %s\n", fName);
                  exit (-1);
               }
               delete [] outImg;
               NmPrint ("Wrote %s\n", fName);
            }
            break;
            
         default:
            NmPrint ("ERROR: Unknown format!\n");
            exit (-1);
      } // end switch on output type.

      // Update gWidth & gHeight here to go to the next mip level
      if (gComputeMipLevels > 0)
      {
         lastWidth = gWidth;
         lastHeight = gHeight;
         gWidth = gWidth / 2;
         gHeight = gHeight / 2;
         if (gWidth < 1)
         {
            if (gHeight > 1)
            {
               gWidth = 1;
            }
         }
         else if (gHeight < 1)
         {
            gHeight = 1;
         }
      }
      else
      {
         gWidth = 0;
         gHeight = 0;
      }
      mipCount++;
   } // end while we still have mip levels to generate.
} // end main

#define ERROR_LOG_FILE "NormalMapperLog.txt"
//=============================================================================
void 
NmErrorLog (const char *szFmt, ...)
{
   char sz[4096], szMsg[4096];
   
   va_list va;
   va_start(va, szFmt);
   vsprintf(szMsg, szFmt, va);
   va_end(va);
   
   sprintf(sz, "%s", szMsg);
   
   sz[sizeof(sz)-1] = '\0';
   
#if 1
   printf (sz);
#endif
#ifdef _DEBUG
#if 1
   OutputDebugString(sz);
#endif
#endif
#ifdef _DEBUG
#if 0
   static bool first = true;
   FILE *fp;
   if (first == true)
   {
      fp = fopen(ERROR_LOG_FILE, "w");
      if (fp == NULL)
         return;
      first = false;
   }
   else
   {
      fp = fopen(ERROR_LOG_FILE, "a");
      if (fp == NULL)
         return;
   }
   
   fprintf (fp, sz);
   fclose (fp);
   fflush (stdout);
#endif
#endif
}

//=============================================================================
void 
NmErrorLog (char *szFmt)
{
   NmErrorLog((const char *)szFmt);
}
