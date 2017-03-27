//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <mx/mx.h>
#include "expressions.h"
#include "expclass.h"
#include "hlfaceposer.h"
#include "StudioModel.h"
#include "cmdlib.h"
#include "FlexPanel.h"
#include "ControlPanel.h"
#include "mxExpressionTray.h"
#include "UtlBuffer.h"
#include "FileSystem.h"
#include "ExpressionTool.h"
#include "faceposer_models.h"
#include "mdlviewer.h"
#include "phonemeconverter.h"

#define ALIGN4( a ) a = (byte *)((int)((byte *)a + 3) & ~ 3)
#define ALIGN16( a ) a = (byte *)((int)((byte *)a + 15) & ~ 15)

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classname - 
//-----------------------------------------------------------------------------
CExpClass::CExpClass( const char *classname ) 
{ 
	strcpy( m_szClassName, classname ); 
	m_szFileName[ 0 ] = 0;
	m_bDirty = false;
	m_nSelectedExpression = -1;

	m_bIsOverride = false;
	m_pOverride = NULL;
	m_bIsPhonemeClass = strstr( classname, "phonemes" ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExpClass::~CExpClass( void )
{
	m_Expressions.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *exp - 
//-----------------------------------------------------------------------------
int	CExpClass::FindExpressionIndex( CExpression *exp )
{
	for ( int i = 0 ; i < GetNumExpressions(); i++ )
	{
		CExpression *e = GetExpression( i );
		if ( e == exp )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpClass::Save( void )
{
	// Save overridden elements
	if ( !IsOverrideClass() && HasOverrideClass() )
	{
		CExpClass *oc = GetOverrideClass();
		Assert( oc );
		oc->Save();
	}

	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
	{
		return;
	}

	const char *filename = GetFileName();
	if ( !filename || !filename[ 0 ] )
		return;

	Con_Printf( "Saving changes to %s to file %s\n", GetName(), GetFileName() );

	CUtlBuffer buf( 0, 0, true );

	int i, j;

	int numflexmaps = 0;
	int flexmap[128]; // maps file local controlls into global controls

	CExpression *expr = NULL;
	// find all used controllers
	for (j = 0; j < hdr->numflexcontrollers; j++)
	{
		for (i = 0; i < GetNumExpressions(); i++)
		{
			expr = GetExpression( i );
			Assert( expr );

			if ( expr->GetType() == CExpression::TYPE_MARKOV )
				continue;

			float *settings = expr->GetSettings();
			float *weights = expr->GetWeights();
			Assert( settings );
			Assert( weights );

			if ( settings[j] != 0 || weights[j] != 0)
			{
				flexmap[numflexmaps++] = j;
				break;
			}
		}
	}

	buf.Printf( "$keys" );
	for (j = 0; j < numflexmaps; j++)
	{
		buf.Printf( " %s", hdr->pFlexcontroller( flexmap[j] )->pszName() );
	}
	buf.Printf( "\n" );

	buf.Printf( "$hasweighting\n" );

	for (i = 0; i < GetNumExpressions(); i++)
	{
		expr = GetExpression( i );

		if ( expr->GetType() == CExpression::TYPE_NORMAL )
		{
			buf.Printf( "\"%s\" ", expr->name );

			// isalpha returns non zero for ents > 256
			if (expr->index <= 'z') 
			{
				buf.Printf( "\"%c\" ", expr->index );
			}
			else
			{
				buf.Printf( "\"0x%04x\" ", expr->index );
			}

			float *settings = expr->GetSettings();
			float *weights = expr->GetWeights();
			Assert( settings );
			Assert( weights );

			for (j = 0; j < numflexmaps; j++)
			{
				buf.Printf( "%.3f %.3f ", settings[flexmap[j]], weights[flexmap[j]] );
			}

			if ( strstr( expr->name, "Right Side Smile" ) )
			{
				Con_Printf( "wrote %s with checksum %s\n",
					expr->name, expr->GetBitmapCheckSum() );
			}

			buf.Printf( "\"%s\"\n", expr->description );
		}
		else
		{
			buf.Printf( "$groupname \"%s\" ", expr->name );
			buf.Printf( "\"%s\"\n", expr->description );
		}
	}

	for (i = 0; i < GetNumExpressions(); i++)
	{
		expr = GetExpression( i );

		if ( expr->GetType() == CExpression::TYPE_NORMAL )
			continue;

		buf.Printf( "$group \"%s\" ", expr->name );
		for ( j = 0; j < expr->m_Group.Size(); j++ )
		{
			CMarkovGroup *g = &expr->m_Group[ j ];
			CExpression *me = GetExpression( g->expression );
			Assert( me );

			buf.Printf( "\"%s\" %i", me->name, g->weight );
			if ( j != expr->m_Group.Size() - 1 )
			{
				buf.Printf( " " );
			}
		}
		buf.Printf( "\n" );
	}

	char relative[ 512 ];
	strcpy( relative, filename );
	if ( strchr( filename, ':' ) )
	{
		filesystem->FullPathToRelativePath( filename, relative );
	}

	MakeFileWriteable( relative );
	FileHandle_t fh = filesystem->Open( relative, "wt" );
	if ( !fh )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Unable to write to %s (read-only?)\n", relative );
		return;
	}
	else
	{
		filesystem->Write( buf.Base(), buf.TellPut(), fh );
		filesystem->Close(fh);
	}

	SetDirty( false );

	for (i = 0; i < GetNumExpressions(); i++)
	{
		expr = GetExpression( i );
		if ( expr )
		{
			expr->ResetUndo();
			expr->SetDirty( false );
		}
	}
}

char const *GetGlobalFlexControllerName( int index );
int GetGlobalFlexControllerCount( void );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpClass::Export( void )
{
	// Export overridden elements
	if ( !IsOverrideClass() && HasOverrideClass() )
	{
		CExpClass *oc = GetOverrideClass();
		Assert( oc );
		oc->Export();
	}

	char vfefilename[ 512 ];
	strcpy( vfefilename, GetFileName() );
	StripExtension( vfefilename );
	DefaultExtension( vfefilename, ".vfe" );

	Con_Printf( "Exporting %s to %s\n", GetName(), vfefilename );

	int i, j;

	int numflexmaps = 0;
	int flexmap[128]; // maps file local controlls into global controls
	CExpression *expr = NULL;

	// find all used controllers
	int fc_count = GetGlobalFlexControllerCount();

	for (j = 0; j < fc_count; j++)
	{
		int k = j;

		for (i = 0; i < GetNumExpressions(); i++)
		{
			expr = GetExpression( i );
			Assert( expr );

			float *settings = expr->GetSettings();
			float *weights = expr->GetWeights();
			Assert( settings );
			Assert( weights );

			if ( settings[k] != 0 || weights[k] != 0 )
			{
				flexmap[numflexmaps++] = k;
				break;
			}
		}
	}

	byte *pData = (byte *)calloc( 1024 * 1024, 1 );
	byte *pDataStart = pData;

	flexsettinghdr_t *fhdr = (flexsettinghdr_t *)pData;
	
	fhdr->id = ('V' << 16) + ('F' << 8) + ('E');
	fhdr->version = 0;
	strcpy( fhdr->name, vfefilename );

	// allocate room for header
	pData += sizeof( flexsettinghdr_t );
	ALIGN4( pData );

	// store flex settings
	flexsetting_t *pSetting = (flexsetting_t *)pData;
	fhdr->numflexsettings = GetNumExpressions();
	fhdr->flexsettingindex = pData - pDataStart;
	pData += sizeof( flexsetting_t ) * fhdr->numflexsettings;
	ALIGN4( pData );
	for (i = 0; i < fhdr->numflexsettings; i++)
	{
		expr = GetExpression( i );
		Assert( expr );

		pSetting[i].index = expr->index;
		pSetting[i].settingindex = pData - (byte *)(&pSetting[i]);

		// Make sure this is zero by default
		pSetting[i].currentindex = 0;

		if ( expr->GetType() != CExpression::TYPE_MARKOV )
		{
			pSetting[i].type = CExpression::TYPE_NORMAL;

			flexweight_t *pFlexWeights = (flexweight_t *)pData;

			float *settings = expr->GetSettings();
			float *weights = expr->GetWeights();
			Assert( settings );
			Assert( weights );

			for (j = 0; j < numflexmaps; j++)
			{
				if (settings[flexmap[j]] != 0 || weights[flexmap[j]] != 0)
				{
					pSetting[i].numsettings++;
					pFlexWeights->key = j;
					pFlexWeights->weight = settings[flexmap[j]];
					pFlexWeights->influence = weights[flexmap[j]];
					pFlexWeights++;
				}
				pData = (byte *)pFlexWeights;
				ALIGN4( pData );
			}
		}
		else if ( expr->GetType() == CExpression::TYPE_MARKOV )
		{
			pSetting[i].type = CExpression::TYPE_MARKOV;

			flexmarkovgroup_t *pMarkovGroups = (flexmarkovgroup_t *)pData;

			for (j = 0; j < expr->GetNumGroups(); j++)
			{
				CMarkovGroup *group = expr->GetGroup( j );
				Assert( group );

				// Skip it if can't find it
				int expindex = FindExpressionIndex( GetExpression( group->expression ) );
				if ( expindex == -1 )
					continue;

				pSetting[i].numsettings++;
				pMarkovGroups->settingnumber = expindex;
				pMarkovGroups->weight = group->weight;
				pMarkovGroups++;

				pData = (byte *)pMarkovGroups;
				ALIGN4( pData );
			}
		}
		else
		{
			// Shouldn't ever happen
			Assert( 0 );
		}
	}

	// store indexed table
	int numindexes = 1;
	for (i = 0; i < fhdr->numflexsettings; i++)
	{
		if (pSetting[i].index >= numindexes)
			numindexes = pSetting[i].index + 1;
	}

	int *pIndex = (int *)pData;
	fhdr->numindexes = numindexes;
	fhdr->indexindex = pData - pDataStart;
	pData += sizeof( int ) * numindexes;
	ALIGN4( pData );
	for (i = 0; i < numindexes; i++)
	{
		pIndex[i] = -1;
	}
	for (i = 0; i < fhdr->numflexsettings; i++)
	{
		pIndex[pSetting[i].index] = i;
	}

	// store flex setting names
	for (i = 0; i < fhdr->numflexsettings; i++)
	{
		expr = GetExpression( i );
		pSetting[i].nameindex = pData - (byte *)(&pSetting[i]);
		strcpy( (char *)pData,  expr->name );
		pData += strlen( expr->name ) + 1;
	}
	ALIGN4( pData );

	// store key names
	char **pKeynames = (char **)pData;
	fhdr->numkeys = numflexmaps;
	fhdr->keynameindex = pData - pDataStart;
	pData += sizeof( char *) * numflexmaps;
	
	for (i = 0; i < numflexmaps; i++)
	{
		pKeynames[i] = (char *)(pData - pDataStart);
		strcpy( (char *)pData,  GetGlobalFlexControllerName( flexmap[i] ) );
		pData += strlen( GetGlobalFlexControllerName( flexmap[i] ) ) + 1;
	}
	ALIGN4( pData );

	// allocate room for remapping
	int *keymapping = (int *)pData;
	fhdr->keymappingindex = pData - pDataStart;
	pData += sizeof( int ) * numflexmaps;
	for (i = 0; i < numflexmaps; i++)
	{
		keymapping[i] = -1;
	}
	ALIGN4( pData );

	fhdr->length = pData - pDataStart;

	char relative[ 512 ];
	strcpy( relative, vfefilename );
	if ( strchr( vfefilename, ':' ) )
	{
		filesystem->FullPathToRelativePath( vfefilename, relative );
	}


	MakeFileWriteable( relative );
	FileHandle_t fh = filesystem->Open( relative, "wb" );
	if ( !fh )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Unable to write to %s (read-only?)\n", relative );
		return;
	}
	else
	{
		filesystem->Write( pDataStart, fhdr->length, fh );
		filesystem->Close(fh);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CExpClass::GetName( void )
{
	return m_szClassName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CExpClass::GetFileName( void )
{
	return m_szFileName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void CExpClass::SetFileName( const char *filename )
{
	strcpy( m_szFileName, filename );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			*description - 
// Output : CExpression
//-----------------------------------------------------------------------------
CExpression *CExpClass::AddGroupExpression( const char *name, const char *description )
{
	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
		return NULL;

	CExpression *exp = FindExpression( name );
	if ( exp )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Can't create, an expression with the name '%s' already exists.\n", name );
		return NULL;
	}

	// Add to end of list
	int idx = m_Expressions.AddToTail();

	exp = &m_Expressions[ idx ];

	exp->SetExpressionClass( GetName() );
	strcpy( exp->name, name );
	strcpy( exp->description, description );
	exp->index = '_';

	if ( IsPhonemeClass() )
	{
		exp->index = TextToPhoneme( name );
	}

	exp->SetType( CExpression::TYPE_MARKOV );

	SetDirty( true );

	exp->m_Bitmap[ models->GetActiveModelIndex() ].valid = false;

	SetDirty( true );

	return exp;
}

void CExpClass::ReloadBitmaps( void )
{
	int c = models->Count();
	for ( int model = 0; model < MAX_FP_MODELS; model++ )
	{
		for ( int i = 0 ; i < GetNumExpressions(); i++ )
		{
			CExpression *e = GetExpression( i );
			if ( !e )
				continue;

			if ( e->m_Bitmap[ model ].valid )
			{
				DeleteObject( e->m_Bitmap[ model ].image );
				e->m_Bitmap[ model ].valid = false;
			}

			if ( model >= c )
				continue;

			if ( !LoadBitmapFromFile( e->GetBitmapFilename( model ), e->m_Bitmap[ model ] ) )
			{
				// Make sure it's active if we're going to take a snapshot!!!
				if ( models->GetActiveModelIndex() != model )
				{
					g_MDLViewer->SetActiveModelTab( model );
				}

				SelectExpression( i );
				e->CreateNewBitmap( model );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			*description - 
//			*flexsettings - 
//			selectnewitem - 
// Output : CExpression
//-----------------------------------------------------------------------------
CExpression *CExpClass::AddExpression( const char *name, const char *description, float *flexsettings, float *flexweights, bool selectnewitem )
{
	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
		return NULL;

	CExpression *exp = FindExpression( name );
	if ( exp )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Can't create, an expression with the name '%s' already exists.\n", name );
		return NULL;
	}

	// Add to end of list
	int idx = m_Expressions.AddToTail();

	exp = &m_Expressions[ idx ];

	float *settings = exp->GetSettings();
	float *weights = exp->GetWeights();
	Assert( settings );
	Assert( weights );

	exp->SetExpressionClass( GetName() );
	strcpy( exp->name, name );
	strcpy( exp->description, description );
	memcpy( settings, flexsettings, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
	memcpy( weights, flexweights, GLOBAL_STUDIO_FLEX_CONTROL_COUNT * sizeof( float ) );
	exp->index = '_';

	if ( IsPhonemeClass() )
	{
		exp->index = TextToPhoneme( name );
	}

	SetDirty( true );

	exp->m_Bitmap[ models->GetActiveModelIndex() ].valid = false;
	if ( !LoadBitmapFromFile( exp->GetBitmapFilename( models->GetActiveModelIndex() ), exp->m_Bitmap[ models->GetActiveModelIndex() ] ) )
	{
		SelectExpression( idx );
		exp->CreateNewBitmap( models->GetActiveModelIndex() );
	}
	
	if ( selectnewitem )
	{
		SelectExpression( idx );
	}

	SetDirty( true );

	return exp;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : CExpression
//-----------------------------------------------------------------------------
CExpression *CExpClass::FindExpression( const char *name )
{
	for ( int i = 0 ; i < m_Expressions.Size(); i++ )
	{
		CExpression *exp = &m_Expressions[ i ];
		if ( !stricmp( exp->name, name ) )
		{
			return exp;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CExpClass::DeleteExpression( const char *name )
{

	for ( int i = 0 ; i < m_Expressions.Size(); i++ )
	{
		CExpression *exp = &m_Expressions[ i ];
		if ( !stricmp( exp->name, name ) )
		{
			// If this class hav overrides, remove it from there, too
			if ( !IsOverrideClass() && HasOverrideClass() )
			{
				CExpClass *oc = GetOverrideClass();
				if ( oc )
				{
					oc->DeleteExpression( name );
				}
			}

			SetDirty( true );

			m_Expressions.Remove( i );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CExpClass::GetNumExpressions( void )
{
	return m_Expressions.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : num - 
// Output : CExpression
//-----------------------------------------------------------------------------
CExpression *CExpClass::GetExpression( int num )
{
	if ( num < 0 || num >= m_Expressions.Size() )
	{
		return NULL;
	}

	CExpression *exp = &m_Expressions[ num ];
	return exp;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpClass::GetDirty( void )
{
	return m_bDirty;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dirty - 
//-----------------------------------------------------------------------------
void CExpClass::SetDirty( bool dirty )
{
	m_bDirty = dirty;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CExpClass::GetIndex( void )
{
	for ( int i = 0; i < expressions->GetNumClasses(); i++ )
	{
		CExpClass *cl = expressions->GetClass( i );
		if ( cl == this )
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : num - 
//-----------------------------------------------------------------------------
void CExpClass::SelectExpression( int num, bool deselect )
{
	m_nSelectedExpression = num;

	g_pFlexPanel->setExpression( num );
	g_pExpressionTrayTool->Select( num, deselect );
	g_pExpressionTrayTool->redraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CExpClass::GetSelectedExpression( void )
{
	return m_nSelectedExpression;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpClass::DeselectExpression( void )
{
	m_nSelectedExpression = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : exp1 - 
//			exp2 - 
//-----------------------------------------------------------------------------
void CExpClass::SwapExpressionOrder( int exp1, int exp2 )
{
	CExpression temp1 = m_Expressions[ exp1 ];
	CExpression temp2 = m_Expressions[ exp2 ];

	m_Expressions.Remove( exp1 );
	m_Expressions.InsertBefore( exp1, temp2 );
	m_Expressions.Remove( exp2 );
	m_Expressions.InsertBefore( exp2, temp1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpClass::NewMarkovIndices( void )
{
	for ( int i = 0; i < m_Expressions.Size(); i++ )
	{
		CExpression *exp = &m_Expressions[ i ];
		if ( !exp || ( exp->GetType() != CExpression::TYPE_MARKOV ) )
			continue;

		exp->NewMarkovIndex();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *checksum - 
// Output : CExpression
//-----------------------------------------------------------------------------
CExpression *CExpClass::FindBitmapForChecksum( const char *checksum )
{
	for ( int i = 0; i < m_Expressions.Size(); i++ )
	{
		CExpression *exp = &m_Expressions[ i ];
		if ( !exp )
			continue;

		const char *checkval = exp->GetBitmapCheckSum();
		Assert( checkval );
		if ( !checkval )
			continue;

		if ( !stricmp( checksum, checkval ) )
			return exp;
	}

	if ( !IsOverrideClass() && HasOverrideClass() )
	{
		CExpClass *oc = GetOverrideClass();
		return oc->FindBitmapForChecksum( checksum );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: After a class is loaded, check the class directory and delete any bmp files that aren't
//  still referenced
//-----------------------------------------------------------------------------
void CExpClass::CheckBitmapConsistency( void )
{
	if ( IsOverrideClass() )
		return;

	char path[ 512 ];
	HANDLE hFindFile;
	WIN32_FIND_DATA wfd;

	sprintf( path, "%s/expressions/%s/%s/*.bmp", GetGameDirectory(), models->GetActiveModelName(), GetName() );

	hFindFile = FindFirstFile( path, &wfd );
	if ( hFindFile != INVALID_HANDLE_VALUE )
	{
		bool morefiles = true;
		while ( morefiles )
		{
			// Don't do anything with directories
			if ( wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY )
			{
				// 
				char testname[ 256 ];
				strcpy( testname, wfd.cFileName );

				StripExtension( testname );
	
				if ( !FindBitmapForChecksum( testname ) )
				{
					// Delete it
					Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Removing unused bitmap file %s\n", 
						va( "%s/expressions/%s/%s/%s", GetGameDirectory(), models->GetActiveModelName(), GetName(), wfd.cFileName ) );

					_unlink( va( "%s/expressions/%s/%s/%s", GetGameDirectory(), models->GetActiveModelName(), GetName(), wfd.cFileName ) );
				}
			}

			morefiles = FindNextFile( hFindFile, &wfd ) ? true : false;
		}

		FindClose( hFindFile );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpClass::IsOverrideClass( void ) const
{
	return m_bIsOverride;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : override - 
//-----------------------------------------------------------------------------
void CExpClass::SetIsOverrideClass( bool override )
{
	m_bIsOverride = override;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpClass::RemoveOverrides( void )
{
	// Doesn't apply to us
	if ( IsOverrideClass() )
		return;

	for ( int i = 0; i < m_Expressions.Size(); i++ )
	{
		CExpression *exp = &m_Expressions[ i ];
		if ( !exp )
			continue;

		if ( !exp->HasOverride() )
			continue;

		exp->SetOverride( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpClass::ApplyOverrides( void )
{
	// Doesn't apply to us
	if ( IsOverrideClass() )
		return;

	if ( !HasOverrideClass() )
		return;

	Assert( m_pOverride );

	int i;
	for ( i = 0; i < m_Expressions.Size(); i++ )
	{
		CExpression *exp = &m_Expressions[ i ];
		if ( !exp )
			continue;

		// See if any override exists
		CExpression *override = m_pOverride->FindExpression( exp->name );
		if ( override )
		{
			exp->SetOverride( m_pOverride );
		}
		else
		{
			exp->SetOverride( NULL );
		}
	}

	// Now walk the override class and see if it has any elements not in the base
	//  class, if so, delete them
	for ( i = m_pOverride->GetNumExpressions() - 1; i >= 0 ; i-- )
	{
		CExpression *exp = m_pOverride->GetExpression( i );
		if ( !exp )
			continue;

		// It exists in the base class
		if ( FindExpression( exp->name ) )
			continue;

		Con_Printf( "Removing obsolete override '%s' from '%s'\n",
			exp->name, GetName() );

		// Remove the override since it doesn't exist in the base class
		m_pOverride->DeleteExpression( exp->name );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpClass::HasOverrideClass( void ) const
{
	return m_pOverride ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *overrideClass - 
//-----------------------------------------------------------------------------
void CExpClass::SetOverrideClass( CExpClass *overrideClass )
{
	m_pOverride = overrideClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CExpClass
//-----------------------------------------------------------------------------
CExpClass *CExpClass::GetOverrideClass( void )
{
	return m_pOverride;
}

//-----------------------------------------------------------------------------
// Purpose: Does this class have expression indices based on phoneme lookups
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpClass::IsPhonemeClass( void ) const
{
	return m_bIsPhonemeClass;
}