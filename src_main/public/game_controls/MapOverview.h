//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: MiniMap.h: interface for the CMiniMap class.
//
// $NoKeywords: $
//=============================================================================

#if !defined HLTVPANEL_H
#define HLTVPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <imapoverview.h>
#include <igameevents.h>
#include <shareddefs.h>

#define MAX_TRAIL_LENGTH	60
#define OVERVIEW_MAP_SIZE	1024	// an overview map is 1024x1024 pixels

class CMapOverview : public vgui::Panel, public IGameEventListener, public IMapOverview
{

	DECLARE_CLASS_SIMPLE( CMapOverview, vgui::Panel );

public:	

	CMapOverview(vgui::Panel *parent, IGameEventManager * gameeventmanager);
	virtual ~CMapOverview();

private:	// private structures & types
	
	typedef vgui::Panel BaseClass;

	// list of game events the hLTV takes care of

	enum HLTVEvent_e {
		PLAYER_KILLED = 0,	// data1 = attacker, data2 = victim, data3 weapon
		PLAYER_HURT,		// data1 = attacker, data2 = victim
		BOMB_PLACED,		// data1 = player
		BOMB_DEFUSED,		// data1 = player
		BOMB_EXPLODE,		// data1 = xpos, data2 = ypos
		HOSTAGE_KILLED,		// data1 = attacker, data2 = hostage
		HOSTAGE_RESCUED,	// data1 = hostage 
		GRENADE_SMOKE,		// data1 = xpos, data2 = ypos
		GRENADE_HE,			// data1 = xpos, data2 = ypos
		GRENADE_FLASH,		// data1 = xpos, data2 = ypos
		TRACER_SHOOT,		// data1 = xpos, data2 = ypos
		ROUND_TIMER,		// data1 = seconds count down (time left, bomb timer, last 5 sec eg)
		ROUND_START,		// data1 = round number
		ROUND_END,			// data1 = winning team (maybe reason type)
		GAME_STARTS,		// data1 = gametype, data2 = rounds or timelimit
		GAME_ENDS,			// data1 = winning team
		MAPEVENTS_MAX		// last event
	};

	typedef struct {
		int		xpos;
		int		ypos;
	} FootStep_t;	

	typedef struct HLTVEntity_s {
		int		entityNr;	// 
		char	name[32];	// 32 Byte
		int		team;		// N,T,CT,H
		int		health;		// 0..100, 7 bit
		int		xpos;		// current x pos
		int		ypos;		// current y pos
		int		origin;		// view origin 0..360
		FootStep_t	trail[MAX_TRAIL_LENGTH];	// save 1 footstep each second for 1 minute
	} HLTVEntity_t;

	typedef struct {
		HLTVEvent_e		type;		// type of event/effect
		float			startTime;	// time of start
		int				idata[8];	// player/entity/position/team whatever
		char			text[255];	// any additional text
	} HLTVEffect_t;

public: // IGameEventListener

	virtual void FireGameEvent( KeyValues * event);
	virtual void GameEventsUpdated();
	virtual bool IsLocalListener() { return false; };

public:	// VGUI overrides

	virtual void Paint();
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	// virtual void PaintBackground();

public:	// IMapOverview Interfaces

	virtual void SetVisible(bool visible);
	virtual void SetBounds(int x, int y, int wide, int tall);

	virtual bool IsVisible();
	virtual void GetBounds(int& x, int& y, int& wide, int& tall);

	virtual void ShowPlayerNames(bool state);
	virtual void ShowTracers(bool state);
	virtual void ShowExplosions(bool state);
	virtual void ShowHealth(bool state);
	virtual void ShowHurts(bool state);
	virtual void ShowTrails(float seconds); 

	virtual void SetMap(const char * map);
	virtual bool SetTeamColor(int team, Color color);
	virtual void SetZoom( float zoom );
	virtual void SetAngle( float angle);
	virtual void SetCenter( Vector2D &mappos); 
	virtual Vector2D WorldToMap( Vector &worldpos );
	
private:
	
	Vector2D	MapToPanel( const Vector2D &mappos );

	void DrawMapTexture();
	
	IGameEventManager * m_GameEventManager;
		
	CUtlVector<HLTVEntity_t>	m_Entities;
	CUtlVector<HLTVEffect_t>	m_Effects;

	Color m_TeamColors[MAX_TEAMS];

	bool m_bShowPlayerNames;
	bool m_bShowTracers;
	bool m_bShowExplosions;
	bool m_bShowHealth;
	bool m_bShowHurts;
	bool m_bShowTrails;

	int	 m_nMapTextureID;	// texture id for current overview image
	KeyValues * m_MapKeyValues; // keyvalues describinng overview parameters

	Vector	m_MapOrigin;	// read from KeyValues files
	float	m_fMapScale;	// origin and sacle used when screenshot was made

	float	m_fZoom;		// current zoom n = overview panel shows 1/n^2 of whole map 
	Vector2D m_ViewOrigin;	// map coordinates that are in the center of the pverview panel
	float	m_fViewAngle;	// rototaion of overview map
	
};

#endif //
