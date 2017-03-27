/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	ImgLib.h
Owner:	russf@gipsysoft.com
Purpose:	Imaging library.
----------------------------------------------------------------------*/
#ifndef IMGLIB_H
#define IMGLIB_H

#ifndef _WINDOWS_
	#include <Windows.h>
#endif	//	_WINDOWS_

#ifdef __cplusplus

	//
	//	Forward declares
	class CDataSourceABC;
	class CDib;

	//
	//	Types

	class CImage
	//
	//	Main image class.
	{
	public:
		CImage();
		virtual ~CImage();

		//	Load from file
		bool Load( LPCTSTR pcszFilename );

		//	Load from resources
		bool Load( HINSTANCE hInst, LPCTSTR pcszName );
		bool Load( HINSTANCE hInst, LPCTSTR pcszName, LPCTSTR pcszResourceType );
		inline bool Load( HINSTANCE hInst, UINT uID ) { return Load( hInst, MAKEINTRESOURCE( uID ) ); }

		//	Load from custom data source
		bool Load( CDataSourceABC &ds );

		//	Unload all frame data
		void UnloadFrames();

		//	Draw a particular frame to a device
		bool DrawFrame( UINT nFrame, HDC hdc, int left, int top ) const;
		bool DrawFrame( UINT nFrame, HDC hdc, int left, int top, int right, int bottom ) const;

		//	Get the number of frames available.
		UINT GetFrameCount() const;

		//	Get the number of milliseconds a frame shoudl be displayed for.
		int GetFrameTime( UINT nFrame ) const;

		//	Image sizes
		int GetWidth() const;
		int GetHeight() const;
		const SIZE &GetSize() const;

		//	Create a copy of the image
		CImage *CreateCopy() const;

		//	Save file as proprietry format
		bool SaveAsRIF( LPCTSTR pcszFilename );

		//	Create this object from a standard windows bitmap
		bool CreateFromBitmap( HBITMAP hbmp );

		//
		//	Effects for the image
		
		//
		//	Simple blur effect
		bool Blur( UINT nTimes );

	protected:
		CImage( class CFrameData *m_pFrameData );

		class CFrameData *m_pFrameData;

	private:
		CImage( const CImage &);
		CImage &operator = ( const CImage &);
	};
#else	//	__cplusplus
	#error C++ compiler required.
#endif	//	__cplusplus

#endif //	IMGLIB_H