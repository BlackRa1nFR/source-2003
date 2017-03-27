/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	DecodeMNG.cpp
Owner:	russf@gipsysoft.com
Purpose:	Decode an MNG file into a frame array.
					The only exported function is:
					bool DecodePNG( CDataSourceABC &ds, CFrameArray &arrFrames, int &nWidth, int &nHeight )

----------------------------------------------------------------------*/
#include "stdafx.h"
#include "DataSourceABC.h"
#include <lpng103\png.h>
#include <ImgLib.h>
#include "ImgLibInt.h"
#include "Config.h"

#ifdef TRACING
	#define ITRACE TRACE
#else	//	TRACING
	#define ITRACE 
#endif	//	TRACING

#ifdef IMGLIB_PNG

static void my_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
  CDataSourceABC* pSourceInfo=(CDataSourceABC*)png_get_io_ptr(png_ptr);
	pSourceInfo->ReadBytes( data, length );
}



static void user_error_fn( png_structp /*png_ptr*/, png_const_charp error_msg )
{
	TRACE( _T("%s\n"), error_msg);
  throw 1;
}

static void user_warning_fn( png_structp /*png_ptr*/, png_const_charp warning_msg )
{
  TRACE( _T("%s\n"), warning_msg);
}


bool DecodePNG( CDataSourceABC &ds, CFrameArray &arrFrames, SIZE &size )
{
	CDib * pDib = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	try
	{
		png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING,(void *) NULL, user_error_fn, user_warning_fn );
		if( !png_ptr )
		{
			ITRACE(_T("Failed to allocate PNG struct\n"));
			return NULL;
		}

		//
		//	Prevents libpng from attempting to read the signature (fools it into thinking it has already).
		png_set_sig_bytes( png_ptr, 8 );

		info_ptr = png_create_info_struct( png_ptr );
		png_set_read_fn( png_ptr, (void*)&ds, my_read_data );
		png_read_info( png_ptr, info_ptr );

		png_uint_32 width, height;
		int bit_depth, color_type, interlace_type;
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
		size.cx = width;
		size.cy = height;
    if( color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA )
			png_set_bgr( png_ptr );

    if (bit_depth == 16)
      png_set_strip_16(png_ptr);

    if (bit_depth < 8)
      png_set_packing(png_ptr);

		int nDestBPP = 32;
    if( bit_depth <= 8 )
      nDestBPP = 8;

    png_read_update_info( png_ptr, info_ptr );

		pDib = new CDib( width, height, info_ptr->pixel_depth );

		RGBQUAD *pct = pDib->GetColorTable();
    if( color_type == PNG_COLOR_TYPE_GRAY  && nDestBPP != 32 )
    {
      int i;
      int NumColors = 1<<(bit_depth);
      for (i=0; i<NumColors; i++)
      {
        BYTE CurColor = static_cast<BYTE>( (i*255)/(NumColors-1) );
				pct->rgbReserved = pct->rgbRed = pct->rgbGreen = pct->rgbBlue = CurColor;
				pct++;
      }
    }

    if (color_type == PNG_COLOR_TYPE_PALETTE && nDestBPP != 32)
    {
      png_color* ppng_color_tab=NULL;

      int   i;
      int   nbColor=0;

      png_get_PLTE(png_ptr,info_ptr,&ppng_color_tab,&nbColor);

      for (i=0; i<nbColor; i++)
      {
				pct->rgbReserved = 0xFF;
				pct->rgbRed = (*(ppng_color_tab+i)).red;
				pct->rgbGreen = (*(ppng_color_tab+i)).green;
				pct->rgbBlue = (*(ppng_color_tab+i)).blue;
				pct++;
      }
    }


		Container::CArray< BYTE * > arrLines;

		pDib->GetLineArray( arrLines );
			
    png_read_image(png_ptr, arrLines.GetData() );
    png_read_end(png_ptr, info_ptr);

		if( pDib )
		{
			CFrame *pFrame = new CFrame( pDib, 0 );
			arrFrames.Add( pFrame );
		}
	}
	catch( ... )
	{
		if( pDib )
		{
			delete pDib;
			pDib = NULL;
		}
	}
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

	if( pDib )
	{
		return true;
	}
	return false;
}

#endif	//	IMGLIB_PNG
