//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "SoundEmitterSystemBase.h"
#include "AI_ResponseSystem.h"
#include "igamesystem.h"
#include "AI_Criteria.h"
#include <KeyValues.h>
#include "filesystem.h"
#include "utldict.h"
#include "ai_speech.h"
#include <ctype.h>

static ConVar sv_debugresponses( "sv_debugresponses", "0", 0, "Show verbose matching output (1 for simple, 2 for rule scoring)" );

inline static char *CopyString( const char *in )
{
	if ( !in )
		return NULL;

	int len = Q_strlen( in );
	char *out = new char[ len + 1 ];
	Q_memcpy( out, in, len );
	out[ len ] = 0;
	return out;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CResponseSystem : public IResponseSystem 
{
public:
	CResponseSystem();
	~CResponseSystem();

	// IResponseSystem
	virtual bool FindBestResponse( const AI_CriteriaSet& set, AI_Response& response );

	virtual void Release() = 0;

protected:

	virtual const char *GetScriptFile( void ) = 0;
	void		LoadRuleSet( const char *setname );

	void		Clear();

private:


	struct Matcher
	{
		Matcher()
		{
			valid = false;

			token[0]=0;
			rawtoken[0] = 0;
			isnumeric = false;
			
			notequal = false;

			usemin = false;
			minequals = false;
			minval = false;
			usemax = false;
			maxequals = false;
			maxval = false;
		}

		void Describe( void )
		{
			if ( !valid )
			{
				Msg( "    invalid!\n" );
				return;
			}
			char sz[ 128 ];

			sz[ 0] = 0;
			int minmaxcount = 0;
			if ( usemin )
			{
				Q_snprintf( sz, sizeof( sz ), ">%s%.3f", minequals ? "=" : "", minval );
				minmaxcount++;
			}
			if ( usemax )
			{
				char sz2[ 128 ];
				Q_snprintf( sz2, sizeof( sz2 ), "<%s%.3f", maxequals ? "=" : "", maxval );

				if ( minmaxcount > 0 )
				{
					Q_strcat( sz, " and " );
				}
				Q_strcat( sz, sz2 );
				minmaxcount++;
			}

			if ( minmaxcount >= 1 )
			{
				Msg( "    matcher:  %s\n", sz );
				return;
			}

			if ( notequal )
			{
				Msg( "    matcher:  !=%s\n", token );
				return;
			}

			Msg( "    matcher:  ==%s\n", token );
		}

		bool	valid;

		bool	isnumeric;

		bool	notequal;

		bool	usemin;
		bool	minequals;
		float	minval;

		bool	usemax;
		bool	maxequals;
		float	maxval;

		char	token[ 128 ];
		char	rawtoken[ 128 ];
	};

	struct Response
	{
		Response()
		{
			type = AI_Response::RESPONSE_NONE;
			value = NULL;
			weight = 1.0f;
			depletioncount = 0;
		}

		Response( const Response& src )
		{
			weight = src.weight;
			type = src.type;
			value = CopyString( src.value );
			depletioncount = src.depletioncount;
		}

		Response& operator =( const Response& src )
		{
			if ( this == &src )
				return *this;
			weight = src.weight;
			type = src.type;
			value = CopyString( src.value );
			depletioncount = src.depletioncount;
			return *this;
		}

		~Response()
		{
			delete[] value;
		}

		AI_Response::RESPONSETYPE	type;

		char						*value;  // fixed up value spot
		float						weight;
		int							depletioncount;
	};

	struct ResponseGroup
	{
		ResponseGroup()
		{
			// By default visit all nodes before repeating
			m_bDepleteBeforeRepeat = true;
			m_nDepletionCount = 1;
		}

		ResponseGroup( const ResponseGroup& src )
		{
			int c = src.group.Count();
			for ( int i = 0; i < c; i++ )
			{
				group.AddToTail( src.group[ i ] );
			}

			rp = src.rp;
			m_bDepleteBeforeRepeat = src.m_bDepleteBeforeRepeat;
			m_nDepletionCount = src.m_nDepletionCount;
		}

		ResponseGroup& operator=( const ResponseGroup& src )
		{
			if ( this == &src )
				return *this;
			int c = src.group.Count();
			for ( int i = 0; i < c; i++ )
			{
				group.AddToTail( src.group[ i ] );
			}

			rp = src.rp;
			m_bDepleteBeforeRepeat = src.m_bDepleteBeforeRepeat;
			m_nDepletionCount = src.m_nDepletionCount;
			return *this;
		}

		bool	HasUndepletedChoices() const
		{
			if ( !m_bDepleteBeforeRepeat )
				return true;

			int c = group.Count();
			for ( int i = 0; i < c; i++ )
			{
				if ( group[ i ].depletioncount != m_nDepletionCount )
					return true;
			}

			return false;
		}

		void	MarkResponseUsed( int idx )
		{
			if ( !m_bDepleteBeforeRepeat )
				return;

			if ( idx < 0 || idx >= group.Count() )
			{
				Assert( 0 );
				return;
			}

			group[ idx ].depletioncount = m_nDepletionCount;
		}

		void	ResetDepletionCount()
		{
			if ( !m_bDepleteBeforeRepeat )
				return;
			++m_nDepletionCount;
		}

		bool	ShouldCheckRepeats() const { return m_bDepleteBeforeRepeat; }
		int		GetDepletionCount() const { return m_nDepletionCount; }

		CUtlVector< Response >	group;

		AI_ResponseParams		rp;

		// Use all slots before repeating any
		bool					m_bDepleteBeforeRepeat;
		// Invalidation counter
		int						m_nDepletionCount;
	};

	struct Criteria
	{
		Criteria()
		{
			name = NULL;
			value = NULL;
			weight = 1.0f;
			required = false;
		}
		Criteria& operator =(const Criteria& src )
		{
			if ( this == &src )
				return *this;

			name = CopyString( src.name );
			value = CopyString( src.value );
			weight = src.weight;
			required = src.required;

			matcher = src.matcher;

			int c = src.subcriteria.Count();
			for ( int i = 0; i < c; i++ )
			{
				subcriteria.AddToTail( src.subcriteria[ i ] );
			}

			return *this;
		}
		Criteria(const Criteria& src )
		{
			name = CopyString( src.name );
			value = CopyString( src.value );
			weight = src.weight;
			required = src.required;

			matcher = src.matcher;

			int c = src.subcriteria.Count();
			for ( int i = 0; i < c; i++ )
			{
				subcriteria.AddToTail( src.subcriteria[ i ] );
			}
		}
		~Criteria()
		{
			delete[] name;
			delete[] value;
		}

		bool IsSubCriteriaType() const
		{
			return ( subcriteria.Count() > 0 ) ? true : false;
		}

		char						*name;
		char						*value;
		float						weight;
		bool						required;

		Matcher						matcher;

		// Indices into sub criteria
		CUtlVector< int >			subcriteria;
	};

	struct Rule
	{
		Rule()
		{
			m_bMatchOnce = false;
			m_bEnabled = true;
		}

		Rule& operator =( const Rule& src )
		{
			if ( this == &src )
				return *this;

			int i;
			int c;
			
			c = src.m_Criteria.Count(); 
			for ( i = 0; i < c; i++ )
			{
				m_Criteria.AddToTail( src.m_Criteria[ i ] );
			}

			c = src.m_Responses.Count(); 
			for ( i = 0; i < c; i++ )
			{
				m_Responses.AddToTail( src.m_Responses[ i ] );
			}

			m_bMatchOnce = src.m_bMatchOnce;
			m_bEnabled = src.m_bEnabled;
			return *this;
		}

		Rule( const Rule& src )
		{
			int i;
			int c;
			
			c = src.m_Criteria.Count(); 
			for ( i = 0; i < c; i++ )
			{
				m_Criteria.AddToTail( src.m_Criteria[ i ] );
			}

			c = src.m_Responses.Count(); 
			for ( i = 0; i < c; i++ )
			{
				m_Responses.AddToTail( src.m_Responses[ i ] );
			}

			m_bMatchOnce = src.m_bMatchOnce;
			m_bEnabled = src.m_bEnabled;
		}

		bool	IsEnabled() const { return m_bEnabled; }
		void	Disable() { m_bEnabled = false; }
		bool	IsMatchOnce() const { return m_bMatchOnce; }

		// Indices into underlying criteria and response dictionaries
		CUtlVector< int >	m_Criteria;
		CUtlVector< int	>	m_Responses;

		bool				m_bMatchOnce;
		bool				m_bEnabled;
	};

	struct Enumeration
	{
		float		value;
	};

	struct ResponseSearchResult
	{
		ResponseSearchResult()
		{
			group = NULL;
			action = NULL;
		}

		ResponseGroup	*group;
		Response		*action;
	};

	inline bool ParseToken( void )
	{
		if ( m_bUnget )
		{
			m_bUnget = false;
			return true;
		}
		if ( m_ScriptStack.Count() <= 0 )
		{
			Assert( 0 );
			return false;
		}

		m_ScriptStack[ 0 ].currenttoken = engine->COM_ParseFile( m_ScriptStack[ 0 ].currenttoken, token );
		m_ScriptStack[ 0 ].tokencount++;
		return m_ScriptStack[ 0 ].currenttoken != NULL ? true : false;
	}

	inline void Unget()
	{
		m_bUnget = true;
	}

	inline bool TokenWaiting( void )
	{
		char *p;
	
		if ( m_ScriptStack.Count() <= 0 )
		{
			Assert( 0 );
			return false;
		}

		p = m_ScriptStack[ 0 ].currenttoken;
		while ( *p && *p!='\n')
		{
			// Special handler for // comment blocks
			if ( *p == '/' && *(p+1) == '/' )
				return false;

			if ( !isspace( *p ) || isalnum( *p ) )
				return true;

			p++;
		}

		return false;
	}
	
	void		ParseOneResponse( const char *responseGroupName, ResponseGroup& group );

	void		ParseInclude( void );
	void		ParseResponse( void );
	void		ParseCriterion( void );
	void		ParseRule( void );
	void		ParseEnumeration( void );

	int			ParseOneCriterion( const char *criterionName );
	
	bool		Compare( const char *setValue, Criteria *c, bool verbose = false );
	bool		CompareUsingMatcher( const char *setValue, Matcher& m, bool verbose = false );
	void		ComputeMatcher( Criteria *c, Matcher& matcher );
	void		ResolveToken( Matcher& matcher );
	float		LookupEnumeration( const char *name, bool& found );

	int			FindBestMatchingRule( const AI_CriteriaSet& set, bool verbose );

	float		ScoreCriteriaAgainstRule( const AI_CriteriaSet& set, int irule, bool verbose = false );
	float		RecursiveScoreSubcriteriaAgainstRule( const AI_CriteriaSet& set, Criteria *parent, bool& exclude, bool verbose /*=false*/ );
	float		ScoreCriteriaAgainstRuleCriteria( const AI_CriteriaSet& set, int icriterion, bool& exclude, bool verbose = false );
	bool		GetBestResponse( ResponseSearchResult& result, Rule *rule, bool verbose = false );
	bool		ResolveResponse( ResponseSearchResult& result, int depth, const char *name, bool verbose = false );
	int			SelectWeightedResponseFromResponseGroup( ResponseGroup *g );
	void		DescribeResponseGroup( ResponseGroup *group, int selected, int depth );
	void		DebugPrint( int depth, const char *fmt, ... );

	void		LoadFromBuffer( const char *scriptfile, const char *buffer );

	char const	*GetCurrentScript();
	int			GetCurrentToken() const;
	void		SetCurrentScript( const char *script );
	bool		IsRootCommand();

	void		PushScript( const char *scriptfile, unsigned char *buffer );
	void		PopScript(void);

	void		ResponseWarning( const char *fmt, ... );

	CUtlDict< ResponseGroup, int >	m_Responses;
	CUtlDict< Criteria, int >	m_Criteria;
	CUtlDict< Rule, int >	m_Rules;
	CUtlDict< Enumeration, int > m_Enumerations;

	char		token[ 1204 ];

	bool		m_bUnget;

	struct ScriptEntry
	{
		unsigned char *buffer;
		char			name[ 256 ];
		char			*currenttoken;
		int				tokencount;
	};

	CUtlVector< ScriptEntry >		m_ScriptStack;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CResponseSystem::CResponseSystem()
{
	token[0] = 0;
	m_bUnget = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CResponseSystem::~CResponseSystem()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CResponseSystem::GetCurrentScript()
{
	if ( m_ScriptStack.Count() <= 0 )
		return "";
	
	return m_ScriptStack[ 0 ].name;
}

void CResponseSystem::PushScript( const char *scriptfile, unsigned char *buffer )
{
	ScriptEntry e;
	Q_snprintf( e.name, sizeof( e.name ), "%s", scriptfile );
	e.buffer = buffer;
	e.currenttoken = (char *)e.buffer;
	e.tokencount = 0;
	m_ScriptStack.AddToHead( e );
}

void CResponseSystem::PopScript(void)
{
	Assert( m_ScriptStack.Count() >= 1 );
	if ( m_ScriptStack.Count() <= 0 )
		return;

	m_ScriptStack.Remove( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::Clear()
{
	m_Responses.RemoveAll();
	m_Criteria.RemoveAll();
	m_Rules.RemoveAll();
	m_Enumerations.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			found - 
// Output : float
//-----------------------------------------------------------------------------
float CResponseSystem::LookupEnumeration( const char *name, bool& found )
{
	int idx = m_Enumerations.Find( name );
	if ( idx == m_Enumerations.InvalidIndex() )
	{
		found = false;
		return 0.0f;
	}


	found = true;
	return m_Enumerations[ idx ].value;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : matcher - 
//-----------------------------------------------------------------------------
void CResponseSystem::ResolveToken( Matcher& matcher )
{
	Q_strcpy( matcher.token, matcher.rawtoken );

	if ( matcher.rawtoken[0] != '[' )
	{
		return;
	}

	Q_strlower( matcher.token );

	// Now lookup enumeration
	bool found = false;
	float f = LookupEnumeration( matcher.token, found );
	if ( !found )
	{
		ResponseWarning( "No such enumeration '%s'\n", matcher.token );
		return;
	}

	Q_snprintf( matcher.token, sizeof( matcher.token ), "%f", f );
}


static bool AppearsToBeANumber( char const *token )
{
	if ( atof( token ) != 0.0f )
		return true;

	char const *p = token;
	while ( *p )
	{
		if ( *p != '0' )
			return false;

		p++;
	}

	return true;
}

void CResponseSystem::ComputeMatcher( Criteria *c, Matcher& matcher )
{
	const char *s = c->value;
	if ( !s )
	{
		matcher.valid = false;
		return;
	}

	const char *in = s;

	matcher.token[ 0 ] = 0;
	matcher.rawtoken[ 0 ] = 0;
	int n = 0;

	bool gt = false;
	bool lt = false;
	bool eq = false;
	bool nt = false;

	bool done = false;
	while ( !done )
	{
		switch( *in )
		{
		case '>':
			{
				gt = true;
				Assert( !lt ); // Can't be both
			}
			break;
		case '<':
			{
				lt = true;
				Assert( !gt );  // Can't be both
			}
			break;
		case '=':
			{
				eq = true;
			}
			break;
		case ',':
		case '\0':
			{
				matcher.rawtoken[ n ] = 0;
				n = 0;

				// Convert raw token to real token in case token is an enumerated type specifier
				ResolveToken( matcher );

				// Fill in first data set
				if ( gt )
				{
					matcher.usemin = true;
					matcher.minequals = eq;
					matcher.minval = (float)atof( matcher.token );

					matcher.isnumeric = true;
				}
				else if ( lt )
				{
					matcher.usemax = true;
					matcher.maxequals = eq;
					matcher.maxval = (float)atof( matcher.token );

					matcher.isnumeric = true;
				}
				else
				{
					if ( *in == ',' )
					{
						// If there's a comma, this better have been a less than or a gt key
						Assert( 0 );
					}

					matcher.notequal = nt;

					matcher.isnumeric = AppearsToBeANumber( matcher.token );
				}

				gt = lt = eq = nt = false;

				if ( !(*in) )
				{
					done = true;
				}
			}
			break;
		case '!':
			nt = true;
			break;
		default:
			matcher.rawtoken[ n++ ] = *in;
			break;
		}

		in++;
	}

	matcher.valid = true;
}

bool CResponseSystem::CompareUsingMatcher( const char *setValue, Matcher& m, bool verbose /*=false*/ )
{
	if ( !m.valid )
		return false;

	float v = (float)atof( setValue );
	if ( setValue[0] == '[' )
	{
		bool found = false;
		v = LookupEnumeration( setValue, found );
	}
	
	int minmaxcount = 0;

	if ( m.usemin )
	{
		if ( m.minequals )
		{
			if ( v < m.minval )
				return false;
		}
		else
		{
			if ( v <= m.minval )
				return false;
		}

		++minmaxcount;
	}

	if ( m.usemax )
	{
		if ( m.maxequals )
		{
			if ( v > m.maxval )
				return false;
		}
		else
		{
			if ( v >= m.maxval )
				return false;
		}

		++minmaxcount;
	}

	// Had one or both criteria and met them
	if ( minmaxcount >= 1 )
	{
		return true;
	}

	if ( m.notequal )
	{
		if ( m.isnumeric )
		{
			if ( v == (float)atof( m.token ) )
				return false;
		}
		else
		{
			if ( !Q_stricmp( setValue, m.token ) )
				return false;
		}

		return true;
	}

	if ( m.isnumeric )
	{
		return v == (float)atof( m.token );
	}

	return !Q_stricmp( setValue, m.token ) ? true : false;
}

bool CResponseSystem::Compare( const char *setValue, Criteria *c, bool verbose /*= false*/ )
{
	Assert( c );
	Assert( setValue );

	bool bret = CompareUsingMatcher( setValue, c->matcher, verbose );

	if ( verbose )
	{
		Msg( "'%20s' vs. '%20s' = ", setValue, c->value );

		{
			//Msg( "\n" );
			//m.Describe();
		}
	}
	return bret;
}

float CResponseSystem::RecursiveScoreSubcriteriaAgainstRule( const AI_CriteriaSet& set, Criteria *parent, bool& exclude, bool verbose /*=false*/ )
{
	float score = 0.0f;
	int subcount = parent->subcriteria.Count();
	for ( int i = 0; i < subcount; i++ )
	{
		int icriterion = parent->subcriteria[ i ];

		bool excludesubrule = false;
		if (verbose)
		{
			Msg( "\n" );
		}
		score += ScoreCriteriaAgainstRuleCriteria( set, icriterion, excludesubrule, verbose );
	}

	exclude = ( parent->required && score == 0.0f ) ? true : false;

	return score * parent->weight;
}

float CResponseSystem::ScoreCriteriaAgainstRuleCriteria( const AI_CriteriaSet& set, int icriterion, bool& exclude, bool verbose /*=false*/ )
{
	Criteria *c = &m_Criteria[ icriterion ];

	if ( c->IsSubCriteriaType() )
	{
		return RecursiveScoreSubcriteriaAgainstRule( set, c, exclude, verbose );
	}

	if ( verbose )
	{
		Msg( "  criterion '%25s':'%15s' ", m_Criteria.GetElementName( icriterion ), c->name );
	}

	exclude = false;

	float score = 0.0f;

	const char *actualValue = "";

	int found = set.FindCriterionIndex( c->name );
	if ( found != -1 )
	{
		actualValue = set.GetValue( found );
		if ( !actualValue )
		{
			Assert( 0 );
			return score;
		}
	}

	Assert( actualValue );

	if ( Compare( actualValue, c, verbose ) )
	{
		float w = set.GetWeight( found );
		score = w * c->weight;

		if ( verbose )
		{
			Msg( "matched, weight %4.2f (s %4.2f x c %4.2f)",
				score, w, c->weight );
		}
	}
	else
	{
		if ( c->required )
		{
			exclude = true;
			if ( verbose )
			{
				Msg( "failed (+exclude rule)" );
			}
		}
		else
		{
			if ( verbose )
			{
				Msg( "failed" );
			}
		}
	}

	return score;
}

float CResponseSystem::ScoreCriteriaAgainstRule( const AI_CriteriaSet& set, int irule, bool verbose /*=false*/ )
{
	Rule *rule = &m_Rules[ irule ];
	float score = 0.0f;

	if ( !rule->IsEnabled() )
		return 0.0f;

	if ( verbose )
	{
		Msg( "Scoring rule '%s' (%i)\n{\n", m_Rules.GetElementName( irule ), irule+1 );
	}

	// Iterate set criteria
	int count = rule->m_Criteria.Count();
	int i;
	for ( i = 0; i < count; i++ )
	{
		int icriterion = rule->m_Criteria[ i ];

		bool exclude = false;
		score += ScoreCriteriaAgainstRuleCriteria( set, icriterion, exclude, verbose );

		if ( verbose )
		{
			Msg( ", score %4.2f\n", score );
		}

		if ( exclude ) 
		{
			score = 0.0f;
			break;
		}
	}

	if ( verbose )
	{
		Msg( "}\n" );
	}
	
	return score;
}

void CResponseSystem::DebugPrint( int depth, const char *fmt, ... )
{
	int indentchars = 3 * depth;
	char *indent = (char *)_alloca( indentchars + 1);
	indent[ indentchars ] = 0;
	while ( --indentchars >= 0 )
	{
		indent[ indentchars ] = ' ';
	}

	// Dump text to debugging console.
	va_list argptr;
	char szText[1024];

	va_start (argptr, fmt);
	vsprintf (szText, fmt, argptr);
	va_end (argptr);

	Msg( "%s%s", indent, szText );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *g - 
// Output : int
//-----------------------------------------------------------------------------
int CResponseSystem::SelectWeightedResponseFromResponseGroup( ResponseGroup *g )
{
	int c = g->group.Count();
	if ( !c )
	{
		Assert( !"Expecting response group with >= 1 elements" );
		return -1;
	}

	if ( !g->HasUndepletedChoices() )
	{
		g->ResetDepletionCount();
	}

	bool checkrepeats = g->ShouldCheckRepeats();
	int	depletioncount = g->GetDepletionCount();

	float totalweight = 0.0f;
	int slot = 0;

	for ( int i = 0; i < c; i++ )
	{
		Response *r = &g->group[ i ];
		if ( checkrepeats && 
			( r->depletioncount == depletioncount ) )
		{
			continue;
		}

		if ( !totalweight )
		{
			slot = i;
		}

		// Always assume very first slot will match
		totalweight += r->weight;
		if ( !totalweight || random->RandomFloat(0,totalweight) < r->weight )
		{
			slot = i;
		}
	}

	g->MarkResponseUsed( slot );

	return slot;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : searchResult - 
//			depth - 
//			*name - 
//			verbose - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CResponseSystem::ResolveResponse( ResponseSearchResult& searchResult, int depth, const char *name, bool verbose /*= false*/ )
{
	int responseIndex = m_Responses.Find( name );
	if ( responseIndex == m_Responses.InvalidIndex() )
		return false;

	ResponseGroup *g = &m_Responses[ responseIndex ];

	int c = g->group.Count();
	if ( !c )
		return false;

	int idx = SelectWeightedResponseFromResponseGroup( g );

	if ( verbose )
	{
		DebugPrint( depth, "%s\n", m_Responses.GetElementName( responseIndex ) );
		DebugPrint( depth, "{\n" );
		DescribeResponseGroup( g, idx, depth );
	}

	bool bret = true;

	Response *result = &g->group[ idx ];
	if ( result->type == AI_Response::RESPONSE_RESPONSE )
	{
		// Recurse
		bret = ResolveResponse( searchResult, depth + 1, result->value );
	}
	else
	{
		searchResult.action = result;
		searchResult.group	= g;
	}

	if( verbose )
	{
		DebugPrint( depth, "}\n" );
	}

	return bret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *group - 
//			selected - 
//			depth - 
//-----------------------------------------------------------------------------
void CResponseSystem::DescribeResponseGroup( ResponseGroup *group, int selected, int depth )
{
	int c = group->group.Count();

	for ( int i = 0; i < c ; i++ )
	{
		Response *r = &group->group[ i ];
		DebugPrint( depth + 1, "%s%20s : %40s %5.3f\n",
			i == selected ? "-> " : "   ",
			AI_Response::DescribeResponse( r->type ),
			r->value,
			r->weight );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *rule - 
// Output : CResponseSystem::Response
//-----------------------------------------------------------------------------
bool CResponseSystem::GetBestResponse( ResponseSearchResult& searchResult, Rule *rule, bool verbose /*=false*/ )
{
	int c = rule->m_Responses.Count();
	if ( !c )
		return false;

	int index = random->RandomInt( 0, c - 1 );
	int groupIndex = rule->m_Responses[ index ];

	ResponseGroup *g = &m_Responses[ groupIndex ];
	int count = g->group.Count();
	if ( !count )
		return false;

	int responseIndex = SelectWeightedResponseFromResponseGroup( g );

	Response *r = &g->group[ responseIndex ];

	int depth = 0;

	if ( verbose )
	{
		DebugPrint( depth, "%s\n", m_Responses.GetElementName( groupIndex ) );
		DebugPrint( depth, "{\n" );

		DescribeResponseGroup( g, responseIndex, depth );
	}

	bool bret = true;

	if ( r->type == AI_Response::RESPONSE_RESPONSE )
	{
		bret = ResolveResponse( searchResult, depth + 1, r->value, verbose );
	}
	else
	{
		searchResult.action = r;
		searchResult.group	= g;
	}

	if ( verbose )
	{
		DebugPrint( depth, "}\n" );
	}

	return bret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
//			verbose - 
// Output : int
//-----------------------------------------------------------------------------
int CResponseSystem::FindBestMatchingRule( const AI_CriteriaSet& set, bool verbose )
{
	CUtlVector< int >	bestrules;
	float bestscore = 0.001f;

	int c = m_Rules.Count();
	int i;
	for ( i = 0; i < c; i++ )
	{
		float score = ScoreCriteriaAgainstRule( set, i, verbose );
		// Check equals so that we keep track of all matching rules
		if ( score >= bestscore )
		{
			// Reset bucket
			if( score != bestscore )
			{
				bestscore = score;
				bestrules.RemoveAll();
			}

			// Add to bucket
			bestrules.AddToTail( i );
		}
	}

	int bestCount = bestrules.Count();
	if ( bestCount <= 0 )
		return -1;

	if ( bestCount == 1 )
		return bestrules[ 0 ];

	// Randomly pick one of the tied matching rules
	int idx = random->RandomInt( 0, bestCount - 1 );
	if ( verbose )
	{
		Msg( "Found %i matching rules, selecting slot %i\n", bestCount, idx );
	}
	return bestrules[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
// Output : AI_Response
//-----------------------------------------------------------------------------
bool CResponseSystem::FindBestResponse( const AI_CriteriaSet& set, AI_Response& response )
{
	bool valid = false;

	bool showRules = ( sv_debugresponses.GetInt() >= 2 ) ? true : false;
	bool showResult = sv_debugresponses.GetBool();

	// Look for match
	int bestRule = FindBestMatchingRule( set, showRules );

	AI_Response::RESPONSETYPE responseType = AI_Response::RESPONSE_NONE;
	AI_ResponseParams rp;

	char ruleName[ 128 ];
	char responseName[ 128 ];
	ruleName[ 0 ] = 0;
	responseName[ 0 ] = 0;
	if ( bestRule != -1 )
	{
		Rule *r = &m_Rules[ bestRule ];

		ResponseSearchResult result;
		if ( GetBestResponse( result, r, showResult ) )
		{
			Q_strncpy( responseName, result.action->value, sizeof( responseName ) );
			responseType = result.action->type;
			rp = result.group->rp;
		}

		Q_strncpy( ruleName, m_Rules.GetElementName( bestRule ), sizeof( ruleName ) );

		// Disable the rule if it only allows for matching one time
		if ( r->IsMatchOnce() )
		{
			r->Disable();
		}

		valid = true;
	}

	response.Init( responseType, responseName, set, rp, ruleName );

	if ( valid && showResult )
	{
		// Rescore the winner and dump to console
		ScoreCriteriaAgainstRule( set, bestRule, true );

		// Describe the response, too
		response.Describe();
	}

	return valid;
}

void CResponseSystem::ParseInclude( void )
{
	char includefile[ 256 ];
	ParseToken();
	Q_snprintf( includefile, sizeof( includefile ), "scripts/%s", token );

	// Try and load it
	int length;
	unsigned char *buffer = engine->LoadFileForMe( includefile, &length );
	if ( !buffer )
	{
		Msg( "Unable to load #included script %s\n", includefile );
		return;
	}

	LoadFromBuffer( includefile, (const char *)buffer );

	engine->FreeFile( buffer );
}

void CResponseSystem::LoadFromBuffer( const char *scriptfile, const char *buffer )
{
	PushScript( scriptfile, (unsigned char * )buffer );

	while ( 1 )
	{
		ParseToken();
		if ( !token[0] )
		{
			break;
		}

		if ( !Q_stricmp( token, "#include" ) )
		{
			ParseInclude();
		}
		else if ( !Q_stricmp( token, "response" ) )
		{
			ParseResponse();
		}
		else if ( !Q_stricmp( token, "criterion" ) || 
			!Q_stricmp( token, "criteria" ) )
		{
			ParseCriterion();
		}
		else if ( !Q_stricmp( token, "rule" ) )
		{
			ParseRule();
		}
		else if ( !Q_stricmp( token, "enumeration" ) )
		{
			ParseEnumeration();
		}
		else
		{
			ResponseWarning( "Unknown entry type '%s', expecting 'response', 'criterion', 'enumeration' or 'rules'\n", 
				token );
			break;
		}
	}

	if ( m_ScriptStack.Count() == 1 )
	{
		DevMsg( 1, "CResponseSystem:  %s (%i rules, %i criteria %i responses)\n",
			GetCurrentScript(), m_Rules.Count(), m_Criteria.Count(), m_Responses.Count() );
	}

	PopScript();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CResponseSystem::LoadRuleSet( const char *basescript )
{
	int length = 0;
	unsigned char *buffer = (unsigned char *)engine->LoadFileForMe( basescript, &length );
	if ( length <= 0 || !buffer )
	{
		DevMsg( 1, "CResponseSystem:  failed to load %s\n", basescript );
		return;
	}

	LoadFromBuffer( basescript, (const char *)buffer );

	Assert( m_ScriptStack.Count() == 0 );
}

static AI_Response::RESPONSETYPE ComputeResponseType( const char *s )
{
	if ( !Q_stricmp( s, "scene" ) )
	{
		return AI_Response::RESPONSE_SCENE;
	}
	else if ( !Q_stricmp( s, "sentence" ) )
	{
		return AI_Response::RESPONSE_SENTENCE;
	}
	else if ( !Q_stricmp( s, "wav" ) )
	{
		return AI_Response::RESPONSE_WAV;
	}
	else if ( !Q_stricmp( s, "speak" ) )
	{
		return AI_Response::RESPONSE_SPEAK;
	}
	else if ( !Q_stricmp( s, "response" ) )
	{
		return AI_Response::RESPONSE_RESPONSE;
	}

	return AI_Response::RESPONSE_NONE;
}

void CResponseSystem::ParseOneResponse( const char *responseGroupName, ResponseGroup& group )
{
	Response newResponse;
	newResponse.weight = 1.0f;
	AI_ResponseParams *rp = &group.rp;

	newResponse.type = ComputeResponseType( token );
	if ( AI_Response::RESPONSE_NONE == newResponse.type )
	{
		ResponseWarning( "response entry '%s' with unknown response type '%s'\n", responseGroupName, token );
		return;
	}

	ParseToken();
	newResponse.value = CopyString( token );

	while ( TokenWaiting() )
	{
		ParseToken();
		if ( !Q_stricmp( token, "weight" ) )
		{
			ParseToken();
			newResponse.weight = (float)atof( token );
			continue;
		}

		if ( !Q_stricmp( token, "nodelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay.start = 0;
			rp->delay.range = 0;
			continue;
		}

		if ( !Q_stricmp( token, "defaultdelay" ) )
		{
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay.start = AIS_DEF_MIN_DELAY;
			rp->delay.range = ( AIS_DEF_MAX_DELAY - AIS_DEF_MIN_DELAY );
			continue;
		}
	
		if ( !Q_stricmp( token, "delay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay = ReadInterval( token );
			continue;
		}
		
		if ( !Q_stricmp( token, "speakonce" ) )
		{
			rp->flags |= AI_ResponseParams::RG_SPEAKONCE;
			continue;
		}
		
		if ( !Q_stricmp( token, "odds" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_ODDS;
			rp->odds = clamp( atoi( token ), 0, 100 );
			continue;
		}
		
		if ( !Q_stricmp( token, "respeakdelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_RESPEAKDELAY;
			rp->respeakdelay = ReadInterval( token );
			continue;
		}
		
		if ( !Q_stricmp( token, "soundlevel" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_SOUNDLEVEL;
			rp->soundlevel = (soundlevel_t)TextToSoundLevel( token );
			continue;
		}
	}

	group.group.AddToTail( newResponse );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CResponseSystem::IsRootCommand()
{
	if ( !Q_stricmp( token, "#include" ) )
		return true;
	if ( !Q_stricmp( token, "response" ) )
		return true;
	if ( !Q_stricmp( token, "enumeration" ) )
		return true;
	if ( !Q_stricmp( token, "criteria" ) )
		return true;
	if ( !Q_stricmp( token, "criterion" ) )
		return true;
	if ( !Q_stricmp( token, "rule" ) )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *kv - 
//-----------------------------------------------------------------------------
void CResponseSystem::ParseResponse( void )
{
	// Should have groupname at start
	char responseGroupName[ 128 ];

	ResponseGroup newGroup;
	AI_ResponseParams *rp = &newGroup.rp;

	// Response Group Name
	ParseToken();
	Q_strncpy( responseGroupName, token, sizeof( responseGroupName ) );

	while ( 1 )
	{
		ParseToken();

		// Oops, part of next definition
		if( IsRootCommand() )
		{
			Unget();
			break;
		}

		if ( !Q_stricmp( token, "{" ) )
		{
			while ( 1 )
			{
				ParseToken();
				if ( !Q_stricmp( token, "}" ) )
					break;

				if ( !Q_stricmp( token, "permitrepeats" ) )
				{
					newGroup.m_bDepleteBeforeRepeat = false;
					continue;
				}

				ParseOneResponse( responseGroupName, newGroup );
			}
			break;
		}

		if ( !Q_stricmp( token, "nodelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay.start = 0;
			rp->delay.range = 0;
			continue;
		}

		if ( !Q_stricmp( token, "defaultdelay" ) )
		{
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay.start = AIS_DEF_MIN_DELAY;
			rp->delay.range = ( AIS_DEF_MAX_DELAY - AIS_DEF_MIN_DELAY );
			continue;
		}
	
		if ( !Q_stricmp( token, "delay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_DELAYAFTERSPEAK;
			rp->delay = ReadInterval( token );
			continue;
		}
		
		if ( !Q_stricmp( token, "speakonce" ) )
		{
			rp->flags |= AI_ResponseParams::RG_SPEAKONCE;
			continue;
		}
		
		if ( !Q_stricmp( token, "odds" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_ODDS;
			rp->odds = clamp( atoi( token ), 0, 100 );
			continue;
		}
		
		if ( !Q_stricmp( token, "respeakdelay" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_RESPEAKDELAY;
			rp->respeakdelay = ReadInterval( token );
			continue;
		}
		
		if ( !Q_stricmp( token, "soundlevel" ) )
		{
			ParseToken();
			rp->flags |= AI_ResponseParams::RG_SOUNDLEVEL;
			rp->soundlevel = (soundlevel_t)TextToSoundLevel( token );
			continue;
		}

		ParseOneResponse( responseGroupName, newGroup );
	}

	m_Responses.Insert( responseGroupName, newGroup );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *criterion - 
//-----------------------------------------------------------------------------
int CResponseSystem::ParseOneCriterion( const char *criterionName )
{
	char key[ 128 ];
	char value[ 128 ];

	Criteria newCriterion;

	bool gotbody = false;

	while ( TokenWaiting() || !gotbody )
	{
		ParseToken();

		// Oops, part of next definition
		if( IsRootCommand() )
		{
			Unget();
			break;
		}

		if ( !Q_stricmp( token, "{" ) )
		{
			gotbody = true;

			while ( 1 )
			{
				ParseToken();
				if ( !Q_stricmp( token, "}" ) )
					break;

				// Look up subcriteria index
				int idx = m_Criteria.Find( token );
				if ( idx != m_Criteria.InvalidIndex() )
				{
					newCriterion.subcriteria.AddToTail( idx );
				}
				else
				{
					ResponseWarning( "Skipping unrecongized subcriterion '%s' in '%s'\n", token, criterionName );
				}
			}
			continue;
		}
		else if ( !Q_stricmp( token, "required" ) )
		{
			newCriterion.required = true;
		}
		else if ( !Q_stricmp( token, "weight" ) )
		{
			ParseToken();
			newCriterion.weight = (float)atof( token );
		}
		else
		{
			Assert( newCriterion.subcriteria.Count() == 0 );

			// Assume it's the math info for a non-subcriteria resposne
			Q_strncpy( key, token, sizeof( key ) );
			ParseToken();
			Q_strncpy( value, token, sizeof( value ) );

			newCriterion.name = CopyString( key );
			newCriterion.value = CopyString( value );

			gotbody = true;
		}
	}

	if ( !newCriterion.IsSubCriteriaType() )
	{
		ComputeMatcher( &newCriterion, newCriterion.matcher );
	}

	int idx = m_Criteria.Insert( criterionName, newCriterion );
	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *kv - 
//-----------------------------------------------------------------------------
void CResponseSystem::ParseCriterion( void )
{
	// Should have groupname at start
	char criterionName[ 128 ];
	ParseToken();
	Q_strncpy( criterionName, token, sizeof( criterionName ) );

	ParseOneCriterion( criterionName );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *kv - 
//-----------------------------------------------------------------------------
void CResponseSystem::ParseEnumeration( void )
{
	char enumerationName[ 128 ];
	ParseToken();
	Q_strncpy( enumerationName, token, sizeof( enumerationName ) );

	ParseToken();
	if ( Q_stricmp( token, "{" ) )
	{
		ResponseWarning( "Expecting '{' in enumeration '%s', got '%s'\n", enumerationName, token );
		return;
	}

	while ( 1 )
	{
		ParseToken();
		if ( !Q_stricmp( token, "}" ) )
			break;

		if ( Q_strlen( token ) <= 0 )
		{
			ResponseWarning( "Expecting more tokens in enumeration '%s'\n", enumerationName );
			break;
		}

		char key[ 128 ];

		Q_strncpy( key, token, sizeof( key ) );
		ParseToken();
		float value = (float)atof( token );

		char sz[ 128 ];
		Q_snprintf( sz, sizeof( sz ), "[%s::%s]", enumerationName, key );
		Q_strlower( sz );

		Enumeration newEnum;
		newEnum.value = value;

		if ( m_Enumerations.Find( sz ) == m_Enumerations.InvalidIndex() )
		{
			m_Enumerations.Insert( sz, newEnum );
		}
		else
		{
			ResponseWarning( "Ignoring duplication enumeration '%s'\n", sz );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *kv - 
//-----------------------------------------------------------------------------
void CResponseSystem::ParseRule( void )
{
	static int instancedCriteria = 0;

	char ruleName[ 128 ];
	ParseToken();
	Q_strncpy( ruleName, token, sizeof( ruleName ) );

	ParseToken();
	if ( Q_stricmp( token, "{" ) )
	{
		ResponseWarning( "Expecting '{' in rule '%s', got '%s'\n", ruleName, token );
		return;
	}

	// entries are "criteria", "response" or an in-line criteria to instance
	Rule newRule;

	char sz[ 128 ];

	while ( 1 )
	{
		ParseToken();
		if ( !Q_stricmp( token, "}" ) )
		{
			break;
		}

		if ( Q_strlen( token ) <= 0 )
		{
			ResponseWarning( "Expecting more tokens in rule '%s'\n", ruleName );
			break;
		}

		if ( !Q_stricmp( token, "matchonce" ) )
		{
			newRule.m_bMatchOnce = true;
			continue;
		}

		if ( !Q_stricmp( token, "response" ) )
		{
			// Read them until we run out.
			while ( TokenWaiting() )
			{
				ParseToken();
				int idx = m_Responses.Find( token );
				if ( idx != m_Responses.InvalidIndex() )
				{
					newRule.m_Responses.AddToTail( idx );
				}
				else
				{
					ResponseWarning( "No such response '%s'\n", token );
				}
			}
			continue;
		}

		if ( !Q_stricmp( token, "criteria" ) ||
			 !Q_stricmp( token, "criterion" ) )
		{
			// Read them until we run out.
			while ( TokenWaiting() )
			{
				ParseToken();

				int idx = m_Criteria.Find( token );
				if ( idx != m_Criteria.InvalidIndex() )
				{
					newRule.m_Criteria.AddToTail( idx );
				}
				else
				{
					ResponseWarning( "No such criterion '%s'\n", token );
				}
			}
			continue;
		}

		// It's an inline criteria, generate a name and parse it in
		Q_snprintf( sz, sizeof( sz ), "[%s%03i]", ruleName, ++instancedCriteria );
		Unget();
		int idx = ParseOneCriterion( sz );
		if ( idx != m_Criteria.InvalidIndex() )
		{
			newRule.m_Criteria.AddToTail( idx );
		}
	}

	m_Rules.Insert( ruleName, newRule );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CResponseSystem::GetCurrentToken() const
{
	if ( m_ScriptStack.Count() <= 0 )
		return -1;
	
	return m_ScriptStack[ 0 ].tokencount;
}


void CResponseSystem::ResponseWarning( const char *fmt, ... )
{
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, fmt);
	Q_vsnprintf(string, sizeof(string), fmt,argptr);
	va_end (argptr);

	Warning( "%s(token %i) : %s", GetCurrentScript(), GetCurrentToken(), string );
}


//-----------------------------------------------------------------------------
// Purpose: A special purpose response system associated with a custom entity
//-----------------------------------------------------------------------------
class CInstancedResponseSystem : public CResponseSystem
{
	typedef CResponseSystem BaseClass;

public:
	CInstancedResponseSystem( const char *scriptfile ) :
	  m_pszScriptFile( scriptfile )
	{
	}
	virtual const char *GetScriptFile( void ) 
	{
		return m_pszScriptFile;
	}

	// CAutoGameSystem
	virtual bool Init()
	{
		const char *basescript = GetScriptFile();
		LoadRuleSet( basescript );
		return true;
	}

	virtual void Release()
	{
		Clear();
		delete this;
	}
private:

	const char *m_pszScriptFile;
};

//-----------------------------------------------------------------------------
// Purpose: The default response system for expressive AIs
//-----------------------------------------------------------------------------
class CDefaultResponseSystem : public CResponseSystem, public CAutoGameSystem
{
	typedef CAutoGameSystem BaseClass;

public:
	virtual const char *GetScriptFile( void ) 
	{
		return "scripts/talker/response_rules.txt";
	}

	// CAutoServerSystem
	virtual bool Init();
	virtual void Shutdown();

	virtual void Release()
	{
		Assert( 0 );
	}

	void AddInstancedResponseSystem( const char *scriptfile, CResponseSystem *sys )
	{
		m_InstancedSystems.Insert( scriptfile, sys );
	}

	CResponseSystem *FindResponseSystem( const char *scriptfile )
	{
		int idx = m_InstancedSystems.Find( scriptfile );
		if ( idx == m_InstancedSystems.InvalidIndex() )
			return NULL;
		return m_InstancedSystems[ idx ];
	}

private:

	void ClearInstanced()
	{
		int c = m_InstancedSystems.Count();
		for ( int i = c - 1 ; i >= 0; i-- )
		{
			CResponseSystem *sys = m_InstancedSystems[ i ];
			sys->Release();
		}
		m_InstancedSystems.RemoveAll();
	}

	CUtlDict< CResponseSystem *, int > m_InstancedSystems;
};

static CDefaultResponseSystem defaultresponsesytem;
IResponseSystem *g_pResponseSystem = &defaultresponsesytem;
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDefaultResponseSystem::Init()
{
	const char *basescript = GetScriptFile();

	LoadRuleSet( basescript );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDefaultResponseSystem::Shutdown()
{
	// Wipe instanced versions
	ClearInstanced();

	// Clear outselves
	Clear();
	// IServerSystem chain
	BaseClass::Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Instance a custom response system
// Input  : *scriptfile - 
// Output : IResponseSystem
//-----------------------------------------------------------------------------
IResponseSystem *InstancedResponseSystemCreate( const char *scriptfile )
{
	IResponseSystem *sys = defaultresponsesytem.FindResponseSystem( scriptfile );
	if ( sys )
		return sys;

	CInstancedResponseSystem *newSys = new CInstancedResponseSystem( scriptfile );
	Assert( newSys );
	if ( !newSys->Init() )
	{
		Assert( 0 );
	}

	defaultresponsesytem.AddInstancedResponseSystem( scriptfile, newSys );

	return ( IResponseSystem * )newSys;
}
