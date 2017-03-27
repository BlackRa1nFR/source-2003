#include <windows.h>
#include <gl/gl.h>
#include <math.h>
#include <assert.h>
#include "ShaderGL.h"


/*** NORMALIZATION CUBE MAP CONSTRUCTION ***/

/* Given a cube map face index, cube map size, and integer 2D face position,
 * return the cooresponding normalized vector.
 */
void
GetCubeVector(int i, int cubesize, int x, int y, float *vector)
{
  float s, t, sc, tc, mag;

  s = ((float)x + 0.5f) / (float)cubesize;
  t = ((float)y + 0.5f) / (float)cubesize;
  sc = s*2.0f - 1.0f;
  tc = t*2.0f - 1.0f;

  switch (i) {
  case 0:
    vector[0] = 1.0;
    vector[1] = -tc;
    vector[2] = -sc;
    break;
  case 1:
    vector[0] = -1.0;
    vector[1] = -tc;
    vector[2] = sc;
    break;
  case 2:
    vector[0] = sc;
    vector[1] = 1.0;
    vector[2] = tc;
    break;
  case 3:
    vector[0] = sc;
    vector[1] = -1.0;
    vector[2] = -tc;
    break;
  case 4:
    vector[0] = sc;
    vector[1] = -tc;
    vector[2] = 1.0;
    break;
  case 5:
    vector[0] = -sc;
    vector[1] = -tc;
    vector[2] = -1.0;
    break;
  }

  mag = 1.0f/( float )sqrt(vector[0]*vector[0] + vector[1]*vector[1] + vector[2]*vector[2]);
  vector[0] *= mag;
  vector[1] *= mag;
  vector[2] *= mag;
}

/* Initialize a cube map texture object that generates RGB values
 * that when expanded to a [-1,1] range in the register combiners
 * form a normalized vector matching the per-pixel vector used to
 * access the cube map.
 */
void MakeNormalizeVectorCubeMap( int size )
{
  float vector[3];
  int i, x, y;
  GLubyte *pixels;

  pixels = (GLubyte*) malloc(size*size*3);
  assert( pixels );
  
  glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  for (i = 0; i < 6; i++) {
    for (y = 0; y < size; y++) {
      for (x = 0; x < size; x++) {
        GetCubeVector(i, size, x, y, vector);
        pixels[3*(y*size+x) + 0] = ( unsigned char )( 128 + 127*vector[0] );
        pixels[3*(y*size+x) + 1] = ( unsigned char )( 128 + 127*vector[1] );
        pixels[3*(y*size+x) + 2] = ( unsigned char )( 128 + 127*vector[2] );
      }
    }
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT+i, 0, GL_RGB8,
      size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
  }

  free(pixels);
}
