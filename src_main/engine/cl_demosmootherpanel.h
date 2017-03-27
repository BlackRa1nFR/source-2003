//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CL_DEMOSMOOTHERPANEL_H
#define CL_DEMOSMOOTHERPANEL_H
#ifdef _WIN32
#pragma once
#endif

//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include <vgui_controls/Frame.h>

namespace vgui
{
class Button;
class Label;
class ListPanel;
class IScheme;
};

#include "cl_demosmoothing.h"

struct demodirectory_t;
class CSmoothingTypeButton;
class CFixEdgeButton;

typedef float (*EASEFUNC)( float t );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDemoSmootherPanel : public vgui::Frame
{
	typedef vgui::Frame BaseClass;

public:
	CDemoSmootherPanel( vgui::Panel *parent );
	~CDemoSmootherPanel();

	virtual void OnTick();

	// Command issued
	virtual void OnCommand(const char *command);

	void		OnVDMChanged( void );

	void		OnRefresh();

	virtual bool		OverrideView( democmdinfo_t& info, int frame, float elapsed );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void	DrawDebuggingInfo(  int frame, float elapsed );


protected:

	bool		CanEdit();

	void		Reset( void );

	demosmoothing_t *GetCurrent( void );

	void		HandleInput( bool active );

	void		DrawSmoothingSample( bool original, bool processed, int samplenumber, demosmoothing_t *sample, demosmoothing_t *next );
	void		DrawTargetSpline( void );
	void		DrawKeySpline( void );
	float		GetTimeForFrame( int frame );
	bool		GetInterpolatedViewPoint( void );
	bool		GetInterpolatedOriginAndAngles( bool readonly, Vector& origin, QAngle& angles );

	void		DrawLegend( int startframe, int endframe );

	void		OnRevert();
	void		OnPreview( bool original );
	void		OnSave();
	void		OnReload();
	void		OnSelect();
	void		OnTogglePause();
	void		OnStep( bool forward );
	void		OnGotoFrame();

	void		OnToggleKeyFrame( void );
	void		OnToggleLookTarget( void );

	void		OnNextKey();
	void		OnPrevKey();
	void		OnNextTarget();
	void		OnPrevTarget();

	void		OnRevertPoint( void );

	void		PopulateMenuList();
	int			GetStartFrame();
	int			GetEndFrame();

	void		OnSaveKey();
	void		OnSetView();

	void		OnSmoothEdges( bool left, bool right );

	void		PerformLinearInterpolatedAngleSmoothing( int startframe, int endframe );

	void		OnSmoothSelectionAngles( void );
	void		OnSmoothSelectionOrigin( void );
	void		OnLinearInterpolateAnglesBasedOnEndpoints( void );
	void		OnLinearInterpolateOriginBasedOnEndpoints( void );
	void		OnSplineSampleOrigin( void );
	void		OnSplineSampleAngles( void );
	void		OnLookAtPoints( bool spline );

	void		OnOriginEaseCurve( EASEFUNC easefunc );

	void		SetLastFrame( bool jumptotarget, int frame );

	void		AddSamplePoints( bool usetarget, bool includeboundaries, CUtlVector< demosmoothing_t * >& points, int start, int end );
	demosmoothing_t *GetBoundedSample(  CUtlVector< demosmoothing_t * >& points, int sample );
	void		FindSpanningPoints( float t, CUtlVector< demosmoothing_t * >& points, int& prev, int& next );

	// Undo/Redo
	void				Undo( void );
	void				Redo( void );

	// Do push before changes
	void				PushUndo( char *description );
	// Do this push after changes, must match pushundo 1for1
	void				PushRedo( char *description );

	void				WipeUndo( void );
	void				WipeRedo( void );

	const char			*GetUndoDescription( void );
	const char			*GetRedoDescription( void );

	bool				CanUndo( void );
	bool				CanRedo( void );

	CSmoothingTypeButton	*m_pType;

	vgui::Button	*m_pRevert;
	vgui::Button	*m_pOK;
	vgui::Button	*m_pCancel;

	vgui::Button	*m_pSave;
	vgui::Button	*m_pReloadFromDisk;

	vgui::TextEntry		*m_pStartFrame;
	vgui::TextEntry		*m_pEndFrame;

	vgui::Button		*m_pPreviewOriginal;
	vgui::Button		*m_pPreviewProcessed;

	vgui::CheckButton	*m_pBackOff;

	vgui::Label			*m_pSelectionInfo;
	vgui::CheckButton	*m_pShowAllSamples;
	vgui::Button		*m_pSelectSamples;

	vgui::Button		*m_pPauseResume;
	vgui::Button		*m_pStepForward;
	vgui::Button		*m_pStepBackward;

	vgui::CheckButton	*m_pHideLegend;

	vgui::CheckButton	*m_pHideOriginal;
	vgui::CheckButton	*m_pHideProcessed;

	vgui::Button		*m_pToggleKeyFrame;
	vgui::Button		*m_pToggleLookTarget;
	vgui::Button		*m_pRevertPoint;

	vgui::Button		*m_pDriveCamera;
	vgui::Button		*m_pDriveCameraSlow;
	vgui::Button		*m_pMoveCameraToPoint;

	vgui::Button		*m_pUndo;
	vgui::Button		*m_pRedo;

	vgui::Button		*m_pNextKey;
	vgui::Button		*m_pPrevKey;
	vgui::Button		*m_pNextTarget;
	vgui::Button		*m_pPrevTarget;

	CFixEdgeButton		*m_pFixEdges;
	vgui::TextEntry		*m_pFixEdgeFrames;

	vgui::Button		*m_pProcessKey;

	vgui::CheckButton	*m_pLockCamera;

	vgui::TextEntry		*m_pGotoFrame;
	vgui::Button		*m_pGoto;

	bool				m_bHasSelection;
	int					m_nSelection[2];
	float				m_flSelectionTimeSpan;

	bool				m_bPreviewing;
	bool				m_bPreviewOriginal;
	float				m_flPreviewStartTime;
	float				m_flPreviewCurrentTime;
	int					m_nPreviewLastFrame;
	bool				m_bPreviewPaused;

	CSmoothingContext	m_Smoothing;

	bool				m_bInputActive;
	int					m_nOldCursor[2];

	Vector				m_ViewOrigin;
	QAngle				m_ViewAngles;

	struct DemoSmoothUndo
	{
		CSmoothingContext *undo;
		CSmoothingContext *redo;
		char		 *udescription;
		char		 *rdescription;
	};

	CUtlVector< DemoSmoothUndo * >	m_UndoStack;
	int					m_nUndoLevel;
	bool				m_bRedoPending;

	bool				m_bDirty;

	Vector				m_vecEyeOffset;
};

#endif // CL_DEMOSMOOTHERPANEL_H
