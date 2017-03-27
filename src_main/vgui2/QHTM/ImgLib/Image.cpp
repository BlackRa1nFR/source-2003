/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	Image.cpp
Owner:	russf@gipsysoft.com
Purpose:	An image, can consist of many frames.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "FileDataSource.h"
#include "ResourceDataSource.h"
#include <Array.h>
#include <ImgLib.h>
#include "ImgLibInt.h"
#include "Config.h"
#include "RIFFormat.h"
#include <ZLib\ZLib.h>

#ifdef IMGLIB_MNG
	extern bool DecodeMNG( CDataSourceABC &ds, CFrameArray &arrFrames, SIZE &size );
	#ifdef _DEBUG
		#pragma comment( lib, "lpng103D.lib" )
	#else
		#pragma comment( lib, "lpng103.lib" )
	#endif	//	_DEBUG
#endif	//	IMGLIB_MNG

#ifdef IMGLIB_PNG
	extern bool DecodePNG( CDataSourceABC &ds, CFrameArray &arrFrames, SIZE &size );
	#ifdef _DEBUG
		#pragma comment( lib, "lpng103D.lib" )
	#else
		#pragma comment( lib, "lpng103.lib" )
	#endif	//	_DEBUG
#endif	//	IMGLIB_PNG

#ifdef IMGLIB_JPG
	extern bool DecodeJPEG( CDataSourceABC &ds, CFrameArray &arrFrames, SIZE &size );
	#ifdef _DEBUG
		#pragma comment( lib, "jpeglibD.lib" )
	#else
		#pragma comment( lib, "jpeglib.lib" )
	#endif	//	_DEBUG
#endif	//	IMGLIB_JPG

#ifdef IMGLIB_GIF
	extern bool DecodeGIF( CDataSourceABC &ds, CFrameArray &arrFrames, SIZE &size );
#endif	//	IMGLIB_GIF

#ifdef IMGLIB_RIF
	extern bool DecodeRIF( CDataSourceABC &ds, CFrameArray &arrFrames, SIZE &size );
#endif	//	IMGLIB_RIF

#ifdef IMGLIB_PCX
	extern bool DecodePCX( CDataSourceABC &ds, CFrameArray &arrFrames, SIZE &size );
#endif	//	IMGLIB_PCX

#ifdef IMGLIB_BMP
	extern bool DecodeBMP( CDataSourceABC &ds, CFrameArray &arrFrames, SIZE &size );
#endif	//	IMGLIB_PCX

#ifdef TRACING
	#define ITRACE TRACE
#else	//	TRACING
	#define ITRACE 
#endif	//	TRACING

class CFrameData
{
public:
	CFrameData()
		: m_nRefCount( 0 )
		{
			m_size.cx = m_size.cy = 0;
		}

	inline const CDib *GetFrameImage( int nFrame ) const
	{
		return m_arrFrames[ nFrame ]->GetDib();
	}

	inline CDib *GetFrameImage( int nFrame )
	{
		return m_arrFrames[ nFrame ]->GetDib();
	}

	void CleanupFrames()
		{
			m_size.cx = m_size.cy = 0;

			for( UINT u = 0; u < m_arrFrames.GetSize(); u++ )
			{
				delete m_arrFrames[ u ];
			}
			m_arrFrames.RemoveAll();
		}

	int GetRefCount() { return m_nRefCount; }

	void AddRef() {	m_nRefCount++; }

	void Release()
		{
			m_nRefCount--;
			if( !m_nRefCount )
				delete this;
		}

	SIZE m_size;
	CFrameArray	m_arrFrames;
private:
	~CFrameData()
		{
			CleanupFrames();
		}

	int m_nRefCount;
};


CImage::CImage( CFrameData *pFrameData )
	: m_pFrameData( pFrameData )
{
	m_pFrameData->AddRef();
}


CImage::CImage()
	: m_pFrameData( new CFrameData )
{
	m_pFrameData->AddRef();
}


CImage::~CImage()
{
	m_pFrameData->Release();
}


void CImage::UnloadFrames()
{
	if( m_pFrameData->GetRefCount() == 1 )
	{
		m_pFrameData->CleanupFrames();
	}
	else
	{
		m_pFrameData->Release();
		m_pFrameData = new CFrameData;
		m_pFrameData->AddRef();
	}
}


bool CImage::Load( HINSTANCE hInst, LPCTSTR pcszName )
{
	CResourceDataSource ds;
	if( ds.Open( hInst, pcszName, RT_RCDATA ) )
	{
		return Load( ds );
	}
	else if (ds.Open( hInst, pcszName, RT_BITMAP ))
	{
		ds.Reset();
		return DecodeBMP( ds, m_pFrameData->m_arrFrames, m_pFrameData->m_size );
	}
	else
	{
		ITRACE(_T("Resource not found\n") );
	}
	return false;
}


bool CImage::Load( HINSTANCE hInst, LPCTSTR pcszName, LPCTSTR pcszResourceType )
{
	CResourceDataSource ds;
	if( ds.Open( hInst, pcszName, pcszResourceType ) )
	{
		return Load( ds );
	}
	else
	{
		ITRACE(_T("Resource not found\n") );
	}
	return false;
}


bool CImage::CreateFromBitmap( HBITMAP hbmp )
{
	UnloadFrames();

	BITMAP bm;
	VAPI( ::GetObject( hbmp, sizeof( bm ), &bm ) );
	CDib *pDib = new CDib( bm.bmWidth, bm.bmHeight, 32 );
	m_pFrameData->m_size.cx = bm.bmWidth;
	m_pFrameData->m_size.cy = bm.bmHeight;

	HBITMAP bmp = pDib->GetBitmap();
	VAPI( bmp );
	HDC hdcScreen = GetDC( NULL );
	HDC hdcSrc = CreateCompatibleDC( hdcScreen );
	HDC hdcDest = CreateCompatibleDC( hdcScreen );
	VAPI( hdcSrc );
	VAPI( hdcDest );
	VAPI( ::ReleaseDC( NULL, hdcScreen ) );

	hbmp = (HBITMAP)SelectObject( hdcSrc, hbmp );
	bmp = (HBITMAP)SelectObject( hdcDest, bmp );
	VAPI( ::BitBlt( hdcDest, 0, 0, bm.bmWidth, bm.bmHeight, hdcSrc, 0, 0, SRCCOPY ) );

//	SelectObject( hdcDest, CreateSolidBrush( RGB( 255, 128, 64 ) ) );
//	Rectangle( hdcDest, 0, 0, 100, 100 );

	hbmp = (HBITMAP)SelectObject( hdcSrc, hbmp );
	bmp = (HBITMAP)SelectObject( hdcDest, bmp );
	GdiFlush();
	VAPI( ::DeleteDC( hdcDest ) );
	VAPI( ::DeleteDC( hdcSrc ) );
	CFrame *pFrame = new CFrame( pDib, 0 );
	m_pFrameData->m_arrFrames.Add( pFrame );
	return true;
}


bool CImage::Load( CDataSourceABC &ds )
{
	UnloadFrames();
	//
	//	Read the first 8 bytes of the image file to see if we can determine what image format
	//	we have
	BYTE bMagicNumber[ 8 ];
	if( ds.ReadBytes( bMagicNumber, 8 ) )
	{
#ifdef IMGLIB_MNG
    if( !memcmp( bMagicNumber, "\212MNG\r\n\032\n", 8 ) )
		{
			return DecodeMNG( ds, m_pFrameData->m_arrFrames, m_pFrameData->m_size );
		}
		else 
#endif	//	IMGLIB_MNG

#ifdef IMGLIB_PNG
		if( !memcmp( bMagicNumber, "\211PNG\r\n\032\n", 8 ) )
		{
			return DecodePNG( ds, m_pFrameData->m_arrFrames, m_pFrameData->m_size );
		}
		else 
#endif	//	IMGLIB_PNG

#ifdef IMGLIB_GIF
		if( !memcmp( bMagicNumber, "GIF89a", 6 ) || !memcmp( bMagicNumber, "GIF87a", 6 ) )
		{
			ds.Reset();
			return DecodeGIF( ds, m_pFrameData->m_arrFrames, m_pFrameData->m_size );
		}
		else 
#endif	//	IMGLIB_GIF

#ifdef IMGLIB_RIF
		if( !memcmp( bMagicNumber, RIF_SIGNATURE, sizeof( RIF_SIGNATURE ) - 1 ) )
		{
			ds.Reset();
			return DecodeRIF( ds, m_pFrameData->m_arrFrames, m_pFrameData->m_size );
		}
		else 
#endif	//	IMGLIB_JPG

#ifdef IMGLIB_JPG
		if( !memcmp( bMagicNumber, "\xFF\xD8", 2 ) )
		{
			ds.Reset();
			return DecodeJPEG( ds, m_pFrameData->m_arrFrames, m_pFrameData->m_size );
		}
		else 
#endif	//	IMGLIB_JPG

#ifdef IMGLIB_BMP
		if( !memcmp( bMagicNumber, "BM", 2 ) )
		{
			ds.Reset();
			return DecodeBMP( ds, m_pFrameData->m_arrFrames, m_pFrameData->m_size );
		}
		else
#endif	//	IMGLIB_PCX

#ifdef IMGLIB_PCX
		if( !memcmp( bMagicNumber, "\x0A", 1 ) )
		{
			ds.Reset();
			return DecodePCX( ds, m_pFrameData->m_arrFrames, m_pFrameData->m_size );
		}
		else
#endif	//	IMGLIB_PCX

		{
			ITRACE(_T("Unknown format\n"));
		}
	}
	else
	{
		ITRACE(_T("Could not read enough signature\n") );
	}
	return false;
}


bool CImage::Load( LPCTSTR pcszFilename )
{
	ASSERT_VALID_STR( pcszFilename );

	CFileDataSource file;
	if( file.Open( pcszFilename ) )
	{
		return Load( file );
	}
	else
	{
		ITRACE(_T("File not found %s\n"), pcszFilename );
	}
	return false;
}


bool CImage::DrawFrame( UINT nFrame, HDC hdc, int left, int top, int right, int bottom ) const
{
	if( GetFrameCount() && nFrame <= GetFrameCount() )
	{
		return m_pFrameData->GetFrameImage( nFrame )->Draw( hdc, left, top, right, bottom  );
	}
	return false;
}


bool CImage::DrawFrame( UINT nFrame, HDC hdc, int x, int y ) const
{
	if( GetFrameCount() && nFrame <= GetFrameCount() )
	{
		return m_pFrameData->GetFrameImage( nFrame )->Draw( hdc, x, y );
	}
	return false;
}


CImage * CImage::CreateCopy() const
{
	CImage *pImage = new CImage( m_pFrameData );
	return pImage;
}


UINT CImage::GetFrameCount() const
{
	return m_pFrameData->m_arrFrames.GetSize();
}

int CImage::GetFrameTime( UINT nFrame ) const
//
//	Get the number of milliseconds a frame should be displayed for.
{
	return m_pFrameData->m_arrFrames[ nFrame ]->GetTime();
}


//	Image sizes
int CImage::GetWidth() const
{
	return m_pFrameData->m_size.cx;
}


int CImage::GetHeight() const
{
	return m_pFrameData->m_size.cx;
}


const SIZE & CImage::GetSize() const
{
	return m_pFrameData->m_size;
}

bool CImage::SaveAsRIF( LPCTSTR pcszFilename )
//
//	Save file as proprietry format
{

#ifdef IMGLIB_RIF
	//
	//	Signature
	//	Version
	//	size
	//	nFrames
	//	Followed by nFrames, everything after this is compressed.

	//	Frame n
	//		FrameDisplayTimeMilliseconds
	//		HasPalette
	//		palette
	//		Frame data
	Container::CArray<unsigned char> arrBytes;

	RIFHeader header;
	memcpy( header.bSig, RIF_SIGNATURE, sizeof( RIF_SIGNATURE ) - 1 );
	memcpy( header.bVersion, RIF_VERSION, sizeof( RIF_VERSION ) - 1 );
	header.nFrameCount = (short)GetFrameCount();
	header.nImageWidth = (short)m_pFrameData->m_size.cx;
	header.nImageHeight = (short)m_pFrameData->m_size.cy;

	CDib *pDibPrevious = NULL, *pDelta = NULL;
	for( UINT u = 0; u < GetFrameCount(); u++ )
	{
		CDib *pDib = m_pFrameData->GetFrameImage( u );
		if( pDibPrevious )
		{
			pDelta = pDib->CreateDeltaDib( pDibPrevious );
			pDibPrevious = pDib;
			pDib = pDelta;
		}
		else
		{
			pDibPrevious = pDib;
		}

		RIFFrameHeader frameheader;
		frameheader.nPaletteColors = (short)pDib->GetColorsUsed();
		frameheader.bHasPalette = frameheader.nPaletteColors ? true : false;
		frameheader.nDisplayTimeMilliSeconds = (short)GetFrameTime( u );

		Container::CArray<RGBTRIPLE> arrColors;
		if( frameheader.bHasPalette )
		{
			const RGBQUAD *pQuad = pDib->GetColorTable();
			for( int nColor = 0; nColor < frameheader.nPaletteColors; nColor++ )
			{
				RGBTRIPLE trp;
				trp.rgbtRed = pQuad[ nColor ].rgbRed;
				trp.rgbtGreen = pQuad[ nColor ].rgbGreen;
				trp.rgbtBlue = pQuad[ nColor ].rgbBlue;
				arrColors.Add( trp );
			}
		}

		int nDataSize = pDib->GetDataSize();
		frameheader.lPaletteBytes = arrColors.GetSize();
		frameheader.lFrameBytes = nDataSize;

		arrBytes.InsertAt( arrBytes.GetSize(), (unsigned char*)&frameheader, sizeof( frameheader ) );
		if( frameheader.bHasPalette )
			arrBytes.InsertAt( arrBytes.GetSize(), (unsigned char*)arrColors.GetData(), arrColors.GetSize() * sizeof( RGBTRIPLE ) );
		arrBytes.InsertAt( arrBytes.GetSize(), (unsigned char*)pDib->GetBits(), nDataSize );
		if( pDelta )
			delete pDelta;
	}


	FILE *pFile = _tfopen( pcszFilename, _T("wb") );
	if( pFile )
	{
		header.lDataBlockSize = arrBytes.GetSize();
		uLongf lDestlen = arrBytes.GetSize() + arrBytes.GetSize() / 1000 + 128;

		Container::CArray<unsigned char> arrBytesDest( lDestlen );
		compress2( arrBytesDest.GetData(), &lDestlen, arrBytes.GetData(), arrBytes.GetSize(), 9 );
		header.lCompressedLength = lDestlen;

		fwrite( (const voidp)&header, sizeof( header ), 1, pFile );
		fwrite( (const voidp)arrBytesDest.GetData(), lDestlen, 1, pFile );
		fclose( pFile );
	}

	return true;
#else	//	IMGLIB_RIF
	UNREFERENCED_PARAMETER( pcszFilename );
	return false;
#endif	//	IMGLIB_RIF
}

bool CImage::Blur( UINT nTimes )
{
#ifdef IMGLIB_EXPERIMENTAL

	for( UINT u = 0; u < m_pFrameData->m_arrFrames.GetSize(); u++ )
	{
		CDib *pDib = m_pFrameData->GetFrameImage( u );
		if( !pDib->Blur( nTimes ) )
		{
			return false;
		}
	}
	return true;

#else	//	IMGLIB_EXPERIMENTAL
	UNREFERENCED_PARAMETER( nTimes );
	return false;
#endif	//	IMGLIB_EXPERIMENTAL
}