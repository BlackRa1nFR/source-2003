//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "basetypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <conio.h>


typedef struct OctTreeNode_s
{
	bool isLeaf;
	int timesUsed;
	float cumulativeColor[3];
	int paletteIndex;
	struct OctTreeNode_s *children[8];
	struct OctTreeNode_s *parent;
	// next in the priority list for this level of the tree.
	// also next for the free list.
	struct OctTreeNode_s *next; 

} OctTreeNode_t;

#define MAX_PAL_SIZE 256
//#define MAX_NUM_NODES ( MAX_PAL_SIZE + ( MAX_PAL_SIZE - 1 ) )
#define MAX_NUM_NODES ( MAX_PAL_SIZE * 8 + 1 )

static int *g_frequencyCounts = NULL;

static OctTreeNode_t nodePool[MAX_NUM_NODES];
static int nextFreeID;
static OctTreeNode_t *freeList;
static OctTreeNode_t *root;

static void PrintPriorityQueues( void );
static void PrintColorsInTree( OctTreeNode_t *node, int depth, 
					    unsigned int r, unsigned int g, unsigned int b );
static void PrintTree( OctTreeNode_t *node, int depth );
static void PrintNode( OctTreeNode_t *node );

static int numColorsUsed;
static int desiredNumColors;
static unsigned char insertColor[3];

static int currNumPalIndices;
static unsigned char *pal;

static int debugNumNodes = 0;

// not prioritized for now.
static OctTreeNode_t *priorityQueues[9];

static OctTreeNode_t *AllocNode()
{
	debugNumNodes++;
	OctTreeNode_t *node;
	if( nextFreeID < MAX_NUM_NODES )
	{
		node = &nodePool[nextFreeID];
		nextFreeID++;
	}
	else
	{
		node = freeList;
		if( !node )
		{
			PrintTree( root, 0 );
			PrintColorsInTree( root, 0, 0, 0, 0 );
			getch();
			assert( node );
		}
		freeList = freeList->next;
	}
	memset( node, 0, sizeof( OctTreeNode_t ) );
	return node;
}

static void FreeNode( OctTreeNode_t *node )
{
	debugNumNodes--;
	node->next = freeList;
	freeList = node;
}

static void InitNodeAllocation( void )
{
	freeList = NULL;
	nextFreeID = 0;
}

static void RemoveFromPriorityQueue( int level, OctTreeNode_t *node )
{
	OctTreeNode_t *searchNode, *prev;
	OctTreeNode_t dummy;
	dummy.next = priorityQueues[level];
	
#if 0
	printf( "\nRemoveFromPriorityQueue:\n" );
	PrintNode( node );
#endif
	
	prev = &dummy;
	for( searchNode = dummy.next; searchNode; searchNode = searchNode->next )
	{
		if( searchNode == node )
		{
			prev->next = node->next;
			FreeNode( node );
			priorityQueues[level] = dummy.next;
			return;
		}
		prev = searchNode;
	}
	assert( 0 );
}

static void ReduceNode( OctTreeNode_t *node, int nodeLevel )
{
	int childNum;

//	printf( "\nbefore reduce: %d\n", numColorsUsed );
	PrintPriorityQueues();
	//PrintTree( root, 0 );
	if( node->timesUsed == 0 )
	{
		numColorsUsed++;	
	}
	for( childNum = 0; childNum < 8; childNum++ )
	{
		OctTreeNode_t *child;
		child = node->children[childNum];
		
		if( child )
		{
			node->cumulativeColor[0] += child->cumulativeColor[0];
			node->cumulativeColor[1] += child->cumulativeColor[1];
			node->cumulativeColor[2] += child->cumulativeColor[2];
			node->timesUsed += child->timesUsed;
			//printf( "before===\n" );
			//PrintPriorityQueues();
			RemoveFromPriorityQueue( nodeLevel + 1, child );
			//printf( "after===\n" );
			//PrintPriorityQueues();
			numColorsUsed--;
			node->children[childNum] = NULL;
		}
	}
	if( !node->isLeaf )
	{
		node->next = priorityQueues[nodeLevel];
		priorityQueues[nodeLevel] = node;
	}
	//PrintPriorityQueues();
	node->isLeaf = true;
//	printf( "\nafter reduce: %d\n", numColorsUsed );
	PrintPriorityQueues();
	//PrintTree( root, 0 );
}

static void ReduceTree( void )
{
	int i;
	for( i = 8; i > 0; i-- )
	{
		if( priorityQueues[i] )
		{
			ReduceNode( priorityQueues[i]->parent, i - 1 );
			return;
		}
	}
	assert( 0 );
}

static void AddToNode( OctTreeNode_t *node, int depth, unsigned char *color )
{
	node->timesUsed++;
	node->cumulativeColor[0] += ( 1.0f / 255.0f ) * color[0];
	node->cumulativeColor[1] += ( 1.0f / 255.0f ) * color[1];
	node->cumulativeColor[2] += ( 1.0f / 255.0f ) * color[2];
	node->isLeaf = true;
	// insert into priority queue if not already there.
	if( node->timesUsed == 1 )
	{
		node->next = priorityQueues[depth];
		priorityQueues[depth] = node;
		numColorsUsed++;
	}
}

static void Insert( OctTreeNode_t *node, OctTreeNode_t *parent, int depth, unsigned int r, unsigned int g, unsigned int b )
{
	if( depth == 8 || node->isLeaf )
	{
		if( numColorsUsed < desiredNumColors )
		{
			// just add it and go since we have pal entries to use.
			AddToNode( node, depth, insertColor );
		}
		else
		{
			// make space and try again.
			while( numColorsUsed >= desiredNumColors )
			{
				ReduceTree();
			}
			Insert( root, NULL, 0, insertColor[0], insertColor[1], insertColor[2] );
		}
		return;
	}

	// figure out which child to go to.
	int childNum;
	
	childNum = 
		( ( r & ( 1 << 7 ) ) >> 5 ) |
		( ( g & ( 1 << 7 ) ) >> 6 ) | 
		( ( b & ( 1 << 7 ) ) >> 7 );

	assert( childNum >= 0 && childNum < 8 );

	// does the child already exist?
	if( !node->children[childNum] )
	{
		// before allocating anything new, make sure we have
		// space for something new and start over
		if( numColorsUsed >= desiredNumColors )
		{
			do
			{
				ReduceTree();
			} while( numColorsUsed >= desiredNumColors );
			Insert( root, NULL, 0, insertColor[0], insertColor[1], insertColor[2] );
			return;
		}

		node->children[childNum] = AllocNode();
		node->children[childNum]->parent = node;
	}
	Insert( node->children[childNum], node, depth + 1, 
		    ( r << 1 ) & 0xff, 
			( g << 1 ) & 0xff, 
			( b << 1 ) & 0xff );
}

void Quantize( unsigned char *image, int numPixels, int bytesPerPixel, int numPalEntries, unsigned char *palette )
{
	pal = palette;
	
	desiredNumColors = numPalEntries;

	root = AllocNode();
	assert( root );
	numColorsUsed = 0;
	desiredNumColors = numPalEntries;

	int i;

	for( i = 0; i < 9; i++ )
	{
		priorityQueues[i] = NULL;
	}
	
	for( i = 0; i < numPixels; i++ )
	{
		memcpy( insertColor, &image[i*bytesPerPixel], 3 );
		Insert( root, NULL, 0, ( unsigned int )insertColor[0], ( unsigned int )insertColor[1], ( unsigned int )insertColor[2] );
#if 0
		printf( "\nafter inserting: %d %d %d\n", 
			( int )image[i*3+0], 
			( int )image[i*3+1], 
			( int )image[i*3+2] );
		PrintColorsInTree( root, 0, 0, 0, 0 );
		PrintTree( root, 0 );
#endif
	}
}

static void Indent( int n )
{
	int i;
	for( i = 0; i < n; i++ )
	{
		printf( "  " );
	}
}

static void PrintTree( OctTreeNode_t *node, int depth )
{
	int i;
	
	if( !node )
	{
		return;
	}
	if( node->isLeaf )
	{
		PrintNode( node );
	}
	for( i = 0; i < 8; i++ )
	{
		if( node->children[i] )
		{
			Indent( depth );
			printf( "[%d]\n", i );
			PrintTree( node->children[i], depth + 1 );
		}
	}
}

static void PrintNode( OctTreeNode_t *node )
{
	printf( "timesUsed: %d isLeaf: %d cumulativeColor: %f %f %f\n", 
		node->timesUsed, ( int )node->isLeaf, node->cumulativeColor[0], node->cumulativeColor[1],
		node->cumulativeColor[2] );
}

static void PrintColorsInTree( OctTreeNode_t *node, int depth, 
					    unsigned int r, unsigned int g, unsigned int b )
{
	int i;
	
	if( !node )
	{
		return;
	}
	
#if 0
	Indent( depth );
	printf( "depth: %d blah: %d %d %d\n", depth, ( int )r, ( int )g, ( int )b );
#endif

	if( depth == 8 )
	{
		PrintNode( node );
		return;
	}
	
	for( i = 0; i < 8; i++ )
	{
		PrintColorsInTree( node->children[i], depth + 1, 
			               ( r << 1 ) | ( ( i & 4 ) >> 2 ),
			               ( g << 1 ) | ( ( i & 2 ) >> 1 ),
			               ( b << 1 ) | ( ( i & 1 ) >> 0 ) );
	}
}

static void PrintPriorityQueues( void )
{
	return;

	int i;
	OctTreeNode_t *node;

	for( i = 0; i < 9; i++ )
	{
		if( priorityQueues[i] )
		{
			printf( "level: %d\n", i );
		}
		for( node = priorityQueues[i]; node; node = node->next )
		{
			PrintNode( node );
		}
	}
}

static void AverageColorsAndBuildPalette( OctTreeNode_t *node )
{
	if( !node )
	{
		return;
	}

	float fColor[3];
	float ooTimesUsed;
	int i;

	if( node->isLeaf )
	{
		ooTimesUsed = 1.0f / node->timesUsed;
		for( i = 0; i < 3; i++ )
		{
			fColor[i] = node->cumulativeColor[i] * ooTimesUsed;
			if( fColor[i] > 1.0f )
			{
				fColor[i] = 1.0f;
			}
			pal[currNumPalIndices*3+i] = ( unsigned char )( fColor[i] * 255 );
		}
		g_frequencyCounts[currNumPalIndices] = node->timesUsed;
		node->paletteIndex = currNumPalIndices;
		currNumPalIndices++;
		return;
	}
	
	for( i = 0; i < 8; i++ )
	{
		AverageColorsAndBuildPalette( node->children[i] );
	}
}

static void RemapPixel( OctTreeNode_t *node, int depth, 
				unsigned char *pixel, unsigned int r, unsigned int g, 
				unsigned int b )
{
	if( !node )
	{
		return;
	}
	
	if( node->isLeaf )
	{
		memcpy( pixel, &pal[3*node->paletteIndex], 3 );
		return;
	}

	// figure out which child to go to.
	int childNum;
	
	childNum = 
		( ( r & ( 1 << 7 ) ) >> 5 ) |
		( ( g & ( 1 << 7 ) ) >> 6 ) | 
		( ( b & ( 1 << 7 ) ) >> 7 );

	assert( childNum >= 0 && childNum < 8 );

	RemapPixel( node->children[childNum], depth + 1, pixel,
		    ( r << 1 ) & 0xff, 
			( g << 1 ) & 0xff, 
			( b << 1 ) & 0xff );
}

/* Warning avoidance..
static void MapImageToPalette( unsigned char *image, int numPixels, int bytesPerPixel )
{
	int i;

	for( i = 0; i < numPixels; i++ )
	{
		RemapPixel( root, 0, &image[i*bytesPerPixel], 
			( unsigned int )image[i*bytesPerPixel], 
			( unsigned int )image[i*bytesPerPixel+1], 
			( unsigned int )image[i*bytesPerPixel+2] );
	}
}
*/

// returns the actual number of palette entries.
int CalcPaletteForImage( unsigned char *image, int numPixels, int bytesPerPixel,
						 unsigned char *palette, int *frequencyCounts, int numPalEntries )
{
	InitNodeAllocation();
	Quantize( image, numPixels, bytesPerPixel, numPalEntries, palette );
	currNumPalIndices = 0;
	g_frequencyCounts = frequencyCounts;
	AverageColorsAndBuildPalette( root );
//	MapImageToPalette( image, numPixels, 3 );
	return numColorsUsed;
}
