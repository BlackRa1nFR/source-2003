/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	ImgLibInt.h
Owner:	russf@gipsysoft.com
Purpose:	Globally used stuff but internal to the library.
----------------------------------------------------------------------*/
#ifndef IMGLIBINT_H
#define IMGLIBINT_H

typedef Container::CArray< BYTE * > CLineArray;
typedef Container::CArray< const BYTE * > CConstLineArray;

class CDib
//
//	A single image
{
public:
	CDib( int nWidth, int nHeight, int nBPP );
	CDib( LPBITMAPINFOHEADER lpbhi );

	virtual ~CDib();

	//	Get the image sizes
	inline int GetWidth() const { return m_nWidth; }
	inline int GetHeight() const { return m_nHeight; }

	//	Draw the image onto a device
	bool Draw( HDC hdc, int x, int y ) const;
	bool Draw( HDC hdc, int left, int top, int right, int bottom ) const;

	//	Get the image data
	inline const BYTE *GetBits() const { return m_pBits; }
	inline BYTE *GetBits() { return m_pBits; }
	inline DWORD GetDataSize() const { return m_dwImageSize; }

	//	Get the line array for this image.
	void GetLineArray( CLineArray &arr );
	void GetLineArray( CConstLineArray &arr ) const;
	

	//	Access the color table (palette) for this image
	inline RGBQUAD *GetColorTable() { return m_pClrTab; }
	inline const RGBQUAD *GetColorTable() const { return m_pClrTab; }

	//	Access the mask table for this image
	inline DWORD *GetMaskTable() { return m_pClrMasks; }

	//	Access the amount of space allocated for the image data
	inline DWORD GetAllocatedImageSize() const { return m_dwImageSize; } 

	inline int GetColorsUsed() const { return m_pBMI->bmiHeader.biClrUsed; }
	HBITMAP GetBitmap() const;

	//	Create a copy of the current DIB
	CDib *CreateCopy() const;

	//	Combine the passed image with this one
	void AddImage( UINT nLeft, UINT nTop, const CDib *pDib );

	//	Create a difference image betwene this image and the pDibPrevious
	CDib *CreateDeltaDib( const CDib *pDibPrevious ) const;
	//	Create an image from the delat image created above;
	CDib *CreatFromDelta( const CDib *pDibDelta ) const;

	//
	//	Effects...
	bool Blur( UINT nTimes );

private:
	int m_nWidth, m_nHeight, m_nBPP;

	BITMAPINFO *m_pBMI;
	RGBQUAD *m_pClrTab;
	DWORD	*m_pClrMasks;
	BYTE *m_pBits;	
	DWORD m_dwImageSize;
private:
	CDib();
	CDib( const CDib & );
	CDib &operator = ( const CDib & );
};


class CFrame
//
//	A single frame
{
public:
	//	Gives ownership of the dib to the frame. Dib should be dynamically created because d'ctor will
	//	delete it.
	CFrame( CDib *pDib, int nTimeMilliseconds );

	virtual ~CFrame();

	//	Get at the actual DIB for the frame
	const CDib *GetDib() const { return m_pDib; }
	CDib *GetDib() { return m_pDib; }

	//	Get the time this frame should be displayed for. 0 == infinite
	int GetTime() const { return m_nTimeMilliseconds; }

private:
	CDib *m_pDib;
	int m_nTimeMilliseconds;

private:
	CFrame();
	CFrame( const CFrame &);
	CFrame &operator =( const CFrame &);
};

#endif //IMGLIBINT_H