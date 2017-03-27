//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef MAP_ENTITY_H
#define MAP_ENTITY_H
#pragma once

typedef struct keypair_s
{
	struct keypair_s *next;
	char	*key;
	char	*value;
} keypair_t;


typedef struct mapentity_s
{
	const char	*pClassName;
	Vector		origin;
	Vector		angles;
	int			model;
	const char 	*pModelName;
	keypair_t	*pKeyValues;
} mapentity_t;

extern mapentity_t *Map_EntityIndex( int index );
extern mapentity_t *Map_FindEntity( const char *pClassName );
extern int			Map_EntityCount( void );
extern const char *Map_EntityValue( const mapentity_t *pEntity, const char *pKeyName );

#endif // MAP_ENTITY_H
