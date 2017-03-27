//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           FlexPanel.h
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
#ifndef INCLUDED_FLEXPANEL
#define INCLUDED_FLEXPANEL



#ifndef INCLUDED_MXWINDOW
#include <mx/mxWindow.h>
#endif

#define IDC_FLEX					7001
#define IDC_FLEXSCALE				7101
#define IDC_FLEXSCROLL				7201

#define IDC_EXPRESSIONRESET			7202

#include "studio.h"

class mxTab;
class mxChoice;
class mxCheckBox;
class mxSlider;
class mxScrollbar;
class mxLineEdit;
class mxLabel;
class mxButton;
class MatSysWindow;
class TextureWindow;
class mxExpressionSlider;


#include "expressions.h"
#include "faceposertoolwindow.h"

/*
	int nameindex;
	int numkeys;
	int keyindex;
	{ char key, char weight }
*/

class ControlPanel;

class FlexPanel : public mxWindow, public IFacePoserToolWindow
{
	typedef mxWindow BaseClass;

	mxExpressionSlider *slFlexScale[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];

	mxScrollbar *slScrollbar;

	mxButton	*btnResetSliders;

public:
	// CREATORS
	FlexPanel (mxWindow *parent);
	virtual ~FlexPanel ();

	virtual void redraw();
	virtual bool PaintBackground( void );

	// MANIPULATORS
	int handleEvent (mxEvent *event);
	
	void initFlexes ();
	float	GetSlider( int iFlexController );
	float	GetSliderRawValue( int iFlexController );
	void	GetSliderRange( int iFlexController, float& minvalue, float& maxvalue );

	void	SetSlider( int iFlexController, float value );
	float	GetInfluence( int iFlexController );
	void	SetInfluence( int iFlexController, float value );
	int		LookupFlex( int iSlider, int barnum );
	int		LookupPairedFlex( int iFlexController );

	int		nFlexSliderIndex[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];
	int		nFlexSliderBarnum[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];


	void PositionSliders( int sboffset );
	void PositionControls( int width, int height );

	void EditExpression( void );
	void NewExpression( void );

	void setExpression( int index );
	void DeleteExpression( int index );
	void SaveExpression( int index );
	void SaveExpressionOverride( int index );
	void RevertExpression( int index );

	void CopyControllerSettings( void );
	void PasteControllerSettings( void );

	void ResetSliders( bool preserveundo );

	void CopyControllerSettingsToStructure( CExpression *exp );

private:
	bool			m_bNewExpressionMode;

	// Since we combine left/right into one, this will be less than hdr->numflexcontrollers
	int				m_nViewableFlexControllerCount;
};

extern FlexPanel		*g_pFlexPanel;

#endif // INCLUDED_FLEXPANEL