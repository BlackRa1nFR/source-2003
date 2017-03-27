//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MATERIALSYSTEM_CONFIG_H
#define MATERIALSYSTEM_CONFIG_H
#ifdef _WIN32
#pragma once
#endif


#define MATERIALSYSTEM_CONFIG_VERSION "VMaterialSystemConfig001"

struct MaterialSystem_Config_t
{
	// changable at runtime.
	float screenGamma;
	float texGamma;
	float overbright;
	bool bAllowCheats;
	bool bLinearFrameBuffer;
	float polyOffset;
	int skipMipLevels; // 0 for normal rendering, 1 for one lod lower, etc.
	float lightScale;	
	bool bFilterLightmaps;
	bool bFilterTextures;
	bool bMipMapTextures;
	bool bBumpmap;
	bool bShowSpecular;
	bool bShowDiffuse;
	bool bShowNormalMap; 
	bool bLightingOnly; // fullbright 2
	bool bCompressedTextures;
	bool bShowMipLevels;
	bool bShowLowResImage;
	bool bReverseDepth;
	bool bBufferPrimitives;
	bool bDrawFlat;
	bool bMeasureFillRate;
	bool bVisualizeFillRate;
	bool bSoftwareLighting;
	bool bNoTransparency;

	// -1 - let the driver buffer as many frames as it wants
	// 0 - sync every frame
	// 1 - sync every two frames
	int maxFrameLatency;	
	
	unsigned char proxiesTestMode;	// 0 = normal, 1 = no proxies, 2 = alpha test all, 3 = color mod all
	bool bEditMode;				// true if in WorldCraft.

 	int	dxSupportLevel;			// 50 for dx5 (single texture), 
 								// 60 for dx6, 
 								// 70 for dx7, 
 								// 80 for dx8
 								// 81 for dx8 with ps1.4
 								// 90 for dx9 with ps2.0
								// 0 means use max possible
	// render budgets
	int m_MaxWorldMegabytes;
	int m_MaxOtherMegabytes;
	int m_MaxTextureMemory;
	int m_MaxLightmapMemory;
	int m_MaxModelMemory;
	int m_NewTextureMemoryPerFrame;
	float m_MaxDepthComplexity;

// not changable at runtime.
	int numTextureUnits; // set to zero if there is no limit on the 
						 // number of texture units to be used.
						 // otherwise, the effective number of texture units
						 // will be max( config->numTexturesUnits, hardwareNumTextureUnits )

	bool m_bForceTrilinear;
	float m_SlopeScaleDepthBias_Decal;
	float m_SlopeScaleDepthBias_Normal;
	float m_DepthBias_Decal;
	float m_DepthBias_Normal;
	bool m_bFastNoBump;
	bool m_bForceHardwareSync;
	bool m_bSuppressRendering;
	int  m_nForceAnisotropicLevel;
	bool m_bForceBilinear;
};


#endif // MATERIALSYSTEM_CONFIG_H
