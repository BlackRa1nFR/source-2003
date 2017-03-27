#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <GL/glu.h>
#include <math.h>
#include <assert.h>

#include "tgaloader.h"
#include "ShaderGL.h"
#include "normalizeCubeMap.h"
#include "nvcombine.h"

#include "mathlib.h"

#include "PrintfXY.h"

int g_windowWidth, g_windowHeight;
bool showHelp = false;

bool leftDown = false;
bool rightDown = false;
bool lastLeftDown = false;
bool lastRightDown = false;

bool drawWireFrame = false;
bool drawBaseTexture = true;
bool drawNormals = false;
bool drawLighting = true;

const char *baseTextureName;
const char *bumpTextureName;

static float normalLen = 1.0f;

int bumpImageWidth, bumpImageHeight;
unsigned char *bumpImageHeightImage;

float bumpScale;

#define QUAD_LIST 1
#define WIRE_LIST 2
#define NORMAL_LIST 3
#define NUM_LISTS 3

#define BASE_TEXTURE 1
#define NORMAL_TEXTURE 2
#define NORMALIZE_CUBEMAP_TEXTURE 3

void CheckGLError( void )
{
	GLenum error;

#if 0
	if( inBeginEndBlock )
	{
		return;
	}
#endif
	
	do
	{
		error = glGetError();
		switch( error )
		{
		case GL_NO_ERROR:
			return;
		case GL_INVALID_ENUM:
			assert( 0 );
			break;
		case GL_INVALID_VALUE:
			assert( 0 );
			break;
		case GL_INVALID_OPERATION:
			assert( 0 );
			break;
		case GL_STACK_OVERFLOW:
			assert( 0 );
			break;
		case GL_STACK_UNDERFLOW:
			assert( 0 );
			break;
		case GL_OUT_OF_MEMORY:
			assert( 0 );
			break;
		default:
			assert( 0 );
		}
	} while( error != GL_NO_ERROR );
}

void DrawLightSource( float *lightPosition, float r, float g, float b )
{
	glPushMatrix();
	glTranslatef( lightPosition[0], lightPosition[1], lightPosition[2] );
	glColor3f( r, g, b );
	glutSolidSphere( 2.0f, 10, 10 );
	glPopMatrix();
}

void LoadHeightMap( const char *tgaFileName )
{
	enum ImageFormat srcImageFormat;
	float dummyGamma;
		
	if( !TGALoader::GetInfo( tgaFileName, &bumpImageWidth, &bumpImageHeight, &srcImageFormat, &dummyGamma ) )
	{
		assert( 0 );
	}

	int memSize;
	memSize = ImageLoader::GetMemRequired( bumpImageWidth, bumpImageHeight, IMAGE_FORMAT_I8, false );
	bumpImageHeightImage = new unsigned char[memSize];
	assert( bumpImageHeightImage );
	
	if( !TGALoader::Load( bumpImageHeightImage, tgaFileName, bumpImageWidth, bumpImageHeight, IMAGE_FORMAT_I8,
		1.0f, false ) )
	{
		assert( 0 );
	}
}

void BuildPosition( vec3_t position, int x, int y )
{
	position[0] = x * 0.25f;
	position[1] = y * 0.25f;
	position[2] = bumpImageHeightImage[x + bumpImageWidth * y] * bumpScale;
}

void BuildNormal( vec3_t normal, int x, int y )
{
	vec3_t position, xvec, yvec, tmpPos;

	position[0] = x * 0.25f;
	position[1] = y * 0.25f;
	position[2] = bumpImageHeightImage[x + bumpImageWidth * y] * bumpScale;

	tmpPos[0] = ( x + 1 ) * 0.25f;
	tmpPos[1] = y * 0.25f;
	tmpPos[2] = bumpImageHeightImage[(x+1) + bumpImageWidth * y] * bumpScale;

	VectorSubtract( tmpPos, position, xvec );

	tmpPos[0] = x * 0.25f;
	tmpPos[1] = (y + 1) + 0.25f;
	tmpPos[2] = bumpImageHeightImage[x + bumpImageWidth * (y+1)] * bumpScale;

	VectorSubtract( tmpPos, position, yvec );

	CrossProduct( xvec, yvec, normal );
	VectorNormalize( normal );
}

void BuildNormalTexture( void )
{
	int x, y;
	vec3_t normal;
	unsigned char *imageData;
	unsigned char *texel;

	imageData = new unsigned char[bumpImageWidth * bumpImageHeight * 4];
	assert( imageData );
	
	for( y = 0; y < bumpImageHeight; y++ )
	{
		for( x = 0; x < bumpImageWidth; x++ )
		{
			texel = &imageData[( x + ( y * bumpImageWidth ) ) * 4];
			BuildNormal( normal, x, y );
			texel[0] = ( unsigned char )( 255.0f * ( ( normal[0] + 1.0f ) * 0.5f ) );
			texel[1] = ( unsigned char )( 255.0f * ( ( normal[1] + 1.0f ) * 0.5f ) );
			texel[2] = ( unsigned char )( 255.0f * ( ( normal[2] + 1.0f ) * 0.5f ) );
			texel[3] = 255; // mag.
		}
	}

#if 0
	FILE *fp;
	fp = fopen( "bumpcolor.ppm", "wb" );
	fprintf( fp, "P6\n%d %d\n255\n", bumpImageWidth, bumpImageHeight );
	int i;
	for( i = 0; i < bumpImageWidth * bumpImageHeight; i++ )
	{
		fwrite( &imageData[i * 4], 1, 3, fp );
	}
	fclose( fp );
#endif

	CheckGLError();
	glBindTexture( GL_TEXTURE_2D, NORMAL_TEXTURE );
	//	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, bumpImageWidth, bumpImageHeight, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, imageData );
	
	CheckGLError();

	delete [] imageData;
}

void BuildDisplayLists( void )
{
	int x, y;
	vec3_t normal;
	vec3_t position;

	glNewList( QUAD_LIST, GL_COMPILE );

	for( y = 0; y < bumpImageHeight-1; y++ )
	{
		glBegin( GL_QUAD_STRIP );
		for( x = 0; x < bumpImageWidth; x++ )
		{
			BuildNormal( normal, x, y );
			glNormal3fv( normal );
			glTexCoord2f( x / ( float )bumpImageWidth, y / ( float ) bumpImageHeight );
			BuildPosition( position, x, y );
			glVertex3fv( position );

			BuildNormal( normal, x, y+1 );
			glNormal3fv( normal );
			glTexCoord2f( x / ( float )bumpImageWidth, (y+1) / ( float ) bumpImageHeight );
			BuildPosition( position, x, y+1 );
			glVertex3fv( position );
		}
		glEnd();
	}

	glEndList();

	glNewList( WIRE_LIST, GL_COMPILE );

	for( y = 0; y < bumpImageHeight; y++ )
	{
		glBegin( GL_LINE_STRIP );
		for( x = 0; x < bumpImageWidth; x++ )
		{
			BuildNormal( normal, x, y );
			glNormal3fv( normal );
			glTexCoord2f( x / ( float )bumpImageWidth, y / ( float ) bumpImageHeight );
			BuildPosition( position, x, y );
			glVertex3fv( position );
		}
		glEnd();
	}

	for( x = 0; x < bumpImageWidth; x++ )
	{
		glBegin( GL_LINE_STRIP );
		for( y = 0; y < bumpImageHeight; y++ )
		{
			BuildNormal( normal, x, y );
			glNormal3fv( normal );
			glTexCoord2f( x / ( float )bumpImageWidth, y / ( float ) bumpImageHeight );
			BuildPosition( position, x, y );
			glVertex3fv( position );
		}
		glEnd();
	}

	glEndList();

#if 0
	glNewList( NORMAL_LIST, GL_COMPILE );

	glBegin( GL_LINES );
	for( y = 0; y < bumpImageHeight; y++ )
	{
		for( x = 0; x < bumpImageWidth; x++ )
		{
			BuildNormal( normal, x, y );
			glNormal3fv( normal );

			vec3_t position;
			BuildPosition( position, x, y );
			glVertex3fv( position );
			VectorScale( normal, normalLen, normal );
			VectorAdd( normal, position, position );
			glVertex3fv( position );
		}
	}
	glEnd();

	glEndList();
#endif
}

static void Push2DMode( void )
{
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_CUBE_MAP_EXT);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, g_windowWidth, 0, g_windowHeight );

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_REGISTER_COMBINERS_NV);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

static void Pop2DMode( void )
{
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable( GL_DEPTH_TEST );
}

void MouseFunc( int button, int state, int x, int y )
{
	if( button == GLUT_LEFT_BUTTON )
	{
		lastLeftDown = leftDown;
		if( state == GLUT_UP )
		{
			leftDown = false;
		}
		else
		{
			leftDown = true;
		}
	}
	else if( button = GLUT_RIGHT_BUTTON )
	{
		lastRightDown = rightDown;
		if( state == GLUT_UP )
		{
			rightDown = false;
		}
		else
		{
			rightDown = true;
		}
	}
}

void MotionFunc( int x, int y )
{
	static int prevX, prevY;
	static bool firstTime = true;

	if( firstTime )
	{
		firstTime = false;
		prevX = x;
		prevY = y;
		return;
	}

	float dx = x - prevX;
	float dy = y - prevY;
	prevX = x;
	prevY = y;
	
	float oldMatrix[16];
	glMatrixMode( GL_MODELVIEW );
	glGetFloatv( GL_MODELVIEW_MATRIX, oldMatrix );
	glLoadIdentity();

	if( rightDown )
	{
		if( leftDown )
		{
			glTranslatef( -dx, 0.0f, -dy );
		}
		else
		{
			glTranslatef( -dx, dy, 0.0f );
		}

	}
	else if( leftDown )
	{
		glRotatef( dx, 0.0f, 1.0f, 0.0f );
		glRotatef( dy, 1.0f, 0.0f, 0.0f );
	}
	glMultMatrixf( oldMatrix );
	
	glutPostRedisplay();
}

void ReshapeFunc( int width, int height )
{
	glViewport(0, 0, width, height );
	g_windowWidth = width;
	g_windowHeight = height;

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 90.0f * ( float )height / ( float )width, ( float )width / ( float )height, 1.0f, 1000.0f );

	glMatrixMode( GL_MODELVIEW );
}

bool LoadTexture( const char *fileName, bool isBump, float bumpScale )
{
	unsigned char *imageData;
	int width, height;

	enum ImageFormat srcImageFormat;
	float dummyGamma;
		
	if( !TGALoader::GetInfo( fileName, &width, &height, &srcImageFormat, &dummyGamma ) )
	{
		assert( 0 );
	}

	int memSize;
	memSize = ImageLoader::GetMemRequired( width, height, IMAGE_FORMAT_RGB888, false );
	imageData = new unsigned char[memSize];
	assert( imageData );
	
	if( !TGALoader::Load( imageData, fileName, width, height, IMAGE_FORMAT_RGB888,
		1.0f, false ) )
	{
		assert( 0 );
	}

	if( !isBump )
	{
		// download a regular old texture
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, imageData );
	}
	else
	{
		assert( 0 );
#if 0
		LoadTextureNormalMap( imageData, width, height, bumpScale );
#endif
	}
	delete [] imageData;

	return true;
}

void SetLight( vec3_t pos )
{
	float lightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float white[] = { 10000.0f, 10000.0f, 10000.0f, 1.0f };

	VectorCopy( pos, lightPosition );
	
	glShadeModel( GL_SMOOTH );
	glLightfv( GL_LIGHT0, GL_POSITION, lightPosition );
	glLightfv( GL_LIGHT0, GL_DIFFUSE, white );

	glLightf( GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.0f );
	glLightf( GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0f );
	glLightf( GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 1.0f );
	
	glEnable( GL_LIGHTING );
	glEnable( GL_LIGHT0 );
}

void IdleFunc( void )
{
	glutPostRedisplay();
}

void LoadData( void )
{
	glDeleteLists( 1, NUM_LISTS );
	
	if( glIsTexture( BASE_TEXTURE ) )
	{
		GLuint textureNum = BASE_TEXTURE;
		glDeleteTextures( 1, &textureNum );
	}
	
	if( glIsTexture( NORMAL_TEXTURE ) )
	{
		GLuint textureNum = NORMAL_TEXTURE;
		glDeleteTextures( 1, &textureNum );
	}
	
	LoadHeightMap( bumpTextureName );

	glBindTexture( GL_TEXTURE_2D, BASE_TEXTURE );
	LoadTexture( baseTextureName, false, 1.0f );
	
	BuildNormalTexture();
	BuildDisplayLists();
}

void KeyboardFunc( unsigned char key, int x, int y )
{
	switch( key )
	{
	case 'h':
		showHelp = !showHelp;
		break;
	case 'w':
		drawWireFrame = !drawWireFrame;
		break;
	case 't':
		drawBaseTexture = !drawBaseTexture;
		break;
	case 'l':
		drawLighting = !drawLighting;
		break;
#if 0
	case 'n':
		drawNormals = !drawNormals;
		break;
#endif
	case 'r':
		LoadData();
		break;
	case 'q':
		exit( 0 );
		break;
	}
}

static void DrawStatus( void )
{
	static int fontHeight = 15;
	int yPos = g_windowHeight - fontHeight;
	Push2DMode();
	glColor3f( 1.0f, 1.0f, 1.0f );

	if( !showHelp )
	{
		PrintfXY( 0, yPos, "hit 'h' for help" );
	}
	else
	{
		PrintfXY( 0, yPos, "hit 'h' to hide help" );
		yPos -= fontHeight;
		PrintfXY( 0, yPos, "'w': toggle wireframe" );
		yPos -= fontHeight;
		PrintfXY( 0, yPos, "'t': toggle base texture" );
		yPos -= fontHeight;
		PrintfXY( 0, yPos, "'l': toggle lighting" );
		yPos -= fontHeight;
#if 0
		PrintfXY( 0, yPos, "'n': toggle normals" );
		yPos -= fontHeight;
#endif
		PrintfXY( 0, yPos, "'r': reload data" );
		yPos -= fontHeight;
		PrintfXY( 0, yPos, "'q': reload data" );
		yPos -= fontHeight;
	}
	Pop2DMode();
}

void RedrawFunc( void )
{
	// all time in milliseconds
	int elapsedTime = glutGet( GLUT_ELAPSED_TIME );
	static int prevElapsedTime = 0;
	static float timeAccum = 0.0;
	int deltaTime = elapsedTime - prevElapsedTime;

	vec3_t lightPosition = { 0.0f, 0.0f, 60.0f };
	
	prevElapsedTime = elapsedTime;
	
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glDepthFunc( GL_LEQUAL );
	glEnable( GL_DEPTH_TEST );

	if( drawLighting )
	{
		SetLight( lightPosition );
	}
	
	if( drawBaseTexture )
	{
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, BASE_TEXTURE );
//		glBindTexture( GL_TEXTURE_2D, NORMAL_TEXTURE );
	}
	else
	{
		glDisable( GL_TEXTURE_2D );
	}
	
	if( drawWireFrame )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glCallList( WIRE_LIST );
	}
	else
	{
		glCallList( QUAD_LIST );
	}

	glDisable( GL_TEXTURE_2D );
	glDisable( GL_LIGHTING );

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if( drawNormals )
	{
		glColor3f( 0.0f, 1.0f, 0.0f );
		glCallList( NORMAL_LIST );
		glColor3f( 1.0f, 1.0f, 1.0f );
	}

	DrawLightSource( lightPosition, 1.0f, 1.0f, 1.0f );
	
	DrawStatus();
	
	glutSwapBuffers();
	glFinish();
}

int main( int argc, char **argv )
{
	if( argc != 4 )
	{
		fprintf( stderr, "Usage: %s texture bumpmap bumpscale\n", argv[0] );
		exit( -1 );
	}
	
	baseTextureName = argv[1];
	bumpTextureName = argv[2];
	bumpScale = atof( argv[3] ) / 255.0f;

	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA );
	glutCreateWindow( "Valve Bumpmap Viewer" );
	glutDisplayFunc( RedrawFunc );
	glutMouseFunc( MouseFunc );
	glutKeyboardFunc( KeyboardFunc );
	glutMotionFunc( MotionFunc );
	glutPassiveMotionFunc( MotionFunc );
    glutReshapeFunc( ReshapeFunc );
	glutIdleFunc( IdleFunc );

	InitGL();

	LoadData();
	
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 90.0f, 1.0f, 1.0f, 1000.0f );
	
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

//	glutFullScreen();
	
	glutMainLoop();
	return 0;
}
