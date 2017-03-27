/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	DecodeBMP.cpp
Owner:	rich@woodbridgeinternalmed.com
Purpose:	Decode an BMP file into a frame array.
			The only exported function is:
			bool DecodeBMP( CDataSourceABC &ds, CFrameArray &arrFrames, int &nWidth, int &nHeight )
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "DataSourceABC.h"
#include <ImgLib.h>
#include "ImgLibInt.h"
#include "Config.h"


#ifdef TRACING
	#define ITRACE TRACE
#else	//	TRACING
	#define ITRACE 
#endif	//	TRACING

#ifdef IMGLIB_BMP

static void ConvertCoreHeader(BITMAPCOREHEADER* coreHeader, BITMAPINFOHEADER* infoHeader)
{
   infoHeader->biWidth = coreHeader->bcWidth;
   infoHeader->biHeight = coreHeader->bcHeight;
   infoHeader->biPlanes = coreHeader->bcPlanes;
   infoHeader->biBitCount = coreHeader->bcBitCount;
   infoHeader->biCompression = BI_RGB;
   infoHeader->biSizeImage = 0;
   infoHeader->biXPelsPerMeter = 0;
   infoHeader->biYPelsPerMeter = 0;
   infoHeader->biClrUsed = 0;
   infoHeader->biClrImportant = 0;
}

static void ConvertCoreColors(RGBQUAD* col, int n)
{
   for (int i = n - 1; i >= 0; i--)
   {
      col[i].rgbRed = ((RGBTRIPLE*)col)[i].rgbtRed;
      col[i].rgbGreen = ((RGBTRIPLE*)col)[i].rgbtGreen;
      col[i].rgbBlue = ((RGBTRIPLE*)col)[i].rgbtBlue;
      col[i].rgbReserved = 0xFF;
   }
}


bool DecodeBMP(CDataSourceABC &ds, CFrameArray &arrFrames, SIZE &size )
{
	// Attempt to read and verify the signature by reading the file header
	BITMAPFILEHEADER bmf;
	BITMAPINFOHEADER infoHeader;
	DWORD offBits = 0;
	DWORD headerSize;
	bool isCoreFile;
	CDib* pDib = 0;
	
	try {
		if ( !ds.ReadBytes((BYTE*)&bmf, sizeof(BITMAPFILEHEADER)) )
			throw 1;
		
		// Determine if it has a BITMAPFILEHEADER
		if (bmf.bfType != 0x4d42)
		{
			ds.Reset();
		}
		else
		{
			// Save offset from file header, if it makes sense
			if (bmf.bfOffBits)
				offBits = bmf.bfOffBits - sizeof(BITMAPFILEHEADER);			
		}

		// Read the header size, and validate
		if ( !ds.ReadBytes((BYTE*)&headerSize, sizeof(DWORD)) || 
				(headerSize != sizeof(BITMAPINFOHEADER) &&
				headerSize != sizeof(BITMAPCOREHEADER) ) )
			throw 1;
		
		isCoreFile = (headerSize == sizeof(BITMAPCOREHEADER));
		
		// Prepare the header structure, correcting possible errors in the file
		infoHeader.biSize = sizeof(BITMAPINFOHEADER);
		
		// If its a core bitmap, convert the header
		if (isCoreFile)
		{
			BITMAPCOREHEADER coreHeader;
			if ( !ds.ReadBytes((BYTE*)&coreHeader.bcWidth, sizeof(BITMAPCOREHEADER) - sizeof(DWORD)) )
				throw 1;
			ConvertCoreHeader(&coreHeader, &infoHeader);
		}
		else
		{
			if( !ds.ReadBytes((BYTE*)&infoHeader.biWidth,
				sizeof(BITMAPINFOHEADER) - sizeof(DWORD)) )
				throw 1;
		}
		
		pDib = new CDib(&infoHeader);
		
		// Read the masks
		if (pDib->GetMaskTable())
		{
			if ( !ds.ReadBytes((BYTE*)pDib->GetMaskTable(), 3 * sizeof(DWORD)) )
				throw 1;
		}
			
		// Calculate number of colors in color table
		if (pDib->GetColorTable())
		{
			int colors = pDib->GetColorsUsed();
			// Calculate size of color table to read
			int colorsize = colors *
				( isCoreFile ? sizeof(RGBTRIPLE) : sizeof(RGBQUAD));
			
			if ( !ds.ReadBytes((BYTE*)pDib->GetColorTable(), colorsize) )
				throw 1;
			if (isCoreFile)
				ConvertCoreColors(pDib->GetColorTable(), colors);
		}			
		// Skip bits if necessary
		DWORD curPos = ds.GetCurrentPos();
		if (offBits && (DWORD)offBits > curPos)
			ds.SetRelativePos(offBits - curPos);
			
		if ( !ds.ReadBytes(pDib->GetBits(), pDib->GetAllocatedImageSize()) )
			throw 1;
		size.cx = pDib->GetWidth();
		size.cy = pDib->GetHeight();
		CFrame *pFrame = new CFrame( pDib, 0 );
		arrFrames.Add( pFrame );
	}
	catch (...) 
	{
		if( pDib )
		{
			delete pDib;
			pDib = NULL;
		}
	}

	if( pDib )
		return true;
	return false;
}

#endif	//	IMGLIB_BMP