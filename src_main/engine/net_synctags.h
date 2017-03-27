//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef NET_SYNCTAGS_H
#define NET_SYNCTAGS_H
#ifdef _WIN32
#pragma once
#endif


class bf_read;
class bf_write;


// SyncTags are used as a debugging tool. If net_synctags is set to 1, then the tags
// are put into the bit streams and verified on the client.
void SyncTag_Write( bf_write *pBuf, const char *pTag );
void SyncTag_Read( bf_read *pBuf, const char *pTag );


#endif // NET_SYNCTAGS_H
