/******************************************************************************
 *  TGAIO.cpp -- File IO routines for Targa files.
 ******************************************************************************
 $Header: //depot/3darg/Tools/NormalMapper/TGAIO.cpp#3 $
 ******************************************************************************
 *  (C) 2000 ATI Research, Inc.  All rights reserved.
 ******************************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <math.h>

#include "TGAIO.h"

////////////////////////////////////////////////////////////////////
// Write the TGA image.
////////////////////////////////////////////////////////////////////
bool 
TGAWriteImage (FILE* fp, int width, int height, int bitDepth, BYTE* image,
               bool aSwapRedBlue)
{
   int i;
   int bytesRead;
   TGAHeaderInfo TGAHeader;
   BYTE *texels;
   
   BYTE *ptr1;
   BYTE *ptr2;
   
   /***************************/
   /* Check for NULL pointers */
   /***************************/
   if ((image == NULL) || (fp == NULL))
   {
      fprintf (stderr, "ERROR: NULL image or file pointer!\n");
      return false;
   }
   
   /***********************/
   /* Check type of image */
   /***********************/
   if ((bitDepth != 24) && (bitDepth != 32))
   {
      fprintf (stderr, "ERROR! Can only write 24 and 32 bit images! This image is %d bits.\n", bitDepth);
      return false;
   }
   
   /****************************/
   /* Swap red and blue colors */
   /****************************/
   texels = new BYTE [width*height*bitDepth/8];
   if (texels == NULL)
   {
      fprintf (stderr, "ERROR allocAtiIng memory for texel array!\n");
      return false;
   }
   
   ptr1 = image;
   ptr2 = texels;
   for (i=0; i<(width*height); i++)
   {
      if (aSwapRedBlue == false)
      { //Note: TGA is swapped by default
         ptr2[0] = ptr1[2];
         ptr2[1] = ptr1[1];
         ptr2[2] = ptr1[0];
      }
      else
      {
         ptr2[0] = ptr1[0];
         ptr2[1] = ptr1[1];
         ptr2[2] = ptr1[2];
      }

      if (bitDepth == 24)
      {
         ptr1 += 3;
         ptr2 += 3;
      }
      else if (bitDepth == 32)
      {
         ptr2[3] = ptr1[3];
         ptr1 += 4;
         ptr2 += 4;
      }
   }
   
   /***************/
   /* Fill header */
   /***************/
   TGAHeader.idlen = 0;    //length of optional identification sequence
   TGAHeader.cmtype = 0;   //indicates whether a palette is present
   TGAHeader.imtype = 2;   //image data type (e.g., uncompressed RGB)
   TGAHeader.cmorg = 0;    //first palette index, if present
   TGAHeader.cmcnt = 0;    //number of palette entries, if present
   TGAHeader.cmsize = 0;   //number of bits per palette entry
   TGAHeader.imxorg = 0;   //horiz pixel coordinate of lower left of image
   TGAHeader.imyorg = 0;   //vert pixel coordinate of lower left of image
   TGAHeader.imwidth = (unsigned short)width;  //image width in pixels
   TGAHeader.imheight = (unsigned short)height; //image height in pixels
   TGAHeader.imdepth = (BYTE)bitDepth;  //image color depth (bits per pixel)
   TGAHeader.imdesc = 8;   //image attribute flags
   
   /**********************/
   /* Write TARGA header */
   /**********************/
   if ((bytesRead = fwrite(&TGAHeader, sizeof(TGAHeader), 1, fp)) != 1)
   {
      fprintf (stderr, "ERROR! Targa header not written successfully!\n");
      return false;
   }
   
   /****************/
   /* Write texels */
   /****************/
   for (i=0; i<width*height*bitDepth/8; i+=8192000)
   {
      int j = 8192000;
      if (i+8192000 > width*height*bitDepth/8)
         j = width*height*bitDepth/8 - i;
      if ((bytesRead = fwrite(&texels[i], sizeof(BYTE), j, fp)) != j)
      {
         fprintf (stderr, "ERROR! Texels not written successfully! Wrote %d instead of  %d\n", bytesRead, j);
         return false;
      }
   }

   /***************/
   /* Free texels */
   /***************/
   if (texels != NULL)
   {
      delete [] texels;
      texels = NULL;
   }
   return true;
}

//=============================================================================
bool
TGAReadImage (FILE* fp, int* width, int* height, int* bitDepth, BYTE** pixels)
{
   //===================//
   // Read TARGA header //
   //===================//
   TGAHeaderInfo TGAHeader;
   if (fread((&TGAHeader), sizeof(TGAHeaderInfo), 1, fp) != 1)
   {
      fprintf (stderr, "ERROR! Bad Targa header!\n");
      return false;
   }

   (*width) = TGAHeader.imwidth;
   (*height) = TGAHeader.imheight;
   (*bitDepth) = TGAHeader.imdepth;
   int numTexels = (*width) * (*height) * ((*bitDepth)/8);

   //=====================================================================//
   // Skip descriptive bytes at end of header, idlen specifies the number //
   //=====================================================================//
   if (fseek (fp, TGAHeader.idlen, SEEK_CUR) != 0)
   {
      fprintf (stderr, "ERROR! Couldn't seek past Targa header!\n");
      return false;
   }

   //============================//
   // Allocate memory for texels //
   //============================//
   (*pixels) = new BYTE [numTexels];
   if ((*pixels) == NULL)
   {
      fprintf (stderr, "ERROR allocating memory for pixels\n");
      return false;
   }

   //=========================//
   // Read texels into memory //
   //=========================//
   if (fread ((*pixels), sizeof(BYTE), numTexels, fp) != (unsigned)numTexels)
   {      
      fprintf (stderr, "ERROR! Couldn't read texels!\n");
      return false;
   }

   return true;
}
