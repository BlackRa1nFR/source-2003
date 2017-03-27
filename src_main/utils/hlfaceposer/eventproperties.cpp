//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include <mx/mx.h>
#include <stdio.h>
#include "resource.h"
#include "EventProperties.h"
#include "mdlviewer.h"
#include "choreoevent.h"
#include "choreoscene.h"
#include "expressions.h"
#include "choreochannel.h"
#include "choreoactor.h"
#include "ifaceposersound.h"
#include "expclass.h"
#include "cmdlib.h"
#include "scriplib.h"
#include "StudioModel.h"
#include "FileSystem.h"
#include <commctrl.h>
#include "faceposer_models.h"
#include "SoundEmitterSystemBase.h"
#include "AddSoundEntry.h"
#include "SoundLookup.h"

static CEventParams g_Params;

static void PopulateTagList( HWND wnd, CEventParams *params )
{
	CChoreoScene *scene = params->m_pScene;
	if ( !scene )
		return;

	HWND control = GetDlgItem( wnd, IDC_TAGS );
	if ( control )
	{
		SendMessage( control, CB_RESETCONTENT, 0, 0 ); 
		SendMessage( control, WM_SETTEXT , 0, (LPARAM)va( "\"%s\" \"%s\"", params->m_szTagName, params->m_szTagWav ) );

		for ( int i = 0; i < scene->GetNumActors(); i++ )
		{
			CChoreoActor *a = scene->GetActor( i );
			if ( !a )
				continue;

			for ( int j = 0; j < a->GetNumChannels(); j++ )
			{
				CChoreoChannel *c = a->GetChannel( j );
				if ( !c )
					continue;

				for ( int k = 0 ; k < c->GetNumEvents(); k++ )
				{
					CChoreoEvent *e = c->GetEvent( k );
					if ( !e )
						continue;

					if ( e->GetNumRelativeTags() <= 0 )
						continue;

					// add each tag to combo box
					for ( int t = 0; t < e->GetNumRelativeTags(); t++ )
					{
						CEventRelativeTag *tag = e->GetRelativeTag( t );
						if ( !tag )
							continue;

						SendMessage( control, CB_ADDSTRING, 0, (LPARAM)va( "\"%s\" \"%s\"", tag->GetName(), e->GetParameters() ) ); 
					}
				}
			}
		}
	}
}


static void PopulateEventChoices2( HWND wnd, CEventParams *params )
{
	HWND control = GetDlgItem( wnd, IDC_EVENTCHOICES2 );
	if ( control )
	{
		SendMessage( control, CB_RESETCONTENT, 0, 0 ); 
		SendMessage( control, WM_SETTEXT , 0, (LPARAM)params->m_szParameters2 );

		// Now populate list
		switch ( params->m_nType )
		{
		default:
			break;
		case CChoreoEvent::EXPRESSION:
			{
				// Find parameter 1
				for ( int c = 0; c < expressions->GetNumClasses(); c++ )
				{
					CExpClass *cl = expressions->GetClass( c );
					if ( !cl || cl->IsOverrideClass() )
						continue;

					if ( stricmp( cl->GetName(), params->m_szParameters ) )
						continue;

					for ( int i = 0 ; i < cl->GetNumExpressions() ; i++ )
					{
						CExpression *exp = cl->GetExpression( i );
						if ( exp )
						{
							// add text to combo box
							SendMessage( control, CB_ADDSTRING, 0, (LPARAM)exp->name ); 
						}
					}
					
					break;
				}
			}
			break;
		case CChoreoEvent::SPEAK:
			{
				// add text to combo box
				SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"60dB" );
				SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"65dB" );
				SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"70dB" );
				SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"75dB" );
				SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"80dB" );
				SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"85dB" );
				SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"90dB" );
			}
			break;
		}
	}
}

#include "mapentities.h"
#include "UtlDict.h"

struct CMapEntityData
{
	CMapEntityData()
	{
		origin.Init();
		angles.Init();
	}

	Vector origin;
	QAngle angles;
};

class CMapEntities : public IMapEntities
{
public:
	CMapEntities();
	~CMapEntities();

	virtual void	CheckUpdateMap( char const *mapname );
	virtual bool	LookupOrigin( char const *name, Vector& origin, QAngle& angles )
	{
		int idx = FindNamedEntity( name );
		if ( idx == -1 )
		{
			origin.Init();
			angles.Init();
			return false;
		}

		CMapEntityData *e = &m_Entities[ idx ];
		Assert( e );
		origin = e->origin;
		angles = e->angles;
		return true;
	}

	int		Count( void );
	char const *GetName( int number );

	int	FindNamedEntity( char const *name );

private:
	char		m_szCurrentMap[ 1024 ];

	CUtlDict< CMapEntityData, int >	m_Entities;
};

static CMapEntities g_MapEntities;
// Expose to rest of tool
IMapEntities *mapentities = &g_MapEntities;

CMapEntities::CMapEntities()
{
	m_szCurrentMap[ 0 ] = 0;
}

CMapEntities::~CMapEntities()
{
	m_Entities.RemoveAll();
}

int CMapEntities::FindNamedEntity( char const *name )
{
	char lowername[ 128 ];
	strcpy( lowername, name );
	_strlwr( lowername );

	int index = m_Entities.Find( lowername );
	if ( index == m_Entities.InvalidIndex() )
		return -1;

	return index;
}

#include "bspfile.h"

void CMapEntities::CheckUpdateMap( char const *mapname )
{
	if ( !mapname || !mapname[ 0 ] )
		return;

	if ( !stricmp( mapname, m_szCurrentMap ) )
		return;

	// Load names from map
	m_Entities.RemoveAll();

	FileHandle_t hfile = filesystem->Open( mapname, "rb" );
	if ( hfile == FILESYSTEM_INVALID_HANDLE )
		return;

	dheader_t header;
	filesystem->Read( &header, sizeof( header ), hfile );

	// Check the header
	if ( header.ident != IDBSPHEADER ||
		 header.version != BSPVERSION )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "BSP file %s is wrong version (%i), expected (%i)\n",
			mapname,
			header.version, 
			BSPVERSION );

		filesystem->Close( hfile );
		return;
	}

	// Find the LUMP_PAKFILE offset
	lump_t *entlump = &header.lumps[ LUMP_ENTITIES ];
	if ( entlump->filelen <= 0 )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "BSP file %s is missing entity lump\n", mapname );

		// It's empty or only contains a file header ( so there are no entries ), so don't add to search paths
		filesystem->Close( hfile );
		return;
	}

	// Seek to correct position
	filesystem->Seek( hfile, entlump->fileofs, FILESYSTEM_SEEK_HEAD );

	char *buffer = new char[ entlump->filelen + 1 ];
	Assert( buffer );

	filesystem->Read( buffer, entlump->filelen, hfile );

	filesystem->Close( hfile );

	buffer[ entlump->filelen ] = 0;

	// Now we have entity buffer, now parse it
	ParseFromMemory( buffer, entlump->filelen );

	while ( 1 )
	{
		if (!GetToken (true))
			break;

		if (strcmp (token, "{") )
			Error ("ParseEntity: { not found");
		
		char name[ 256 ];
		char origin[ 256 ];
		char angles[ 256 ];

		name[ 0 ] = 0;
		origin[ 0 ] = 0;
		angles[ 0 ] = 0;

		do
		{
			char key[ 256 ];
			char value[ 256 ];

			if (!GetToken (true))
			{
				Error ("ParseEntity: EOF without closing brace");
			}

			if (!strcmp (token, "}") )
				break;

			strcpy( key, token );

			GetToken (false);

			strcpy( value, token );

			// Con_Printf( "Parsed %s -- %s\n", key, value );

			if ( !stricmp( key, "name" ) )
			{
				strcpy( name, value );
			}
			if ( !stricmp( key, "targetname" ) )
			{
				strcpy( name, value );
			}
			if ( !stricmp( key, "origin" ) )
			{
				strcpy( origin, value );
			}
			if ( !stricmp( key, "angles" ) )
			{
				strcpy( angles, value );
			}

		} while (1);

		if ( name[ 0 ] )
		{
			if ( FindNamedEntity( name ) == - 1 )
			{
				CMapEntityData ent;
				
				float org[3];
				if ( origin[ 0 ] )
				{
					if ( 3 == sscanf( origin, "%f %f %f", &org[ 0 ], &org[ 1 ], &org[ 2 ] ) )
					{
						ent.origin = Vector( org[ 0 ], org[ 1 ], org[ 2 ] );

						// Con_Printf( "read %f %f %f for entity %s\n", org[0], org[1], org[2], name );
					}
				}
				if ( angles[ 0 ] )
				{
					if ( 3 == sscanf( angles, "%f %f %f", &org[ 0 ], &org[ 1 ], &org[ 2 ] ) )
					{
						ent.angles = QAngle( org[ 0 ], org[ 1 ], org[ 2 ] );

						// Con_Printf( "read %f %f %f for entity %s\n", org[0], org[1], org[2], name );
					}
				}

				m_Entities.Insert( name, ent );
			}
		}
	}

	delete[] buffer;
}

int CMapEntities::Count( void )
{
	return m_Entities.Count();
}

char const *CMapEntities::GetName( int number )
{
	if ( number < 0 || number >= m_Entities.Count() )
		return NULL;

	return m_Entities.GetElementName( number );
}


static void PopulateNamedActorList( HWND wnd, CEventParams *params )
{
	char const *mapname = NULL;
	if ( params->m_pScene )
	{
		mapname = params->m_pScene->GetMapname();
	}

	if ( mapname )
	{
		g_MapEntities.CheckUpdateMap( mapname );

		for ( int i = 0; i < g_MapEntities.Count(); i++ )
		{
			char const *name = g_MapEntities.GetName( i );
			if ( name && name[ 0 ] )
			{
				SendMessage( wnd, CB_ADDSTRING, 0, (LPARAM)name ); 
			}
		}
	}
	else
	{
		for ( int i = 0 ; i < params->m_pScene->GetNumActors() ; i++ )
		{
			CChoreoActor *actor = params->m_pScene->GetActor( i );
			if ( actor )
			{
				// add text to combo box
				SendMessage( wnd, CB_ADDSTRING, 0, (LPARAM)actor->GetName() ); 
			}
		}
	}
}

static bool NameLessFunc( const char *const& name1, const char *const& name2 )
{
	if ( stricmp( name1, name2 ) < 0 )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wnd - 
// Output : static void
//-----------------------------------------------------------------------------
static void PopulateSoundList( HWND wnd, bool showAll )
{
	CUtlRBTree< char const *, int >		m_SortedNames( 0, 0, NameLessFunc );

	int c = soundemitter->GetSoundCount();
	for ( int i = 0; i < c; i++ )
	{
		char const *name = soundemitter->GetSoundName( i );

		if ( name && name[ 0 ] )
		{
			bool add = true;
			if ( !showAll )
			{
				CSoundParameters params;
				if ( soundemitter->GetParametersForSound( name, params ) )
				{
					if ( params.channel != CHAN_VOICE )
					{
						add = false;
					}
				}
			}
			if ( add )
			{
				m_SortedNames.Insert( name );
			}
		}
	}

	char sz[ 512 ];
	SendMessage( wnd, WM_GETTEXT, sizeof( sz ), (LPARAM)sz );

	// Remove all
	SendMessage( wnd, CB_RESETCONTENT, 0, 0 );

	i = m_SortedNames.FirstInorder();
	while ( i != m_SortedNames.InvalidIndex() )
	{
		char const *name = m_SortedNames[ i ];
		if ( name && name[ 0 ] )
		{
			SendMessage( wnd, CB_ADDSTRING, 0, (LPARAM)name ); 
		}

		i = m_SortedNames.NextInorder( i );
	}

	SendMessage( wnd, WM_SETTEXT , 0, (LPARAM)sz );
}

static void SetPitchYawText( HWND wnd, CEventParams *params )
{
	HWND control;

	control = GetDlgItem( wnd, IDC_STATIC_PITCHVAL );
	SendMessage( control, WM_SETTEXT , 0, (LPARAM)va( "%i", params->pitch ) );
	control = GetDlgItem( wnd, IDC_STATIC_YAWVAL );
	SendMessage( control, WM_SETTEXT , 0, (LPARAM)va( "%i", params->yaw ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wnd - 
//			*params - 
// Output : static
//-----------------------------------------------------------------------------
static void ShowControlsForEventType( HWND wnd, CEventParams *params )
{
	HWND control;

	if ( params->m_nType != CChoreoEvent::LOOKAT )
	{
		ShowWindow( GetDlgItem( wnd, IDC_STATIC_PITCH ), SW_HIDE );
		ShowWindow( GetDlgItem( wnd, IDC_STATIC_YAW ), SW_HIDE );
		ShowWindow( GetDlgItem( wnd, IDC_SLIDER_PITCH ), SW_HIDE );
		ShowWindow( GetDlgItem( wnd, IDC_SLIDER_YAW ), SW_HIDE );
		ShowWindow( GetDlgItem( wnd, IDC_STATIC_PITCHVAL ), SW_HIDE );
		ShowWindow( GetDlgItem( wnd, IDC_STATIC_YAWVAL ), SW_HIDE );
		ShowWindow( GetDlgItem( wnd, IDC_CHECK_LOOKAT ), SW_HIDE );
	}
	else
	{
		SetPitchYawText( wnd, params );

		if ( params->pitch != 0 ||
			 params->yaw != 0 )
		{
			params->usepitchyaw = true;
		}
		else
		{
			params->usepitchyaw = false;
		}
		
		control = GetDlgItem( wnd, IDC_CHECK_LOOKAT );
		SendMessage( control, BM_SETCHECK, (WPARAM) params->usepitchyaw ? BST_CHECKED : BST_UNCHECKED, 0 );

		// Set up sliders
		control = GetDlgItem( wnd, IDC_SLIDER_PITCH );
		SendMessage( control, TBM_SETRANGE, 0, (LPARAM)MAKELONG( -100, 100 ) );
		SendMessage( control, TBM_SETPOS, 1, (LPARAM)(LONG)params->pitch );
	
		control = GetDlgItem( wnd, IDC_SLIDER_YAW );
		SendMessage( control, TBM_SETRANGE, 0, (LPARAM)MAKELONG( -100, 100 ) );
		SendMessage( control, TBM_SETPOS, 1, (LPARAM)(LONG)params->yaw );
	}

	if ( params->m_nType != CChoreoEvent::EXPRESSION && params->m_nType != CChoreoEvent::SPEAK)
	{
		control = GetDlgItem( wnd, IDC_EVENTCHOICES2 );
		if ( control )
		{
			ShowWindow( control, SW_HIDE );
		}
	}

	if ( params->m_nType == CChoreoEvent::FLEXANIMATION ||
		 params->m_nType == CChoreoEvent::INTERRUPT )
	{
		control = GetDlgItem( wnd, IDC_EVENTCHOICES );
		if ( control )
		{
			ShowWindow( control, SW_HIDE );
		}
	}

	if ( params->m_nType != CChoreoEvent::SPEAK )
	{
		control = GetDlgItem( wnd, IDC_SHOW_ALL_SOUNDS );
		if ( control )
		{
			ShowWindow( control, SW_HIDE );
		}
	}

	PopulateTagList( wnd, params );

	if ( params->m_nType == CChoreoEvent::SPEAK )
	{
		/*
		// Hide the combo box
		control = GetDlgItem( wnd, IDC_EVENTCHOICES );
		if ( control )
		{
			ShowWindow( control, SW_HIDE );
		}

		SetDlgItemText( wnd, IDC_WAVNAME, params->m_szParameters );
		*/
		control = GetDlgItem( wnd, IDC_EVENTCHOICES );
		if ( control )
		{
			PopulateSoundList( control, false );
			SetDlgItemText( wnd, IDC_EVENTCHOICES, params->m_szParameters );
		}
		
		control = GetDlgItem( wnd, IDC_WAVNAME );
		if ( control )
		{
			ShowWindow( control, SW_HIDE );
		}

		PopulateEventChoices2( wnd, params );
	}
	else if ( params->m_nType == CChoreoEvent::SUBSCENE )
	{
		// Hide the combo box
		control = GetDlgItem( wnd, IDC_EVENTCHOICES );
		if ( control )
		{
			ShowWindow( control, SW_HIDE );
		}

		SetDlgItemText( wnd, IDC_WAVNAME, params->m_szParameters );
	}
	else
	{
		// Hide the wav button and the wav static
		control = GetDlgItem( wnd, IDC_SELECTWAV );
		if ( control )
		{
			ShowWindow( control, SW_HIDE );
		}

		control = GetDlgItem( wnd, IDC_WAVNAME );
		if ( control )
		{
			ShowWindow( control, SW_HIDE );
		}

		control = GetDlgItem( wnd, IDC_EVENTCHOICES );
		if ( control )
		{
			SendMessage( control, CB_RESETCONTENT, 0, 0 ); 
			SendMessage( control, WM_SETTEXT , 0, (LPARAM)params->m_szParameters );

			// Now populate list
			switch ( params->m_nType )
			{
			default:
				break;
			case CChoreoEvent::LOOKAT:
			case CChoreoEvent::MOVETO:
			case CChoreoEvent::FACE:
				{
					PopulateNamedActorList( control, params );

					// These events can be directed at another player, too
					SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"!player" );
					SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"!enemy" );
					SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"!self" );
					SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"!friend" );
					SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"!target1" );
					SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"!target2" );
					SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"!target3" );
					SendMessage( control, CB_ADDSTRING, 0, (LPARAM)"!target4" );
				}
				break;
			case CChoreoEvent::GESTURE:
				{
					studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
					if (hdr)
					{
						for (int i = 0; i < hdr->numseq; i++)
						{
							// if (stricmp( hdr->pSeqdesc(i)->pszActivityName(), "ACT_GESTURE") == 0)
							{
								SendMessage( control, CB_ADDSTRING, 0, (LPARAM)hdr->pSeqdesc(i)->pszLabel() ); 
							}
						}
					}
				}
				// SendMessage( control, WM_SETTEXT , 0, (LPARAM)params->m_szParameters );
				break;
			case CChoreoEvent::SEQUENCE:
				{
					studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
					if (hdr)
					{
						for (int i = 0; i < hdr->numseq; i++)
						{
							SendMessage( control, CB_ADDSTRING, 0, (LPARAM)hdr->pSeqdesc(i)->pszLabel() ); 
						}
					}
				}
				// SendMessage( control, WM_SETTEXT , 0, (LPARAM)params->m_szParameters );
				break;
			case CChoreoEvent::EXPRESSION:
				{
					for ( int i = 0 ; i < expressions->GetNumClasses() ; i++ )
					{
						CExpClass *cl = expressions->GetClass( i );
						if( !cl || cl->IsOverrideClass() )
							continue;

						// add text to combo box
						SendMessage( control, CB_ADDSTRING, 0, (LPARAM)cl->GetName() ); 
					}
					
					//SendMessage( control, CB_SELECTSTRING , 0, (LPARAM)params->m_szParameters );
					PopulateEventChoices2( wnd, params );
				}
				break;
			case CChoreoEvent::FIRETRIGGER:
				{
					for ( int i = 0 ; i < 4 ; i++ )
					{
						char szName[256];

						sprintf( szName, "%d", i + 1 );

						// add text to combo box
						SendMessage( control, CB_ADDSTRING, 0, (LPARAM)szName ); 
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wnd - 
//			*params - 
// Output : static void
//-----------------------------------------------------------------------------
static void ParseTags( HWND wnd, CEventParams *params )
{
	strcpy( params->m_szTagName, "" );
	strcpy( params->m_szTagWav, "" );

	if ( params->m_bUsesTag )
	{
		// Parse out the two tokens
		char selectedText[ 512 ];
		selectedText[ 0 ] = 0;
		HWND control = GetDlgItem( wnd, IDC_TAGS );
		if ( control )
		{
			SendMessage( control, WM_GETTEXT, (WPARAM)sizeof( selectedText ), (LPARAM)selectedText );
		}

		ParseFromMemory( selectedText, strlen( selectedText ) );
		if ( TokenAvailable() )
		{
			GetToken( false );
			char tagname[ 256 ];
			strcpy( tagname, token );
			if ( TokenAvailable() )
			{
				GetToken( false );
				char wavename[ 256 ];
				strcpy( wavename, token );

				// Valid
				strcpy( params->m_szTagName, tagname );
				strcpy( params->m_szTagWav, wavename );

			}
			else
			{
				params->m_bUsesTag = false;
			}
		}
		else
		{
			params->m_bUsesTag = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wnd - 
//			*params - 
// Output : static void
//-----------------------------------------------------------------------------
static void UpdateTagRadioButtons( HWND wnd, CEventParams *params )
{
	if ( params->m_bUsesTag )
	{
		SendMessage( GetDlgItem( wnd, IDC_RELATIVESTART ), BM_SETCHECK,
			( WPARAM )BST_CHECKED, (LPARAM)0 );
		SendMessage( GetDlgItem( wnd, IDC_ABSOLUTESTART ), BM_SETCHECK,
			( WPARAM )BST_UNCHECKED, (LPARAM)0 );
	}
	else
	{
		SendMessage( GetDlgItem( wnd, IDC_ABSOLUTESTART ), BM_SETCHECK,
			( WPARAM )BST_CHECKED, (LPARAM)0 );
		SendMessage( GetDlgItem( wnd, IDC_RELATIVESTART ), BM_SETCHECK,
			( WPARAM )BST_UNCHECKED, (LPARAM)0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : slider - 
//			curValue - 
// Output : static void
//-----------------------------------------------------------------------------
static void SetupSlider( HWND slider, float curValue )
{
	SendMessage( slider, TBM_SETRANGE, (WPARAM)FALSE, (LPARAM)MAKELONG( 0, 100 ) );
	int curval = 100.0f * curValue;
	curval = clamp( curval, 0, 100 );

	// invert
	curval = 100 - curval;

	SendMessage( slider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(LONG)curval );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : slider - 
// Output : static float
//-----------------------------------------------------------------------------
static float GetSliderPos( HWND slider )
{
	int curval = SendMessage( slider, TBM_GETPOS, 0, 0 );

	// invert
	curval = 100 - curval;

	return ( float )( curval ) / 100.0f;
}

static void GetSplineRect( HWND dlg, HWND placeholder, RECT& rcOut )
{
	GetWindowRect( placeholder, &rcOut );
	RECT rcDlg;
	GetWindowRect( dlg, &rcDlg );

	OffsetRect( &rcOut, -rcDlg.left, -rcDlg.top );
}

static void DrawSpline( HWND dlg, HDC hdc, HWND placeholder )
{
	RECT rcOut;

	GetSplineRect( dlg, placeholder, rcOut );

	HBRUSH bg = CreateSolidBrush( GetSysColor( COLOR_BTNFACE ) );
	FillRect( hdc, &rcOut, bg );
	DeleteObject( bg );

	CChoreoEvent *e = g_Params.m_pEvent;
	if ( !e )
		return;

	// Draw spline
	float range = ( float )( rcOut.right - rcOut.left );
	if ( range <= 1.0f )
		return;

	float height = ( float )( rcOut.bottom - rcOut.top );

	HPEN pen = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNTEXT ) );
	HPEN oldPen = (HPEN)SelectObject( hdc, pen );

	float duration = e->GetDuration();
	float starttime = e->GetStartTime();

	for ( int i = 0; i < (int)range; i++ )
	{
		float frac = ( float )i / ( range - 1 );

		float scale = 1.0f - e->GetIntensity( starttime + frac * duration );

		int h = ( int ) ( scale * ( height - 1 ) );

		if ( i == 0 )
		{
			MoveToEx( hdc, rcOut.left + i, rcOut.top + h, NULL );
		}
		else
		{
			LineTo( hdc, rcOut.left + i, rcOut.top + h );
		}
	}

	SelectObject( hdc, oldPen );

	HBRUSH frame = CreateSolidBrush( GetSysColor( COLOR_BTNSHADOW ) );
	InflateRect( &rcOut, 1, 1 );
	FrameRect( hdc, &rcOut, frame );
	DeleteObject( frame );
}

static void FindWaveInSoundEntries( CUtlVector< int >& entryList, char const *search )
{
	int c = soundemitter->GetSoundCount();
	for ( int i = 0; i < c; i++ )
	{
		CSoundEmitterSystemBase::CSoundParametersInternal *params = soundemitter->InternalGetParametersForSound( i );
		if ( !params )
			continue;

		int waveCount = params->soundnames.Count();
		for ( int wave = 0; wave < waveCount; wave++ )
		{
			char const *waveName = soundemitter->GetWaveName( params->soundnames[ wave ] );

			if ( !Q_stricmp( waveName, search ) )
			{
				entryList.AddToTail( i );
				break;
			}
		}
	}
}

static void OnSelectWaveFile( HWND hwndDlg )
{
	char workingdir[ 256 ];
	Q_getwd( workingdir );
	strlwr( workingdir );
	COM_FixSlashes( workingdir );

	const char *filename = NULL;

	bool inSoundAlready = false;

	if ( strstr( workingdir, va( "%s/sound", GetGameDirectory() ) ) )
	{
		inSoundAlready = true;
	}
	// Show file io
	filename = mxGetOpenFileName( 
		0, 
		inSoundAlready ? "." : FacePoser_MakeWindowsSlashes( va( "%s/sound/", GetGameDirectory() ) ), 
		"*.wav" );

	if ( !filename || !filename[ 0 ] )
		return;

	char relative[ 512 ];
	strcpy( relative, filename );
	filesystem->FullPathToRelativePath( filename, relative );

	COM_FixSlashes( relative );

	// Try and find entry in soundemitter system
	CUtlVector< int >	entryList;

	FindWaveInSoundEntries( entryList, relative + Q_strlen( "sound/") );

	if ( entryList.Count() >= 1 )
	{
		// Show the Lookup dialog
		CSoundLookupParams params;
		Q_memset( &params, 0, sizeof( params ) );
		strcpy( params.m_szDialogTitle, "Sound Entry Lookup" );
		Q_snprintf( params.m_szPrompt, sizeof( params.m_szPrompt ), "Sound entries for '%s'", relative );
		Q_strncpy( params.m_szWaveFile, relative + Q_strlen( "sound/"), sizeof( params.m_szWaveFile ) );
		params.entryList = &entryList;
		params.m_szSoundName[ 0 ] = 0;

		if ( SoundLookup( &params, hwndDlg ) )
		{
			if( params.m_szSoundName[ 0 ] )
			{
				Q_strcpy( relative, params.m_szSoundName );
			}
		}
	}
	else
	{
		// Prompt if they want to create new entry and if yes, show the AddSound dialog

		int retval = mxMessageBox( NULL, va( "Do you want to create a sound entry for '%s'?", relative ), g_appTitle, MX_MB_YESNO | MX_MB_QUESTION );
		if ( retval == 0 )
		{
			// Create a new sound entry for this sound
			CAddSoundParams params;
			Q_memset( &params, 0, sizeof( params ) );
			Q_strcpy( params.m_szDialogTitle, "Add Sound Entry" );
			Q_strcpy( params.m_szWaveFile, relative + Q_strlen( "sound/") );

			if ( AddSound( &params, hwndDlg ) )
			{
				// Add it to soundemitter and check out script files
				if ( params.m_szSoundName[ 0 ] && 
					 params.m_szScriptName[ 0 ] )
				{
					Q_strcpy( relative, params.m_szSoundName );
				}
			}
		}
	}


	SetDlgItemText( hwndDlg, IDC_EVENTCHOICES, relative );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hwndDlg - 
//			uMsg - 
//			wParam - 
//			lParam - 
// Output : static BOOL CALLBACK
//-----------------------------------------------------------------------------
static BOOL CALLBACK EventPropertiesDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps; 
			HDC hdc;
			
            hdc = BeginPaint(hwndDlg, &ps); 
			DrawSpline( hwndDlg, hdc, GetDlgItem( hwndDlg, IDC_STATIC_SPLINE ) );
            EndPaint(hwndDlg, &ps); 

            return FALSE; 
		}
		break;
	case WM_VSCROLL:
		{
			RECT rcOut;
			GetSplineRect( hwndDlg, GetDlgItem( hwndDlg, IDC_STATIC_SPLINE ), rcOut );

			InvalidateRect( hwndDlg, &rcOut, TRUE );
			UpdateWindow( hwndDlg );
			return FALSE;
		}
		break;
    case WM_INITDIALOG:
		{
			g_Params.PositionSelf( hwndDlg );
			
			char szType[ 256 ];
			strcpy( szType, "Unknown!!!" );

			switch ( g_Params.m_nType )
			{
			case CChoreoEvent::EXPRESSION:
				strcat( g_Params.m_szDialogTitle, " : Expression" );
				strcpy( szType, "Expression:" );
				break;
			case CChoreoEvent::FLEXANIMATION:
				strcat( g_Params.m_szDialogTitle, " : Flexanimation" );
				strcpy( szType, "Flex controller animation:" );
				break;
			case CChoreoEvent::LOOKAT:
				strcat( g_Params.m_szDialogTitle, " : Lookat" );
				strcpy( szType, "Look at actor:" );
				break;
			case CChoreoEvent::MOVETO:
				strcat( g_Params.m_szDialogTitle, " : Moveto" );
				strcpy( szType, "Move to actor:" );
				break;
			case CChoreoEvent::FACE:
				strcat( g_Params.m_szDialogTitle, " : Face" );
				strcpy( szType, "Face actor:" );
				break;
			case CChoreoEvent::SPEAK:
				strcat( g_Params.m_szDialogTitle, " : Speak" );
				strcpy( szType, "Speak .wav:" );
				break;
			case CChoreoEvent::GESTURE:
				strcat( g_Params.m_szDialogTitle, " : Gesture" );
				strcpy( szType, "Gesture:" );
				break;
			case CChoreoEvent::FIRETRIGGER:
				strcat( g_Params.m_szDialogTitle, " : Firetrigger" );
				strcpy( szType, "Scene Trigger:" );
				break;
			case CChoreoEvent::SUBSCENE:
				strcat( g_Params.m_szDialogTitle, " : Subscene" );
				strcpy( szType, "Sub-scene:" );
				break;
			case CChoreoEvent::SEQUENCE:
				strcat( g_Params.m_szDialogTitle, " : Sequence" );
				strcpy( szType, "Sequence:" );
				break;
			case CChoreoEvent::INTERRUPT:
				strcat( g_Params.m_szDialogTitle, " : Interrupt" );
				strcpy( szType, "Interrupt:" );
				ShowWindow( GetDlgItem( hwndDlg, IDC_CHECK_RESUMECONDITION ), SW_HIDE );
				break;
			default:
				break;
			}

			SetDlgItemText( hwndDlg, IDC_TYPENAME, szType );
			SetDlgItemText( hwndDlg, IDC_EVENTNAME, g_Params.m_szName );

			SetDlgItemText( hwndDlg, IDC_STARTTIME, va( "%f", g_Params.m_flStartTime ) );
			SetDlgItemText( hwndDlg, IDC_ENDTIME, va( "%f", g_Params.m_flEndTime ) );
			SendMessage( GetDlgItem( hwndDlg, IDC_CHECK_ENDTIME ), BM_SETCHECK, 
				( WPARAM ) g_Params.m_bHasEndTime ? BST_CHECKED : BST_UNCHECKED,
				( LPARAM )0 );

			if ( !g_Params.m_bHasEndTime )
			{
				ShowWindow( GetDlgItem( hwndDlg, IDC_ENDTIME ), SW_HIDE );
			}

			if ( g_Params.m_bFixedLength )
			{
				ShowWindow( GetDlgItem( hwndDlg, IDC_ENDTIME ), SW_HIDE );
				ShowWindow( GetDlgItem( hwndDlg, IDC_CHECK_ENDTIME ), SW_HIDE );
			}

			SendMessage( GetDlgItem( hwndDlg, IDC_CHECK_RESUMECONDITION ), BM_SETCHECK, 
				( WPARAM ) g_Params.m_bResumeCondition ? BST_CHECKED : BST_UNCHECKED,
				( LPARAM )0 );

			UpdateTagRadioButtons( hwndDlg, &g_Params );

			// Show/Hide dialog controls
			ShowControlsForEventType( hwndDlg, &g_Params );

			SetWindowText( hwndDlg, g_Params.m_szDialogTitle );

			SetFocus( GetDlgItem( hwndDlg, IDC_EVENTNAME ) );
		}
		return FALSE;  
		
    case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			switch ( g_Params.m_nType )
			{
			case CChoreoEvent::SPEAK:
				{
					char sz[ 512 ];
					GetDlgItemText( hwndDlg, IDC_EVENTCHOICES, sz, sizeof( sz ) );

					// Strip off game directory stuff
					strcpy( g_Params.m_szParameters, sz );
					char *p = strstr( sz, "\\sound\\" );
					if ( p )
					{
						strcpy( g_Params.m_szParameters, p + strlen( "\\sound\\" ) );
					}
				}

				break;
			case CChoreoEvent::SUBSCENE:
				{
					char sz[ 512 ];
					GetDlgItemText( hwndDlg, IDC_WAVNAME, sz, sizeof( sz ) );

					// Strip off game directory stuff
					strcpy( g_Params.m_szParameters, sz );
				}
				break;
			default:
				{
					HWND control = GetDlgItem( hwndDlg, IDC_EVENTCHOICES );
					if ( control )
					{
						SendMessage( control, WM_GETTEXT, (WPARAM)sizeof( g_Params.m_szParameters ), (LPARAM)g_Params.m_szParameters );
					}
				}
				break;
			}

			GetDlgItemText( hwndDlg, IDC_EVENTNAME, g_Params.m_szName, sizeof( g_Params.m_szName ) );

			char szTime[ 32 ];
			GetDlgItemText( hwndDlg, IDC_STARTTIME, szTime, sizeof( szTime ) );
			g_Params.m_flStartTime = atof( szTime );
			GetDlgItemText( hwndDlg, IDC_ENDTIME, szTime, sizeof( szTime ) );
			g_Params.m_flEndTime = atof( szTime );

			// Parse tokens from tags
			ParseTags( hwndDlg, &g_Params );

			EndDialog( hwndDlg, 1 );
			break;
        case IDCANCEL:
			EndDialog( hwndDlg, 0 );
			break;
		case IDC_SELECTWAV:
			{
				if ( g_Params.m_nType == CChoreoEvent::SPEAK )
				{
					OnSelectWaveFile( hwndDlg );
				}
				else if ( g_Params.m_nType == CChoreoEvent::SUBSCENE )
				{
					char workingdir[ 256 ];
					Q_getwd( workingdir );
					strlwr( workingdir );
					COM_FixSlashes( workingdir );

					const char *filename = NULL;

					bool inScenesAlready = false;

					if ( strstr( workingdir, va( "%s/scenes", GetGameDirectory() ) ) )
					{
						inScenesAlready = true;
					}
					// Show file io
					filename = mxGetOpenFileName( 
						0, 
						inScenesAlready ? "." : FacePoser_MakeWindowsSlashes( va( "%s/scenes/", GetGameDirectory() ) ), 
						"*.vcd" );

					if ( filename && filename[ 0 ] )
					{
						SetDlgItemText( hwndDlg, IDC_WAVNAME, filename );
					}
				}

			}
			break;
		case IDC_SHOW_ALL_SOUNDS:
			{
				bool showAll = SendMessage( GetDlgItem( hwndDlg, IDC_SHOW_ALL_SOUNDS ), BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false;

				PopulateSoundList( GetDlgItem( hwndDlg, IDC_EVENTCHOICES ), showAll );
			}
			break;
		case IDC_CHECK_ENDTIME:
			{
				g_Params.m_bHasEndTime = SendMessage( GetDlgItem( hwndDlg, IDC_CHECK_ENDTIME ), BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false;
				if ( !g_Params.m_bHasEndTime )
				{
					ShowWindow( GetDlgItem( hwndDlg, IDC_ENDTIME ), SW_HIDE );
				}
				else
				{
					ShowWindow( GetDlgItem( hwndDlg, IDC_ENDTIME ), SW_RESTORE );
				}
			}
			break;
		case IDC_CHECK_RESUMECONDITION:
			{
				g_Params.m_bResumeCondition = SendMessage( GetDlgItem( hwndDlg, IDC_CHECK_RESUMECONDITION ), BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false;
			}
			break;
		case IDC_EVENTCHOICES:
			{
				HWND control = (HWND)lParam;
				if ( control )
				{
					SendMessage( control, WM_GETTEXT, (WPARAM)sizeof( g_Params.m_szParameters ), (LPARAM)g_Params.m_szParameters );
					PopulateEventChoices2( hwndDlg, &g_Params );
				}
			}
			break;
		case IDC_EVENTCHOICES2:
			{
				HWND control = (HWND)lParam;
				if ( control )
				{
					SendMessage( control, WM_GETTEXT, (WPARAM)sizeof( g_Params.m_szParameters2 ), (LPARAM)g_Params.m_szParameters2 );
				}
			}
			break;
		case IDC_ABSOLUTESTART:
			{
				g_Params.m_bUsesTag = false;
				UpdateTagRadioButtons( hwndDlg, &g_Params );
			}
			break;
		case IDC_RELATIVESTART:
			{
				g_Params.m_bUsesTag = true;
				UpdateTagRadioButtons( hwndDlg, &g_Params );
			}
			break;
		case IDC_CHECK_LOOKAT:
			{
				HWND control = GetDlgItem( hwndDlg, IDC_CHECK_LOOKAT );
				bool checked = SendMessage( control, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false;
				if ( !checked )
				{
					g_Params.yaw = 0;
					g_Params.pitch = 0;

					SetPitchYawText( hwndDlg, &g_Params );

					control = GetDlgItem( hwndDlg, IDC_SLIDER_PITCH );
					SendMessage( control, TBM_SETPOS, 1, (LPARAM)(LONG)g_Params.pitch );
	
					control = GetDlgItem( hwndDlg, IDC_SLIDER_YAW );
					SendMessage( control, TBM_SETPOS, 1, (LPARAM)(LONG)g_Params.yaw );
				}

			}
			break;
		}
		return TRUE;
	case WM_HSCROLL:
		{
			HWND control = (HWND)lParam;
			if ( control == GetDlgItem( hwndDlg, IDC_SLIDER_YAW ) ||
				 control == GetDlgItem( hwndDlg, IDC_SLIDER_PITCH ) )
			{
				g_Params.yaw = (float)SendMessage( GetDlgItem( hwndDlg, IDC_SLIDER_YAW ), TBM_GETPOS, 0, 0 );
				g_Params.pitch = (float)SendMessage( GetDlgItem( hwndDlg, IDC_SLIDER_PITCH ), TBM_GETPOS, 0, 0 );

				SetPitchYawText( hwndDlg, &g_Params );

				control = GetDlgItem( hwndDlg, IDC_CHECK_LOOKAT );
				if ( g_Params.pitch != 0 ||
					 g_Params.yaw != 0 )
				{
					g_Params.usepitchyaw = true;
				}
				else
				{
					g_Params.usepitchyaw = false;
				}
				
				SendMessage( control, BM_SETCHECK, (WPARAM) g_Params.usepitchyaw ? BST_CHECKED : BST_UNCHECKED, 0 );

				return TRUE;
			}
		}
		return FALSE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *view - 
//			*actor - 
// Output : int
//-----------------------------------------------------------------------------
int EventProperties( CEventParams *params )
{
	g_Params = *params;

	int retval = DialogBox( (HINSTANCE)GetModuleHandle( 0 ), 
		MAKEINTRESOURCE( IDD_EVENTPROPERTIES ),
		(HWND)g_MDLViewer->getHandle(),
		(DLGPROC)EventPropertiesDialogProc );

	*params = g_Params;

	return retval;
}