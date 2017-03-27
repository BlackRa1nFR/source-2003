/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Generic NPC - purely for scripted sequence work.
//=========================================================
#include "cbase.h"
#include "shareddefs.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_baseactor.h"
#include "vstdlib/strtools.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICNPC_NOTSOLID					4 

ConVar flex_looktime( "flex_looktime", "5" );

//---------------------------------------------------------
// Sounds
//---------------------------------------------------------


//=========================================================
// NPC's Anim Events Go Here
//=========================================================

class CGenericActor : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CGenericActor, CAI_BaseActor );

	void	Spawn( void );
	void	Precache( void );
	float	MaxYawSpeed( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests ( void );

	
	void	TempGunEffect( void );

};
LINK_ENTITY_TO_CLASS( generic_actor, CGenericActor );

//=========================================================
// Classify - indicates this NPC's place in the 
// relationship table.
//=========================================================
Class_T	CGenericActor :: Classify ( void )
{
	return	CLASS_NONE;
}

//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CGenericActor :: MaxYawSpeed ( void )
{
	return 90;
}

//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericActor :: HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::HandleAnimEvent( pEvent );
}

//=========================================================
// GetSoundInterests - generic NPC can't hear.
//=========================================================
int CGenericActor :: GetSoundInterests ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CGenericActor :: Spawn()
{
	Precache();

	SetModel( STRING( GetModelName() ) );

/*
	if ( FStrEq( STRING( GetModelName() ), "models/player.mdl" ) )
		UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if ( FStrEq( STRING( GetModelName() ), "models/player.mdl" ) || FStrEq( STRING( GetModelName() ), "models/holo.mdl" ) )
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
	else
		UTIL_SetSize(this, NAI_Hull::Mins(HULL_HUMAN), NAI_Hull::Maxs(HULL_HUMAN));

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );

	NPCInit();

	if ( HasSpawnFlags( SF_GENERICNPC_NOTSOLID ) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_takedamage = DAMAGE_NO;
	}
	Relink();
}

//=========================================================
// Precache - precaches all resources this NPC needs
//=========================================================
void CGenericActor :: Precache()
{
	engine->PrecacheModel( STRING( GetModelName() ) );
}	

//=========================================================
// AI Schedules Specific to this NPC
//=========================================================






// -----------------------------------------------------------------------


// FIXME: delete this code

class CFlextalkActor : public CGenericActor
{
private:
	DECLARE_CLASS( CFlextalkActor, CGenericActor );
public:
	DECLARE_DATADESC();

	CFlextalkActor() { m_iszSentence = NULL_STRING; m_sentence = 0; }
	//void GenericCyclerSpawn(char *szModel, Vector vecMin, Vector vecMax);
	//virtual int	ObjectCaps( void ) { return (BaseClass :: ObjectCaps() | FCAP_IMPULSE_USE); }
	//int OnTakeDamage( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType );
	//void Spawn( void );
	//void Precache( void );
	//void Think( void );

	virtual void ProcessExpressions( void );

	// Don't treat as a live target
	//virtual bool IsAlive( void ) { return FALSE; }

	float m_flextime;
	int m_flexnum;
	float m_flextarget[64];
	float m_blinktime;
	float m_looktime;
	Vector m_lookTarget;
	float m_speaktime;
	int	m_istalking;
	int	m_phoneme;

	string_t m_iszSentence;
	int m_sentence;

	void SetFlexTarget( int flexnum, float value );
	int LookupFlex( const char *szTarget );
};

BEGIN_DATADESC( CFlextalkActor )

	DEFINE_FIELD( CFlextalkActor, m_flextime, FIELD_TIME ),
	DEFINE_FIELD( CFlextalkActor, m_flexnum, FIELD_INTEGER ),
	DEFINE_ARRAY( CFlextalkActor, m_flextarget, FIELD_FLOAT, 64 ),
	DEFINE_FIELD( CFlextalkActor, m_blinktime, FIELD_TIME ),
	DEFINE_FIELD( CFlextalkActor, m_looktime, FIELD_TIME ),
	DEFINE_FIELD( CFlextalkActor, m_lookTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CFlextalkActor, m_speaktime, FIELD_TIME ),
	DEFINE_FIELD( CFlextalkActor, m_istalking, FIELD_INTEGER ),
	DEFINE_FIELD( CFlextalkActor, m_phoneme, FIELD_INTEGER ),
	DEFINE_KEYFIELD( CFlextalkActor, m_iszSentence, FIELD_STRING, "Sentence" ),
	DEFINE_FIELD( CFlextalkActor, m_sentence, FIELD_INTEGER ),

END_DATADESC()



LINK_ENTITY_TO_CLASS( cycler_actor, CFlextalkActor );

extern ConVar	flex_expression;
extern ConVar	flex_talk;

// Cycler member functions


extern const char *predef_flexcontroller_names[];
extern float predef_flexcontroller_values[7][30];

void CFlextalkActor::SetFlexTarget( int flexnum, float value )
{
	m_flextarget[flexnum] = value;

	const char *pszType = GetFlexControllerType( flexnum );

	for (int i = 0; i < GetNumFlexControllers(); i++)
	{
		if (i != flexnum)
		{
			const char *pszOtherType = GetFlexControllerType( i );
			if (stricmp( pszType, pszOtherType ) == 0)
			{
				m_flextarget[i] = 0;
			}
		}
	}

	float value2 = value;
	if (1 || random->RandomFloat( 0.0, 1.0 ) < 0.2)
	{
		value2 = random->RandomFloat( value - 0.2, value + 0.2 );
		value2 = clamp( value2, 0.0, 1.0 );
	}


	// HACK, for now, consider then linked is named "right_" or "left_"
	if (strncmp( "right_", GetFlexControllerName( flexnum ), 6 ) == 0)
	{
		m_flextarget[flexnum+1] = value2;
	}
	else if (strncmp( "left_", GetFlexControllerName( flexnum ), 5 ) == 0)
	{
		m_flextarget[flexnum-1] = value2;
	}
}


int CFlextalkActor::LookupFlex( const char *szTarget  )
{
	for (int i = 0; i < GetNumFlexControllers(); i++)
	{
		const char *pszFlex = GetFlexControllerName( i );
		if (stricmp( szTarget, pszFlex ) == 0)
		{
			return i;
		}
	}
	return -1;
}


void CFlextalkActor :: ProcessExpressions( void )
{
	if (m_Expressions.Count() != 0)
	{
		BaseClass::ProcessExpressions( );
		return;
	}
	
	// only do this if they have more than eyelid movement
	if (GetNumFlexControllers() > 2)
	{
		const char *pszExpression = flex_expression.GetString();

		if (pszExpression && pszExpression[0] == '+' && pszExpression[1] != '\0')
		{
			int i;
			int j = atoi( &pszExpression[1] );
			for (i = 0; i < GetNumFlexControllers(); i++)
			{
				m_flextarget[m_flexnum] = 0;
			}

			for (i = 0; i < 35 && predef_flexcontroller_names[i]; i++)
			{
				m_flexnum = LookupFlex( predef_flexcontroller_names[i] );
				m_flextarget[m_flexnum] = predef_flexcontroller_values[j][i];
				// Msg( "%s %.3f\n", predef_flexcontroller_names[i], predef_flexcontroller_values[j][i] );
			}
		}
		else if (pszExpression && pszExpression[0] != '\0' && strcmp(pszExpression, "+") != 0)
		{
			char szExpression[128];
			char szTemp[32];

			Q_strncpy( szExpression, pszExpression ,sizeof(szExpression));
			char *pszExpression = szExpression;

			while (*pszExpression != '\0')
			{
				if (*pszExpression == '+')
					*pszExpression = ' ';
				
				pszExpression++;
			}

			pszExpression = szExpression;

			while (*pszExpression)
			{
				if (*pszExpression != ' ')
				{
					if (*pszExpression == '-')
					{
						for (int i = 0; i < GetNumFlexControllers(); i++)
						{
							m_flextarget[i] = 0;
						}
					}
					else if (*pszExpression == '?')
					{
						for (int i = 0; i < GetNumFlexControllers(); i++)
						{
							Msg( "\"%s\" ", GetFlexControllerName( i ) );
						}
						Msg( "\n" );
						flex_expression.SetValue( "" );
					}
					else
					{
						if (sscanf( pszExpression, "%s", szTemp ) == 1)
						{
							m_flexnum = LookupFlex( szTemp );

							if (m_flexnum != -1 && m_flextarget[m_flexnum] != 1)
							{
								m_flextarget[m_flexnum] = 1.0;
								// SetFlexTarget( m_flexnum );
							}
							pszExpression += strlen( szTemp ) - 1;
						}
					}
				}
				pszExpression++;
			}
		} 
		else if (m_flextime < gpGlobals->curtime)
		{
			m_flextime = gpGlobals->curtime + random->RandomFloat( 0.3, 0.5 ) * (30.0 / GetNumFlexControllers());
			m_flexnum = random->RandomInt( 0, GetNumFlexControllers() - 1 );

			if (m_flextarget[m_flexnum] == 1)
			{
				m_flextarget[m_flexnum] = 0;
			}
			else if (stricmp( GetFlexControllerType( m_flexnum ), "phoneme" ) != 0)
			{
				if (strstr( GetFlexControllerName( m_flexnum ), "upper_raiser" ) == NULL)
				{
					Msg( "%s:%s\n", GetFlexControllerType( m_flexnum ), GetFlexControllerName( m_flexnum ) );
					SetFlexTarget( m_flexnum, random->RandomFloat( 0.5, 1.0 ) );
				}
			}
		}

		// slide it up.
		for (int i = 0; i < GetNumFlexControllers(); i++)
		{
			if (GetFlexWeight( i ) != m_flextarget[i])
				SetFlexWeight( i, GetFlexWeight( i ) + (m_flextarget[i] - GetFlexWeight( i )) / random->RandomFloat( 2.0, 4.0 ) );

			if (m_flexWeight[i] > 1)
				m_flexWeight.Set( i, 1 );
			if (m_flexWeight[i] < 0)
				m_flexWeight.Set( i, 0 );
		}

		if (flex_talk.GetInt() == -1)
		{
			m_istalking = 1;

			char pszSentence[256];
			Q_snprintf( pszSentence,sizeof(pszSentence), "%s%d", STRING(m_iszSentence), m_sentence++ );
			int sentenceIndex = engine->SentenceIndexFromName( pszSentence );
			if (sentenceIndex >= 0)
			{
				Msg( "%d : %s\n", sentenceIndex, pszSentence );
				CPASAttenuationFilter filter( this );
				enginesound->EmitSentenceByIndex( filter, entindex(), CHAN_VOICE, sentenceIndex, 1, SNDLVL_TALKING, 0, PITCH_NORM );
			}
			else
			{
				m_sentence = 0;
			}

			flex_talk.SetValue( "0" );
		}
		else if (flex_talk.GetInt() == -2)
		{
			m_flNextEyeLookTime = gpGlobals->curtime + 1000.0;
		}
		else if (flex_talk.GetInt() == -3)
		{
			m_flNextEyeLookTime = gpGlobals->curtime;
			flex_talk.SetValue( "0" );
		}
		else if (flex_talk.GetInt() == -4)
		{
			AddLookTarget( UTIL_PlayerByIndex( 1 ), 0.5, flex_looktime.GetFloat()  );
			flex_talk.SetValue( "0" );
		}
		else if (flex_talk.GetInt() == -5)
		{
			PickRandomLookTarget( true );
			flex_talk.SetValue( "0" );
		}
	}
}
