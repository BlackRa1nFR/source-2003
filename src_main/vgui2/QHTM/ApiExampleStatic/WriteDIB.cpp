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

File:	WriteDIB.cpp
Owner:	russf@gipsysoft.com
Purpose:	Provide a method of writing a DIB to file.
----------------------------------------------------------------------*/
#include "stdafx.h"


#define WIDTHBYTES(bits)      ((((bits) + 31)>>5)<<2) 

WORD DibNumColors ( VOID FAR * pv ) 
{ 
    int bits; 
    LPBITMAPINFOHEADER lpbi; 
    LPBITMAPCOREHEADER lpbc; 
 
    lpbi = ((LPBITMAPINFOHEADER)pv); 
    lpbc = ((LPBITMAPCOREHEADER)pv); 
 
    /*  With the BITMAPINFO format headers, the size of the palette 
     *  is in biClrUsed, whereas in the BITMAPCORE - style headers, it 
     *  is dependent on the bits per pixel ( = 2 raised to the power of 
     *  bits/pixel). 
     */ 
    if (lpbi->biSize != sizeof(BITMAPCOREHEADER)) 
    { 
        if (lpbi->biClrUsed != 0) 
            return (WORD)lpbi->biClrUsed; 
        bits = lpbi->biBitCount; 
    } 
    else 
        bits = lpbc->bcBitCount; 
 
    switch (bits) 
    { 
    case 1: 
        return 2; 
    case 4: 
        return 16; 
    case 8: 
        return 256; 
    default: 
        /* A 24 bitcount DIB has no color table */ 
        return 0; 
    } 
} 


WORD PaletteSize (VOID FAR * pv) 
{ 
    LPBITMAPINFOHEADER lpbi; 
    WORD NumColors; 
 
    lpbi      = (LPBITMAPINFOHEADER)pv; 
    NumColors = DibNumColors(lpbi); 
 
    if (lpbi->biSize == sizeof(BITMAPCOREHEADER)) 
        return static_cast<WORD>( NumColors * sizeof(RGBTRIPLE) );
    else 
        return static_cast<WORD>( NumColors * sizeof(RGBQUAD) ); 
} 


#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B') 

bool WriteDIB( LPCTSTR szFile, HANDLE hdib )
{
    BITMAPFILEHEADER    bmfHdr;     // Header for Bitmap file 
    LPBITMAPINFOHEADER  lpBI;       // Pointer to DIB info structure 
    HANDLE              fh;         // file handle for opened file 
    DWORD               dwDIBSize; 
    DWORD               dwWritten; 
 
    if (!hdib) 
        return false; 
 
    fh = CreateFile(szFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL); 
 
    if (fh == INVALID_HANDLE_VALUE) 
        return false; 
 
    // Get a pointer to the DIB memory, the first of which contains 
    // a BITMAPINFO structure 
 
    lpBI = (LPBITMAPINFOHEADER)GlobalLock(hdib); 
    if (!lpBI) 
    { 
        (void)CloseHandle(fh); 
        return false; 
    } 
 
    // Check to see if we're dealing with an OS/2 DIB.  If so, don't 
    // save it because our functions aren't written to deal with these 
    // DIBs. 
 
    if (lpBI->biSize != sizeof(BITMAPINFOHEADER)) 
    { 
        GlobalUnlock(hdib); 
        (void)CloseHandle(fh); 
        return false; 
    } 
 
    // Fill in the fields of the file header 
 
    // Fill in file type (first 2 bytes must be "BM" for a bitmap) 
 
    bmfHdr.bfType = DIB_HEADER_MARKER;  // "BM" 
 
    // Calculating the size of the DIB is a bit tricky (if we want to 
    // do it right).  The easiest way to do this is to call GlobalSize() 
    // on our global handle, but since the size of our global memory may have 
    // been padded a few bytes, we may end up writing out a few too 
    // many bytes to the file (which may cause problems with some apps, 
    // like HC 3.0). 
    // 
    // So, instead let's calculate the size manually. 
    // 
    // To do this, find size of header plus size of color table.  Since the 
    // first DWORD in both BITMAPINFOHEADER and BITMAPCOREHEADER conains 
    // the size of the structure, let's use this. 
 
    // Partial Calculation 
 
    dwDIBSize = *(LPDWORD)lpBI + PaletteSize((LPSTR)lpBI);
 
    // Now calculate the size of the image 
 
    // It's an RLE bitmap, we can't calculate size, so trust the biSizeImage 
    // field 
 
    if ((lpBI->biCompression == BI_RLE8) || (lpBI->biCompression == BI_RLE4)) 
        dwDIBSize += lpBI->biSizeImage; 
    else 
    { 
        DWORD dwBmBitsSize;  // Size of Bitmap Bits only 
 
        // It's not RLE, so size is Width (DWORD aligned) * Height 
 
        dwBmBitsSize = WIDTHBYTES((lpBI->biWidth)*((DWORD)lpBI->biBitCount)) * 
                lpBI->biHeight; 
 
        dwDIBSize += dwBmBitsSize; 
 
        // Now, since we have calculated the correct size, why don't we 
        // fill in the biSizeImage field (this will fix any .BMP files which  
        // have this field incorrect). 
 
        lpBI->biSizeImage = dwBmBitsSize; 
    } 
 
 
    // Calculate the file size by adding the DIB size to sizeof(BITMAPFILEHEADER) 
                    
    bmfHdr.bfSize = dwDIBSize + sizeof(BITMAPFILEHEADER); 
    bmfHdr.bfReserved1 = 0; 
    bmfHdr.bfReserved2 = 0; 
 
    // Now, calculate the offset the actual bitmap bits will be in 
    // the file -- It's the Bitmap file header plus the DIB header, 
    // plus the size of the color table. 
     
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + lpBI->biSize + 
            PaletteSize((LPSTR)lpBI); 
 
    // Write the file header 
 
    (void)WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL); 
 
    // Write the DIB header and the bits -- use local version of 
    // MyWrite, so we can write more than 32767 bytes of data 
     
    (void)WriteFile(fh, (LPSTR)lpBI, dwDIBSize, &dwWritten, NULL); 
 
    GlobalUnlock(hdib); 
    (void)CloseHandle(fh); 
 
    if (dwWritten == 0) 
        return false; // oops, something happened in the write 
	return true;
}