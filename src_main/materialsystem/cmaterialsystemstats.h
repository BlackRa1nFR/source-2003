//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CMATERIALSYSTEMSTATS_H
#define CMATERIALSYSTEMSTATS_H
#pragma once

#include "IMaterialSystemStats.h"

class CMaterialSystemStats : public IMaterialSystemStats
{
public:
	//
	// stuff from IMaterialSystemStats
	//
	double GetBytesUniqueTexturesInFrame();
	double GetUniqueTexelsInFrame();

	double GetBytesTexturesInFrame();
	double GetTexelsInFrame();

	double GetBytesUniqueTextureDownloadsDueToCacheMissInFrame();
	double GetUniqueTexelDownloadsDueToCacheMissInFrame();

	double GetBytesTextureDownloadsDueToCacheMissInFrame();
	double GetTexelDownloadsDueToCacheMissInFrame();

	double GetUniqueLightmapTexelsInFrame();

	// these are never compressed
	double GetBytesProceduralTextureDownloadsInFrame();

	int GetNumImplicitFlushesInFrame();

	int	GetNumTextureCacheMissesInFrame();
	int GetNumTextureBindsInFrame();
	int GetNumUniqueTextureBindsInFrame();

	void SetTextureGroup( int textureGroup )
	{
		m_TextureGroup = textureGroup;
	}

	//
	// stuff local to the material system
	//
	static void BeginFrame();
	static void EndFrame();

	static void AddBytesUniqueTexturesInFrame( double bytes );
	static void AddUniqueTexelsInFrame( double texels );

	static void AddBytesTexturesInFrame( double bytes );
	static void AddTexelsInFrame( double texels );

	static void AddBytesUniqueTextureDownloadsDueToCacheMissInFrame( double bytes );
	static void AddUniqueTexelDownloadsDueToCacheMissInFrame( double texels );

	static void AddBytesTextureDownloadsDueToCacheMissInFrame( double bytes );
	static void AddTexelDownloadsDueToCacheMissInFrame( double texels );

	static void AddBytesProceduralTextureDownloadsInFrame( double bytes );

	static void AddTextureCacheMiss( int count );
	static void AddTextureBind( int count );
	static void AddUniqueTextureBind( int count );

	static void AddImplicitFlush( void );

	static void BeginBindProxy( void );
	static void EndBindProxy( void );

	static void AddUniqueLightmapPageBind( void );

private:
	struct TextureStats_t
	{
		double bytesUniqueTexturesInFrame;
		double uniqueTexelsInFrame;
		
		double bytesTexturesInFrame;
		double texelsInFrame;
		
		double bytesUniqueTextureDownloadsDueToCacheMissInFrame;
		double uniqueTexelDownloadsDueToCacheMissInFrame;
		
		double bytesTextureDownloadsDueToCacheMissInFrame;
		double texelDownloadsDueToCacheMissInFrame;
		
		double bytesProceduralTextureDownloadsInFrame;
		
		int numTextureCacheMissesInFrame;
		int numTextureBindsInFrame;
		int numUniqueTextureBindsInFrame;
		
		int numImplicitFlushesInFrame;
	};

	static TextureStats_t m_TextureStats[MATERIAL_SYSTEM_STATS_NUM_TEXTURE_GROUPS];
	static int m_TextureGroup;
	static int m_NumUniqueLightmapPageBinds;
};

inline void CMaterialSystemStats::AddBytesUniqueTexturesInFrame( double bytes )
{
	m_TextureStats[m_TextureGroup].bytesUniqueTexturesInFrame += bytes;
}

inline void CMaterialSystemStats::AddUniqueTexelsInFrame( double texels )
{
	m_TextureStats[m_TextureGroup].uniqueTexelsInFrame += texels;
}

inline void CMaterialSystemStats::AddBytesTexturesInFrame( double bytes )
{
	m_TextureStats[m_TextureGroup].bytesTexturesInFrame += bytes;
}

inline void CMaterialSystemStats::AddTexelsInFrame( double texels )
{
	m_TextureStats[m_TextureGroup].texelsInFrame += texels;
}

inline void CMaterialSystemStats::AddBytesUniqueTextureDownloadsDueToCacheMissInFrame( double bytes )
{
	m_TextureStats[m_TextureGroup].bytesUniqueTextureDownloadsDueToCacheMissInFrame += bytes;
}

inline void CMaterialSystemStats::AddUniqueTexelDownloadsDueToCacheMissInFrame( double texels )
{
	m_TextureStats[m_TextureGroup].uniqueTexelDownloadsDueToCacheMissInFrame += texels;
}

inline void CMaterialSystemStats::AddBytesTextureDownloadsDueToCacheMissInFrame( double bytes )
{
	m_TextureStats[m_TextureGroup].bytesTextureDownloadsDueToCacheMissInFrame += bytes;
}

inline void CMaterialSystemStats::AddTexelDownloadsDueToCacheMissInFrame( double texels )
{
	m_TextureStats[m_TextureGroup].texelDownloadsDueToCacheMissInFrame += texels;
}

inline void CMaterialSystemStats::AddBytesProceduralTextureDownloadsInFrame( double bytes )
{
	m_TextureStats[m_TextureGroup].bytesProceduralTextureDownloadsInFrame += bytes;
}

inline void CMaterialSystemStats::AddTextureCacheMiss( int count )
{
	m_TextureStats[m_TextureGroup].numTextureCacheMissesInFrame += count;
}

inline void CMaterialSystemStats::AddTextureBind( int count )
{
	m_TextureStats[m_TextureGroup].numTextureBindsInFrame += count;
}

inline void CMaterialSystemStats::AddUniqueTextureBind( int count )
{
	m_TextureStats[m_TextureGroup].numUniqueTextureBindsInFrame++;
}

inline void CMaterialSystemStats::AddImplicitFlush( void )
{
	m_TextureStats[m_TextureGroup].numImplicitFlushesInFrame++;
}

inline void CMaterialSystemStats::AddUniqueLightmapPageBind( void )
{
	m_NumUniqueLightmapPageBinds++;
}

#endif // CMATERIALSYSTEMSTATS_H
