//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VISMAT_H
#define VISMAT_H
#ifdef _WIN32
#pragma once
#endif



void BuildVisLeafs( int threadnum );


// MPI uses these.
struct transfer_s* BuildVisLeafs_Start();

// If PatchCB is non-null, it is called after each row is generated (used by MPI).
void BuildVisLeafs_Cluster(
	int threadnum, 
	struct transfer_s *transfers, 
	int iCluster, 
	void (*PatchCB)(int iThread, int patchnum, patch_t *patch) );

void BuildVisLeafs_End( struct transfer_s *transfers );



#endif // VISMAT_H
