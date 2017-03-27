/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/


//
// studiomdl.c: generates a studio .mdl file from a .qc script
// models/<scriptname>.mdl.
//


#pragma warning( disable : 4244 )
#pragma warning( disable : 4237 )
#pragma warning( disable : 4305 )


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include "istudiorender.h"
#include "filesystem_tools.h"

#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib.h"
#define EXTERN
#include "studio.h"
#include "studiomdl.h"
#include "collisionmodel.h"
#include "optimize.h"
#include "vstdlib/strtools.h"
#include "bspflags.h"


bool g_collapse_bones = false;
bool g_quiet = false;
bool g_badCollide = false;
bool g_IHVTest = false;
bool g_bCheckLengths = false;
bool g_bPrintBones = false;
bool g_bPerf = false;

CUtlVector< s_hitboxset > g_hitboxsets;
CUtlVector< char >	g_KeyValueText;

/*
=================
=================
*/

int k_memtotal;
void *kalloc( int num, int size )
{
	// printf( "calloc( %d, %d )\n", num, size );
	// printf( "%d ", num * size );
	k_memtotal += num * size;
	return calloc( num, size );
}

void kmemset( void *ptr, int value, int size )
{
	// printf( "kmemset( %x, %d, %d )\n", ptr, value, size );
	memset( ptr, value, size );
	return;
}


//-----------------------------------------------------------------------------
// Key value block
//-----------------------------------------------------------------------------
static void AppendKeyValueText( CUtlVector< char > *pKeyValue, const char *pString )
{
	int nLen = strlen(pString);
	int nFirst = pKeyValue->AddMultipleToTail( nLen );
	memcpy( pKeyValue->Base() + nFirst, pString, nLen );
}

int	KeyValueTextSize( CUtlVector< char > *pKeyValue )
{
	return pKeyValue->Count();
}

const char *KeyValueText( CUtlVector< char > *pKeyValue )
{
	return pKeyValue->Base();
}

void Option_KeyValues( CUtlVector< char > *pKeyValue );

/*
=================
=================
*/


/*
=================
=================
*/




int lookupControl( char *string )
{
	if (stricmp(string,"X")==0) return STUDIO_X;
	if (stricmp(string,"Y")==0) return STUDIO_Y;
	if (stricmp(string,"Z")==0) return STUDIO_Z;
	if (stricmp(string,"XR")==0) return STUDIO_XR;
	if (stricmp(string,"YR")==0) return STUDIO_YR;
	if (stricmp(string,"ZR")==0) return STUDIO_ZR;

	if (stricmp(string,"LX")==0) return STUDIO_LX;
	if (stricmp(string,"LY")==0) return STUDIO_LY;
	if (stricmp(string,"LZ")==0) return STUDIO_LZ;
	if (stricmp(string,"LXR")==0) return STUDIO_LXR;
	if (stricmp(string,"LYR")==0) return STUDIO_LYR;
	if (stricmp(string,"LZR")==0) return STUDIO_LZR;

	return -1;
}



/*
=================
=================
*/

void Cmd_PoseParameter( )
{
	if (g_numposeparameters >= MAXSTUDIOPOSEPARAM)
	{
		Error( "too many pose parameters (max %d)\n", MAXSTUDIOPOSEPARAM );
	}

	// name
	GetToken (false);
	strcpyn( g_pose[g_numposeparameters].name, token );

	// min
	GetToken (false);
	g_pose[g_numposeparameters].min = atof (token);

	// max
	GetToken (false);
	g_pose[g_numposeparameters].max = atof (token);

	while (TokenAvailable())
	{
		GetToken (false);

		if (!stricmp( token, "wrap" ))
		{
			g_pose[g_numposeparameters].flags |= STUDIO_LOOPING;
			g_pose[g_numposeparameters].loop = g_pose[g_numposeparameters].max - g_pose[g_numposeparameters].min;
		}
		else if (!stricmp( token, "loop" ))
		{
			g_pose[g_numposeparameters].flags |= STUDIO_LOOPING;
			GetToken (false);
			g_pose[g_numposeparameters].loop = atof( token );
		}
	}
	g_numposeparameters++;
}


int LookupPoseParameter( char *name )
{
	int i;
	for ( i = 0; i < g_numposeparameters; i++)
	{
		if (!stricmp( name, g_pose[i].name))
		{
			return i;
		}
	}
	strcpyn( g_pose[i].name, name );
	g_numposeparameters = i + 1;

	if (g_numposeparameters > MAXSTUDIOPOSEPARAM)
	{
		Error( "too many pose parameters (max %d)\n", MAXSTUDIOPOSEPARAM );
	}

	return i;
}

/*
=================
=================
*/

// search case-insensitive for string2 in string
char *stristr( const char *string, const char *string2 )
{
	int c, len;
	c = tolower( *string2 );
	len = strlen( string2 );

	while (string) {
		for (; *string && tolower( *string ) != c; string++);
		if (*string) {
			if (strnicmp( string, string2, len ) == 0) {
				break;
			}
			string++;
		}
		else {
			return NULL;
		}
	}
	return (char *)string;
}

/*
=================
=================
*/


int lookup_texture( char *texturename )
{
	int i;

	StripExtension( texturename );

	for (i = 0; i < g_numtextures; i++) 
	{
		if (stricmp( g_texture[i].name, texturename ) == 0) 
		{
			return i;
		}
	}

	if (i >= MAXSTUDIOSKINS)
		Error("Too many materials used, max %d\n", ( int )MAXSTUDIOSKINS );

//	printf( "texture %d = %s\n", i, texturename );
	strcpyn( g_texture[i].name, texturename );

	g_texture[i].material = -1;
	/*
	if (stristr( texturename, "chrome" ) != NULL) {
		texture[i].flags = STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME;
	}
	else {
		texture[i].flags = 0;
	}
	*/
	g_numtextures++;
	return i;
}


int use_texture_as_material( int textureindex )
{
	if (g_texture[textureindex].material == -1)
	{
		// printf("%d %d %s\n", textureindex, g_nummaterials, g_texture[textureindex].name );
		g_material[g_nummaterials] = textureindex;
		g_texture[textureindex].material = g_nummaterials++;
	}

	return g_texture[textureindex].material;
}

int material_to_texture( int material )
{
	int i;
	for( i = 0; i < g_numtextures; i++ )
	{
		if( g_texture[i].material == material )
		{
			return i;
		}
	}
	return -1;
}

//Wrong name for the use of it.
void scale_vertex( Vector &org )
{
	org[0] = org[0] * g_currentscale;
	org[1] = org[1] * g_currentscale;
	org[2] = org[2] * g_currentscale;
}



void SetSkinValues( )
{
	int			i, j;
	int			index;

	if (numcdtextures == 0)
	{
		char szName[256];

		// strip down till it finds "models"
		strcpyn( szName, fullpath );
		while (szName[0] != '\0' && strnicmp( "models", szName, 6 ) != 0)
		{
			strcpy( &szName[0], &szName[1] );
		}
		if (szName[0] != '\0')
		{
			StripFilename( szName );
			strcat( szName, "/" );
		}
		else
		{
			if( *g_pPlatformName )
			{
				strcat( szName, "platform_" );
				strcat( szName, g_pPlatformName );
				strcat( szName, "/" );	
			}
			strcpy( szName, "models/" );	
			strcat( szName, outname );
			StripExtension( szName );
			strcat( szName, "/" );
		}
		cdtextures[0] = strdup( szName );
		numcdtextures = 1;
	}

	for (i = 0; i < g_numtextures; i++)
	{
		char szName[256];
		strcpy( szName, g_texture[i].name );
		StripExtension( szName );
		strcpyn( g_texture[i].name, szName );
	}

	// build texture groups
	for (i = 0; i < MAXSTUDIOSKINS; i++)
	{
		for (j = 0; j < MAXSTUDIOSKINS; j++)
		{
			g_skinref[i][j] = j;
		}
	}
	index = 0;
	for (i = 0; i < g_numtexturelayers[0]; i++)
	{
		for (j = 0; j < g_numtexturereps[0]; j++)
		{
			g_skinref[i][g_texturegroup[0][0][j]] = g_texturegroup[0][i][j];
		}
	}
	if (i != 0)
	{
		g_numskinfamilies = i;
	}
	else
	{
		g_numskinfamilies = 1;
		g_numskinref = g_numtextures;
	}

	// printf ("width: %i  height: %i\n",width, height);
	/*
	printf ("adjusted width: %i height: %i  top : %i  left: %i\n",
			pmesh->skinwidth, pmesh->skinheight, pmesh->skintop, pmesh->skinleft );
	*/
}

/*
=================
=================
*/


int LookupXNode( char *name )
{
	int i;
	for ( i = 1; i <= g_numxnodes; i++)
	{
		if (stricmp( name, g_xnodename[i] ) == 0)
		{
			return i;
		}
	}
	g_xnodename[i] = strdup( name );
	g_numxnodes = i;
	return i;
}


/*
=================
=================
*/

char	g_szFilename[1024];
FILE	*g_fpInput;
char	g_szLine[4096];
int		g_iLinecount;


void Build_Reference( s_source_t *psource)
{
	int		i, parent;
	Vector	angle;

	for (i = 0; i < psource->numbones; i++)
	{
		matrix3x4_t m;
		AngleMatrix( psource->rawanim[0][i].rot, m );
		m[0][3] = psource->rawanim[0][i].pos[0];
		m[1][3] = psource->rawanim[0][i].pos[1];
		m[2][3] = psource->rawanim[0][i].pos[2];

		parent = psource->localBone[i].parent;
		if (parent == -1) 
		{
			// scale the done pos.
			// calc rotational matrices
			MatrixCopy( m, psource->boneToPose[i] );
		}
		else 
		{
			// calc compound rotational matrices
			// FIXME : Hey, it's orthogical so inv(A) == transpose(A)
			ConcatTransforms( psource->boneToPose[parent], m, psource->boneToPose[i] );
		}
		// printf("%3d %f %f %f\n", i, psource->bonefixup[i].worldorg[0], psource->bonefixup[i].worldorg[1], psource->bonefixup[i].worldorg[2] );
		/*
		AngleMatrix( angle, m );
		printf("%8.4f %8.4f %8.4f\n", m[0][0], m[1][0], m[2][0] );
		printf("%8.4f %8.4f %8.4f\n", m[0][1], m[1][1], m[2][1] );
		printf("%8.4f %8.4f %8.4f\n", m[0][2], m[1][2], m[2][2] );
		*/
	}
}




int Grab_Nodes( s_node_t *pnodes )
{
	int index;
	char name[1024];
	int parent;
	int numbones = 0;

	for (index = 0; index < MAXSTUDIOSRCBONES; index++)
	{
		pnodes[index].parent = -1;
	}

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (sscanf( g_szLine, "%d \"%[^\"]\" %d", &index, name, &parent ) == 3)
		{
			// check for duplicated bones
			/*
			if (strlen(pnodes[index].name) != 0)
			{
				Error( "bone \"%s\" exists more than once\n", name );
			}
			*/
			
			strcpyn( pnodes[index].name, name );
			pnodes[index].parent = parent;
			if (index > numbones)
			{
				numbones = index;
			}
		}
		else 
		{
			return numbones + 1;
		}
	}
	Error( "Unexpected EOF at line %d\n", g_iLinecount );
	return 0;
}




void clip_rotations( RadianEuler& rot )
{
	int j;
	// clip everything to : -M_PI <= x < M_PI

	for (j = 0; j < 3; j++) {
		while (rot[j] >= M_PI) 
			rot[j] -= M_PI*2;
		while (rot[j] < -M_PI) 
			rot[j] += M_PI*2;
	}
}


void clip_rotations( Vector& rot )
{
	int j;
	// clip everything to : -180 <= x < 180

	for (j = 0; j < 3; j++) {
		while (rot[j] >= 180) 
			rot[j] -= 180*2;
		while (rot[j] < -180) 
			rot[j] += 180*2;
	}
}



/*
=================
Cmd_Eyeposition
=================
*/
void Cmd_Eyeposition (void)
{
// rotate points into frame of reference so g_model points down the positive x
// axis
	//	FIXME: these coords are bogus
	GetToken (false);
	eyeposition[1] = atof (token);

	GetToken (false);
	eyeposition[0] = -atof (token);

	GetToken (false);
	eyeposition[2] = atof (token);
}



/*
=================
Cmd_Eyeposition
=================
*/
void Cmd_Illumposition (void)
{
// rotate points into frame of reference so g_model points down the positive x
// axis
	//	FIXME: these coords are bogus
	GetToken (false);
	illumposition[1] = atof (token);

	GetToken (false);
	illumposition[0] = -atof (token);

	GetToken (false);
	illumposition[2] = atof (token);

	illumpositionset = true;
}


/*
=================
Cmd_Modelname
=================
*/
void Cmd_Modelname (void)
{
	GetToken (false);
	strcpyn (outname, token);
}

static void AddReadPathFromPlatform( const char *pPlatformName )
{
	char *tmp = ( char * )_alloca( strlen( qdir ) + 1 );
	char path[MAX_PATH];
	
	strcpy( tmp, qdir );
	char *tmp2 = Q_stristr( tmp, "models" );
	if( tmp2 )
	{
		*tmp2 = '\0';
		Q_strcpy( path, tmp );
		if( *pPlatformName )
		{
			Q_strcat( path, "models_" );
			Q_strcat( path, pPlatformName );
			Q_strcat( path, "/" );
		}
		else
		{
			Q_strcat( path, "models/" );
		}
		CmdLib_AddBasePath( path );
	}
}

void Cmd_Platform (void)
{
	GetToken (false);
	if( Q_strcasecmp( token, "dx6" ) == 0 )
	{
		strcpy( g_pPlatformName, token );
		AddReadPathFromPlatform( "dx6" );
		AddReadPathFromPlatform( "dx7" );
		AddReadPathFromPlatform( "" );
	}
	else if( Q_strcasecmp( token, "dx7" ) == 0 )
	{
		strcpy( g_pPlatformName, token );
		AddReadPathFromPlatform( "dx7" );
		AddReadPathFromPlatform( "" );
	}
	else
	{
		Warning( "unknown platform: \"%s\"\n", token );
	}
}

void Cmd_Autocenter()
{
	g_centerstaticprop = true;
}

/*
===============
===============
*/


void Option_Studio( s_model_t *pmodel )
{
	if (!GetToken (false)) return;
	strcpyn( pmodel->filename, token );

	// pmodel = (s_model_t *)kalloc( 1, sizeof( s_model_t ) );
	// g_bodypart[g_numbodyparts].pmodel[g_bodypart[g_numbodyparts].nummodels] = pmodel;


	flip_triangles = 1;

	pmodel->scale = g_currentscale = g_defaultscale;

	while (TokenAvailable())
	{
		GetToken(false);
		if (stricmp( "reverse", token ) == 0)
		{
			flip_triangles = 0;
		}
		else if (stricmp( "scale", token ) == 0)
		{
			GetToken(false);
			pmodel->scale = g_currentscale = atof( token );
		}
		else if (stricmp( "faces", token ) == 0)
		{
			GetToken( false );
			GetToken( false );
		}
		else if (stricmp( "bias", token ) == 0)
		{
			GetToken( false );
		}
		else if (stricmp("{", token ) == 0)
		{
			UnGetToken( );
			break;
		}
		else
		{
			Error("unknown command \"%s\"\n", token );
		}
	}

	pmodel->source = Load_Source( pmodel->filename, "" );
	// load source
}


int Option_Blank( )
{
	g_model[g_nummodels] = (s_model_t *)kalloc( 1, sizeof( s_model_t ) );

	g_source[g_numsources] = (s_source_t *)kalloc( 1, sizeof( s_source_t ) );
	g_model[g_nummodels]->source = g_source[g_numsources];
	g_numsources++;

	g_bodypart[g_numbodyparts].pmodel[g_bodypart[g_numbodyparts].nummodels] = g_model[g_nummodels];

	strcpyn( g_model[g_nummodels]->name, "blank" );

	g_bodypart[g_numbodyparts].nummodels++;
	g_nummodels++;
	return 0;
}


void Cmd_Bodygroup( )
{
	int is_started = 0;

	if (!GetToken(false)) return;

	if (g_numbodyparts == 0) {
		g_bodypart[g_numbodyparts].base = 1;
	}
	else {
		g_bodypart[g_numbodyparts].base = g_bodypart[g_numbodyparts-1].base * g_bodypart[g_numbodyparts-1].nummodels;
	}
	strcpyn( g_bodypart[g_numbodyparts].name, token );

	do
	{
		GetToken (true);
		if (endofscript)
			return;
		else if (token[0] == '{')
			is_started = 1;
		else if (token[0] == '}')
			break;
		else if (stricmp("studio", token ) == 0)
		{
			g_model[g_nummodels] = (s_model_t *)kalloc( 1, sizeof( s_model_t ) );
			g_bodypart[g_numbodyparts].pmodel[g_bodypart[g_numbodyparts].nummodels] = g_model[g_nummodels];
			g_bodypart[g_numbodyparts].nummodels++;
		
			Option_Studio( g_model[g_nummodels] );
			g_nummodels++;
		}
		else if (stricmp("blank", token ) == 0)
			Option_Blank( );
		else
		{
			Error("unknown bodygroup option: \"%s\"\n", token );
		}
	} while (1);

	g_numbodyparts++;
	return;
}


void Cmd_Body( )
{
	int is_started = 0;

	if (!GetToken(false)) return;

	if (g_numbodyparts == 0) {
		g_bodypart[g_numbodyparts].base = 1;
	}
	else {
		g_bodypart[g_numbodyparts].base = g_bodypart[g_numbodyparts-1].base * g_bodypart[g_numbodyparts-1].nummodels;
	}
	strcpyn(g_bodypart[g_numbodyparts].name, token );

	g_model[g_nummodels] = (s_model_t *)kalloc( 1, sizeof( s_model_t ) );
	g_bodypart[g_numbodyparts].pmodel[g_bodypart[g_numbodyparts].nummodels] = g_model[g_nummodels];
	g_bodypart[g_numbodyparts].nummodels = 1;

	Option_Studio( g_model[g_nummodels] );

	g_nummodels++;
	g_numbodyparts++;
}



/*
===============
===============
*/

void Grab_Animation( s_source_t *psource )
{
	Vector pos;
	RadianEuler rot;
	char cmd[1024];
	int index;
	int	t = -99999999;
	int size;

	psource->startframe = -1;

	size = psource->numbones * sizeof( s_bone_t );

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (sscanf( g_szLine, "%d %f %f %f %f %f %f", &index, &pos[0], &pos[1], &pos[2], &rot[0], &rot[1], &rot[2] ) == 7)
		{
			if (psource->startframe < 0)
			{
				Error( "Missing frame start(%d) : %s", g_iLinecount, g_szLine );
			}

			if (psource->rawanim[t] == NULL)
			{
				psource->rawanim[t] = (s_bone_t *)kalloc( 1, size );

				// duplicate previous frames keys
				if (t > 0 && psource->rawanim[t-1])
				{
					for (int j = 0; j < psource->numbones; j++)
					{
						VectorCopy( psource->rawanim[t-1][j].pos, psource->rawanim[t][j].pos );
						VectorCopy( psource->rawanim[t-1][j].rot, psource->rawanim[t][j].rot );
					}
				}
			}

			scale_vertex( pos );
			VectorCopy( pos, psource->rawanim[t][index].pos );
			VectorCopy( rot, psource->rawanim[t][index].rot );

			clip_rotations( rot ); // !!!
		}
		else if (sscanf( g_szLine, "%s %d", cmd, &index ))
		{
			if (strcmp( cmd, "time" ) == 0) 
			{
				t = index;
				if (psource->startframe == -1)
				{
					psource->startframe = t;
				}
				if (t < psource->startframe)
				{
					Error( "Frame Error(%d) : %s", g_iLinecount, g_szLine );
				}
				if (t > psource->endframe)
				{
					psource->endframe = t;
				}
				t -= psource->startframe;
			}
			else if (strcmp( cmd, "end") == 0) 
			{
				psource->numframes = psource->endframe - psource->startframe + 1;
				Build_Reference( psource );
				return;
			}
			else
			{
				Error( "Error(%d) : %s", g_iLinecount, g_szLine );
			}
		}
		else
		{
			Error( "Error(%d) : %s", g_iLinecount, g_szLine );
		}
	}
	Error( "unexpected EOF: %s\n", psource->filename );
}





int Option_Activity( s_sequence_t *psequence )
{
	qboolean found;

	found = false;

	GetToken(false);
	strcpy( psequence->activityname, token );

	GetToken(false);
	psequence->actweight = atoi(token);

	return 0;
}



/*
===============
===============
*/


int Option_Event ( s_sequence_t *psequence )
{
	int event;

	if (psequence->numevents + 1 >= MAXSTUDIOEVENTS)
	{
		printf("too many events\n");
		exit(0);
	}

	GetToken (false);
	event = atoi( token );
	psequence->event[psequence->numevents].event = event;

	GetToken( false );
	psequence->event[psequence->numevents].frame = atoi( token );

	psequence->numevents++;

	// option token
	if (TokenAvailable())
	{
		GetToken( false );
		if (token[0] == '}') // opps, hit the end
			return 1;
		// found an option
		strcpyn( psequence->event[psequence->numevents-1].options, token );
	}

	return 0;
}



void Option_IKRule( s_ikrule_t *pRule )
{
	// chain
	GetToken( false );

	int i;
	for ( i = 0; i < g_numikchains; i++)
	{
		if (stricmp( token, g_ikchain[i].name ) == 0)
		{
			break;
		}
	}
	if (i >= g_numikchains)
	{
		Error( "unknown chain \"%s\" in ikrule\n", token );
	}
	pRule->chain = i;

	// type
	GetToken( false );
	if (stricmp( token, "touch" ) == 0)
	{
		pRule->type = IK_SELF;

		// bone
		GetToken( false );
		strcpyn( pRule->bonename, token );
	}
	else if (stricmp( token, "footstep" ) == 0)
	{
		pRule->type = IK_GROUND;

		// slot
		GetToken( false );
		pRule->slot = atoi( token );

		pRule->height = g_ikchain[pRule->chain].height;
		pRule->floor = g_ikchain[pRule->chain].floor;
		pRule->radius = g_ikchain[pRule->chain].radius;
	}
	else if (stricmp( token, "world" ) == 0)
	{
		pRule->type = IK_WORLD;

		// slot
		GetToken( false );
		pRule->slot = atoi( token );
	}
	else if (stricmp( token, "release" ) == 0)
	{
		pRule->type = IK_RELEASE;
	}

	pRule->contact = -1;

	while (TokenAvailable())
	{
		GetToken( false );
		if (stricmp( token, "height" ) == 0)
		{
			GetToken( false );
			pRule->height = atof( token );
		}
		else if (stricmp( token, "range" ) == 0)
		{
			// ramp
			GetToken( false );
			if (token[0] == '.')
				pRule->start = -1;
			else
				pRule->start = atoi( token );

			GetToken( false );
			if (token[0] == '.')
				pRule->peak = -1;
			else
				pRule->peak = atoi( token );
	
			GetToken( false );
			if (token[0] == '.')
				pRule->tail = -1;
			else
				pRule->tail = atoi( token );

			GetToken( false );
			if (token[0] == '.')
				pRule->end = -1;
			else
				pRule->end = atoi( token );
		}
		else if (stricmp( token, "floor" ) == 0)
		{
			GetToken( false );
			pRule->floor = atof( token );
		}
		else if (stricmp( token, "pad" ) == 0)
		{
			GetToken( false );
			pRule->radius = atof( token ) / 2.0f;
		}
		else if (stricmp( token, "contact" ) == 0)
		{
			GetToken( false );
			pRule->contact = atoi( token );
		}
		else
		{
			UnGetToken();
			return;
		}
	}
}


/*
=================
Cmd_Origin
=================
*/
void Cmd_Origin (void)
{
	GetToken (false);
	g_defaultadjust.x = atof (token);

	GetToken (false);
	g_defaultadjust.y = atof (token);

	GetToken (false);
	g_defaultadjust.z = atof (token);

	if (TokenAvailable()) {
		GetToken (false);
		g_defaultrotation.z = DEG2RAD( atof( token ) + 90);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Set the default root rotation so that the Y axis is up instead of the Z axis (for Maya)
//-----------------------------------------------------------------------------
void Cmd_UpAxis( void )
{
	// We want to create a rotation that rotates from the art space
	// (specified by the up direction) to a z up space
	// Note: x, -x, -y are untested
	GetToken (false);
	if (!stricmp( token, "x" ))
	{
		// rotate 90 degrees around y to move x into z
		g_defaultrotation.x = 0.0f;
		g_defaultrotation.y = M_PI / 2.0f;
	}
	if (!stricmp( token, "-x" ))
	{
		// untested
		g_defaultrotation.x = 0.0f;
		g_defaultrotation.y = -M_PI / 2.0f;
	}
	else if (!stricmp( token, "y" ))
	{
		// rotate 90 degrees around x to move y into z
		g_defaultrotation.x = M_PI / 2.0f;
		g_defaultrotation.y = 0.0f;
	}
	else if (!stricmp( token, "-y" ))
	{
		// untested
		g_defaultrotation.x = -M_PI / 2.0f;
		g_defaultrotation.y = 0.0f;
	}
	else if (!stricmp( token, "z" ))
	{
		// there's still a built in 90 degree Z rotation :(
		g_defaultrotation.x = 0.0f;
		g_defaultrotation.y = 0.0f;
	}
	else if (!stricmp( token, "-z" ))
	{
		// there's still a built in 90 degree Z rotation :(
		g_defaultrotation.x = 0.0f;
		g_defaultrotation.y = 0.0f;
	}
	else
	{
		Error( "unknown $upaxis option: \"%s\"\n", token );
	}
}


/*
=================
=================
*/
void Cmd_ScaleUp (void)
{

	GetToken (false);
	g_defaultscale = atof (token);

	g_currentscale = g_defaultscale;
}


/*
=================
=================
*/

int Cmd_SequenceGroup( )
{
	GetToken (false);
	strcpyn( g_sequencegroup[g_numseqgroups].label, token );
	g_numseqgroups++;

	return 0;
}


int Cmd_SequenceGroupSize( )
{
	GetToken (false);
	maxseqgroupsize = 1024 * atoi( token );
	return 0;
}

#if 0 // Old stuff. We aren't bound to an activity map anymore.
int lookupActivity( char *szActivity )
{
	int i;

	for (i = 0; activity_map[i].name; i++)
	{
		if (stricmp( szActivity, activity_map[i].name ) == 0)
			return activity_map[i].type;
	}
	// match ACT_#
	if (strnicmp( szActivity, "ACT_", 4 ) == 0)
	{
		return atoi( &szActivity[4] );
	}
	return 0;
}
#endif // Old stuff.


static void FlipFacing( s_source_t *pSrc )
{
	unsigned short tmp;

	int i, j;
	for( i = 0; i < pSrc->nummeshes; i++ )
	{
		s_mesh_t *pMesh = &pSrc->mesh[i];
		for( j = 0; j < pMesh->numfaces; j++ )
		{
			s_face_t &f = pSrc->face[pMesh->faceoffset + j];
			tmp = f.b;  f.b  = f.c;  f.c  = tmp;
		}
	}
}


//-----------------------------------------------------------------------------
// Checks to see if the model source was already loaded
//-----------------------------------------------------------------------------
static s_source_t *FindCachedSource( char const* name, char const* xext )
{
	int i;

	if( xext[0] )
	{
		// we know what extension is necessary. . look for it.
		sprintf (g_szFilename, "%s%s.%s", cddir[numdirs], name, xext );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
	}
	else
	{
		// we don't know what extension to use, so look for all of 'em.
		sprintf (g_szFilename, "%s%s.vrm", cddir[numdirs], name );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
		sprintf (g_szFilename, "%s%s.smd", cddir[numdirs], name );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
		/*
		sprintf (g_szFilename, "%s%s.vta", cddir[numdirs], name );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
		*/
	}

	// Not found
	return 0;
}


//-----------------------------------------------------------------------------
// Loads an animation/model source
//-----------------------------------------------------------------------------

s_source_t *Load_Source( char const *name, const char *ext, bool reverse )
{
	assert(name);
	char* pTempName = (char*)_alloca(strlen(name) + 1 );
	char xext[32];
	int result = false;

	strcpy( pTempName, name );
	ExtractFileExtension( pTempName, xext, sizeof( xext ) );

	if (xext[0] == '\0')
	{
		strcpyn( xext, ext );
	}
	else
	{
		StripExtension( pTempName );
	}

	s_source_t* pSource = FindCachedSource( pTempName, xext );
	if (pSource)
		return pSource;

	g_source[g_numsources] = (s_source_t *)kalloc( 1, sizeof( s_source_t ) );
	strcpyn( g_source[g_numsources]->filename, g_szFilename );

	if ( xext[0] == '\0' || stricmp( xext, "vrm" ) == 0)
	{
		sprintf (g_szFilename, "%s%s.vrm", cddir[numdirs], pTempName );
		strcpyn( g_source[g_numsources]->filename, g_szFilename );
		result = Load_VRM( g_source[g_numsources] );
	}
	if ( ( !result && xext[0] == '\0' ) || stricmp( xext, "smd" ) == 0)
	{
		sprintf (g_szFilename, "%s%s.smd", cddir[numdirs], pTempName );
		strcpyn( g_source[g_numsources]->filename, g_szFilename );
		result = Load_SMD( g_source[g_numsources] );
	}
	if (( !result && xext[0] == '\0' ) || stricmp( xext, "vta" ) == 0)
	{
		sprintf (g_szFilename, "%s%s.vta", cddir[numdirs], pTempName );
		strcpyn( g_source[g_numsources]->filename, g_szFilename );
		result = Load_VTA( g_source[g_numsources] );
	}

	if (!result)
	{
		Error( "could not load file '%s'\n", g_source[g_numsources]->filename );
	}

	g_numsources++;
	if( reverse )
	{
		FlipFacing( g_source[g_numsources-1] );
	}
	return g_source[g_numsources-1];
}


s_animation_t *LookupAnimation( char *name )
{
	int i;
	for ( i = 0; i < g_numani; i++)
	{
		if (stricmp( g_panimation[i]->name, token ) == 0)
		{
			return g_panimation[i];
		}
	}

	for (i = 0; i < g_numseq; i++)
	{
		if (stricmp( g_sequence[i].name, token ) == 0)
		{
			return g_sequence[i].panim[0][0];
		}
	}
	return NULL;
}


int ParseCmdlistToken( int &numcmds, s_animcmd_t *cmds )
{
	if (numcmds >= MAXSTUDIOCMDS)
	{
		return false;
	}
	s_animcmd_t *pcmd = &cmds[numcmds];
	if (stricmp("fixuploop", token ) == 0)
	{
		pcmd->cmd = CMD_FIXUP;

		GetToken( false );
		pcmd->u.fixuploop.start = atoi( token );
		GetToken( false );
		pcmd->u.fixuploop.end = atoi( token );
	}
	else if (strnicmp("weightlist", token, 6 ) == 0)
	{
		GetToken( false );

		int i;
		for ( i = 1; i < g_numweightlist; i++)
		{
			if (stricmp( g_weightlist[i].name, token ) == 0)
			{
				break;
			}
		}
		if (i == g_numweightlist)
		{
			Error( "unknown weightlist '%s\'\n", token );
		}
		pcmd->cmd = CMD_WEIGHTS;
		pcmd->u.weightlist.index = i;
	}
	else if (stricmp("subtract", token ) == 0)
	{
		pcmd->cmd = CMD_SUBTRACT;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			Error( "unknown delta animation '%s\'\n", token );
		}

		pcmd->u.subtract.ref = extanim;

		GetToken( false );
		pcmd->u.subtract.frame = atoi( token );

		pcmd->u.subtract.flags |= STUDIO_POST;
	}
	else if (stricmp("presubtract", token ) == 0) // FIXME: rename this to something better
	{
		pcmd->cmd = CMD_SUBTRACT;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			Error( "unknown delta animation '%s\'\n", token );
		}

		pcmd->u.subtract.ref = extanim;

		GetToken( false );
		pcmd->u.subtract.frame = atoi( token );
	}
	else if (stricmp( "alignto", token ) == 0)
	{
		pcmd->cmd = CMD_AO;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			Error( "unknown alignto animation '%s\'\n", token );
		}

		pcmd->u.ao.ref = extanim;
		pcmd->u.ao.motiontype = STUDIO_X | STUDIO_Y;
		pcmd->u.ao.srcframe = 0;
		pcmd->u.ao.destframe = 0;
	}
	else if (stricmp( "align", token ) == 0)
	{
		pcmd->cmd = CMD_AO;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			Error( "unknown alignto animation '%s\'\n", token );
		}

		pcmd->u.ao.ref = extanim;

		// motion type to match
		pcmd->u.ao.motiontype = 0;
		GetToken( false );
		int ctrl;
		while ((ctrl = lookupControl( token )) != -1)
		{
			pcmd->u.ao.motiontype |= ctrl;
			GetToken( false );
		}
		if (pcmd->u.ao.motiontype == 0)
		{
			Error( "missing controls on align\n" );
		}

		// frame of reference animation to match
		pcmd->u.ao.srcframe = atoi( token );

		// against what frame of the current animation
		GetToken( false );
		pcmd->u.ao.destframe = atoi( token );
	}
	else if (stricmp( "match", token ) == 0)
	{
		pcmd->cmd = CMD_MATCH;

		GetToken( false );

		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			Error( "unknown alignto animation '%s\'\n", token );
		}

		pcmd->u.match.ref = extanim;
	}
	else if (stricmp( "rotateto", token ) == 0)
	{
		pcmd->cmd = CMD_ANGLE;

		GetToken( false );
		pcmd->u.angle.angle = atof( token );
	}
	else if (stricmp( "ikrule", token ) == 0)
	{
		pcmd->cmd = CMD_IKRULE;

		pcmd->u.ikrule.pRule = &g_ikrule[g_numikrules++];

		Option_IKRule( pcmd->u.ikrule.pRule );

		// Option_IKRule( &panim->ikrule[panim->numikrules++] );
	}
	else if (stricmp( "ikfixup", token ) == 0)
	{
		pcmd->cmd = CMD_IKFIXUP;

		pcmd->u.ikfixup.pRule = &g_ikrule[g_numikrules++];

		Option_IKRule( pcmd->u.ikrule.pRule );

		// Option_IKRule( &panim->ikrule[panim->numikrules++] );
	}
	else if (stricmp( "walkframe", token ) == 0)
	{
		pcmd->cmd = CMD_MOTION;

		// frame
		GetToken( false );
		pcmd->u.motion.iEndFrame = atoi( token );

		// motion type to match
		pcmd->u.motion.motiontype = 0;
		while (TokenAvailable())
		{
			GetToken( false );
			int ctrl = lookupControl( token );
			if (ctrl != -1)
			{
				pcmd->u.motion.motiontype |= ctrl;
			}
			else
			{
				UnGetToken();
				break;
			}
		}

		/*
		GetToken( false ); // X
		pcmd->u.motion.x = atof( token );

		GetToken( false ); // Y
		pcmd->u.motion.y = atof( token );

		GetToken( false ); // A
		pcmd->u.motion.zr = atof( token );
		*/
	}
	else if (stricmp( "walkalignto", token ) == 0)
	{
		pcmd->cmd = CMD_REFMOTION;

		GetToken( false );
		pcmd->u.motion.iEndFrame = atoi( token );

		pcmd->u.motion.iSrcFrame = pcmd->u.motion.iEndFrame;

		GetToken( false ); // reference animation
		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			Error( "unknown alignto animation '%s\'\n", token );
		}
		pcmd->u.motion.pRefAnim = extanim;

		pcmd->u.motion.iRefFrame = 0;

		// motion type to match
		pcmd->u.motion.motiontype = 0;
		while (TokenAvailable())
		{
			GetToken( false );
			int ctrl = lookupControl( token );
			if (ctrl != -1)
			{
				pcmd->u.motion.motiontype |= ctrl;
			}
			else
			{
				UnGetToken();
				break;
			}
		}


		/*
		GetToken( false ); // X
		pcmd->u.motion.x = atof( token );

		GetToken( false ); // Y
		pcmd->u.motion.y = atof( token );

		GetToken( false ); // A
		pcmd->u.motion.zr = atof( token );
		*/
	}
	else if (stricmp( "walkalign", token ) == 0)
	{
		pcmd->cmd = CMD_REFMOTION;

		// end frame to apply motion over
		GetToken( false );
		pcmd->u.motion.iEndFrame = atoi( token );

		// reference animation
		GetToken( false );
		s_animation_t *extanim = LookupAnimation( token );
		if (extanim == NULL)
		{
			Error( "unknown alignto animation '%s\'\n", token );
		}
		pcmd->u.motion.pRefAnim = extanim;

		// motion type to match
		pcmd->u.motion.motiontype = 0;
		while (TokenAvailable())
		{
			GetToken( false );
			int ctrl = lookupControl( token );
			if (ctrl != -1)
			{
				pcmd->u.motion.motiontype |= ctrl;
			}
			else
			{
				UnGetToken();
				break;
			}
		}
		if (pcmd->u.motion.motiontype == 0)
		{
			Error( "missing controls on walkalign\n" );
		}

		// frame of reference animation to match
		pcmd->u.motion.iRefFrame = atoi( token );

		// against what frame of the current animation
		GetToken( false );
		pcmd->u.motion.iSrcFrame = atoi( token );
	}
	else if (stricmp("derivative", token ) == 0)
	{
		pcmd->cmd = CMD_DERIVATIVE;

		// get scale
		GetToken( false );
		pcmd->u.derivative.scale = atof( token );
	}
	else if (stricmp("noanimation", token ) == 0)
	{
		pcmd->cmd = CMD_NOANIMATION;
	}
	else if (stricmp("lineardelta", token ) == 0)
	{
		pcmd->cmd = CMD_LINEARDELTA;
		pcmd->u.linear.flags |= STUDIO_POST;
	}
	else if (stricmp("splinedelta", token ) == 0)
	{
		pcmd->cmd = CMD_LINEARDELTA;
		pcmd->u.linear.flags |= STUDIO_POST;
		pcmd->u.linear.flags |= STUDIO_SPLINE;
	}
	else if (stricmp("compress", token ) == 0)
	{
		pcmd->cmd = CMD_COMPRESS;

		// get frames to skip
		GetToken( false );
		pcmd->u.compress.frames = atoi( token );
	}
	else if (stricmp("numframes", token ) == 0)
	{
		pcmd->cmd = CMD_NUMFRAMES;

		// get frames to force
		GetToken( false );
		pcmd->u.compress.frames = atoi( token );
	}
	else
	{
		return false;
	}
	numcmds++;
	return true;
}


int ParseAnimationToken( s_animation_t *panim )
{
	if (stricmp("fps", token ) == 0)
	{
		GetToken( false );
		panim->fps = atof( token );
		if ( panim->fps <= 0.0f )
		{
			Error( "ParseAnimationToken:  fps (%f from '%s') <= 0.0\n", panim->fps, token );
		}
	}
	else if (stricmp("origin", token ) == 0)
	{
		GetToken (false);
		panim->adjust.x = atof (token);

		GetToken (false);
		panim->adjust.y = atof (token);

		GetToken (false);
		panim->adjust.z = atof (token);
	}
	else if (stricmp("rotate", token ) == 0)
	{
		GetToken( false );
		// FIXME: broken for Maya
		panim->rotation.z = DEG2RAD( atof( token ) + 90 );
	}
	else if (stricmp("angles", token ) == 0)
	{
		GetToken( false );
		panim->rotation.x = DEG2RAD( atof( token ) );
		GetToken( false );
		panim->rotation.y = DEG2RAD( atof( token ) );
		GetToken( false );
		panim->rotation.z = DEG2RAD( atof( token ) + 90.0);
	}
	else if (stricmp("scale", token ) == 0)
	{
		GetToken( false );
		panim->scale = atof( token );
	}
	else if (strnicmp("loop", token, 4 ) == 0)
	{
		panim->flags |= STUDIO_LOOPING;
	}
	else if (strnicmp("startloop", token, 5 ) == 0)
	{
		GetToken( false );
		panim->looprestart = atoi( token );
		panim->flags |= STUDIO_LOOPING;
	}
	else if (stricmp("fudgeloop", token ) == 0)
	{
		panim->fudgeloop = true;
		panim->flags |= STUDIO_LOOPING;
	}
	else if (strnicmp("snap", token, 4 ) == 0)
	{
		panim->flags |= STUDIO_SNAP;
	}
	else if (strnicmp("frame", token, 5 ) == 0)
	{
		GetToken( false );
		panim->startframe = atoi( token );
		GetToken( false );
		panim->endframe = atoi( token );

		if (panim->startframe < panim->source->startframe)
			panim->startframe = panim->source->startframe;

		if (panim->endframe > panim->source->endframe)
			panim->endframe = panim->source->endframe;

		if (panim->endframe < panim->startframe)
			Error( "end frame before start frame in %s", panim->name );

		panim->numframes = panim->endframe - panim->startframe + 1;
	}
	else if (stricmp("post", token) == 0)
	{
		panim->flags |= STUDIO_POST;
	}
	else if (ParseCmdlistToken( panim->numcmds, panim->cmds ))
	{

	}
	else if (stricmp("cmdlist", token) == 0)
	{
		GetToken( false ); // A

		int i;
		for ( i = 0; i < g_numcmdlists; i++)
		{
			if (stricmp( g_cmdlist[i].name, token) == 0)
			{
				break;
			}
		}
		if (i == g_numcmdlists)
			Error( "unknown cmdlist %s\n", token );

		for (int j = 0; j < g_cmdlist[i].numcmds; j++)
		{
			if (panim->numcmds >= MAXSTUDIOCMDS)
			{
				Error("Too many cmds in %s\n", panim->name );
			}
			panim->cmds[panim->numcmds++] = g_cmdlist[i].cmds[j];
		}
	}
	else if (lookupControl( token ) != -1)
	{
		panim->motiontype |= lookupControl( token );
	}
	else
	{
		return false;
	}
	return true;
}



int Cmd_Cmdlist( )
{
	int depth = 0;

	// name
	GetToken(false);
	strcpyn( g_cmdlist[g_numcmdlists].name, token );

	while (1)
	{
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				printf("missing }\n" );
				exit(1);
			}
			return 1;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		else if (ParseCmdlistToken( g_cmdlist[g_numcmdlists].numcmds, g_cmdlist[g_numcmdlists].cmds ))
		{

		}
		else
		{
			Error( "unknown command: %s\n", token );
		}

		if (depth < 0)
		{
			printf("missing {\n");
			exit(1);
		}
	};

	g_numcmdlists++;
	return 0;
}


int Cmd_Animation( )
{
	int depth = 0;

	// allocate animation entry
	g_panimation[g_numani] = (s_animation_t *)kalloc( 1, sizeof( s_animation_t ) );
	g_panimation[g_numani]->index = g_numani;
	s_animation_t *panim = g_panimation[g_numani];

	// name
	GetToken(false);
	strcpyn( panim->name, token );

	if (LookupAnimation( token ) != NULL)
	{
		Error( "Duplicate animation name \"%s\"\n", token );		
	}
	g_numani++;

	// filename
	GetToken(false);
	strcpyn( panim->filename, token );

	panim->source = Load_Source( panim->filename, "smd" );
	panim->startframe = panim->source->startframe;
	panim->endframe = panim->source->endframe;

	VectorCopy( g_defaultadjust, panim->adjust );
	panim->rotation = g_defaultrotation;
	panim->scale = 1.0f;
	panim->fps = 30.0;
	panim->weightlist = 0;

	while (1)
	{
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				printf("missing }\n" );
				exit(1);
			}
			return 1;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		else if (ParseAnimationToken( panim ))
		{

		}
		else
		{
			Error( "Unknown animation option\'%s\'\n", token );
		}

		if (depth < 0)
		{
			printf("missing {\n");
			exit(1);
		}
	};

	panim->numframes = panim->endframe - panim->startframe + 1;

	return 0;
}



s_animation_t *Cmd_ImpliedAnimation( s_sequence_t *psequence, char *filename )
{
	// allocate animation entry
	g_panimation[g_numani] = (s_animation_t *)kalloc( 1, sizeof( s_animation_t ) );
	g_panimation[g_numani]->index = g_numani;
	s_animation_t *panim = g_panimation[g_numani];
	g_numani++;

	panim->isimplied = true;

	panim->startframe = 0;
	panim->endframe = MAXSTUDIOANIMATIONS - 1;

	strcpy( panim->name, "@" );
	strcat( panim->name, psequence->name );
	strcpyn( panim->filename, filename );

	VectorCopy( g_defaultadjust, panim->adjust );
	panim->scale = 1.0f;
	panim->rotation = g_defaultrotation;
	panim->fps = 30;
	panim->weightlist = 0;

	panim->source = Load_Source( panim->filename, "smd" );

	if (panim->startframe < panim->source->startframe)
		panim->startframe = panim->source->startframe;
	
	if (panim->endframe > panim->source->endframe)
		panim->endframe = panim->source->endframe;

	if (panim->endframe < panim->startframe)
		Error( "end frame before start frame in %s", panim->name );

	panim->numframes = panim->endframe - panim->startframe + 1;

	return panim;
}


void CopyAnimationSettings( s_animation_t *pdest, s_animation_t *psrc )
{
	pdest->fps = psrc->fps;

	VectorCopy( psrc->adjust, pdest->adjust );
	pdest->scale = psrc->scale;
	pdest->rotation = psrc->rotation;

	pdest->motiontype = psrc->motiontype;

	//Adrian - Hey! Revisit me later.
	/*if (pdest->startframe < psrc->startframe)
		pdest->startframe = psrc->startframe;
	
	if (pdest->endframe > psrc->endframe)
		pdest->endframe = psrc->endframe;
	
	if (pdest->endframe < pdest->startframe)
		Error( "fixedup end frame before start frame in %s", pdest->name );

	pdest->numframes = pdest->endframe - pdest->startframe + 1;*/

	for (int i = 0; i < psrc->numcmds; i++)
	{
		if (pdest->numcmds >= MAXSTUDIOCMDS)
		{
			Error("Too many cmds in %s\n", pdest->name );
		}
		pdest->cmds[pdest->numcmds++] = psrc->cmds[i];
	}
}


int Cmd_Sequence( )
{
	int depth = 0;
	s_animation_t *animations[64];
	int i, j, k, n;
	int numblends = 0;

	if (!GetToken(false)) return 0;

	// allocate sequence
	if (LookupAnimation( token ) != NULL)
	{
		Error( "Duplicate sequence name \"%s\"\n", token );		
	}
	s_sequence_t *pseq = &g_sequence[g_numseq];
	g_numseq++;

	// initialize sequence
	strcpyn( pseq->name, token );

	pseq->actweight = 0;
	pseq->activityname[0] = '\0';
	pseq->activity = -1; // -1 is the default for 'no activity'

	pseq->seqgroup = g_numseqgroups - 1;
	pseq->paramindex[0] = -1;
	pseq->paramindex[1] = -1;

	pseq->groupsize[0] = 0;
	pseq->groupsize[1] = 0;

	pseq->fadeintime = 0.2;
	pseq->fadeouttime = 0.2;

	while (1)
	{
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				printf("missing }\n" );
				exit(1);
			}
			return 1;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		/*
		else if (stricmp("deform", token ) == 0)
		{
			Option_Deform( pseq );
		}
		*/

		else if (stricmp("event", token ) == 0)
		{
			depth -= Option_Event( pseq );
		}
		else if (stricmp("activity", token ) == 0)
		{
			Option_Activity( pseq );
		}
		else if (strnicmp( token, "ACT_", 4 ) == 0)
		{
			UnGetToken( );
			Option_Activity( pseq );
		}

		else if (stricmp("snap", token ) == 0)
		{
			pseq->flags |= STUDIO_SNAP;
		}

		else if (stricmp("blendwidth", token ) == 0)
		{
			GetToken( false );
			pseq->groupsize[0] = atoi( token );
		}

		else if (stricmp("blend", token ) == 0)
		{
			i = 0;
			if (pseq->paramindex[0] != -1)
			{
				i = 1;
			}

			GetToken( false );
			j = LookupPoseParameter( token );
			pseq->paramindex[i] = j;
			pseq->paramattachment[i] = -1;
			GetToken( false );
			pseq->paramstart[i] = atof( token );
			GetToken( false );
			pseq->paramend[i] = atof( token );

			g_pose[j].min  = min( g_pose[j].min, pseq->paramstart[i] );
			g_pose[j].min  = min( g_pose[j].min, pseq->paramend[i] );
			g_pose[j].max  = max( g_pose[j].max, pseq->paramstart[i] );
			g_pose[j].max  = max( g_pose[j].max, pseq->paramend[i] );
		}
		else if (stricmp("calcblend", token ) == 0)
		{
			i = 0;
			if (pseq->paramindex[0] != -1)
			{
				i = 1;
			}

			GetToken( false );
			j = LookupPoseParameter( token );
			pseq->paramindex[i] = j;

			GetToken( false );
			pseq->paramattachment[i] = LookupAttachment( token );
			if (pseq->paramattachment[i] == -1)
			{
				Error( "Unknown calcblend attachment \"%s\"\n", token );
			}

			GetToken( false );
			pseq->paramcontrol[i] = lookupControl( token );
		}
		else if (stricmp("node", token ) == 0)
		{
			GetToken( false );
			pseq->entrynode = pseq->exitnode = LookupXNode( token );
		}
		else if (stricmp("transition", token ) == 0)
		{
			GetToken( false );
			pseq->entrynode = LookupXNode( token );
			GetToken( false );
			pseq->exitnode = LookupXNode( token );
		}
		else if (stricmp("rtransition", token ) == 0)
		{
			GetToken( false );
			pseq->entrynode = LookupXNode( token );
			GetToken( false );
			pseq->exitnode = LookupXNode( token );
			pseq->nodeflags |= 1;
		}
		else if (stricmp("exitphase", token ) == 0)
		{
			GetToken( false );
			pseq->exitphase = atof( token );
		}
		else if (stricmp("delta", token) == 0)
		{
			pseq->flags |= STUDIO_DELTA;
			pseq->flags |= STUDIO_POST;
		}
		else if (stricmp("fadedelta", token) == 0)
		{
			pseq->flags |= STUDIO_DELTA;
			pseq->flags |= STUDIO_POST;
			pseq->flags |= STUDIO_WEIGHT;
		}
		else if (stricmp("post", token) == 0) // remove
		{
			pseq->flags |= STUDIO_POST; 
		}
		else if (stricmp("predelta", token) == 0)
		{
			pseq->flags |= STUDIO_DELTA;
		}
		else if (stricmp("autoplay", token) == 0)
		{
			pseq->flags |= STUDIO_AUTOPLAY;
		}
		else if (stricmp( "fadein", token ) == 0)
		{
			GetToken( false );
			pseq->fadeintime = atof( token );
		}
		else if (stricmp( "fadeout", token ) == 0)
		{
			GetToken( false );
			pseq->fadeouttime = atof( token );
		}
		else if (stricmp( "addlayer", token ) == 0)
		{
			GetToken( false );
			strcpyn( pseq->autolayer[pseq->numautolayers].name, token );
			pseq->numautolayers++;
		}
		else if (stricmp( "iklock", token ) == 0)
		{
			GetToken(false);
			strcpyn( pseq->iklock[pseq->numiklocks].name, token );

			GetToken(false);
			pseq->iklock[pseq->numiklocks].flPosWeight = atof( token );

			GetToken(false);
			pseq->iklock[pseq->numiklocks].flLocalQWeight = atof( token );
		
			pseq->numiklocks++;
		}
		else if (stricmp( "keyvalues", token ) == 0)
		{
			Option_KeyValues( &pseq->KeyValue );
		}
		else if (stricmp( "blendlayer", token ) == 0)
		{
			pseq->autolayer[pseq->numautolayers].flags |= STUDIO_WEIGHT;

			GetToken( false );
			strcpyn( pseq->autolayer[pseq->numautolayers].name, token );

			GetToken( false );
			pseq->autolayer[pseq->numautolayers].start = atoi( token );

			GetToken( false );
			pseq->autolayer[pseq->numautolayers].peak = atoi( token );

			GetToken( false );
			pseq->autolayer[pseq->numautolayers].tail = atoi( token );

			GetToken( false );
			pseq->autolayer[pseq->numautolayers].end = atoi( token );

			while (TokenAvailable( ))
			{
				GetToken( false );
				if (stricmp( "fullweight", token ) == 0)
				{
					pseq->autolayer[pseq->numautolayers].flags &= ~STUDIO_WEIGHT;
				}
				else if (stricmp( "spline", token ) == 0)
				{
					pseq->autolayer[pseq->numautolayers].flags |= STUDIO_SPLINE;
				}
				else
				{
					UnGetToken();
					break;
				}
			}

			pseq->numautolayers++;
		}
		else if (numblends && ParseAnimationToken( animations[0] ))
		{

		}
		else
		{
			// assume it's an animation reference
			// first look up an existing animation
			for (n = 0; n < g_numani; n++)
			{
				if (stricmp( token, g_panimation[n]->name ) == 0)
				{
					animations[numblends++] = g_panimation[n];
					break;
				}
			}

			if (n >= g_numani)
			{
				// assume it's an implied animation
				animations[numblends++] = Cmd_ImpliedAnimation( pseq, token );
			}
			// hack to allow animation commands to refer to same sequence
			if (numblends == 1)
			{
				pseq->panim[0][0] = animations[0];
			}

		}

		if (depth < 0)
		{
			printf("missing {\n");
			exit(1);
		}
	};

	if (numblends == 0)
	{
		printf("no animations found\n");
		exit(1);
	}

	if (pseq->groupsize[0] == 0)
	{
		if (numblends < 4)
		{
			pseq->groupsize[0] = numblends;
			pseq->groupsize[1] = 1;
		}
		else
		{
			i = sqrt( numblends );
			if (i * i == numblends)
			{
				pseq->groupsize[0] = i;
				pseq->groupsize[1] = i;
			}
			else
			{
				Error( "non-square (%d) number of blends without \"blendwidth\" set\n", numblends );
			}
		}
	}
	else
	{
		pseq->groupsize[1] = numblends / pseq->groupsize[0];

		if (pseq->groupsize[0] * pseq->groupsize[1] != numblends)
		{
			Error( "missing animation blends. Expected %d, found %d\n", 
				pseq->groupsize[0] * pseq->groupsize[1], numblends );
		}
	}

	for (i = 0; i < numblends; i++)
	{
		j = i % pseq->groupsize[0];
		k = i / pseq->groupsize[0];
		
		pseq->panim[j][k] = animations[i];

		if (i > 0 && animations[i]->isimplied)
		{
			CopyAnimationSettings( animations[i], animations[0] );
		}
		pseq->flags |= animations[i]->flags;
	}

	pseq->numblends = numblends;

	return 0;
}



int Cmd_Weightlist( )
{
	int depth = 0;
	int i;

	if (!GetToken(false)) 
		return 0;

	strcpyn( g_weightlist[g_numweightlist].name, token );

	g_weightlist[g_numweightlist].numbones = 0;

	while (1)
	{
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				printf("missing }\n" );
				exit(1);
			}
			return 1;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		else
		{
			i = g_weightlist[g_numweightlist].numbones++;
			strcpyn( g_weightlist[g_numweightlist].bonename[i], token );
			GetToken( false );
			g_weightlist[g_numweightlist].boneweight[i] = atof( token );
		}

		if (depth < 0)
		{
			printf("missing {\n");
			exit(1);
		}
	};

	g_numweightlist++;

	return 0;
}




void Option_Eyeball( s_model_t *pmodel )
{
	Vector	tmp;
	int i, j;
	int mesh_material;
	char szMeshMaterial[256];

	s_eyeball_t *eyeball = &(pmodel->eyeball[pmodel->numeyeballs++]);

	// name
	GetToken (false);
	strcpyn( eyeball->name, token );

	// bone name
	GetToken (false);
	for (i = 0; i < pmodel->source->numbones; i++)
	{
		if (stricmp( pmodel->source->localBone[i].name, token ) == 0)
		{
			eyeball->bone = i;
			break;
		}
	}
	if (i >= pmodel->source->numbones)
	{
		Error( "unknown eyeball bone \"%s\"\n", token );
	}

	// X
	GetToken (false);
	tmp[0] = atof (token);

	// Y
	GetToken (false);
	tmp[1] = atof (token);

	// Z
	GetToken (false);
	tmp[2] = atof (token);

	// mesh material 
	GetToken (false);
	strcpyn( szMeshMaterial, token );
	mesh_material = use_texture_as_material( lookup_texture( token ) );

	// diameter
	GetToken (false);
	eyeball->radius = atof (token) / 2.0;

	// Z angle offset
	GetToken (false);
	eyeball->zoffset = tan( DEG2RAD( atof (token) ) );

	// iris material
	GetToken (false);
	eyeball->iris_material = use_texture_as_material( lookup_texture( token ) );

	// pupil scale
	GetToken (false);
	eyeball->iris_scale = 1.0 / atof( token );

	eyeball->glint_material = use_texture_as_material( lookup_texture( "glint" ) );
	
	VectorCopy( tmp, eyeball->org );

	for (i = 0; i < pmodel->source->nummeshes; i++)
	{
		j = pmodel->source->meshindex[i]; // meshes are internally stored by material index

		if (j == mesh_material)
		{
			eyeball->mesh = i; // FIXME: should this be pre-adjusted?
			break;
		}
	}

	if (i >= pmodel->source->nummeshes)
	{
		Error("can't find eyeball texture \"%s\" on model\n", szMeshMaterial );
	}
	
	// translate eyeball into bone space
	VectorITransform( tmp, pmodel->source->boneToPose[eyeball->bone], eyeball->org );

	tmp[0] = 0; tmp[1] = 0; tmp[2] = 1;
	VectorIRotate( tmp, pmodel->source->boneToPose[eyeball->bone], eyeball->up );

	tmp[0] = 0; tmp[1] = 1; tmp[2] = 0; // FIXME: this is backwards
	VectorIRotate( tmp, pmodel->source->boneToPose[eyeball->bone], eyeball->forward );

	// these get overwritten by "eyelid" data
	eyeball->upperlidflexdesc		= 0;
	eyeball->lowerlidflexdesc		= 0;
}

void Option_Spherenormals( s_source_t *psource )
{
	Vector	pos;
	int i, j;
	int mesh_material;
	char szMeshMaterial[256];

	// mesh material 
	GetToken (false);
	strcpyn( szMeshMaterial, token );
	mesh_material = use_texture_as_material( lookup_texture( token ) );

	// X
	GetToken (false);
	pos[0] = atof (token);

	// Y
	GetToken (false);
	pos[1] = atof (token);

	// Z
	GetToken (false);
	pos[2] = atof (token);

	for (i = 0; i < psource->nummeshes; i++)
	{
		j = psource->meshindex[i]; // meshes are internally stored by material index

		if (j == mesh_material)
		{
			Vector *vertex = &psource->vertex[psource->mesh[i].vertexoffset];
			Vector *normal = &psource->normal[psource->mesh[i].vertexoffset];

			for (int k = 0; k < psource->mesh[i].numvertices; k++)
			{
				Vector n = vertex[k] - pos;
				VectorNormalize( n );
				if (DotProduct( n, normal[k] ) < 0.0)
				{
					normal[k] = -1 * n;
				}
				else
				{
					normal[k] = n;
				}
#if 0
				normal[k][0] += 0.5f * ( 2.0f * ( ( float )rand() ) / ( float )RAND_MAX ) - 1.0f;
				normal[k][1] += 0.5f * ( 2.0f * ( ( float )rand() ) / ( float )RAND_MAX ) - 1.0f;
				normal[k][2] += 0.5f * ( 2.0f * ( ( float )rand() ) / ( float )RAND_MAX ) - 1.0f;
				VectorNormalize( normal[k] );
#endif
			}
			break;
		}
	}

	if (i >= psource->nummeshes)
	{
		Error("can't find spherenormal texture \"%s\" on model\n", szMeshMaterial );
	}
}


int Add_Flexdesc( const char *name )
{
	int flexdesc;
	for ( flexdesc = 0; flexdesc < g_numflexdesc; flexdesc++)
	{
		if (stricmp( name, g_flexdesc[flexdesc].FACS ) == 0)
		{
			break;
		}
	}

	if (flexdesc >= MAXSTUDIOFLEXDESC)
	{
		Error( "Too many flex types, max %d\n", MAXSTUDIOFLEXDESC );
	}

	if (flexdesc == g_numflexdesc)
	{
		strcpyn( g_flexdesc[flexdesc].FACS, name );

		g_numflexdesc++;
	}
	return flexdesc;
}

void Option_Flex( char *name, char *vtafile, int imodel )
{
	if (g_numflexkeys >= MAXSTUDIOFLEXKEYS)
	{
		Error( "Too many flexes, max %d\n", MAXSTUDIOFLEXKEYS );
	}

	int flexdesc = Add_Flexdesc( name );

	// initialize
	g_flexkey[g_numflexkeys].imodel = imodel;
	g_flexkey[g_numflexkeys].flexdesc = flexdesc;
	g_flexkey[g_numflexkeys].target0 = 0.0;
	g_flexkey[g_numflexkeys].target1 = 1.0;
	g_flexkey[g_numflexkeys].target2 = 10;
	g_flexkey[g_numflexkeys].target3 = 11;
	g_flexkey[g_numflexkeys].split = 0;

	while (TokenAvailable())
	{
		GetToken(false);

		if (stricmp( token, "frame") == 0)
		{
			GetToken (false);

			g_flexkey[g_numflexkeys].frame = atoi( token );
		}
		else if (stricmp( token, "position") == 0)
		{
			GetToken (false);
			g_flexkey[g_numflexkeys].target1 = atof( token );
		}
		else if (stricmp( token, "split") == 0)
		{
			GetToken (false);
			g_flexkey[g_numflexkeys].split = atof( token );
		}
		else
		{
			Error( "unknown option: %s", token );
		}

	}

	if (g_numflexkeys > 1)
	{
		if (g_flexkey[g_numflexkeys-1].flexdesc == g_flexkey[g_numflexkeys].flexdesc)
		{
			g_flexkey[g_numflexkeys-1].target2 = g_flexkey[g_numflexkeys-1].target1;
			g_flexkey[g_numflexkeys-1].target3 = g_flexkey[g_numflexkeys].target1;
			g_flexkey[g_numflexkeys].target0 = g_flexkey[g_numflexkeys-1].target1;
		}
	}

	// link to source
	g_flexkey[g_numflexkeys].source = Load_Source( vtafile, "vta" );
	g_numflexkeys++;
	// this needs to be per model.
}



void Option_Eyelid( int imodel )
{
	char type[256];
	char vtafile[256];

	// type
	GetToken (false);
	strcpyn( type, token );

	// source
	GetToken (false);
	strcpyn( vtafile, token );

	int lowererframe, neutralframe, raiserframe;
	float lowerertarget, neutraltarget, raisertarget;
	int lowererdesc, neutraldesc, raiserdesc;
	int basedesc;
	float split = 0;
	char szEyeball[64] = {""};

	basedesc = g_numflexdesc;
	strcpyn( g_flexdesc[g_numflexdesc++].FACS, type );

	while (TokenAvailable())
	{
		GetToken(false);

		char localdesc[256];
		strcpy( localdesc, type );
		strcat( localdesc, "_" );
		strcat( localdesc, token );

		if (stricmp( token, "lowerer") == 0)
		{
			GetToken (false);
			lowererframe = atoi( token );
			GetToken (false);
			lowerertarget = atof( token );
			lowererdesc = g_numflexdesc;
			strcpyn( g_flexdesc[g_numflexdesc++].FACS, localdesc );
		}
		else if (stricmp( token, "neutral") == 0)
		{
			GetToken (false);
			neutralframe = atoi( token );
			GetToken (false);
			neutraltarget = atof( token );
			neutraldesc = g_numflexdesc;
			strcpyn( g_flexdesc[g_numflexdesc++].FACS, localdesc );
		}
		else if (stricmp( token, "raiser") == 0)
		{
			GetToken (false);
			raiserframe = atoi( token );
			GetToken (false);
			raisertarget = atof( token );
			raiserdesc = g_numflexdesc;
			strcpyn( g_flexdesc[g_numflexdesc++].FACS, localdesc );
		}
		else if (stricmp( token, "split") == 0)
		{
			GetToken (false);
			split = atof( token );
		}
		else if (stricmp( token, "eyeball") == 0)
		{
			GetToken (false);
			strcpy( szEyeball, token );
		}


		else
		{
			Error( "unknown option: %s", token );
		}
	}

	g_flexkey[g_numflexkeys+0].source = Load_Source( vtafile, "vta" );
	g_flexkey[g_numflexkeys+0].frame = lowererframe;
	g_flexkey[g_numflexkeys+0].flexdesc = basedesc;
	g_flexkey[g_numflexkeys+0].imodel = imodel;
	g_flexkey[g_numflexkeys+0].split = split;
	g_flexkey[g_numflexkeys+0].target0 = -11;
	g_flexkey[g_numflexkeys+0].target1 = -10;
	g_flexkey[g_numflexkeys+0].target2 = lowerertarget;
	g_flexkey[g_numflexkeys+0].target3 = neutraltarget;

	g_flexkey[g_numflexkeys+1].source = g_flexkey[g_numflexkeys+0].source;
	g_flexkey[g_numflexkeys+1].frame = neutralframe;
	g_flexkey[g_numflexkeys+1].flexdesc = basedesc;
	g_flexkey[g_numflexkeys+1].imodel = imodel;
	g_flexkey[g_numflexkeys+1].split = split;
	g_flexkey[g_numflexkeys+1].target0 = lowerertarget;
	g_flexkey[g_numflexkeys+1].target1 = neutraltarget;
	g_flexkey[g_numflexkeys+1].target2 = neutraltarget;
	g_flexkey[g_numflexkeys+1].target3 = raisertarget;

	g_flexkey[g_numflexkeys+2].source = g_flexkey[g_numflexkeys+0].source;
	g_flexkey[g_numflexkeys+2].frame = raiserframe;
	g_flexkey[g_numflexkeys+2].flexdesc = basedesc;
	g_flexkey[g_numflexkeys+2].imodel = imodel;
	g_flexkey[g_numflexkeys+2].split = split;
	g_flexkey[g_numflexkeys+2].target0 = neutraltarget;
	g_flexkey[g_numflexkeys+2].target1 = raisertarget;
	g_flexkey[g_numflexkeys+2].target2 = 10;
	g_flexkey[g_numflexkeys+2].target3 = 11;
	g_numflexkeys += 3;

	s_model_t *pmodel = g_model[imodel];
	for (int i = 0; i < pmodel->numeyeballs; i++)
	{
		s_eyeball_t *peyeball = &(pmodel->eyeball[i]);

		if (szEyeball[0] != '\0')
		{
			if (stricmp( peyeball->name, szEyeball ) != 0)
				continue;
		}

		switch( type[0] )
		{
		case 'u':
			peyeball->upperlidflexdesc	= basedesc;
			peyeball->upperflexdesc[0]	= lowererdesc; 
			peyeball->uppertarget[0]	= lowerertarget;
			peyeball->upperflexdesc[1]	= neutraldesc; 
			peyeball->uppertarget[1]	= neutraltarget;
			peyeball->upperflexdesc[2]	= raiserdesc; 
			peyeball->uppertarget[2]	= raisertarget;
			break;
		case 'l':
			peyeball->lowerlidflexdesc	= basedesc;
			peyeball->lowerflexdesc[0]	= lowererdesc; 
			peyeball->lowertarget[0]	= lowerertarget;
			peyeball->lowerflexdesc[1]	= neutraldesc; 
			peyeball->lowertarget[1]	= neutraltarget;
			peyeball->lowerflexdesc[2]	= raiserdesc; 
			peyeball->lowertarget[2]	= raisertarget;
			break;
		}
	}
}

/*
=================
=================
*/
int Option_Mouth( s_model_t *pmodel )
{
	// index
	GetToken (false);
	int index = atoi( token );
	if (index >= g_nummouths)
		g_nummouths = index + 1;

	// flex controller name
	GetToken (false);
	g_mouth[index].flexdesc = Add_Flexdesc( token );

	// bone name
	GetToken (false);
	strcpyn( g_mouth[index].bonename, token );

	// vector
	GetToken (false);
	g_mouth[index].forward[0] = atof( token );
	GetToken (false);
	g_mouth[index].forward[1] = atof( token );
	GetToken (false);
	g_mouth[index].forward[2] = atof( token );
	return 0;
}



void Option_Flexcontroller( s_model_t *pmodel )
{
	char type[256];
	float range_min = 0.0;
	float range_max = 1.0;

	// g_flex
	GetToken (false);
	strcpy( type, token );

	while (TokenAvailable())
	{
		GetToken(false);

		if (stricmp( token, "range") == 0)
		{
			GetToken(false);
			range_min = atof( token );

			GetToken(false);
			range_max = atof( token );
		}
		else
		{
			if (g_numflexcontrollers >= MAXSTUDIOFLEXCTRL)
			{
				Error( "Too many flex controllers, max %d\n", MAXSTUDIOFLEXCTRL );
			}


			strcpyn( g_flexcontroller[g_numflexcontrollers].name, token );
			strcpyn( g_flexcontroller[g_numflexcontrollers].type, type );
			g_flexcontroller[g_numflexcontrollers].min = range_min;
			g_flexcontroller[g_numflexcontrollers].max = range_max;
			g_numflexcontrollers++;
		}
	}

	// this needs to be per model.
}

void Option_Flexrule( s_model_t *pmodel, char *name )
{
	int precidence[32];
	precidence[ STUDIO_CONST ] = 	0;
	precidence[ STUDIO_FETCH1 ] =	0;
	precidence[ STUDIO_FETCH2 ] =	0;
	precidence[ STUDIO_ADD ] =		1;
	precidence[ STUDIO_SUB ] =		1;
	precidence[ STUDIO_MUL ] =		2;
	precidence[ STUDIO_DIV ] =		2;
	precidence[ STUDIO_NEG ] =		4;
	precidence[ STUDIO_EXP ] =		3;
	precidence[ STUDIO_OPEN ] =		0;	// only used in token parsing
	precidence[ STUDIO_CLOSE ] =	0;

	s_flexop_t stream[MAX_OPS];
	int i = 0;
	s_flexop_t stack[MAX_OPS];
	int j = 0;
	int k = 0;

	s_flexrule_t *pRule = &g_flexrule[g_numflexrules++];

	if (g_numflexrules > MAXSTUDIOFLEXRULES)
	{
		Error( "Too many flex rules (max %d)\n", MAXSTUDIOFLEXRULES );
	}

	int flexdesc;
	for ( flexdesc = 0; flexdesc < g_numflexdesc; flexdesc++)
	{
		if (stricmp( name, g_flexdesc[flexdesc].FACS ) == 0)
		{
			break;
		}
	}

	if (flexdesc >= g_numflexdesc)
	{
		Error( "Rule for unknown flex %s\n", name );
	}

	pRule->flex = flexdesc;
	pRule->numops = 0;

	// = 
	GetToken(false);

	// parse all the tokens
	while (TokenAvailable())
	{
		GetExprToken(false);

		if ( token[0] == '(' )
		{
			stream[i].op = STUDIO_OPEN;
		}
		else if ( token[0] == ')' )
		{
			stream[i].op = STUDIO_CLOSE;
		}
		else if ( token[0] == '+' )
		{
			stream[i].op = STUDIO_ADD;
		}
		else if ( token[0] == '-' )
		{
			// check for unary operator here ?
			stream[i].op = STUDIO_SUB;
		}
		else if ( token[0] == '*' )
		{
			stream[i].op = STUDIO_MUL;
		}
		else if ( token[0] == '/' )
		{
			stream[i].op = STUDIO_DIV;
		}
		else if ( isdigit( token[0] ))
		{
			stream[i].op = STUDIO_CONST;
			stream[i].d.value = atof( token );
		}
		else 
		{
			if (token[0] == '%')
			{
				GetExprToken(false);

				for (k = 0; k < g_numflexdesc; k++)
				{
					if (stricmp( token, g_flexdesc[k].FACS ) == 0)
					{
						stream[i].op = STUDIO_FETCH2;
						stream[i].d.index = k;
						break;
					}
				}
				if (k >= g_numflexdesc)
				{
					Error( "unknown flex %s\n", token );
				}
			}
			else
			{
				for (k = 0; k < g_numflexcontrollers; k++)
				{
					if (stricmp( token, g_flexcontroller[k].name ) == 0)
					{
						stream[i].op = STUDIO_FETCH1;
						stream[i].d.index = k;
						break;
					}
				}
				if (k >= g_numflexcontrollers)
				{
					Error( "unknown controller %s\n", token );
				}
			}
		}
		i++;
	}

	j = 0;
	for (k = 0; k < i; k++)
	{
		switch( stream[k].op )
		{
		case STUDIO_CONST:
		case STUDIO_FETCH1:
		case STUDIO_FETCH2:
			pRule->op[pRule->numops++] = stream[k];
			break;
		case STUDIO_OPEN:
			stack[j++] = stream[k];
			break;
		case STUDIO_CLOSE:
			while (j > 0 && stack[j-1].op != STUDIO_OPEN)
			{
				pRule->op[pRule->numops++] = stack[j-1];
				j--;
			}
			if (j > 0) 
				j--;
			break;
		case STUDIO_ADD:
		case STUDIO_SUB:
		case STUDIO_MUL:
		case STUDIO_DIV:
			if (j > 0 && precidence[stream[k].op] < precidence[stack[j-1].op])
			{
				pRule->op[pRule->numops++] = stack[j-1];
				j--;
			}
			stack[j++] = stream[k];
			break;
		}
		if (pRule->numops >= MAX_OPS)
			Error("expression for \"%s\" too complicated\n", g_flexdesc[pRule->flex].FACS );
	}
	while (j > 0)
	{
		pRule->op[pRule->numops++] = stack[j-1];
		j--;
		if (pRule->numops >= MAX_OPS)
			Error("expression for \"%s\" too complicated\n", g_flexdesc[pRule->flex].FACS );
	}

	if (0)
	{
		printf("%s = ", g_flexdesc[pRule->flex].FACS );
		for ( i = 0; i < pRule->numops; i++)
		{
			switch( pRule->op[i].op )
			{
			case STUDIO_CONST: printf("%f ", pRule->op[i].d.value ); break;
			case STUDIO_FETCH1: printf("%s ", g_flexcontroller[pRule->op[i].d.index].name ); break;
			case STUDIO_FETCH2: printf("[%d] ", pRule->op[i].d.index ); break;
			case STUDIO_ADD: printf("+ "); break;
			case STUDIO_SUB: printf("- "); break;
			case STUDIO_MUL: printf("* "); break;
			case STUDIO_DIV: printf("/ "); break;
			default:
				printf("err%d ", pRule->op[i].op ); break;
			}
		}
		printf("\n");
	}
}


int Cmd_Model( )
{
	g_model[g_nummodels] = (s_model_t *)kalloc( 1, sizeof( s_model_t ) );
	
	// name
	if (!GetToken(false)) return 0;
	strcpyn( g_model[g_nummodels]->name, token );

	// fake g_bodypart stuff
	if (g_numbodyparts == 0) {
		g_bodypart[g_numbodyparts].base = 1;
	}
	else {
		g_bodypart[g_numbodyparts].base = g_bodypart[g_numbodyparts-1].base * g_bodypart[g_numbodyparts-1].nummodels;
	}
	strcpyn( g_bodypart[g_numbodyparts].name, token );

	g_bodypart[g_numbodyparts].pmodel[g_bodypart[g_numbodyparts].nummodels] = g_model[g_nummodels];
	g_bodypart[g_numbodyparts].nummodels = 1;
	g_numbodyparts++;

	Option_Studio( g_model[g_nummodels] );
	
	int depth = 0;
	while (1)
	{
		char FAC[256], vtafile[256];
		if (depth > 0)
		{
			if(!GetToken(true)) 
			{
				break;
			}
		}
		else
		{
			if (!TokenAvailable()) 
			{
				break;
			}
			else 
			{
				GetToken (false);
			}
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				printf("missing }\n" );
				exit(1);
			}
			return 1;
		}
		if (stricmp("{", token ) == 0)
		{
			depth++;
		}
		else if (stricmp("}", token ) == 0)
		{
			depth--;
		}
		else if (stricmp("eyeball", token ) == 0)
		{
			Option_Eyeball( g_model[g_nummodels] );
		}
		else if (stricmp("eyelid", token ) == 0)
		{
			Option_Eyelid( g_nummodels );
		}
		else if (stricmp("flex", token ) == 0)
		{
			// g_flex
			GetToken (false);
			strcpy( FAC, token );
			if (depth == 0)
			{
				// file
				GetToken (false);
				strcpy( vtafile, token );
			}
			Option_Flex( FAC, vtafile, g_nummodels ); // FIXME: this needs to point to a model used, not loaded!!!
		}
		else if (stricmp("defaultflex", token ) == 0)
		{
			if (depth == 0)
			{
				// file
				GetToken (false);
				strcpy( vtafile, token );
			}

			// g_flex
			Option_Flex( "default", vtafile, g_nummodels ); // FIXME: this needs to point to a model used, not loaded!!!
			g_defaultflexkey = &g_flexkey[g_numflexkeys-1];
		}
		else if (stricmp("flexfile", token ) == 0)
		{
			// file
			GetToken (false);
			strcpy( vtafile, token );
		}
		else if (stricmp("localvar", token ) == 0)
		{
			while (TokenAvailable())
			{
				GetToken( false );
				Add_Flexdesc( token );
			}
		}
		else if (stricmp("mouth", token ) == 0)
		{
			Option_Mouth( g_model[g_nummodels] );
		}
		else if (stricmp("flexcontroller", token ) == 0)
		{
			Option_Flexcontroller( g_model[g_nummodels] );
		}
		else if (token[0] == '%' )
		{
			Option_Flexrule( g_model[g_nummodels], &token[1] );
		}
		else if (stricmp("attachment", token ) == 0)
		{
		// 	Option_Attachment( g_model[g_nummodels] );
		}
		else if (stricmp( token, "spherenormals") == 0)
		{
			Option_Spherenormals( g_model[g_nummodels]->source );
		}
		else
		{
			Error( "unknown model option \"%s\"\n", token );
		}

		if (depth < 0)
		{
			printf("missing {\n");
			exit(1);
		}
	};

	g_nummodels++;
	return 0;
}

/*
=================
=================
*/

int Cmd_IKChain( )
{
	if (!GetToken(false)) return 0;

	strcpyn( g_ikchain[g_numikchains].name, token );

	GetToken(false);
	strcpyn( g_ikchain[g_numikchains].bonename, token );

	g_ikchain[g_numikchains].axis = STUDIO_Z;
	g_ikchain[g_numikchains].value = 0.0;
	g_ikchain[g_numikchains].height = 18.0;
	g_ikchain[g_numikchains].floor = 0.0;
	g_ikchain[g_numikchains].radius = 0.0;

	while (TokenAvailable())
	{
		GetToken(false);

		if (lookupControl( token ) != -1)
		{
			g_ikchain[g_numikchains].axis = lookupControl( token );
			GetToken(false);
			g_ikchain[g_numikchains].value = atof( token );
		}
		else if (stricmp( "height", token ) == 0)
		{
			GetToken(false);
			g_ikchain[g_numikchains].height = atof( token );
		}
		else if (stricmp( "pad", token ) == 0)
		{
			GetToken(false);
			g_ikchain[g_numikchains].radius = atof( token ) / 2.0;
		}
		else if (stricmp( "floor", token ) == 0)
		{
			GetToken(false);
			g_ikchain[g_numikchains].floor = atof( token );
		}
	}
	g_numikchains++;

	return 1;
}


/*
=================
=================
*/


void Cmd_IKAutoplayLock( )
{
	GetToken(false);
	strcpyn( g_ikautoplaylock[g_numikautoplaylocks].name, token );

	GetToken(false);
	g_ikautoplaylock[g_numikautoplaylocks].flPosWeight = atof( token );

	GetToken(false);
	g_ikautoplaylock[g_numikautoplaylocks].flLocalQWeight = atof( token );
	
	g_numikautoplaylocks++;
}


/*
=================
=================
*/
int Cmd_Root (void)
{
	if (GetToken (false))
	{
		strcpyn( rootname, token );
		return 0;
	}
	return 1;
}


int Cmd_Controller (void)
{
	if (GetToken (false))
	{
		if (!stricmp("mouth",token))
		{
			g_bonecontroller[g_numbonecontrollers].inputfield = 4;
		}
		else
		{
			g_bonecontroller[g_numbonecontrollers].inputfield = atoi(token);
		}
		if (GetToken(false))
		{
			strcpyn( g_bonecontroller[g_numbonecontrollers].name, token );
			GetToken(false);
			if ((g_bonecontroller[g_numbonecontrollers].type = lookupControl(token)) == -1) 
			{
				printf("unknown g_bonecontroller type '%s'\n", token );
				return 0;
			}
			GetToken(false);
			g_bonecontroller[g_numbonecontrollers].start = atof( token );
			GetToken(false);
			g_bonecontroller[g_numbonecontrollers].end = atof( token );

			if (g_bonecontroller[g_numbonecontrollers].type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
			{
				if (((int)(g_bonecontroller[g_numbonecontrollers].start + 360) % 360) == ((int)(g_bonecontroller[g_numbonecontrollers].end + 360) % 360))
				{
					g_bonecontroller[g_numbonecontrollers].type |= STUDIO_RLOOP;
				}
			}
			g_numbonecontrollers++;
		}
	}
	return 1;
}


/*
=================
=================
*/

// Debugging function that enumerate all a models bones to stdout.
static void SpewBones()
{
	Warning("g_numbones %i\n",g_numbones);

	for ( int i = g_numbones; --i >= 0; )
	{
		printf("%s\n",g_bonetable[i].name);
	}
}

void Cmd_ScreenAlign ( void )
{
	if (GetToken (false))
	{
		
		Assert( g_numscreenalignedbones < MAXSTUDIOSRCBONES );

		strcpyn( g_screenalignedbone[g_numscreenalignedbones].name, token );
		g_screenalignedbone[g_numscreenalignedbones].flags = BONE_SCREEN_ALIGN_SPHERE;

		if( GetToken( false ) )
		{
			if( !strcmpi( "sphere", token )  )
			{
				g_screenalignedbone[g_numscreenalignedbones].flags = BONE_SCREEN_ALIGN_SPHERE;				
			}
			else if( !strcmpi( "cylinder", token ) )
			{
				g_screenalignedbone[g_numscreenalignedbones].flags = BONE_SCREEN_ALIGN_CYLINDER;				
			}
		}

		g_numscreenalignedbones++;

	} else
	{
		Error( "$screenalign: expected bone name\n" );
	}
}

/*
=================
=================
*/
void Cmd_BBox (void)
{
	GetToken (false);
	bbox[0][0] = atof( token );

	GetToken (false);
	bbox[0][1] = atof( token );

	GetToken (false);
	bbox[0][2] = atof( token );

	GetToken (false);
	bbox[1][0] = atof( token );

	GetToken (false);
	bbox[1][1] = atof( token );

	GetToken (false);
	bbox[1][2] = atof( token );

	g_wrotebbox = true;
}

/*
=================
=================
*/
void Cmd_CBox (void)
{
	GetToken (false);
	cbox[0][0] = atof( token );

	GetToken (false);
	cbox[0][1] = atof( token );

	GetToken (false);
	cbox[0][2] = atof( token );

	GetToken (false);
	cbox[1][0] = atof( token );

	GetToken (false);
	cbox[1][1] = atof( token );

	GetToken (false);
	cbox[1][2] = atof( token );

	g_wrotecbox = true;
}



/*
=================
=================
*/
void Cmd_Gamma (void)
{
	GetToken (false);
	g_gamma = atof( token );
}





/*
=================
=================
*/
int Cmd_TextureGroup( )
{
	int i;
	int depth = 0;
	int index = 0;
	int group = 0;

	if (g_numtextures == 0)
		Error( "texturegroups must follow model loading\n");

	if (!GetToken(false)) return 0;

	if (g_numskinref == 0)
		g_numskinref = g_numtextures;

	while (1)
	{
		if(!GetToken(true)) 
		{
			break;
		}

		if (endofscript)
		{
			if (depth != 0)
			{
				Error("missing }\n" );
			}
			return 1;
		}
		if (token[0] == '{')
		{
			depth++;
		}
		else if (token[0] == '}')
		{
			depth--;
			if (depth == 0)
				break;
			group++;
			index = 0;
		}
		else if (depth == 2)
		{
			i = use_texture_as_material( lookup_texture( token ) );
			g_texturegroup[g_numtexturegroups][group][index] = i;
			if (group != 0)
				g_texture[i].parent = g_texturegroup[g_numtexturegroups][0][index];
			index++;
			g_numtexturereps[g_numtexturegroups] = index;
			g_numtexturelayers[g_numtexturegroups] = group + 1;
		}
	}

	g_numtexturegroups++;

	return 0;
}



/*
=================
=================
*/
int Cmd_Hitgroup( )
{
	GetToken (false);
	g_hitgroup[g_numhitgroups].group = atoi( token );
	GetToken (false);
	strcpyn( g_hitgroup[g_numhitgroups].name, token );
	g_numhitgroups++;

	return 0;
}


int Cmd_Hitbox( )
{
	bool autogenerated = false;
	if ( g_hitboxsets.Size() == 0 )
	{
		g_hitboxsets.AddToTail();
		autogenerated = true;
	}

	// Last one
	s_hitboxset *set = &g_hitboxsets[ g_hitboxsets.Size() - 1 ];
	if ( autogenerated )
	{
		memset( set, 0, sizeof( *set ) );

		// fill in name if it wasn't specified in the .qc
		strcpy( set->hitboxsetname, "default" );
	}

	GetToken (false);
	set->hitbox[set->numhitboxes].group = atoi( token );
	
	// Grab the bone name:
	GetToken (false);
	strcpyn( set->hitbox[set->numhitboxes].name, token );

	GetToken (false);
	set->hitbox[set->numhitboxes].bmin[0] = atof( token );
	GetToken (false);
	set->hitbox[set->numhitboxes].bmin[1] = atof( token );
	GetToken (false);
	set->hitbox[set->numhitboxes].bmin[2] = atof( token );
	GetToken (false);
	set->hitbox[set->numhitboxes].bmax[0] = atof( token );
	GetToken (false);
	set->hitbox[set->numhitboxes].bmax[1] = atof( token );
	GetToken (false);
	set->hitbox[set->numhitboxes].bmax[2] = atof( token );

	//Scale hitboxes
	scale_vertex( set->hitbox[set->numhitboxes].bmin );
	scale_vertex( set->hitbox[set->numhitboxes].bmax );
	// clear out the hitboxname:
	memset( set->hitbox[set->numhitboxes].hitboxname, 0, sizeof( set->hitbox[set->numhitboxes].hitboxname ) );

	// Grab the hit box name if present:
	if( TokenAvailable() )
	{
		GetToken (false);
		strcpyn( set->hitbox[set->numhitboxes].hitboxname, token );
	}


	set->numhitboxes++;

	return 0;
}

int Cmd_HitboxSet( void )
{
	// Add a new hitboxset
	s_hitboxset *set = &g_hitboxsets[ g_hitboxsets.AddToTail() ];
	GetToken( false );
	memset( set, 0, sizeof( *set ) );
	strcpy( set->hitboxsetname, token );
	return 0;
}


//-----------------------------------------------------------------------------
// Assigns a default surface property to the entire model
//-----------------------------------------------------------------------------
struct SurfacePropName_t
{
	char m_pJointName[128];
	char m_pSurfaceProp[128];
};

static char								s_pDefaultSurfaceProp[128];
static CUtlVector<SurfacePropName_t>	s_JointSurfaceProp;

//-----------------------------------------------------------------------------
// Assigns a default surface property to the entire model
//-----------------------------------------------------------------------------
int	Cmd_SurfaceProp ()
{
	GetToken (false);
	strcpyn( s_pDefaultSurfaceProp, token );
	return 0;
}


//-----------------------------------------------------------------------------
// Assigns a surface property to a particular joint
//-----------------------------------------------------------------------------
int	Cmd_JointSurfaceProp ()
{
	// Get joint name...
	GetToken (false);

	// Search for the name in our list
	int i;
	for ( i = s_JointSurfaceProp.Count(); --i >= 0; )
	{
		if (!stricmp(s_JointSurfaceProp[i].m_pJointName, token))
		{
			break;
		}
	}

	// Add new entry if we haven't seen this name before
	if (i < 0)
	{
		i = s_JointSurfaceProp.AddToTail();
		strcpyn( s_JointSurfaceProp[i].m_pJointName, token );
	}

	// surface property name
	GetToken(false);
	strcpyn( s_JointSurfaceProp[i].m_pSurfaceProp, token );
	return 0;
}


//-----------------------------------------------------------------------------
// Returns the default surface prop name
//-----------------------------------------------------------------------------
char* GetDefaultSurfaceProp ( )
{
	return s_pDefaultSurfaceProp;
}


//-----------------------------------------------------------------------------
// Returns surface property for a given joint
//-----------------------------------------------------------------------------
static char* FindSurfaceProp ( char const* pJointName )
{
	for ( int i = s_JointSurfaceProp.Count(); --i >= 0; )
	{
		if (!stricmp(s_JointSurfaceProp[i].m_pJointName, pJointName))
		{
			return s_JointSurfaceProp[i].m_pSurfaceProp;
		}
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Returns surface property for a given joint
//-----------------------------------------------------------------------------
char* GetSurfaceProp ( char const* pJointName )
{
	while( pJointName )
	{
		// First try to find this joint
		char* pSurfaceProp = FindSurfaceProp( pJointName );
		if (pSurfaceProp)
			return pSurfaceProp;

		// If we can't find the joint, then find it's parent...
		if (!g_numbones)
			return s_pDefaultSurfaceProp;

		int i = findGlobalBone( pJointName );

		if ((i >= 0) && (g_bonetable[i].parent >= 0))
		{
			pJointName = g_bonetable[g_bonetable[i].parent].name;
		}
		else
		{
			pJointName = 0;
		}
	}

	// No match, return the default one
	return s_pDefaultSurfaceProp;
}


//-----------------------------------------------------------------------------
// Returns surface property for a given joint
//-----------------------------------------------------------------------------
void ConsistencyCheckSurfaceProp ( )
{
	for ( int i = s_JointSurfaceProp.Count(); --i >= 0; )
	{
		int j = findGlobalBone( s_JointSurfaceProp[i].m_pJointName );

		if (j < 0)
		{
			printf("*** ERROR: You specified a joint surface property for joint\n"
				"    \"%s\" which either doesn't exist or was optimized out.\n", s_JointSurfaceProp[i].m_pJointName );
		}
	}
}


//-----------------------------------------------------------------------------
// Assigns a default contents to the entire model
//-----------------------------------------------------------------------------
struct ContentsName_t
{
	char m_pJointName[128];
	int m_nContents;
};

static int s_nDefaultContents = CONTENTS_SOLID;
static CUtlVector<ContentsName_t>	s_JointContents;


//-----------------------------------------------------------------------------
// Parse contents flags
//-----------------------------------------------------------------------------
static void ParseContents( int *pAddFlags, int *pRemoveFlags )
{
	*pAddFlags = 0;
	*pRemoveFlags = 0;
	do 
	{
		GetToken (false);

		if ( !stricmp( token, "grate" ) )
		{
			*pAddFlags |= CONTENTS_GRATE;
			*pRemoveFlags |= CONTENTS_SOLID;
		}
		else if ( !stricmp( token, "ladder" ) )
		{
			*pAddFlags |= CONTENTS_LADDER;
		}
		else if ( !stricmp( token, "solid" ) )
		{
			*pAddFlags |= CONTENTS_SOLID;
		}
		else if ( !stricmp( token, "monster" ) )
		{
			*pAddFlags |= CONTENTS_MONSTER;
		}
		else if ( !stricmp( token, "notsolid" ) )
		{
			*pRemoveFlags |= CONTENTS_SOLID;
		}
	} while (TokenAvailable());
}


//-----------------------------------------------------------------------------
// Assigns a default contents to the entire model
//-----------------------------------------------------------------------------
int	Cmd_Contents()
{
	int nAddFlags, nRemoveFlags;
	ParseContents( &nAddFlags, &nRemoveFlags );
	s_nDefaultContents |= nAddFlags;
	s_nDefaultContents &= ~nRemoveFlags;
	return 0;
}


//-----------------------------------------------------------------------------
// Assigns contents to a particular joint
//-----------------------------------------------------------------------------
int	Cmd_JointContents ()
{
	// Get joint name...
	GetToken (false);

	// Search for the name in our list
	int i;
	for ( i = s_JointContents.Count(); --i >= 0; )
	{
		if (!stricmp(s_JointContents[i].m_pJointName, token))
		{
			break;
		}
	}

	// Add new entry if we haven't seen this name before
	if (i < 0)
	{
		i = s_JointContents.AddToTail();
		strcpyn( s_JointContents[i].m_pJointName, token );
	}

	int nAddFlags, nRemoveFlags;
	ParseContents( &nAddFlags, &nRemoveFlags );
	s_JointContents[i].m_nContents = CONTENTS_SOLID;
	s_JointContents[i].m_nContents |= nAddFlags;
	s_JointContents[i].m_nContents &= ~nRemoveFlags;
	return 0;
}


//-----------------------------------------------------------------------------
// Returns the default contents
//-----------------------------------------------------------------------------
int GetDefaultContents( )
{
	return s_nDefaultContents;
}


//-----------------------------------------------------------------------------
// Returns contents for a given joint
//-----------------------------------------------------------------------------
static int FindContents( char const* pJointName )
{
	for ( int i = s_JointContents.Count(); --i >= 0; )
	{
		if (!stricmp(s_JointContents[i].m_pJointName, pJointName))
		{
			return s_JointContents[i].m_nContents;
		}
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Returns contents for a given joint
//-----------------------------------------------------------------------------
int GetContents( char const* pJointName )
{
	while( pJointName )
	{
		// First try to find this joint
		int nContents = FindContents( pJointName );
		if (nContents != -1)
			return nContents;

		// If we can't find the joint, then find it's parent...
		if (!g_numbones)
			return s_nDefaultContents;

		int i = findGlobalBone( pJointName );

		if ((i >= 0) && (g_bonetable[i].parent >= 0))
		{
			pJointName = g_bonetable[g_bonetable[i].parent].name;
		}
		else
		{
			pJointName = 0;
		}
	}

	// No match, return the default one
	return s_nDefaultContents;
}


//-----------------------------------------------------------------------------
// Checks specified contents
//-----------------------------------------------------------------------------
void ConsistencyCheckContents( )
{
	for ( int i = s_JointContents.Count(); --i >= 0; )
	{
		int j = findGlobalBone( s_JointContents[i].m_pJointName );

		if (j < 0)
		{
			printf("*** ERROR: You specified a joint contents for joint\n"
				"    \"%s\" which either doesn't exist or was optimized out.\n", s_JointSurfaceProp[i].m_pJointName );
		}
	}
}


/*
=================
=================
*/
int Cmd_Attachment( )
{
	// name
	GetToken (false);
	strcpyn( g_attachment[g_numattachments].name, token );

	// bone name
	GetToken (false);
	strcpyn( g_attachment[g_numattachments].bonename, token );

	Vector tmp;

	// position
	GetToken (false);
	tmp.x = atof( token );
	GetToken (false);
	tmp.y = atof( token );
	GetToken (false);
	tmp.z = atof( token );

	scale_vertex( tmp );
	// identity matrix
	AngleMatrix( QAngle( 0, 0, 0 ), g_attachment[g_numattachments].local );

	while (TokenAvailable())
	{
		GetToken (false);

		if (stricmp(token,"absolute") == 0)
		{
			g_attachment[g_numattachments].type |= IS_ABSOLUTE;
			AngleIMatrix( g_defaultrotation, g_attachment[g_numattachments].local );
			// AngleIMatrix( Vector( 0, 0, 0 ), g_attachment[g_numattachments].local );
		}
		else if (stricmp(token,"rigid") == 0)
		{
			g_attachment[g_numattachments].type |= IS_RIGID;
		}
		else if (stricmp(token,"rotate") == 0)
		{
			QAngle angles;
			for (int i = 0; i < 3; ++i)
			{
				if (!TokenAvailable())
					break;

				GetToken(false);
				angles[i] = atof( token );
			}
			AngleMatrix( angles, g_attachment[g_numattachments].local );
		}
		else if (stricmp(token,"x_and_z_axes") == 0)
		{
			int i;
			Vector xaxis, yaxis, zaxis;
			for (i = 0; i < 3; ++i)
			{
				if (!TokenAvailable())
					break;

				GetToken(false);
				xaxis[i] = atof( token );
			}
			for (i = 0; i < 3; ++i)
			{
				if (!TokenAvailable())
					break;

				GetToken(false);
				zaxis[i] = atof( token );
			}
			VectorNormalize( xaxis );
			VectorMA( zaxis, -DotProduct( zaxis, xaxis ), xaxis, zaxis );
			VectorNormalize( zaxis );
			CrossProduct( zaxis, xaxis, yaxis );
			MatrixSetColumn( xaxis, 0, g_attachment[g_numattachments].local );
			MatrixSetColumn( yaxis, 1, g_attachment[g_numattachments].local );
			MatrixSetColumn( zaxis, 2, g_attachment[g_numattachments].local );
			MatrixSetColumn( vec3_origin, 3, g_attachment[g_numattachments].local );
		}
		else
		{
			Error("unknown attachment (%s) option: ", g_attachment[g_numattachments].name, token );
		}
	}

	g_attachment[g_numattachments].local[0][3] = tmp.x;
	g_attachment[g_numattachments].local[1][3] = tmp.y;
	g_attachment[g_numattachments].local[2][3] = tmp.z;

	g_numattachments++;
	return 0;
}


int LookupAttachment( char *name )
{
	int i;
	for (i = 0; i < g_numattachments; i++)
	{
		if (stricmp( g_attachment[i].name, name ) == 0)
		{
			return i;
		}
	}
	return -1;
}


/*
=================
=================
*/
void Cmd_Renamebone( )
{
	// from
	GetToken (false);
	strcpyn( g_renamedbone[g_numrenamedbones].from, token );

	// to
	GetToken (false);
	strcpyn( g_renamedbone[g_numrenamedbones].to, token );

	g_numrenamedbones++;
}


/*
=================
=================
*/


void Cmd_Skiptransition( )
{
	int nskips = 0;
	int list[10];

	while (TokenAvailable())
	{
		GetToken (false);
		list[nskips++] = LookupXNode( token );
	}

	for (int i = 0; i < nskips; i++)
	{
		for (int j = 0; j < nskips; j++)
		{
			if (list[i] != list[j])
			{
				g_xnodeskip[g_numxnodeskips][0] = list[i];
				g_xnodeskip[g_numxnodeskips][1] = list[j];
				g_numxnodeskips++;
			}
		}
	}
}


//-----------------------------------------------------------------------------
//
// The following code is all related to LODs
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Parse replacemodel command, causes an LOD to use a new model
//-----------------------------------------------------------------------------

static void Cmd_ReplaceModel( LodScriptData_t& lodData )
{
	int i = lodData.modelReplacements.AddToTail();
	CLodScriptReplacement_t& newReplacement = lodData.modelReplacements[i];

	// from
	GetToken( false );

	// Strip off extensions for the source...
	char* pDot = strrchr( token, '.' );
	if (pDot)
		*pDot = 0;

	newReplacement.SetSrcName( token );

	// to
	GetToken( false );
	newReplacement.SetDstName( token );

	// check for "reverse"
	bool reverse =  false;
	if( TokenAvailable() && GetToken( false ) )
	{
		if( stricmp( "reverse", token ) == 0 )
		{
			reverse = true;
		}
		else
		{
			Error( "\"%s\" unexpected\n", token );
		}
	}

	// If the LOD system tells us to replace "blank", let's forget
	// we ever read this. Have to do it here so parsing works
	if( !stricmp( newReplacement.GetSrcName(), "blank" ) )
	{
		lodData.modelReplacements.FastRemove( i );
		return;
	}
	
	// Load the source right here baby! That way its bones will get converted
	newReplacement.m_pSource = Load_Source( newReplacement.GetDstName(), 
		"smd", reverse ); 
}

//-----------------------------------------------------------------------------
// Parse removemodel command, causes an LOD to stop using a model
//-----------------------------------------------------------------------------

static void Cmd_RemoveModel( LodScriptData_t& lodData )
{
	int i = lodData.modelReplacements.AddToTail();
	CLodScriptReplacement_t& newReplacement = lodData.modelReplacements[i];

	// from
	GetToken( false );

	// Strip off extensions...
	char* pDot = strrchr( token, '.' );
	if (pDot)
		*pDot = 0;

	newReplacement.SetSrcName( token );

	// to
	newReplacement.SetDstName( "" );

	// If the LOD system tells us to replace "blank", let's forget
	// we ever read this. Have to do it here so parsing works
	if( !stricmp( newReplacement.GetSrcName(), "blank" ) )
	{
		lodData.modelReplacements.FastRemove( i );
	}
}

//-----------------------------------------------------------------------------
// Parse replacebone command, causes a part of an LOD model to use a different bone
//-----------------------------------------------------------------------------

static void Cmd_ReplaceBone( LodScriptData_t& lodData )
{
	int i = lodData.boneReplacements.AddToTail();
	CLodScriptReplacement_t& newReplacement = lodData.boneReplacements[i];

	// from
	GetToken( false );
	newReplacement.SetSrcName( token );

	// to
	GetToken( false );
	newReplacement.SetDstName( token );
}

//-----------------------------------------------------------------------------
// Parse bonetreecollapse command, causes the entire subtree to use the same bone as the node
//-----------------------------------------------------------------------------

static void Cmd_BoneTreeCollapse( LodScriptData_t& lodData )
{
	int i = lodData.boneTreeCollapses.AddToTail();
	CLodScriptReplacement_t& newCollapse = lodData.boneTreeCollapses[i];

	// from
	GetToken( false );
	newCollapse.SetSrcName( token );
}

//-----------------------------------------------------------------------------
// Parse replacematerial command, causes a material to be used in place of another
//-----------------------------------------------------------------------------

static void Cmd_ReplaceMaterial( LodScriptData_t& lodData )
{
	int i = lodData.materialReplacements.AddToTail();
	CLodScriptReplacement_t& newReplacement = lodData.materialReplacements[i];

	// from
	GetToken( false );
	newReplacement.SetSrcName( token );

	// to
	GetToken( false );
	newReplacement.SetDstName( token );
}

//-----------------------------------------------------------------------------
// Parse removemesh command, causes a mesh to not be used anymore
//-----------------------------------------------------------------------------

static void Cmd_RemoveMesh( LodScriptData_t& lodData )
{
	int i = lodData.meshRemovals.AddToTail();
	CLodScriptReplacement_t& newReplacement = lodData.meshRemovals[i];

	// from
	GetToken( false );
	COM_FixSlashes( token );
	newReplacement.SetSrcName( token );
}

static void Cmd_LOD( char const *cmdname )
{
	if ( gflags & STUDIOHDR_FLAGS_HASSHADOWLOD )
	{
		Error( "Model can only have one $shadowlod and it must be the last lod in the .qc (%d) : %s\n", g_iLinecount, g_szLine );
	}

	int i = g_ScriptLODs.AddToTail();
	LodScriptData_t& newLOD = g_ScriptLODs[i];

	if( g_ScriptLODs.Count() > MAX_NUM_LODS )
	{
		Error( "Too many LODs (MAX_NUM_LODS==%d)\n", ( int )MAX_NUM_LODS );
	}

	// Shadow lod has -1 as switch value
	newLOD.switchValue = -1.0f;

	bool isShadowCall = ( !stricmp( cmdname, "$shadowlod" ) ) ? true : false;

	if ( isShadowCall )
	{
		if ( TokenAvailable() )
		{
			GetToken( false );
			printf( "Warning:  (%d) : %s:  Ignoring switch value on %s command line\n", cmdname, g_iLinecount, g_szLine );
		}
		// Disable facial animation by default
		newLOD.EnableFacialAnimation( false );
	}
	else
	{
		if ( TokenAvailable() )
		{
			GetToken( false );
			newLOD.switchValue = atof( token );
			if ( newLOD.switchValue < 0.0f )
			{
				Error( "Negative switch value reserved for $shadowlod (%d) : %s\n", g_iLinecount, g_szLine );
			}
		}
		else
		{
			Error( "Expected LOD switch value (%d) : %s\n", g_iLinecount, g_szLine );
		}
	}

	GetToken( true );
	if( strcmp( "{", token ) != 0 )
	{
		Error( "\"{\" expected while processing %s (%d) : %s", cmdname, g_iLinecount, g_szLine );
	}

	while( 1 )
	{
		GetToken( true );
		if( stricmp( "replacemodel", token ) == 0 )
		{
			Cmd_ReplaceModel(newLOD);
		}
		else if( stricmp( "removemodel", token ) == 0 )
		{
			Cmd_RemoveModel(newLOD);
		}
		else if( stricmp( "replacebone", token ) == 0 )
		{
			Cmd_ReplaceBone( newLOD );
		}
		else if( stricmp( "bonetreecollapse", token ) == 0 )
		{
			Cmd_BoneTreeCollapse( newLOD );
		}
		else if( stricmp( "replacematerial", token ) == 0 )
		{
			Cmd_ReplaceMaterial( newLOD );
		}
		else if( stricmp( "removemesh", token ) == 0 )
		{
			Cmd_RemoveMesh( newLOD );
		}
		else if( stricmp( "nofacial", token ) == 0 )
		{
			newLOD.EnableFacialAnimation( false );
		}
		else if( stricmp( "facial", token ) == 0 )
		{
			newLOD.EnableFacialAnimation( true );
		}
		else if ( stricmp( "use_shadowlod_materials", token ) == 0 )
		{
			if (isShadowCall)
			{
				gflags |= STUDIOHDR_FLAGS_USE_SHADOWLOD_MATERIALS;
			}
		}
		else if( strcmp( "}", token ) == 0 )
		{
			break;
		}
		else
		{
			Error( "invalid input while processing %s (%d) : %s", cmdname, g_iLinecount, g_szLine );
		}
	}
}

void Cmd_ShadowLOD( void )
{
	if (!g_quiet)
	{
		printf( "Processing $shadowlod\n" );
	}

	// Act like it's a regular lod entry
	Cmd_LOD( "$shadowlod" );
	// Mark .mdl as having shadow lod (we also check above that we have only one of these
	//  and that it's the last entered lod )
	gflags |= STUDIOHDR_FLAGS_HASSHADOWLOD;
}

//-----------------------------------------------------------------------------
// A couple commands related to translucency sorting
//-----------------------------------------------------------------------------

void Cmd_Opaque( )
{
	// Force Opaque has precedence
	gflags |= STUDIOHDR_FLAGS_FORCE_OPAQUE;
	gflags &= ~STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS;
}

void Cmd_TranslucentTwoPass( )
{
	// Force Opaque has precedence
	if ((gflags & STUDIOHDR_FLAGS_FORCE_OPAQUE) == 0)
	{
		gflags |= STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS;
	}
}


//-----------------------------------------------------------------------------
// Key value block!
//-----------------------------------------------------------------------------
void Option_KeyValues( CUtlVector< char > *pKeyValue )
{
	// Simply read in the block between { }s as text 
	// and plop it out unchanged into the .mdl file. 
	// Make sure to respect the fact that we may have nested {}s
	int nLevel = 1;

	if ( !GetToken( true ) )
		return;

	if ( token[0] != '{' )
		return;

	AppendKeyValueText( pKeyValue, "mdlkeyvalue\n{\n" );

	while ( GetToken(true) )
	{
		if ( !strcmp( token, "}" ) )
		{
			nLevel--;
			if ( nLevel <= 0 )
				break;
			AppendKeyValueText( pKeyValue, " }\n" );
		}
		else if ( !strcmp( token, "{" ) )
		{
			AppendKeyValueText( pKeyValue, "{\n" );
			nLevel++;
		}
		else
		{
			// tokens inside braces are quoted
			if ( nLevel > 1 )
			{
				AppendKeyValueText( pKeyValue, "\"" );
				AppendKeyValueText( pKeyValue, token );
				AppendKeyValueText( pKeyValue, "\" " );
			}
			else
			{
				AppendKeyValueText( pKeyValue, token );
				AppendKeyValueText( pKeyValue, " " );
			}
		}
	}

	if ( nLevel >= 1 )
	{
		Error( "Keyvalue block missing matching braces.\n" );
	}

	AppendKeyValueText( pKeyValue, "}\n" );
}



/*
=================
=================
*/

void Cmd_ForcedHierarchy( )
{
	// child name
	GetToken (false);
	strcpyn( g_forcedhierarchy[g_numforcedhierarchy].childname, token );

	// parent name
	GetToken (false);
	strcpyn( g_forcedhierarchy[g_numforcedhierarchy].parentname, token );

	g_numforcedhierarchy++;
}


/*
=================
=================
*/

void Cmd_InsertHierarchy( )
{
	// child name
	GetToken (false);
	strcpyn( g_forcedhierarchy[g_numforcedhierarchy].childname, token );

	// subparent name
	GetToken (false);
	strcpyn( g_forcedhierarchy[g_numforcedhierarchy].subparentname, token );

	// parent name
	GetToken (false);
	strcpyn( g_forcedhierarchy[g_numforcedhierarchy].parentname, token );

	g_numforcedhierarchy++;
}


/*
=================
=================
*/

void Cmd_ForceRealign( )
{
	// bone name
	GetToken (false);
	strcpyn( g_forcedrealign[g_numforcedrealign].name, token );

	// skip
	GetToken (false);

	// skip
	GetToken (false);
	g_forcedrealign[g_numforcedrealign].rot.x = DEG2RAD( atof( token ) );

	// skip
	GetToken (false);
	g_forcedrealign[g_numforcedrealign].rot.y = DEG2RAD( atof( token ) );

	// skip
	GetToken (false);
	g_forcedrealign[g_numforcedrealign].rot.z = DEG2RAD( atof( token ) );

	g_numforcedrealign++;
}


/*
=================
=================
*/

void Cmd_LimitRotation( )
{
	// bone name
	GetToken (false);
	strcpyn( g_limitrotation[g_numlimitrotation].name, token );

	while (TokenAvailable())
	{
		// sequence name
		GetToken (false);
		strcpyn( g_limitrotation[g_numlimitrotation].sequencename[g_limitrotation[g_numlimitrotation].numseq++], token );
	}

	g_numlimitrotation++;
}

/*
=================
=================
*/

void Grab_Vertexanimation( s_source_t *psource )
{
	char	cmd[1024];
	int		index;
	Vector	pos;
	Vector	normal;
	int		t = -1;
	int		count = 0;
	static s_vertanim_t	tmpvanim[MAXSTUDIOVERTS*4];

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (sscanf( g_szLine, "%d %f %f %f %f %f %f", &index, &pos[0], &pos[1], &pos[2], &normal[0], &normal[1], &normal[2] ) == 7)
		{
			if (psource->startframe < 0)
			{
				Error( "Missing frame start(%d) : %s", g_iLinecount, g_szLine );
			}

			if (t < 0)
			{
				Error( "VTA Frame Sync (%d) : %s", g_iLinecount, g_szLine );
			}

			tmpvanim[count].vertex = index;
			VectorCopy( pos, tmpvanim[count].pos );
			VectorCopy( normal, tmpvanim[count].normal );
			count++;

			if (index >= psource->numvertices)
				psource->numvertices = index + 1;
		}
		else
		{
			// flush data

			if (count)
			{
				psource->numvanims[t] = count;

				psource->vanim[t] = (s_vertanim_t *)kalloc( count, sizeof( s_vertanim_t ) );

				memcpy( psource->vanim[t], tmpvanim, count * sizeof( s_vertanim_t ) );
			}
			else if (t > 0)
			{
				psource->numvanims[t] = 0;
			}

			// next command
			if (sscanf( g_szLine, "%s %d", cmd, &index ))
			{
				if (strcmp( cmd, "time" ) == 0) 
				{
					t = index;
					count = 0;

					if (t < psource->startframe)
					{
						Error( "Frame Error(%d) : %s", g_iLinecount, g_szLine );
					}
					if (t > psource->endframe)
					{
						Error( "Frame Error(%d) : %s", g_iLinecount, g_szLine );
					}

					t -= psource->startframe;
				}
				else if (strcmp( cmd, "end") == 0) 
				{
					psource->numframes = psource->endframe - psource->startframe + 1;
					return;
				}
				else
				{
					Error( "Error(%d) : %s", g_iLinecount, g_szLine );
				}

			}
			else
			{
				Error( "Error(%d) : %s", g_iLinecount, g_szLine );
			}
		}
	}
	Error( "unexpected EOF: %s\n", psource->filename );
}

int OpenGlobalFile( char *src )
{
	int		time1;
	char	filename[1024];

	strcpy( filename, ExpandPath( src ) );

	int pathLength;
	int numBasePaths = CmdLib_GetNumBasePaths();
	// This is kinda gross. . . doing the same work in cmdlib on SafeOpenRead.
	if( CmdLib_HasBasePath( filename, pathLength ) )
	{
		char tmp[1024];
		int i;
		for( i = 0; i < numBasePaths; i++ )
		{
			strcpy( tmp, CmdLib_GetBasePath( i ) );
			strcat( tmp, filename + pathLength );
			time1 = FileTime( tmp );
			if( time1 != -1 )
			{
				if ((g_fpInput = fopen(tmp, "r")) == 0) 
				{
					fprintf(stderr,"reader: could not open file '%s'\n", src );
					return 0;
				}
				else
				{
					return 1;
				}
			}
		}
		return 0;
	}
	else
	{
		time1 = FileTime (filename);
		if (time1 == -1)
			return 0;

		if ((g_fpInput = fopen(filename, "r")) == 0) 
		{
			fprintf(stderr,"reader: could not open file '%s'\n", src );
			return 0;
		}

		return 1;
	}
}



int Load_VTA( s_source_t *psource )
{
	char	cmd[1024];
	int		option;

	if (!OpenGlobalFile( psource->filename ))
		return 0;

	if (!g_quiet)
		printf ("VTA MODEL %s\n", psource->filename);

	g_iLinecount = 0;
	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		sscanf( g_szLine, "%s %d", cmd, &option );
		if (strcmp( cmd, "version" ) == 0) 
		{
			if (option != 1) 
			{
				Error("bad version\n");
			}
		}
		else if (strcmp( cmd, "nodes" ) == 0) 
		{
			psource->numbones = Grab_Nodes( psource->localBone );
		}
		else if (strcmp( cmd, "skeleton" ) == 0) 
		{
			Grab_Animation( psource );
		}
		else if (strcmp( cmd, "vertexanimation" ) == 0) 
		{
			Grab_Vertexanimation( psource );
		}
		else 
		{
			printf("unknown studio command\n" );
		}
	}
	fclose( g_fpInput );

	is_v1support = true;

	return 1;
}


void Grab_AxisInterpBones( )
{
	char	cmd[1024], tmp[1025];
	Vector	basepos;
	s_axisinterpbone_t *pAxis = NULL;
	s_axisinterpbone_t *pBone = &g_axisinterpbones[g_numaxisinterpbones];

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (IsEnd( g_szLine )) 
		{
			return;
		}
		int i = sscanf( g_szLine, "%s \"%[^\"]\" \"%[^\"]\" \"%[^\"]\" \"%[^\"]\" %d", cmd, pBone->bonename, tmp, pBone->controlname, tmp, &pBone->axis );
		if (i == 6 && stricmp( cmd, "bone") == 0)
		{
			// printf( "\"%s\" \"%s\" \"%s\" \"%s\"\n", cmd, pBone->bonename, tmp, pBone->controlname );
			pAxis = pBone;
			pBone->axis = pBone->axis - 1;	// MAX uses 1..3, engine 0..2
			g_numaxisinterpbones++;
			pBone = &g_axisinterpbones[g_numaxisinterpbones];
		}
		else if (stricmp( cmd, "display" ) == 0)
		{
			// skip all display info
		}
		else if (stricmp( cmd, "type" ) == 0)
		{
			// skip all type info
		}
		else if (stricmp( cmd, "basepos" ) == 0)
		{
			i = sscanf( g_szLine, "basepos %f %f %f", &basepos.x, &basepos.y, &basepos.z );
			// skip all type info
		}
		else if (stricmp( cmd, "axis" ) == 0)
		{
			Vector pos;
			QAngle rot;
			int j;
			i = sscanf( g_szLine, "axis %d %f %f %f %f %f %f", &j, &pos[0], &pos[1], &pos[2], &rot[2], &rot[0], &rot[1] );
			if (i == 7)
			{
				VectorAdd( basepos, pos, pAxis->pos[j] );
				AngleQuaternion( rot, pAxis->quat[j] );
			}
		}
	}
}



void Grab_QuatInterpBones( )
{
	char	cmd[1024];
	Vector	basepos;
	s_quatinterpbone_t *pAxis = NULL;
	s_quatinterpbone_t *pBone = &g_quatinterpbones[g_numquatinterpbones];

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (IsEnd( g_szLine )) 
		{
			return;
		}
		int i = sscanf( g_szLine, "%s %s %s %s %s", cmd, pBone->bonename, pBone->parentname, pBone->controlparentname, pBone->controlname );
		if (i == 5 && stricmp( cmd, "<helper>") == 0)
		{
			// printf( "\"%s\" \"%s\" \"%s\" \"%s\"\n", cmd, pBone->bonename, tmp, pBone->controlname );
			pAxis = pBone;
			g_numquatinterpbones++;
			pBone = &g_quatinterpbones[g_numquatinterpbones];
		}
		else if (stricmp( cmd, "<display>" ) == 0)
		{
			// skip all display info
			Vector size;
			float distance;

			i = sscanf( g_szLine, "<display> %f %f %f %f", 
				&size[0], &size[1], &size[2],
				&distance );

			if (i == 4)
			{
				pAxis->percentage = distance / 100.0;
				pAxis->size = size;
			}
			else
			{
				Error("unable to parse procedual bones\n");
			}
		}
		else if (stricmp( cmd, "<basepos>" ) == 0)
		{
			i = sscanf( g_szLine, "<basepos> %f %f %f", &basepos.x, &basepos.y, &basepos.z );
			// skip all type info
		}
		else if (stricmp( cmd, "<trigger>" ) == 0)
		{
			float tolerance;
			RadianEuler trigger;
			Vector pos;
			RadianEuler ang;

			QAngle rot;
			int j;
			i = sscanf( g_szLine, "<trigger> %f %f %f %f %f %f %f %f %f %f", 
				&tolerance,
				&trigger.x, &trigger.y, &trigger.z,
				&ang.x, &ang.y, &ang.z,
				&pos.x, &pos.y, &pos.z );

			if (i == 10)
			{
				trigger.x = DEG2RAD( trigger.x );
				trigger.y = DEG2RAD( trigger.y );
				trigger.z = DEG2RAD( trigger.z );
				ang.x = DEG2RAD( ang.x );
				ang.y = DEG2RAD( ang.y );
				ang.z = DEG2RAD( ang.z );

				j = pAxis->numtriggers++;
				pAxis->tolerance[j] = DEG2RAD( tolerance );
				AngleQuaternion( trigger, pAxis->trigger[j] );
				VectorAdd( basepos, pos, pAxis->pos[j] );
				AngleQuaternion( ang, pAxis->quat[j] );
			}
			else
			{
				Error("unable to parse procedual bones\n");
			}
		}
		else
		{
			Error("unknown procedual bone data\n");
		}
	}
}


int Load_ProceduralBones( )
{
	char	filename[256];
	char	cmd[1024];
	int		option;

	GetToken( false );
	strcpy( filename, token );

	if (!OpenGlobalFile( filename ))
		return 0;

	g_iLinecount = 0;

	char ext[32];
	ExtractFileExtension( filename, ext, sizeof( ext ) );

	if (stricmp( ext, "vrd") == 0)
	{
		Grab_QuatInterpBones( );
	}
	else
	{
		while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
		{
			g_iLinecount++;
			sscanf( g_szLine, "%s", cmd, &option );
			if (strcmp( cmd, "version" ) == 0) 
			{
				if (option != 1) 
				{
					Error("bad version\n");
				}
			}
			else if (strcmp( cmd, "proceduralbones" ) == 0) 
			{
				Grab_AxisInterpBones( );
			}
		}
	}
	fclose( g_fpInput );

	return 1;
}



/*
===============
ParseScript
===============
*/
void ParseScript (void)
{
	while (1)
	{
		do
		{	// look for a line starting with a $ command
			GetToken (true);
			if (endofscript)
				return;
			if (token[0] == '$')
				break;
			while (TokenAvailable())
				GetToken (false);
		} while (1);
	
		if (!stricmp (token, "$modelname"))
		{
			Cmd_Modelname ();
		}
		else if (!stricmp (token, "$cd"))
		{
			if (cdset)
				Error ("Two $cd in one model");
			cdset = true;
			GetToken (false);
			strcpy (cddir[0], token);
			strcat (cddir[0], "/" );
			numdirs = 0;
		}
		else if (!stricmp (token, "$cdmaterials"))
		{
			while (TokenAvailable())
			{
				GetToken (false);
				char szPath[256];
				strcpy( szPath, token );
				strcat( szPath, "/" );
				COM_FixSlashes( szPath );
				cdtextures[numcdtextures] = strdup( szPath );
				numcdtextures++;
			}
		}
		else if (!stricmp (token, "$pushd"))
		{
			GetToken(false);

			strcpy( cddir[numdirs+1], cddir[numdirs] );
			strcat( cddir[numdirs+1], token );
			strcat( cddir[numdirs+1], "/" );
			numdirs++;
		}
		else if (!stricmp (token, "$popd"))
		{
			if (numdirs > 0)
				numdirs--;
		}

		else if (!stricmp (token, "$scale"))
		{
			Cmd_ScaleUp ();
		}

		else if (!stricmp (token, "$root"))
		{
			Cmd_Root ();
		}
		else if (!stricmp (token, "$controller"))
		{
			Cmd_Controller ();
		}
		else if (!stricmp( token, "$screenalign" ))
		{
			Cmd_ScreenAlign( );
		}
		else if (!stricmp (token, "$model"))
		{
			Cmd_Model();
		}
		else if (!stricmp (token, "$collisionmodel"))
		{
			Cmd_CollisionModel( false );
		}
		else if (!stricmp (token, "$collisionjoints"))
		{
			Cmd_CollisionModel( true );
		}
		else if (!stricmp (token, "$collisiontext"))
		{
			Cmd_CollisionText();
		}
		else if (!stricmp (token, "$body"))
		{
			Cmd_Body();
		}

		else if (!stricmp (token, "$bodygroup"))
		{
			Cmd_Bodygroup();
		}

		else if (!stricmp (token, "$animation" ))
		{
			Cmd_Animation();
		}
		else if (!stricmp (token, "$autocenter" ))
		{
			Cmd_Autocenter();
		}

		/*
		else if (!stricmp (token, "$pose" ))
		{
			Cmd_Pose();
		}
		*/

		else if (!stricmp (token, "$sequence"))
		{
			Cmd_Sequence ();
		}

		else if (!stricmp (token, "$cmdlist"))
		{
			Cmd_Cmdlist ();
		}

		else if (!stricmp (token, "$sequencegroup"))
		{
			Cmd_SequenceGroup ();
		}

		else if (!stricmp (token, "$sequencegroupsize"))
		{
			Cmd_SequenceGroupSize ();
		}

		else if (!stricmp (token, "$weightlist"))
		{
			Cmd_Weightlist();
		}

		else if (!stricmp (token, "$ikchain"))
		{
			Cmd_IKChain();
		}

		else if (!stricmp (token, "$ikautoplaylock"))
		{
			Cmd_IKAutoplayLock();
		}

		else if (!stricmp (token, "$eyeposition"))
		{
			Cmd_Eyeposition ();
		}

		else if (!stricmp (token, "$illumposition"))
		{
			Cmd_Illumposition ();
		}

		else if (!stricmp (token, "$origin"))
		{
			Cmd_Origin ();
		}
		else if (!stricmp (token, "$upaxis"))
		{
			Cmd_UpAxis( );
		}
		else if (!stricmp (token, "$bbox"))
		{
			Cmd_BBox ();
		}
		else if (!stricmp (token, "$cbox"))
		{
			Cmd_CBox ();
		}
		else if (!stricmp (token, "$gamma"))
		{
			Cmd_Gamma ();
		}
		else if (!stricmp (token, "$texturegroup"))
		{
			Cmd_TextureGroup ();
		}
		else if (!stricmp (token, "$hgroup"))
		{
			Cmd_Hitgroup ();
		}
		else if (!stricmp (token, "$hbox"))
		{
			Cmd_Hitbox ();
		}
		else if (!strcmp( token, "$hboxset"))
		{
			Cmd_HitboxSet ();
		}
		else if (!stricmp (token, "$surfaceprop"))
		{
			Cmd_SurfaceProp ();
		}
		else if (!stricmp (token, "$jointsurfaceprop"))
		{
			Cmd_JointSurfaceProp ();
		}
		else if (!stricmp (token, "$contents"))
		{
			Cmd_Contents ();
		}
		else if (!stricmp (token, "$jointcontents"))
		{
			Cmd_JointContents ();
		}
		else if (!stricmp (token, "$attachment"))
		{
			Cmd_Attachment ();
		}
		else if (!stricmp (token, "$externaltextures"))
		{
			split_textures = 1;
		}
		else if (!stricmp (token, "$cliptotextures"))
		{
			clip_texcoords = 1;
		}
		else if (!stricmp (token, "$renamebone"))
		{
			Cmd_Renamebone ();
		}
		else if (!stricmp (token, "$collapsebones"))
		{
			g_collapse_bones = true;
		}
		else if (!stricmp (token, "$alwayscollapse"))
		{
			g_collapse_bones = true;
			GetToken(false);
			g_collapse[g_numcollapse++] = strdup( token );
		}
		else if (!stricmp (token, "$proceduralbones"))
		{
			Load_ProceduralBones( );
		}
		else if (!stricmp (token, "$skiptransition"))
		{
			Cmd_Skiptransition( );
		}
		else if (!stricmp (token, "$staticprop"))
		{
			g_staticprop = true;
			gflags |= STUDIOHDR_FLAGS_STATIC_PROP;
		}
		else if (!stricmp (token, "$realignbones"))
		{
			g_realignbones = true;
		}
		else if (!stricmp (token, "$forcerealign"))
		{
			Cmd_ForceRealign( );
		}
		else if (!stricmp (token, "$lod"))
		{
			Cmd_LOD( "$lod" );
		}
		else if (!stricmp (token, "$shadowlod"))
		{
			Cmd_ShadowLOD();
		}
		else if (!stricmp( token, "$poseparameter" ))
		{
			Cmd_PoseParameter( );
		}
		else if (!stricmp( token, "$heirarchy" ) || !stricmp( token, "$hierarchy" ))
		{
			Cmd_ForcedHierarchy( );
		}
		else if (!stricmp( token, "$insertbone" ))
		{
			Cmd_InsertHierarchy( );
		}
		else if (!stricmp( token, "$limitrotation" ))
		{
			Cmd_LimitRotation( );
		}
		else if (!stricmp( token, "$opaque" ))
		{
			Cmd_Opaque( );
		}
		else if (!stricmp( token, "$mostlyopaque" ))
		{
			Cmd_TranslucentTwoPass( );
		}
		else if (!stricmp( token, "$platform" ))
		{
			Cmd_Platform( );
		}
		else if (!stricmp (token, "$keyvalues"))
		{
			Option_KeyValues( &g_KeyValueText );
		}
		else
		{
			Error ("bad command %s\n", token);
		}
	}
}

/*
==============
main
==============
*/
int main (int argc, char **argv)
{
	int		i;
	char	path[1024];

	InstallSpewFunction();
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false );
	g_currentscale = g_defaultscale = 1.0;
	g_defaultrotation = RadianEuler( 0, 0, M_PI / 2 );

	// skip weightlist 0
	g_numweightlist = 1;

	eyeposition = Vector( 0, 0, 0 );
	gflags = 0;
	numrep = 0;
	tag_reversed = 0;
	tag_normals = 0;
	flip_triangles = 1;
	maxseqgroupsize = 1024 * 1024;

	normal_blend = cos( DEG2RAD( 2.0 ));

	g_gamma = 2.2;

	g_staticprop = false;
	g_centerstaticprop = false;

	g_realignbones = false;

	if (argc == 1)
		Error ("usage: studiomdl [-game gamedir] [-t texture] -r(tag reversed) -n(tag bad normals) -f(flip all triangles) [-a normal_blend_angle] -h(dump hboxes) -i(ignore warnings) -d(dump glview files) [-g max_sequencegroup_size(K)] [-quiet] [-fullcollide(don't truncate really big collisionmodels)] [-checklengths] [-printbones] file.qc");
		
	g_bDumpGLViewFiles = false;
	int gameIdx = -1;
	g_quiet = false;
	for (i = 1; i < argc - 1; i++) 
	{
		if (argv[i][0] == '-') 
		{
			if (!stricmp(argv[i], "-ihvtest"))
			{
				++i;
				g_IHVTest = true;
				continue;
			}

			if (!stricmp(argv[i], "-game"))
			{
				++i;
				gameIdx = i;
				continue;
			}

			if (!stricmp(argv[i], "-quiet"))
			{
				g_quiet = true;
				continue;
			}

			if (!stricmp(argv[i], "-fullcollide"))
			{
				g_badCollide = true;
				continue;
			}

			if (!stricmp(argv[i], "-checklengths"))
			{
				g_bCheckLengths = true;
				continue;
			}

			if (!stricmp(argv[i], "-printbones"))
			{
				g_bPrintBones = true;
				continue;
			}

			if (!stricmp(argv[i], "-perf"))
			{
				g_bPerf = true;
				continue;
			}

			switch( argv[i][1] )
			{
			case 't':
				i++;
				strcpy ( defaulttexture[numrep], argv[i]);
				if (i < argc - 2 && argv[i + 1][0] != '-') {
					i++;
					strcpy ( sourcetexture[numrep], argv[i]);
					printf ("Replacing %s with %s\n", sourcetexture[numrep], defaulttexture[numrep] );
				}
				printf ("Using default texture: %s\n", defaulttexture);
				numrep++;
				break;
			case 'r':
				tag_reversed = 1;
				break;
			case 'n':
				tag_normals = 1;
				break;
			case 'f':
				flip_triangles = 0;
				break;
			case 'a':
				i++;
				normal_blend = cos( DEG2RAD( atof( argv[i] ) ) );
				break;
			case 'h':
				dump_hboxes = 1;
				break;
			case 'g':
				i++;
				maxseqgroupsize = 1024 * atoi( argv[i] );
				break;
			case 'i':
				ignore_warnings = 1;
				break;
			case 'd':
				g_bDumpGLViewFiles = true;
				break;
			case 'p':
				i++;
				strcpy( qproject, argv[i] );
				break;
			}
		}
	}	

	strcpy( g_sequencegroup[g_numseqgroups].label, "default" );
	g_numseqgroups = 1;

//
// load the script
//

	if(gameIdx >= 0)
	{
		strcpy( qproject, argv[gameIdx] );
	}
	strcpy( path, argv[i] );

	CmdLib_InitFileSystem( path, true );

	if (!g_quiet)
		printf("%s, %s, %s\n", qdir, gamedir, basegamedir );

	ExtractFileBase( path, path, sizeof( path ) );
	DefaultExtension (path, ".qc");
	LoadScriptFile (path);

	strcpy( fullpath, path );
	strcpy( fullpath, ExpandPath( fullpath ) );
	strcpy( fullpath, ExpandArg( fullpath ) );
	

	// default to having one entry in the LOD list that doesn't do anything so
	// that we don't have to do any special cases for the first LOD.
	g_ScriptLODs.Purge();
	g_ScriptLODs.AddToTail(); // add an empty one
	g_ScriptLODs[0].switchValue = 0.0f;
	
	//
	// parse it
	//
	ClearModel ();
	strcpy (outname, argv[i]);
	StripExtension (outname);

	strcpy( g_pPlatformName, "" );
	
	ParseScript ();
	
	SetSkinValues ();

	SimplifyModel ();

	ConsistencyCheckSurfaceProp();
	ConsistencyCheckContents();

	BuildCollisionModel ();
	WriteFile ();

	return 0;
}




