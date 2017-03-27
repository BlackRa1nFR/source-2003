//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef EXPRESSION_H
#define EXPRESSION_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "mxBitmapTools.h"
#include "hlfaceposer.h"

#define GLOBAL_STUDIO_FLEX_CONTROL_COUNT ( MAXSTUDIOFLEXCTRL * 4 )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CExpUndoInfo
{
public:
	float				setting[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];
	float				weight[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];

	float				redosetting[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];
	float				redoweight[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];

	int					counter;
};

class CExpression;
class CExpClass;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMarkovGroup
{
public:
	int					expression;
	int					weight;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CExpression
{
public:
	typedef enum
	{
		TYPE_NORMAL = 0,
		TYPE_MARKOV
	} EXPRESSIONTYPE;

						CExpression	( void );
						~CExpression ( void );

						CExpression( const CExpression& from );

	void				SetModified( bool mod );
	bool				GetModified( void );
						
	void				ResetUndo( void );

	bool				CanUndo( void );
	bool				CanRedo( void );

	int					UndoLevels( void );
	int					UndoCurrent( void );

	const char 			*GetBitmapFilename( int modelindex );
	const char			*GetBitmapCheckSum( void );
	void				CreateNewBitmap( int modelindex );

	void				PushUndoInformation( void );
	void				PushRedoInformation( void );

	void				Undo( void );
	void				Redo( void );

	void				SetSelected( bool selected );
	bool				GetSelected( void );

	void				SetType( EXPRESSIONTYPE etype );
	EXPRESSIONTYPE		GetType( void );

	void				AddToGroup( CExpression *expression, int weight );
	void				RemoveFromGroup( CExpression *expression );
	void				SetWeight( CExpression *expression, int weight );
	void				ClearGroup( void );
	CMarkovGroup		*FindExpressionInGroup( CExpression *expression );

	CMarkovGroup		*GetCurrentGroup( void );
	void				NextGroup( void );
	void				PrevGroup( void );

	CMarkovGroup		*GetGroup( int num );
	int					GetNumGroups( void );
	int					GetCurrentGroupIndex( void );

	int					GetCurrentMarkovGroupIndex( void );
	void				NewMarkovIndex( void );

	float				*GetSettings( void );
	float				*GetWeights( void );

	// Walk markov group to get to underlying CExpression * 
	//  (if not markov, returns this pointer)
	CExpression			*GetExpression( void );

	bool				GetDirty( void );
	void				SetDirty( bool dirty );

	bool				HasOverride( void ) const;
	CExpression			*GetOverride( void );
	void				SetOverride( CExpClass *overrideClass );

	void				Revert( void );

	CExpClass			*GetExpressionClass( void );
	void				SetExpressionClass( char const *classname );

	EXPRESSIONTYPE		type;

	// name of expression
	char				name[32];			
	int					index;
	char				description[128];

	mxbitmapdata_t		m_Bitmap[ MAX_FP_MODELS ];

	bool				m_bModified;

	// Undo information
	CUtlVector< CExpUndoInfo * >		undo;
	int								m_nUndoCurrent;

	bool				m_bSelected;

	CUtlVector< CMarkovGroup >	m_Group;
	int					m_nCurrentGroup;

	int					m_nMarkovIndex;

	bool				m_bDirty;

private:
	// settings of fields
	float				setting[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];		
	float				weight[ GLOBAL_STUDIO_FLEX_CONTROL_COUNT ];		

	char				expressionclass[ 128 ];

	void WipeRedoInformation(  void );

	CExpClass			*m_pOverrideClass;

};

#endif // EXPRESSION_H
