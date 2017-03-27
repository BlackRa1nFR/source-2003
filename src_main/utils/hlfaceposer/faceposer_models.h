//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FACEPOSER_MODELS_H
#define FACEPOSER_MODELS_H
#ifdef _WIN32
#pragma once
#endif

class StudioModel;

class IFaceposerModels
{
public:
						IFaceposerModels();
	virtual				~IFaceposerModels();

	virtual int			Count( void ) const;
	virtual char const	*GetModelName( int index );
	virtual char const	*GetModelFileName( int index );
	virtual int			GetActiveModelIndex( void ) const;
	virtual char const	*GetActiveModelName( void );
	virtual StudioModel *GetActiveStudioModel( void );
	virtual int			FindModelByFilename( char const *filename );

	virtual int			LoadModel( char const *filename );
	virtual void		FreeModel( int index );

	virtual void		CloseAllModels( void );

	virtual StudioModel *GetStudioModel( int index );
	virtual studiohdr_t *GetStudioHeader( int index );
	virtual int			GetIndexForStudioModel( StudioModel *model );

	virtual int			GetModelIndexForActor( char const *actorname );
	virtual StudioModel *GetModelForActor( char const *actorname );

	virtual char const *GetActorNameForModel( int modelindex );
	virtual void		SetActorNameForModel( int modelindex, char const *actorname );

	virtual int			CountVisibleModels( void );
	virtual void		ShowModelIn3DView( int modelindex, bool show );
	virtual bool		IsModelShownIn3DView( int modelindex );

	virtual void		SaveModelList( void );
	virtual void		LoadModelList( void );

	virtual void		ReleaseModels( void );
	virtual void		RestoreModels( void );
	
	virtual void		RefreshModels( void );

	virtual void		CheckResetFlexes( void );
	virtual void		ClearOverlaysSequences( void );


private:
	class CFacePoserModel
	{
	public:
		CFacePoserModel( char const *modelfile, StudioModel *model );

		void SetActorName( char const *actorname )
		{
			strcpy( m_szActorName, actorname );
		}

		char const *GetActorName( void ) const
		{
			return m_szActorName;
		}

		StudioModel *GetModel( void ) const
		{
			return m_pModel;
		}

		char const *GetModelFileName( void ) const
		{
			return m_szModelFileName;
		}

		char const	*GetShortModelName( void ) const
		{
			return m_szShortName;
		}

		void SetVisibleIn3DView( bool visible )
		{
			m_bVisibileIn3DView = visible;
		}

		bool GetVisibleIn3DView( void ) const
		{
			return m_bVisibileIn3DView;
		}

		// For material system purposes
		void Release( void );
		void Restore( void );

		void Refresh( void )
		{
			// Forces a reload from disk
			Release();
			Restore();
		}

	private:
		enum
		{
			MAX_ACTOR_NAME = 64,
			MAX_MODEL_FILE = 128,
			MAX_SHORT_NAME = 32,
		};

		char			m_szActorName[ MAX_ACTOR_NAME ];
		char			m_szModelFileName[ MAX_MODEL_FILE ];
		char			m_szShortName[ MAX_SHORT_NAME ];
		StudioModel		*m_pModel;
		bool			m_bVisibileIn3DView;
	};

	CFacePoserModel *GetEntry( int index );

	CUtlVector< CFacePoserModel > m_Models;

	int					m_nLastRenderFrame;
};

extern IFaceposerModels *models;

#endif // FACEPOSER_MODELS_H
