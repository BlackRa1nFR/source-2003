//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Hint node utilities and functions.
//
// $NoKeywords: $
//=============================================================================

#ifndef	AI_HINT_H
#define	AI_HINT_H
#pragma once

//Flags for FindHintNode
#define bits_HINT_NODE_NONE				0x00000000
#define bits_HINT_NODE_VISIBLE			0x00000001
#define	bits_HINT_NODE_NEAREST			0x00000002
// Find a random hintnode meeting other criteria
#define bits_HINT_NODE_RANDOM			0x00000004

//=========================================================
// hints - these MUST coincide with the HINTS listed under
// info_node in the FGD file!
//=========================================================
enum Hint_e
{
	HINT_ANY = -1,
	HINT_NONE = 0,
	HINT_NOT_USED_WORLD_DOOR,
	HINT_WORLD_WINDOW,
	HINT_NOT_USED_WORLD_BUTTON,
	HINT_NOT_USED_WORLD_MACHINERY,
	HINT_NOT_USED_WORLD_LEDGE,
	HINT_NOT_USED_WORLD_LIGHT_SOURCE,
	HINT_NOT_USED_WORLD_HEAT_SOURCE,
	HINT_NOT_USED_WORLD_BLINKING_LIGHT,
	HINT_NOT_USED_WORLD_BRIGHT_COLORS,
	HINT_NOT_USED_WORLD_HUMAN_BLOOD,
	HINT_NOT_USED_WORLD_ALIEN_BLOOD,

	HINT_WORLD_WORK_POSITION,
	HINT_WORLD_VISUALLY_INTERESTING,

	HINT_TACTICAL_COVER_MED	= 100,
	HINT_TACTICAL_COVER_LOW,
	HINT_TACTICAL_SPAWN,
	HINT_TACTICAL_PINCH,				// Exit / entrance to an arena
	HINT_NOT_USED_TACTICAL_GUARD,
	HINT_TACTICAL_ENEMY_DISADVANTAGED,	//Disadvantageous position for the enemy
	HINT_HEALTH_KIT,
	HINT_TACTICAL_BOOBY_TRAP, // good place for a booby trap device (combine soldier)

	HINT_NOT_USED_URBAN_STREETCORNER = 200,
	HINT_NOT_USED_URBAN_STREETLAMP,
	HINT_NOT_USED_URBAN_DARK_SPOT,
	HINT_NOT_USED_URBAN_POSTER,
	HINT_URBAN_SHELTER,

	HINT_NOT_USED_ASSASSIN_SECLUDED = 300,
	HINT_NOT_USED_ASSASSIN_RAFTERS,
	HINT_NOT_USED_ASSASSIN_GROUND,
	HINT_NOT_USED_ASSASSIN_MONKEYBARS,

	HINT_ANTLION_BURROW_POINT = 400,

	HINT_ROLLER_PATROL_POINT = 500,
	HINT_ROLLER_CLEANUP_POINT,

	HINT_NOT_USED_PSTORM_ROCK_SPAWN = 600,

	HINT_CROW_FLYTO_POINT = 700,

	 // TF2 Hints
	HINT_BUG_PATROL_POINT = 800,

	HINT_FOLLOW_WAIT_POINT = 900,

	// HL1 port hints
	HINT_HL1_WORLD_MACHINERY = 1000,
	HINT_HL1_WORLD_BLINKING_LIGHT,
	HINT_HL1_WORLD_HUMAN_BLOOD,
	HINT_HL1_WORLD_ALIEN_BLOOD,
};

//==================================================
// CHintCriteria
//==================================================

class CHintCriteria
{
public:

	CHintCriteria();
	~CHintCriteria();

	int		HasFlag( int bitmask )	const	{ return ( m_iFlags & bitmask );	}
	void	SetFlag( int bitmask );

	void		SetGroup( string_t group );
	string_t	GetGroup( void )	const	{ return m_strGroup;	}

	int		GetHintType( void )		const	{ return m_iHintType;	}
	bool	HasIncludeZones( void )	const	{ return ( m_zoneInclude.Count() != 0 ); }
	bool	HasExcludeZones( void )	const	{ return ( m_zoneExclude.Count() != 0 ); }
	
	void	AddIncludePosition( const Vector &position, float radius );
	void	AddExcludePosition( const Vector &position, float radius );
	void	SetHintType( int hintType );

	bool	InIncludedZone( const Vector &testPosition );
	bool	InExcludedZone( const Vector &testPosition );

private:

	struct	hintZone_t
	{
		Vector		position;
		float		radius;
	};

	typedef CUtlVector < hintZone_t >	zoneList_t;

	void		AddZone( zoneList_t &list, const Vector &position, float radius );
	bool		InZone( zoneList_t &zone, const Vector &testPosition );

	int			m_iFlags;
	int			m_iHintType;
	string_t	m_strGroup;
	
	zoneList_t	m_zoneInclude;
	zoneList_t	m_zoneExclude;
};

class CAI_Node;

//==================================================
// CHintCriteria
//==================================================

class CAI_Hint : public CBaseEntity
{
	DECLARE_CLASS( CAI_Hint, CBaseEntity );
public:

	DECLARE_DATADESC();

						CAI_Hint( void );
						~CAI_Hint( void );

	static CAI_Hint		*FindHint( CAI_BaseNPC *pNPC, const Vector &position, CHintCriteria *pHintCriteria );
	static CAI_Hint		*FindHint( CAI_BaseNPC *pNPC, CHintCriteria *pHintCriteria );
	static CAI_Hint		*FindHint( const Vector &position, CHintCriteria *pHintCriteria );
	static CAI_Hint		*FindHint( CAI_BaseNPC *pNPC, Hint_e nHintType, int nFlags, float flMaxDist, const Vector *pMaxDistFrom = NULL );

	// Purpose: Finds a random suitable hint within the requested radious of the npc
	static CAI_Hint		*FindHintRandom( CAI_BaseNPC *pNPC, Hint_e nHintType, int nFlags, float flMaxDist, const Vector *pMaxDistFrom = NULL );

	static CAI_Hint		*CreateHint(string_t nName, const Vector &pos, Hint_e nHintType, int nNodeID, string_t strGroup);
	static void			DrawHintOverlays(float flDrawDuration);

	static CAI_Hint		*m_pLastFoundHint;			// Last used hint 
	static CAI_Hint		*m_pAllHints;				// A linked list of all hints

	static int			GetFlags( const char *token );

	void				Spawn( void );
	bool				Lock( CBaseEntity *pNPC );	// Makes unavailable for hints
	void				Unlock( float delay = 0.0 );		// Makes available for hints after delay
	bool				IsLocked(void);				// Whether this node is available for use.
	bool				IsLockedBy( CBaseEntity *pNPC );				// Whether this node is available for use.
	void				GetPosition(CBaseCombatCharacter *pBCC, Vector *vPosition);
	void				GetPosition( Hull_t hull, Vector *vPosition );
	Vector				GetDirection( );
	float				Yaw(void);
	
	bool				IsViewable( void );

	string_t			GetGroup( void )	const { return m_strGroup;	}
	Hint_e				HintType(void)		const { return m_nHintType;  };
	CBaseEntity			*User(void)			{ return m_hHintOwner; };

	// Input handlers
	void InputEnableHint( inputdata_t &inputdata );
	void InputDisableHint( inputdata_t &inputdata );

	int					DrawDebugTextOverlays(void);

	CAI_Node			*GetNode( void );

	virtual int	ObjectCaps( void ) { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	// The next hint in list of all hints
	CAI_Hint			*m_pNextHint;				
	
private:

	Hint_e				m_nHintType;		// there is something interesting in the world at this node's position
	EHANDLE				m_hHintOwner;		// Is hint locked (being used by NPC)
	int					m_nNodeID;			// What nodes is this hint attached to
	int					m_iDisabled;		// Initial state
//	float				m_flRadius;			// range to search	// UNDONE
	float				m_flNextUseTime;	// When can I be used again?
	string_t			m_strGroup;			// Group name (if any) for this hint

	COutputEvent		m_OnNPCArrival;		// Triggered when the door is told to open.
};


#endif	//AI_HINT_H
