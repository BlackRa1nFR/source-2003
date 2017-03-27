/******************************************************************************
 *  NmFileIO.cpp -- File IO routines for normal mapper.
 ******************************************************************************
 $Header: //depot/3darg/Tools/NormalMapper/NmFileIO.cpp#11 $
 ******************************************************************************
 *  (C) 2000 ATI Research, Inc.  All rights reserved.
 ******************************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <math.h>

#include "NmFileIO.h"

#define EPSILON 1.0e-7
static const double ATI_VERT_TINY = EPSILON;
static const double ATI_NORM_TINY = EPSILON;
static const double ATI_TEX_TINY = EPSILON;
static const double ATI_TAN_ANGLE = 0.3;

////////////////////////////////////////////////////////////////////
// Write a block of triangles to the file
////////////////////////////////////////////////////////////////////
bool
NmWriteTriangles (FILE* fp, int numTris, NmRawTriangle* tris)
{
   // Write the number of triangles.
   if (fwrite (&numTris, sizeof (int), 1, fp) != 1)
   {
      cerr << "Unable to write number of triangles\n";
      return false;
   }
   
   // Write the triangles.
   if (fwrite (tris, sizeof(NmRawTriangle), numTris, fp) !=
          (unsigned)numTris)
   {
      cerr << "Unable to write triangles\n";
      return false;
   }
   return true;
} // end NmWriteTriangles

////////////////////////////////////////////////////////////////////
// Read a block of triangles from the file
////////////////////////////////////////////////////////////////////
bool
NmReadTriangles (FILE* fp, int* numTris, NmRawTriangle** tris)
{
   // Read the number of triangles.
   if (fread (numTris, sizeof (int), 1, fp) != 1)
   {
      cerr << "Unable to read number of triangles\n";
      return false;
   }
   if ((*numTris) < 1)
   {
      cerr << "No triangles found in file\n";
      return false;
   }
   
   // Allocate space for triangles
   (*tris) = new NmRawTriangle [(*numTris)];
   if ((*tris) == NULL)
   {
      cerr << "Unable to allocate space for " << (*tris) << " triangles\n";
      return false;
   }

   // Now read the triangles into the allocated space.
   if (fread ((*tris), sizeof (NmRawTriangle), (*numTris), fp) != 
       (unsigned)(*numTris))
   {
      cerr << "Unable to read triangles\n";
#ifdef _DEBUG
      if (ferror (fp))
      {
         perror ("Read Triangles");
      }
      else if (feof (fp))
      {
         cerr << "End of file!\n";
      }
#endif
      return false;
   }
   
   // success
   return true;
} // end NmReadTriangles

////////////////////////////////////////////////////////////////////
// Compute tangent space for the given triangle list
////////////////////////////////////////////////////////////////////
bool 
NmComputeTangents (int numTris, NmRawTriangle* tris, NmRawTangentSpace** tan)
{
   // Check inputs
   if ((numTris == 0) || (tris == NULL) || (tan == NULL))
   {
      return false;
   }

   // First we need to allocate up the tangent space storage
   (*tan) = new NmRawTangentSpace [numTris];
   if ((*tan) == NULL)
   {
      return false;
   }

   // Cheat and use the double version to compute then convert into floats
   NmRawTangentSpaceD* tanD;
   if (!NmComputeTangentsD(numTris, tris, &tanD))
   {
      return false;
   }

   // Now find tangent space
   for (int i = 0; i < numTris; i++)
   {
      for (int j = 0; j < 3; j++)
      {
         (*tan)[i].tangent[j].x = (float)(tanD[i].tangent[j].x);
         (*tan)[i].tangent[j].y = (float)(tanD[i].tangent[j].y);
         (*tan)[i].tangent[j].z = (float)(tanD[i].tangent[j].z);
         (*tan)[i].binormal[j].x = (float)(tanD[i].binormal[j].x);
         (*tan)[i].binormal[j].y = (float)(tanD[i].binormal[j].y);
         (*tan)[i].binormal[j].z = (float)(tanD[i].binormal[j].z);
      }
   }
   delete [] tanD;
   return true;
} // end NmComputeTangents

//==========================================================================
// Normalize a vector
//==========================================================================
static inline void
NormalizeD (double v[3])
{
   double len = sqrt((v[0]*v[0])+(v[1]*v[1])+(v[2]*v[2]));
   if (len < EPSILON)
   {
      v[0] = 1.0;
      v[1] = 0.0;
      v[2] = 0.0;
   }
   else
   {
      v[0] = v[0]/len;
      v[1] = v[1]/len;
      v[2] = v[2]/len;
   }
}

//==========================================================================
// Calculates the dot product of two vectors.
//==========================================================================
static inline double
DotProduct3D (double vectorA[3], double vectorB[3])
{
   return( (vectorA[0] * vectorB[0]) + (vectorA[1] * vectorB[1]) +
           (vectorA[2] * vectorB[2]) );
} // end of DotProduct

//=============================================================================
// Compare the values
//=============================================================================
static int
NmCompareD (NmTangentPointD* v0, NmTangentPointD* v1)
{
   if (fabs(v0->vertex[0] - v1->vertex[0]) > ATI_VERT_TINY)
   {
      if (v0->vertex[0] > v1->vertex[0])
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
   if (fabs(v0->vertex[1] - v1->vertex[1]) > ATI_VERT_TINY)
   {
      if (v0->vertex[1] > v1->vertex[1])
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
   if (fabs(v0->vertex[2] - v1->vertex[2]) > ATI_VERT_TINY)
   {
      if (v0->vertex[2] > v1->vertex[2])
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
   
   if (fabs(v0->normal[0] - v1->normal[0]) > ATI_NORM_TINY)
   {
      if (v0->normal[0] > v1->normal[0])
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
   if (fabs(v0->normal[1] - v1->normal[1]) > ATI_NORM_TINY)
   {
      if (v0->normal[1] > v1->normal[1])
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
   if (fabs(v0->normal[2] - v1->normal[2]) > ATI_NORM_TINY)
   {
      if (v0->normal[2] > v1->normal[2])
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }

   if (fabs(v0->uv[0] - v1->uv[0]) > ATI_TEX_TINY)
   {
      if (v0->uv[0] > v1->uv[0])
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
   if (fabs(v0->uv[1] - v1->uv[1]) > ATI_TEX_TINY)
   {
      if (v0->uv[1] > v1->uv[1])
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
   double t0[3] = {v0->tangent[0], v0->tangent[1], v0->tangent[2]};
   NormalizeD (t0);
   double t1[3] = {v1->tangent[0], v1->tangent[1], v1->tangent[2]};
   NormalizeD (t1);
   double dp3 = DotProduct3D (t0, t1);
   if (dp3 < ATI_TAN_ANGLE)
   {
      if (dp3 < 0.0)
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
   double b0[3] = {v0->binormal[0], v0->binormal[1], v0->binormal[2]};
   NormalizeD (b0);
   double b1[3] = {v1->binormal[0], v1->binormal[1], v1->binormal[2]};
   NormalizeD (b1);
   dp3 = DotProduct3D (b0, b1);
   if (dp3 < ATI_TAN_ANGLE)
   {
      if (dp3 < 0.0)
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
   return 0;
}

//=============================================================================
// Binary traversal function
//=============================================================================
static int 
NmIndexBinaryTraverseD(NmTangentPointD* value, // The reference element
                       NmTangentPointD* data,// pointer to the indexed data
                       int* indices,         // pointer index
                       int count,            // number of items in the array
                       int* result) // buffer for the result of the last compare
{
   int high;
   int low;
   int mid;
   int nextMid;
   
   high = count;
   low = 0;
   mid = low + ((high - low) >> 1);
   *result = -1;

   while (low != high) 
   {
      *result = NmCompareD (value, &(data[indices[mid]])); 
      if (*result == 0)
      {
         break;
      }
      else if (*result < 0)
      {
         high = mid;
      }
      else
      {
         low = mid;
      }
     
      nextMid = low + ((high - low) >> 1);
      if (mid == nextMid)
      {
         break;
      }

      mid = nextMid;
   }

   return mid;
}

////////////////////////////////////////////////////////////////////
// Compute tangent space for the given triangle
////////////////////////////////////////////////////////////////////
static void
ComputeTangentVectorsD (NmRawTriangle* pgon, int idx,
                        double tan[3], double norm[3], double binorm[3])
{
   // Clear outputs.
   tan[0] = 1.0; tan[1] = 0.0; tan[2] = 0.0;
   binorm[0] = 0.0; binorm[1] = 0.0; binorm[2] = 1.0;
   norm[0] = pgon->norm[idx].x;
   norm[1] = pgon->norm[idx].y;
   norm[2] = pgon->norm[idx].z;

   // Make sure we have valid data.
   if (pgon->texCoord[0].u == 0.0 && 
       pgon->texCoord[1].u == 0.0 && 
       pgon->texCoord[2].u == 0.0)
   {
      return;
   }

   // Compute edge vectors.
   double vec1[3];
   vec1[0] = pgon->vert[1].x - pgon->vert[0].x;
   vec1[1] = pgon->vert[1].y - pgon->vert[0].y;
   vec1[2] = pgon->vert[1].z - pgon->vert[0].z;
   double vec2[3];
   vec2[0] = pgon->vert[2].x - pgon->vert[0].x;
   vec2[1] = pgon->vert[2].y - pgon->vert[0].y;
   vec2[2] = pgon->vert[2].z - pgon->vert[0].z;

   // Calculate u vector
   double u1 = pgon->texCoord[1].u - pgon->texCoord[0].u;
   double u2 = pgon->texCoord[2].u - pgon->texCoord[0].u;
   double v1 = pgon->texCoord[1].v - pgon->texCoord[0].v;
   double v2 = pgon->texCoord[2].v - pgon->texCoord[0].v;

   double a = (u1 - v1*u2/v2);
   if (a != 0.0)
   {
      a = 1.0/a;
   }
   double b = (u2 - v2*u1/v1);
   if (b != 0.0)
   {
      b = 1.0/b;
   }

   double duTmp[3];
   duTmp[0] = a*vec1[0] + b*vec2[0];
   duTmp[1] = a*vec1[1] + b*vec2[1];
   duTmp[2] = a*vec1[2] + b*vec2[2];
   double tmpf = 1.0 / sqrt((duTmp[0]*duTmp[0]) + (duTmp[1]*duTmp[1]) +
                               (duTmp[2]*duTmp[2]));
   duTmp[0] *= tmpf;
   duTmp[1] *= tmpf;
   duTmp[2] *= tmpf;

   // Calculate v vector
   a = (v1 - u1*v2/u2);
   if (a != 0.0)
   {
      a = 1.0/a;
   }
   b = (v2 - u2*v1/u1);
   if (b != 0.0)
   {
      b = 1.0/b;
   }

   double dvTmp[3];
   dvTmp[0] = a*vec1[0] + b*vec2[0];
   dvTmp[1] = a*vec1[1] + b*vec2[1];
   dvTmp[2] = a*vec1[2] + b*vec2[2];
   tmpf = 1.0 / sqrt((dvTmp[0]*dvTmp[0]) + (dvTmp[1]*dvTmp[1]) +
                      (dvTmp[2]*dvTmp[2]));
   dvTmp[0] *= tmpf;
   dvTmp[1] *= tmpf;
   dvTmp[2] *= tmpf;

   // Project the vector into the plane defined by the normal.
   tmpf = (duTmp[0]*norm[0]) + (duTmp[1]*norm[1]) + (duTmp[2]*norm[2]);
   tan[0] = duTmp[0] - (tmpf * norm[0]);
   tan[1] = duTmp[1] - (tmpf * norm[1]);
   tan[2] = duTmp[2] - (tmpf * norm[2]);
   tmpf = 1.0 / sqrt(tan[0]*tan[0] + tan[1]*tan[1] + tan[2]*tan[2]);
   tan[0] *= tmpf;
   tan[1] *= tmpf;
   tan[2] *= tmpf;

   tmpf = (dvTmp[0]*norm[0]) + (dvTmp[1]*norm[1]) + (dvTmp[2]*norm[2]);
   binorm[0] = dvTmp[0] - (tmpf * norm[0]);
   binorm[1] = dvTmp[1] - (tmpf * norm[1]);
   binorm[2] = dvTmp[2] - (tmpf * norm[2]);
   tmpf = 1.0 / sqrt(binorm[0]*binorm[0] + binorm[1]*binorm[1] +
                             binorm[2]*binorm[2]);
   binorm[0] *= tmpf;
   binorm[1] *= tmpf;
   binorm[2] *= tmpf;
} // end ComputeTangentVectors

////////////////////////////////////////////////////////////////////
// Copy data into a point structure
////////////////////////////////////////////////////////////////////
static void
NmCopyPointD (NmRawTriangle* tri, int v, NmTangentPointD* point)
{
   point->vertex[0] = tri->vert[v].x;
   point->vertex[1] = tri->vert[v].y;
   point->vertex[2] = tri->vert[v].z;
   point->normal[0] = tri->norm[v].x;
   point->normal[1] = tri->norm[v].y;
   point->normal[2] = tri->norm[v].z;
   point->uv[0] = tri->texCoord[v].u;
   point->uv[1] = tri->texCoord[v].v;
   double norm[3];
   ComputeTangentVectorsD (tri, v, point->tangent, norm, point->binormal);
   point->count = 1;
}

////////////////////////////////////////////////////////////////////
// Insert a point and get it's index.
////////////////////////////////////////////////////////////////////
static int
NmInsertD (NmRawTriangle* tri, int v, int* num, int* sortIndex,
           NmTangentPointD* point)
{
   // Make sure we have some stuff to check.
   int ret = 0;
   if ((*num) > 0)
   {
      // Copy point into available slot.
      NmTangentPointD* pt = &(point[(*num)]);
      NmCopyPointD (tri, v, pt);

      // Search for it.
      int compValue;
      int pos = NmIndexBinaryTraverseD (pt, point, sortIndex, (*num),
                                        &compValue);
      
      // Now see if we need to insert.
      if (compValue == 0)
      {
         point[sortIndex[pos]].tangent[0] += pt->tangent[0];
         point[sortIndex[pos]].tangent[1] += pt->tangent[1];
         point[sortIndex[pos]].tangent[2] += pt->tangent[2];
         point[sortIndex[pos]].binormal[0] += pt->binormal[0];
         point[sortIndex[pos]].binormal[1] += pt->binormal[1];
         point[sortIndex[pos]].binormal[2] += pt->binormal[2];
         point[sortIndex[pos]].count++;
         ret = sortIndex[pos];
      }
      else if (compValue < 0) // we are inserting before this index
      {
         ret = (*num);
         memmove (&(sortIndex[pos + 1]), &(sortIndex[pos]), 
                  ((*num) - pos) * sizeof(int));
         
         sortIndex[pos] = (*num);
         (*num)++;
      }
      else // we are appending after this index
      {
         ret = (*num);
         if (pos < ((*num) - 1))
         {
            memmove(&(sortIndex[pos + 2]), &(sortIndex[pos + 1]), 
                    ((*num) - pos - 1) * sizeof(int));
         }
         sortIndex[pos + 1] = (*num);
         (*num)++;
      }
   }
   else
   {  // First point just add it into our list.
      NmCopyPointD (tri, v, &(point[(*num)]));
      sortIndex[(*num)] = 0;
      ret = (*num);
      (*num)++;
   }
   return ret;
}

////////////////////////////////////////////////////////////////////
// Compute tangent space for the given triangle list
////////////////////////////////////////////////////////////////////
bool 
NmComputeTangentsD (int numTris, NmRawTriangle* tris, NmRawTangentSpaceD** tan)
{
   // Check inputs
   if ((numTris == 0) || (tris == NULL) || (tan == NULL))
   {
      return false;
   }

   // First we need to allocate up the tangent space storage
   (*tan) = new NmRawTangentSpaceD [numTris];
   if ((*tan) == NULL)
   {
      return false;
   }

   // Allocate storage structures
   NmIndex* index = new NmIndex [numTris];
   if (index == NULL)
   {
      return false;
   }
   int* sortIndex = new int [numTris*3]; // Brute force it
   NmTangentPointD* point = new NmTangentPointD [numTris*3]; // Brute force it
   if (point == NULL)
   {
      return false;
   }
   
   // Now go through finding matching vertices and computing tangents.
   int count = 0;
   int i;
   for (i = 0; i < numTris; i++)
   {
      for (int j = 0; j < 3; j++)
      {
         index[i].idx[j] = NmInsertD (&tris[i], j, &count, sortIndex, point);
      }      
   }

   // Next we renormalize
   for (i = 0; i < count; i++)
   {
      point[i].tangent[0] = point[i].tangent[0]/(double)point[i].count;
      point[i].tangent[1] = point[i].tangent[1]/(double)point[i].count;
      point[i].tangent[2] = point[i].tangent[2]/(double)point[i].count;
      NormalizeD (point[i].tangent);

      point[i].binormal[0] = point[i].binormal[0]/(double)point[i].count;
      point[i].binormal[1] = point[i].binormal[1]/(double)point[i].count;
      point[i].binormal[2] = point[i].binormal[2]/(double)point[i].count;
      NormalizeD (point[i].binormal);
   }

   // Copy tangent data into structures
   for (i = 0; i < numTris; i++)
   {
      for (int j = 0; j < 3; j++)
      {
         (*tan)[i].tangent[j].x = point[index[i].idx[j]].tangent[0];
         (*tan)[i].tangent[j].y = point[index[i].idx[j]].tangent[1];
         (*tan)[i].tangent[j].z = point[index[i].idx[j]].tangent[2];
         (*tan)[i].binormal[j].x = point[index[i].idx[j]].binormal[0];
         (*tan)[i].binormal[j].y = point[index[i].idx[j]].binormal[1];
         (*tan)[i].binormal[j].z = point[index[i].idx[j]].binormal[2];
      }
   }

   // Clean up
   delete [] index;
   delete [] sortIndex;
   delete [] point;

   // Success!
   return true;
} // end NmComputeTangent
