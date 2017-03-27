#include <stdio.h>
#include <ctype.h>
#include "physics_material.h"
#include "physics_vehicle.h"

#include "ivp_physics.hxx"
#include "ivp_material.hxx"
#include "utlsymbol.h"
#include "commonmacros.h"
#include "vstdlib/strtools.h" 
#include "vcollide_parse_private.h"

static int Limit( int value, int low, int high )
{
	if ( value < low )
		value = low;
	if ( value > high )
		value = high;

	return value;
}

//-----------------------------------------------------------------------------
// Purpose: This is the data stored for each material/surface propery list
//-----------------------------------------------------------------------------
class CSurface : public IVP_Material
{
public:
// IVP_Material
    virtual IVP_DOUBLE get_friction_factor()
	{
		return data.friction;
	}
    
    virtual IVP_DOUBLE get_elasticity()
	{
		return data.elasticity;
	}
    virtual const char *get_name();
	// UNDONE: not implemented here.
	virtual IVP_DOUBLE get_second_friction_factor() 
	{ 
		return 0; 
	}
    virtual IVP_DOUBLE get_adhesion()
	{
		return 0;
	}

// strings
	CUtlSymbol			m_name;
	unsigned short		m_pad;

// physics properties
	surfacedata_t	data;
};


static IVP_Material_Simple g_ShadowMaterial( 0.8f, 0.f );

inline bool IsMoveable( IVP_Real_Object *pObject )
{
	IVP_Core *pCore = pObject->get_core();
	if ( pCore->pinned || pCore->physical_unmoveable )
		return false;
	return true;
}

inline bool IVPFloatPointIsZero( const IVP_U_Float_Point &test )
{
	const float eps = 1e-4;
	return test.quad_length() < eps ? true : false;
}

class CPhysicsSurfaceProps;

class CIVPMaterialManager : public IVP_Material_Manager
{
	typedef IVP_Material_Manager BaseClass;
public:
	CIVPMaterialManager( void );
	void Init( CPhysicsSurfaceProps *pProps ) { m_props = pProps; }
	void SetPropMap( int *map, int mapSize );
	int RemapIVPMaterialIndex( int ivpMaterialIndex );

	// IVP_Material_Manager
	virtual IVP_Material *get_material_by_index(const IVP_U_Point *world_position, int index);

    virtual IVP_DOUBLE get_friction_factor(IVP_Contact_Situation *situation)	// returns values >0, value of 1.0f means object stands on a 45 degres hill
	{
		// vehicle wheels get no friction with stuff that isn't ground
		// helps keep control of the car
		// traction on less than 60 degree slopes.
		if ( fabs(situation->surf_normal.k[1]) < 0.5 )
		{
			// BUGBUG: IVP sometimes sends us a bogus normal
			// when doing a material realc on existing contacts!
			if ( !IVPFloatPointIsZero(situation->surf_normal) )
			{
				if ( (IsVehicleWheel( situation->objects[0] ) && !IsMoveable( situation->objects[1] ))
					|| (IsVehicleWheel( situation->objects[1] ) && !IsMoveable( situation->objects[0] )) )
				{
					
					//Vector tmp;ConvertDirectionToHL( situation->surf_normal, tmp );Msg("Wheel sliding on surface %.2f %.2f %.2f\n", tmp.x, tmp.y, tmp.z );
					return 0;
				}
			}
		}

		return BaseClass::get_friction_factor( situation );
	}

#if _DEBUG
    virtual IVP_DOUBLE get_elasticity(IVP_Contact_Situation *situation)		// range [0, 1.0f[, the relative speed after a collision compared to the speed before
	{
		return BaseClass::get_elasticity( situation );
	}
#endif

private:
	CPhysicsSurfaceProps	*m_props;
	unsigned short			m_propMap[128];
};


//-----------------------------------------------------------------------------
// Purpose: This is the main database of materials
//-----------------------------------------------------------------------------
class CPhysicsSurfaceProps : public IPhysicsSurfacePropsInternal
{
public:
	CPhysicsSurfaceProps( void );
	~CPhysicsSurfaceProps( void );

	virtual int		ParseSurfaceData( const char *pFilename, const char *pTextfile );
	virtual int		SurfacePropCount( void );
	virtual int		GetSurfaceIndex( const char *pPropertyName );
	virtual void	GetPhysicsProperties( int surfaceDataIndex, float *density, float *thickness, float *friction, float *elasticity );
	virtual surfacedata_t *GetSurfaceData( int surfaceDataIndex );
	virtual const char *GetString( unsigned short stringTableIndex, int offset );
	virtual const char *GetPropName( int surfaceDataIndex );
	virtual void SetWorldMaterialIndexTable( int *pMapArray, int mapSize );
	virtual int RemapIVPMaterialIndex( int ivpMaterialIndex )
	{
		return m_ivpManager.RemapIVPMaterialIndex( ivpMaterialIndex );
	}
	bool IsReservedMaterialIndex( int materialIndex );
	virtual IVP_Material *GetReservedMaterial( int materialIndex );
	virtual const char *GetReservedMaterialName( int materialIndex );
	int GetReservedSurfaceIndex( const char *pPropertyName );

	// The database is derived from the IVP material class
	IVP_Material *GetIVPMaterial( int materialIndex );
	virtual int GetIVPMaterialIndex( IVP_Material *pIVP );
	IVP_Material_Manager *GetIVPManager( void ) { return &m_ivpManager; }

	const char *GetNameString( CUtlSymbol name )
	{
		return m_strings.String(name);
	}

private:
	CSurface	*GetInternalSurface( int materialIndex );
	
	void			AddStringList( CUtlVector<CUtlSymbol> &list, unsigned short *index, unsigned short *count );
	void			CopyPhysicsProperties( CSurface *pOut, int baseIndex );
	bool			AddFileToDatabase( const char *pFilename );

private:
	CUtlSymbolTable				m_strings;
	CUtlVector<CUtlSymbol>		m_soundList;
	CUtlVector<CSurface>		m_props;
	CUtlVector<CUtlSymbol>		m_fileList;
	CIVPMaterialManager			m_ivpManager;
};


// Singleton database object
CPhysicsSurfaceProps g_SurfaceDatabase;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPhysicsSurfaceProps, IPhysicsSurfaceProps, VPHYSICS_SURFACEPROPS_INTERFACE_VERSION, g_SurfaceDatabase);


// Global pointer to singleton for VPHYSICS.DLL internal access
IPhysicsSurfacePropsInternal *physprops = &g_SurfaceDatabase;


const char *CSurface::get_name()
{
	return g_SurfaceDatabase.GetNameString( m_name );
}

CPhysicsSurfaceProps::CPhysicsSurfaceProps( void ) : m_fileList(8,8)
{
	m_ivpManager.Init( this );
}


CPhysicsSurfaceProps::~CPhysicsSurfaceProps( void )
{
}

int CPhysicsSurfaceProps::SurfacePropCount( void )
{
	return m_props.Size();
}

// Add the filename to a list to make sure each file is only processed once
bool CPhysicsSurfaceProps::AddFileToDatabase( const char *pFilename )
{
	CUtlSymbol id = m_strings.AddString( pFilename );

	for ( int i = 0; i < m_fileList.Size(); i++ )
	{
		if ( m_fileList[i] == id )
			return false;
	}

	m_fileList.AddToTail( id );
	return true;
}

int CPhysicsSurfaceProps::GetSurfaceIndex( const char *pPropertyName )
{
	if ( pPropertyName[0] == '$' )
	{
		int index = GetReservedSurfaceIndex( pPropertyName );
		if ( index >= 0 )
			return index;
	}

	CUtlSymbol id = m_strings.Find( pPropertyName );
	if ( id.IsValid() )
	{
		// BUGBUG: Linear search is slow!!!
		for ( int i = 0; i < m_props.Size(); i++ )
		{
			if ( m_props[i].m_name == id )
				return i;
		}
	}

	return -1;
}


const char *CPhysicsSurfaceProps::GetPropName( int surfaceDataIndex )
{
	if ( IsReservedMaterialIndex( surfaceDataIndex ) )
	{
		return GetReservedMaterialName( surfaceDataIndex );
	}
	CSurface *pSurface = GetInternalSurface( surfaceDataIndex );
	if ( pSurface )
	{
		return GetNameString( pSurface->m_name );
	}
	return NULL;
}


// UNDONE: move reserved materials into this table, or into a parallel table
// that gets hooked out here.
CSurface *CPhysicsSurfaceProps::GetInternalSurface( int materialIndex )
{
	if ( materialIndex < 0 || materialIndex > m_props.Size()-1 )
	{
		return NULL;
	}
	return &m_props[materialIndex];
}

void CPhysicsSurfaceProps::GetPhysicsProperties( int materialIndex, float *density, float *thickness, float *friction, float *elasticity )
{
	CSurface *pSurface = GetInternalSurface( materialIndex );
	if ( pSurface )
	{
		if ( friction )
			*friction = (float)pSurface->data.friction;
		if ( elasticity )
			*elasticity = (float)pSurface->data.elasticity;
		if ( density )
			*density = pSurface->data.density;
		if ( thickness )
			*thickness = pSurface->data.thickness;
	}
}


surfacedata_t *CPhysicsSurfaceProps::GetSurfaceData( int materialIndex )
{
	CSurface *pSurface = GetInternalSurface( materialIndex );
	if (!pSurface)
		pSurface = GetInternalSurface( GetSurfaceIndex( "default" ) );

	assert ( pSurface );
	return &pSurface->data;
}

const char *CPhysicsSurfaceProps::GetString( unsigned short stringTableIndex, int offset )
{
	CUtlSymbol index = m_soundList[stringTableIndex + offset];
	return m_strings.String( index );
}


bool CPhysicsSurfaceProps::IsReservedMaterialIndex( int materialIndex )
{
	return (materialIndex > 127) ? true : false;
}

IVP_Material *CPhysicsSurfaceProps::GetReservedMaterial( int materialIndex )
{
	switch( materialIndex )
	{
	case MATERIAL_INDEX_SHADOW:
		return &g_ShadowMaterial;
	}

	return NULL;
}

const char *CPhysicsSurfaceProps::GetReservedMaterialName( int materialIndex )
{
	// NOTE: All of these must start with '$'
	switch( materialIndex )
	{
	case MATERIAL_INDEX_SHADOW:
		return "$MATERIAL_INDEX_SHADOW";
	}

	return NULL;
}

int CPhysicsSurfaceProps::GetReservedSurfaceIndex( const char *pPropertyName )
{
	if ( !Q_stricmp( pPropertyName, "$MATERIAL_INDEX_SHADOW" ) )
	{
		return MATERIAL_INDEX_SHADOW;
	}
	return -1;
}

IVP_Material *CPhysicsSurfaceProps::GetIVPMaterial( int materialIndex )
{
	if ( IsReservedMaterialIndex( materialIndex ) )
	{
		return GetReservedMaterial( materialIndex );
	}
	return GetInternalSurface(materialIndex);
}


int CPhysicsSurfaceProps::GetIVPMaterialIndex( IVP_Material *pIVP )
{
	int index = (CSurface *)pIVP - m_props.Base();
	if ( index >= 0 && index < m_props.Size() )
		return index;

	return -1;
}

void CPhysicsSurfaceProps::AddStringList( CUtlVector<CUtlSymbol> &list, unsigned short *index, unsigned short *count )
{
	if ( !list.Size() )
		return;

	*index = m_soundList.AddMultipleToTail( list.Size() );
	for ( int i = 0; i < list.Size(); i++ )
	{
		m_soundList[*index+i] = list[i];
	}
	*count = list.Size();
}


enum stringlist_t
{
	STRING_STEP_LEFT = 0,
	STRING_STEP_RIGHT,
	STRING_IMPACT_SOUND,
	STRING_SCRAPE_SOUND,
	STRING_BULLET_IMPACT_SOUND,
	STRING_BULLET_DECAL,

	STRING_LIST_COUNT
};

void CPhysicsSurfaceProps::CopyPhysicsProperties( CSurface *pOut, int baseIndex )
{
	CSurface *pSurface = GetInternalSurface( baseIndex );
	if ( pSurface )
	{
		pOut->data.density = pSurface->data.density;
		pOut->data.thickness = pSurface->data.thickness;
		pOut->data.friction = pSurface->data.friction;
		pOut->data.elasticity = pSurface->data.elasticity;

		pOut->data.gameMaterial = pSurface->data.gameMaterial;
	}
}


int CPhysicsSurfaceProps::ParseSurfaceData( const char *pFileName, const char *pTextfile )
{
	if ( !AddFileToDatabase( pFileName ) )
	{
		return 0;
	}

	const char *pText = pTextfile;

	do
	{
		char key[MAX_KEYVALUE], value[MAX_KEYVALUE];

		pText = ParseKeyvalue( pText, key, value );
		if ( !strcmp(value, "{") )
		{
			CSurface prop;
			int baseMaterial = GetSurfaceIndex( "default" );

			// we accumulate the sound/string lists here separately in case
			// they aren't contiguous in the text file
			CUtlVector<CUtlSymbol>	stringlists[STRING_LIST_COUNT];

			memset( &prop.data, 0, sizeof(prop.data) );
			prop.m_name = m_strings.AddString( key );
			prop.data.gameMaterial = 0;
			prop.data.maxSpeedFactor = 1.0f;
			prop.data.jumpFactor = 1.0f;
			prop.data.climbable = 0.0f;
			CopyPhysicsProperties( &prop, baseMaterial );

			do
			{
				pText = ParseKeyvalue( pText, key, value );
				if ( !strcmpi( key, "}" ) )
				{
					// already in the database, don't add again
					if ( GetSurfaceIndex( m_strings.String(prop.m_name) ) >= 0 )
						break;

					// Add each string list in one contiguous chunk
					AddStringList( stringlists[STRING_STEP_LEFT], &prop.data.stepleft, &prop.data.stepleftCount );
					AddStringList( stringlists[STRING_STEP_RIGHT], &prop.data.stepright, &prop.data.steprightCount );
					AddStringList( stringlists[STRING_IMPACT_SOUND], &prop.data.impact, &prop.data.impactCount );
					AddStringList( stringlists[STRING_SCRAPE_SOUND], &prop.data.scrape, &prop.data.scrapeCount );
					AddStringList( stringlists[STRING_BULLET_IMPACT_SOUND], &prop.data.bulletImpact, &prop.data.bulletImpactCount );
					AddStringList( stringlists[STRING_BULLET_DECAL], &prop.data.bulletDecal, &prop.data.bulletDecalCount );

					CSurface *pSurface = GetInternalSurface( baseMaterial );

					// Fill in empty slots with default values;
					if ( pSurface )
					{
						// Write any values that the script explicitly did not set
						if ( !prop.data.stepleftCount )
						{
							prop.data.stepleft = pSurface->data.stepleft;
							prop.data.stepleftCount = pSurface->data.stepleftCount;
						}
						if ( !prop.data.steprightCount )
						{
							prop.data.stepright = pSurface->data.stepright;
							prop.data.steprightCount = pSurface->data.steprightCount;
						}
						if ( !prop.data.impactCount )
						{
							prop.data.impact = pSurface->data.impact;
							prop.data.impactCount = pSurface->data.impactCount;
						}
						if ( !prop.data.scrapeCount )
						{
							prop.data.scrape = pSurface->data.scrape;
							prop.data.scrapeCount = pSurface->data.scrapeCount;
						}
						if ( !prop.data.bulletImpactCount )
						{
							prop.data.bulletImpact = pSurface->data.bulletImpact;
							prop.data.bulletImpactCount = pSurface->data.bulletImpactCount;
						}
						if ( !prop.data.bulletDecalCount )
						{
							prop.data.bulletDecal = pSurface->data.bulletDecal;
							prop.data.bulletDecalCount = pSurface->data.bulletDecalCount;
						}
					}
					m_props.AddToTail( prop );
					break;
				}
				else if ( !strcmpi( key, "base" ) )
				{
					baseMaterial = GetSurfaceIndex( value );
					CopyPhysicsProperties( &prop, baseMaterial );
				}
				else if ( !strcmpi( key, "thickness" ) )
				{
					prop.data.thickness = atof(value);
				}
				else if ( !strcmpi( key, "density" ) )
				{
					prop.data.density = atof(value);
				}
				else if ( !strcmpi( key, "elasticity" ) )
				{
					prop.data.elasticity = atof(value);
				}
				else if ( !strcmpi( key, "friction" ) )
				{
					prop.data.friction = atof(value);
				}
				else if ( !strcmpi( key, "maxspeedfactor" ) )
				{
					prop.data.maxSpeedFactor = atof(value);
				}
				else if ( !strcmpi( key, "jumpfactor" ) )
				{
					prop.data.jumpFactor = atof(value);
				}
				else if ( !strcmpi( key, "climbable" ) )
				{
					prop.data.climbable = atoi(value);
				}
				else if ( !strcmpi( key, "stepleft" ) )
				{
					CUtlSymbol sym = m_strings.AddString( value );
					stringlists[STRING_STEP_LEFT].AddToTail( sym );
				}
				else if ( !strcmpi( key, "stepright" ) )
				{
					CUtlSymbol sym = m_strings.AddString( value );
					stringlists[STRING_STEP_RIGHT].AddToTail( sym );
				}
				else if ( !strcmpi( key, "impact" ) )
				{
					CUtlSymbol sym = m_strings.AddString( value );
					stringlists[STRING_IMPACT_SOUND].AddToTail( sym );
				}
				else if ( !strcmpi( key, "scrape" ) )
				{
					CUtlSymbol sym = m_strings.AddString( value );
					stringlists[STRING_SCRAPE_SOUND].AddToTail( sym );
				}
				else if ( !strcmpi( key, "bulletimpact" ) )
				{
					CUtlSymbol sym = m_strings.AddString( value );
					stringlists[STRING_BULLET_IMPACT_SOUND].AddToTail( sym );
				}
				else if ( !strcmpi( key, "bulletdecal" ) )
				{
					CUtlSymbol sym = m_strings.AddString( value );
					stringlists[STRING_BULLET_DECAL].AddToTail( sym );
				}
				else if ( !strcmpi( key, "gamematerial" ) )
				{
					if ( strlen(value) == 1 && !isdigit(value[0]) )
					{
						prop.data.gameMaterial = toupper(value[0]);
					}
					else
					{
						prop.data.gameMaterial = atoi(value);
					}
				}
				else if ( !strcmpi( key, "dampening" ) )
				{
					prop.data.dampening = atof(value);
				}
				else
				{
					// force a breakpoint
					AssertMsg2( 0, "Bad surfaceprop key %s (%s)\n", key, value );
				}
			} while (pText);
		}
	} while (pText);

	return m_props.Size();
}


void CPhysicsSurfaceProps::SetWorldMaterialIndexTable( int *pMapArray, int mapSize )
{
	m_ivpManager.SetPropMap( pMapArray, mapSize );
}

CIVPMaterialManager::CIVPMaterialManager( void ) : IVP_Material_Manager( IVP_FALSE )
{
	// by default every index maps to itself (NULL translation)
	for ( int i = 0; i < ARRAYSIZE(m_propMap); i++ )
	{
		m_propMap[i] = i;
	}
}

int CIVPMaterialManager::RemapIVPMaterialIndex( int ivpMaterialIndex )
{
	if ( ivpMaterialIndex > 127 )
		return ivpMaterialIndex;
	
	return m_propMap[ivpMaterialIndex];
}

// remap the incoming (from IVP) index and get the appropriate material
// note that ivp will only supply indices between 1 and 127
IVP_Material *CIVPMaterialManager::get_material_by_index(const IVP_U_Point *world_position, int index)
{
	return m_props->GetIVPMaterial( RemapIVPMaterialIndex(index) );
}

// Installs a LUT for remapping IVP material indices to physprop indices
// A table of the names of the materials in index order is stored with the 
// compiled bsp file.  This is then remapped dynamically without touching the
// per-triangle indices on load.  If we wanted to support multiple LUTs, it would
// be better to preprocess/remap the triangles in the collision models at load time
void CIVPMaterialManager::SetPropMap( int *map, int mapSize )
{
	// ??? just ignore any extra bits
	if ( mapSize > 128 )
	{
		mapSize = 128;
	}

	for ( int i = 0; i < mapSize; i++ )
	{
		m_propMap[i] = (unsigned short)map[i];
	}
}
