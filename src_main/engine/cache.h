#ifndef CACHE_H
#define CACHE_H
#pragma once

#include "cache_user.h"

void Cache_Flush (void);

void *Cache_Check (cache_user_t *c);
// returns the cached data, and moves to the head of the LRU list
// if present, otherwise returns NULL

void Cache_Free (cache_user_t *c);

void *Cache_Alloc (cache_user_t *c, int size, const char *name);
// Returns NULL if all purgable data was tossed and there still
// wasn't enough room.

void Cache_Report (void);

void Cache_EnterCriticalSection( );			// all cache entries that subsequently allocated or successfully checked are considered "locked" and will not be freed when additional memory is needed
void Cache_ExitCriticalSection( );			// reset all protected blocks to normal

#endif			// CACHE_H