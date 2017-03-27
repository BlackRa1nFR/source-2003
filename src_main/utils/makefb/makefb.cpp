/*
	makefb.cpp
	
	Merges a fullbright and non-fullbright image to produce a composite image with
	an appropriately arranged palette
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//-------------------------------------------------------------- Constants

#define MAX_NON_FULL_BRIGHT 224
#define FIRST_FULL_BRIGHT 224
#define MAX_FULL_BRIGHT 32

//-------------------------------------------------------------- Globals

unsigned char *pbFBits, *pbNBits;
RGBQUAD *ppalF, *ppalN;
int remapF[ MAX_FULL_BRIGHT ], remapN[ MAX_NON_FULL_BRIGHT ];
int remapFIndex = -1, remapNIndex = -1;


//-------------------------------------------------------------- Functions

void ReportError(const char *pszError)
{
	printf("\n>>>>>>>>>>>>> ERROR <<<<<<<<<<<<\n");
	printf("%s",pszError);
	Beep(800,500);
	exit(EXIT_FAILURE);
}

void ReportWarning(const char *pszError)
{
	printf("\nWARNING:\t");
	printf("%s",pszError);
}

void PrintUsage(char *pname)
{
	printf("\n\tusage:%s <fullbright bmp> \n\n",pname);
	printf("\t%s.exe combine a full bright file and its\n",pname);
	printf("\tnon-full bright file into a stand alone bmp that\n");
	printf("\tcontains both full and non-full bright colors.\n\n");
	printf("\tIt is assumed that the full bright file name is prefixed\n");
	printf("\t\"F_\" and that the non-full bright file is prefixed with\n");
	printf("\t\"N_\". The generated file name will have the same file name\n");
	printf("\twithout the prefix (i.e.?_<outputname>).\n\n");
	printf("\tSee Mike if you have any questions.\n\n");
}

int LoadBMP(HANDLE hFile, BYTE** ppbBits, RGBQUAD** ppbPalette,	
			 BITMAPFILEHEADER *pbmfh, BITMAPINFOHEADER *pbmih)
{
	int rc = 0;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	ULONG cbBmpBits;
	BYTE* pbBmpBits;
	RGBQUAD *pbPal = NULL;
	BOOL fSuccess;
	DWORD dwActual;

	if (pbmfh == NULL)
		pbmfh = &bmfh;
	if (pbmih == NULL)
		pbmih = &bmih;

	// Bogus parameter check
	if (!(ppbPalette != NULL && ppbBits != NULL))
		ReportError("Internal Error.\n");
			

	// Read file header
	fSuccess = ReadFile(hFile, pbmfh, sizeof(*pbmfh), &dwActual, NULL);
	if (!fSuccess || (dwActual != sizeof(*pbmfh)))
		ReportError("Read Failure.\n");

	// Bogus file header check
	if (!(pbmfh->bfReserved1 == 0 && pbmfh->bfReserved2 == 0))
		ReportError("Invalid Bitmap File Header\n");

	// Read info header
	fSuccess = ReadFile(hFile, pbmih, sizeof(*pbmih), &dwActual, NULL);
	if (!fSuccess || (dwActual != sizeof(*pbmih)))
		ReportError("Read Failure.\n");

	// Bogus info header check
	if (!(pbmih->biSize == sizeof(*pbmih) && pbmih->biPlanes == 1))
		ReportError("Invalid Bitmap File Header -- biPlanes != 1\n");

	// Bogus bit depth?  Only 8-bit supported.
	if (pbmih->biBitCount != 8)
		ReportError("Invalid Bitmap File Header -- Not 8Bpp\n");
	
	// Bogus compression?  Only non-compressed supported.
	if (pbmih->biCompression != BI_RGB)
		ReportError("Invalid Bitmap File Header -- Not RGB\n");

	// Alloc room for palette
	pbPal = (RGBQUAD *)malloc(sizeof(RGBQUAD) * 256);
	if (pbPal == NULL)
		ReportError("Allocation Failure.\n");

	// Read palette (256 entries)
	fSuccess = ReadFile(hFile, pbPal, sizeof(RGBQUAD) * 256, &dwActual, NULL);
	if (!fSuccess || (dwActual != (sizeof(RGBQUAD) * 256)))
		ReportError("Read Failure.\n");

	// Read bitmap bits (remainder of file)
	cbBmpBits = pbmfh->bfSize - (sizeof(RGBQUAD) * 256) - sizeof(*pbmfh) 
			- sizeof(*pbmih);

#if 0
	int i;
	byte *pb = (unsigned char *)malloc(cbBmpBits);

	fSuccess = ReadFile(hFile, pb, cbBmpBits, &dwActual, NULL);
	if (!fSuccess || (dwActual != cbBmpBits))
		ReportError("Read Failure.\n");

	pbBmpBits = (unsigned char *)malloc(cbBmpBits);
	if (pbBmpBits == NULL)
		ReportError("Allocation Failure.\n");

	pb += (pbmih->biHeight - 1) * pbmih->biWidth;
	for(i = 0; i < pbmih->biHeight; i++)
	{
		memcpy(&pbBmpBits[pbmih->biWidth * i], pb, pbmih->biWidth);
		pb -= pbmih->biWidth;
	}

	pb += pbmih->biWidth;
	free(pb);
#else
	pbBmpBits = (unsigned char *)malloc(cbBmpBits);
	if (pbBmpBits == NULL)
		ReportError("Allocation Failure.\n");

	fSuccess = ReadFile(hFile, pbBmpBits, cbBmpBits, &dwActual, NULL);
	if (!fSuccess || (dwActual != cbBmpBits))
		ReportError("Read Failure.\n");
#endif

	// Set output parameters
	*ppbPalette = pbPal;
	*ppbBits = pbBmpBits;

	return rc;
}


BOOL CmpRGB(RGBQUAD rgb1, RGBQUAD rgb2)
{
	if ((rgb1.rgbRed != rgb2.rgbRed) || 
			(rgb1.rgbGreen != rgb2.rgbGreen) || 
			(rgb1.rgbBlue != rgb2.rgbBlue))
		return FALSE;
	return TRUE;
}


void CreateFBRemap()
{
	int i;

	// Initialize
	remapFIndex = -1;
	memset( remapF, 0xff, MAX_FULL_BRIGHT * sizeof(int) );

	int remap = FIRST_FULL_BRIGHT;

	for( i=0; i<MAX_FULL_BRIGHT; i++)
	{
		int iMagic = -1;

		if ((ppalF[i].rgbRed == 0) && (ppalF[i].rgbGreen == 0) && (ppalF[i].rgbBlue == 255))
			iMagic = i;
		else if ((ppalF[i].rgbRed == 0) && (ppalF[i].rgbGreen == 255) && (ppalF[i].rgbBlue == 0))
			iMagic = i;
		else if ((ppalF[i].rgbRed == 255) && (ppalF[i].rgbGreen == 0) && (ppalF[i].rgbBlue == 0))
			iMagic = i;

		if( iMagic != -1 )
		{
			// Found a magic color
			if( remapFIndex != -1 )
				ReportError( "More than one magic color in fullbright image.\n" );

			remapFIndex = iMagic;
			remapF[ i ] = 255;
		}
		else
		{
			// Find new location for non-magic color
	 		remapF[ i ] = remap++;
		}

	}

	if( remapFIndex != -1 )
		printf("\tFullbright magic color was found at index %d.\n", remapFIndex);
	else
		printf("\tNo Magic color found, assuminng completely fully bright.\n");
}
	

void CreateNormalRemap()
{
	int i;

	// Initialize
	remapNIndex = -1;
	memset( remapN, 0xff, MAX_NON_FULL_BRIGHT * sizeof(int) );

	int remap = 0;

	for (i=0; i<MAX_NON_FULL_BRIGHT; i++)
	{
		int iMagic = -1;

		if ((ppalN[i].rgbRed == 0) && (ppalN[i].rgbGreen == 0) && (ppalN[i].rgbBlue == 255))
			iMagic = i;
		else if ((ppalN[i].rgbRed == 0) && (ppalN[i].rgbGreen == 255) && (ppalN[i].rgbBlue == 0))
			iMagic = i;
		else if ((ppalN[i].rgbRed == 255) && (ppalN[i].rgbGreen == 0) && (ppalN[i].rgbBlue == 0))
			iMagic = i;

		if( iMagic != -1 )
		{
			// Found a magic color
			if( remapNIndex != -1 )
				ReportError( "More than one magic color in normal image.\n" );

			remapNIndex = iMagic;
			remapN[ i ] = 255;
		}
		else
		{
			// Find new location for non-magic color
	 		remapN[ i ] = remap++;
		}

	}

	if( remapNIndex != -1 )
		printf("\tNon-fullbright magic color was found at index %d.\n", remapNIndex);
	else
		printf("\tNo Magic color found, assuminng no fullbright.\n");

}


void MergeBitmaps( int w, int h )
{
	int i;

	// Merge bitmap bits
	for (i = 0; i<w * h; i++)
	{
		if( pbNBits[i] > MAX_NON_FULL_BRIGHT )
			ReportError( "Too many colors in normal image\n" );

		if( pbNBits[i] == remapNIndex )
		{
			// Get color from fullbright image
			int fcolor = pbFBits[i];
			if( fcolor > MAX_FULL_BRIGHT )
				ReportError( "Too many colors in fullbright image\n" );

			if( fcolor == remapFIndex )
			{
				// actual transparent pixel
				pbNBits[i] = 255;
			}
			else
				pbNBits[i] = remapF[ fcolor ]; 
		}
		else
		{
			// Make sure fullbright image is transparent here
			if( pbFBits[i] != remapFIndex )
			{
				printf( "WARNING: Images overlap at ( %d, %d )\n", i%w, i/w );
//				Beep(400,200);
			}

			// Remap normal color
			pbNBits[i] = remapN[ pbNBits[i] ];
		}
	}

	// Merge palettes
	RGBQUAD newpal[ 256 ];
	for( i=0; i<MAX_NON_FULL_BRIGHT; i++ )
		memcpy( &newpal[ remapN[ i ] ], &ppalN[ i ], sizeof(RGBQUAD) );

	for( i=0; i<MAX_FULL_BRIGHT; i++ )
		memcpy( &newpal[ remapF[ i ] ], &ppalF[ i ], sizeof(RGBQUAD) );;
	
	// ND  I decree that there is no transparency
#if 0
	// Add magic color
	newpal[255].rgbBlue = 255;
	newpal[255].rgbRed = newpal[255].rgbGreen = newpal[255].rgbReserved = 0;
#endif

	memcpy( ppalN, newpal, 256 * sizeof(RGBQUAD) );
}


int main(int argc, void **argv)
{
	HANDLE hFile;
	char szBuf[1024];
	char *pszName = (char *)argv[1];
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih, bmih2;

	printf("\t\n---------%s-----------\n\n", argv[0]);
	printf("\tvalve L.L.C.\n");

	if ((argc != 2) || (*pszName == '-'))
	{
		PrintUsage((char *)argv[0]);
		exit(1);
	}

	// Make sure that we have access to our source files

	printf("\tLoading %s ... ", pszName);

	hFile = CreateFile(pszName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, NULL);
		
	if (hFile == INVALID_HANDLE_VALUE)
	{
		sprintf(szBuf, "Couldn't open %s.", argv[1]);
		ReportError(szBuf);
	}
	
	if (LoadBMP(hFile, &pbFBits, &ppalF, &bmfh, &bmih) < 0)
		ReportError("\n");		// LoadBMP displays error.

	CloseHandle(hFile);

	printf("Loaded.\n");

	*pszName = 'N';

	printf("\tLoading %s ... ", pszName);

	hFile = CreateFile(pszName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, NULL);
		
	if (hFile == INVALID_HANDLE_VALUE)
	{
		sprintf(szBuf, "Couldn't open %s.", argv[1]);
		ReportError(szBuf);
	}

	if (LoadBMP(hFile, &pbNBits, &ppalN, NULL, &bmih2) < 0)
		ReportError("\n");		// LoadBMP displays error.

	CloseHandle(hFile);

	printf("Loaded.\n");

	if ((bmih.biWidth != bmih2.biWidth) || (bmih.biHeight != bmih2.biHeight))
	{
		sprintf(szBuf, "BMP Sizes don't match (%d,%d), (%d,%d)\n",
			bmih.biWidth, bmih.biHeight, bmih2.biWidth, bmih2.biHeight);
		ReportError(szBuf);
	}

	*pszName = 'M'; // Skip past the "N_"
	char *pOutName = strdup( pszName );
	if( pOutName[ 2 ] == '+' )
	{
		pOutName[ 1 ] = pOutName[ 2 ];
		pOutName[ 2 ] = pOutName[ 3 ];
		pOutName[ 3 ] = '~';
	}
	else
	{
		pOutName[ 1 ] = '~';
	}

	hFile = CreateFile( &pOutName[1], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		sprintf(szBuf, " Could not create the target bitmap: %s\n", pszName);
		ReportError(szBuf);
	}

	free( pOutName );

	// Merge files
	CreateFBRemap();
	CreateNormalRemap();
	MergeBitmaps( bmih.biWidth, bmih.biHeight );

	// Write Files
	BOOL fSuccess;
	DWORD dwActual;

	printf("\tWriting %s ... ", pszName);

	// Write out new bitmap
	fSuccess = WriteFile(hFile, &bmfh, sizeof(bmfh), &dwActual, NULL);
	if (!fSuccess || (dwActual != sizeof(bmfh)))
		ReportError("Write Failure.\n");

	fSuccess = WriteFile(hFile, &bmih, sizeof(bmih), &dwActual, NULL);
	if (!fSuccess || (dwActual != sizeof(bmih)))
		ReportError("Write Failure.\n");

	fSuccess = WriteFile(hFile, ppalN, sizeof(RGBQUAD) * 256, &dwActual, NULL);
	if (!fSuccess || (dwActual != (sizeof(RGBQUAD) * 256)))
		ReportError("Write Failure.\n");

	fSuccess = WriteFile(hFile, pbNBits, (bmih.biWidth * bmih.biHeight), &dwActual, NULL);
	if (!fSuccess || (dwActual != (DWORD)(bmih.biWidth * bmih.biHeight)))
		ReportError("Write Failure.\n");

	CloseHandle(hFile);

	printf("done!\n\n");

	return 0;
}	




// Verify that the unused colors in the N_ and F_ files are all the same

#if 0
	RGBQUAD rgbN, rgbF;
	RGBQUAD *prgb;

	prgb = ppalN;
	prgb += MAX_NON_FULL_BRIGHT - 1;
	rgbN = *prgb;

	printf("\tNon Full Bright filler color (%d,%d,%d)\n", rgbN.rgbBlue, 
			rgbN.rgbGreen, rgbN.rgbBlue);

	for (i = MAX_NON_FULL_BRIGHT; i < 256; i++, prgb++)
	{
		if (!CmpRGB(rgbN, *prgb))
		{
			sprintf(szBuf, "Palette Index %i does not match (%d,%d,%d)", i,
				rgbN.rgbRed, rgbN.rgbGreen, rgbN.rgbBlue);
			ReportWarning(szBuf);
		}
	}

	prgb = ppalF;
	prgb += MAX_FULL_BRIGHT - 1;
	rgbF = *prgb;

	printf("\tFull Bright filler color (%d,%d,%d)\n", rgbF.rgbBlue, 
			rgbF.rgbGreen, rgbF.rgbBlue);

	for (i = MAX_FULL_BRIGHT; i < 256; i++, prgb++)
	{
		if (!CmpRGB(rgbF, *prgb))
		{
			sprintf(szBuf, "Palette Index %i does not match (%d,%d,%d)", i, 
				rgbF.rgbRed, rgbF.rgbGreen, rgbF.rgbBlue);
			ReportWarning(szBuf);
		}
	}
#endif

