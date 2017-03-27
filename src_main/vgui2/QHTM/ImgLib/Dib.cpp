/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	Dib.cpp
Owner:	russf@gipsysoft.com
Purpose:	Device indepenedent bitmap.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "ImgLibInt.h"
#include <ImgLib.h>
#include "Config.h"

inline static long GetBytesPerLine( long width, long bpp )
//
//	returns the number of bytes required for a single line.
{
	return ((((width * bpp) + 31) & ~31) >> 3);
}


inline static long GetBytesMemNeeded( long width, long height, long bpp )
//
//	Return the number of bytes need for the total image
{
  return GetBytesPerLine( width, bpp ) * height;
}


static LPBITMAPINFO DibCreate( int bits, int dx, int dy, int compression = BI_RGB )
//
//	Given a bit depth and size create a BITMAPINFO structure complete with color table
//	Color table is only created and not initialised.
//	Also, the color tabel exists at the end of the structure.
//	Corrected to handle 16 and 32 bit images
{

	int nColorCount = 0;
	if( bits <= 8 )
		nColorCount = 1 << bits;

  DWORD dwSizeImage;
  dwSizeImage = dy * (( dx * bits / 8 + 3 ) & ~ 3 );

  // Calculate size of mask table, if needed
  DWORD dwMaskSpace = (compression == BI_BITFIELDS ? 3 * sizeof(DWORD) : 0);
  
  LPBITMAPINFO lpbi = (BITMAPINFO*)new char[ sizeof( BITMAPINFO )+ dwSizeImage + ( nColorCount * sizeof( RGBQUAD ) ) + dwMaskSpace ];

  if (lpbi == NULL)
      return NULL;

  lpbi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER) ;
  lpbi->bmiHeader.biWidth           = dx;
  lpbi->bmiHeader.biHeight          = dy;
  lpbi->bmiHeader.biPlanes          = 1;
  lpbi->bmiHeader.biBitCount        = static_cast<WORD>( bits );
  lpbi->bmiHeader.biCompression     = compression ;
  lpbi->bmiHeader.biSizeImage       = dwSizeImage;
  lpbi->bmiHeader.biXPelsPerMeter   = 0 ;
  lpbi->bmiHeader.biYPelsPerMeter   = 0 ;
  lpbi->bmiHeader.biClrUsed         = 0 ;
  lpbi->bmiHeader.biClrImportant    = 0 ;
  return lpbi;
}

CDib::CDib( int nWidth, int nHeight, int nBPP )
	: m_nWidth( nWidth )
	, m_nHeight( nHeight )
	, m_nBPP( nBPP )
	, m_pBMI( DibCreate( nBPP, nWidth, nHeight ) )
	, m_pClrTab( NULL )
	, m_pClrMasks( NULL )
	, m_pBits( NULL )
{
	if( nBPP <= 8 )
	{
    m_pClrTab = (RGBQUAD *)(m_pBMI->bmiColors);
	}

	m_dwImageSize = GetBytesMemNeeded( m_nWidth, m_nHeight, m_nBPP );
	m_pBits = new BYTE [ m_dwImageSize ];
}

// Create a Dib directly from a BITMAPINFO structure. 
// This makes a copy of the passed structure
// This is generally used when loading BMP/RLE files
CDib::CDib( LPBITMAPINFOHEADER lpbhi)
	: m_nWidth( 0 )
	, m_nHeight( 0 )
	, m_nBPP( 0 )
	, m_pBMI( NULL )
	, m_pClrTab( NULL )
	, m_pClrMasks( NULL )
	, m_pBits( NULL )
{
   DWORD dwMaskSpace = lpbhi->biCompression == BI_BITFIELDS ? 3 * sizeof(DWORD) : 0;

   int nColors = lpbhi->biClrUsed;
   if (!nColors)
	   nColors = (lpbhi->biBitCount <= 8) ? ( 1 << lpbhi->biBitCount ) : 0;

   //DWORD dwColorSpace = nColors * sizeof(RGBQUAD);

   m_dwImageSize = lpbhi->biSizeImage;
   // Don't recalculate for RLE compressed images!
   if (lpbhi->biCompression == BI_RGB || lpbhi->biCompression == BI_BITFIELDS)
      m_dwImageSize = GetBytesMemNeeded(lpbhi->biWidth, lpbhi->biHeight, lpbhi->biBitCount);
	
   // Allocate the space, and copy the data, fill in the blanks...
   m_pBMI = DibCreate( lpbhi->biBitCount, lpbhi->biWidth, lpbhi->biHeight, lpbhi->biCompression);
   // Replace fields that may have us fooled, such as RLE compression
   m_pBMI->bmiHeader.biSizeImage = m_dwImageSize;
   m_pBMI->bmiHeader.biClrUsed = nColors;
   
   m_nWidth = lpbhi->biWidth;
   m_nHeight = lpbhi->biHeight;
   m_nBPP = lpbhi->biBitCount;

   // Locate the color table
   if (m_nBPP <= 8)
	   m_pClrTab = (RGBQUAD*)((BYTE*)m_pBMI->bmiColors + dwMaskSpace);
   // Locate the mask table
   if (m_pBMI->bmiHeader.biCompression == BI_BITFIELDS)
	   m_pClrMasks = (DWORD*)(m_pBMI->bmiColors);
   
   // Since the bytes needed are not necessarily predicted by the
   // image size, use what we found in the header
   m_pBits = new BYTE [ m_dwImageSize ];
}

CDib::~CDib()
{
	delete[] m_pBMI;
	delete[] m_pBits;
}


void CDib::GetLineArray( CLineArray &arr )
//
//	Sets the elements of the line array to point to the start of each line in the
//	DIB data.
//
//	This will NOT work for RLE encoded images.
{
	arr.SetSize( m_nHeight );

	const long bpl = GetBytesPerLine( m_nWidth, m_nBPP );
	for( int i= m_nHeight - 1; i >=  0; i--)
	{
		arr[i] = GetBits() + ( ((m_nHeight - 1) - i ) * bpl );
	}
}

void CDib::GetLineArray( CConstLineArray &arr ) const
//
//	Sets the elements of the line array to point to the start of each line in the
//	DIB data.
//
//	This will NOT work for RLE encoded images.
{
	arr.SetSize( m_nHeight );

	const long bpl = GetBytesPerLine( m_nWidth, m_nBPP );
	for( int i= m_nHeight - 1; i >=  0; i--)
	{
		arr[i] = GetBits() + ( ((m_nHeight - 1) - i ) * bpl );
	}
}



/////////////////////////////////////////////////////////////////////
// Static functions


bool CDib::Draw( HDC hdc, int x, int y ) const
{
	if( ::StretchDIBits(hdc,
              x,                        // Destination x
              y,                        // Destination y
              m_nWidth,	                // Destination width
              m_nHeight,                // Destination height
              0,                        // Source x
              0,                        // Source y
              m_nWidth,									// Source width
              m_nHeight,	  						// Source height
              GetBits(),								// Pointer to bits
              (BITMAPINFO *) m_pBMI,    // BITMAPINFO
              DIB_RGB_COLORS,           // Options
              SRCCOPY ) == GDI_ERROR )								// Raster operator code
		return false;
	return true;
}


bool CDib::Draw( HDC hdc, int left, int top, int right, int bottom ) const
{
	if( ::StretchDIBits(hdc,
              left,                     // Destination x
              top,                      // Destination y
              right - left,             // Destination width
              bottom - top,             // Destination height
              0,                        // Source x
              0,                        // Source y
              m_nWidth,									// Source width
              m_nHeight,	  						// Source height
              GetBits(),								// Pointer to bits
              (BITMAPINFO *) m_pBMI,    // BITMAPINFO
              DIB_RGB_COLORS,           // Options
              SRCCOPY ) == GDI_ERROR )								// Raster operator code
		return false;
	return true;
}


static const RGBQUAD rgbTransparent = { 0, 0, 0, 0 };

CDib *CDib::CreateDeltaDib( const CDib *pDibPrevious ) const
{
	//ASSERT( pDibPrevious->GetWidth() == GetWidth() );
	//ASSERT( pDibPrevious->GetHeight() == GetHeight() );
	CDib *pDibDiff = NULL;
	if( pDibPrevious->GetWidth() == GetWidth() && pDibPrevious->GetHeight() == GetHeight() )
	{

		pDibDiff = new CDib( &m_pBMI->bmiHeader );

		const RGBQUAD * pPrevious = (RGBQUAD * )pDibPrevious->GetBits();
		const RGBQUAD * pThis = (RGBQUAD * )GetBits();
		RGBQUAD * pNew = (RGBQUAD * )pDibDiff->GetBits();

		for( UINT u=0; u < GetDataSize() / sizeof( RGBQUAD ); u++ )
		{
			if( pPrevious->rgbBlue != pThis->rgbBlue
				|| pPrevious->rgbGreen != pThis->rgbGreen
				|| pPrevious->rgbRed != pThis->rgbRed
				)
			{
				*pNew = *pThis;
			}
			else
			{
				*pNew = rgbTransparent;
			}
			pNew++;
			pPrevious++;
			pThis++;
		}
	}
	
	return pDibDiff;
}

#ifdef IMGLIB_RIF

CDib *CDib::CreatFromDelta( const CDib *pDibDelta ) const
{
	ASSERT( pDibDelta->GetWidth() == GetWidth() );
	ASSERT( pDibDelta->GetHeight() == GetHeight() );

	CDib *pDibNew = new CDib( &m_pBMI->bmiHeader );

	const RGBQUAD * pDelta = (RGBQUAD * )pDibDelta->GetBits();
	const RGBQUAD * pThis = (RGBQUAD * )GetBits();
	RGBQUAD * pNew = (RGBQUAD * )pDibNew->GetBits();

	for( UINT u=0; u < GetDataSize() / sizeof( RGBQUAD ); u++ )
	{
		if( pDelta->rgbBlue != rgbTransparent.rgbBlue
				|| pDelta->rgbGreen != rgbTransparent.rgbGreen
				|| pDelta->rgbRed != rgbTransparent.rgbRed
				|| pDelta->rgbReserved != rgbTransparent.rgbReserved
		 )
		{
			*pNew = *pDelta;
		}
		else
		{
			*pNew = *pThis;
		}

		pNew++;
		pDelta++;
		pThis++;
	}
	return pDibNew;
}
#endif	//	IMGLIB_RIF


CDib *CDib::CreateCopy() const
{
	CDib *pDibNew = new CDib( &m_pBMI->bmiHeader );

	const RGBQUAD * pThis = (RGBQUAD * )GetBits();
	RGBQUAD * pNew = (RGBQUAD * )pDibNew->GetBits();

	const UINT nCount = GetDataSize() / sizeof( RGBQUAD );
	for( UINT u=0; u < nCount; u++ )
	{
		*pNew++ = *pThis++;
	}
	return pDibNew;
}


void CDib::AddImage( UINT nLeft, UINT nTop, const CDib *pDib )
{
	CLineArray arrThis;
	GetLineArray( arrThis );
	
	CConstLineArray arr;
	pDib->GetLineArray( arr );
	const UINT uDibHeight = pDib->GetHeight();
	const UINT uDibWidth = pDib->GetWidth();
	for( UINT uRow = 0; uRow < uDibHeight; uRow++ )
	{
		const RGBQUAD *pSourceLine = (RGBQUAD *)arr[ uRow ];
		RGBQUAD *pDestLine = (RGBQUAD *)arrThis[ uRow + nTop ];
		for( UINT uCol = 0; uCol < uDibWidth; uCol++ )
		{
			const RGBQUAD *pSource = pSourceLine + uCol;
			RGBQUAD *pDest = pDestLine + (uCol + nLeft);
			*pDest = *pSource;
		}
	}
}


HBITMAP CDib::GetBitmap() const
{
  return CreateDIBSection (NULL, (BITMAPINFO *)m_pBMI, DIB_RGB_COLORS, (void **)&m_pBits, 0, 0 );
}


#define BLUR( color, bytes )  \
		    (BYTE)((bPrev[nCol - bytes].color + bPrev[nCol].color + bPrev[nCol + bytes].color \
 		   + pbyte[nCol - bytes].color + pbyte[nCol].color + pbyte[nCol + bytes].color	\
		   + bNext[nCol - bytes].color + bNext[nCol].color + bNext[nCol + bytes].color) / 9)\

#ifdef IMGLIB_EXPERIMENTAL

bool CDib::Blur( UINT nTimes )
{

	//
	//	REVIEW - russf - There is a problem when blurring a mostly white image in that it gives some
	//										artifact sliding down the display.
	CLineArray arr;
	GetLineArray( arr );

	#define bytes 1

	while( nTimes )
	{
		for( UINT nRow = bytes; nRow < arr.GetSize() - bytes; nRow++ )
		{
			RGBQUAD * bPrev = (RGBQUAD *)arr[ nRow - 1];
			RGBQUAD * pbyte = (RGBQUAD *)arr[ nRow ];
			RGBQUAD * bNext = (RGBQUAD *)arr[ nRow + 1];
			for( UINT nCol = bytes; nCol < (UINT)GetWidth() - bytes ; nCol++ )
			{
				pbyte[nCol].rgbBlue = BLUR( rgbBlue, bytes );
				pbyte[nCol].rgbRed = BLUR( rgbRed, bytes );
				pbyte[nCol].rgbGreen = BLUR( rgbGreen, bytes );
			}
		}
		nTimes--;
	}
	return true;
}

#endif	//	IMGLIB_EXPERIMENTAL