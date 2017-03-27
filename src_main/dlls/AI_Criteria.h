//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef AI_CRITERIA_H
#define AI_CRITERIA_H
#ifdef _WIN32
#pragma once
#endif

class CAI_ExpressiveNPC;

#include "utldict.h"
#include "interval.h"

class AI_CriteriaSet
{
public:
	AI_CriteriaSet();
	AI_CriteriaSet( const AI_CriteriaSet& src );
	~AI_CriteriaSet();

	void AppendCriteria( const char *criteria, const char *value = "", float weight = 1.0f );

	void Describe();

	int GetCount() const;
	int			FindCriterionIndex( const char *name ) const;

	const char *GetName( int index ) const;
	const char *GetValue( int index ) const;
	float		GetWeight( int index ) const;

private:

	KeyValues	*m_pCriteria;
	CUtlDict< KeyValues *, int > m_Lookup;
};

struct AI_ResponseParams
{
	DECLARE_SIMPLE_DATADESC();

	enum
	{
		RG_DELAYAFTERSPEAK = (1<<0),
		RG_SPEAKONCE = (1<<1),
		RG_ODDS = (1<<2),
		RG_RESPEAKDELAY = (1<<3),
		RG_SOUNDLEVEL = (1<<4),
	};

	AI_ResponseParams()
	{
		flags = 0;
		odds = 100;
		Q_memset( &delay, 0, sizeof( delay ) );
		Q_memset( &respeakdelay, 0, sizeof( respeakdelay ) );
		Q_memset( &soundlevel, 0, sizeof( soundlevel ) );
	}

	int						flags;
	int						odds;
	interval_t				delay;
	interval_t				respeakdelay;
	soundlevel_t			soundlevel;
};

//-----------------------------------------------------------------------------
// Purpose: Generic container for a response to a match to a criteria set
//  This is what searching for a response returns
//-----------------------------------------------------------------------------
class AI_Response
{
public:
	DECLARE_SIMPLE_DATADESC();

	typedef enum
	{
		RESPONSE_NONE = 0,
		RESPONSE_SPEAK,
		RESPONSE_SENTENCE,
		RESPONSE_SCENE,
		RESPONSE_WAV,
		RESPONSE_RESPONSE, // A reference to another response by name

		NUM_RESPONSES,
	} RESPONSETYPE;


	AI_Response();
	~AI_Response();

	void	Release();

	const char *GetName() const;
	const char *GetResponse() const;
	const AI_ResponseParams *GetParams() const { return &m_Params; }
	RESPONSETYPE GetType() const { return m_Type; }
	soundlevel_t	GetSoundLevel() const;
	float			GetRespeakDelay() const;
	bool			GetSpeakOnce() const;
	int				GetOdds() const;
	float			GetDelay() const;

	void Describe();

	const AI_CriteriaSet* GetCriteria();

	void	Init( RESPONSETYPE type, 
				const char *responseName, 
				const AI_CriteriaSet& criteria, 
				const AI_ResponseParams& responseparams,
				const char *matchingRule );

	static const char *DescribeResponse( RESPONSETYPE type );

private:
	enum
	{
		MAX_RESPONSE_NAME = 128,
		MAX_RULE_NAME = 128
	};

	RESPONSETYPE	m_Type;
	char			m_szResponseName[ MAX_RESPONSE_NAME ];

	// The initial criteria to which we are responsive
	AI_CriteriaSet	*m_pCriteria;

	AI_ResponseParams m_Params;

	char			m_szMatchingRule[ MAX_RULE_NAME ];
};

#endif // AI_CRITERIA_H
