//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include <mx/mx.h>
#include "expressions.h"
#include "hlfaceposer.h"
#include "StudioModel.h"
#include "cmdlib.h"
#include "viewersettings.h"
#include "matsyswin.h"
#include "checksum_crc.h"
#include "expclass.h"
#include "ControlPanel.h"
#include "faceposer_models.h"

static int g_counter = 0;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExpression::CExpression( void )
{
	name[ 0 ] = 0;
	index = 0;
	description[ 0 ] = 0;
	memset( setting, 0, sizeof( setting ) );

	for ( int i = 0; i < MAX_FP_MODELS; i++ )
	{
		m_Bitmap[ i ].valid = false;
	}

	m_nUndoCurrent = 0;
	m_bModified = false;

	m_bSelected = false;

	type = TYPE_NORMAL;

	m_nCurrentGroup = 0;

	m_nMarkovIndex = -1;

	m_bDirty = false;

	expressionclass[ 0 ] = 0;

	m_pOverrideClass = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Copy constructor
// Input  : from - 
//-----------------------------------------------------------------------------
CExpression::CExpression( const CExpression& from )
{
	int i;

	type = from.type;
	strcpy( name, from.name );
	index = from.index;
	strcpy( description, from.description );
	
	for ( i = 0; i < MAX_FP_MODELS; i++ )
	{
		m_Bitmap[ i ] = from.m_Bitmap[ i ];
	}

	m_bModified = from.m_bModified;

	for ( i = 0 ; i < from.undo.Size(); i++ )
	{
		CExpUndoInfo *newUndo = new CExpUndoInfo();
		*newUndo = *from.undo[ i ];
		undo.AddToTail( newUndo );
	}

	m_nUndoCurrent = from.m_nUndoCurrent;

	m_bSelected = from.m_bSelected;

	for ( i = 0; i < from.m_Group.Size(); i++ )
	{
		CMarkovGroup *g;
		int j = m_Group.AddToTail();
		g = &m_Group[ j ];

		g->expression = from.m_Group[ i ].expression;
		g->weight = from.m_Group[ i ].weight;
	}

	m_nCurrentGroup = from.m_nCurrentGroup;

	m_nMarkovIndex = from.m_nMarkovIndex;
	m_bDirty = from.m_bDirty;

	strcpy( expressionclass, from.expressionclass );

	memcpy( setting, from.setting, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
	memcpy( weight, from.weight, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );

	m_pOverrideClass = from.m_pOverrideClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExpression::~CExpression( void )
{
	ResetUndo();
	m_Group.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpression::GetDirty( void )
{
	return m_bDirty;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dirty - 
//-----------------------------------------------------------------------------
void CExpression::SetDirty( bool dirty )
{
	m_bDirty = dirty;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CExpression::GetCurrentMarkovGroupIndex( void )
{
	if ( type == TYPE_NORMAL )
	{
		Assert( 0 );
		return 0;
	}

	if ( m_nMarkovIndex == -1 )
	{
		NewMarkovIndex();
	}
	return m_nMarkovIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
// Output : static int
//-----------------------------------------------------------------------------
int RandomLong( int start, int end )
{
	return ( start + rand() % ( end - start + 1 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpression::NewMarkovIndex( void )
{
	if ( type != TYPE_MARKOV )
		return;

	int weighttotal = 0;
	int member = 0;
	for (int i = 0; i < m_Group.Size(); i++)
	{
		CMarkovGroup *group = &m_Group[ i ];
		if ( !group )
			continue;

		weighttotal += group->weight;
		if ( !weighttotal || RandomLong(0,weighttotal-1) < group->weight )
		{
			member = i;
		}
	}

	m_nMarkovIndex = member;
};

//-----------------------------------------------------------------------------
// Purpose: Get markov expression if it's a markov type
// Output : CExpression
//-----------------------------------------------------------------------------
CExpression *CExpression::GetExpression( void )
{
	if ( type != TYPE_NORMAL )
	{
		// Use the created on or create a new one
		int groupmember = GetCurrentMarkovGroupIndex();
		Assert( groupmember >= 0 );
		Assert( groupmember < m_Group.Size() );
		CMarkovGroup *g = &m_Group[ groupmember ];
		CExpClass *cl = GetExpressionClass();
		if ( cl )
		{
			CExpression *exp = cl->GetExpression( g->expression );
			Assert( exp );
			return exp->GetExpression();
		}
		else
		{
			Assert( 0 );
		}
	}

	return this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float *CExpression::GetSettings( void )
{
	if ( type == TYPE_NORMAL )
	{
		return setting;
	}
	else
	{
		// Use the created on or create a new one
		int groupmember = GetCurrentMarkovGroupIndex();
		Assert( groupmember >= 0 );
		Assert( groupmember < m_Group.Size() );
		CMarkovGroup *g = &m_Group[ groupmember ];
		CExpClass *cl = GetExpressionClass();
		if ( cl )
		{
			CExpression *exp = cl->GetExpression( g->expression );
			Assert( exp );
			return exp->GetSettings();
		}
		else
		{
			Assert( 0 );
		}
	}

	return setting;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float *CExpression::GetWeights( void )
{
	if ( type == TYPE_NORMAL )
	{
		return weight;
	}
	else
	{
		// Use the created on or create a new one
		int groupmember = GetCurrentMarkovGroupIndex();
		Assert( groupmember >= 0 );
		Assert( groupmember < m_Group.Size() );
		CMarkovGroup *g = &m_Group[ groupmember ];
		CExpClass *cl = GetExpressionClass();
		if ( cl )
		{
			CExpression *exp = cl->GetExpression( g->expression );
			Assert( exp );
			return exp->GetWeights();
		}
		else
		{
			Assert( 0 );
		}
	}

	return weight;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : num - 
// Output : CMarkovGroup
//-----------------------------------------------------------------------------
CMarkovGroup *CExpression::GetGroup( int num )
{
	return &m_Group[ num ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CExpression::GetNumGroups( void )
{
	return m_Group.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CExpression::GetCurrentGroupIndex( void )
{
	Assert( type == TYPE_MARKOV );
	if ( type == TYPE_NORMAL )
		return 0;

	return m_nCurrentGroup;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CExpression
//-----------------------------------------------------------------------------
CMarkovGroup *CExpression::GetCurrentGroup( void )
{
	Assert( type == TYPE_MARKOV );
	if ( type == TYPE_NORMAL )
		return NULL;

	if ( m_nCurrentGroup >= m_Group.Size() || 
		m_nCurrentGroup < 0 )
		return NULL;

	return &m_Group[ m_nCurrentGroup ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpression::NextGroup( void )
{
	Assert( type == TYPE_MARKOV );
	if ( type == TYPE_NORMAL )
		return;

	m_nCurrentGroup = m_nCurrentGroup + 1;
	if ( m_nCurrentGroup >= m_Group.Size() )
	{
		m_nCurrentGroup = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpression::PrevGroup( void )
{
	Assert( type == TYPE_MARKOV );
	if ( type == TYPE_NORMAL )
		return;

	m_nCurrentGroup = m_nCurrentGroup - 1;
	if ( m_nCurrentGroup < 0 )
	{
		m_nCurrentGroup = max( 0, m_Group.Size() - 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : etype - 
//-----------------------------------------------------------------------------
void CExpression::SetType( EXPRESSIONTYPE etype )
{
	type = etype;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CExpression::EXPRESSIONTYPE
//-----------------------------------------------------------------------------
CExpression::EXPRESSIONTYPE CExpression::GetType( void )
{
	return type;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *expression - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
CMarkovGroup *CExpression::FindExpressionInGroup( CExpression *expression )
{
	CExpClass *cl = GetExpressionClass();
	if ( !cl )
	{
		Assert( 0 );
		return NULL;
	}

	for ( int i = 0; i < m_Group.Size(); i++ )
	{
		if ( cl->GetExpression( m_Group[ i ].expression ) == expression )
			return &m_Group [ i ];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *expression - 
//			weight - 
//-----------------------------------------------------------------------------
void CExpression::AddToGroup( CExpression *expression, int weight )
{
	CExpClass *cl = GetExpressionClass();
	if ( !cl )
	{
		Assert( 0 );
		return;
	}

	if ( FindExpressionInGroup( expression ) )
		return;

	// Don't allow recursive markov groups for now
	if ( expression->type != TYPE_NORMAL )
		return;

	// Keep sorted
	int idx = 0;
	CMarkovGroup *p = NULL;

	if ( m_Group.Size() ==  0 )
	{
		idx = m_Group.AddToTail();
		p = &m_Group[ idx ];
	}
	else
	{
		// Keep list sorted
		idx = 0;
		while ( idx < m_Group.Size() )
		{
			p = &m_Group[ idx ];
			if ( weight < p->weight )
			{
				idx++;
			}
			else
			{
				idx = m_Group.InsertBefore( idx );
				p = &m_Group[ idx ];
				break;
			}
		}

		if ( idx >= m_Group.Size() )
		{
			idx = m_Group.AddToTail();
			p = &m_Group[ idx ];
		}
	}
	
	Assert( p );
	p->expression = cl->FindExpressionIndex( expression );
	p->weight = weight;

	m_nCurrentGroup = idx;

	NewMarkovIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *expression - 
//-----------------------------------------------------------------------------
void CExpression::RemoveFromGroup( CExpression *expression )
{
	CMarkovGroup *markov = FindExpressionInGroup( expression );
	if ( !markov )
		return;

	if ( m_Group.Size() <= 1 )
	{
		Con_Printf( "Can't remove last member of group, you should delete the group instead\n" );
		return;
	}

	for ( int i = 0; i < m_Group.Size(); i++ )
	{
		if ( &m_Group[ i ] == markov )
		{
			m_Group.Remove( i );
			break;
		}
	}

	m_nCurrentGroup = 0;

	NewMarkovIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *expression - 
//			weight - 
//-----------------------------------------------------------------------------
void CExpression::SetWeight( CExpression *expression, int weight )
{
	CMarkovGroup *markov = FindExpressionInGroup( expression );
	if ( !markov )
	{
		Con_Printf( "%s is not in expression group for %s\n",
			expression->name, name );
		return;
	}

	markov->weight = weight;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpression::ClearGroup( void )
{
	m_Group.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mod - 
//-----------------------------------------------------------------------------
void CExpression::SetModified( bool mod )
{
	m_bModified = mod;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpression::GetModified( void )
{
	return m_bModified;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : selected - 
//-----------------------------------------------------------------------------
void CExpression::SetSelected( bool selected )
{
	m_bSelected = selected;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpression::GetSelected( void )
{
	return m_bSelected;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpression::ResetUndo( void )
{
	CExpUndoInfo *u;
	for ( int i = 0; i < undo.Size(); i++ )
	{
		u = undo[ i ];
		delete u;
	}

	undo.RemoveAll();
	m_nUndoCurrent = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpression::CanRedo( void )
{
	if ( type == TYPE_MARKOV )
		return false;

	if ( !undo.Size() )
		return false;

	if ( m_nUndoCurrent == 0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpression::CanUndo( void )
{
	if ( type == TYPE_MARKOV )
		return false;

	if ( !undo.Size() )
		return false;

	if ( m_nUndoCurrent >= undo.Size() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CExpression::UndoLevels( void )
{
	if ( type == TYPE_MARKOV )
		return 0;

	return undo.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CExpression::UndoCurrent( void )
{
	if ( type == TYPE_MARKOV )
		return 0;

	return m_nUndoCurrent;
}

static char *COM_BinPrintf( unsigned char *buf, int nLen )
{
	static char szReturn[4096];
	unsigned char c;
	char szChunk[10];
	int i;

	memset(szReturn, 0, sizeof( szReturn ) );

	for (i = 0; i < nLen; i++)
	{
		c = (unsigned char)buf[i];
		sprintf(szChunk, "%02x", c);
		strcat(szReturn, szChunk);
	}

	return szReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CExpression::GetBitmapCheckSum( void )
{
	CRC32_t crc;
	CRC32_Init( &crc );

	float *s = setting;
	float *w = weight;

	// Note, we'll use the pristine values if this has changed
	if ( undo.Size() >= 1 )
	{
		s = undo[ undo.Size() - 1 ]->setting;
		w = undo[ undo.Size() - 1 ]->weight;
	}
	CRC32_ProcessBuffer( &crc, (void *)s, sizeof( setting ) );
	CRC32_ProcessBuffer( &crc, (void *)w, sizeof( weight ) );
	CRC32_Final( &crc );

	// Create string name out of binary data
	static char filename[ 512 ];
	sprintf( filename, "%s", COM_BinPrintf( (unsigned char *)&crc, 4 ) );
	return filename;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CExpression::GetBitmapFilename( int modelindex )
{
	static char outfile[ 256 ];
	char *in, *out;
	char filename[ 256 ];
	filename[ 0 ] = 0;
	char const *classname = "error";
	CExpClass *cl = GetExpressionClass();
	if ( cl )
	{
		classname = cl->GetName();
	}
	else
	{
		Assert( 0 );
	}


	sprintf( filename, "%s/expressions/%s/%s/%s.bmp", GetGameDirectory(), models->GetModelName( modelindex ), classname, GetBitmapCheckSum() );

	COM_FixSlashes( filename );
	strlwr( filename );
	
	in = filename;
	out = outfile;

	while ( *in )
	{
		if ( isalnum( *in ) ||
			*in == '_' || 
			*in == '\\' || 
			*in == '/' ||
			*in == '.' ||
			*in == ':' )
		{
			*out++ = *in;
		}
		in++;
	}
	*out = 0;

	CreatePath( outfile );
	
	return outfile;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpression::CreateNewBitmap( int modelindex )
{
	MatSysWindow *pWnd = g_pMatSysWindow;
	if ( !pWnd ) 
		return;

	StudioModel *model = models->GetStudioModel( modelindex );
	if ( !model )
		return;

	studiohdr_t *hdr = models->GetStudioHeader( modelindex );
	if ( !hdr )
		return;

	char filename[ 256 ];
	strcpy( filename, GetBitmapFilename( modelindex ) );
	if ( !strstr( filename, ".bmp" ) )
		return;

	Con_ColorPrintf( FILE_R, FILE_G, FILE_B, "Creating bitmap %s for expression '%s'\n", filename, name );

	int oldsequence = model->GetSequence();
	float oldCycle = model->m_cycle;

	model->SetSequence( 0 );
	model->SetPoseParameter( 0, 0.0 );
	model->SetPoseParameter( 1, 0.0 );

	QAngle oldrot, oldLight;
	Vector oldtrans;
	
	VectorCopy( g_viewerSettings.rot, oldrot );
	VectorCopy( g_viewerSettings.trans, oldtrans );
	VectorCopy( g_viewerSettings.lightrot, oldLight );

	g_viewerSettings.rot.Init();
	g_viewerSettings.trans.Init();
	g_viewerSettings.lightrot.Init();

	g_viewerSettings.lightrot.y = -180;

	Vector size;
	VectorSubtract( hdr->hull_max, hdr->hull_min, size );

	float eyeheight = hdr->hull_min.z + 0.9 * size.z;
	float width = width = ( size.x + size.y ) / 2.0f;

	g_viewerSettings.trans.x = size.z * .6f;
	g_viewerSettings.trans.z += eyeheight;

	pWnd->SuppressResize( true );

	int snapshotsize = 128;

	RECT rcClient;
	HWND wnd = (HWND)pWnd->getHandle();

	WINDOWPLACEMENT wp;

	GetWindowPlacement( wnd, &wp );

	GetClientRect( wnd, &rcClient );

	MoveWindow( wnd, 0, 0, snapshotsize + 16, snapshotsize + 16, TRUE );

	// Snapshots are taken of the back buffer; 
	// we need to render to the back buffer but not move it to the front
	pWnd->SuppressBufferSwap( true );
	pWnd->redraw();
	pWnd->SuppressBufferSwap( false );

	// make it square, assumes w > h
	pWnd->TakeSnapshotRect( filename, 0, 0, snapshotsize, snapshotsize );

	// Move back to original position
	SetWindowPlacement( wnd, &wp );

	pWnd->SuppressResize( false );

	VectorCopy( oldrot, g_viewerSettings.rot );
	VectorCopy( oldtrans, g_viewerSettings.trans );
	VectorCopy( oldLight, g_viewerSettings.lightrot );

	model->SetSequence( oldsequence );
	model->m_cycle = oldCycle;

	// Now try and load the new bitmap
	if ( m_Bitmap[ modelindex ].valid )
	{
		DeleteObject( m_Bitmap[ modelindex ].image );
		m_Bitmap[ modelindex ].valid = false;
	}
	LoadBitmapFromFile( GetBitmapFilename( modelindex ), m_Bitmap[ modelindex ] );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *exp - 
//-----------------------------------------------------------------------------
void CExpression::PushUndoInformation( void )
{
	SetModified( true );

	// A real change to the data wipes out the redo counters
	WipeRedoInformation();

	CExpUndoInfo *newundo = new CExpUndoInfo;

	memcpy( newundo->setting, setting, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
	memset( newundo->redosetting, 0, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
	memcpy( newundo->weight, weight, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
	memset( newundo->redoweight, 0, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );

	newundo->counter = g_counter++;

	undo.AddToHead( newundo );

	Assert( m_nUndoCurrent == 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *exp - 
//-----------------------------------------------------------------------------
void CExpression::PushRedoInformation( void )
{
	Assert( undo.Size() >= 1 );

	CExpUndoInfo *redo = undo[ 0 ];
	memcpy( redo->redosetting, setting, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
	memcpy( redo->redoweight, weight, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *exp - 
//-----------------------------------------------------------------------------
void CExpression::Undo( void )
{
	if ( !CanUndo() )
		return;

	Assert( m_nUndoCurrent < undo.Size() );

	CExpUndoInfo *u = undo[ m_nUndoCurrent++ ];
	Assert( u );
	
	memcpy( setting, u->setting, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
	memcpy( weight, u->weight, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *exp - 
//-----------------------------------------------------------------------------
void CExpression::Redo( void )
{
	if ( !CanRedo() )
		return;

	Assert( m_nUndoCurrent >= 1 );
	Assert( m_nUndoCurrent <= undo.Size() );

	CExpUndoInfo *u = undo[ --m_nUndoCurrent ];
	Assert( u );
	
	memcpy( setting, u->redosetting, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
	memcpy( weight, u->redoweight, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *exp - 
//-----------------------------------------------------------------------------
void CExpression::WipeRedoInformation( void )
{
	// Wipe out all stuff newer then m_nUndoCurrent
	int level = 0;
	while ( level < m_nUndoCurrent )
	{
		CExpUndoInfo *u = undo[ 0 ];
		undo.Remove( 0 );
		Assert( u );
		delete u;
		level++;
	}

	m_nUndoCurrent = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpression::HasOverride( void ) const
{
	if ( m_pOverrideClass && m_pOverrideClass->FindExpression( name ) )
	{
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CExpression
//-----------------------------------------------------------------------------
CExpression *CExpression::GetOverride( void )
{
	if ( !m_pOverrideClass )
		return NULL;

	return m_pOverrideClass->FindExpression( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *override - 
//-----------------------------------------------------------------------------
void CExpression::SetOverride( CExpClass *overrideClass )
{
	m_pOverrideClass = overrideClass;
}

//-----------------------------------------------------------------------------
// Purpose: Revert to last saved state
//-----------------------------------------------------------------------------
void CExpression::Revert( void )
{
	SetDirty( false );

	if ( undo.Size() <= 0 )
		return;

	// Go back to original data
	CExpUndoInfo *u = undo[ undo.Size() - 1 ];
	Assert( u );

	memcpy( setting, u->setting, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
	memcpy( weight, u->weight, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );

	ResetUndo();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CExpClass
//-----------------------------------------------------------------------------
CExpClass *CExpression::GetExpressionClass( void )
{
	CExpClass *cl = expressions->FindClass( expressionclass );
	Assert( cl );
	return cl;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classname - 
//-----------------------------------------------------------------------------
void CExpression::SetExpressionClass( char const *classname )
{
	strcpy( expressionclass, classname );
}

