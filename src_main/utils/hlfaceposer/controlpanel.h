//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           ControlPanel.h
// last modified:  May 29 programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
// version:        1.2
//
// email:          mete@swissquake.ch
// web:            http://www.swissquake.ch/chumbalum-soft/
//
#ifndef INCLUDED_CONTROLPANEL
#define INCLUDED_CONTROLPANEL



#ifndef INCLUDED_MXWINDOW
#include <mx/mxWindow.h>
#endif

#include "faceposertoolwindow.h"

#define IDC_TAB						1901
#define IDC_RENDERMODE				2001
#define IDC_GROUND					2003
#define IDC_MOVEMENT				2004
#define IDC_BACKGROUND				2005
#define IDC_HITBOXES				2006
#define IDC_BONES					2007
#define IDC_ATTACHMENTS				2008
#define IDC_PHYSICSMODEL			2009
#define IDC_PHYSICSHIGHLIGHT		2010
#define IDC_MODELSPACING			2011
#define IDC_TOOLSDRIVEMOUTH			2012

#define IDC_SEQUENCE				3001
#define IDC_SPEEDSCALE				3002
#define IDC_PRIMARYBLEND			3003
#define IDC_SECONDARYBLEND			3004

#define IDC_BODYPART				4001
#define IDC_SUBMODEL				4002
#define IDC_CONTROLLER				4003
#define IDC_CONTROLLERVALUE			4004
#define IDC_SKINS					4005

#define IDC_EXPRESSIONCLASS			5001
#define IDC_EXPRESSIONTRAY			5002

class mxTab;
class mxChoice;
class mxCheckBox;
class mxSlider;
class mxLineEdit;
class mxLabel;
class mxButton;
class MatSysWindow;
class TextureWindow;
class mxExpressionTray;
class FlexPanel;
class PhonemeEditor;
class mxExpressionTab;
class mxExpressionSlider;
class ExpressionTool;
class CChoreoView;


class ControlPanel : public mxWindow, public IFacePoserToolWindow
{
	typedef mxWindow BaseClass;

	mxTab *tab;
	mxChoice *cRenderMode;
	mxCheckBox *cbGround, *cbMovement, *cbBackground;
	mxChoice *cSequence;
	mxSlider *slSpeedScale;
	mxLabel *lSpeedScale;
	mxSlider *slPrimaryBlend, *slSecondaryBlend;
	mxChoice *cBodypart, *cController, *cSubmodel;
	mxSlider *slController;
	mxChoice *cSkin;
	mxLabel *lModelInfo1, *lModelInfo2;

	mxLineEdit *leMeshScale, *leBoneScale;
	mxSlider *slModelGap;

	mxCheckBox *cbAllWindowsDriveSpeech;
	
public:
	// CREATORS
	ControlPanel (mxWindow *parent);
	virtual ~ControlPanel ();

	// MANIPULATORS

	virtual int handleEvent (mxEvent *event);
	virtual void redraw();

	virtual void		OnDelete();
	virtual bool		CanClose();

	virtual void		Think( float dt );

	void dumpModelInfo ();
	void OnModelChanged( const char *filename );

	void setRenderMode (int mode);
	void setShowGround (bool b);
	void setShowMovement (bool b);
	void setShowBackground (bool b);
	void setHighlightBone( int index );

	void initSequences ();
	void setSequence (int index);

	void setSpeed( float value );

	void initPoseParameters ();
	void setBlend(int index, float value );

	void initBodyparts ();
	void setBodypart (int index);
	void setSubmodel (int index);

	void initBoneControllers ();
	void setBoneController (int index);
	void setBoneControllerValue (int index, float value);

	void initSkins ();

	void setModelInfo ();

	void initTextures ();

	void centerView ();

	void fullscreen ();

	void CenterOnFace( void );

	void PositionControls( int width, int height );

	bool CloseClass( int classindex );

	bool Close();
	bool Closeall();

	void Copy( void );
	void Paste( void );

	void Undo( void );
	void Redo( void );

	void UndoExpression( int index );
	void RedoExpression( int index );

	void DeleteExpression( int index );
	void DeleteExpressionOverride( int index );

	float GetModelGap( void );

	bool	AllToolsDriveSpeech( void );

};

extern ControlPanel		*g_pControlPanel;

#endif // INCLUDED_CONTROLPANEL