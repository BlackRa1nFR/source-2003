/******************************************************************************
 *  ArgFileIO.h -- The interface into the .arg file IO routines
 ******************************************************************************
 $Header: //depot/3darg/Tools/NormalMapper/ArgFileIO.h#1 $
 ******************************************************************************
 *  (C) 2000 ATI Research, Inc.  All rights reserved.
 ******************************************************************************/

#ifndef __ARGFILEIO__H
#define __ARGFILEIO__H

#include <stdio.h>

#pragma pack (push)
#pragma pack (1)   //dont pad the following struct


//=============================================================================
typedef enum
{
   ATI_BITMAP_FORMAT_UNKNOWN = 0,
   ATI_BITMAP_FORMAT_8,
   ATI_BITMAP_FORMAT_88,
   ATI_BITMAP_FORMAT_888,
   ATI_BITMAP_FORMAT_8888,
   ATI_BITMAP_FORMAT_16,
   ATI_BITMAP_FORMAT_1616,
   ATI_BITMAP_FORMAT_16161616,
   ATI_BITMAP_FORMAT_S1616,
   ATI_BITMAP_FORMAT_S16161616,
   ATI_BITMAP_FORMAT_1010102,
   //ATI_BITMAP_FORMAT_111110,
   ATI_BITMAP_FORMAT_FORCE_DWORD = 0xffffffff
} AtiBitmapFormatEnum;

typedef struct
{
   unsigned short width;
   unsigned short height;
   unsigned char bitDepth;
   AtiBitmapFormatEnum format;
   unsigned char *pixels;
} AtiBitmap;

#pragma pack (pop)

extern bool AtiRead3DARGImageFile (AtiBitmap *bmp, char *fileName);
extern bool AtiWrite3DARGImageFile (AtiBitmap *bmp, char *fileName);

#endif // __ARGFILEIO__H
