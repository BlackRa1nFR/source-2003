// Begin BonesPro.H file
#ifndef _bonespro_h_
#define _bonespro_h_

#include "max.h"

// PROPEPRTY ID ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#define BP_CLASS_ID_OSM Class_ID(0x37e26e45, 0x543b6b2a)
#define BP_CLASS_ID_WSM Class_ID(0x37e26e45, 0x543b6a2a)

#define BP_TIME_ATTACHED TimeValue(0x80000000) 
#define BP_PROPID        (PROPID_USER+0xFF)

#define BP_PROPID_GET_N_BONES        (BP_PROPID + 0)
#define BP_PROPID_GET_N_VERTS        (BP_PROPID + 1)
#define BP_PROPID_GET_WEIGHTS        (BP_PROPID + 2)
#define BP_PROPID_GET_BONE           (BP_PROPID + 3)
#define BP_PROPID_GET_BONE_STAT      (BP_PROPID + 4)
#define BP_PROPID_GET_BONE_BY_NAME   (BP_PROPID + 5)
#define BP_PROPID_SET_BONE_FALLOFF   (BP_PROPID + 6)
#define BP_PROPID_SET_BONE_STRENGTH  (BP_PROPID + 7)
#define BP_PROPID_REFRESH            (BP_PROPID + 8)
#define BP_PROPID_SET_BONE_MARK      (BP_PROPID + 9)
#define BP_PROPID_GET_VERT_SEL       (BP_PROPID + 10)
#define BP_PROPID_SET_VERT_SEL       (BP_PROPID + 11)
#define BP_PROPID_SET_VERT_SEL_ANI   (BP_PROPID + 12)
#define BP_PROPID_SET_BONE_SEL_ANI   (BP_PROPID + 13)
#define BP_PROPID_GET_BV             (BP_PROPID + 14)
#define BP_PROPID_SET_BV             (BP_PROPID + 15)

#define BP_PROPID_END                (BP_PROPID + 50)


#pragma pack( push, before_bonespro_h )
#pragma pack(1)

typedef struct 
{
  int   nb;
  int   nv;
  float w[1];
} BonesPro_WeightArray;

typedef struct
{
  TimeValue t; 
  int       index; 
  char      name [256]; 
  INode*    node; 
  float     matrix [4][3]; 
  float     scale [3]; 
  float     falloff; 
  float     strength; 
  int       marked; 
} BonesPro_Bone;

typedef struct
{
  int index;
  int selected;
} BonesPro_Vertex;

typedef struct
{
  int bindex;
  int vindex;
  int included;
  int forced_weight;
} BonesPro_BoneVertex;

#pragma pack( pop, before_bonespro_h )

#endif
// End BonesPro.H file

