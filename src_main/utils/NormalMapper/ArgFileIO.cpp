/******************************************************************************
 *  ArgFileIO.cpp -- File IO routines for normal mapper.
 ******************************************************************************
 $Header: //depot/3darg/Tools/NormalMapper/ArgFileIO.cpp#1 $
 ******************************************************************************
 *  (C) 2000 ATI Research, Inc.  All rights reserved.
 *****************************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <math.h>

#include "ArgFileIO.h"

////////////////////////////////////////////////////////////////////
// Read an .arg style file
////////////////////////////////////////////////////////////////////
bool 
AtiRead3DARGImageFile (AtiBitmap *bmp, char *fileName)
{
   //===============================================//
   // Open texture map file for binary data reading //
   //===============================================//
   FILE *fp = NULL;
   if ((fp = fopen (fileName, "rb")) == NULL)
   {
      fprintf (stderr, "Warning! Could not open texture map \"%s\"\n", fileName);
      return false;
   }

   //=============//
   // Read header //
   //=============//
   int countRead = fread (bmp, sizeof(AtiBitmap), 1, fp);
   if (countRead != 1)
   {
      fprintf (stderr, "ERROR! Bad arg header!\n");
      fclose (fp);
      return false;
   }

   //============================//
   // Allocate memory for texels //
   //============================//
   int numTexels = bmp->width * bmp->height * (bmp->bitDepth/8);
   bmp->pixels = new unsigned char [numTexels];
   if (bmp->pixels == NULL)
   {
      fprintf (stderr, "ERROR allocAtiIng memory for pixels from file \"%s\"\n", fileName);
      fclose (fp);
      return false;
   }

   //=========================//
   // Read texels into memory //
   //=========================//
   if ((countRead = fread(bmp->pixels, sizeof(unsigned char), numTexels, fp)) != numTexels)
   {
      fprintf (stderr, "ERROR! Couldn't read texels!\n");
      fclose (fp);
      return false;
   }

   //============//
   // Close file //
   //============//
   fclose(fp);

   return true;
}

////////////////////////////////////////////////////////////////////
// Write an .arg style file
////////////////////////////////////////////////////////////////////
bool
AtiWrite3DARGImageFile (AtiBitmap *bmp, char *fileName)
{
   //===============================================//
   // Open texture map file for binary data writing //
   //===============================================//
   FILE *fp = NULL;
   if ((fp = fopen (fileName, "wb")) == NULL)
   {
      fprintf (stderr, "Warning! Could not open texture map \"%s\"\n", fileName);
      return false;
   }

   //=============//
   // Write header //
   //=============//
   int countWritten = fwrite (bmp, sizeof(AtiBitmap), 1, fp);
   if (countWritten != 1)
   {
      fprintf (stderr, "Error writing header!\n");
      fclose (fp);
      return false;
   }

   //=========================//
   // Read texels into memory //
   //=========================//
   int numTexels = bmp->width * bmp->height * (bmp->bitDepth/8);
   if (fwrite (bmp->pixels, sizeof(unsigned char), numTexels, fp) != (unsigned)numTexels)
   {
      fprintf (stderr, "ERROR! Couldn't write texels!\n");
      fclose (fp);
      return false;
   }

   //============//
   // Close file //
   //============//
   fclose(fp);

   return true;
}
