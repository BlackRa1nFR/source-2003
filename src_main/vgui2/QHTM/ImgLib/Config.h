/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	Config.h
Owner:	russf@gipsysoft.com
Purpose:	Decide which image formats you want.
----------------------------------------------------------------------*/
#ifndef CONFIG_H
#define CONFIG_H

	//
	//	This is here for any experimental stuff I do, comment it out to "play"
	//#define IMGLIB_EXPERIMENTAL

	//
	//	The following preprocessor symbols allow for each format to be included
	//	seperately
	//	IMGLIB_MNG			--	Include MNG, it will also want to link to libpng
	//	IMGLIB_PNG			--	Include PNG
	//	IMGLIB_JPG			--	Include JPG
	//	IMGLIB_PCX			--	Include PCX
	//	IMGLIB_BMP			--	Include BMP

	#ifdef BUILD_LIGHT

		//
		//	In the light version we only allow BMP files.
		#define IMGLIB_BMP

	#else	//	BUILD_LIGHT

		//
		//	You can remove support for image formats just by commenting out the #define's below.

		#define IMGLIB_MNG
		#define IMGLIB_PNG
		//	Not that you need a licence to use GIF
		#define IMGLIB_GIF
		#define IMGLIB_JPG
		#define IMGLIB_PCX
		#define IMGLIB_BMP

		//	My own animated format, produces files smaller than MNG and a little larger than GIF.
		//#define IMGLIB_RIF
	#endif	//	BUILD_LIGHT

#endif //CONFIG_H