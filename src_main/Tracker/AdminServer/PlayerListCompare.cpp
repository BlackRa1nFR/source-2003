//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include "ServerListCompare.h"
#include "server.h"
#include "serverpage.h"


#include <VGUI_ListPanel.h>
#include <VGUI_KeyValues.h>

	
//-----------------------------------------------------------------------------
// Purpose: Ping comparison function
//-----------------------------------------------------------------------------
int __cdecl PlayerNameCompare(const void *elem1, const void *elem2 )
{
	vgui::ListPanel::DATAITEM *p1, *p2;
	p1 = *(vgui::ListPanel::DATAITEM **)elem1;
	p2 = *(vgui::ListPanel::DATAITEM **)elem2;

	if ( !p1 || !p2 )  // No meaningful comparison
	{
		return 0;  
	}

	const char *name1 = p1->kv->GetString("name");
	const char *name2 = p2->kv->GetString("name");

	return stricmp(name1,name2);
}


//-----------------------------------------------------------------------------
// Purpose: Ping comparison function
//-----------------------------------------------------------------------------
int __cdecl PlayerPingCompare(const void *elem1, const void *elem2 )
{
	vgui::ListPanel::DATAITEM *p1, *p2;
	p1 = *(vgui::ListPanel::DATAITEM **)elem1;
	p2 = *(vgui::ListPanel::DATAITEM **)elem2;

	if ( !p1 || !p2 )  // No meaningful comparison
	{
		return 0;  
	}

	int ping1 = p1->kv->GetInt("ping");
	int ping2 = p2->kv->GetInt("ping");


	if ( ping1 < ping2 )
		return -1;
	else if ( ping1 > ping2 )
		return 1;

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Ping comparison function
//-----------------------------------------------------------------------------
int __cdecl PlayerAuthCompare(const void *elem1, const void *elem2 )
{
	vgui::ListPanel::DATAITEM *p1, *p2;
	p1 = *(vgui::ListPanel::DATAITEM **)elem1;
	p2 = *(vgui::ListPanel::DATAITEM **)elem2;

	if ( !p1 || !p2 )  // No meaningful comparison
	{
		return 0;  
	}

	const char *authid1 = p1->kv->GetString("authid");
	const char *authid2 = p2->kv->GetString("authid");

	return stricmp(authid1,authid2);
}




//-----------------------------------------------------------------------------
// Purpose: Loss comparison function
//-----------------------------------------------------------------------------
int __cdecl PlayerLossCompare(const void *elem1, const void *elem2 )
{
	vgui::ListPanel::DATAITEM *p1, *p2;
	p1 = *(vgui::ListPanel::DATAITEM **)elem1;
	p2 = *(vgui::ListPanel::DATAITEM **)elem2;

	if ( !p1 || !p2 )  // No meaningful comparison
	{
		return 0;  
	}

	int loss1 = p1->kv->GetInt("loss");
	int loss2 = p2->kv->GetInt("loss");


	if ( loss1 < loss2 )
		return -1;
	else if ( loss1 > loss2 )
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Frags comparison function
//-----------------------------------------------------------------------------
int __cdecl PlayerFragsCompare(const void *elem1, const void *elem2 )
{
	vgui::ListPanel::DATAITEM *p1, *p2;
	p1 = *(vgui::ListPanel::DATAITEM **)elem1;
	p2 = *(vgui::ListPanel::DATAITEM **)elem2;

	if ( !p1 || !p2 )  // No meaningful comparison
	{
		return 0;  
	}

	int frags1 = p1->kv->GetInt("frags");
	int frags2 = p2->kv->GetInt("frags");


	if ( frags1 < frags2 )
		return -1;
	else if ( frags1 > frags2 )
		return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Player connection time comparison function
//-----------------------------------------------------------------------------
int __cdecl PlayerTimeCompare(const void *elem1, const void *elem2 )
{
	vgui::ListPanel::DATAITEM *p1, *p2;
	int h1,h2,m1,m2,s1,s2;
	float t1=0,t2=0;

	p1 = *(vgui::ListPanel::DATAITEM **)elem1;
	p2 = *(vgui::ListPanel::DATAITEM **)elem2;

	if ( !p1 || !p2 )  // No meaningful comparison
	{
		return 0;  
	}

	const char *time1 = p1->kv->GetString("time");
	const char *time2 = p2->kv->GetString("time");

	sscanf(time1,"%i:%i:%i",&h1,&m1,&s1);
	sscanf(time2,"%i:%i:%i",&h2,&m2,&s2);

	t1=(float)(h1*3600+m1*60+s1);
	t2=(float)(h2*3600+m2*60+s2);
	
	if ( t1 < t2 )
		return -1;
	else if ( t1 > t2 )
		return 1;

	return 0;
}