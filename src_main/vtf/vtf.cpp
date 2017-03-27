//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: The VTF file format I/O class to help simplify access to VTF files
//
// NOTE:
//
// $NoKeywords: $
//=============================================================================

#include "imageloader.h"
#include "vtf/vtf.h"
#include "utlbuffer.h"
#include "tier0/dbg.h"
#include "vector.h"
#include "mathlib.h"
#include "vstdlib/strtools.h"
#include "tier0/mem.h"

// FIXME: Reenable once we have a multithreaded tier0 for tools
// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Disk format for VTF files
//
// NOTE: After the header is the low-res image data
// Then follows image data, which is sorted in the following manner
//
//	for each mip level (starting with 1x1, and getting larger)
//		for each animation frame
//			for each face
//				store the image data for the face
//
// NOTE: In memory, we store the data in the following manner:
//	for each animation frame
//		for each face
//			for each mip level (starting with the largest, and getting smaller)
//				store the image data for the face
//
// This is done because the various image manipulation function we have
// expect this format
//-----------------------------------------------------------------------------
#pragma pack(1)

// version number for the disk texture cache
#define VTF_MAJOR_VERSION 7
#define VTF_MINOR_VERSION 1

struct VTFFileHeader_t
{
	char fileTypeString[4]; // "VTF" Valve texture file
	int version[2]; // version[0].version[1]
	int headerSize;
	unsigned short width;
	unsigned short height;
	unsigned int flags;
	unsigned short numFrames;
	unsigned short startFrame;
	VectorAligned reflectivity; // This is a linear value, right?  Average of all frames?
	float bumpScale;
	ImageFormat imageFormat;
	unsigned char numMipLevels;

	ImageFormat lowResImageFormat;
	unsigned char lowResImageWidth;
	unsigned char lowResImageHeight;	
};

#pragma pack()


//-----------------------------------------------------------------------------
// Implementation of the VTF Texture
//-----------------------------------------------------------------------------
class CVTFTexture : public IVTFTexture
{
public:
	CVTFTexture();
	virtual ~CVTFTexture();

	virtual bool Init( int nWidth, int nHeight, ImageFormat fmt, int iFlags, int iFrameCount );

	// Methods to initialize the low-res image
	virtual void InitLowResImage( int nWidth, int nHeight, ImageFormat fmt );

	// Methods to set other texture fields
	virtual void SetBumpScale( float flScale );
	virtual void SetReflectivity( const Vector &vecReflectivity );

	// These are methods to help with optimization of file access
	virtual void LowResFileInfo( int *pStartLocation, int *pSizeInBytes ) const;
	virtual void ImageFileInfo( int nFrame, int nFace, int nMip, int *pStartLocation, int *pSizeInBytes) const;
	virtual int FileSize( int nMipSkipCount = 0 ) const;

	// When unserializing, we can skip a certain number of mip levels,
	// and we also can just load everything but the image data
	virtual bool Unserialize( CUtlBuffer &buf, bool bBufferHeaderOnly = false, int nSkipMipLevels = 0 );
	virtual bool Serialize( CUtlBuffer &buf );

	// Attributes...
	virtual int Width() const;
	virtual int Height() const;
	virtual int MipCount() const;

	virtual int RowSizeInBytes( int nMipLevel ) const;

	virtual ImageFormat Format() const;
	virtual int FaceCount() const;
	virtual int FrameCount() const;
	virtual int Flags() const;

	virtual float BumpScale() const;
	virtual const Vector &Reflectivity() const;

	virtual bool IsCubeMap() const;
	virtual bool IsNormalMap() const;

	virtual int LowResWidth() const;
	virtual int LowResHeight() const;
	virtual ImageFormat LowResFormat() const;

	// Computes the size (in bytes) of a single mipmap of a single face of a single frame 
	virtual int ComputeMipSize( int iMipLevel ) const;

	// Computes the size (in bytes) of a single face of a single frame
	// All mip levels starting at the specified mip level are included
	virtual int ComputeFaceSize( int iStartingMipLevel = 0 ) const;

	// Computes the total size of all faces, all frames
	virtual int ComputeTotalSize( ) const;

	// Computes the dimensions of a particular mip level
	virtual void ComputeMipLevelDimensions( int iMipLevel, int *pWidth, int *pHeight ) const;

	// Computes the size of a subrect (specified at the top mip level) at a particular lower mip level
	virtual void ComputeMipLevelSubRect( Rect_t* pSrcRect, int nMipLevel, Rect_t *pSubRect ) const;

	// Returns the base address of the image data
	virtual unsigned char *ImageData();

	// Returns a pointer to the data associated with a particular frame, face, and mip level
	virtual unsigned char *ImageData( int iFrame, int iFace, int iMipLevel );

	// Returns a pointer to the data associated with a particular frame, face, mip level, and offset
	virtual unsigned char *ImageData( int iFrame, int iFace, int iMipLevel, int x, int y );

	// Returns the base address of the low-res image data
	virtual unsigned char *LowResImageData();

	// Converts the texture's image format. Use IMAGE_FORMAT_DEFAULT
	virtual void ConvertImageFormat( ImageFormat fmt, bool bNormalToDUDV );

	// Generate spheremap based on the current cube faces (only works for cubemaps)
	// The look dir indicates the direction of the center of the sphere
	virtual void GenerateSpheremap( LookDir_t lookDir );

	// Fixes the cubemap faces orientation from our standard to the
	// standard the material system needs.
	virtual void FixCubemapFaceOrientation( );

	// Generates mipmaps from the base mip levels
	virtual void GenerateMipmaps();

	// Put 1/miplevel (1..n) into alpha.
	virtual void PutOneOverMipLevelInAlpha();

	// Computes the reflectivity
	virtual void ComputeReflectivity( );

	// Computes the alpha flags
	virtual void ComputeAlphaFlags();

	// Gets the texture all internally consistent assuming you've loaded
	// mip 0 of all faces of all frames
	virtual void PostProcess(bool bGenerateSpheremap, LookDir_t lookDir = LOOK_DOWN_Z);

	// Generate the low-res image bits
	virtual bool ConstructLowResImage();

private:
	// Allocate image data blocks with an eye toward re-using memory
	void AllocateImageData( int nMemorySize );
	void AllocateLowResImageData( int nMemorySize );

	// Compute the mip count based on the size + flags
	int ComputeMipCount( ) const;

	// Unserialization of low-res data
	bool LoadLowResData( CUtlBuffer &buf );

	// Unserialization of image data
	bool LoadImageData( CUtlBuffer &buf, const VTFFileHeader_t &header, int nSkipMipLevels );

	// Shutdown
	void Shutdown();

	// Makes a single frame of spheremap
	void ComputeSpheremapFrame( unsigned char **ppCubeFaces, unsigned char *pSpheremap, LookDir_t lookDir );

	// Serialization of image data
	bool WriteImageData( CUtlBuffer &buf );

	// Computes the size (in bytes) of a single mipmap of a single face of a single frame 
	int ComputeMipSize( int iMipLevel, ImageFormat fmt ) const;

	// Computes the size (in bytes) of a single face of a single frame
	// All mip levels starting at the specified mip level are included
	int ComputeFaceSize( int iStartingMipLevel, ImageFormat fmt ) const;

	// Computes the total size of all faces, all frames
	int ComputeTotalSize( ImageFormat fmt ) const;

	// Computes the location of a particular face, frame, and mip level
	int GetImageOffset( int iFrame, int iFace, int iMipLevel, ImageFormat fmt ) const;

private:
	int m_nWidth;
	int m_nHeight;
	ImageFormat m_Format;

	int m_nMipCount;
	int m_nFaceCount;
	int m_nFrameCount;

	int m_nImageAllocSize;
	int m_nFlags;
	unsigned char *m_pImageData;

	Vector m_vecReflectivity;
	float m_flBumpScale;
	
	// FIXME: Do I need this?
	int m_iStartFrame;

	// Low res data
	int m_nLowResImageAllocSize;
	ImageFormat m_LowResImageFormat;
	int m_nLowResImageWidth;
	int m_nLowResImageHeight;
	unsigned char *m_pLowResImageData;

	// FIXME: Remove
	// This is to make sure old-format .vtf files are read properly
	int	m_pVersion[2];
};


//-----------------------------------------------------------------------------
// Class factory
//-----------------------------------------------------------------------------
IVTFTexture *CreateVTFTexture()
{
	return new CVTFTexture;
}

void DestroyVTFTexture( IVTFTexture *pTexture )
{
	CVTFTexture *pTex = static_cast<CVTFTexture*>(pTexture);
	if (pTex)
		delete pTex;
}


//-----------------------------------------------------------------------------
// Allows us to only load in the first little bit of the VTF file to get info
//-----------------------------------------------------------------------------
int VTFFileHeaderSize()
{
	return sizeof(VTFFileHeader_t);
}


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CVTFTexture::CVTFTexture()
{
	m_nWidth = 0;
	m_nHeight = 0;
	m_Format = (ImageFormat)-1;

	m_nMipCount = 0;
	m_nFaceCount = 0;
	m_nFrameCount = 0;
	
	// FIXME: Is the start frame needed?
	m_iStartFrame = 0;
	m_flBumpScale = 1.0f;
	m_vecReflectivity.Init( 1.0, 1.0, 1.0f );

	m_nFlags = 0;
	m_pImageData = NULL;
	m_nImageAllocSize = 0;

	// Image format
	m_LowResImageFormat = (ImageFormat)-1;
	m_nLowResImageWidth = 0;
	m_nLowResImageHeight = 0;
	m_pLowResImageData = NULL;
	m_nLowResImageAllocSize = 0;
}

CVTFTexture::~CVTFTexture()
{
	Shutdown();
}

//-----------------------------------------------------------------------------
// Compute the mip count based on the size + flags
//-----------------------------------------------------------------------------
int CVTFTexture::ComputeMipCount( ) const
{
	// NOTE: No matter what, all mip levels should be created because
	// we have to worry about various fallbacks
	return ImageLoader::GetNumMipMapLevels( m_nWidth, m_nHeight );
}


//-----------------------------------------------------------------------------
// Returns true if it's a power of two
//-----------------------------------------------------------------------------
static bool IsPowerOfTwo( int input )
{
	return ( input & ( input-1 ) ) == 0;
}


//-----------------------------------------------------------------------------
// Allocate image data blocks with an eye toward re-using memory
//-----------------------------------------------------------------------------
void CVTFTexture::AllocateImageData( int nMemorySize )
{
	if (m_nImageAllocSize < nMemorySize)
	{
		if (m_pImageData)
		{
			delete [] m_pImageData;
		}

		m_pImageData = new unsigned char[ nMemorySize ];
		m_nImageAllocSize = nMemorySize;
	}
}

void CVTFTexture::AllocateLowResImageData( int nMemorySize )
{
	if (m_nLowResImageAllocSize < nMemorySize)
	{
		if (m_pLowResImageData)
		{
			delete [] m_pLowResImageData;
		}

		m_pLowResImageData = new unsigned char[ nMemorySize ];
		m_nLowResImageAllocSize = nMemorySize;
	}
}



//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
bool CVTFTexture::Init( int nWidth, int nHeight, ImageFormat fmt, int iFlags, int iFrameCount )
{
	if (iFlags & TEXTUREFLAGS_ENVMAP)
	{
		if (nWidth != nHeight)
		{
			Warning("Height and width must be equal for cubemaps!\n");
			return false;
		}
	}

	if( !IsPowerOfTwo( nWidth ) || !IsPowerOfTwo( nHeight ) )
	{
		Warning( "Image dimensions must be power of 2!\n" );
		return false;
	}

	if (fmt == IMAGE_FORMAT_DEFAULT)
		fmt = IMAGE_FORMAT_RGBA8888;

	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_Format = fmt;

	m_nFlags = iFlags;

	m_nMipCount = ComputeMipCount();
	m_nFrameCount = iFrameCount;
	m_nFaceCount = (iFlags & TEXTUREFLAGS_ENVMAP) ? CUBEMAP_FACE_COUNT : 1;
	
	// Need to do this because Shutdown deallocated the low-res image
	m_nLowResImageWidth = m_nLowResImageHeight = 0;

	// Allocate me some bits!
	int iMemorySize = ComputeTotalSize();
	AllocateImageData( iMemorySize );
	return true;
}


//-----------------------------------------------------------------------------
// Methods to initialize the low-res image
//-----------------------------------------------------------------------------
void CVTFTexture::InitLowResImage( int nWidth, int nHeight, ImageFormat fmt )
{
	m_nLowResImageWidth = nWidth;
	m_nLowResImageHeight = nHeight;
	m_LowResImageFormat = fmt;

	// Allocate low-res bits
	int iLowResImageSize = ImageLoader::GetMemRequired( m_nLowResImageWidth, 
		m_nLowResImageHeight, m_LowResImageFormat, false );
	AllocateLowResImageData(iLowResImageSize);
}


//-----------------------------------------------------------------------------
// Methods to set other texture fields
//-----------------------------------------------------------------------------
void CVTFTexture::SetBumpScale( float flScale )
{
	m_flBumpScale = flScale;
}

void CVTFTexture::SetReflectivity( const Vector &vecReflectivity )
{
	VectorCopy( vecReflectivity, m_vecReflectivity );
}

	
//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------
void CVTFTexture::Shutdown()
{
	if (m_pImageData)
	{
		delete[] m_pImageData;
		m_pImageData = NULL;
	}

	if (m_pLowResImageData)
	{
		delete[] m_pLowResImageData;
		m_pLowResImageData = NULL;
	}
}


//-----------------------------------------------------------------------------
// These are methods to help with optimization of file access
//-----------------------------------------------------------------------------
void CVTFTexture::LowResFileInfo( int *pStartLocation, int *pSizeInBytes) const
{
	// Once the header is read in, they indicate where to start reading
	// other data, and how many bytes to read....
	*pStartLocation = VTFFileHeaderSize();

	if ((m_nLowResImageWidth == 0) || (m_nLowResImageHeight == 0))
	{
		*pSizeInBytes = 0;
	}
	else
	{
		*pSizeInBytes = ImageLoader::GetMemRequired( m_nLowResImageWidth, 
			m_nLowResImageHeight, m_LowResImageFormat, false );
	}
}

void CVTFTexture::ImageFileInfo( int nFrame, int nFace, int nMipLevel, int *pStartLocation, int *pSizeInBytes) const
{
	int i;
	int iMipWidth;
	int iMipHeight;

	// The image data starts after the low-res image
	int iLowResSize;
	int nOffset;
	LowResFileInfo( &nOffset, &iLowResSize );
	nOffset += iLowResSize;

	// get to the right miplevel
	for( i = m_nMipCount - 1; i > nMipLevel; --i )
	{
		ComputeMipLevelDimensions( i, &iMipWidth, &iMipHeight );
		int iMipLevelSize = ImageLoader::GetMemRequired( iMipWidth, iMipHeight, m_Format, false );
		nOffset += iMipLevelSize * m_nFrameCount * m_nFaceCount;
	}

	// get to the right frame
	ComputeMipLevelDimensions( nMipLevel, &iMipWidth, &iMipHeight );
	int nFaceSize = ImageLoader::GetMemRequired( iMipWidth, iMipHeight, m_Format, false );

	// For backwards compatibility, we don't read in the spheremap fallback on
	// older format .VTF files...
	int nFacesToRead = m_nFaceCount;
	if (IsCubeMap())
	{
		if ((m_pVersion[0] == 7) && (m_pVersion[1] < 1))
		{
			nFacesToRead = 6;
			if (nFace == CUBEMAP_FACE_SPHEREMAP)
				--nFace;
		}
	}

	int nFrameSize = nFacesToRead * nFaceSize;
	nOffset += nFrameSize * nFrame;
	
	// get to the right face
	nOffset += nFace * nFaceSize;
	
	*pStartLocation = nOffset;
	*pSizeInBytes = nFaceSize;
}

int CVTFTexture::FileSize( int nMipSkipCount ) const
{
	// The image data starts after the low-res image
	int nLowResSize;
	int nOffset;
	LowResFileInfo( &nOffset, &nLowResSize );
	nOffset += nLowResSize;

	int nFaceSize = ComputeFaceSize( nMipSkipCount );
	int nImageSize = nFaceSize * m_nFaceCount * m_nFrameCount;
	return nOffset + nImageSize;
}


//-----------------------------------------------------------------------------
// Unserialization of low-res data
//-----------------------------------------------------------------------------
bool CVTFTexture::LoadLowResData( CUtlBuffer &buf )
{
	// Allocate low-res bits
	InitLowResImage( m_nLowResImageWidth, m_nLowResImageHeight, m_LowResImageFormat );
	int nLowResImageSize = ImageLoader::GetMemRequired( m_nLowResImageWidth, 
		m_nLowResImageHeight, m_LowResImageFormat, false );
	buf.Get( m_pLowResImageData, nLowResImageSize );

	return buf.IsValid();
}


//-----------------------------------------------------------------------------
// Unserialization of image data
//-----------------------------------------------------------------------------
bool CVTFTexture::LoadImageData( CUtlBuffer &buf, const VTFFileHeader_t &header, int nSkipMipLevels )
{
	// Fix up the mip count + size based on how many mip levels we skip...
	if (nSkipMipLevels > 0)
	{
		Assert( m_nMipCount > nSkipMipLevels );
		if (header.numMipLevels < nSkipMipLevels)
		{
			// NOTE: This can only happen with older format .vtf files
			Warning("Warning! Encountered old format VTF file; please rebuild it!\n");
			return false;
		}

		ComputeMipLevelDimensions( nSkipMipLevels, &m_nWidth, &m_nHeight );
		m_nMipCount -= nSkipMipLevels;
	}

	// read the texture image (including mipmaps if they are there and needed.)
	int iImageSize = ComputeFaceSize( );
	iImageSize *= m_nFaceCount * m_nFrameCount;

	// For backwards compatibility, we don't read in the spheremap fallback on
	// older format .VTF files...
	int nFacesToRead = m_nFaceCount;
	if (IsCubeMap())
	{
		if ((header.version[0] == 7) && (header.version[1] < 1))
			nFacesToRead = 6;
	}

	// NOTE: We load the bits this way because we store the bits in memory
	// differently that the way they are stored on disk; we store on disk
	// differently so we can only load up 
	// NOTE: The smallest mip levels are stored first!!
	AllocateImageData( iImageSize );
	for (int iMip = m_nMipCount; --iMip >= 0; )
	{
		// NOTE: This is for older versions...
		if (header.numMipLevels - nSkipMipLevels <= iMip)
			continue;

		int iMipSize = ComputeMipSize( iMip );

		for (int iFrame = 0; iFrame < m_nFrameCount; ++iFrame)
		{
			for (int iFace = 0; iFace < nFacesToRead; ++iFace)
			{
				unsigned char *pMipBits = ImageData( iFrame, iFace, iMip );
				buf.Get( pMipBits, iMipSize );
			}
		}
	}

	return buf.IsValid();
}


//-----------------------------------------------------------------------------
// Unserialization
//-----------------------------------------------------------------------------
bool CVTFTexture::Unserialize( CUtlBuffer &buf, bool bBufferHeaderOnly, int nSkipMipLevels )
{
	// When unserializing, we can skip a certain number of mip levels,
	// and we also can just load everything but the image data

	VTFFileHeader_t header;
	memset( &header, 0, sizeof(VTFFileHeader_t) );

	buf.Get( &header, sizeof(VTFFileHeader_t) );
	if (!buf.IsValid())
	{
		Warning("*** Error unserializing VTF file... is the file empty?\n");
		return false;
	}

	// Validity check
	if ( Q_strncmp( header.fileTypeString, "VTF", 4 ) )
	{
		Warning("*** Tried to load a non-VTF file as a VTF file!\n");
		return false;
	}

	if( header.version[0] != VTF_MAJOR_VERSION )
	{
		Warning("*** Encountered VTF file with an invalid version!\n");
		return false;
	}
	if( (header.flags & TEXTUREFLAGS_ENVMAP) && (header.width != header.height) )
	{
		Warning("*** Encountered VTF non-square cubemap!\n");
		return false;
	}
	if( header.width <= 0 || header.height <= 0 )
	{
		Warning( "*** Encountered VTF invalid texture size!\n" );
		return false;
	}

	m_nWidth = header.width;
	m_nHeight = header.height;
	m_Format = header.imageFormat;

	m_nFlags = header.flags;

	m_nFrameCount = header.numFrames;

	// NOTE: We're going to store space for all mip levels, even if we don't 
	// have data on disk for them. This is for backward compatibility
	m_nMipCount = ComputeMipCount();
	m_nFaceCount = (m_nFlags & TEXTUREFLAGS_ENVMAP) ? CUBEMAP_FACE_COUNT : 1;

	m_vecReflectivity = header.reflectivity;
	m_flBumpScale = header.bumpScale;

	// FIXME: Why is this needed?
	m_iStartFrame = header.startFrame;

	// FIXME: Remove
	// This is to make sure old-format .vtf files are read properly
	m_pVersion[0] = header.version[0];
	m_pVersion[1] = header.version[1];

	if( header.lowResImageWidth == 0 || header.lowResImageHeight == 0 )
	{
		m_nLowResImageWidth = 0;
		m_nLowResImageHeight = 0;
	}
	else
	{
		m_nLowResImageWidth = header.lowResImageWidth;
		m_nLowResImageHeight = header.lowResImageHeight;
	}
	m_LowResImageFormat = header.lowResImageFormat;
	
	// Helpful for us to be able to get VTF info without reading in a ton
	if (bBufferHeaderOnly)
		return true;

	if (!LoadLowResData( buf ))
		return false;

	if (!LoadImageData( buf, header, nSkipMipLevels ))
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Serialization of image data
//-----------------------------------------------------------------------------
bool CVTFTexture::WriteImageData( CUtlBuffer &buf )
{
	// NOTE: We load the bits this way because we store the bits in memory
	// differently that the way they are stored on disk; we store on disk
	// differently so we can only load up 
	// NOTE: The smallest mip levels are stored first!!
	for (int iMip = m_nMipCount; --iMip >= 0; )
	{
		int iMipSize = ComputeMipSize( iMip );

		for (int iFrame = 0; iFrame < m_nFrameCount; ++iFrame)
		{
			for (int iFace = 0; iFace < m_nFaceCount; ++iFace)
			{
				unsigned char *pMipBits = ImageData( iFrame, iFace, iMip );
				buf.Put( pMipBits, iMipSize );
			}
		}
	}

	return buf.IsValid();
}


//-----------------------------------------------------------------------------
// Serialization
//-----------------------------------------------------------------------------
bool CVTFTexture::Serialize( CUtlBuffer &buf )
{
	if (!m_pImageData)
	{
		Warning("*** Unable to serialize... have no image data!\n");
		return false;
	}

	VTFFileHeader_t header;
	Q_strncpy( header.fileTypeString, "VTF", 4 );
	header.version[0] = VTF_MAJOR_VERSION;
	header.version[1] = VTF_MINOR_VERSION;
	header.headerSize = sizeof(VTFFileHeader_t);

	header.width = m_nWidth;
	header.height = m_nHeight;
	header.flags = m_nFlags;
	header.numFrames = m_nFrameCount;
	header.numMipLevels = m_nMipCount;
	header.imageFormat = m_Format;
	VectorCopy( m_vecReflectivity, header.reflectivity );
	header.bumpScale = m_flBumpScale;

	// FIXME: Why is this needed?
	header.startFrame = m_iStartFrame;

	header.lowResImageWidth = m_nLowResImageWidth;
	header.lowResImageHeight = m_nLowResImageHeight;
	header.lowResImageFormat = m_LowResImageFormat;

	buf.Put( &header, sizeof(VTFFileHeader_t) );
	if (!buf.IsValid())
		return false;

	// Write the low-res image
	if (m_pLowResImageData)
	{
		int iLowResImageSize = ImageLoader::GetMemRequired( m_nLowResImageWidth, 
			m_nLowResImageHeight, m_LowResImageFormat, false );
		buf.Put( m_pLowResImageData, iLowResImageSize );
		if (!buf.IsValid())
			return false;
	}
	else
	{
		// If we have a non-zero image size, we better have bits!
		Assert((m_nLowResImageWidth == 0) || (m_nLowResImageHeight == 0));
	}
	
	// Write out the image
	WriteImageData( buf );
	return buf.IsValid();
}


//-----------------------------------------------------------------------------
// Attributes...
//-----------------------------------------------------------------------------
int CVTFTexture::Width() const
{
	return m_nWidth;
}

int CVTFTexture::Height() const
{
	return m_nHeight;
}

int CVTFTexture::MipCount() const
{
	return m_nMipCount;
}

ImageFormat CVTFTexture::Format() const
{
	return m_Format;
}

int CVTFTexture::FaceCount() const
{
	return m_nFaceCount;
}

int CVTFTexture::FrameCount() const
{
	return m_nFrameCount;
}

int CVTFTexture::Flags() const
{
	return m_nFlags;
}

bool CVTFTexture::IsCubeMap() const
{
	return (m_nFlags & TEXTUREFLAGS_ENVMAP) != 0; 
}

bool CVTFTexture::IsNormalMap() const
{
	return (m_nFlags & TEXTUREFLAGS_NORMAL) != 0; 
}

float CVTFTexture::BumpScale() const
{
	return m_flBumpScale;
}

const Vector &CVTFTexture::Reflectivity() const
{
	return m_vecReflectivity;
}

unsigned char *CVTFTexture::ImageData()
{
	return m_pImageData;
}

int CVTFTexture::LowResWidth() const
{
	return m_nLowResImageWidth;
}

int CVTFTexture::LowResHeight() const
{
	return m_nLowResImageHeight;
}

ImageFormat CVTFTexture::LowResFormat() const
{
	return m_LowResImageFormat;
}

unsigned char *CVTFTexture::LowResImageData()
{
	return m_pLowResImageData;
}

int CVTFTexture::RowSizeInBytes( int nMipLevel ) const
{
	int nWidth = (m_nWidth >> nMipLevel);
	if (nWidth < 1)
		nWidth = 1;
	return ImageLoader::SizeInBytes( m_Format ) * nWidth;
}


//-----------------------------------------------------------------------------
// Returns a pointer to the data associated with a particular frame, face, and mip level
//-----------------------------------------------------------------------------
unsigned char *CVTFTexture::ImageData( int iFrame, int iFace, int iMipLevel )
{
	Assert( m_pImageData );
	int iOffset = GetImageOffset( iFrame, iFace, iMipLevel, m_Format );
	return &m_pImageData[iOffset];
}


//-----------------------------------------------------------------------------
// Returns a pointer to the data associated with a particular frame, face, mip level, and offset
//-----------------------------------------------------------------------------
unsigned char *CVTFTexture::ImageData( int iFrame, int iFace, int iMipLevel, int x, int y )
{
#ifdef _DEBUG
	int nWidth, nHeight;
	ComputeMipLevelDimensions( iMipLevel, &nWidth, &nHeight );
	Assert( (x >= 0) && (x <= nWidth) && (y >= 0) && (y <= nHeight) );
#endif

	int nRowBytes = RowSizeInBytes( iMipLevel );
	int nTexelBytes = ImageLoader::SizeInBytes( m_Format );

	unsigned char *pMipBits = ImageData( iFrame, iFace, iMipLevel );
	pMipBits += y * nRowBytes + x * nTexelBytes;
	return pMipBits;
}


//-----------------------------------------------------------------------------
// Computes the size (in bytes) of a single mipmap of a single face of a single frame 
//-----------------------------------------------------------------------------
inline int CVTFTexture::ComputeMipSize( int iMipLevel, ImageFormat fmt ) const
{
	Assert( iMipLevel < m_nMipCount );
	int w, h;
	ComputeMipLevelDimensions( iMipLevel, &w, &h );
	return ImageLoader::GetMemRequired( w, h, fmt, false );		
}

int CVTFTexture::ComputeMipSize( int iMipLevel ) const
{
	// Version for the public interface; don't want to expose the fmt parameter
	return ComputeMipSize( iMipLevel, m_Format );
}


//-----------------------------------------------------------------------------
// Computes the size of a single face of a single frame 
// All mip levels starting at the specified mip level are included
//-----------------------------------------------------------------------------
inline int CVTFTexture::ComputeFaceSize( int iStartingMipLevel, ImageFormat fmt ) const
{
	int iSize = 0;
	int w = m_nWidth;
	int h = m_nHeight;

	for( int i = 0; i < m_nMipCount; ++i )
	{
		if (i >= iStartingMipLevel)
			iSize += ImageLoader::GetMemRequired( w, h, fmt, false );
		w >>= 1;
		h >>= 1;
		if( w < 1 )
			w = 1;
		if( h < 1 )
			h = 1;
	}
	return iSize;
}

int CVTFTexture::ComputeFaceSize( int iStartingMipLevel ) const
{
	// Version for the public interface; don't want to expose the fmt parameter
	return ComputeFaceSize( iStartingMipLevel, m_Format );
}


//-----------------------------------------------------------------------------
// Computes the total size of all faces, all frames
//-----------------------------------------------------------------------------
inline int CVTFTexture::ComputeTotalSize( ImageFormat fmt ) const
{
	// Compute the number of bytes required to store a single face/frame
	int iMemRequired = ComputeFaceSize( 0, fmt );

	// Now compute the total image size
	return m_nFaceCount * m_nFrameCount * iMemRequired;
}

int CVTFTexture::ComputeTotalSize( ) const
{
	// Version for the public interface; don't want to expose the fmt parameter
	return ComputeTotalSize( m_Format );
}


//-----------------------------------------------------------------------------
// Computes the location of a particular face, frame, and mip level
//-----------------------------------------------------------------------------
int CVTFTexture::GetImageOffset( int iFrame, int iFace, int iMipLevel, ImageFormat fmt ) const
{
	Assert( iFrame < m_nFrameCount );
	Assert( iFace < m_nFaceCount );
	Assert( iMipLevel < m_nMipCount );

	int i;
	int iOffset = 0;

	// get to the right frame
	int iFaceSize = ComputeFaceSize(0, fmt);
	iOffset = iFrame * m_nFaceCount * iFaceSize;

	// Get to the right face
	iOffset += iFace * iFaceSize;

	// Get to the right mip level
	for (i = 0; i < iMipLevel; ++i)
	{
		iOffset += ComputeMipSize( i, fmt );
	}
	
	return iOffset;
}


//-----------------------------------------------------------------------------
// Computes the dimensions of a particular mip level
//-----------------------------------------------------------------------------
void CVTFTexture::ComputeMipLevelDimensions( int iMipLevel, int *pMipWidth, int *pMipHeight ) const
{
	Assert(iMipLevel < m_nMipCount);

	*pMipWidth = m_nWidth >> iMipLevel;
	*pMipHeight = m_nHeight >> iMipLevel;
	if( *pMipWidth < 1 )
		*pMipWidth = 1;
	if( *pMipHeight < 1 )
		*pMipHeight = 1;
}


//-----------------------------------------------------------------------------
// Computes the size of a subrect at a particular mip level
//-----------------------------------------------------------------------------
void CVTFTexture::ComputeMipLevelSubRect( Rect_t *pSrcRect, int nMipLevel, Rect_t *pSubRect ) const
{
	Assert( pSrcRect->x >= 0 && pSrcRect->y >= 0 && 
		(pSrcRect->x + pSrcRect->width <= m_nWidth) &&  
		(pSrcRect->y + pSrcRect->height <= m_nHeight) );
	
	if (nMipLevel == 0)
	{
		*pSubRect = *pSrcRect;
		return;
	}

	float flInvShrink = 1.0f / (float)(1 << nMipLevel);
	pSubRect->x = pSrcRect->x * flInvShrink;
	pSubRect->y = pSrcRect->y * flInvShrink;
	pSubRect->width = (int)ceil( (pSrcRect->x + pSrcRect->width) * flInvShrink ) - pSubRect->x;
	pSubRect->height = (int)ceil( (pSrcRect->y + pSrcRect->height) * flInvShrink ) - pSubRect->y;
}


//-----------------------------------------------------------------------------
// Converts the texture's image format. Use IMAGE_FORMAT_DEFAULT
// if you want to be able to use various tool functions below
//-----------------------------------------------------------------------------
void CVTFTexture::ConvertImageFormat( ImageFormat fmt, bool bNormalToDUDV )
{
	if ( !m_pImageData )
		return;

	if (fmt == IMAGE_FORMAT_DEFAULT)
		fmt = IMAGE_FORMAT_RGBA8888;

	if( bNormalToDUDV && !( fmt == IMAGE_FORMAT_UV88 || fmt == IMAGE_FORMAT_UVWQ8888 ) )
	{
		Assert( 0 );
		return;
	}

	if (m_Format == fmt)
		return;

	// FIXME: Should this be re-written to not do an allocation?
	int iConvertedSize = ComputeTotalSize( fmt );

	unsigned char *pConvertedImage = (unsigned char*)MemAllocScratch(iConvertedSize);
	for (int iMip = 0; iMip < m_nMipCount; ++iMip)
	{
		int nMipWidth, nMipHeight;
		ComputeMipLevelDimensions( iMip, &nMipWidth, &nMipHeight );

		for (int iFrame = 0; iFrame < m_nFrameCount; ++iFrame)
		{
			for (int iFace = 0; iFace < m_nFaceCount; ++iFace)
			{
				unsigned char *pSrcData = ImageData( iFrame, iFace, iMip );
				unsigned char *pDstData = pConvertedImage + 
					GetImageOffset( iFrame, iFace, iMip, fmt );

				if( bNormalToDUDV )
				{
					if( fmt == IMAGE_FORMAT_UV88 )
					{
						ImageLoader::ConvertNormalMapRGBA8888ToDUDVMapUV88( pSrcData,
							nMipWidth, nMipHeight, pDstData );
					}
					else if( fmt == IMAGE_FORMAT_UVWQ8888 )
					{
						ImageLoader::ConvertNormalMapRGBA8888ToDUDVMapUVWQ8888( pSrcData,
							nMipWidth, nMipHeight, pDstData );
					}
					else
					{
						Assert( 0 );
						return;
					}
				}
				else
				{
					ImageLoader::ConvertImageFormat( pSrcData, m_Format, 
						pDstData, fmt, nMipWidth, nMipHeight );
				}
			}
		}
	}

	AllocateImageData(iConvertedSize);
	memcpy( m_pImageData, pConvertedImage, iConvertedSize );
	m_Format = fmt;
	MemFreeScratch();
}


//-----------------------------------------------------------------------------
// Enums + structures related to conversion from cube to spheremap
//-----------------------------------------------------------------------------
struct SphereCalc_t
{
	Vector dir;
	float m_flRadius;
	float m_flOORadius;
	float m_flRadiusSq;
	LookDir_t m_LookDir;
	Vector m_vecLookDir;
	unsigned char m_pColor[4];
	unsigned char **m_ppCubeFaces;
	int	m_iSize;
};


//-----------------------------------------------------------------------------
//
// Methods associated with computing a spheremap from a cubemap
//
//-----------------------------------------------------------------------------
static void CalcInit( SphereCalc_t *pCalc, int iSize, unsigned char **ppCubeFaces, LookDir_t lookDir = LOOK_DOWN_Z )
{
	// NOTE: Width + height should be the same
	pCalc->m_flRadius = iSize * 0.5f;
	pCalc->m_flRadiusSq = pCalc->m_flRadius * pCalc->m_flRadius;
	pCalc->m_flOORadius = 1.0f / pCalc->m_flRadius;
	pCalc->m_LookDir = lookDir;
    pCalc->m_ppCubeFaces = ppCubeFaces;
	pCalc->m_iSize = iSize;

	switch( lookDir)
	{
	case LOOK_DOWN_X:
		pCalc->m_vecLookDir.Init( 1, 0, 0 );
		break;

	case LOOK_DOWN_NEGX:
		pCalc->m_vecLookDir.Init( -1, 0, 0 );
		break;

	case LOOK_DOWN_Y:
		pCalc->m_vecLookDir.Init( 0, 1, 0 );
		break;

	case LOOK_DOWN_NEGY:
		pCalc->m_vecLookDir.Init( 0, -1, 0 );
		break;

	case LOOK_DOWN_Z:
		pCalc->m_vecLookDir.Init( 0, 0, 1 );
		break;

	case LOOK_DOWN_NEGZ:
		pCalc->m_vecLookDir.Init( 0, 0, -1 );
		break;
	}
}

static void TransformNormal( SphereCalc_t *pCalc, Vector& normal )
{
	Vector vecTemp = normal;

	switch( pCalc->m_LookDir)
	{
	// Look down +x
	case LOOK_DOWN_X:
		normal[0] = vecTemp[2];
		normal[2] = -vecTemp[0];
		break;

	// Look down -x
	case LOOK_DOWN_NEGX:
		normal[0] = -vecTemp[2];
		normal[2] = vecTemp[0];
		break;

	// Look down +y
	case LOOK_DOWN_Y:
		normal[0] = -vecTemp[0];
		normal[1] = vecTemp[2];
		normal[2] = vecTemp[1];
		break;

	// Look down -y
	case LOOK_DOWN_NEGY:
		normal[0] = vecTemp[0];
		normal[1] = -vecTemp[2];
		normal[2] = vecTemp[1];
		break;

	// Look down +z
	case LOOK_DOWN_Z:
		return;

	// Look down -z
	case LOOK_DOWN_NEGZ:
		normal[0] = -vecTemp[0];
		normal[2] = -vecTemp[2];
		break;
	}
}


//-----------------------------------------------------------------------------
// Given a iFace normal, determine which cube iFace to sample
//-----------------------------------------------------------------------------
static int CalcFaceIndex( const Vector& normal )
{
	float absx, absy, absz;

	absx = normal[0] >= 0 ? normal[0] : -normal[0];
	absy = normal[1] >= 0 ? normal[1] : -normal[1];
	absz = normal[2] >= 0 ? normal[2] : -normal[2];

	if ( absx > absy )
	{
		if ( absx > absz )
		{
			// left/right
			if ( normal[0] >= 0 )
				return CUBEMAP_FACE_RIGHT;
			return CUBEMAP_FACE_LEFT;
		}
	}
	else
	{
		if ( absy > absz )
		{
			// front / back
			if ( normal[1] >= 0 )
				return CUBEMAP_FACE_BACK;
			return CUBEMAP_FACE_FRONT;
		}
	}

	// top / bottom
	if ( normal[2] >= 0 )
		return CUBEMAP_FACE_UP;
	return CUBEMAP_FACE_DOWN;
}


static void CalcColor( SphereCalc_t *pCalc, int iFace, const Vector &normal, unsigned char *color )
{
	float x, y, w;

	int size = pCalc->m_iSize;
	float hw = 0.5 * size;
	
	if ( (iFace == CUBEMAP_FACE_LEFT) || (iFace == CUBEMAP_FACE_RIGHT) )
	{
		w = hw / normal[0];
		x = -normal[2];
		y = -normal[1];
		if ( iFace == CUBEMAP_FACE_LEFT )
			y = -y;
	}
	else if ( (iFace == CUBEMAP_FACE_FRONT) || (iFace == CUBEMAP_FACE_BACK) )
	{
		w = hw / normal[1];
		x = normal[0];
		y = normal[2];
		if ( iFace == CUBEMAP_FACE_FRONT )
			x = -x;
	}
	else
	{
		w = hw / normal[2];
		x = -normal[0];
		y = -normal[1];
		if ( iFace == CUBEMAP_FACE_UP )
			x = -x;
	}

	x = (x * w) + hw - 0.5;
	y = (y * w) + hw - 0.5;

	int u = (int)(x+0.5);
	int v = (int)(y+0.5);

	if ( u < 0 ) u = 0;
	else if ( u > (size-1) ) u = (size-1);

	if ( v < 0 ) v = 0;
	else if ( v > (size-1) ) v = (size-1);

	int offset = (v * size + u) * 4;

	unsigned char *pPix = pCalc->m_ppCubeFaces[iFace] + offset;
	color[0] = pPix[0];
	color[1] = pPix[1];
	color[2] = pPix[2];
	color[3] = pPix[3];
}


//-----------------------------------------------------------------------------
// Computes the spheremap color at a particular (x,y) texcoord
//-----------------------------------------------------------------------------
static void CalcSphereColor( SphereCalc_t *pCalc, float x, float y )
{
	Vector normal;
	float flRadiusSq = x*x + y*y;
	if (flRadiusSq > pCalc->m_flRadiusSq)
	{
		// Force a glancing reflection
		normal.Init( 0, 1, 0 );
	}
	else
	{
		// Compute the z distance based on x*x + y*y + z*z = r*r 
		float z = sqrt( pCalc->m_flRadiusSq - flRadiusSq );

		// Here's the untransformed surface normal
		normal.Init( x, y, z );
		normal *= pCalc->m_flOORadius;
	}

	// Transform the normal based on the actual view direction
	TransformNormal( pCalc, normal );

	// Compute the reflection vector (full spheremap solution)
	// R = 2 * (N dot L)N - L
	Vector vecReflect;
	float nDotL = DotProduct( normal, pCalc->m_vecLookDir );
	VectorMA( pCalc->m_vecLookDir, -2.0f * nDotL, normal, vecReflect );
	vecReflect *= -1.0f;

	int iFace = CalcFaceIndex( vecReflect );
	CalcColor( pCalc, iFace, vecReflect, pCalc->m_pColor );
}


//-----------------------------------------------------------------------------
// Makes a single frame of spheremap
//-----------------------------------------------------------------------------
void CVTFTexture::ComputeSpheremapFrame( unsigned char **ppCubeFaces, unsigned char *pSpheremap, LookDir_t lookDir )
{
	SphereCalc_t sphere;
	CalcInit( &sphere, m_nWidth, ppCubeFaces, lookDir );
	int offset = 0;
	for ( int y = 0; y < m_nHeight; y++ )
	{
		for ( int x = 0; x < m_nWidth; x++ )
		{
			int r = 0, g = 0, b = 0, a = 0;
			float u = (float)x - m_nWidth * 0.5f;
			float v = m_nHeight * 0.5f - (float)y;

			CalcSphereColor( &sphere, u, v );
			r += sphere.m_pColor[0];
			g += sphere.m_pColor[1];
			b += sphere.m_pColor[2];
			a += sphere.m_pColor[3];

			CalcSphereColor( &sphere, u + 0.25, v );
			r += sphere.m_pColor[0];
			g += sphere.m_pColor[1];
			b += sphere.m_pColor[2];
			a += sphere.m_pColor[3];

			v += 0.25;
			CalcSphereColor( &sphere, u + 0.25, v );
			r += sphere.m_pColor[0];
			g += sphere.m_pColor[1];
			b += sphere.m_pColor[2];
			a += sphere.m_pColor[3];

			CalcSphereColor( &sphere, u, v );
			r += sphere.m_pColor[0];
			g += sphere.m_pColor[1];
			b += sphere.m_pColor[2];
			a += sphere.m_pColor[3];

			pSpheremap[ offset + 0 ] = r >> 2;
			pSpheremap[ offset + 1 ] = g >> 2;
			pSpheremap[ offset + 2 ] = b >> 2;
			pSpheremap[ offset + 3 ] = a >> 2;
			offset += 4;
		}
	}
}


//-----------------------------------------------------------------------------
// Generate spheremap based on the current images (only works for cubemaps)
// The look dir indicates the direction of the center of the sphere
//-----------------------------------------------------------------------------
void CVTFTexture::GenerateSpheremap( LookDir_t lookDir )
{
	if (!IsCubeMap())
		return;

	Assert( m_Format == IMAGE_FORMAT_RGBA8888 );

	// We'll be doing our work in IMAGE_FORMAT_RGBA8888 mode 'cause it's easier
	unsigned char *pCubeMaps[6];

	// Allocate the bits for the spheremap
	int iMemRequired = ImageLoader::GetMemRequired( m_nWidth, m_nHeight, IMAGE_FORMAT_RGBA8888, false );
	unsigned char *pSphereMapBits = (unsigned char *)MemAllocScratch(iMemRequired);

	// Generate a spheremap for each frame of the cubemap
	for (int iFrame = 0; iFrame < m_nFrameCount; ++iFrame)
	{
		// Point to our own textures (highest mip level)
		for (int iFace = 0; iFace < 6; ++iFace)
		{
			pCubeMaps[iFace] = ImageData( iFrame, iFace, 0 );
		}

		// Compute the spheremap of the top LOD
		ComputeSpheremapFrame( pCubeMaps, pSphereMapBits, lookDir );

		// Compute the mip levels of the spheremap, converting from RGBA8888 to our format
		unsigned char *pFinalSphereMapBits = ImageData( iFrame, CUBEMAP_FACE_SPHEREMAP, 0 );
		ImageLoader::GenerateMipmapLevels( pSphereMapBits, pFinalSphereMapBits, 
			m_nWidth, m_nHeight, m_Format, 2.2, 2.2 );
	}

	// Free memory
	MemFreeScratch();
}


//-----------------------------------------------------------------------------
// Rotate the image depending on what iFace we've got...
// We need to do this because we define the cube textures in a different
// format from DX8.
//-----------------------------------------------------------------------------
static void FixCubeMapFacing( unsigned char* pImage, int cubeFaceID, int size, ImageFormat fmt )
{
	int retVal;	
	switch( cubeFaceID )
	{
	case CUBEMAP_FACE_RIGHT:	// +x
		retVal = ImageLoader::RotateImageLeft( pImage, pImage, size, fmt );
		Assert( retVal );
		retVal = ImageLoader::FlipImageVertically( pImage, pImage, size, fmt );
		Assert( retVal );
		break;

	case CUBEMAP_FACE_LEFT:		// -x
		retVal = ImageLoader::RotateImageLeft( pImage, pImage, size, fmt );
		Assert( retVal );
		retVal = ImageLoader::FlipImageHorizontally( pImage, pImage, size, fmt );
		Assert( retVal );
		break;

	case CUBEMAP_FACE_BACK:		// +y
		retVal = ImageLoader::RotateImage180( pImage, pImage, size, fmt );
		Assert( retVal );
		retVal = ImageLoader::FlipImageHorizontally( pImage, pImage, size, fmt );
		Assert( retVal );
		break;

	case CUBEMAP_FACE_FRONT:	// -y
		retVal = ImageLoader::FlipImageHorizontally( pImage, pImage, size, fmt );
		Assert( retVal );
		break;

	case CUBEMAP_FACE_UP:		// +z
		retVal = ImageLoader::RotateImageLeft( pImage, pImage, size, fmt );
		Assert( retVal );
		retVal = ImageLoader::FlipImageVertically( pImage, pImage, size, fmt );
		Assert( retVal );
		break;

	case CUBEMAP_FACE_DOWN:		// -z
		retVal = ImageLoader::FlipImageHorizontally( pImage, pImage, size, fmt );
		Assert( retVal );
		retVal = ImageLoader::RotateImageLeft( pImage, pImage, size, fmt );
		Assert( retVal );
		break;
	}
}


//-----------------------------------------------------------------------------
// Fixes the cubemap faces orientation from our standard to what the material system needs
//-----------------------------------------------------------------------------
void CVTFTexture::FixCubemapFaceOrientation( )
{
	if (!IsCubeMap())
		return;

	Assert( m_Format == IMAGE_FORMAT_RGBA8888 );

	for (int iMipLevel = 0; iMipLevel < m_nMipCount; ++iMipLevel)
	{
		int iMipSize, iTemp;
		ComputeMipLevelDimensions( iMipLevel, &iMipSize, &iTemp );
		Assert( iMipSize == iTemp );

		for (int iFrame = 0; iFrame < m_nFrameCount; ++iFrame)
		{
			for (int iFace = 0; iFace < 6; ++iFace)
			{
				FixCubeMapFacing( ImageData( iFrame, iFace, iMipLevel ), iFace, iMipSize, m_Format );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Generates mipmaps from the base mip levels
//-----------------------------------------------------------------------------
void CVTFTexture::GenerateMipmaps()
{
	Assert( m_Format == IMAGE_FORMAT_RGBA8888 );

	// FIXME: Should we be doing anything special for normalmaps
	// other than a final normalization pass?
	// FIXME: I don't think that we ever normalize the top mip level!!!!!!
	bool bNormalMap = ( Flags() & ( TEXTUREFLAGS_NORMAL | TEXTUREFLAGS_NORMALTODUDV ) ) != 0;
	for (int iMipLevel = 1; iMipLevel < m_nMipCount; ++iMipLevel)
	{
		int nMipWidth, nMipHeight;
		ComputeMipLevelDimensions( iMipLevel, &nMipWidth, &nMipHeight );
		float mipColorScale = 1.0f;
		if( Flags() & TEXTUREFLAGS_PREMULTCOLORBYONEOVERMIPLEVEL )
		{
			mipColorScale = 1.0f / ( float )( 1 << iMipLevel );
		}

		for (int iFrame = 0; iFrame < m_nFrameCount; ++iFrame)
		{
			for (int iFace = 0; iFace < m_nFaceCount; ++iFace)
			{
				unsigned char *pSrcLevel = ImageData( iFrame, iFace, 0 );
				unsigned char *pDstLevel = ImageData( iFrame, iFace, iMipLevel );
				ImageLoader::ResampleRGBA8888( pSrcLevel, pDstLevel, m_nWidth, m_nHeight,
					nMipWidth, nMipHeight, 2.2, 2.2, mipColorScale, bNormalMap );

				if( m_nFlags & TEXTUREFLAGS_NORMAL )
				{
					ImageLoader::NormalizeNormalMapRGBA8888( pDstLevel, nMipWidth * nMipHeight );
				}
			}
		}
	}
}

void CVTFTexture::PutOneOverMipLevelInAlpha()
{
	Assert( m_Format == IMAGE_FORMAT_RGBA8888 );

	for (int iMipLevel = 0; iMipLevel < m_nMipCount; ++iMipLevel)
	{
		int nMipWidth, nMipHeight;
		ComputeMipLevelDimensions( iMipLevel, &nMipWidth, &nMipHeight );
		int size = nMipWidth * nMipHeight;
		unsigned char ooMipLevel = ( unsigned char )( 255.0f * ( 1.0f / ( float )( 1 << iMipLevel ) ) );

		for (int iFrame = 0; iFrame < m_nFrameCount; ++iFrame)
		{
			for (int iFace = 0; iFace < m_nFaceCount; ++iFace)
			{
				unsigned char *pDstLevel = ImageData( iFrame, iFace, iMipLevel );
				unsigned char *pDst;
				for( pDst = pDstLevel; pDst < pDstLevel + size * 4; pDst += 4 )
				{
					pDst[3] = ooMipLevel;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Computes the reflectivity
//-----------------------------------------------------------------------------
void CVTFTexture::ComputeReflectivity( )
{
	Assert( m_Format == IMAGE_FORMAT_RGBA8888 );

	int divisor = 0;
	m_vecReflectivity.Init( 0.0f, 0.0f, 0.0f );
	for( int iFrame = 0; iFrame < m_nFrameCount; ++iFrame )
	{
		for( int iFace = 0; iFace < m_nFaceCount; ++iFace )
		{
			Vector vecFaceReflect;
			unsigned char* pSrc = ImageData( iFrame, iFace, 0 );
			int nNumPixels = m_nWidth * m_nHeight;

			VectorClear( vecFaceReflect );
			for (int i = 0; i < nNumPixels; ++i, pSrc += 4 )
			{
				vecFaceReflect[0] += TextureToLinear( pSrc[0] );
				vecFaceReflect[1] += TextureToLinear( pSrc[1] );
				vecFaceReflect[2] += TextureToLinear( pSrc[2] );
			}	

			vecFaceReflect /= nNumPixels;

			m_vecReflectivity += vecFaceReflect;
			++divisor;
		}
	}
	m_vecReflectivity /= divisor;
}


//-----------------------------------------------------------------------------
// Computes the alpha flags
//-----------------------------------------------------------------------------
void CVTFTexture::ComputeAlphaFlags()
{
	Assert( m_Format == IMAGE_FORMAT_RGBA8888 );

	m_nFlags &= ~(TEXTUREFLAGS_EIGHTBITALPHA | TEXTUREFLAGS_ONEBITALPHA);
	
	if( TEXTUREFLAGS_ONEOVERMIPLEVELINALPHA )
	{
		m_nFlags |= TEXTUREFLAGS_EIGHTBITALPHA;
		return;
	}
	
	for( int iFrame = 0; iFrame < m_nFrameCount; ++iFrame )
	{
		for( int iFace = 0; iFace < m_nFaceCount; ++iFace )
		{
			// If we're all 0 or all 255, assume it's opaque
			bool bHasZero = false;
			bool bHas255 = false;

			unsigned char* pSrcBits = ImageData( iFrame, iFace, 0 );
			int nNumPixels = m_nWidth * m_nHeight;
			while (--nNumPixels >= 0)
			{
				if (pSrcBits[3] == 0)
					bHasZero = true;
				else if (pSrcBits[3] == 255)
					bHas255 = true;
				else
				{
					// Have grey at all? 8 bit alpha baby
					m_nFlags &= ~TEXTUREFLAGS_ONEBITALPHA;
					m_nFlags |= TEXTUREFLAGS_EIGHTBITALPHA;
					return;
				}

				pSrcBits += 4;
			}

			// If we have both 0 at 255, we're at least one-bit alpha
			if (bHasZero && bHas255)
			{
				m_nFlags |= TEXTUREFLAGS_ONEBITALPHA;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Gets the texture all internally consistent assuming you've loaded
// mip 0 of all faces of all frames
//-----------------------------------------------------------------------------
void CVTFTexture::PostProcess(bool bGenerateSpheremap, LookDir_t lookDir)
{
	Assert( m_Format == IMAGE_FORMAT_RGBA8888 );

	// Set up the cube map faces
	if (IsCubeMap())
	{
		// Rotate the cubemaps so they're appropriate for the material system
		FixCubemapFaceOrientation();

		// FIXME: We could theoretically not compute spheremap mip levels
		// in generate spheremaps; should we? The trick is when external
		// clients can be expected to call it

		// Compute the spheremap fallback for cubemaps if we weren't able to load up one...
		if (bGenerateSpheremap)
			GenerateSpheremap(lookDir);
	}

	// Generate mipmap levels
	GenerateMipmaps();

	if( Flags() & TEXTUREFLAGS_ONEOVERMIPLEVELINALPHA )
	{
		PutOneOverMipLevelInAlpha();
	}
	
	// Compute reflectivity
	ComputeReflectivity();

	// Are we 8-bit or 1-bit alpha?
	// NOTE: We have to do this *after*	computing the spheremap fallback for
	// cubemaps or it'll throw the flags off
	ComputeAlphaFlags();
}


//-----------------------------------------------------------------------------
// Generate the low-res image bits
//-----------------------------------------------------------------------------
bool CVTFTexture::ConstructLowResImage()
{
	Assert( m_Format == IMAGE_FORMAT_RGBA8888 );
	Assert( m_pLowResImageData );

	CUtlMemory<unsigned char> lowResSizeImage;
	lowResSizeImage.EnsureCapacity( m_nLowResImageWidth * m_nLowResImageHeight * 4 );
	unsigned char *tmpImage = lowResSizeImage.Base();
	if( !ImageLoader::ResampleRGBA8888( ImageData(0, 0, 0), tmpImage, 
		m_nWidth, m_nHeight, m_nLowResImageWidth, m_nLowResImageHeight, 2.2f, 2.2f ) )
	{
		return false;
	}
	
	// convert to the low-res size version with the correct image format
	return ImageLoader::ConvertImageFormat( tmpImage, IMAGE_FORMAT_RGBA8888, 
		m_pLowResImageData, m_LowResImageFormat, m_nLowResImageWidth, m_nLowResImageHeight ); 
}
