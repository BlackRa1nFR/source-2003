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
// write.c: writes a studio .mdl file
//



#pragma warning( disable : 4244 )
#pragma warning( disable : 4237 )
#pragma warning( disable : 4305 )


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib.h"
#include "studio.h"
#include "studiomdl.h"
#include "collisionmodel.h"
#include "optimize.h"
#include "matsys.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"

int totalframes = 0;
float totalseconds = 0;
extern int numcommandnodes;

// WriteFile is the only externally visible function in this file.
// pData points to the current location in an output buffer and pStart is
// the beginning of the buffer.

/*
============
WriteModel
============
*/

static byte *pData;
static byte *pStart;

#define ALIGN4( a ) a = (byte *)((int)((byte *)a + 3) & ~ 3)
#define ALIGN16( a ) a = (byte *)((int)((byte *)a + 15) & ~ 15)
#define ALIGN32( a ) a = (byte *)((int)((byte *)a + 31) & ~ 31)
#define ALIGN64( a ) a = (byte *)((int)((byte *)a + 63) & ~ 63)


void WriteSeqKeyValues( mstudioseqdesc_t *pseqdesc, CUtlVector< char > *pKeyValue );

//-----------------------------------------------------------------------------
// Purpose: stringtable is a session global string table.
//-----------------------------------------------------------------------------

struct stringtable_t
{
	byte	*base;
	int		*ptr;
	char	*string;
	int		dupindex;
	byte	*addr;
};

static int numStrings;
static stringtable_t strings[32768];

static void BeginStringTable(  )
{
	strings[0].base = NULL;
	strings[0].ptr = NULL;
	strings[0].string = "";
	strings[0].dupindex = -1;
	numStrings = 1;
}

//-----------------------------------------------------------------------------
// Purpose: add a string to the file-global string table.
//			Keep track of fixup locations
//-----------------------------------------------------------------------------
static void AddToStringTable( void *base, int *ptr, char *string )
{
	for (int i = 0; i < numStrings; i++)
	{
		if (!string || !strcmp( string, strings[i].string ))
		{
			strings[numStrings].base = (byte *)base;
			strings[numStrings].ptr = ptr;
			strings[numStrings].string = string;
			strings[numStrings].dupindex = i;
			numStrings++;
			return;
		}
	}

	strings[numStrings].base = (byte *)base;
	strings[numStrings].ptr = ptr;
	strings[numStrings].string = string;
	strings[numStrings].dupindex = -1;
	numStrings++;
}

//-----------------------------------------------------------------------------
// Purpose: Write out stringtable
//			fixup local pointers
//-----------------------------------------------------------------------------
static void WriteStringTable( )
{
	// force null at first address
	strings[0].addr = pData;
	*pData = '\0';
	pData++;

	// save all the rest
	for (int i = 1; i < numStrings; i++)
	{
		if (strings[i].dupindex == -1)
		{
			// not in table yet
			// calc offset relative to local base
			*strings[i].ptr = pData - strings[i].base;
			// keep track of address in case of duplication
			strings[i].addr = pData;
			// copy string data, add a terminating \0
			strcpy( (char *)pData, strings[i].string );
			pData += strlen( strings[i].string );
			*pData = '\0';
			pData++;
		}
		else
		{
			// already in table, calc offset of existing string relative to local base
			*strings[i].ptr = strings[strings[i].dupindex].addr - strings[i].base;
		}
	}
	ALIGN4( pData );
}



static void WriteBoneInfo( studiohdr_t *phdr )
{
	int i, j, k;
	mstudiobone_t *pbone;
	mstudiobonecontroller_t *pbonecontroller;
	mstudioattachment_t *pattachment;
	mstudiobbox_t *pbbox;

	// save bone info
	pbone = (mstudiobone_t *)pData;
	phdr->numbones = g_numbones;
	phdr->boneindex = (pData - pStart);

	char* pSurfacePropName = GetDefaultSurfaceProp( );
	AddToStringTable( phdr, &phdr->surfacepropindex, pSurfacePropName );
	phdr->contents = GetDefaultContents();

	for (i = 0; i < g_numbones; i++) 
	{
		AddToStringTable( &pbone[i], &pbone[i].sznameindex, g_bonetable[i].name );
		pbone[i].parent			= g_bonetable[i].parent;
		pbone[i].flags			= g_bonetable[i].flags;
		pbone[i].procindex		= 0;
		pbone[i].physicsbone	= g_bonetable[i].physicsBoneIndex;
		pbone[i].value[0]		= g_bonetable[i].pos[0];
		pbone[i].value[1]		= g_bonetable[i].pos[1];
		pbone[i].value[2]		= g_bonetable[i].pos[2];
		pbone[i].value[3]		= g_bonetable[i].rot[0];
		pbone[i].value[4]		= g_bonetable[i].rot[1];
		pbone[i].value[5]		= g_bonetable[i].rot[2];
		pbone[i].scale[0]		= g_bonetable[i].posscale[0];
		pbone[i].scale[1]		= g_bonetable[i].posscale[1];
		pbone[i].scale[2]		= g_bonetable[i].posscale[2];
		pbone[i].scale[3]		= g_bonetable[i].rotscale[0];
		pbone[i].scale[4]		= g_bonetable[i].rotscale[1];
		pbone[i].scale[5]		= g_bonetable[i].rotscale[2];
		MatrixInvert( g_bonetable[i].boneToPose, pbone[i].poseToBone );
		pbone[i].qAlignment		= g_bonetable[i].qAlignment;

		// TODO: rewrite on version change!!  see studio.h for what to do.
		Assert( STUDIO_VERSION == 36 );
		AngleQuaternion( RadianEuler( g_bonetable[i].rot[0], g_bonetable[i].rot[1], g_bonetable[i].rot[2] ), pbone[i].quat );
		QuaternionAlign( pbone[i].qAlignment, pbone[i].quat, pbone[i].quat );

		pSurfacePropName = GetSurfaceProp( g_bonetable[i].name );
		AddToStringTable( &pbone[i], &pbone[i].surfacepropidx, pSurfacePropName );
		pbone[i].contents = GetContents( g_bonetable[i].name );
	}

	pData += g_numbones * sizeof( mstudiobone_t );
	ALIGN4( pData );

	// save procedural bone info
	if (g_numaxisinterpbones)
	{
		mstudioaxisinterpbone_t *pProc = (mstudioaxisinterpbone_t *)pData;
		for (i = 0; i < g_numaxisinterpbones; i++)
		{
			j = g_axisinterpbonemap[i];
			k = g_axisinterpbones[j].bone;
			pbone[k].procindex		= (byte *)&pProc[i] - (byte *)&pbone[k];
			pbone[k].proctype		= STUDIO_PROC_AXISINTERP;
			// printf("bone %d %d\n", j, pbone[k].procindex );
			pProc[i].control		= g_axisinterpbones[j].control;
			pProc[i].axis			= g_axisinterpbones[j].axis;
			for (k = 0; k < 6; k++)
			{
				VectorCopy( g_axisinterpbones[j].pos[k], pProc[i].pos[k] );
				pProc[i].quat[k] = g_axisinterpbones[j].quat[k];
			}
		}
		pData += g_numaxisinterpbones * sizeof( mstudioaxisinterpbone_t );
		ALIGN4( pData );
	}

	if (g_numquatinterpbones)
	{
		mstudioquatinterpbone_t *pProc = (mstudioquatinterpbone_t *)pData;
		pData += g_numquatinterpbones * sizeof( mstudioquatinterpbone_t );
		ALIGN4( pData );

		for (i = 0; i < g_numquatinterpbones; i++)
		{
			j = g_quatinterpbonemap[i];
			k = g_quatinterpbones[j].bone;
			pbone[k].procindex		= (byte *)&pProc[i] - (byte *)&pbone[k];
			pbone[k].proctype		= STUDIO_PROC_QUATINTERP;
			// printf("bone %d %d\n", j, pbone[k].procindex );
			pProc[i].control		= g_quatinterpbones[j].control;

			mstudioquatinterpinfo_t *pTrigger = (mstudioquatinterpinfo_t *)pData;
			pProc[i].numtriggers	= g_quatinterpbones[j].numtriggers;
			pProc[i].triggerindex	= (byte *)pTrigger - (byte *)&pProc[i];
			pData += pProc[i].numtriggers * sizeof( mstudioquatinterpinfo_t );

			for (k = 0; k < pProc[i].numtriggers; k++)
			{
				pTrigger[k].inv_tolerance	= 1.0 / g_quatinterpbones[j].tolerance[k];
				pTrigger[k].trigger		= g_quatinterpbones[j].trigger[k];
				pTrigger[k].pos			= g_quatinterpbones[j].pos[k];
				pTrigger[k].quat		= g_quatinterpbones[j].quat[k];
			}
		}
	}

	// map g_bonecontroller to bones
	for (i = 0; i < g_numbones; i++) 
	{
		for (j = 0; j < 6; j++)	
		{
			pbone[i].bonecontroller[j] = -1;
		}
	}
	
	for (i = 0; i < g_numbonecontrollers; i++) 
	{
		j = g_bonecontroller[i].bone;
		switch( g_bonecontroller[i].type & STUDIO_TYPES )
		{
		case STUDIO_X:
			pbone[j].bonecontroller[0] = i;
			break;
		case STUDIO_Y:
			pbone[j].bonecontroller[1] = i;
			break;
		case STUDIO_Z:
			pbone[j].bonecontroller[2] = i;
			break;
		case STUDIO_XR:
			pbone[j].bonecontroller[3] = i;
			break;
		case STUDIO_YR:
			pbone[j].bonecontroller[4] = i;
			break;
		case STUDIO_ZR:
			pbone[j].bonecontroller[5] = i;
			break;
		default:
			printf("unknown g_bonecontroller type\n");
			exit(1);
		}
	}


	// save g_bonecontroller info
	pbonecontroller = (mstudiobonecontroller_t *)pData;
	phdr->numbonecontrollers = g_numbonecontrollers;
	phdr->bonecontrollerindex = (pData - pStart);

	for (i = 0; i < g_numbonecontrollers; i++) 
	{
		pbonecontroller[i].bone			= g_bonecontroller[i].bone;
		pbonecontroller[i].inputfield	= g_bonecontroller[i].inputfield;
		pbonecontroller[i].type			= g_bonecontroller[i].type;
		pbonecontroller[i].start		= g_bonecontroller[i].start;
		pbonecontroller[i].end			= g_bonecontroller[i].end;
	}
	pData += g_numbonecontrollers * sizeof( mstudiobonecontroller_t );
	ALIGN4( pData );

	// save attachment info
	pattachment = (mstudioattachment_t *)pData;
	phdr->numattachments = g_numattachments;
	phdr->attachmentindex = (pData - pStart);

	for (i = 0; i < g_numattachments; i++) 
	{
		pattachment[i].bone			= g_attachment[i].bone;
		AddToStringTable( &pattachment[i], &pattachment[i].sznameindex, g_attachment[i].name );
		MatrixCopy( g_attachment[i].local, pattachment[i].local );
	}
	pData += g_numattachments * sizeof( mstudioattachment_t );
	ALIGN4( pData );
	
	// save hitbox sets
	phdr->numhitboxsets = g_hitboxsets.Size();

	// Remember start spot
	mstudiohitboxset_t *hitboxset = (mstudiohitboxset_t *)pData;
	phdr->hitboxsetindex = ( pData - pStart );

	pData += phdr->numhitboxsets * sizeof( mstudiohitboxset_t );
	ALIGN4( pData );

	for ( int s = 0; s < g_hitboxsets.Size(); s++, hitboxset++ )
	{
		s_hitboxset *set = &g_hitboxsets[ s ];

		AddToStringTable( hitboxset, &hitboxset->sznameindex, set->hitboxsetname );

		hitboxset->numhitboxes = set->numhitboxes;
		hitboxset->hitboxindex = ( pData - (byte *)hitboxset );

		// save bbox info
		pbbox = (mstudiobbox_t *)pData;
		for (i = 0; i < hitboxset->numhitboxes; i++) 
		{
			pbbox[i].bone				= set->hitbox[i].bone;
			pbbox[i].group				= set->hitbox[i].group;
			VectorCopy( set->hitbox[i].bmin, pbbox[i].bbmin );
			VectorCopy( set->hitbox[i].bmax, pbbox[i].bbmax );
			pbbox[i].szhitboxnameindex = 0;

			// NJS: Just a cosmetic change, next time the model format is rebuilt, please use the NEXT_MODEL_FORMAT_REVISION.
			// also, do a grep to find the corresponding #ifdefs.
			#ifdef NEXT_MODEL_FORMAT_REVISION
				AddToStringTable( &(pbbox[i]), &(pbbox[i].szhitboxnameindex), set->hitbox[i].hitboxname );	
			#else
				AddToStringTable( phdr, &(pbbox[i].szhitboxnameindex), set->hitbox[i].hitboxname );	
			#endif
		}

		pData += hitboxset->numhitboxes * sizeof( mstudiobbox_t );
		ALIGN4( pData );
	}
}


static void WriteSequenceInfo( studiohdr_t *phdr )
{
	int i, j, k;

	mstudioseqgroup_t	*pseqgroup;
	mstudioseqdesc_t	*pseqdesc;
	mstudioseqdesc_t	*pbaseseqdesc;
	mstudioevent_t		*pevent;
	byte				*ptransition;

	
	// write models to disk with this flag set false. This will force
	// the sequences to be indexed by activity whenever the g_model is loaded
	// from disk.
	phdr->sequencesindexed = false;

	// save g_sequence info
	pseqdesc = (mstudioseqdesc_t *)pData;
	pbaseseqdesc = pseqdesc;
	phdr->numseq = g_numseq;
	phdr->seqindex = (pData - pStart);
	pData += g_numseq * sizeof( mstudioseqdesc_t );

	bool bErrors = false;

	for (i = 0; i < g_numseq; i++, pseqdesc++) 
	{
		byte *pSequenceStart = (byte *)pseqdesc;

		AddToStringTable( pseqdesc, &pseqdesc->szlabelindex, g_sequence[i].name );
		AddToStringTable( pseqdesc, &pseqdesc->szactivitynameindex, g_sequence[i].activityname );

		pseqdesc->flags			= g_sequence[i].flags;

		pseqdesc->numblends		= g_sequence[i].numblends;
		pseqdesc->groupsize[0]	= g_sequence[i].groupsize[0];
		pseqdesc->groupsize[1]	= g_sequence[i].groupsize[1];
		for (j = 0; j < MAXSTUDIOBLENDS; j++)
		{
			for (k = 0; k < MAXSTUDIOBLENDS; k++)
			{
				if (g_sequence[i].panim[j][k])
					pseqdesc->anim[j][k] = g_sequence[i].panim[j][k]->index;
				else
					pseqdesc->anim[j][k] = 0; // !!! bad
			}
		}

		pseqdesc->paramindex[0]	= g_sequence[i].paramindex[0];
		pseqdesc->paramstart[0] = g_sequence[i].paramstart[0];
		pseqdesc->paramend[0]	= g_sequence[i].paramend[0];
		pseqdesc->paramindex[1]	= g_sequence[i].paramindex[1];
		pseqdesc->paramstart[1] = g_sequence[i].paramstart[1];
		pseqdesc->paramend[1]	= g_sequence[i].paramend[1];

		if (g_sequence[i].groupsize[0] > 1 || g_sequence[i].groupsize[1] > 1)
		{
			// save posekey values
			float *pposekey			= (float *)pData;
			pseqdesc->posekeyindex	= (pData - pSequenceStart);
			pData += (pseqdesc->groupsize[0] + pseqdesc->groupsize[1]) * sizeof( float );
			for (j = 0; j < pseqdesc->groupsize[0]; j++)
			{
				*(pposekey++) = g_sequence[i].param0[j];
				// printf("%.2f ", g_sequence[i].param0[j] );
			}
			for (j = 0; j < pseqdesc->groupsize[1]; j++)
			{
				*(pposekey++) = g_sequence[i].param1[j];
				// printf("%.2f ", g_sequence[i].param1[j] );
			}
			// printf("\n" );
		}

		// pseqdesc->motiontype	= g_sequence[i].motiontype;
		// pseqdesc->motionbone	= 0; // g_sequence[i].motionbone;
		// VectorCopy( g_sequence[i].linearmovement, pseqdesc->linearmovement );
		// pseqdesc->seqgroup		= g_sequence[i].seqgroup;

		pseqdesc->activity		= g_sequence[i].activity;
		pseqdesc->actweight		= g_sequence[i].actweight;

		VectorCopy( g_sequence[i].bmin, pseqdesc->bbmin );
		VectorCopy( g_sequence[i].bmax, pseqdesc->bbmax );

		pseqdesc->fadeintime	= g_sequence[i].fadeintime;
		pseqdesc->fadeouttime	= g_sequence[i].fadeouttime;

		pseqdesc->entrynode		= g_sequence[i].entrynode;
		pseqdesc->exitnode		= g_sequence[i].exitnode;
		pseqdesc->entryphase	= g_sequence[i].entryphase;
		pseqdesc->exitphase		= g_sequence[i].exitphase;
		pseqdesc->nodeflags		= g_sequence[i].nodeflags;

		// save events
		pevent					= (mstudioevent_t *)pData;
		pseqdesc->numevents		= g_sequence[i].numevents;
		pseqdesc->eventindex	= (pData - pSequenceStart);
		pData += pseqdesc->numevents * sizeof( mstudioevent_t );
		for (j = 0; j < g_sequence[i].numevents; j++)
		{
			k = g_sequence[i].panim[0][0]->numframes - 1;

			if (g_sequence[i].event[j].frame <= k)
				pevent[j].cycle		= g_sequence[i].event[j].frame / ((float)k);
			else if (k == 0 && g_sequence[i].event[j].frame == 0)
				pevent[j].cycle		= 0;
			else
			{
				printf("Error: event %d (frame %d) out of range in %s\n", g_sequence[i].event[j].event, g_sequence[i].event[j].frame, g_sequence[i].name );
				bErrors = true;
			}

			pevent[j].event		= g_sequence[i].event[j].event;
			// printf("%4d : %d %f\n", pevent[j].event, g_sequence[i].event[j].frame, pevent[j].cycle );
			memcpy( pevent[j].options, g_sequence[i].event[j].options, sizeof( pevent[j].options ) );
		}
		ALIGN4( pData );

		// save ikrules
		pseqdesc->numikrules	= g_sequence[i].numikrules;

		// save autolayers
		mstudioautolayer_t *pautolayer			= (mstudioautolayer_t *)pData;
		pseqdesc->numautolayers	= g_sequence[i].numautolayers;
		pseqdesc->autolayerindex = (pData - pSequenceStart);
		pData += pseqdesc->numautolayers * sizeof( mstudioautolayer_t );
		for (j = 0; j < g_sequence[i].numautolayers; j++)
		{
			pautolayer[j].iSequence = g_sequence[i].autolayer[j].sequence;
			pautolayer[j].flags		= g_sequence[i].autolayer[j].flags;
			pautolayer[j].start		= g_sequence[i].autolayer[j].start / (g_sequence[i].panim[0][0]->numframes - 1);
			pautolayer[j].peak		= g_sequence[i].autolayer[j].peak / (g_sequence[i].panim[0][0]->numframes - 1);
			pautolayer[j].tail		= g_sequence[i].autolayer[j].tail / (g_sequence[i].panim[0][0]->numframes - 1);
			pautolayer[j].end		= g_sequence[i].autolayer[j].end / (g_sequence[i].panim[0][0]->numframes - 1);
		}

		// save boneweights
		float *pweight			= (float *)pData;
		pseqdesc->weightlistindex = (pData - pSequenceStart);
		pData += g_numbones * sizeof( float );
		for (j = 0; j < g_numbones; j++)
		{
			pweight[j] = g_sequence[i].weight[j];
		}

		// save iklocks
		mstudioiklock_t *piklock	= (mstudioiklock_t *)pData;
		pseqdesc->numiklocks		= g_sequence[i].numiklocks;
		pseqdesc->iklockindex		= (pData - pSequenceStart);
		pData += pseqdesc->numiklocks * sizeof( mstudioiklock_t );
		ALIGN4( pData );

		for (j = 0; j < pseqdesc->numiklocks; j++)
		{
			piklock->chain			= g_sequence[i].iklock[j].chain;
			piklock->flPosWeight	= g_sequence[i].iklock[j].flPosWeight;
			piklock->flLocalQWeight	= g_sequence[i].iklock[j].flLocalQWeight;
			piklock++;
		}

		WriteSeqKeyValues( pseqdesc, &g_sequence[i].KeyValue );
	}

	if (bErrors)
	{
		Error( "exiting due to Errors\n");
	}
	// save g_sequence group info
	pseqgroup = (mstudioseqgroup_t *)pData;
	phdr->numseqgroups = g_numseqgroups;
	phdr->seqgroupindex = (pData - pStart);
	pData += phdr->numseqgroups * sizeof( mstudioseqgroup_t );
	ALIGN4( pData );

	for (i = 0; i < g_numseqgroups; i++) 
	{
		AddToStringTable( &pseqgroup[i], &pseqgroup[i].szlabelindex, g_sequencegroup[i].label );
		AddToStringTable( &pseqgroup[i], &pseqgroup[i].sznameindex, g_sequencegroup[i].name );
	}

	// save transition graph
	ptransition	= (byte *)pData;
	phdr->numtransitions = g_numxnodes;
	phdr->transitionindex = (pData - pStart);
	pData += g_numxnodes * g_numxnodes * sizeof( byte );
	ALIGN4( pData );
	for (i = 0; i < g_numxnodes; i++)
	{
//		printf("%2d (%12s) : ", i + 1, g_xnodename[i+1] );
		for (j = 0; j < g_numxnodes; j++)
		{
			*ptransition++ = g_xnode[i][j];
//			printf(" %2d", g_xnode[i][j] );
		}
//		printf("\n" );
	}
}


static byte *WriteAnimations( byte *pData, byte *pStart, int group, studiohdr_t *phdr )
{
	int i, j, k;
	int	n;

	mstudioanimdesc_t	*panimdesc;
	mstudioanim_t		*panim;
	mstudioanimvalue_t	*panimvalue;

	// hack for seqgroup 0
	// pseqgroup->data = (pData - pStart);

	// save animations
	panimdesc = (mstudioanimdesc_t *)pData;
	if( phdr )
	{
		phdr->numanim = g_numani;
		phdr->animdescindex = (pData - pStart);
	}
	pData += g_numani * sizeof( *panimdesc );
	ALIGN4( pData );

	//      ------------ ------- ------- : ------- (-------)
	if( !g_quiet )
	{
		printf("   animation       x       y       ips    angle\n");
	}
	for (i = 0; i < g_numani; i++) 
	{
		
		AddToStringTable( &panimdesc[i], &panimdesc[i].sznameindex, g_panimation[i]->name );

		panimdesc[i].fps		= g_panimation[i]->fps;
		panimdesc[i].flags		= g_panimation[i]->flags;

		totalframes				+= g_panimation[i]->numframes;
		totalseconds			+= g_panimation[i]->numframes / g_panimation[i]->fps;

		panimdesc[i].numframes	= g_panimation[i]->numframes;

		// panimdesc[i].motiontype = g_panimation[i]->motiontype;	
		// panimdesc[i].motionbone = g_panimation[i]->motionbone;
		// VectorCopy( g_panimation[i]->linearpos, panimdesc[i].linearpos );

		j = g_panimation[i]->numpiecewisekeys - 1;
		if (g_panimation[i]->piecewisemove[j].pos[0] != 0 || g_panimation[i]->piecewisemove[j].pos[1] != 0) 
		{
			float t = (g_panimation[i]->numframes - 1) / g_panimation[i]->fps;

			float r = 1 / t;
			
			float a = atan2( g_panimation[i]->piecewisemove[j].pos[1], g_panimation[i]->piecewisemove[j].pos[0] ) * (180 / M_PI);
			float d = sqrt( DotProduct( g_panimation[i]->piecewisemove[j].pos, g_panimation[i]->piecewisemove[j].pos ) );
			if( !g_quiet )
			{
				printf("%12s %7.2f %7.2f : %7.2f (%7.2f) %.1f\n", g_panimation[i]->name, g_panimation[i]->piecewisemove[j].pos[0], g_panimation[i]->piecewisemove[j].pos[1], d * r, a, t );
			}
		}

		// VectorCopy( g_panimation[i]->linearrot, panimdesc[i].linearrot );
		// panimdesc[i].automoveposindex = g_panimation[i]->automoveposindex;
		// panimdesc[i].automoveangleindex = g_panimation[i]->automoveangleindex;

		VectorCopy( g_panimation[i]->bmin, panimdesc[i].bbmin );
		VectorCopy( g_panimation[i]->bmax, panimdesc[i].bbmax );		

		panim					= (mstudioanim_t *)pData;
		panimdesc[i].animindex	= (pData - (byte *)(&panimdesc[i]));
		pData += g_numbones * sizeof( *panim );
		ALIGN4( pData );

		panimvalue					= (mstudioanimvalue_t *)pData;

		// save animation value info
		for (j = 0; j < g_numbones; j++)
		{
			// panim->weight = g_panimation[i]->weight[j];
			// printf( "%s %.1f\n", g_bonetable[j].name, panim->weight );
			panim->flags = 0;

			for (k = 0; k < 6; k++)
			{
				if (g_panimation[i]->numanim[j][k] == 0)
				{
					panim->u.offset[k] = 0;
				}
				else
				{
					panim->u.offset[k] = ((byte *)panimvalue - (byte *)panim);
					for (n = 0; n < g_panimation[i]->numanim[j][k]; n++)
					{
						panimvalue->value = g_panimation[i]->anim[j][k][n].value;
						panimvalue++;
					}
				}
			}

			if (panim->u.offset[0] == 0 && panim->u.offset[1] == 0 && panim->u.offset[2] == 0)
			{
				panim->u.pose.pos[0] = g_panimation[i]->sanim[0][j].pos[0];
				panim->u.pose.pos[1] = g_panimation[i]->sanim[0][j].pos[1];
				panim->u.pose.pos[2] = g_panimation[i]->sanim[0][j].pos[2];
			}
			else
			{
				panim->flags |= STUDIO_POS_ANIMATED;
			}

			if (panim->u.offset[3] == 0 && panim->u.offset[4] == 0 && panim->u.offset[5] == 0)
			{
				Quaternion q;
				AngleQuaternion( g_panimation[i]->sanim[0][j].rot, q );
				QuaternionAlign( g_bonetable[j].qAlignment, q, q );
				panim->u.pose.q[0] = q[0];
				panim->u.pose.q[1] = q[1];
				panim->u.pose.q[2] = q[2];
				panim->u.pose.q[3] = q[3];
			}
			else
			{
				panim->flags |= STUDIO_ROT_ANIMATED;
			}

			// printf("%s:%s %d\n", g_panimation[i]->filename, g_bonetable[j].name, (byte *)panimvalue - (byte *)panim );
			panim++;
		}

		// printf("raw bone data %d : %s\n", (byte *)panimvalue - pData, g_sequence[i].name);
		pData = (byte *)panimvalue;
		ALIGN4( pData );
	}

	// write movement keys
	for (i = 0; i < g_numani; i++) 
	{
		// panimdesc[i].entrancevelocity = g_panimation[i]->entrancevelocity;
		panimdesc[i].nummovements = g_panimation[i]->numpiecewisekeys;
		panimdesc[i].movementindex = (pData - (byte*)&panimdesc[i]);

		mstudiomovement_t	*pmove = (mstudiomovement_t *)pData;
		pData += panimdesc[i].nummovements * sizeof( *pmove );
		ALIGN4( pData );

		for (j = 0; j < panimdesc[i].nummovements; j++)
		{
			pmove[j].endframe		= g_panimation[i]->piecewisemove[j].endframe;
			pmove[j].motionflags	= g_panimation[i]->piecewisemove[j].flags;
			pmove[j].v0				= g_panimation[i]->piecewisemove[j].v0;
			pmove[j].v1				= g_panimation[i]->piecewisemove[j].v1;
			pmove[j].angle			= RAD2DEG( g_panimation[i]->piecewisemove[j].rot[2] );
			VectorCopy( g_panimation[i]->piecewisemove[j].vector, pmove[j].vector );
			VectorCopy( g_panimation[i]->piecewisemove[j].pos, pmove[j].position );

		}
	}

	// write IK error keys
	for (i = 0; i < g_numani; i++) 
	{
		if (g_panimation[i]->numikrules)
		{
			panimdesc[i].numikrules = g_panimation[i]->numikrules;
			panimdesc[i].ikruleindex = (pData - (byte*)&panimdesc[i]);

			mstudioikrule_t *pikrule = (mstudioikrule_t *)pData;
			pData += panimdesc[i].numikrules * sizeof( *pikrule );
			ALIGN4( pData );

			for (j = 0; j < panimdesc[i].numikrules; j++)
			{
				pikrule->index	= g_panimation[i]->ikrule[j].index;

				pikrule->chain	= g_panimation[i]->ikrule[j].chain;
				pikrule->bone	= g_panimation[i]->ikrule[j].bone;
				pikrule->type	= g_panimation[i]->ikrule[j].type;
				pikrule->slot	= g_panimation[i]->ikrule[j].slot;
				pikrule->pos	= g_panimation[i]->ikrule[j].pos;
				pikrule->q		= g_panimation[i]->ikrule[j].q;
				pikrule->height	= g_panimation[i]->ikrule[j].height;
				pikrule->floor	= g_panimation[i]->ikrule[j].floor;
				pikrule->radius = g_panimation[i]->ikrule[j].radius;

				pikrule->start	= g_panimation[i]->ikrule[j].start / (g_panimation[i]->numframes - 1.0f);
				pikrule->peak	= g_panimation[i]->ikrule[j].peak / (g_panimation[i]->numframes - 1.0f);
				pikrule->tail	= g_panimation[i]->ikrule[j].tail / (g_panimation[i]->numframes - 1.0f);
				pikrule->end	= g_panimation[i]->ikrule[j].end / (g_panimation[i]->numframes - 1.0f);

				pikrule->contact= g_panimation[i]->ikrule[j].contact / (g_panimation[i]->numframes - 1.0f);

				/*
				printf("%d %d %d %d : %.2f %.2f %.2f %.2f\n", 
					g_panimation[i]->ikrule[j].start, g_panimation[i]->ikrule[j].peak, g_panimation[i]->ikrule[j].tail, g_panimation[i]->ikrule[j].end, 
					pikrule->start, pikrule->peak, pikrule->tail, pikrule->end );
				*/

				pikrule->iStart = g_panimation[i]->ikrule[j].start;
				pikrule->ikerrorindex = (pData - (byte*)pikrule);

				mstudioikerror_t *perror = (mstudioikerror_t *)pData;
				pData += g_panimation[i]->ikrule[j].numerror * sizeof( *perror );

				for (k = 0; k < g_panimation[i]->ikrule[j].numerror; k++)
				{
					perror[k].pos = g_panimation[i]->ikrule[j].pError[k].pos;
					perror[k].q = g_panimation[i]->ikrule[j].pError[k].q;
				}
				pikrule++;
			}
		}
	}

#if 0
	for (i = 0; i < g_numseq; i++) 
	{
		if (g_sequence[i].seqgroup == group)
		{
			// save animations
			panim					= (mstudioanim_t *)pData;
			g_sequence[i].animindex	= (pData - pStart);
			pData += g_sequence[i].numblends * numbones * sizeof( mstudioanim_t );
			ALIGN4( pData );
			
			panimvalue					= (mstudioanimvalue_t *)pData;
			for (q = 0; q < g_sequence[i].numblends; q++)
			{
				// save animation value info
				for (j = 0; j < numbones; j++)
				{
					panim->weight = 1.0;
					panim->flags = 0;
					for (k = 0; k < 6; k++)
					{
						if (g_sequence[i].panim[q]->numanim[j][k] == 0)
						{
							panim->offset[k] = 0;
						}
						else
						{
							panim->offset[k] = ((byte *)panimvalue - (byte *)panim);
							for (n = 0; n < g_sequence[i].panim[q]->numanim[j][k]; n++)
							{
								panimvalue->value = g_sequence[i].panim[q]->anim[j][k][n].value;
								panimvalue++;
							}
						}
					}
					if (((byte *)panimvalue - (byte *)panim) > 65535)
						Error("g_sequence \"%s\" is greate than 64K\n", g_sequence[i].name );
					panim++;
				}
			}

			// printf("raw bone data %d : %s\n", (byte *)panimvalue - pData, g_sequence[i].name);
			pData = (byte *)panimvalue;
			ALIGN4( pData );
		}
	}
#endif
	return pData;
}
	
static void WriteTextures( studiohdr_t *phdr )
{
	int i, j;
	short	*pref;

	// save texture info
	mstudiotexture_t *ptexture = (mstudiotexture_t *)pData;
	phdr->numtextures = g_nummaterials;
	phdr->textureindex = (pData - pStart);
	pData += g_nummaterials * sizeof( mstudiotexture_t );
	for (i = 0; i < g_nummaterials; i++) 
	{
		j = g_material[i];
		AddToStringTable( &ptexture[i], &ptexture[i].sznameindex, g_texture[j].name );
		// ptexture[i].width		= g_texture[j].width;
		// ptexture[i].height		= g_texture[j].height;
		// ptexture[i].dPdu		= g_texture[j].dPdu;
		// ptexture[i].dPdv		= g_texture[j].dPdv;
	}
	ALIGN4( pData );

	int *cdtextureoffset = (int *)pData;
	phdr->numcdtextures = numcdtextures;
	phdr->cdtextureindex = (pData - pStart);
	pData += numcdtextures * sizeof( int );
	for (i = 0; i < numcdtextures; i++) 
	{
		AddToStringTable( phdr, &cdtextureoffset[i], cdtextures[i] );
	}
	ALIGN4( pData );

	// save texture directory info


	phdr->skinindex = (pData - pStart);
	phdr->numskinref = g_numskinref;
	phdr->numskinfamilies = g_numskinfamilies;
	pref	= (short *)pData;

	for (i = 0; i < phdr->numskinfamilies; i++) 
	{
		for (j = 0; j < phdr->numskinref; j++) 
		{
			*pref = g_skinref[i][j];
			pref++;
		}
	}
	pData = (byte *)pref;
	ALIGN4( pData );

}

static void WriteModel( studiohdr_t *phdr )
{
	int i, j, k, m;

	mstudiobodyparts_t	*pbodypart;
	mstudiomodel_t	*pmodel;
	s_source_t		*psource;


	int				cur;

	// mstudioattachment_t *pattachment;
	mstudiovertanim_t	*pvertanim;
	s_vertanim_t		*pvanim;

	// write bodypart info
	pbodypart = (mstudiobodyparts_t *)pData;
	phdr->numbodyparts = g_numbodyparts;
	phdr->bodypartindex = (pData - pStart);
	pData += g_numbodyparts * sizeof( mstudiobodyparts_t );

	pmodel = (mstudiomodel_t *)pData;
	pData += g_nummodelsbeforeLOD * sizeof( mstudiomodel_t );

	for (i = 0, j = 0; i < g_numbodyparts; i++)
	{
		AddToStringTable( &pbodypart[i], &pbodypart[i].sznameindex, g_bodypart[i].name );
		pbodypart[i].nummodels		= g_bodypart[i].nummodels;
		pbodypart[i].base			= g_bodypart[i].base;
		pbodypart[i].modelindex		= ((byte *)&pmodel[j]) - (byte *)&pbodypart[i];
		j += g_bodypart[i].nummodels;
	}
	ALIGN4( pData );

	// write global flex names
	mstudioflexdesc_t *pflexdesc = (mstudioflexdesc_t *)pData;
	phdr->numflexdesc			= g_numflexdesc;
	phdr->flexdescindex			= (pData - pStart);
	pData += g_numflexdesc * sizeof( mstudioflexdesc_t );
	ALIGN4( pData );

	for (j = 0; j < g_numflexdesc; j++)
	{
		// printf("%d %s\n", j, g_flexdesc[j].FACS );
		AddToStringTable( pflexdesc, &pflexdesc->szFACSindex, g_flexdesc[j].FACS );
		pflexdesc++;
	}

	// write global flex controllers
	mstudioflexcontroller_t *pflexcontroller = (mstudioflexcontroller_t *)pData;
	phdr->numflexcontrollers	= g_numflexcontrollers;
	phdr->flexcontrollerindex	= (pData - pStart);
	pData += g_numflexcontrollers * sizeof( mstudioflexcontroller_t );
	ALIGN4( pData );

	for (j = 0; j < g_numflexcontrollers; j++)
	{
		AddToStringTable( pflexcontroller, &pflexcontroller->sznameindex, g_flexcontroller[j].name );
		AddToStringTable( pflexcontroller, &pflexcontroller->sztypeindex, g_flexcontroller[j].type );
		pflexcontroller->min = g_flexcontroller[j].min;
		pflexcontroller->max = g_flexcontroller[j].max;
		pflexcontroller->link = -1;
		pflexcontroller++;
	}

	// write flex rules
	mstudioflexrule_t *pflexrule = (mstudioflexrule_t *)pData;
	phdr->numflexrules			= g_numflexrules;
	phdr->flexruleindex			= (pData - pStart);
	pData += g_numflexrules * sizeof( mstudioflexrule_t );
	ALIGN4( pData );

	for (j = 0; j < g_numflexrules; j++)
	{
		pflexrule->flex		= g_flexrule[j].flex;
		pflexrule->numops	= g_flexrule[j].numops;
		pflexrule->opindex	= (pData - (byte *)pflexrule);

		mstudioflexop_t *pflexop = (mstudioflexop_t *)pData;

		for (i = 0; i < pflexrule->numops; i++)
		{
			pflexop[i].op = g_flexrule[j].op[i].op;
			pflexop[i].d.index = g_flexrule[j].op[i].d.index;
		}

		pData += sizeof( mstudioflexop_t ) * pflexrule->numops;
		ALIGN4( pData );

		pflexrule++;
	}

	// write ik chains
	mstudioikchain_t *pikchain = (mstudioikchain_t *)pData;
	phdr->numikchains			= g_numikchains;
	phdr->ikchainindex			= (pData - pStart);
	pData += g_numikchains * sizeof( mstudioikchain_t );
	ALIGN4( pData );

	for (j = 0; j < g_numikchains; j++)
	{
		AddToStringTable( pikchain, &pikchain->sznameindex, g_ikchain[j].name );
		pikchain->numlinks		= g_ikchain[j].numlinks;

		mstudioiklink_t *piklink = (mstudioiklink_t *)pData;
		pikchain->linkindex		= (pData - (byte *)pikchain);
		pData += pikchain->numlinks * sizeof( mstudioiklink_t );

		for (i = 0; i < pikchain->numlinks; i++)
		{
			piklink[i].bone = g_ikchain[j].link[i];
		}

		pikchain++;
	}

	// save autoplay locks
	mstudioiklock_t *piklock = (mstudioiklock_t *)pData;
	phdr->numikautoplaylocks	= g_numikautoplaylocks;
	phdr->ikautoplaylockindex	= (pData - pStart);
	pData += g_numikautoplaylocks * sizeof( mstudioiklock_t );
	ALIGN4( pData );

	for (j = 0; j < g_numikautoplaylocks; j++)
	{
		piklock->chain			= g_ikautoplaylock[j].chain;
		piklock->flPosWeight	= g_ikautoplaylock[j].flPosWeight;
		piklock->flLocalQWeight	= g_ikautoplaylock[j].flLocalQWeight;
		piklock++;
	}

	// save mouth info
	mstudiomouth_t *pmouth = (mstudiomouth_t *)pData;
	phdr->nummouths = g_nummouths;
	phdr->mouthindex = (pData - pStart);
	pData += g_nummouths * sizeof( mstudiomouth_t );
	ALIGN4( pData );

	for (i = 0; i < g_nummouths; i++) {
		pmouth[i].bone			= g_mouth[i].bone;
		VectorCopy( g_mouth[i].forward, pmouth[i].forward );
		pmouth[i].flexdesc = g_mouth[i].flexdesc;
	}

	// save pose parameters
	mstudioposeparamdesc_t *ppose = (mstudioposeparamdesc_t *)pData;
	phdr->numposeparameters = g_numposeparameters;
	phdr->poseparamindex = (pData - pStart);
	pData += g_numposeparameters * sizeof( mstudioposeparamdesc_t );
	ALIGN4( pData );

	for (i = 0; i < g_numposeparameters; i++)
	{
		AddToStringTable( &ppose[i], &ppose[i].sznameindex, g_pose[i].name );
		ppose[i].start	= g_pose[i].min;
		ppose[i].end	= g_pose[i].max;
		ppose[i].flags	= g_pose[i].flags;
		ppose[i].loop	= g_pose[i].loop;
	}

	// write model
	cur = (int)pData;
	for (i = 0; i < g_nummodelsbeforeLOD; i++) 
	{
		int n = 0;

		byte *pModelStart = (byte *)(&pmodel[i]);
		
		strcpy( pmodel[i].name, g_model[i]->filename );

		// pmodel[i].mrmbias = g_model[i]->mrmbias;
		// pmodel[i].minresolution = g_model[i]->minresolution;
		// pmodel[i].maxresolution = g_model[i]->maxresolution;

		// save bbox info
		
		psource = g_model[i]->source;

		// save mesh info
		pmodel[i].numvertices = psource->numvertices;
		if( psource->numvertices >= MAXSTUDIOVERTS )
		{
			// We have to check this here so that we don't screw up decal
			// vert caching in the runtime.
			Error( "Too many verts in model. (%d verts, MAXSTUDIOVERTS==%d)\n", 
				psource->numvertices, ( int )MAXSTUDIOVERTS );
		}

		mstudiomesh_t *pmesh = (mstudiomesh_t *)pData;
		pmodel[i].meshindex = (pData - pModelStart);
		pData += psource->nummeshes * sizeof( mstudiomesh_t );
		ALIGN4( pData );

		pmodel[i].nummeshes = psource->nummeshes;
		for (m = 0; m < pmodel[i].nummeshes; m++)
		{
			n = psource->meshindex[m];

			pmesh[m].material		= n;
			pmesh[m].modelindex		= (byte *)&pmodel[i] - (byte *)&pmesh[m];
			pmesh[m].numvertices	= psource->mesh[n].numvertices;
			pmesh[m].vertexoffset	= psource->mesh[n].vertexoffset;
//			pmesh[m].numfaces		= psource->mesh[n].numfaces;
			// pmesh[m].faceindex		= psource->mesh[n].faceoffset;
		}

		// save vertices
		ALIGN64( pData );			
		mstudiovertex_t *pVert = (mstudiovertex_t *)pData;
		pmodel[i].vertexindex		= (pData - pModelStart); 
		pData += pmodel[i].numvertices * sizeof( mstudiovertex_t );
		for (j = 0; j < pmodel[i].numvertices; j++)
		{
//			printf( "saving bone weight %d for model %d at 0x%p\n",
//				j, i, &pbone[j] );

			VectorCopy( psource->vertex[j], pVert[j].m_vecPosition );
			VectorCopy( psource->normal[j], pVert[j].m_vecNormal );
			Vector2DCopy( psource->texcoord[j], pVert[j].m_vecTexCoord );

			mstudioboneweight_t *pBoneWeight = &pVert[j].m_BoneWeights;
			memset( pBoneWeight, 0, sizeof( mstudioboneweight_t ) );
			pBoneWeight->numbones = psource->globalBoneweight[j].numbones;
			for (k = 0; k < pBoneWeight->numbones; k++)
			{
				pBoneWeight->bone[k] = psource->globalBoneweight[j].bone[k];
				pBoneWeight->weight[k] = psource->globalBoneweight[j].weight[k];
			}
		}

		// save tangent space S
		ALIGN4( pData );			
		Vector4D *ptangents = (Vector4D *)pData;
		pmodel[i].tangentsindex		= (pData - pModelStart); 
		pData += pmodel[i].numvertices * sizeof( Vector4D );
		for (j = 0; j < pmodel[i].numvertices; j++)
		{
			Vector4DCopy( psource->tangentS[j], ptangents[j] );
#ifdef _DEBUG
			float w = ptangents[j].w;
			Assert( w == 1.0f || w == -1.0f );
#endif
		}

		if( !g_quiet )
		{
			printf("vertices   %6d bytes (%d vertices)\n", pData - cur, psource->numvertices );
		}
		cur = (int)pData;

#if 0
		// save attachments
		pattachment					= (mstudioattachment_t *)pData;
		pmodel[i].numattachments	= g_model[i]->numattachments;
		pmodel[i].attachmentindex	= (pData - pModelStart);
		pData += pmodel[i].numattachments * sizeof( mstudioattachment_t );
		ALIGN4( pData );
		for (j = 0; j < pmodel[i].numattachments; j++)
		{
			pattachment[j].bone			= g_model[i]->attachment[j].bone;
			VectorCopy( g_model[i]->attachment[j].org, pattachment[j].org );
			VectorCopy( g_model[i]->attachment[j].vector[0], pattachment[j].vector[0] );
			VectorCopy( g_model[i]->attachment[j].vector[1], pattachment[j].vector[1] );
			VectorCopy( g_model[i]->attachment[j].vector[2], pattachment[j].vector[2] );
		}	
#endif

		// save eyeballs
		mstudioeyeball_t *peyeball;
		peyeball					= (mstudioeyeball_t *)pData;
		pmodel[i].numeyeballs		= g_model[i]->numeyeballs;
		pmodel[i].eyeballindex		= (pData - pModelStart);
		pData += g_model[i]->numeyeballs * sizeof( mstudioeyeball_t );
			
		ALIGN4( pData );
		for (j = 0; j < g_model[i]->numeyeballs; j++)
		{
			k = g_model[i]->eyeball[j].mesh;
			pmesh[k].materialtype		= 1;	// FIXME: tag custom material
			pmesh[k].materialparam		= j;	// FIXME: tag custom material

			peyeball[j].bone			= g_model[i]->eyeball[j].bone;
			VectorCopy( g_model[i]->eyeball[j].org, peyeball[j].org );
			peyeball[j].zoffset			= g_model[i]->eyeball[j].zoffset;
			peyeball[j].radius			= g_model[i]->eyeball[j].radius;
			VectorCopy( g_model[i]->eyeball[j].up, peyeball[j].up );
			VectorCopy( g_model[i]->eyeball[j].forward, peyeball[j].forward );
			peyeball[j].iris_material	= g_model[i]->eyeball[j].iris_material;
			peyeball[j].iris_scale		= g_model[i]->eyeball[j].iris_scale;
			peyeball[j].glint_material	= g_model[i]->eyeball[j].glint_material;

			//peyeball[j].upperflex			= g_model[i]->eyeball[j].upperflex;
			//peyeball[j].lowerflex			= g_model[i]->eyeball[j].lowerflex;
			for (k = 0; k < 3; k++)
			{
				peyeball[j].upperflexdesc[k]	= g_model[i]->eyeball[j].upperflexdesc[k];
				peyeball[j].lowerflexdesc[k]	= g_model[i]->eyeball[j].lowerflexdesc[k];
				peyeball[j].uppertarget[k]		= g_model[i]->eyeball[j].uppertarget[k];
				peyeball[j].lowertarget[k]		= g_model[i]->eyeball[j].lowertarget[k];
			}

			peyeball[j].upperlidflexdesc	= g_model[i]->eyeball[j].upperlidflexdesc;
			peyeball[j].lowerlidflexdesc	= g_model[i]->eyeball[j].lowerlidflexdesc;
		}	

		if( !g_quiet )
		{
			printf("faces      %6d bytes (%d faces)\n", pData - cur, psource->numfaces );
		}
		cur = (int)pData;

		// move flexes into individual meshes
		for (m = 0; m < pmodel[i].nummeshes; m++)
		{
			int numflexkeys[MAXSTUDIOFLEXKEYS];
			pmesh[m].numflexes = 0;

			// initialize array
			for (j = 0; j < g_numflexkeys; j++)
			{
				numflexkeys[j] = 0;
			}

			// count flex instances per mesh
			for (j = 0; j < g_numflexkeys; j++)
			{
				if (g_flexkey[j].imodel == i)
				{
					for (k = 0; k < g_flexkey[j].numvanims; k++)
					{
						n = g_flexkey[j].vanim[k].vertex - pmesh[m].vertexoffset;
						if (n >= 0 && n < pmesh[m].numvertices)
						{
							if (numflexkeys[j]++ == 0)
							{
								pmesh[m].numflexes++;
							}
						}
					}
				}
			}

			if (pmesh[m].numflexes)
			{
				pmesh[m].flexindex	= (pData - (byte *)&pmesh[m]);
				mstudioflex_t *pflex = (mstudioflex_t *)pData;
				pData += pmesh[m].numflexes * sizeof( mstudioflex_t );
				ALIGN4( pData );

				for (j = 0; j < g_numflexkeys; j++)
				{
					if (!numflexkeys[j])
						continue;

					pflex->flexdesc		= g_flexkey[j].flexdesc;
					pflex->target0		= g_flexkey[j].target0;
					pflex->target1		= g_flexkey[j].target1;
					pflex->target2		= g_flexkey[j].target2;
					pflex->target3		= g_flexkey[j].target3;
					pflex->numverts		= numflexkeys[j];
					pflex->vertindex	= (pData - (byte *)pflex);
					// printf("%d %d %s : %f %f %f %f\n", j, g_flexkey[j].flexdesc, g_flexdesc[g_flexkey[j].flexdesc].FACS, g_flexkey[j].target0, g_flexkey[j].target1, g_flexkey[j].target2, g_flexkey[j].target3 );
					// if (j < 9) printf("%d %d %s : %d (%d) %f\n", j, g_flexkey[j].flexdesc, g_flexdesc[g_flexkey[j].flexdesc].FACS, g_flexkey[j].numvanims, pflex->numverts, g_flexkey[j].target );

					// printf("%d %d : %d %f\n", j, g_flexkey[j].flexnum, g_flexkey[j].numvanims, g_flexkey[j].target );

					pvertanim				= (mstudiovertanim_t *)pData;
					pData += pflex->numverts * sizeof( mstudiovertanim_t );
					ALIGN4( pData );

					pvanim = g_flexkey[j].vanim;
				
					for (k = 0; k < g_flexkey[j].numvanims; k++)
					{
						n = g_flexkey[j].vanim[k].vertex - pmesh[m].vertexoffset;
						if (n >= 0 && n < pmesh[m].numvertices)
						{
							pvertanim->index	= n;
							VectorCopy( pvanim->pos, pvertanim->delta );
							VectorCopy( pvanim->normal, pvertanim->ndelta );
							pvertanim++;
							// if (j < 9) printf("%d %.2f %.2f %.2f\n", n, pvanim->pos[0], pvanim->pos[1], pvanim->pos[2] );
						}
						// printf("%d %.2f %.2f %.2f\n", pvanim->vertex, pvanim->pos[0], pvanim->pos[1], pvanim->pos[2] );
						pvanim++;
					}
					pflex++;
				}
			}
		}

		if( !g_quiet )
		{
			printf("flexes     %6d bytes (%d flexes)\n", pData - cur, g_numflexkeys );
		}
		cur = (int)pData;
	}
}

#if 0
static void WriteStringTable( studiohdr_t *phdr )
{
	int i;

	phdr->stringtableindex = ( pData - pStart );

	int *pstringtable = (int *)pData;
	phdr->stringtableindex = ( pData - pStart );
	pData += numstrings * sizeof( int );

	for ( i = 0 ; i < numstrings ; i++ )
	{
		pstringtable[i] = ( pData - pStart );
		stringtable[i].offset = ( pData - pStart );
		memcpy( pData, stringtable[ i ].pstring, strlen( stringtable[ i ].pstring ) + 1 );
		pData += strlen( stringtable[ i ].pstring ) + 1;
	}
	ALIGN4( pData );
}
#endif

#define FILEBUFFER (4 * 1024 * 1024)
	

void LoadMaterials( studiohdr_t *phdr )
{
	int					i, j;

	// get index of each material
	if( phdr->textureindex != 0 )
	{
		for( i = 0; i < phdr->numtextures; i++ )
		{
			char szPath[256];
			IMaterial *pMaterial = NULL;
			bool found = false;
			// search through all specified directories until a valid material is found
			for( j = 0; j < phdr->numcdtextures && !found; j++ )
			{
				strcpy( szPath, phdr->pCdtexture( j ) );
				strcat( szPath, phdr->pTexture( i )->pszName( ) );

				pMaterial = g_pMaterialSystem->FindMaterial( szPath, &found, false );
			}
			if( !found )
			{
				// hack - if it isn't found, go through the motions of looking for it again
				// so that the materialsystem will give an error.
				for( j = 0; j < phdr->numcdtextures; j++ )
				{
					strcpy( szPath, phdr->pCdtexture( j ) );
					strcat( szPath, phdr->pTexture( i )->pszName( ) );
					g_pMaterialSystem->FindMaterial( szPath, NULL, true );
				}
			}

			phdr->pTexture( i )->material = pMaterial;

			// FIXME: hack, needs proper client side material system interface
			IMaterialVar *clientShaderVar = pMaterial->FindVar( "$clientShader", &found, false );
			if( found )
			{
				if (stricmp( clientShaderVar->GetStringValue(), "MouthShader") == 0)
				{
					phdr->pTexture( i )->flags = 1;
				}
			}
		}
	}
}


void WriteKeyValues( studiohdr_t *phdr, CUtlVector< char > *pKeyValue )
{
	phdr->keyvalueindex = (pData - pStart);
	phdr->keyvaluesize = KeyValueTextSize( pKeyValue );
	if (phdr->keyvaluesize)
	{
		memcpy(	pData, KeyValueText( pKeyValue ), phdr->keyvaluesize );

		// Add space for a null terminator
		pData[phdr->keyvaluesize] = 0;
		++phdr->keyvaluesize;

		pData += phdr->keyvaluesize * sizeof( char );
	}
	ALIGN4( pData );
}


void WriteSeqKeyValues( mstudioseqdesc_t *pseqdesc, CUtlVector< char > *pKeyValue )
{
	pseqdesc->keyvalueindex = (pData - (byte *)pseqdesc);
	pseqdesc->keyvaluesize = KeyValueTextSize( pKeyValue );
	if (pseqdesc->keyvaluesize)
	{
		memcpy(	pData, KeyValueText( pKeyValue ), pseqdesc->keyvaluesize );

		// Add space for a null terminator
		pData[pseqdesc->keyvaluesize] = 0;
		++pseqdesc->keyvaluesize;

		pData += pseqdesc->keyvaluesize * sizeof( char );
	}
	ALIGN4( pData );
}


void WriteFile (void)
{
	FileHandle_t modelouthandle;
	int			total = 0;
	int			i;
	char		filename[260];
	studiohdr_t *phdr;
	studioseqhdr_t *pseqhdr;

	pStart = (byte *)kalloc( 1, FILEBUFFER );

	StripExtension (outname);

	for (i = 1; i < g_numseqgroups; i++)
	{
		// write the non-default g_sequence group data to separate files
		char groupname[260], localname[260];

		sprintf( groupname, "%s%02d.mdl", outname, i );

		printf ("writing %s:\n", groupname);
		modelouthandle = SafeOpenWrite (groupname);

		pseqhdr = (studioseqhdr_t *)pStart;
		pseqhdr->id = IDSTUDIOSEQHEADER;
		pseqhdr->version = STUDIO_VERSION;

		pData = pStart + sizeof( studioseqhdr_t ); 

		BeginStringTable( );

		pData = WriteAnimations( pData, pStart, i, NULL );

		ExtractFileBase( groupname, localname, sizeof( localname ) );
		sprintf( g_sequencegroup[i].name, "models/%s.mdl", localname );
		strcpy( pseqhdr->name, g_sequencegroup[i].name );
		pseqhdr->length = pData - pStart;

		WriteStringTable( );

		printf("total     %6d\n", pseqhdr->length );

		SafeWrite( modelouthandle, pStart, pseqhdr->length );

		g_pFileSystem->Close(modelouthandle);
		memset( pStart, 0, pseqhdr->length );
	}

//
// write the g_model output file
//
	phdr = (studiohdr_t *)pStart;

	phdr->id = IDSTUDIOHEADER;
	phdr->version = STUDIO_VERSION;

	strcat (outname, ".mdl");
	strcpy( phdr->name, outname );

	// strcpy( outname, ExpandPath( outname ) );

	strcpy( filename, gamedir );
	if( *g_pPlatformName )
	{
		strcat( filename, "platform_" );
		strcat( filename, g_pPlatformName );
		strcat( filename, "/" );
	}
	strcat( filename, "models/" );	
	strcat( filename, outname );	

	if( !g_quiet )
	{
		printf ("---------------------\n");
		printf ("writing %s:\n", filename);
	}
	modelouthandle = SafeOpenWrite (filename);

	VectorCopy( eyeposition, phdr->eyeposition );

	VectorCopy( illumposition, phdr->illumposition );

	if ( !g_wrotebbox )
	{
		VectorCopy( g_sequence[0].bmin, bbox[0] );
		VectorCopy( g_sequence[0].bmax, bbox[1] );
	}
	if ( !g_wrotecbox )
	{
		// no default clipping box, just use per-sequence box
		VectorCopy( vec3_origin, cbox[0] );
		VectorCopy( vec3_origin, cbox[1] );
	}

	VectorCopy( bbox[0], phdr->hull_min ); 
	VectorCopy( bbox[1], phdr->hull_max ); 
	VectorCopy( cbox[0], phdr->view_bbmin ); 
	VectorCopy( cbox[1], phdr->view_bbmax ); 

	phdr->flags = gflags;
	phdr->mass = GetCollisionModelMass();	


	pData = (byte *)phdr + sizeof( studiohdr_t );

	BeginStringTable( );

	WriteBoneInfo( phdr );
	if( !g_quiet )
	{
		printf("bones      %6d bytes (%d)\n", pData - pStart - total, g_numbones );
	}
	total = pData - pStart;

	pData = WriteAnimations( pData, pStart, 0, phdr );

	WriteSequenceInfo( phdr );
	if( !g_quiet )
	{
		printf("sequences  %6d bytes (%d frames) [%d:%02d]\n", pData - pStart - total, totalframes, (int)totalseconds / 60, (int)totalseconds % 60 );
	}
	total  = pData - pStart;

	WriteModel( phdr );
	if( !g_quiet )
	{
		printf("models     %6d bytes\n", pData - pStart - total );
	}
	total  = pData - pStart;

	WriteTextures( phdr );
	if( !g_quiet )
	{
 		printf("textures   %6d bytes\n", pData - pStart - total );
	}
	total  = pData - pStart;

	WriteKeyValues( phdr, &g_KeyValueText );
	if( !g_quiet )
	{
		printf("keyvalues  %6d bytes\n", pData - pStart - total );
	}
	total  = pData - pStart;

	WriteStringTable( );

	total  = pData - pStart;

	phdr->checksum = 0;
	for (i = 0; i < total; i += 4)
	{
		// TODO: does this need something more than a simple shift left and add checksum?
		phdr->checksum = (phdr->checksum << 1) + ((phdr->checksum & 0x8000000) ? 1 : 0) + *((long *)(pStart + i));
	}

	WriteCollisionModel( phdr->checksum );

	if( !g_quiet )
	{
		printf("collision     %6d bytes\n", pData - pStart - total );
	}

	phdr->length = pData - pStart;
	if( !g_quiet )
	{
		printf("total      %6d\n", phdr->length );
	}

	SafeWrite( modelouthandle, pStart, phdr->length );

	g_pFileSystem->Close(modelouthandle);

#ifdef _DEBUG
	int bodyPartID;
	for( bodyPartID = 0; bodyPartID < phdr->numbodyparts; bodyPartID++ )
	{
		mstudiobodyparts_t *pBodyPart = phdr->pBodypart( bodyPartID );
		int modelID;
		for( modelID = 0; modelID < pBodyPart->nummodels; modelID++ )
		{
			mstudiomodel_t *pModel = pBodyPart->pModel( modelID );
			int vertID;
			for( vertID = 0; vertID < pModel->numvertices; vertID++ )
			{
				Vector4D *pTangentS = pModel->TangentS( vertID );
				Assert( pTangentS->w == -1.0f || pTangentS->w == 1.0f );
			}
		}
	}
#endif
	
	// Load materials for this model via the material system so that the
	// optimizer can ask questions about the materials.
	// NOTE: This data won't be included in the mdl file since it is
	// already written.
	char materialDir[256];
	strcpy( materialDir, gamedir );
	strcat( materialDir, "materials" );	
	InitMaterialSystem( materialDir );
	LoadMaterials( phdr );
	OptimizedModel::WriteOptimizedFiles( phdr, g_bodypart );
}



