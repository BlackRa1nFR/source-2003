#include "cbase.h"
#include "ai_nav_property.h"
#include "igamesystem.h"
#include "utlrbtree.h"


typedef CBaseEntity *ENTPTR;
static bool EntityLess( const ENTPTR &e0, const ENTPTR &e1 )
{
	return ((int)e0) < ((int)e1);
}

// UNDONE: This should be a hash of some kind as often as it gets queried
static CUtlRBTree<CBaseEntity *, unsigned short> gIgnoreList;

class CNavPropertyDatabase : public CAutoGameSystem, public IEntityListener
{
public:
	bool Init() 
	{ 
		gIgnoreList.SetLessFunc( EntityLess );
		gEntList.AddListenerEntity( (IEntityListener *)this );
		return true; 
	}
	
	virtual void LevelShutdownPreEntity() 
	{
		gIgnoreList.RemoveAll();
	}
	
	void Shutdown()
	{
		gEntList.RemoveListenerEntity( (IEntityListener *)this );
	}

	virtual void OnEntityCreated( CBaseEntity *pEntity ) {}

	virtual void OnEntityDeleted( CBaseEntity *pEntity )
	{
		gIgnoreList.Remove( pEntity );
	}
};

// singleton for the database
static CNavPropertyDatabase gNavProperties;

void NavPropertyAdd( CBaseEntity *pEntity, navproperties_t property )
{
	Assert(property==NAV_IGNORE);
	gIgnoreList.Insert( pEntity );
}


void NavPropertyRemove( CBaseEntity *pEntity, navproperties_t property )
{
	Assert(property==NAV_IGNORE);
	gIgnoreList.Remove( pEntity );
}


bool NavPropertyIsOnEntity( CBaseEntity *pEntity, navproperties_t property )
{
	Assert(property==NAV_IGNORE);
	if ( gIgnoreList.Find( pEntity ) != gIgnoreList.InvalidIndex() )
		return true;
	return false;
}

