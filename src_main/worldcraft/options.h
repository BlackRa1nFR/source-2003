//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines options that are written to the registry.
//
// $NoKeywords: $
//=============================================================================

#ifndef OPTIONS_H
#define OPTIONS_H
#pragma once


#include <fstream.h>
#include <afxtempl.h>
#include "GameData.h"


enum TextureAlignment_t;


class COptionsGeneral
{
	public:
		int iUndoLevels;
		BOOL bLockingTextures;
		TextureAlignment_t eTextureAlignment;
		BOOL bLoadwinpos;
		BOOL bIndependentwin;
		BOOL bGroupWhileIgnore;
		BOOL bStretchArches;
		BOOL bShowHelpers;
}; 


class COptionsTextures
{
	public:
		CStringArray TextureFiles;
		int nTextureFiles;
		float fBrightness;
};


class COptionsColors
{
	public:

		bool bUseCustom;			// Whether to use the custom colors or not.
		bool bScaleAxisColor;		// Whether to use the intensity slider to scale the axis color in the 2D view.
		bool bScaleGridColor;		// Whether to use the intensity slider to scale the grid color in the 2D view.
		bool bScaleGridDotColor;	// Whether to use the intensity slider to scale the dotted grid color in the 2D view.
		bool bScaleGrid10Color;		// Whether to use the intensity slider to scale the 10 grid color in the 2D view.
		bool bScaleGrid1024Color;	// Whether to use the intensity slider to scale the 1024 grid color in the 2D view.

		COLORREF clrAxis;			// The 2D view axis color.
		COLORREF clrGrid;			// The 2D view grid color.
		COLORREF clrGridDot;		// The 2D view dotted grid color.
		COLORREF clrGrid10;			// The 2D view grid color for every 10th line.
		COLORREF clrGrid1024;		// The 2D view grid color for every 1024 units line.
		COLORREF clrBackground;		// The 2D view background color.
		COLORREF clrBrush;			// The color of brushes.
		COLORREF clrEntity;			// The default color of point entities & brush entities, can be overridden by the FGD.
		COLORREF clrSelection;		// The color of selected objects.
		COLORREF clrVertex;			// The color of vertices.
		COLORREF clrToolHandle;		// The color of tool handles.
		COLORREF clrToolBlock;		// The color of the block tool.
		COLORREF clrToolSelection;	// The color of the selection tool.
		COLORREF clrToolMorph;		// The color of the morph tool.
		COLORREF clrToolPath;		// The color of the path tool.
		COLORREF clrToolDrag;		// The color of tool bounds while it is being dragged.
};


class COptionsView2D
{
	public:

		BOOL bCrosshairs;
		BOOL bGroupCarve;
		BOOL bScrollbars;
		BOOL bRotateConstrain;
		BOOL bDrawVertices;
		BOOL bWhiteOnBlack;
		BOOL bGridHigh1024;
		BOOL bGridHigh10;
		BOOL bHideSmallGrid;
		BOOL bNudge;
		BOOL bOrientPrimitives;
		BOOL bAutoSelect;
		BOOL bSelectbyhandles;
		BOOL bKeepclonegroup;
		BOOL bGridHigh64;
		BOOL bCenteroncamera;
		BOOL bUsegroupcolors;
		BOOL bGridDots;
		int iDefaultGrid;
		int iGridIntensity;
		int iGridHighSpec;
};


class COptionsView3D
{
	public:

		BOOL bHardware;			// Whether to use hardware acceleration (disabled for OpenGL).
		BOOL bReverseY;			// Wether to reverse the mouse's Y axis when mouse looking.
		BOOL bUseMouseLook;		// Whether to use the engine's default movement controls.
		int iBackPlane;			// Distance to far clipping plane in world units.
		int nModelDistance;		// Distance in world units within which studio models render.
		BOOL bAnimateModels;	// Whether to animate studio models.
		int nForwardSpeedMax;	// Max forward speed in world units per second.
		int nTimeToMaxSpeed;	// Time to max forward speed in milliseconds.
		BOOL bFilterTextures;	// Whether to filter textures.
		BOOL bReverseSelection;	// Whether to animate studio models.
};


class COptionsConfigs
{
	public:

		COptionsConfigs(void);
		virtual ~COptionsConfigs(void);

		CGameConfig *AddConfig(void);

		int LoadGameConfigs(const char *pszFileName);
		void SaveGameConfigs(const char *pszFileName);

		// find a game config based on ID:
		CGameConfig *FindConfig(DWORD dwID, int *piIndex = NULL);

		int nConfigs;
		CTypedPtrArray<CPtrArray, CGameConfig*> Configs;

	protected:

		int ImportOldGameConfigs(const char *pszFileName);
};


class COptions
{
	public:

		COptions();

		void Init();

		BOOL Read();
		void SetDefaults();
		void Write(BOOL fOverwrite);
		void PerformChanges(DWORD = 0xffffffff);

		// Accessors:
		TextureAlignment_t GetTextureAlignment(void);
		TextureAlignment_t SetTextureAlignment(TextureAlignment_t eTextureAlignment);

		bool GetShowHelpers(void);
		void SetShowHelpers(bool bShow);

		BOOL SetLockingTextures(BOOL b);
		BOOL IsLockingTextures(void);

		// Attributes:
		UINT uDaysSinceInstalled;

		// Flags for PerformChanges:
		enum
		{
			secTextures = 0x01,
			secGeneral = 0x02,
			secView2D = 0x04,
			secView3D = 0x08,
			secConfigs = 0x10
		};

		COptionsGeneral general;
		COptionsView2D view2d;
		COptionsView3D view3d;
		COptionsTextures textures;
		COptionsConfigs configs;
		COptionsColors colors;

	private:

		void ReadColorSettings(void);
};

extern COptions Options;


#endif // OPTIONS_H
