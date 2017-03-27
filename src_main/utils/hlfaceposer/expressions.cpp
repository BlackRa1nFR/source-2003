//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "expressions.h"
#include <mx/mx.h>
#include "ControlPanel.h"
#include "hlfaceposer.h"
#include "StudioModel.h"
#include "cmdlib.h"
#include "scriplib.h"
#include "expclass.h"
#include "mxExpressionTab.h"
#include "mxExpressionTray.h"
#include "FileSystem.h"
#include "faceposer_models.h"
#include "UtlDict.h"

bool Sys_Error(const char *pMsg, ...);
extern char g_appTitle[];

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct CFlexControllerName
{
	bool dummy;
};

static CUtlDict< CFlexControllerName, int > g_GlobalFlexControllers;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
char const *GetGlobalFlexControllerName( int index )
{
	return g_GlobalFlexControllers.GetElementName( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int GetGlobalFlexControllerCount( void )
{
	return g_GlobalFlexControllers.Count();
}
//-----------------------------------------------------------------------------
// Purpose: Accumulates throughout runtime session, oh well
// Input  : *szName - 
// Output : int
//-----------------------------------------------------------------------------
int AddGlobalFlexController( StudioModel *model, char *szName )
{
	int idx = g_GlobalFlexControllers.Find( szName );
	if ( idx != g_GlobalFlexControllers.InvalidIndex() )
	{
		return idx;
	}

	CFlexControllerName lookup;
	lookup.dummy = true;

	idx = g_GlobalFlexControllers.Insert( szName, lookup );

	//Con_Printf( "Added global flex controller %i %s from %s\n", idx, szName, model->getStudioHeader()->name );

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *model - 
//-----------------------------------------------------------------------------
void SetupModelFlexcontrollerLinks( StudioModel *model )
{
	if ( !model )
		return;

	studiohdr_t *hdr = model->getStudioHeader();
	if ( !hdr )
		return;

	// Already set up!!!
	if ( hdr->pFlexcontroller( 0 )->link != -1 )
		return;

	int i, j;

	for (i = 0; i < hdr->numflexcontrollers; i++)
	{
		j = AddGlobalFlexController( model, hdr->pFlexcontroller( i )->pszName() );
		hdr->pFlexcontroller( i )->link = j;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CExpressionManager : public IExpressionManager
{
public:
							CExpressionManager( void );
							~CExpressionManager( void );

	void					Reset( void );

	void					ActivateExpressionClass( CExpClass *cl );

	// File I/O
	void					LoadClass( const char *filename );
	void					LoadOverrides( CExpClass *baseClass, char const *inpath );
	void					CreateNewClass( const char *filename );
	bool					CloseClass( CExpClass *cl );

	CExpClass				*AddCExpClass( const char *classname, const char *filename );
	CExpClass				*AddOverrideClass( const char *classname );
	int						GetNumClasses( void );

	CExpression				*GetCopyBuffer( void );

	bool					CanClose( void );

	CExpClass				*GetActiveClass( void );
	CExpClass				*GetClass( int num );
	CExpClass				*FindClass( const char *classname );
	CExpClass				*FindOverrideClass( const char *classname );
	int						ConvertOverrideClassIndex( int index );

	void					NewMarkovIndices( void );
private:
	// Methods
	const char				*GetClassnameFromFilename( const char *filename );

// UI
	void					PopulateClassCB( CExpClass *cl );

	void					RemoveCExpClass( CExpClass *cl );

private:
	// Data
	CExpClass				*m_pActiveClass;
	CUtlVector < CExpClass * > m_Classes;

	CExpression				m_CopyBuffer;
};

// Expose interface
static CExpressionManager g_ExpressionManager;
IExpressionManager *expressions = &g_ExpressionManager;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExpressionManager::CExpressionManager( void )
{
	m_pActiveClass = NULL;
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExpressionManager::~CExpressionManager( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpressionManager::Reset( void )
{
	while ( m_Classes.Size() > 0 )
	{
		CExpClass *p = m_Classes[ 0 ];
		m_Classes.Remove( 0 );
		delete p;
	}

	m_pActiveClass = NULL;

	memset( &m_CopyBuffer, 0, sizeof( m_CopyBuffer ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CExpClass	*CExpressionManager::GetActiveClass( void )
{
	return m_pActiveClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : num - 
// Output : CExpClass
//-----------------------------------------------------------------------------
CExpClass *CExpressionManager::GetClass( int num )
{
	return m_Classes[ num ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classname - 
// Output : CExpClass
//-----------------------------------------------------------------------------
CExpClass *CExpressionManager::AddOverrideClass( const char *classname )
{
	CExpClass *pclass = new CExpClass( classname );
	if ( !pclass )
		return NULL;
	
	m_Classes.AddToTail( pclass );

	char filename[ 512 ];
	sprintf( filename, "%s/expressions/%s/%s.txt", GetGameDirectory(), models->GetActiveModelName(), classname );

	pclass->SetFileName( filename );

	return pclass;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classname - 
//			*filename - 
// Output : CExpClass *
//-----------------------------------------------------------------------------
CExpClass * CExpressionManager::AddCExpClass( const char *classname, const char *filename )
{
	Assert( !FindClass( classname ) );

	CExpClass *pclass = new CExpClass( classname );
	if ( !pclass )
		return NULL;
	
	m_Classes.AddToTail( pclass );

	pclass->SetFileName( filename );

	return pclass;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cl - 
//-----------------------------------------------------------------------------
void CExpressionManager::RemoveCExpClass( CExpClass *cl )
{
	for ( int i = 0; i < m_Classes.Size(); i++ )
	{
		CExpClass *p = m_Classes[ i ];
		if ( p == cl )
		{
			m_Classes.Remove( i );
			delete p;
			break;
		}
	}

	if ( m_Classes.Size() >= 1 )
	{
		ActivateExpressionClass( m_Classes[ 0 ] );
	}
	else
	{
		ActivateExpressionClass( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cl - 
//-----------------------------------------------------------------------------
void CExpressionManager::ActivateExpressionClass( CExpClass *cl )
{
	m_pActiveClass = cl;
	int select = 0;
	for ( int i = 0; i < GetNumClasses(); i++ )
	{
		CExpClass *c = GetClass( i );
		if ( cl == c )
		{
			select = i;
			break;
		}
	}
	
	g_pExpressionClass->select( select );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CExpressionManager::GetNumClasses( void )
{
	return m_Classes.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classname - 
// Output : CExpClass
//-----------------------------------------------------------------------------
CExpClass *CExpressionManager::FindClass( const char *classname )
{
	for ( int i = 0; i < m_Classes.Size(); i++ )
	{
		CExpClass *cl = m_Classes[ i ];

		if ( cl->IsOverrideClass() )
			continue;

		if ( !stricmp( classname, cl->GetName() ) )
		{
			return cl;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CExpressionManager::GetClassnameFromFilename( const char *filename )
{
	char cleanname[ 256 ];
	static char classname[ 256 ];
	classname[ 0 ] = 0;

	Assert( filename && filename[ 0 ] );

	strcpy( cleanname, filename );

	COM_FixSlashes( cleanname );
	ExtractFileBase( cleanname, classname, sizeof( classname ) );
	strlwr( classname );
	return classname;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CExpression
//-----------------------------------------------------------------------------
CExpression *CExpressionManager::GetCopyBuffer( void )
{
	return &m_CopyBuffer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpressionManager::CanClose( void )
{
	for ( int i = 0; i < m_Classes.Size(); i++ )
	{
		CExpClass *pclass = m_Classes[ i ];
		if ( pclass->GetDirty() )
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *inpath - 
//-----------------------------------------------------------------------------
void CExpressionManager::LoadOverrides( CExpClass *baseClass, char const *inpath )
{
	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	Assert( hdr );

	char filename[ 512 ];

	filesystem->FullPathToRelativePath( inpath, filename );

	const char *classname = GetClassnameFromFilename( filename );

	sprintf( filename, "expressions/%s/%s.txt", models->GetActiveModelName(), classname );

	Con_Printf( "Checking for overrides in %s\n", filename );

	// Already loaded, don't do anything
	if ( FindOverrideClass( classname ) )
		return;

	// No override file
	if ( !FPFullpathFileExists( va( "%s/%s", GetGameDirectory(), (char *)filename ) ) )
	{
		return;
	}

	// Import actual data
	LoadScriptFile( va( "%s/%s", GetGameDirectory(), (char *)filename ) );

	CExpClass *overrides = AddOverrideClass( classname );
	if ( !overrides )
		return;
	
	int numflexmaps = 0;
	int flexmap[128]; // maps file local controlls into global controls
	bool bHasWeighting = false;

	while (1)
	{
		GetToken (true);
		if (endofscript)
			break;
		if (stricmp( token, "$keys" ) == 0)
		{
			numflexmaps = 0;
			while (TokenAvailable())
			{
				GetToken( false );
				for (int i = 0; i < hdr->numflexcontrollers; i++)
				{
					if (stricmp( hdr->pFlexcontroller(i)->pszName(), token ) == 0)
					{
						flexmap[numflexmaps++] = i;
						break;
					}
				}
			}
		}
		else if ( !stricmp( token, "$group" ) )
		{
			char name[ 256 ];
			char member[ 256 ];
			int weight;

			// name
			GetToken( false );
			strcpy( name, token );

			CExpression *exp = overrides->FindExpression( name );

			while ( TokenAvailable() )
			{
				GetToken( false );
				strcpy( member, token );

				GetToken( false );
				weight = atoi( token );

				// Add to group
				if ( exp )
				{
					if ( exp->GetType() != CExpression::TYPE_MARKOV )
					{
						Con_Printf( "Can't add %s to %s, %s is not a markov group parent\n", member, name, name );
					}
					else
					{
						CExpression *exp2 = overrides->FindExpression( member );
						if ( !exp2 )
						{
							Con_Printf( "Can't add %s to %s, no such member %s defined\n", member, name, member );
						}
						else
						{
							exp->AddToGroup( exp2, weight );
						}
					}
				}
				else
				{
					Con_Printf( "Can't add %s to %s, no such $group defined\n", member, name );
				}
			}
		}
		else if ( !stricmp( token, "$groupname" ) )
		{
			char name[ 256 ];
			char desc[ 256 ];
			
			// name
			GetToken( false );
			strcpy( name, token );
			// description
			GetToken( false );
			strcpy( desc, token );

			overrides->AddGroupExpression( name, desc );
		}
		else if ( !stricmp( token, "$hasweighting" ) )
		{
			bHasWeighting = true;
		}
		else
		{
			float setting[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];
			float weight[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];
			char name[ 256 ];
			char desc[ 256 ];
			int index;

			memset( setting, 0, sizeof( setting ) );
			memset( weight, 0, sizeof( weight ) );

			strcpy( name, token );

			// phoneme index
			GetToken( false );
			if (token[1] == 'x')
			{
				sscanf( &token[2], "%x", &index );
			}
			else
			{
				index = (int)token[0];
			}

			// key values
			for (int i = 0; i < numflexmaps; i++)
			{
				GetToken( false );
				setting[flexmap[i]] = atof( token );
				if (bHasWeighting)
				{
					GetToken( false );
					weight[flexmap[i]] = atof( token );
				}
				else
				{
					weight[flexmap[i]] = 1.0;
				}
			}

			// description
			GetToken( false );
			strcpy( desc, token );

			CExpression *exp = overrides->AddExpression( name, desc, setting, weight, false );
			if ( overrides->IsPhonemeClass() && exp )
			{
				if ( exp->index != index )
				{
					Con_Printf( "CExpressionManager::LoadOverrides (%s):  phoneme index for %s in .txt file is wrong (expecting %i got %i), ignoring...\n",
						classname, name, exp->index, index );
				}
			}
		}
	}

	overrides->SetDirty( false );

	baseClass->SetOverrideClass( overrides );
	overrides->SetIsOverrideClass( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void CExpressionManager::LoadClass( const char *inpath )
{
	char filename[ 512 ];
	filesystem->FullPathToRelativePath( inpath, filename );

	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Can't load expressions from %s, must load a .mdl file first!\n",
			filename );
		return;
	}

	Con_Printf( "Loading expressions from %s\n", filename );

	const char *classname = GetClassnameFromFilename( filename );
	
	// Already loaded, don't do anything
	if ( FindClass( classname ) )
		return;

	if ( !FPFullpathFileExists( va( "%s/%s", GetGameDirectory(), (char *)filename ) ) )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Expression file %s doesn't exist, can't load\n", filename );
		return;
	}

	// Import actual data
	LoadScriptFile( va( "%s/%s", GetGameDirectory(), (char *)filename ) );

	CExpClass *active = AddCExpClass( classname, filename );
	if ( !active )
		return;

	ActivateExpressionClass( active );

	int numflexmaps = 0;
	int flexmap[128]; // maps file local controlls into global controls
	bool bHasWeighting = false;

	while (1)
	{
		GetToken (true);
		if (endofscript)
			break;
		if (stricmp( token, "$keys" ) == 0)
		{
			numflexmaps = 0;
			while (TokenAvailable())
			{
				flexmap[numflexmaps] = -1;
				GetToken( false );
				for (int i = 0; i < hdr->numflexcontrollers; i++)
				{
					if (stricmp( hdr->pFlexcontroller(i)->pszName(), token ) == 0)
					{
						flexmap[numflexmaps] = i;
						break;
					}
				}
				numflexmaps++;
			}
		}
		else if ( !stricmp( token, "$group" ) )
		{
			char name[ 256 ];
			char member[ 256 ];
			int weight;

			// name
			GetToken( false );
			strcpy( name, token );

			CExpression *exp = active->FindExpression( name );

			while ( TokenAvailable() )
			{
				GetToken( false );
				strcpy( member, token );

				GetToken( false );
				weight = atoi( token );

				// Add to group
				if ( exp )
				{
					if ( exp->GetType() != CExpression::TYPE_MARKOV )
					{
						Con_Printf( "Can't add %s to %s, %s is not a markov group parent\n", member, name, name );
					}
					else
					{
						CExpression *exp2 = active->FindExpression( member );
						if ( !exp2 )
						{
							Con_Printf( "Can't add %s to %s, no such member %s defined\n", member, name, member );
						}
						else
						{
							exp->AddToGroup( exp2, weight );
						}
					}
				}
				else
				{
					Con_Printf( "Can't add %s to %s, no such $group defined\n", member, name );
				}
			}
		}
		else if ( !stricmp( token, "$groupname" ) )
		{
			char name[ 256 ];
			char desc[ 256 ];
			
			// name
			GetToken( false );
			strcpy( name, token );
			// description
			GetToken( false );
			strcpy( desc, token );

			active->AddGroupExpression( name, desc );
		}
		else if ( !stricmp( token, "$hasweighting" ) )
		{
			bHasWeighting = true;
		}
		else
		{
			float setting[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];
			float weight[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];
			char name[ 256 ];
			char desc[ 256 ];
			int index;

			memset( setting, 0, sizeof( setting ) );
			memset( weight, 0, sizeof( weight ) );

			strcpy( name, token );

			// phoneme index
			GetToken( false );
			if (token[1] == 'x')
			{
				sscanf( &token[2], "%x", &index );
			}
			else
			{
				index = (int)token[0];
			}

			// key values
			for (int i = 0; i < numflexmaps; i++)
			{
				if (flexmap[i] > -1)
				{
					GetToken( false );
					setting[flexmap[i]] = atof( token );
					if (bHasWeighting)
					{
						GetToken( false );
						weight[flexmap[i]] = atof( token );
					}
					else
					{
						weight[flexmap[i]] = 1.0;
					}
				}
				else
				{
					GetToken( false );
					if (bHasWeighting)
					{
						GetToken( false );
					}
				}
			}

			// description
			GetToken( false );
			strcpy( desc, token );

			CExpression *exp = active->AddExpression( name, desc, setting, weight, false );
			if ( active->IsPhonemeClass() && exp )
			{
				if ( exp->index != index )
				{
					Con_Printf( "CExpressionManager::LoadClass (%s):  phoneme index for %s in .txt file is wrong (expecting %i got %i), ignoring...\n",
						classname, name, exp->index, index );
				}
			}
		}
	}

	LoadOverrides( active, inpath );

	active->CheckBitmapConsistency();

	active->ApplyOverrides();

	PopulateClassCB( active );

	active->SelectExpression( 0 );

	active->SetDirty( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void CExpressionManager::CreateNewClass( const char *filename )
{
	studiohdr_t *hdr = models->GetActiveStudioModel()->getStudioHeader ();
	if ( !hdr )
	{
		Con_ColorPrintf( ERROR_R, ERROR_G, ERROR_B, "Can't create new expression file %s, must load a .mdl file first!\n", filename );
		return;
	}

	// Tell the use that the filename was loaded, expressions are empty for now
	const char *classname = GetClassnameFromFilename( filename );
	
	// Already loaded, don't do anything
	if ( FindClass( classname ) )
		return;

	Con_Printf( "Creating %s\n", filename );

	CExpClass *active = AddCExpClass( classname, filename );
	if ( !active )
		return;

	ActivateExpressionClass( active );

	// Select the newly created class
	PopulateClassCB( active );

	// Select first expression
	active->SelectExpression( 0 );

	// Nothing has changed so far
	active->SetDirty( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cl - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CExpressionManager::CloseClass( CExpClass *cl )
{
	if ( !cl )
		return true;

	if ( cl->GetDirty() )
	{
		int retval = mxMessageBox( NULL, va( "Save changes to class '%s'?", cl->GetName() ), g_appTitle, MX_MB_YESNOCANCEL );
		if ( retval == 2 )
		{
			return false;
		}
		if ( retval == 0 )
		{
			Con_Printf( "Saving changes to %s : %s\n", cl->GetName(), cl->GetFileName() );
			cl->Save();
		}
	}

	cl->RemoveOverrides();

	// The memory can be freed here, so be more careful
	char temp[ 256 ];
	strcpy( temp, cl->GetName() );

	if ( cl->HasOverrideClass() )
	{
		RemoveCExpClass( cl->GetOverrideClass() );
	}
	RemoveCExpClass( cl );

	Con_Printf( "Closed expression class %s\n", temp );

	CExpClass *active = GetActiveClass();
	if ( !active )
	{
		PopulateClassCB( NULL );
		g_pExpressionTrayTool->redraw();
		return true;
	}

	// Select the first remaining class
	PopulateClassCB( active );

	// Select first expression
	active->SelectExpression( 0 );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : classnum - 
//-----------------------------------------------------------------------------
void CExpressionManager::PopulateClassCB( CExpClass *current )
{
	g_pExpressionClass->removeAll();
	int select = 0;
	for ( int i = 0; i < GetNumClasses(); i++ )
	{
		CExpClass *cl = GetClass( i );

		// Don't put overrides into the tab control though
		if ( !cl || cl->IsOverrideClass() )
			continue;

		g_pExpressionClass->add( cl->GetName() );
		
		if ( cl == current )
		{
			select = i;
		}
	}
	
	g_pExpressionClass->select( select );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExpressionManager::NewMarkovIndices( void )
{
	for ( int i = 0; i < m_Classes.Size(); i++ )
	{
		CExpClass *p = m_Classes[ i ];
		if ( p )
		{
			p->NewMarkovIndices();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classname - 
// Output : CExpClass
//-----------------------------------------------------------------------------
CExpClass *CExpressionManager::FindOverrideClass( char const *classname )
{
	for ( int i = 0; i < m_Classes.Size(); i++ )
	{
		CExpClass *cl = m_Classes[ i ];
		if ( !cl->IsOverrideClass() )
			continue;

		if ( !stricmp( classname, cl->GetName() ) )
		{
			return cl;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : int
//-----------------------------------------------------------------------------
int CExpressionManager::ConvertOverrideClassIndex( int index )
{
	int c = 0;
	for ( int i = 0; i <= m_Classes.Size(); i++ )
	{
		CExpClass *cl = m_Classes[ i ];
		if ( cl->IsOverrideClass() )
			continue;
		
		if ( c == index )
			return i;

		c++;
	}

	return index;
}
