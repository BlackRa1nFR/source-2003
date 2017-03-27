#include "hlfaceposer.h"
#include "StudioModel.h"
#include "faceposer_models.h"
#include "cmdlib.h"
#include "ifaceposerworkspace.h"
#include <mx/mx.h>
#include "mdlviewer.h"
#include "mxexpressiontray.h"
#include "ControlPanel.h"

void SetupModelFlexcontrollerLinks( StudioModel *model );

IFaceposerModels::CFacePoserModel::CFacePoserModel( char const *modelfile, StudioModel *model )
{
	m_pModel = model;
	m_szActorName[ 0 ] = 0;
	m_szShortName[ 0 ] = 0;
	strcpy( m_szModelFileName, modelfile );

	studiohdr_t *hdr = model->getStudioHeader();
	if ( hdr )
	{	
		strcpy( m_szShortName, hdr->name );
		StripExtension( m_szShortName );
	}

	m_bVisibileIn3DView = false;
}

void IFaceposerModels::CFacePoserModel::Release( void )
{
	m_pModel->FreeModel();
}

void IFaceposerModels::CFacePoserModel::Restore( void )
{
	if (m_pModel->LoadModel( m_pModel->GetFileName() ) )
	{
		m_pModel->PostLoadModel( m_pModel->GetFileName() );
	}
	SetupModelFlexcontrollerLinks( m_pModel );
}

IFaceposerModels::IFaceposerModels()
{
	m_nLastRenderFrame = -1;
}

IFaceposerModels::~IFaceposerModels()
{
}

IFaceposerModels::CFacePoserModel *IFaceposerModels::GetEntry( int index )
{
	if ( index < 0 || index >= Count() )
		return NULL;

	CFacePoserModel *m = &m_Models[ index ];
	if ( !m )
		return NULL;
	return m;
}

int IFaceposerModels::Count( void ) const
{
	return m_Models.Count();
}

char const *IFaceposerModels::GetModelName( int index )
{
	CFacePoserModel *entry = GetEntry( index );
	if ( !entry )
		return "";

	return entry->GetShortModelName();
}

char const *IFaceposerModels::GetModelFileName( int index )
{
	CFacePoserModel *entry = GetEntry( index );
	if ( !entry )
		return "";

	return entry->GetModelFileName();
}

int IFaceposerModels::GetActiveModelIndex( void ) const
{
	return g_MDLViewer->GetActiveModelTab();
}

char const *IFaceposerModels::GetActiveModelName( void )
{
	return GetModelName( GetActiveModelIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : StudioModel
//-----------------------------------------------------------------------------
StudioModel *IFaceposerModels::GetActiveStudioModel( void )
{
	StudioModel *mdl = GetStudioModel( GetActiveModelIndex() );
	if ( !mdl )
		return g_pStudioModel;
	return mdl;
}

int IFaceposerModels::FindModelByFilename( char const *filename )
{
	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		if ( !stricmp( m->GetModelFileName(), filename ) )
			return i;
	}

	return -1;
}

void SetupModelFlexcontrollerLinks( StudioModel *model );

int IFaceposerModels::LoadModel( char const *filename )
{
	int idx = FindModelByFilename( filename );
	if ( idx == -1 && Count() < MAX_FP_MODELS )
	{
		StudioModel *model = new StudioModel();

		StudioModel *save = g_pStudioModel;
		g_pStudioModel = model;
		if ( !model->LoadModel( filename ) )
		{
			delete model;
			g_pStudioModel = save;
			return 0; // ?? ERROR
		}
		g_pStudioModel = save;

		SetupModelFlexcontrollerLinks( model );

		CFacePoserModel newEntry( filename, model );
		
		idx = m_Models.AddToTail( newEntry );

		g_MDLViewer->InitModelTab();
		
		g_MDLViewer->SetActiveModelTab( idx );

		g_pControlPanel->CenterOnFace();
	}
	return idx;
}

void IFaceposerModels::FreeModel( int index  )
{
	CFacePoserModel *entry = GetEntry( index );
	if ( !entry )
		return;

	StudioModel *m = entry->GetModel();
	m->FreeModel();
	delete m;

	m_Models.Remove( index );

	g_MDLViewer->InitModelTab();
}

void IFaceposerModels::CloseAllModels( void )
{
	int c = Count();
	for ( int i = c - 1; i >= 0; i-- )
	{
		FreeModel( i );
	}
}

StudioModel *IFaceposerModels::GetStudioModel( int index )
{
	CFacePoserModel *m = GetEntry( index );
	if ( !m )
		return NULL;

	if ( !m->GetModel() )
		return NULL;

	return m->GetModel();
}

studiohdr_t *IFaceposerModels::GetStudioHeader( int index )
{
	StudioModel *m = GetStudioModel( index );
	if ( !m )
		return NULL;

	studiohdr_t *hdr = m->getStudioHeader();
	if ( !hdr )
		return NULL;
	return hdr;
}

int IFaceposerModels::GetModelIndexForActor( char const *actorname )
{
	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		if ( !stricmp( m->GetActorName(), actorname ) )
			return i;
	}

	return 0;
}

StudioModel *IFaceposerModels::GetModelForActor( char const *actorname )
{
	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		if ( !stricmp( m->GetActorName(), actorname ) )
			return m->GetModel();
	}

	return NULL;
}

char const *IFaceposerModels::GetActorNameForModel( int modelindex )
{
	CFacePoserModel *m = GetEntry( modelindex );
	if ( !m )
		return "";
	return m->GetActorName();
}

void IFaceposerModels::SetActorNameForModel( int modelindex, char const *actorname )
{
	CFacePoserModel *m = GetEntry( modelindex );
	if ( !m )
		return;

	m->SetActorName( actorname );
}

void IFaceposerModels::SaveModelList( void )
{
	workspacefiles->StartStoringFiles( IWorkspaceFiles::MODELDATA );
	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		workspacefiles->StoreFile( IWorkspaceFiles::MODELDATA, m->GetModelFileName() );
	}
	workspacefiles->FinishStoringFiles( IWorkspaceFiles::MODELDATA );
}

void IFaceposerModels::LoadModelList( void )
{
	int files = workspacefiles->GetNumStoredFiles( IWorkspaceFiles::MODELDATA );
	for ( int i = 0; i < files; i++ )
	{
		char const *filename = workspacefiles->GetStoredFile( IWorkspaceFiles::MODELDATA, i );
		LoadModel( filename );
	}
}

void IFaceposerModels::ReleaseModels( void )
{
	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		m->Release();
	}
}

void IFaceposerModels::RestoreModels( void )
{
	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		m->Restore();
	}
}


void IFaceposerModels::RefreshModels( void )
{
	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		m->Refresh();
	}
}

int IFaceposerModels::CountVisibleModels( void )
{
	int num = 0;
	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		if ( m->GetVisibleIn3DView() )
		{
			num++;
		}
	}

	return num;
}

void IFaceposerModels::ShowModelIn3DView( int modelindex, bool show )
{
	CFacePoserModel *m = GetEntry( modelindex );
	if ( !m )
		return;

	m->SetVisibleIn3DView( show );
}

bool IFaceposerModels::IsModelShownIn3DView( int modelindex )
{
	CFacePoserModel *m = GetEntry( modelindex );
	if ( !m )
		return false;

	return m->GetVisibleIn3DView();
}

int IFaceposerModels::GetIndexForStudioModel( StudioModel *model )
{
	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		if ( m->GetModel() == model )
			return i;
	}
	return -1;
}

void IFaceposerModels::CheckResetFlexes( void )
{
	int current_render_frame = g_MDLViewer->GetCurrentFrame();
	if ( current_render_frame == m_nLastRenderFrame )
		return;

	m_nLastRenderFrame = current_render_frame;

	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		StudioModel *model = m->GetModel();
		if ( !model )
			continue;

		studiohdr_t *hdr = model->getStudioHeader();
		if ( !hdr )
			continue;

		for ( int i = 0; i < hdr->numflexcontrollers; i++ )
		{
			model->SetFlexController( i, 0.0f );
		}
	}
}

void IFaceposerModels::ClearOverlaysSequences( void )
{
	int c = Count();
	for ( int i = 0; i < c; i++ )
	{
		CFacePoserModel *m = GetEntry( i );
		if ( !m )
			continue;

		StudioModel *model = m->GetModel();
		if ( !model )
			continue;

		model->ClearOverlaysSequences();
	}
}


static IFaceposerModels g_ModelManager;
IFaceposerModels *models = &g_ModelManager;