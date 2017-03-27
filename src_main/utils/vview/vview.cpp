#include "stdafx.h"
#include "glapp.h"
#include <stdio.h>
#include "mathlib.h"
#include "bsprender.h"
#include "cmodel_engine.h"
#include "map_entity.h"
#include "font_gl.h"

extern "C" char g_szAppName[];
extern "C" void AppInit( void );
extern "C" void AppRender( float frametime, float mouseDeltaX, float mouseDeltaY );
extern "C" void AppExit( void );
extern "C" void AppKey( int key, int down );
extern "C" void AppChar( int key );

char g_szAppName[] = "GL BSP Viewer";

static void SetupCamera( Vector& position, Vector& angles );
static void FPSControls( float frametime, float mouseDeltaX, float mouseDeltaY, Vector& cameraPosition, Vector& cameraAngles );
static void TracerCreate( Vector& position, Vector& direction );
static void TracerDraw( void );

struct CameraInfo_t
{
	Vector	position;
	Vector	angles;
	float	speed;
	Vector	forward;
	Vector	right;
	Vector	up;
} g_Camera;

static unsigned char gKeys[256];
#define MAX_KEY_EVENTS	16
struct 
{
	int	list[MAX_KEY_EVENTS];
	int	index;
} g_KeyEvents = { {-1}, 0 };

extern void EngineInit( void );

CFontGL font;
static char g_LastTexture[256];
extern int g_PolyCount;

void AppInit( void )
{
	g_Camera.position.Init(0,0,0);
	g_Camera.angles.Init(0,0,0);
	g_Camera.speed = 1500.0f;

	EngineInit();
	font.Init( "Arial", 14 );

	const char *pMapName = FindParameterArg( "-map" );

	int speed = FindNumParameter( "-speed" );
	if ( speed > 0 )
	{
		g_Camera.speed = (float)speed;
	}

	if ( !pMapName )
	{
		Error("Usage vview -map <mapname.bsp>\n");
		return;
	}
	if ( !BspLoad( pMapName ) )
		return;

	mapentity_t *pEntity = Map_FindEntity( "info_player_start" );
	if ( pEntity )
	{
		VectorCopy( pEntity->origin, g_Camera.position ); 
		VectorCopy( pEntity->angles, g_Camera.angles ); 
	}

	glCullFace( GL_BACK );
	glEnable( GL_CULL_FACE );
	glFrontFace( GL_CW );
	glAlphaFunc( GL_GREATER, 0.25 );
}

void AppRender( float frametime, float mouseDeltaX, float mouseDeltaY )
{
	static float fps = 0;

	if ( frametime > 0 )
	{
		fps = (fps * 0.9) + (0.1f/frametime);
	}

	AngleVectors( g_Camera.angles, &g_Camera.forward, &g_Camera.right, &g_Camera.up );
	FPSControls( frametime, mouseDeltaX, mouseDeltaY, g_Camera.position, g_Camera.angles );
	SetupCamera( g_Camera.position, g_Camera.angles );

	for ( int i = 0; i < MAX_KEY_EVENTS; i++ )
	{
		if ( g_KeyEvents.list[i] < 0 )
			break;
		switch( g_KeyEvents.list[i] )
		{
		case ' ':
			TracerCreate( g_Camera.position, g_Camera.forward );
			break;
		}
		g_KeyEvents.list[i] = -1;
	}
	// Clear the list
	g_KeyEvents.index = 0;

	BspRender( g_Camera.position );
	TracerDraw();

	// draw 2D elements
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	
    glOrtho( 0, WinData.width, WinData.height, 0, -100, 100 );

    // set up a projection matrix to fill the client window
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glDepthMask( 0 );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );

	char fpsText[256];
	sprintf( fpsText, "FPS: %.0f  Texture: %s, POLYS: %4d", fps, g_LastTexture, g_PolyCount );
	font.Print( fpsText, 0, 0, 255, 255, 255 );

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glDepthMask( 1 );
}


void AppExit( void )
{
	BspFree();
}


void AppChar( int key )
{
	if ( g_KeyEvents.index < MAX_KEY_EVENTS )
	{
		g_KeyEvents.list[ g_KeyEvents.index ++ ] = key;
	}

	switch( key )
	{
	case 'v':
		BspToggleVis();
		break;
	case 't':
		BspToggleTexture();
		break;
	case 'p':
		BspTogglePortals();
		break;
	case 'b':
		BspToggleBmodels();
		break;
	case 'l':
		BspToggleLight();
		break;
	}
}


void AppKey( int key, int down )
{
	gKeys[ key & 255 ] = down;
}



// utility routines
void SetupCamera( Vector& position, Vector& angles )
{
	glMatrixMode( GL_MODELVIEW );

    glRotatef( -90, 1, 0, 0 );		// Set up the Half-Life axes
    glRotatef(  90, 0, 0, 1 );
    glRotatef( -angles[2],  1, 0, 0);	// roll
    glRotatef( -angles[0],  0, 1, 0);	// pitch
    glRotatef( -angles[1],  0, 0, 1);	// yaw

	glTranslatef( -position[0], -position[1], -position[2] );
}

void FPSControls( float frametime, float mouseDeltaX, float mouseDeltaY, Vector& cameraPosition, Vector& cameraAngles )
{
	cameraAngles[1] -= mouseDeltaX;
	cameraAngles[0] -= mouseDeltaY;

	if ( cameraAngles[0] < -85 )
		cameraAngles[0] = -85;
	if ( cameraAngles[0] > 85 )
		cameraAngles[0] = 85;

	Vector forward, right, up;
	AngleVectors( cameraAngles, &forward, &right, &up );

	if ( gKeys[ 'W' ] )
		VectorMA( cameraPosition, frametime * g_Camera.speed, forward, cameraPosition );
	if ( gKeys[ 'S' ] )
		VectorMA( cameraPosition, -frametime * g_Camera.speed, forward, cameraPosition );
	if ( gKeys[ 'A' ] )
		VectorMA( cameraPosition, -frametime * g_Camera.speed, right, cameraPosition );
	if ( gKeys[ 'D' ] )
		VectorMA( cameraPosition, frametime * g_Camera.speed, right, cameraPosition );
}


#define GL_fabs(x)		((x)<0?(-(x)):(x))
// Simple routine to visualize a vector
// Assume a unit vector for direction
void GL_DrawVector( Vector& origin, Vector& direction, float size, float arrowSize, int r, int g, int b )
{
	Vector up, right, end, tmp, dot;

	VectorMA( origin, size, direction, end );
	VectorClear( up );

	if ( GL_fabs(direction[2]) > 0.9 )
		up[0] = 1;
	else
		up[2] = 1;

	CrossProduct( direction, up, right );
	glDisable( GL_TEXTURE_2D );
	glColor3ub( r, g, b );
	glBegin( GL_LINES );
	glVertex3fv( origin.Base() );
	glVertex3fv( end.Base() );
	
	VectorMA( end, -arrowSize, direction, tmp );
	VectorMA( tmp, arrowSize, right, dot );

	glVertex3fv( end.Base() );
	glVertex3fv( dot.Base() );

	VectorMA( tmp, -arrowSize, right, dot );
	glVertex3fv( end.Base() );
	glVertex3fv( dot.Base() );
	glEnd();
	glEnable( GL_TEXTURE_2D );
}

void GL_DrawBox( const Vector& origin, const Vector& mins, const Vector& maxs, int r, int g, int b )
{
	Vector start, end;
	int i;

	glDisable( GL_TEXTURE_2D );
	glColor3ub( r, g, b );

	glBegin( GL_LINES );

	for ( i = 0; i < 4; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			int x, y, z;
			x = j;
			y = (j+1)%3;
			z = (j+2)%3;

			VectorCopy( origin, start );
			VectorCopy( origin, end );
			start[x] += mins[x];
			end[x] += maxs[x];
			if ( i < 2 )
			{
				start[y] += mins[y];
				end[y] += mins[y];
			}
			else
			{
				start[y] += maxs[y];
				end[y] += maxs[y];
			}
			if ( i & 1 )
			{
				start[z] += mins[z];
				end[z] += mins[z];
			}
			else
			{
				start[z] += maxs[z];
				end[z] += maxs[z];
			}
			glVertex3fv( start.Base() );
			glVertex3fv( end.Base() );
		}
	}
	glEnd();

	glEnable( GL_TEXTURE_2D );

}


typedef struct tracer_s
{
	unsigned int	die;
	Vector			start;
	Vector			end;
	Vector			direction;
	Vector			normal;
	float			length;
} tracer_t;
#define MAX_TRACERS		32
static tracer_t g_Tracers[MAX_TRACERS];
static int g_TracerIndex = 0;

// set this to the size of the box to visualize
//Vector mins = {-16,-16,-36}, maxs = {16,16,36};
Vector mins(0,0,0), maxs(0,0,0);		// point sized
//Vector mins = {-2,-2,-12}, maxs = {2,2,60};	// tall / thin box

void TracerFire( tracer_t *pTracer );

void TracerCreate( Vector& position, Vector& direction )
{
	tracer_t *pTracer = g_Tracers + g_TracerIndex;

	g_TracerIndex = ( g_TracerIndex + 1 ) % MAX_TRACERS;
	pTracer->die = g_Time + 150000;
	VectorCopy( position, pTracer->start );
	VectorMA( position, 2048, direction, pTracer->end );
	VectorCopy( direction, pTracer->direction );

	VectorSubtract( pTracer->end, pTracer->start, pTracer->direction );
	VectorNormalize( pTracer->direction );
	TracerFire( pTracer );
}


void TracerFire( tracer_t *pTracer )
{	
	// trace against the world
	trace_t hit = CM_BoxTrace( pTracer->start, pTracer->end, mins, maxs, 0, MASK_ALL );

#if 0
	// trace against all brush models
	for ( int i = 1; i < count; i++ )
	{
		cmodel_t *model = CM_InlineModelNumber( i );
		trace_t	tmp = CM_BoxTrace( pTracer->start, pTracer->end, mins, maxs, model->headnode, MASK_ALL );
		if ( tmp.fraction < hit.fraction )
		{
			hit = tmp;
		}
	}
#endif

	VectorCopy( hit.endpos, pTracer->end );
	VectorCopy( hit.plane.normal, pTracer->normal );
	Vector delta;
	VectorSubtract( pTracer->end, pTracer->start, delta );
	pTracer->length = VectorNormalize( delta );

	strcpy( g_LastTexture, hit.surface->name );

#if 0
	// check the texture name
	char tmp[512];
	sprintf( tmp, "Hit %s\n", hit.surface->name );
	OutputDebugString( tmp );
#endif
}

void TracerDraw( void )
{
	for ( int i = 0; i < MAX_TRACERS; i++ )
	{
		tracer_t *pTracer = g_Tracers + i;

		if ( pTracer->die > g_Time )
		{
			GL_DrawVector( pTracer->start, pTracer->direction, pTracer->length, 8, 255, 0, 0 );
			GL_DrawVector( pTracer->end, pTracer->normal, 16, 2, 0, 0, 255 );
			GL_DrawBox( pTracer->end, mins, maxs, 0, 255, 0 );
		}
	}
}
