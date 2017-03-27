/******************************************************************************
 *  NmFileIO.h -- The interface into the normal map file IO routines.
 ******************************************************************************
 $Header: //depot/3darg/Tools/NormalMapper/NmFileIO.h#6 $
 ******************************************************************************
 *  (C) 2000 ATI Research, Inc.  All rights reserved.
 ******************************************************************************/

#ifndef __NMFILEIO__H
#define __NMFILEIO__H

#include <stdio.h>

#pragma pack (push)
#pragma pack (1)   //dont pad the following struct

typedef union
{
   struct { float x, y, z; };
   struct { float v[3]; };
} NmRawPoint;

typedef union
{
   struct { double x, y, z; };
   struct { double v[3]; };
} NmRawPointD;

typedef union
{
   struct { float u, v; };
   struct { float uv[2]; };
} NmRawTexCoord;

typedef struct
{
   NmRawPoint vert[3];
   NmRawPoint norm[3];
   NmRawTexCoord texCoord[3];
} NmRawTriangle;

// Tangent space structure.
typedef struct
{
   NmRawPoint tangent[3];
   NmRawPoint binormal[3];
} NmRawTangentSpace;
typedef struct
{
   NmRawPointD tangent[3];
   NmRawPointD binormal[3];
} NmRawTangentSpaceD;


// index structure.
typedef struct
{
   int idx[3];
} NmIndex;

// A tangent point for computing smooth tangent space
typedef struct
{
   double vertex[3];
   double normal[3];
   double uv[2];
   double tangent[3];
   double binormal[3];
   int count;
} NmTangentPointD;

#pragma pack (pop)

extern bool NmReadTriangles (FILE* fp, int* numTris, NmRawTriangle** tris);
extern bool NmWriteTriangles (FILE* fp, int numTris, NmRawTriangle* tris);
extern bool NmComputeTangents (int numTris, NmRawTriangle* tris, 
                               NmRawTangentSpace** tan);
extern bool NmComputeTangentsD (int numTris, NmRawTriangle* tris, 
                                NmRawTangentSpaceD** tan);

#endif // __NMFILEIO__H
