#include <stdio.h>
#include <math.h>
#include "hlfaceposer.h"
#include "PhonemeEditor.h"
#include "PhonemeEditorColors.h"
#include "snd_audio_source.h"
#include "snd_wave_source.h"
#include "ifaceposersound.h"
#include "choreowidgetdrawhelper.h"
#include "mxBitmapButton.h"
#include "phonemeproperties.h"
#include "riff.h"
#include "cmdlib.h"
#include "scriplib.h"
#include "StudioModel.h"
#include "expressions.h"
#include "expclass.h"
#include "InputProperties.h"
#include "PhonemeExtractor.h"
#include "PhonemeConverter.h"
#include "choreoevent.h"
#include "choreoscene.h"
#include "ChoreoView.h"
#include "FileSystem.h"
#include "UtlBuffer.h"
#include "AudioWaveOutput.h"
#include "StudioModel.h"
#include "viewerSettings.h"
#include "ControlPanel.h"
#include "faceposer_models.h"
#include "vstdlib/strtools.h"
#include "tabwindow.h"
#include "MatSysWin.h"
#include "iclosecaptionmanager.h"
#include "EditPhrase.h"

#define PLENTY_OF_TIME 99999.9
#define MINIMUM_PHRASE_GAP 0.02f
#define DEFAULT_PHRASE_LENGTH 1.0f

// Close captioning
void PhonemeEditor::CloseCaption_EditInsertFirstPhrase( void )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	if ( m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ) != 0 )
	{
		Con_Printf( "Can't insert first phrase into sentence, already has data\n" );
		return;
	}

	CEditPhraseParams params;
	memset( &params, 0, sizeof( params ) );
	strcpy( params.m_szDialogTitle, "Edit Phrase" );
	strcpy( params.m_szPrompt, "Current Phrase:" );
	ConvertANSIToUnicode( m_Tags.GetText(), params.m_szInputText, sizeof( params.m_szInputText ) );

	params.m_nLeft = -1;
	params.m_nTop = -1;

	params.m_bPositionDialog = false;

	if ( !EditPhrase( &params ) )
	{
		SetFocus( (HWND)getHandle() );
		return;
	}

	if ( wcslen( params.m_szInputText ) <= 0 )
	{
		return;
	}

	float start, end;
	m_Tags.GetEstimatedTimes( start, end );

	SetDirty( true );

	PushUndo();

	CCloseCaptionPhrase *phrase = new CCloseCaptionPhrase( params.m_szInputText );
	phrase->SetSelected( true );
	phrase->SetStartTime( start );
	phrase->SetEndTime( end );

	m_Tags.AddCloseCaptionPhrase( CC_ENGLISH, phrase );

	PushRedo();

	// Add it
	redraw();
}

void PhonemeEditor::CloseCaption_EditPhrase( CCloseCaptionPhrase *phrase )
{
	CEditPhraseParams params;
	memset( &params, 0, sizeof( params ) );
	strcpy( params.m_szDialogTitle, "Edit Phrase" );
	strcpy( params.m_szPrompt, "Current Phrase:" );
	wcscpy( params.m_szInputText, phrase->GetStream() );

	params.m_nLeft = -1;
	params.m_nTop = -1;

	params.m_bPositionDialog = true;

	if ( params.m_bPositionDialog )
	{
		RECT rcPhrase;
		CloseCaption_GetPhraseRect( phrase, rcPhrase );

		// Convert to screen coords
		POINT pt;
		pt.x = rcPhrase.left;
		pt.y = rcPhrase.top;

		ClientToScreen( (HWND)getHandle(), &pt );

		params.m_nLeft	= pt.x;
		params.m_nTop	= pt.y;
	}

	if ( !EditPhrase( &params ) )
	{
		SetFocus( (HWND)getHandle() );
		return;
	}

	SetFocus( (HWND)getHandle() );

	SetDirty( true );

	PushUndo();

	phrase->SetStream( params.m_szInputText );

	PushRedo();

	redraw();
}

void PhonemeEditor::CloseCaption_EditPhrase( void )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	CCloseCaptionPhrase *pPhrase = CloseCaption_GetClickedPhrase();
	if ( !pPhrase )
		return;

	CloseCaption_EditPhrase( pPhrase );
}

void PhonemeEditor::CloseCaption_EditInsertBefore( void )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	CCloseCaptionPhrase *phrase = CloseCaption_GetSelectedPhrase();
	if ( !phrase )
		return;

	float gap = CloseCaption_GetTimeGapToNextPhrase( false, phrase );
	if ( gap < MINIMUM_PHRASE_GAP )
	{
		Con_Printf( "Can't insert before, gap of %.2f ms is too small\n", 1000.0f * gap );
		return;
	}

	// Don't have really long phrases
	gap = min( gap, DEFAULT_PHRASE_LENGTH );

	int clicked = CloseCaption_IndexOfPhrase( phrase );
	Assert( clicked >= 0 );

	CEditPhraseParams params;
	memset( &params, 0, sizeof( params ) );
	strcpy( params.m_szDialogTitle, "Insert Phrase" );
	strcpy( params.m_szPrompt, "Phrase:" );
	wcscpy( params.m_szInputText, L"" );

	params.m_nLeft = -1;
	params.m_nTop = -1;

	params.m_bPositionDialog = true;
	if ( params.m_bPositionDialog )
	{
		RECT rcPhrase;
		CloseCaption_GetPhraseRect( phrase, rcPhrase );

		// Convert to screen coords
		POINT pt;
		pt.x = rcPhrase.left;
		pt.y = rcPhrase.top;

		ClientToScreen( (HWND)getHandle(), &pt );

		params.m_nLeft	= pt.x;
		params.m_nTop	= pt.y;
	}

	int iret = EditPhrase( &params );
	SetFocus( (HWND)getHandle() );
	if ( !iret )
	{
		return;
	}

	if ( wcslen( params.m_szInputText ) <= 0 )
	{
		return;
	}

	SetDirty( true );

	PushUndo();

	CCloseCaptionPhrase *newphrase = new CCloseCaptionPhrase( params.m_szInputText );
	newphrase->SetEndTime( phrase->GetStartTime() );
	newphrase->SetStartTime( phrase->GetStartTime() - gap );
	newphrase->SetSelected( true );
	phrase->SetSelected( false );

	m_Tags.m_CloseCaption[ CC_ENGLISH ].InsertBefore( clicked, newphrase );

	PushRedo();

	// Add it
	redraw();
}

void PhonemeEditor::CloseCaption_EditInsertAfter( void )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	CCloseCaptionPhrase *phrase = CloseCaption_GetSelectedPhrase();
	if ( !phrase )
		return;

	float gap = CloseCaption_GetTimeGapToNextPhrase( true, phrase );
	if ( gap < MINIMUM_PHRASE_GAP )
	{
		Con_Printf( "Can't insert after, gap of %.2f ms is too small\n", 1000.0f * gap );
		return;
	}

	// Don't have really long phrases
	gap = min( gap, DEFAULT_PHRASE_LENGTH );

	int clicked = CloseCaption_IndexOfPhrase( phrase );
	Assert( clicked >= 0 );

	CEditPhraseParams params;
	memset( &params, 0, sizeof( params ) );
	strcpy( params.m_szDialogTitle, "Insert Phrase" );
	strcpy( params.m_szPrompt, "Phrase:" );
	wcscpy( params.m_szInputText, L"" );

	params.m_nLeft = -1;
	params.m_nTop = -1;

	params.m_bPositionDialog = true;
	if ( params.m_bPositionDialog )
	{
		RECT rcPhrase;
		CloseCaption_GetPhraseRect( phrase, rcPhrase );

		// Convert to screen coords
		POINT pt;
		pt.x = rcPhrase.left;
		pt.y = rcPhrase.top;

		ClientToScreen( (HWND)getHandle(), &pt );

		params.m_nLeft	= pt.x;
		params.m_nTop	= pt.y;
	}

	int iret = EditPhrase( &params );
	SetFocus( (HWND)getHandle() );
	if ( !iret )
	{
		return;
	}

	if ( wcslen( params.m_szInputText ) <= 0 )
	{
		return;
	}

	SetDirty( true );

	PushUndo();

	CCloseCaptionPhrase *newphrase = new CCloseCaptionPhrase( params.m_szInputText );
	newphrase->SetEndTime( phrase->GetEndTime() + gap );
	newphrase->SetStartTime( phrase->GetEndTime() );
	newphrase->SetSelected( true );
	phrase->SetSelected( false );

	m_Tags.m_CloseCaption[ CC_ENGLISH ].InsertAfter( clicked, newphrase );

	PushRedo();

	// Add it
	redraw();
}

void PhonemeEditor::CloseCaption_EditDelete( void )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	CountSelected();

	if ( m_nSelectedPhraseCount < 1 )
		return;

	SetDirty( true );

	PushUndo();

	for ( int i = m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH )- 1; i >= 0; i-- )
	{
		CCloseCaptionPhrase *phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		if ( !phrase || !phrase->GetSelected() )
			continue;

		m_Tags.RemoveCloseCaptionPhrase( CC_ENGLISH, i );
	}

	PushRedo();

	redraw();
}

void PhonemeEditor::CloseCaption_Select( bool forward )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	CountSelected();

	if ( m_nSelectedPhraseCount != 1 )
		return;

	// Figure out it's phrase and index
	CCloseCaptionPhrase *phrase = CloseCaption_GetSelectedPhrase();
	if ( !phrase )
		return;

	int phraseNum = CloseCaption_IndexOfPhrase( phrase );
	if ( phraseNum == -1 )
		return;

	if ( forward )
	{
		phraseNum++;

		for ( ; phraseNum < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); phraseNum++ )
		{
			phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, phraseNum );
			phrase->SetSelected( true );
		}
	}
	else
	{
		phraseNum--;

		for ( ; phraseNum >= 0; phraseNum-- )
		{
			phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, phraseNum );
			phrase->SetSelected( true );
		}

	}

	redraw();
}

char const *PhonemeEditor::CloseCaption_StreamToShortName( CCloseCaptionPhrase *phrase )
{
	static char shortname[ 32 ];

	char converted[ 1024 ];
	ConvertUnicodeToANSI( phrase->GetStream(), converted, sizeof( converted ) );

	Q_strncpy( shortname, converted, sizeof( shortname ) );

	if ( strlen( shortname ) > 16 )
	{
		shortname[ 16 ] = 0;
		strcat( shortname, "..." );
	}
	return shortname;
}

void PhonemeEditor::CloseCaption_ShowMenu( CCloseCaptionPhrase *phrase, int mx, int my, mxPopupMenu *pop )
{
	CountSelected();

	bool showmenu = false;
	if ( !pop )
	{
		pop = new mxPopupMenu();
		showmenu = true;
	}
	Assert( pop );

	if ( m_nSelectedPhraseCount <= 0 )
	{
		if ( m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ) == 0 )
		{
			pop->add( va( "Add phrase" ), IDC_EDIT_CC_ADDFIRSTPHRASE );
			pop->add( "Create default", IDC_EDIT_CC_DEFAULT_PHRASE );
		}
	}
	else
	{
		pop->add( va( "Delete %s", m_nSelectedPhraseCount > 1 ? "phrases" : va( "'%s'", CloseCaption_StreamToShortName( phrase ) ) ), IDC_EDIT_CC_DELETEPHRASE );

		if ( m_nSelectedPhraseCount == 1 )
		{
			int index = CloseCaption_IndexOfPhrase( phrase );
			bool valid = false;
			if ( index != -1 )
			{
				int token = CloseCaption_GetPhraseTokenUnderMouse( phrase, mx );

				CloseCaption_SetClickedPhrase( index, token );
				valid = true;
			}

			if ( valid )
			{
				pop->add( va( "Edit phrase '%s'...", CloseCaption_StreamToShortName( phrase ) ), IDC_EDIT_CC_PHRASE );

				float nextGap = CloseCaption_GetTimeGapToNextPhrase( true, phrase );
				float prevGap = CloseCaption_GetTimeGapToNextPhrase( false, phrase );

				if ( nextGap > MINIMUM_PHRASE_GAP ||
					 prevGap > MINIMUM_PHRASE_GAP )
				{
					pop->addSeparator();
					if ( prevGap > MINIMUM_PHRASE_GAP )
					{
						pop->add( va( "Insert phrase before '%s'...", CloseCaption_StreamToShortName( phrase ) ), IDC_EDIT_CC_INSERTPHRASEBEFORE );
					}
					if ( nextGap > MINIMUM_PHRASE_GAP )
					{
						pop->add( va( "Insert phrase after '%s'...", CloseCaption_StreamToShortName( phrase ) ), IDC_EDIT_CC_INSERTPHRASEAFTER );
					}
				}

				pop->addSeparator();
				pop->add( va( "Select all phrases after '%s'", CloseCaption_StreamToShortName( phrase ) ), IDC_SELECT_CC_PHRASESRIGHT );
				pop->add( va( "Select all phrases before '%s'", CloseCaption_StreamToShortName( phrase ) ), IDC_SELECT_CC_PHRASESLEFT );
				int count = phrase->CountTokens();
				int clicked = CloseCaption_GetClickedPhraseToken();
				if ( clicked >= 0 && clicked < count - 1 )
				{
					pop->add( va( "Split phrase '%s' after '%s'", 
						CloseCaption_StreamToShortName( phrase ),
						phrase->GetToken( CloseCaption_GetClickedPhraseToken() ) ), IDC_EDIT_CC_SPLIT_PHRASE );
				}
			}
		}
	}

	if ( CloseCaption_AreSelectedPhrasesContiguous() && m_nSelectedPhraseCount > 1 )
	{
		pop->addSeparator();
		pop->add( va( "Merge phrases" ), IDC_EDIT_CC_MERGE_PHRASES );
		pop->add( va( "Snap phrases" ), IDC_CC_SNAPPHRASES );
		if ( m_nSelectedPhraseCount == 2 )
		{
			pop->add( va( "Separate phrases" ), IDC_CC_SEPARATEPHRASES );
		}
	}

	if ( m_nSelectedPhraseCount > 0 )
	{
		pop->addSeparator();

		pop->add( va( "Deselect all" ), IDC_DESELECT_CC_PHRASES );
	}

	if ( m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH )> 0 )
	{
		pop->addSeparator();
		pop->add( va( "Select all" ), IDC_SELECT_CC_ALLPHRASES );
		pop->add( va( "Cleanup phrases" ), IDC_CC_CLEANUP );
	}

	if ( showmenu )
	{
		pop->popup( this, mx, my );
	}
}

void PhonemeEditor::ShowContextMenu_CloseCaption( int mx, int my )
{
	CountSelected();

	// Construct main
	mxPopupMenu *pop = new mxPopupMenu();

	if ( m_nSelectedPhraseCount > 0 )
	{
		CCloseCaptionPhrase *phrase = CloseCaption_GetSelectedPhrase();
		if ( !phrase )
		{
			// find first one...
			int c = m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH );
			for ( int i = 0; i < c; i++ )
			{
				phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
				if ( phrase->GetSelected() )
					break;
			}

			if ( i >= c )
			{
				phrase = NULL;
			}
		}

		if ( phrase )
		{
			CloseCaption_ShowMenu( phrase, mx, my, pop );
		}
	}
	else
	{
		if ( m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ) > 0 )
		{
			pop->add( va( "Select all" ), IDC_SELECT_CC_ALLPHRASES );
		}
	}

	if ( m_nUndoLevel != 0 || m_nUndoLevel != m_UndoStack.Size()  )
	{
		pop->addSeparator();

		if ( m_nUndoLevel != 0 )
		{
			pop->add( va( "Undo" ), IDC_UNDO );
		}
		if ( m_nUndoLevel != m_UndoStack.Size() )
		{
			pop->add( va( "Redo" ), IDC_REDO );
		}
		pop->add( va( "Clear Undo Info" ), IDC_CLEARUNDO );
	}
	pop->popup( this, mx, my );
}


bool PhonemeEditor::CloseCaption_IsMouseOverRow( int my )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return false;

	RECT rc;

	CloseCaption_GetTrayTopBottom( rc );

	if ( my < rc.top )
		return false;
	
	if ( my > rc.bottom )
		return false;
	
	return true;
}

void PhonemeEditor::CloseCaption_FinishMove( int startx, int endx )
{
	float clicktime	= GetTimeForPixel( startx );
	float endtime	= GetTimeForPixel( endx );

	// Find the phonemes who have the closest start/endtime to the starting click time
	CCloseCaptionPhrase *current, *next;

	if ( !CloseCaption_FindSpanningPhrases( clicktime, &current, &next ) )
	{
		return;
	}

	SetDirty( true );

	PushUndo();

	if ( current && !next )
	{
		// cap movement
		current->SetEndTime( current->GetEndTime() + ( endtime - clicktime ) );
	}
	else if ( !current && next )
	{
		// cap movement
		next->SetStartTime( next->GetStartTime() + ( endtime - clicktime ) );
	}
	else
	{
		// cap movement
		endtime = min( endtime, next->GetEndTime() - 1.0f / GetPixelsPerSecond() );
		endtime = max( endtime, current->GetStartTime() + 1.0f / GetPixelsPerSecond() );

		current->SetEndTime( endtime );
		next->SetStartTime( endtime );
	}

	CloseCaption_CleanupPhrases( false );

	PushRedo();

	redraw();
}

void PhonemeEditor::CloseCaption_FinishDrag( int startx, int endx )
{
	float clicktime	= GetTimeForPixel( startx );
	float endtime	= GetTimeForPixel( endx );

	float dt = endtime - clicktime;

	SetDirty( true );

	PushUndo();

	TraversePhrases( ITER_MoveSelectedPhrases, dt );

	CloseCaption_CleanupPhrases( false );

	PushRedo();

	redraw();
}

int PhonemeEditor::CloseCaption_FindPhraseForTime( float time )
{
	for ( int i = 0; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); i++ )
	{
		CCloseCaptionPhrase *pCurrent = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );

		if ( time < pCurrent->GetStartTime() )
			continue;

		if ( time > pCurrent->GetEndTime() )
			continue;

		return i;
	}

	return -1;
}

void PhonemeEditor::CloseCaption_DeselectPhrases( void )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	for ( int i = 0 ; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); i++ )
	{
		CCloseCaptionPhrase *w = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		Assert( w );
		w->SetSelected( false );
	}
}
void PhonemeEditor::CloseCaption_SnapPhrases( void )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	for ( int i = 0; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH )- 1; i++ )
	{
		CCloseCaptionPhrase *current = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		CCloseCaptionPhrase *next = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i + 1 );

		if ( !current->GetSelected() || !next->GetSelected() )
			continue;

		// Move next phrase to end of current
		next->SetStartTime( current->GetEndTime() );
	}

	redraw();
}

void PhonemeEditor::CloseCaption_SeparatePhrases( void )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	// Three pixels
	double time_epsilon = ( 1.0f / GetPixelsPerSecond() ) * 6;

	for ( int i = 0; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH )- 1; i++ )
	{
		CCloseCaptionPhrase *current = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		CCloseCaptionPhrase *next = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i + 1 );

		if ( !current->GetSelected() || !next->GetSelected() )
			continue;

		// Close enough?
		if ( fabs( current->GetEndTime() - next->GetStartTime() ) > time_epsilon )
		{
			Con_Printf( "Can't split %s and %s, already split apart\n",
				current->GetStream(), next->GetStream() );
			continue;
		}

		// Offset next phrase start time a bit
		next->SetStartTime( next->GetStartTime() + time_epsilon );

		break;
	}

	redraw();
}

CCloseCaptionPhrase	*PhonemeEditor::CloseCaption_GetPhraseUnderMouse( int mx, int my )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return NULL;

	// Deterime if phoneme boundary is under the cursor
	//
	if ( !m_pWaveFile )
		return NULL;

	RECT rc;
	GetWorkspaceRect( rc );

	if ( !CloseCaption_IsMouseOverRow( my ) )
		return NULL;

	float starttime = m_nLeftOffset / GetPixelsPerSecond();
	float endtime = w2() / GetPixelsPerSecond() + starttime;

	for ( int k = 0; k < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); k++ )
	{
		CCloseCaptionPhrase *phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, k );

		float t1 = phrase->GetStartTime();
		float t2 = phrase->GetEndTime();

		float frac1 = ( t1 - starttime ) / ( endtime - starttime );
		float frac2 = ( t2 - starttime ) / ( endtime - starttime );

		frac1 = min( 1.0f, frac1 );
		frac1 = max( 0.0f, frac1 );
		frac2 = min( 1.0f, frac2 );
		frac2 = max( 0.0f, frac2 );

		if ( frac1 == frac2 )
			continue;

		int x1 = ( int )( frac1 * w2() );
		int x2 = ( int )( frac2 * w2() );

		if ( mx >= x1 && mx <= x2 )
		{
			return phrase;
		}
	}

	return NULL;
}

void PhonemeEditor::CloseCaption_GetTrayTopBottom( RECT& rc )
{
	RECT wkrc;
	GetWorkspaceRect( wkrc );

	int workspaceheight = wkrc.bottom - wkrc.top;

	rc.top  = wkrc.top + workspaceheight/ 2 + 2;
	rc.bottom = rc.top + ( m_nTickHeight + 2 );
}

void PhonemeEditor::CloseCaption_GetPhraseRect( const CCloseCaptionPhrase *phrase, RECT& rc )
{
	CloseCaption_GetTrayTopBottom( rc );
	
	rc.left = GetMouseForTime( phrase->GetStartTime() );
	rc.right = GetMouseForTime( phrase->GetEndTime() );
}

CCloseCaptionPhrase	*PhonemeEditor::CloseCaption_GetClickedPhrase( void )
{
	if ( m_nClickedPhrase < 0 )
		return NULL;

	if ( m_nClickedPhrase >= m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ))
		return NULL;

	CCloseCaptionPhrase *phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, m_nClickedPhrase );
	return phrase;
}

int PhonemeEditor::CloseCaption_GetClickedPhraseToken( void )
{
	if ( !CloseCaption_GetClickedPhrase() )
		return -1;

	return m_nClickedPhraseToken;
}

void PhonemeEditor::CloseCaption_SelectNextPhrase( int direction )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	CountSelected();

	if ( m_nSelectedPhraseCount != 1 )
	{
		// Selected first phrase then
		if ( m_nSelectedPhraseCount == 0 && m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH )> 0 )
		{
			CCloseCaptionPhrase *phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, direction ? m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH )- 1 : 0 );
			phrase->SetSelected( true );
			m_nSelectedPhraseCount = 1;
		}
		else
		{
			return;
		}
	}

	Con_Printf( "Move to next phrase %s\n", direction == -1 ? "left" : "right" );

	for ( int i = 0; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); i++ )
	{
		CCloseCaptionPhrase *phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		if ( !phrase )
			continue;

		if ( m_nSelectedPhraseCount == 1 )
		{
			if ( !phrase->GetSelected() )
				continue;
		}

		// Deselect phrase
		phrase->SetSelected( false );

		// Deselect this one and move 
		int nextphrase = i + direction;
		if ( nextphrase < 0 )
		{
			nextphrase = m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH )- 1;
		}
		else if ( nextphrase >= m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ))
		{
			nextphrase = 0;
		}

		phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, nextphrase );
		phrase->SetSelected( true );

		redraw();
		return;
	}
}

void PhonemeEditor::CloseCaption_ShiftSelectedPhrase( int direction )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	CountSelected();

	switch ( m_nSelectedPhraseCount )
	{
	case 1:
		break;
	case 0:
		Con_Printf( "Can't shift phrases, none selected\n" );
		return;
	default:
		Con_Printf( "Can only shift one phrase at a time via keyboard\n" );
		return;
	}

	RECT rc;
	GetWorkspaceRect( rc );

	// Determine start/stop positions
	int totalsamples = (int)( m_pWaveFile->GetRunningLength() * m_pWaveFile->SampleRate() );

	float starttime = m_nLeftOffset / GetPixelsPerSecond();
	float endtime = w2() / GetPixelsPerSecond() + starttime;

	float timeperpixel = ( endtime - starttime ) / (float)( rc.right - rc.left );

	float movetime = timeperpixel * (float)direction;

	float maxmove = CloseCaption_ComputeMaxPhraseShift( direction > 0 ? true : false, false );

	if ( direction > 0 )
	{
		if ( movetime > maxmove )
		{
			movetime = maxmove;
			Con_Printf( "Further shift is blocked on right\n" );
		}
	}
	else
	{
		if ( movetime < -maxmove )
		{
			movetime = -maxmove;
			Con_Printf( "Further shift is blocked on left\n" );
		}
	}

	if ( fabs( movetime ) < 0.0001f )
		return;

	SetDirty( true );

	PushUndo();

	TraversePhrases( ITER_MoveSelectedPhrases, movetime );

	PushRedo();

	redraw();

	Con_Printf( "Shift phrase %s\n", direction == -1 ? "left" : "right" );
}

void PhonemeEditor::CloseCaption_ExtendSelectedPhraseEndTime( int direction )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	CountSelected();

	if ( m_nSelectedPhraseCount != 1 )
		return;

	RECT rc;
	GetWorkspaceRect( rc );

	// Determine start/stop positions
	int totalsamples = (int)( m_pWaveFile->GetRunningLength() * m_pWaveFile->SampleRate() );

	float starttime = m_nLeftOffset / GetPixelsPerSecond();
	float endtime = w2() / GetPixelsPerSecond() + starttime;

	float timeperpixel = ( endtime - starttime ) / (float)( rc.right - rc.left );

	float movetime = timeperpixel * (float)direction;

	SetDirty( true );

	PushUndo();

	TraversePhrases( ITER_ExtendSelectedPhraseEndTimes, movetime );

	PushRedo();

	redraw();

	Con_Printf( "Extend phrase end %s\n", direction == -1 ? "left" : "right" );
}

float PhonemeEditor::CloseCaption_GetTimeGapToNextPhrase( bool forward, CCloseCaptionPhrase *currentPhrase, CCloseCaptionPhrase **ppNextPhrase /*= NULL*/ )
{
	if ( ppNextPhrase )
	{
		*ppNextPhrase = NULL;
	}

	if ( !currentPhrase )
		return 0.0f;

	int phrasenum = CloseCaption_IndexOfPhrase( currentPhrase );
	if ( phrasenum == -1 )
		return 0.0f;

	// Go in correct direction
	int newphrasenum = phrasenum + ( forward ? 1 : -1 );

	// There is no next phrase
	if ( newphrasenum >= m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ))
	{
		return PLENTY_OF_TIME;
	}

	// There is no previous phrase
	if ( newphrasenum < 0 )
	{
		return PLENTY_OF_TIME;
	}

	if ( ppNextPhrase )
	{
		*ppNextPhrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, newphrasenum );
	}

	// Otherwise, figure out time gap
	if ( forward )
	{
		float currentEnd = currentPhrase->GetEndTime();
		float nextStart = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, newphrasenum )->GetStartTime();

		return ( nextStart - currentEnd );
	}
	else
	{
		float previousEnd = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, newphrasenum )->GetEndTime();
		float currentStart = currentPhrase->GetStartTime();

		return ( currentStart - previousEnd );
	}

	
	Assert( 0 );
	return 0.0f;
}

int PhonemeEditor::CloseCaption_IndexOPhrase( CCloseCaptionPhrase *phrase )
{
	for ( int i = 0 ; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); i++ )
	{
		if ( m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i ) == phrase )
			return i;
	}
	return -1;
}

CCloseCaptionPhrase *PhonemeEditor::CloseCaption_GetSelectedPhrase( void )
{
	CountSelected();

	if ( m_nSelectedPhraseCount != 1 )
		return NULL;

	for ( int i = 0; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); i++ )
	{
		CCloseCaptionPhrase *w = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		if ( !w || !w->GetSelected() )
			continue;

		return w;
	}
	return NULL;
}

bool PhonemeEditor::CloseCaption_AreSelectedPhrasesContiguous( void )
{
	CountSelected();

	if ( m_nSelectedPhraseCount < 1 )
		return false;

	if ( m_nSelectedPhraseCount == 1 )
		return true;

	// Find first selected phrase
	int runcount = 0;
	bool parity = false;

	for ( int i = 0 ; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); i++ )
	{
		CCloseCaptionPhrase *phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		if ( !phrase )
			continue;

		if ( phrase->GetSelected() )
		{
			if ( !parity )
			{
				parity = true;
				runcount++;
			}
		}
		else
		{
			if ( parity )
			{
				parity = false;
			}
		}
	}

	if ( runcount == 1 )
		return true;

	return false;
}

float PhonemeEditor::CloseCaption_ComputeMaxPhraseShift( bool forward, bool allowcrop )
{
	// skipping selected phrases, figure out max time shift of phrases before they selection touches any
	// unselected phrases
	// if allowcrop is true, then the maximum extends up to end of next phrase
	float maxshift = PLENTY_OF_TIME;

	if ( forward )
	{
		for ( int i = 0; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); i++ )
		{
			CCloseCaptionPhrase *phrase1 = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
			if ( !phrase1 || !phrase1->GetSelected() )
				continue;

			CCloseCaptionPhrase *phrase2 = NULL;
			for ( int search = i + 1; search < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); search++ )
			{
				CCloseCaptionPhrase *check = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, search );
				if ( !check || check->GetSelected() )
					continue;

				phrase2 = check;
				break;
			}

			if ( phrase2 )
			{
				float shift;
				if ( allowcrop )
				{
					shift = phrase2->GetEndTime() - phrase1->GetEndTime();
				}
				else
				{
					shift = phrase2->GetStartTime() - phrase1->GetEndTime();
				}

				if ( shift < maxshift )
				{
					maxshift = shift;
				}
			}
		}
	}
	else
	{
		for ( int i = m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH )-1; i >= 0; i-- )
		{
			CCloseCaptionPhrase *phrase1 = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
			if ( !phrase1 || !phrase1->GetSelected() )
				continue;

			CCloseCaptionPhrase *phrase2 = NULL;
			for ( int search = i - 1; search >= 0 ; search-- )
			{
				CCloseCaptionPhrase *check = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, search );
				if ( !check || check->GetSelected() )
					continue;

				phrase2 = check;
				break;
			}

			if ( phrase2 )
			{
				float shift;
				if ( allowcrop )
				{
					shift = phrase1->GetStartTime() - phrase2->GetStartTime();
				}
				else
				{
					shift = phrase1->GetStartTime() - phrase2->GetEndTime();
				}

				if ( shift < maxshift )
				{
					maxshift = shift;
				}
			}
			else if ( i == 0 )
			{
				maxshift = phrase1->GetStartTime();
			}
		}
	}

	return maxshift;
}

bool PhonemeEditor::CloseCaption_FindSpanningPhrases( float time, CCloseCaptionPhrase **pp1, CCloseCaptionPhrase **pp2 )
{
	Assert( pp1 && pp2 );

	*pp1 = NULL;
	*pp2 = NULL;

	// Three pixels
	double time_epsilon = ( 1.0f / GetPixelsPerSecond() ) * 4;

	CCloseCaptionPhrase *previous = NULL;
	for ( int i = 0; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); i++ )
	{
		CCloseCaptionPhrase *current = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		double dt;

		if ( !previous )
		{
			dt = fabs( current->GetStartTime() - time );
			if ( dt < time_epsilon )
			{
				*pp2 = current;
				return true;
			}
		}
		else
		{
			int found = 0;

			dt = fabs( previous->GetEndTime() - time );
			if ( dt < time_epsilon )
			{
				*pp1 = previous;
				found++;
			}

			dt = fabs( current->GetStartTime() - time );
			if ( dt < time_epsilon )
			{
				*pp2 = current;
				found++;
			}

			if ( found != 0 )
			{
				return true;
			}
		}
	
		previous = current;
	}

	if ( m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ) > 0 )
	{
		CCloseCaptionPhrase *last = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, m_Tags.GetCloseCaptionPhraseCount(CC_ENGLISH)-1 );
		float dt;
		dt = fabs( last->GetEndTime() - time );
		if ( dt < time_epsilon )
		{
			*pp1 = last;
			return true;
		}
	}

	return false;
}

void PhonemeEditor::CloseCaption_CleanupPhrases( bool prepareundo )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	// 2 pixel gap
	float snap_epsilon = 2.49f / GetPixelsPerSecond();

	if ( prepareundo )
	{
		SetDirty( true );
		PushUndo();
	}

	CloseCaption_SortPhrases( false );

	for ( int i = 0 ; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ) ; i++ )
	{
		CCloseCaptionPhrase *phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		if ( !phrase )
			continue;

		CCloseCaptionPhrase *next = NULL;
		if ( i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ) - 1 )
		{
			next = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i + 1 );
		}

		if ( phrase && next )
		{
			// Check for phrases close enough
			float eps = next->GetStartTime() - phrase->GetEndTime();
			if ( eps && eps <= snap_epsilon )
			{
				float t = (phrase->GetEndTime() + next->GetStartTime()) * 0.5;
				phrase->SetEndTime( t );
				next->SetStartTime( t );
			}
		}

		float dt = phrase->GetEndTime() - phrase->GetStartTime();
		if ( dt <= 0.01 )
		{
			phrase->SetEndTime( phrase->GetStartTime() + DEFAULT_PHRASE_LENGTH );
		}
	}

	if ( prepareundo )
	{
		PushRedo();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void PhonemeEditor::CloseCaption_SetClickedPhrase( int index, int token )
{
	m_nClickedPhrase = index;
	m_nClickedPhraseToken = token;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : CCloseCaptionPhrase	*phrase - 
// Output : int
//-----------------------------------------------------------------------------
int PhonemeEditor::CloseCaption_IndexOfPhrase( CCloseCaptionPhrase *phrase )
{
	return m_Tags.FindCloseCaptionPhraseIndex( CC_ENGLISH, phrase );
}

void PhonemeEditor::TraversePhrases( PEPHRASEITERFUNC pfn, float fparam )
{
	int c = m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH );

	for ( int i = 0; i < c; i++ )
	{
		CCloseCaptionPhrase *phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		if ( !phrase )
			continue;

		(this->*pfn)( phrase, fparam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : amount - 
//-----------------------------------------------------------------------------
void PhonemeEditor::ITER_MoveSelectedPhrases( CCloseCaptionPhrase *phrase, float amount )
{
	if ( !phrase->GetSelected() )
		return;

	phrase->SetStartTime( phrase->GetStartTime() + amount );
	phrase->SetEndTime( phrase->GetEndTime() + amount );
}

void PhonemeEditor::ITER_ExtendSelectedPhraseEndTimes( CCloseCaptionPhrase *phrase, float amount )
{
	if ( !phrase->GetSelected() )
		return;

	if ( phrase->GetEndTime() + amount <= phrase->GetStartTime() )
		return;

	phrase->SetEndTime( phrase->GetEndTime() + amount );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : drawHelper - 
//			rcWorkSpace - 
//-----------------------------------------------------------------------------
void PhonemeEditor::CloseCaption_Redraw( CChoreoWidgetDrawHelper& drawHelper, RECT& rcWorkSpace, CSentence& sentence )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	float starttime = m_nLeftOffset / GetPixelsPerSecond();
	float endtime = w2() / GetPixelsPerSecond() + starttime;

	RECT rcCC;
	CloseCaption_GetTrayTopBottom( rcCC );

	int ypos = rcCC.top;

	const char *fontName = "Arial Unicode MS";

	RECT rcTitle = rcCC;
	OffsetRect( &rcTitle, 0, -20 );
	rcTitle.left = 15;
	rcTitle.right = w2();

	drawHelper.DrawColoredText( "Arial", 15, FW_BOLD, PEColor( COLOR_PHONEME_CC_TEXT ), rcTitle,
		"Close caption..." );

	drawHelper.DrawColoredLine( PEColor( COLOR_PHONEME_CC_TAG_FILLER_NORMAL ), PS_SOLID, 1,
		0, rcCC.top-1, w2(), rcCC.top-1 );
	drawHelper.DrawColoredLine( PEColor( COLOR_PHONEME_CC_TAG_FILLER_NORMAL ), PS_SOLID, 1,
		0, rcCC.bottom, w2(), rcCC.bottom );
	

	bool drawselected;
	for ( int pass = 0; pass < 2 ; pass++ )
	{
		drawselected = pass == 0 ? false : true;

		int count = sentence.GetCloseCaptionPhraseCount( CC_ENGLISH );
		for (int k = 0; k < count; k++)
		{
			CCloseCaptionPhrase *phrase = sentence.GetCloseCaptionPhrase( CC_ENGLISH, k );
			if ( !phrase )
				continue;

			if ( phrase->GetSelected() != drawselected )
				continue;

			float t1 = phrase->GetStartTime();
			float t2 = phrase->GetEndTime();

			// Tag it
			float frac = ( t1 - starttime ) / ( endtime - starttime );

			int xpos = ( int )( frac * rcWorkSpace.right );

			//if ( frac <= 0.0 )
			//	xpos = 0;

			// Draw duration
			float frac2  = ( t2 - starttime ) / ( endtime - starttime );
			if ( frac2 < 0.0 )
				continue;

			int xpos2 = ( int )( frac2 * rcWorkSpace.right );

			// Draw line and vertical ticks
			RECT rcPhrase;
			CloseCaption_GetPhraseRect( phrase, rcPhrase );

			/*
			rcPhrase.left = xpos;
			rcPhrase.right = xpos2;
			rcPhrase.top = ypos;
			rcPhrase.bottom = ypos + m_nTickHeight - 1;
			*/

			drawHelper.DrawFilledRect( 
				PEColor( phrase->GetSelected() ? COLOR_PHONEME_CC_TAG_SELECTED : COLOR_PHONEME_CC_TAG_FILLER_NORMAL ), 
				rcPhrase );

			COLORREF border = PEColor( phrase->GetSelected() ? COLOR_PHONEME_CC_TAG_BORDER_SELECTED : COLOR_PHONEME_CC_TAG_BORDER );

			drawHelper.DrawOutlinedRect( border, PS_SOLID, 1, rcPhrase );

			//if ( frac >= 0.0 && frac <= 1.0 )
			{
				int fontsize = 9;

				RECT rcText;
				rcText.left = xpos;
				rcText.right = xpos2;
				rcText.top = rcCC.top;
				rcText.bottom = rcCC.bottom;

				int availw = xpos2 - xpos;

				int tokenCount = phrase->CountTokens();
				if ( tokenCount >= 1 )
				{
					int pixelsPerToken = availw / tokenCount;

					rcText.right = rcText.left + pixelsPerToken;
					
					for ( int i = 0; i < tokenCount; i++ )
					{
						wchar_t const *token = phrase->GetToken( i );
						if ( token && token[ 0 ] )
						{
							int texw = drawHelper.CalcTextWidthW( fontName,
								fontsize,
								FW_NORMAL,
								L"%s", token );

							RECT rcOutput = rcText;

							if ( texw < pixelsPerToken )
							{
								rcOutput.left += ( pixelsPerToken - texw ) / 2;
							}

							if ( i != tokenCount - 1 )
							{
								// Draw  divider
								drawHelper.DrawColoredLine( border, PS_SOLID, 1,
									rcText.right, rcText.top + 5, rcText.right, rcText.bottom - 5 );
							}

							drawHelper.DrawColoredTextW( 
								fontName, 
								fontsize, 
								FW_NORMAL, 
								PEColor( phrase->GetSelected() ? COLOR_PHONEME_CC_TAG_TEXT_SELECTED : COLOR_PHONEME_CC_TAG_TEXT ), 
								rcOutput,
								L"%s", token );
						}

						OffsetRect( &rcText, pixelsPerToken, 0 );
					}
				}
			}
		}
	}

	RECT rcOutput;
	CloseCaption_GetCCAreaRect( rcOutput );
	CloseCaption_DrawCCArea( drawHelper, rcOutput );
}

void PhonemeEditor::CloseCaption_GetCCAreaRect( RECT& rcOutput )
{
	GetScrubAreaRect( rcOutput );

	rcOutput.left = w2() - 200;
	rcOutput.right = w2();
	rcOutput.top = rcOutput.bottom + 2;
	rcOutput.bottom = rcOutput.top + 125;
}

void PhonemeEditor::CloseCaption_DrawCCArea( CChoreoWidgetDrawHelper& drawHelper, RECT& rcOutput )
{
	closecaptionmanager->Draw( drawHelper, rcOutput );
}

void PhonemeEditor::ITER_AddFocusRectSelectedPhrases( CCloseCaptionPhrase *phrase, float amount )
{
	if ( !phrase->GetSelected() )
		return;

	RECT phraseRect;
	CloseCaption_GetPhraseRect( phrase, phraseRect );

	AddFocusRect( phraseRect );
}

void PhonemeEditor::CloseCaption_FinishPhraseMove( int startx, int endx )
{
	float clicktime	= GetTimeForPixel( startx );
	float endtime	= GetTimeForPixel( endx );

	// Find the phonemes who have the closest start/endtime to the starting click time
	CCloseCaptionPhrase *current, *next;

	if ( !CloseCaption_FindSpanningPhrases( clicktime, &current, &next ) )
	{
		return;
	}

	SetDirty( true );

	PushUndo();

	if ( current && !next )
	{
		// cap movement
		current->SetEndTime( current->GetEndTime() + ( endtime - clicktime ) );
	}
	else if ( !current && next )
	{
		// cap movement
		next->SetStartTime( next->GetStartTime() + ( endtime - clicktime ) );
	}
	else
	{
		// cap movement
		endtime = min( endtime, next->GetEndTime() - 1.0f / GetPixelsPerSecond() );
		endtime = max( endtime, current->GetStartTime() + 1.0f / GetPixelsPerSecond() );

		current->SetEndTime( endtime );
		next->SetStartTime( endtime );
	}

	CloseCaption_CleanupPhrases( false );

	PushRedo();

	redraw();
}

void PhonemeEditor::CloseCaption_FinishPhraseDrag( int startx, int endx )
{
	float clicktime	= GetTimeForPixel( startx );
	float endtime	= GetTimeForPixel( endx );

	float dt = endtime - clicktime;

	SetDirty( true );

	PushUndo();

	TraversePhrases( ITER_MoveSelectedPhrases, dt );

	CloseCaption_CleanupPhrases( false );

	PushRedo();

	redraw();
}

void PhonemeEditor::CloseCaption_SelectPhrases( void )
{
	if ( GetMode() != MODE_CLOSECAPTION )
		return;

	for ( int i = 0 ; i < m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ); i++ )
	{
		CCloseCaptionPhrase *w = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		Assert( w );
		w->SetSelected( true );
	}

	redraw();
}

void PhonemeEditor::CloseCaption_EditDefaultPhrase( void )
{
	if ( m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH ) == 0 )
	{
		SetDirty( true );

		PushUndo();

		m_Tags.SetCloseCaptionFromText( CC_ENGLISH );

		PushRedo();

		redraw();
	}
	else
	{
		Con_Printf( "Can only set default phrase on a sentence without existing phrases\n" );
	}
}



void PhonemeEditor::CloseCaption_SplitPhraseAfterToken( CCloseCaptionPhrase *phrase, int splitToken )
{
	int count = phrase->CountTokens();
	if ( count < 2 )
	{
		Con_Printf( "PhonemeEditor::CloseCaption_SplitPhraseAtToken:  Can't split %s, %i tokens total\n",
			phrase->GetStream(),
			count );
		return;
	}

	if ( splitToken >= count - 1 )
	{
		// After end...sigh
		return;
	}

	wchar_t stream1[ 4096 ];
	wchar_t stream2[ 4096 ];

	stream1[0] = L'\0';
	stream2[0] = L'\0';

	wchar_t const *token;
	int count1 = 0;
	int count2 = 0;

	for ( int i = 0; i < count; i++ )
	{
		token = phrase->GetToken( i );
		Assert( token && token[ 0 ] );
		if ( i <= splitToken )
		{
			if ( count1 != 0 )
			{
				wcscat( stream1, L" " );
			}

			wcscat( stream1, token );
			count1++;
		}
		else
		{
			if ( count2 != 0 )
			{
				wcscat( stream2, L" " );
			}

			wcscat( stream2, token );
			count2++;
		}
	}

	SetDirty( true );

	PushUndo();

	CCloseCaptionPhrase *newPhrase = new CCloseCaptionPhrase( stream2 );
	phrase->SetStream( stream1 );
	float oldend = phrase->GetEndTime();
	float dt = oldend - phrase->GetStartTime();
	float splitTime = dt * (float)(splitToken+1) / (float)(count);

	phrase->SetEndTime( phrase->GetStartTime() + splitTime );
	newPhrase->SetStartTime( phrase->GetEndTime() );
	newPhrase->SetEndTime( oldend );
	newPhrase->SetSelected( true );

	int first = m_Tags.FindCloseCaptionPhraseIndex( CC_ENGLISH, phrase );
	m_Tags.InsertCloseCaptionPhraseAtIndex( CC_ENGLISH, newPhrase, first + 1 );

	PushRedo();

	redraw();
}

void PhonemeEditor::CloseCaption_SplitPhrase( void )
{
	CCloseCaptionPhrase *phrase = CloseCaption_GetClickedPhrase();
	if ( !phrase )
		return;

	int token = CloseCaption_GetClickedPhraseToken();
	if ( token < 0 )
		return;

	CloseCaption_SplitPhraseAfterToken( phrase, token );

	redraw();
}

void PhonemeEditor::CloseCaption_MergeSelected( void )
{
	CountSelected();
	if ( m_nSelectedPhraseCount < 2 )
	{
		Con_Printf( "CloseCaption_MergeSelected:  requires 2 or more selected phrases\n" );
		return;
	}

	if ( !CloseCaption_AreSelectedPhrasesContiguous() )
	{
		Con_Printf( "CloseCaption_MergeSelected:  selected phrases must be contiguous\n" );
		return;
	}

	SetDirty( true );

	PushUndo();

	CUtlVector< CCloseCaptionPhrase * > selected;

	float beststart = 100000.0f;
	float bestend = -100000.0f;

	int c = m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH );
	int i;
	int insertslot = c -1;

	// Walk backwards and remove
	for ( i = c - 1; i >= 0; i-- )
	{
		CCloseCaptionPhrase *phrase = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
		if ( !phrase || !phrase->GetSelected() )
			continue;

		if ( phrase->GetStartTime() < beststart )
		{
			beststart = phrase->GetStartTime();
		}
		if ( phrase->GetEndTime() > bestend )
		{
			bestend = phrase->GetEndTime();
		}

		selected.AddToHead( new CCloseCaptionPhrase( *phrase ) );

		// Remember the earliest slot
		if ( i < insertslot )
		{
			insertslot = i;
		}

		m_Tags.RemoveCloseCaptionPhrase( CC_ENGLISH, i );
	}

	if ( selected.Count() <= 0 )
		return;

	CCloseCaptionPhrase *newphrase = new CCloseCaptionPhrase( selected[ 0 ]->GetStream() );
	Assert( newphrase );
	wchar_t sz[ 4096 ];
	delete selected[ 0 ];
	for ( i = 1; i < selected.Count(); i++ )
	{
		_snwprintf( sz, sizeof( sz ), newphrase->GetStream() );
		// Phrases don't have leading/trailing spaces so it should be safe to just append a space here
		wcscat( sz, L" " );
		wcscat( sz, selected[ i ]->GetStream() );
		newphrase->SetStream( sz );
		delete selected[ i ];
	}
	selected.RemoveAll();

	m_Tags.InsertCloseCaptionPhraseAtIndex( CC_ENGLISH, newphrase, insertslot );
	newphrase->SetSelected( true );
	newphrase->SetStartTime( beststart );
	newphrase->SetEndTime( bestend );

	PushRedo();

	redraw();
}

int PhonemeEditor::CloseCaption_GetPhraseTokenUnderMouse( const CCloseCaptionPhrase *phrase, int mx )
{
	int index = -1;
	int count = phrase->CountTokens();
	if ( count >= 1 )
	{
		RECT rcPhrase;
		CloseCaption_GetPhraseRect( phrase, rcPhrase );

		float t = GetTimeForPixel( mx );
		float dt = phrase->GetEndTime() - phrase->GetStartTime();
		if ( dt > 0.0f )
		{
			float frac = ( t - phrase->GetStartTime() ) / dt;
			if ( frac >= 0 && frac <= 1.0f )
			{
				int availw = rcPhrase.right - rcPhrase.left;

				int clickpoint = frac * availw;

				int pixelsPerToken = availw / count;

				index = clickpoint / pixelsPerToken;
				index = clamp( index, 0, count - 1 );
			}
		}
	}
	return index;
}

void PhonemeEditor::CloseCaption_SortPhrases( bool prepareundo )
{
	if ( prepareundo )
	{
		SetDirty( true );
		PushUndo();
	}

	// Just bubble sort by start time
	int c = m_Tags.GetCloseCaptionPhraseCount( CC_ENGLISH );
	for ( int i = 0; i < c; i++ )
	{
		for ( int j = i + 1; j < c; j++ )
		{
			CCloseCaptionPhrase *p1 = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, i );
			CCloseCaptionPhrase *p2 = m_Tags.GetCloseCaptionPhrase( CC_ENGLISH, j );

			if ( p1->GetStartTime() < p2->GetStartTime() )
				continue;

			// Swap them
			m_Tags.m_CloseCaption[ CC_ENGLISH ][ i ] = p2;
			m_Tags.m_CloseCaption[ CC_ENGLISH ][ j ] = p1;
		}
	}

	if ( prepareundo )
	{
		PushRedo();
	}
}
