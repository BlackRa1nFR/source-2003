
#ifndef CL_ENTS_H
#define CL_ENTS_H


#include "packed_entity.h"


extern PackedDataAllocator g_ClientPackedDataAllocator;


// Manage and access the baseline list.
bool			AllocateEntityBaselines(int nBaselines);
void			FreeEntityBaselines();

PackedEntity*	GetStaticBaseline( int entIndex );
bool			GetDynamicBaseline( int iClass, void const **pData, int *pDatalen );

// Allocate and free packed data for PackedEntity's.
bool			AllocPackedData( int size, DataHandle &data );
void			CL_UpdateFrameLerp();

#endif


