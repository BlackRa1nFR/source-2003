/******************************************************************************
 *  TGAIO.h -- Targa file IO descriptions
 ******************************************************************************
 $Header: //depot/3darg/Tools/NormalMapper/TGAIO.h#3 $
 ******************************************************************************
 *  (C) 2000 ATI Research, Inc.  All rights reserved.
 ******************************************************************************/

#ifndef __TGAIO__H
#define __TGAIO__H

#include <windows.h>
#include <stdio.h>

//Targa header info
#pragma pack (push)
#pragma pack (1)	//dont pad the following struct

typedef struct _TGAHeaderInfo
{
   BYTE idlen;    //length of optional identification sequence
   BYTE cmtype;   //indicates whether a palette is present
   BYTE imtype;   //image data type (e.g., uncompressed RGB)
   WORD cmorg;    //first palette index, if present
   WORD cmcnt;    //number of palette entries, if present
   BYTE cmsize;   //number of bits per palette entry
   WORD imxorg;   //horiz pixel coordinate of lower left of image
   WORD imyorg;   //vert pixel coordinate of lower left of image
   WORD imwidth;  //image width in pixels
   WORD imheight; //image height in pixels
   BYTE imdepth;  //image color depth (bits per pixel)
   BYTE imdesc;   //image attribute flags
}TGAHeaderInfo;

typedef struct _pixel
{
   BYTE red;
   BYTE blue;
   BYTE green;
   BYTE alpha;
} pixel;

#pragma pack (pop)

extern bool TGAWriteImage (FILE* fp, int width, int height, int bpp,
                           BYTE* image, bool aSwapRedBlue = false);
extern bool TGAReadImage (FILE* fp, int* width, int* height, int* bitDepth,
                          BYTE** pixels);

#endif // __TGAIO__H
