//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEFLEX_H
#define BASEFLEX_H
#ifdef _WIN32
#pragma once
#endif


#include "BaseAnimatingOverlay.h"
#include "utlvector.h"

struct flexsettinghdr_t;
struct flexsetting_t;

//-----------------------------------------------------------------------------
// Purpose:  A .vfe referenced by a scene during .vcd playback
//-----------------------------------------------------------------------------
class CFacialExpressionFile
{
public:
	enum
	{
		MAX_FLEX_FILENAME = 128,
	};

	char			filename[ MAX_FLEX_FILENAME ];
	void			*buffer;
};

class CChoreoEvent;
class CChoreoScene;

//-----------------------------------------------------------------------------
// Purpose: One of a number of currently playing expressions for this actor
//-----------------------------------------------------------------------------
class CExpressionInfo
{
public:
	// The event handle of the current expression
	CChoreoEvent	*m_pEvent;

	// Current Scene
	CChoreoScene	*m_pScene;

	// Set after the first time the expression flex settings are configured ( allows
	//  bumpting markov index only at start of expression playback, not every frame )
	bool			m_bStarted;

public:
	//	EVENT local data...
	// FIXME: Evil, make accessors or figure out better place
	// FIXME: This won't work, scenes don't save and restore...
	int						m_iLayer;
	int						m_nSequence;
	float					m_flStartAnim;
	float					m_flPrevAnim;
};

//-----------------------------------------------------------------------------
// Purpose: Animated characters who have vertex flex capability (e.g., facial expressions)
//-----------------------------------------------------------------------------
class CBaseFlex : public CBaseAnimatingOverlay
{
	DECLARE_CLASS( CBaseFlex, CBaseAnimatingOverlay );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	// Construction
						CBaseFlex( void );
						~CBaseFlex( void );

	virtual void		SetModel( const char *szModelName );

	virtual void		Blink( );

	virtual	void		SetViewtarget( const Vector &viewtarget );
	virtual	const Vector &GetViewtarget( void );

	virtual void		SetFlexWeight( char *szName, float value );
	virtual void		SetFlexWeight( int index, float value );
	virtual float		GetFlexWeight( char *szName );
	virtual float		GetFlexWeight( int index );

	// Look up flex controller index by global name
	int					FindFlexController( const char *szName );

	// Manipulation of expressions for the object
	// Should be called by think function to process all expressions
	// The default implementation resets m_flexWeight array and calls
	//  AddSceneExpressions
	virtual void		ProcessExpressions( void );

	// Assumes m_flexWeight array has been set up, this adds the actual currently playing
	//  expressions to the flex weights
	virtual void		AddSceneExpressions( void );

	// Remove all playing expressions
	virtual void		ClearExpressions( CChoreoScene *scene );

	// Add the event/expression to the queue for this actor
	virtual void		AddSceneEvent( CChoreoScene *scene, CChoreoEvent *event );
	// Remvoe the event/expression from the queue for this actor
	virtual void		RemoveSceneEvent( CChoreoEvent *event );

	// Vector from actor to eye target
	CNetworkVector( m_viewtarget );
	// Blink state
	CNetworkVar( int, m_blinktoggle );

	// FIXME: ( MAXSTUDIOFLEXES == 64 ) seems high, this should be reset to what we're really going to use, but we can't use a per-model dynamic number
	//  since save/restore and data declaration can't take a variable number of fields.
	CNetworkArray( float, m_flexWeight, 64 );	

protected:
	// For handling .vfe files
	// Search list, or add if not in list
	void				*FindSceneFile( const char *filename );
	// Clear out the list
	void				DeleteSceneFiles( void );

	// Find setting by name
	flexsetting_t const *FindNamedSetting( flexsettinghdr_t const *pSettinghdr, const char *expr );

	// Called at the lowest level to actually apply an expression
	virtual void		AddExpression( const char *expr, float scale, flexsettinghdr_t *pSettinghdr, 
							flexsettinghdr_t *pOverrideHdr, bool newexpression );

	// Called at the lowest level to actually apply a flex animation
	virtual void		AddFlexAnimation( CExpressionInfo *info );


	virtual void		AddFlexSequence( CExpressionInfo *info );

	virtual void		AddFlexGesture( CExpressionInfo *info );

	// Array of active expressions
	CUtlVector < CExpressionInfo >		m_Expressions;

	// List of loaded .vfe files for this actor

	// FIXME:  Seems like this stuff be static and shared between all actors
	CUtlVector<CFacialExpressionFile *> m_FileList;
};

EXTERN_SEND_TABLE(DT_BaseFlex);


#endif // BASEFLEX_H
