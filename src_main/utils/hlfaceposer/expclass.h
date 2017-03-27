//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef EXPCLASS_H
#define EXPCLASS_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

class CExpression;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CExpClass
{
public:

						CExpClass( const char *classname );
	virtual 			~CExpClass( void );

	bool				IsOverrideClass( void ) const;
	void				SetIsOverrideClass( bool override );

	bool				HasOverrideClass( void ) const;
	void				SetOverrideClass( CExpClass *overrideClass );
	CExpClass			*GetOverrideClass( void );

	void				RemoveOverrides( void );
	void				ApplyOverrides( void );

	void				Save( void );
	void				Export( void );

	void				CheckBitmapConsistency( void );
	void				ReloadBitmaps( void );

	const char			*GetName( void );
	const char			*GetFileName( void );
	void				SetFileName( const char *filename );

	CExpression			*AddExpression( const char *name, const char *description, float *flexsettings, float *flexweights, bool selectnewitem );
	CExpression			*AddGroupExpression( const char *name, const char *description );
	CExpression			*FindExpression( const char *name );
	int					FindExpressionIndex( CExpression *exp );
	void				DeleteExpression( const char *name );
	
	int					GetNumExpressions( void );
	CExpression			*GetExpression( int num );

	bool				GetDirty( void );
	void				SetDirty( bool dirty );

	void				SelectExpression( int num, bool deselect = true );
	int					GetSelectedExpression( void );
	void				DeselectExpression( void );

	void				SwapExpressionOrder( int exp1, int exp2 );

	void				NewMarkovIndices( void );

	CExpression			*FindBitmapForChecksum( const char *checksum );

	// Get index of this class in the global class list
	int					GetIndex( void );

	bool				IsPhonemeClass( void ) const;

private:

	char				m_szClassName[ 128 ];
	char				m_szFileName[ 128 ];
	bool				m_bDirty;
	int					m_nSelectedExpression;
	CUtlVector < CExpression > m_Expressions;

	CExpClass			*m_pOverride;
	bool				m_bIsOverride;
	bool				m_bIsPhonemeClass;
};
#endif // EXPCLASS_H
