
#ifndef PACKEDENTITYARRAY_H
#define PACKEDENTITYARRAY_H


#include "const.h"


class PackedEntities
{
public:
	PackedEntities() : entities(0) {}

	int				num_entities;
	int				max_entities;
	
	// FIXME: Can we use the packed ent dict on the client too?
	// Client only fields:
	PackedEntity	*entities;
};

// This is a convenient way to setup a PackedEntities list.
template<int arrayLen>
class PackedEntitiesArray : public PackedEntities
{
public:
						PackedEntitiesArray()
						{
							entities = m_Entities;
							num_entities = 0;
							max_entities = arrayLen;
						}

	PackedEntity		m_Entities[arrayLen];
};


#endif

