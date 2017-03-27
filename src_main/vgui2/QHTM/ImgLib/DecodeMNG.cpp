/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	DecodeMNG.cpp
Owner:	russf@gipsysoft.com
Purpose:	Decode an MNG file into a frame array.
					The only exported function is:
					bool DecodeMNG( CDataSourceABC &ds, CFrameArray &arrFrames, int &nWidth, int &nHeight )

----------------------------------------------------------------------*/
#include "stdafx.h"
#include "DataSourceABC.h"
#include <lpng103\png.h>
#include <ImgLib.h>
#include "ImgLibInt.h"
#include "Config.h"

#ifdef TRACING
	#define ITRACE TRACE
	#define ITRACEA TRACEA
#else	//	TRACING
	#define ITRACE
	#define ITRACEA
#endif	//	TRACING

#ifdef IMGLIB_MNG

static inline long GetStreamLong( BYTE *pb )
{
	return ( pb[0] << 24)+(pb[1] << 16)+(pb[2] << 8 ) + pb[3];
}

static inline short GetStreamShort( BYTE *pb )
{
	return static_cast<short>( (pb[0] << 8 ) + pb[1] );
}

class CChunkData
{
public:
	CChunkData( UINT uSize ) : m_pb( new BYTE[ uSize ] ) {}
	~CChunkData() { delete[] m_pb; }

	operator BYTE *() { return m_pb; }

private:
	BYTE *m_pb;
};



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


static CDib * ReadPNG( CDataSourceABC &ds )
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
      png_colorp ppng_color_tab=NULL;

      int   i;
      int   nbColor=0;

      png_get_PLTE(png_ptr,info_ptr,&ppng_color_tab,&nbColor);

      for (i=0; i<nbColor; i++)
      {
				pct->rgbReserved = 0xFF;
				pct->rgbRed = ppng_color_tab->red;
				pct->rgbGreen = ppng_color_tab->green;
				pct->rgbBlue = ppng_color_tab->blue;
				pct++;
				ppng_color_tab++;
      }
    }


		Container::CArray< BYTE * > arrLines;

		pDib->GetLineArray( arrLines );
			
    png_read_image(png_ptr, arrLines.GetData() );
    png_read_end(png_ptr, info_ptr);
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
	return pDib;
}


bool DecodeMNG( CDataSourceABC &ds, CFrameArray &arrFrames, SIZE &size )
{
  int delay=100;
  int ticks_per_second=1000;

	//
	//	According to the spec. each chunk has the following format:
	//	Length, name, data, CRC
	do
	{
		long lChunkLength;
		if( !ds.MSBReadLong( lChunkLength ) )
		{
			ITRACE(_T("Failed to read chunk length\n"));
			return false;
		}

		BYTE ChunkType[ 4 ];
		if( !ds.ReadBytes( ChunkType, 4 ) )
		{
			ITRACE(_T("Failed to read chunk type\n"));
			return false;
		}

    if( memcmp( ChunkType, "MEND", 4 ) == 0)
      break;

		CChunkData pChunkData( lChunkLength );
		if( !pChunkData )
		{
			ITRACE(_T("Failed to allocate memory for the chunk\n"));
			return false;
		}

		if( lChunkLength && !ds.ReadBytes( pChunkData, lChunkLength ) )
		{
			ITRACE(_T("Failed to read chunk data\n"));
			return false;
		}

		//
		//	We don't care about the CRC
		long lCRC;
		ds.MSBReadLong( lCRC );

		//
		//	Get here and we have our data chunks.
		if( memcmp( ChunkType, "MHDR", 4 ) == 0)
		{
			size.cx = GetStreamLong( pChunkData );
			size.cy = GetStreamLong( pChunkData + 4);
			ticks_per_second = GetStreamLong( pChunkData + 8 );
//	Might be useful later...
/*
			long layer_count = GetStreamLong( pChunkData + 12 );
			long frame_count = GetStreamLong( pChunkData + 16 );
			long play_time = GetStreamLong( pChunkData + 20 );
			long profile = GetStreamLong( pChunkData + 24 );
			continue;
*/
		}
		else if( memcmp( ChunkType, "FRAM", 4 ) == 0)
		{
			if( lChunkLength > 6 )
			{
				BYTE *p = pChunkData;
        p++; /* framing mode */
        while (*p && ((p-pChunkData) < lChunkLength) )
				{
          p++;
				}
        if ((p-pChunkData) < (lChunkLength-4))
        {
          p+=5;
          if (*(p-4))
					{
            delay = GetStreamLong( p ) * 1000 / ticks_per_second;
					}
        }
			}
			continue;
		}
		else if( memcmp( ChunkType, "BACK", 4 ) == 0)
		{
			#ifdef _DEBUG
			//
			//	The spec. I used said that teh background colors are red, green, blue but the file written
			//	by PSP is the other way around as per below.
			int nBlue = GetStreamShort( pChunkData );
			int nGreen = GetStreamShort( pChunkData + 2);
			int nRed = GetStreamShort( pChunkData + 4);
			ITRACE(_T("Background color is RGB( %d, %d, %d )\n"), nRed, nGreen, nBlue );
			#endif	//	_DEBUG
		}
		else if( memcmp( ChunkType, "IHDR", 4 ) == 0)
		{
			//
			//	Read an actual PNG image from the stream. 

			//
			//	Move back so that the PNG library can read it's header.
			ds.SetRelativePos( -((int) lChunkLength + 12) );

			CDib *pDib = ReadPNG( ds );
			if( pDib )
			{
				if( !delay )	delay = 1000;
				CFrame *pFrame = new CFrame( pDib, delay );
				arrFrames.Add( pFrame );
			}
			else
				return false;
		}
		else
		{
			#ifdef _DEBUG
				TCHAR szChunkBuf[ 5 ];
				memcpy( szChunkBuf, ChunkType, 4 );
				szChunkBuf[ 4 ] = '\000';
				ITRACEA( "Unknown and unsupported MNG chunk \"%s\"\n", szChunkBuf );
			#endif	//	_DEBUG
		}

	} while( 1 );
	return true;
}

#endif	//	IMGLIB_MNG
