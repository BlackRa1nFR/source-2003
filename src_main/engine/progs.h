/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef PROGS_H
#define PROGS_H

#pragma once

// Forward declare this type to avoid problems
class CSaveRestoreData;
struct edict_t;

//============================================================================

edict_t *ED_Alloc( void );
void	ED_Free( edict_t *ed );

edict_t *EDICT_NUM(int n);
int NUM_FOR_EDICT(const edict_t *e);

//============================================================================


#endif // PROGS_H