/*----------------------------------------------------------------------
Copyright (c) 2000 Russ Freeman. All Rights Reserved.
Email: russf@gipsysoft.com
Web site: http://www.gipsysoft.com

This code may be used in compiled form in any way you desire. This
file may be redistributed unmodified by any means PROVIDING it is 
not sold for profit without the authors written consent, and 
providing that this notice and the authors name is included. If 
the source code in this file is used in any commercial application 
then a simple email would be nice.

This file is provided 'as is' with no expressed or implied warranty.
The author accepts no liability if it causes any damage to your
computer.

Expect bugs.

Please use and enjoy. Please let me know of any bugs/mods/improvements 
that you have found/implemented and I will fix/incorporate them into this
file.

File:	ConvertBitmapToDIB.cpp
Owner:	russf@gipsysoft.com
Purpose:	convert a DBB to a DIB
----------------------------------------------------------------------*/
#include "stdafx.h"

// bitmap               - Device dependent bitmap
// dwCompression        - Type of compression - see BITMAPINFOHEADER
// pPal                 - Logical palette
HANDLE DDBToDIB( HBITMAP hBitmap, DWORD dwCompression, HPALETTE hPal ) 
{
        BITMAP                  bm;
        BITMAPINFOHEADER        bi;
        LPBITMAPINFOHEADER      lpbi;
        DWORD                   dwLen;
        HANDLE                  hDIB;
        HANDLE                  handle;
        HDC                     hDC;


        // The function has no arg for bitfields
        if( dwCompression == BI_BITFIELDS )
                return NULL;

        // If a palette has not been supplied use defaul palette
        if (hPal==NULL)
                hPal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);

        // Get bitmap information
        (void)GetObject( hBitmap, sizeof(bm), (LPSTR)&bm );

        // Initialize the bitmapinfoheader
        bi.biSize               = sizeof(BITMAPINFOHEADER);
        bi.biWidth              = bm.bmWidth;
        bi.biHeight             = bm.bmHeight;
        bi.biPlanes             = 1;
        bi.biBitCount           = static_cast<USHORT>( bm.bmPlanes * bm.bmBitsPixel );
        bi.biCompression        = dwCompression;
        bi.biSizeImage          = 0;
        bi.biXPelsPerMeter      = 0;
        bi.biYPelsPerMeter      = 0;
        bi.biClrUsed            = 0;
        bi.biClrImportant       = 0;

        // Compute the size of the  infoheader and the color table
        int nColors = (1 << bi.biBitCount);
        if( nColors > 256 ) 
                nColors = 0;
        dwLen  = bi.biSize + nColors * sizeof(RGBQUAD);

        // We need a device context to get the DIB from
        hDC = GetDC(NULL);
        hPal = SelectPalette(hDC,hPal,FALSE);
        (void)RealizePalette(hDC);

        // Allocate enough memory to hold bitmapinfoheader and color table
        hDIB = GlobalAlloc(GMEM_FIXED,dwLen);

        if (!hDIB){
                (void)SelectPalette(hDC,hPal,FALSE);
                ReleaseDC(NULL,hDC);
                return NULL;
        }

        lpbi = (LPBITMAPINFOHEADER)hDIB;

        *lpbi = bi;

        // Call GetDIBits with a NULL lpBits param, so the device driver 
        // will calculate the biSizeImage field 
        (void)GetDIBits(hDC, hBitmap, 0L, (DWORD)bi.biHeight,
                        (LPBYTE)NULL, (LPBITMAPINFO)lpbi, (DWORD)DIB_RGB_COLORS);

        bi = *lpbi;

        // If the driver did not fill in the biSizeImage field, then compute it
        // Each scan line of the image is aligned on a DWORD (32bit) boundary
        if (bi.biSizeImage == 0){
                bi.biSizeImage = ((((bi.biWidth * bi.biBitCount) + 31) & ~31) / 8) 
                                                * bi.biHeight;

                // If a compression scheme is used the result may infact be larger
                // Increase the size to account for this.
                if (dwCompression != BI_RGB)
                        bi.biSizeImage = (bi.biSizeImage * 3) / 2;
        }

        // Realloc the buffer so that it can hold all the bits
        dwLen += bi.biSizeImage;
				handle = GlobalReAlloc(hDIB, dwLen, GMEM_MOVEABLE);
        if( handle )
                hDIB = handle;
        else{
                GlobalFree(hDIB);

                // Reselect the original palette
                (void)SelectPalette(hDC,hPal,FALSE);
                ReleaseDC(NULL,hDC);
                return NULL;
        }

        // Get the bitmap bits
        lpbi = (LPBITMAPINFOHEADER)hDIB;

        // FINALLY get the DIB
        BOOL bGotBits = GetDIBits( hDC, hBitmap,
                                0L,                             // Start scan line
                                (DWORD)bi.biHeight,             // # of scan lines
                                (LPBYTE)lpbi                    // address for bitmap bits
                                + (bi.biSize + nColors * sizeof(RGBQUAD)),
                                (LPBITMAPINFO)lpbi,             // address of bitmapinfo
                                (DWORD)DIB_RGB_COLORS);         // Use RGB for color table

        if( !bGotBits )
        {
                GlobalFree(hDIB);
                
                (void)SelectPalette(hDC,hPal,FALSE);
                ReleaseDC(NULL,hDC);
                return NULL;
        }

        (void)SelectPalette(hDC,hPal,FALSE);
        ReleaseDC(NULL,hDC);
        return hDIB;
}
