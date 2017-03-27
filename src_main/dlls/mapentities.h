//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPENTITIES_H
#define MAPENTITIES_H
#ifdef _WIN32
#pragma once
#endif


class IMapEntityFilter
{
public:
	virtual bool ShouldCreateEntity( const char *pClassname ) = 0;
};

// Use the filter so you can prevent certain entities from being created out of the map.
// CSPort does this when restarting rounds. It wants to reload most entities from the map, but certain
// entities like the world entity need to be left intact.
void MapEntity_ParseAllEntities( const char *pMapData, IMapEntityFilter *pFilter=NULL, bool bActivateEntities=false );

const char *MapEntity_ParseEntity( CBaseEntity *&pEntity, const char *pEntData, IMapEntityFilter *pFilter );


#endif // MAPENTITIES_H
