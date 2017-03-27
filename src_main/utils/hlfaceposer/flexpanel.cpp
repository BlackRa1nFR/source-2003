//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           FlexPanel.cpp
// last modified:  May 29 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
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
#include "FlexPanel.h"
#include "ViewerSettings.h"
#include "StudioModel.h"
#include "MatSysWin.h"
#include "ControlPanel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mx/mx.h>
#include <mx/mxBmp.h>

#include "mxbitmapwindow.h"
#include "mxExpressionTray.h"
#include "expressions.h"
#include "cmdlib.h"
#include "mdlviewer.h"
#include "hlfaceposer.h"
#include "ExpressionProperties.h"
#include "expclass.h"
#include "choreowidgetdrawhelper.h"
#include "mxExpressionSlider.h"
#include "faceposer_models.h"

extern char g_appTitle[];

#define LINE_HEIGHT 20

FlexPanel		*g_pFlexPanel = 0;

void FlexPanel::PositionControls( int width, int height )
{
	int buttonwidth = 80;
	int buttonx = 3;
	int row = height - 18;
	int buttonheight = 18;

	btnResetSliders->setBounds( buttonx, row, buttonwidth, buttonheight );
}

FlexPanel::FlexPanel (mxWindow *parent)
: IFacePoserToolWindow( "FlexPanel", "Flex Sliders" ), mxWindow( parent, 0, 0, 0, 0 )
{
	m_nViewableFlexControllerCount = 0;

	m_bNewExpressionMode = true;

	btnResetSliders = new mxButton( this, 0, 0, 100, 20, "Zero Sliders", IDC_EXPRESSIONRESET );

	mxWindow *wFlex = this;
	for (int i = 0; i < GLOBAL_STUDIO_FLEX_CONTROL_COUNT; i++)
	{
		int w = 5; // (i / 4) * 156 + 5;
		int h = i * LINE_HEIGHT + 5; // (i % 4) * 20 + 5;

		slFlexScale[i] = new mxExpressionSlider (wFlex, w, h, 220, LINE_HEIGHT, IDC_FLEXSCALE + i);
	}

	slScrollbar = new mxScrollbar( wFlex, 0, 0, 18, 100, IDC_FLEXSCROLL, mxScrollbar::Vertical );
	slScrollbar->setRange( 0, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * LINE_HEIGHT );
	slScrollbar->setPagesize( 100 );
}



FlexPanel::~FlexPanel ()
{
}

void FlexPanel::redraw()
{
	if ( !ToolCanDraw() )
		return;

	CChoreoWidgetDrawHelper helper( this, GetSysColor( COLOR_BTNFACE ) );
	HandleToolRedraw( helper );

	BaseClass::redraw();
}

void FlexPanel::PositionSliders( int sboffset )
{
	int reservedheight = GetCaptionHeight() + 5 /*gap at top*/ + 1 * 20 /* space for buttons/edit controls*/;

	int widthofslidercolumn = slFlexScale[ 0 ]->w() + 10;

	int colsavailable = ( this->w2() - 20 /*scrollbar*/ - 10 /*left edge gap + right gap*/ ) / widthofslidercolumn;
	// Need at least one column
	colsavailable = max( colsavailable, 1 );

	int rowsneeded = GLOBAL_STUDIO_FLEX_CONTROL_COUNT;

	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( hdr )
	{
		rowsneeded = m_nViewableFlexControllerCount;
	}

	int rowsvisible = ( this->h2() - reservedheight ) / LINE_HEIGHT;

	int rowspercol = rowsvisible;

	if ( rowsvisible * colsavailable < rowsneeded )
	{
		// Figure out how many controls should go in each available column
		rowspercol = (rowsneeded + (colsavailable - 1)) / colsavailable;

		slScrollbar->setPagesize( rowsvisible * LINE_HEIGHT );
		slScrollbar->setRange( 0, rowspercol * LINE_HEIGHT );
	}

	int row = 0;
	int col = 0;
	for (int i = 0; i < GLOBAL_STUDIO_FLEX_CONTROL_COUNT; i++)
	{
		int x = 5 + col * widthofslidercolumn;
		int y = row * LINE_HEIGHT + 5 + GetCaptionHeight() - sboffset; // (i % 4) * 20 + 5;

		slFlexScale[ i ]->setBounds( x, y, slFlexScale[i]->w(), slFlexScale[i]->h() );

		if ( i >= rowsneeded || 
			( y + LINE_HEIGHT - 5 > ( this->h2() - reservedheight ) ) )
		{
			slFlexScale[ i ]->setVisible( false );
		}
		else
		{
			slFlexScale[ i ]->setVisible( true );
		}

		row++;
		if ( row >= rowspercol )
		{
			col++;
			row = 0;
		}
	}
}

int FlexPanel::handleEvent (mxEvent *event)
{
	int iret = 0;

	if ( HandleToolEvent( event ) )
	{
		return iret;
	}

	switch ( event->event )
	{
	case mxEvent::Size:
		{
			int trueh = h2() - GetCaptionHeight();
			PositionControls( w2(), h2() );
			slScrollbar->setPagesize( trueh );
			slScrollbar->setBounds( w2() - 18, GetCaptionHeight(), 18, trueh );
			PositionSliders( 0 );
			iret = 1;
		}
		break;
	case mxEvent::Action:
		{
			iret = 1;
			switch ( event->action )
			{
			default:
				iret = 0;
				break;
			case IDC_FLEXSCROLL:
				{
					if ( event->event == mxEvent::Action &&
						event->modifiers == SB_THUMBTRACK)
					{
						int offset = event->height; // ((mxScrollbar *) event->widget)->getValue( );
						
						slScrollbar->setValue( offset ); // if (offset > slScrollbar->getPagesize()
						
						PositionSliders( offset );

						IFacePoserToolWindow::SetActiveTool( this );
					}
				}
				break;
			case IDC_EXPRESSIONRESET:
				{
					ResetSliders( true );
					IFacePoserToolWindow::SetActiveTool( this );
				}
				break;
			}

			if ( event->action >= IDC_FLEXSCALE && event->action < IDC_FLEXSCALE + GLOBAL_STUDIO_FLEX_CONTROL_COUNT)
			{
				iret = 1;

				bool pushundo = false;
				
				mxExpressionSlider *slider = ( mxExpressionSlider * )event->widget;
				int barnumber = event->height;
				int slidernum = ( event->action - IDC_FLEXSCALE );
				
				float value = slider->getValue ( barnumber );
				float influ = slider->getInfluence( );
				
				switch( event->modifiers )
				{
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					break;
				case SB_ENDSCROLL:
					pushundo = true;
					break;
				}
				int flex = LookupFlex( slidernum, barnumber );
				int flex2 = LookupPairedFlex( flex );
				float value2 = GetSlider( flex2 );
				
				CExpClass *active = expressions->GetActiveClass();
				if ( active )
				{
					int index = active->GetSelectedExpression();
					if ( pushundo && index != -1 )
					{
						CExpression *exp = active->GetExpression( index );
						if ( exp )
						{
							float *settings = exp->GetSettings();
							float *weights = exp->GetWeights();
							Assert( settings );	
							
							if ( settings[ flex ] != value || 
								 settings[ flex2 ] != value2 || 
								 weights[ flex ] != influ )
							{
								exp->PushUndoInformation();
								
								active->SetDirty( true );
								
								settings[ flex ] = value;
								settings[ flex2 ] = value2;
								weights[ flex ] = influ;
								weights[ flex2 ] = influ;
								
								exp->PushRedoInformation();
								
								g_pExpressionTrayTool->redraw();
							}
						}
					}
				}
				
				// Update the face
				models->GetActiveStudioModel()->SetFlexController( flex, value * influ );
				if (flex2 != flex)
					models->GetActiveStudioModel()->SetFlexController( flex2, value2 * influ );

				g_viewerSettings.solveHeadTurn = 1;
				IFacePoserToolWindow::SetActiveTool( this );
			}
		}
	}
	
	return iret;
}

void FlexPanel::initFlexes ()
{
	m_nViewableFlexControllerCount = 0;

	memset( nFlexSliderIndex, 0, sizeof( nFlexSliderIndex ) );
	memset( nFlexSliderBarnum, 0, sizeof( nFlexSliderBarnum ) );

	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if (hdr)
	{
		for (int j = 0; j < GLOBAL_STUDIO_FLEX_CONTROL_COUNT; j++)
		{
			slFlexScale[j]->setVisible( false );
			slFlexScale[j]->setLabel( "" );
		}

		// J is the slider number we're filling in
		j = 0;

		for ( int k = 0; k < hdr->numflexcontrollers; k++ )
		{
			// Lookup global flex controller index
			int controller = hdr->pFlexcontroller( k )->link;
			Assert( controller != -1 );

			//Con_Printf( "%i Setting up %s global %i\n", k, hdr->pFlexcontroller(k)->pszName(), controller );

			slFlexScale[j]->setLabel( hdr->pFlexcontroller(k)->pszName() );

			if ( nFlexSliderIndex[controller] == 0 )
			{
				nFlexSliderIndex[controller] = j;
				nFlexSliderBarnum[controller] = 0;
			}

			if (hdr->pFlexcontroller(k)->min != hdr->pFlexcontroller(k)->max)
				slFlexScale[j]->setRange( 0, hdr->pFlexcontroller(k)->min, hdr->pFlexcontroller(k)->max );

			if (strncmp( "right_", hdr->pFlexcontroller(k)->pszName(), 6 ) == 0)
			{
				if (hdr->pFlexcontroller(k)->min != hdr->pFlexcontroller(k)->max)
					slFlexScale[j]->setRange( 1, hdr->pFlexcontroller(k+1)->min, hdr->pFlexcontroller(k+1)->max );
				slFlexScale[j]->setLabel( &hdr->pFlexcontroller(k)->pszName()[6] );

				slFlexScale[j]->SetMode( true );
				k++;
				controller = hdr->pFlexcontroller( k )->link;
				Assert( controller != -1 );
				if ( nFlexSliderIndex[controller] == 0 )
				{
					nFlexSliderIndex[controller] = j;
					nFlexSliderBarnum[controller] = 1;
				}
			}
			m_nViewableFlexControllerCount++;

			slFlexScale[j]->setVisible( true );
			slFlexScale[j]->redraw();

			j++;
		}
	}

	slScrollbar->setRange( 0, m_nViewableFlexControllerCount * LINE_HEIGHT + 5 );

	int trueh = h2() - GetCaptionHeight();
	PositionControls( w2(), h2() );
	slScrollbar->setPagesize( trueh );
	slScrollbar->setBounds( w2() - 18, GetCaptionHeight(), 18, trueh );
	PositionSliders( 0 );
}


float	
FlexPanel::GetSlider( int iFlexController )
{
	return slFlexScale[ nFlexSliderIndex[ iFlexController ] ]->getValue( nFlexSliderBarnum[ iFlexController ] );
}

float FlexPanel::GetSliderRawValue( int iFlexController )
{
	return slFlexScale[ nFlexSliderIndex[ iFlexController ] ]->getRawValue( nFlexSliderBarnum[ iFlexController ] );
}

void FlexPanel::GetSliderRange( int iFlexController, float& minvalue, float& maxvalue )
{
	int barnum = nFlexSliderBarnum[ iFlexController ];

	mxExpressionSlider *sl = slFlexScale[ nFlexSliderIndex[ iFlexController ] ]; 
	Assert( sl );
	minvalue = sl->getMinValue( barnum );
	maxvalue = sl->getMaxValue( barnum );
}

void
FlexPanel::SetSlider( int iFlexController, float value )
{
	slFlexScale[ nFlexSliderIndex[ iFlexController ] ]->setValue( nFlexSliderBarnum[ iFlexController ], value );
}

float	
FlexPanel::GetInfluence( int iFlexController )
{
	return slFlexScale[ nFlexSliderIndex[ iFlexController ] ]->getInfluence( );
}

void
FlexPanel::SetInfluence( int iFlexController, float value )
{
	// Con_Printf( "SetInfluence( %d, %.0f ) : %d %d\n", iFlexController, value, nFlexSliderIndex[ iFlexController ], nFlexSliderBarnum[ iFlexController ] );
	if ( nFlexSliderBarnum[ iFlexController ] == 0)
	{
		slFlexScale[ nFlexSliderIndex[ iFlexController ] ]->setInfluence( value );
	}
}

char const *GetGlobalFlexControllerName( int index );

int
FlexPanel::LookupFlex( int iSlider, int barnum )
{
	for (int i = 0; i < GLOBAL_STUDIO_FLEX_CONTROL_COUNT; i++)
	{
		if (nFlexSliderIndex[i] == iSlider && nFlexSliderBarnum[i] == barnum)
		{
			//char const *name = GetGlobalFlexControllerName( i );
			//Con_Printf( "lookup slider %i bar %i == %s\n",
				//iSlider, barnum, name );

			return i;
		}
	}

	Con_Printf( "lookup slider %i bar %i failed\n",
		iSlider, barnum);
	return 0;
}


int
FlexPanel::LookupPairedFlex( int iFlexController )
{
	if (nFlexSliderBarnum[ iFlexController ] == 1)
	{
		return iFlexController - 1;
	}
	else if (nFlexSliderIndex[ iFlexController + 1 ] == nFlexSliderIndex[ iFlexController ])
	{
		return iFlexController + 1;
	}
	return iFlexController;
}

void
FlexPanel::setExpression( int index )
{
	if ( !models->GetActiveStudioModel() )
		return;

	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
		return;

	CExpClass *active = expressions->GetActiveClass();
	if ( !active )
		return;

	CExpression *exp = active->GetExpression( index );
	if ( !exp )
		return;

	// Con_Printf( "Setting expression to %i:'%s'\n", index, exp->name );

	if ( FacePoser_GetOverridesShowing() )
	{
		if ( exp->HasOverride() && exp->GetOverride() )
		{
			exp = exp->GetOverride();
		}
	}

	float *settings = exp->GetSettings();
	float *weights = exp->GetWeights();
	Assert( settings );	
	Assert( weights );	

	for (int i = 0; i < hdr->numflexcontrollers; i++)
	{
		int j = hdr->pFlexcontroller( i )->link;
		if ( j == -1 )
			continue;

		SetSlider( j, settings[j] );
		SetInfluence( j, weights[j] );
		models->GetActiveStudioModel()->SetFlexController( i, settings[j] * weights[j] );
	}
}

void FlexPanel::DeleteExpression( int index )
{
	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
		return;
	CExpClass *active = expressions->GetActiveClass();
	if ( !active )
		return;

	CExpression *exp = active->GetExpression( index );
	if ( !exp )
		return;

	active->DeleteExpression( exp->name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void FlexPanel::RevertExpression( int index )
{
	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
		return;

	CExpClass *active = expressions->GetActiveClass();
	if ( !active )
		return;

	CExpression *exp = active->GetExpression( index );
	if ( !exp )
		return;

	exp->Revert();
	setExpression( index );
	g_pExpressionTrayTool->redraw();
}

void FlexPanel::SaveExpressionOverride( int index )
{
	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
		return;

	CExpClass *active = expressions->GetActiveClass();
	if ( !active )
		return;

	CExpression *exp = active->GetExpression( index );
	if ( !exp )
		return;

	// Found it in the base, now create the override if it doesn't exist
	//
	CExpClass *overrides = expressions->FindOverrideClass( active->GetName() );
	if ( !overrides )
	{
		// create one
		overrides = expressions->AddOverrideClass( active->GetName() );
		if( overrides )
		{
			overrides->SetDirty( true );
			active->SetOverrideClass( overrides );
			overrides->SetIsOverrideClass( true );
		}
	}

	if ( !overrides )
		return;

	CExpression *oe = overrides->FindExpression( exp->name );
	if ( !oe )
	{
		oe = overrides->AddExpression( exp->name, exp->description, exp->GetSettings(), exp->GetWeights(), false );
	}

	if ( !oe )
		return;
	
	float *settings = oe->GetSettings();
	float *weights = oe->GetWeights();
	Assert( settings );	
	Assert( weights );	
	for ( int i = 0; i < hdr->numflexcontrollers; i++ )
	{
		int j = hdr->pFlexcontroller( i )->link;

		settings[ j ] = GetSlider( j );
		weights[ j ] = GetInfluence( j );
	}

	exp->Revert();

	oe->CreateNewBitmap( models->GetActiveModelIndex() );

	oe->ResetUndo();

	oe->SetDirty( false );

	// So overrides save when we save
	active->SetDirty( true );

	active->ApplyOverrides();

	g_pExpressionTrayTool->redraw();
}

void FlexPanel::SaveExpression( int index )
{
	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
		return;

	CExpClass *active = expressions->GetActiveClass();
	if ( !active )
		return;

	CExpression *exp = active->GetExpression( index );
	if ( !exp )
		return;

	int retval = mxMessageBox( this, "Overwrite existing expression?", g_appTitle, MX_MB_YESNO | MX_MB_QUESTION );
	if ( retval != 0 )
		return;

	float *settings = exp->GetSettings();
	float *weights = exp->GetWeights();
	Assert( settings );	
	Assert( weights );	
	for ( int i = 0; i < hdr->numflexcontrollers; i++ )
	{
		int j = hdr->pFlexcontroller( i )->link;

		settings[ j ] = GetSlider( j );
		weights[ j ] = GetInfluence( j );
	}

	exp->CreateNewBitmap( models->GetActiveModelIndex() );

	exp->ResetUndo();

	exp->SetDirty( false );

	g_pExpressionTrayTool->redraw();
}

void FlexPanel::CopyControllerSettingsToStructure( CExpression *exp )
{
	Assert( exp );

	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( hdr )
	{
		float *settings = exp->GetSettings();
		float *weights = exp->GetWeights();

		for (int i = 0; i < hdr->numflexcontrollers; i++)
		{
			int j = hdr->pFlexcontroller( i )->link;

			settings[ j ] = GetSlider( j );
			weights[ j ] = GetInfluence( j );
		}
	}
}

void FlexPanel::ResetSliders( bool preserveundo )
{
	CExpClass *active = expressions->GetActiveClass();
	if ( !active )
		return;

	bool needredo = false;
	CExpression zeroes;

	CExpression *exp = NULL;
	int index = active->GetSelectedExpression();
	if ( index != -1 )
	{
		exp = active->GetExpression( index );
		if ( exp )
		{
			float *settings = exp->GetSettings();
			Assert( settings );

			if ( memcmp( settings, zeroes.GetSettings(), GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) ) )
			{
				if ( preserveundo )
				{
					exp->PushUndoInformation();
					needredo = true;
				}

				active->SetDirty( true );

				g_pExpressionTrayTool->redraw();
			}
		}
	}

	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( hdr )
	{
		int i;

		if( exp )
		{
			float *settings = exp->GetSettings();
			float *weights = exp->GetWeights();

			Assert( settings && weights );

			for ( i = 0; i < GLOBAL_STUDIO_FLEX_CONTROL_COUNT; i++ )
			{
				settings[ i ] = 0.0f;
				weights[ i ] = 0.0f;
			}
		}

		for ( i = 0; i < hdr->numflexcontrollers; i++ )
		{
			int j = hdr->pFlexcontroller( i )->link;

			if ( j == -1 )
				continue;

			SetSlider( j, 0.0f );
			SetInfluence( j, 0.0f );
			models->GetActiveStudioModel()->SetFlexController( i, 0.0 );
		}
	}

	if ( exp && needredo && preserveundo )
	{
		exp->PushRedoInformation();
	}
}

void FlexPanel::CopyControllerSettings( void )
{
	CExpression *exp = expressions->GetCopyBuffer();
	memset( exp, 0, sizeof( *exp ) );
	CopyControllerSettingsToStructure( exp );
}

void FlexPanel::PasteControllerSettings( void )
{
	CExpClass *active = expressions->GetActiveClass();
	if ( !active )
		return;

	bool needredo = false;
	CExpression *paste = expressions->GetCopyBuffer();
	if ( !paste )
		return;

	CExpression *exp = NULL;
	int index = active->GetSelectedExpression();
	if ( index != -1 )
	{
		exp = active->GetExpression( index );
		if ( exp )
		{
			float *settings = exp->GetSettings();
			Assert( settings );

			// UPDATEME
			if ( memcmp( settings, paste->GetSettings(), GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) ) )
			{
				exp->PushUndoInformation();
				needredo = true;

				active->SetDirty( true );

				g_pExpressionTrayTool->redraw();
			}
		}
	}

	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( hdr )
	{
		float *settings = paste->GetSettings();
		float *weights = paste->GetWeights();
		Assert( settings );
		Assert( weights );

		for (int i = 0; i < hdr->numflexcontrollers; i++)
		{
			int j = hdr->pFlexcontroller( i )->link;

			SetSlider( j, settings[j] );
			SetInfluence( j, weights[j] );
			models->GetActiveStudioModel()->SetFlexController( i, settings[j] * weights[j] );
		}
	}

	if ( exp && needredo )
	{
		exp->PushRedoInformation();
	}

}

void FlexPanel::EditExpression( void )
{
	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Can't edit face pose, must load a model first!\n" );
		return;
	}
	
	CExpClass *active = expressions->GetActiveClass();
	if ( !active )
		return;

	int index = active->GetSelectedExpression();
	if ( index == -1 )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Can't edit face pose, must select a face from list first!\n" );
		return;
	}

	CExpression *exp = active->GetExpression( index );
	if ( !exp )
	{
		return;
	}

	bool namechanged = false;
	CExpressionParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Edit Expression" );
	strcpy( params.m_szName, exp->name );
	strcpy( params.m_szDescription, exp->description );

	if ( !ExpressionProperties( &params ) )
		return;

	namechanged = stricmp( exp->name, params.m_szName ) ? true : false;

	if ( ( strlen( params.m_szName ) <= 0 ) ||
		!stricmp( params.m_szName, "unnamed" ) )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "You must type in a valid name\n" );
		return;
	}

	if ( ( strlen( params.m_szDescription ) <= 0 ) ||
   	   !stricmp( params.m_szDescription, "description" ) )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "You must type in a valid description\n" );
		return;
	}

	if ( namechanged )
	{
		Con_Printf( "Deleting old bitmap %s\n", exp->GetBitmapFilename( models->GetActiveModelIndex() ) );

		// Remove old bitmap
		_unlink( exp->GetBitmapFilename( models->GetActiveModelIndex() ) );
	}

	strcpy( exp->name, params.m_szName );
	strcpy( exp->description, params.m_szDescription );

	if ( namechanged )
	{
		exp->CreateNewBitmap( models->GetActiveModelIndex() );
	}

	active->SetDirty( true );

	g_pExpressionTrayTool->redraw();
}

void FlexPanel::NewExpression( void )
{
	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Can't create new face pose, must load a model first!\n" );
		return;
	}

	CExpClass *active = expressions->GetActiveClass();
	if ( !active )
		return;

	g_pExpressionTrayTool->Deselect();

	CExpressionParams params;
	memset( &params, 0, sizeof( params ) );

	strcpy( params.m_szDialogTitle, "Add Expression" );
	strcpy( params.m_szName, "" );
	strcpy( params.m_szDescription, "" );

	if ( !ExpressionProperties( &params ) )
		return;

	if ( ( strlen( params.m_szName ) <= 0 ) ||
		!stricmp( params.m_szName, "unnamed" ) )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "You must type in a valid name\n" );
		return;
	}

	if ( ( strlen( params.m_szDescription ) <= 0 ) ||
   	   !stricmp( params.m_szDescription, "description" ) )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "You must type in a valid description\n" );
		return;
	}

	float settings[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];
	float weights[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];
	memset( settings, 0, sizeof( settings ) );
	memset( weights, 0, sizeof( settings ) );
	for ( int i = 0; i < hdr->numflexcontrollers; i++ )
	{
		int j = hdr->pFlexcontroller( i )->link;

		settings[ j ] = GetSlider( j );
		weights[ j ] = GetInfluence( j );
	}	

	active->AddExpression( params.m_szName, params.m_szDescription, settings, weights, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool FlexPanel::PaintBackground( void )
{
	redraw();
	return false;
}