//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "SpectateGames.h"

void CSpectateGames::RequestServers(int Start, const char *filterString)
{
	char filter[2048];

	strcpy(filter, filterString);
	strcat(filter, "\\proxy\\1");

	BaseClass::RequestServers(Start, filter);
}

bool CSpectateGames::CheckPrimaryFilters(serveritem_t &server)
{
	if (!server.proxy)
		return false;

	return BaseClass::CheckPrimaryFilters(server);
}


