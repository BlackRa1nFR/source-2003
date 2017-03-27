
#include "vbsp.h"

/*

tag all brushes with original contents
brushes may contain multiple contents
there will be no brush overlap after csg phase




each side has a count of the other sides it splits

the best split will be the one that minimizes the total split counts
of all remaining sides

precalc side on plane table

evaluate split side
{
cost = 0
for all sides
	for all sides
		get 
		if side splits side and splitside is on same child
			cost++;
}


  */

void SplitBrush2( bspbrush_t *brush, int planenum, bspbrush_t **front, bspbrush_t **back )
{
	SplitBrush( brush, planenum, front, back );
#if 0
	if (*front && (*front)->sides[(*front)->numsides-1].texinfo == -1)
		(*front)->sides[(*front)->numsides-1].texinfo = (*front)->sides[0].texinfo;	// not -1
	if (*back && (*back)->sides[(*back)->numsides-1].texinfo == -1)
		(*back)->sides[(*back)->numsides-1].texinfo = (*back)->sides[0].texinfo;	// not -1
#endif
}

/*
===============
SubtractBrush

Returns a list of brushes that remain after B is subtracted from A.
May by empty if A is contained inside B.

The originals are undisturbed.
===============
*/
bspbrush_t *SubtractBrush (bspbrush_t *a, bspbrush_t *b)
{	// a - b = out (list)
	int		i;
	bspbrush_t	*front, *back;
	bspbrush_t	*out, *in;

	in = a;
	out = NULL;
	for (i=0 ; i<b->numsides && in ; i++)
	{
		SplitBrush2 (in, b->sides[i].planenum, &front, &back);
		if (in != a)
			FreeBrush (in);
		if (front)
		{	// add to list
			front->next = out;
			out = front;
		}
		in = back;
	}
	if (in)
		FreeBrush (in);
	else
	{	// didn't really intersect
		FreeBrushList (out);
		return a;
	}
	return out;
}

/*
===============
IntersectBrush

Returns a single brush made up by the intersection of the
two provided brushes, or NULL if they are disjoint.

The originals are undisturbed.
===============
*/
bspbrush_t *IntersectBrush (bspbrush_t *a, bspbrush_t *b)
{
	int		i;
	bspbrush_t	*front, *back;
	bspbrush_t	*in;

	in = a;
	for (i=0 ; i<b->numsides && in ; i++)
	{
		SplitBrush2 (in, b->sides[i].planenum, &front, &back);
		if (in != a)
			FreeBrush (in);
		if (front)
			FreeBrush (front);
		in = back;
	}

	if (in == a)
		return NULL;

	in->next = NULL;
	return in;
}


/*
===============
BrushesDisjoint

Returns true if the two brushes definately do not intersect.
There will be false negatives for some non-axial combinations.
===============
*/
qboolean BrushesDisjoint (bspbrush_t *a, bspbrush_t *b)
{
	int		i, j;

	// check bounding boxes
	for (i=0 ; i<3 ; i++)
		if (a->mins[i] >= b->maxs[i]
		|| a->maxs[i] <= b->mins[i])
			return true;	// bounding boxes don't overlap

	// check for opposing planes
	for (i=0 ; i<a->numsides ; i++)
	{
		for (j=0 ; j<b->numsides ; j++)
		{
			if (a->sides[i].planenum ==
			(b->sides[j].planenum^1) )
				return true;	// opposite planes, so not touching
		}
	}

	return false;	// might intersect
}

/*
===============
IntersectionContents

Returns a content word for the intersection of two brushes.
Some combinations will generate a combination (water + clip),
but most will be the stronger of the two contents.
===============
*/
int	IntersectionContents (int c1, int c2)
{
	int		out;

	out = c1 | c2;

	if (out & CONTENTS_SOLID)
		out = CONTENTS_SOLID;

	return out;
}


int		minplanenums[3];
int		maxplanenums[3];

/*
===============
ClipBrushToBox

Any planes shared with the box edge will be set to no texinfo
===============
*/
bspbrush_t	*ClipBrushToBox (bspbrush_t *brush, const Vector& clipmins, const Vector& clipmaxs)
{
	int		i, j;
	bspbrush_t	*front,	*back;
	int		p;

	for (j=0 ; j<2 ; j++)
	{
		if (brush->maxs[j] > clipmaxs[j])
		{
			SplitBrush (brush, maxplanenums[j], &front, &back);
			if (front)
				FreeBrush (front);
			brush = back;
			if (!brush)
				return NULL;
		}
		if (brush->mins[j] < clipmins[j])
		{
			SplitBrush (brush, minplanenums[j], &front, &back);
			if (back)
				FreeBrush (back);
			brush = front;
			if (!brush)
				return NULL;
		}
	}

	// remove any colinear faces

	for (i=0 ; i<brush->numsides ; i++)
	{
		p = brush->sides[i].planenum & ~1;
		if (p == maxplanenums[0] || p == maxplanenums[1] 
			|| p == minplanenums[0] || p == minplanenums[1])
		{
			brush->sides[i].texinfo = TEXINFO_NODE;
			brush->sides[i].visible = false;
		}
	}
	return brush;
}


//-----------------------------------------------------------------------------
// Creates a clipped brush from a map brush
//-----------------------------------------------------------------------------
static bspbrush_t *CreateClippedBrush( mapbrush_t *mb, const Vector& clipmins, const Vector& clipmaxs )
{
	int nNumSides = mb->numsides;
	if (!nNumSides)
		return NULL;

	// if the brush is outside the clip area, skip it
	for (int j=0 ; j<3 ; j++)
	{
		if (mb->mins[j] >= clipmaxs[j] || mb->maxs[j] <= clipmins[j])
		{
			return NULL;
		}
	}

	// make a copy of the brush
	bspbrush_t *newbrush = AllocBrush( nNumSides );
	newbrush->original = mb;
	newbrush->numsides = nNumSides;
	memcpy (newbrush->sides, mb->original_sides, nNumSides*sizeof(side_t));

	for (j=0 ; j<nNumSides; j++)
	{
		if (newbrush->sides[j].winding)
		{
			newbrush->sides[j].winding = CopyWinding (newbrush->sides[j].winding);
		}

		if (newbrush->sides[j].surf & SURF_HINT)
		{
			newbrush->sides[j].visible = true;	// hints are always visible
		}

        // keep a pointer to the original map brush side -- use to create the original face later!!
        //newbrush->sides[j].original = &mb->original_sides[j];
	}

	VectorCopy (mb->mins, newbrush->mins);
	VectorCopy (mb->maxs, newbrush->maxs);

	// carve off anything outside the clip box
	newbrush = ClipBrushToBox (newbrush, clipmins, clipmaxs);
	return newbrush;
}


//-----------------------------------------------------------------------------
// Creates a clipped brush from a map brush
//-----------------------------------------------------------------------------
static void ComputeBoundingPlanes( const Vector& clipmins, const Vector& clipmaxs )
{
	Vector normal;
	float dist;
	for (int i=0 ; i<2 ; i++)
	{
		VectorClear (normal);
		normal[i] = 1;
		dist = clipmaxs[i];
		maxplanenums[i] = FindFloatPlane (normal, dist);
		dist = clipmins[i];
		minplanenums[i] = FindFloatPlane (normal, dist);
	}
}


// This forces copies of texinfo data for matching sides of a brush
void CopyMatchingTexinfos( bspbrush_t *pDest, const bspbrush_t *pSource )
{
	for ( int i = 0; i < pDest->numsides; i++ )
	{
		side_t *pSide = &pDest->sides[i];
		plane_t *pPlane = &mapplanes[pSide->planenum];
		bool found = false;
		for ( int j = 0; j < pSource->numsides; j++ )
		{
			const side_t *pSourceSide = &pSource->sides[j];
			plane_t *pSourcePlane = &mapplanes[pSourceSide->planenum];

			float dot = DotProduct( pPlane->normal, pSourcePlane->normal );
			if ( dot == 1.0 || pSide->planenum == pSourceSide->planenum )
			{
				found = true;
				pSide->texinfo = pSourceSide->texinfo;
				if ( pSide->original )
				{
					pSide->original->texinfo = pSide->texinfo;
				}
				break;
			}
		}
		if ( !found )
		{
			texinfo_t *pTexInfo = &texinfo[pSide->texinfo];
			dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
			Msg("Found no matching plane for %s\n", TexDataStringTable_GetString( pTexData->nameStringTableID ) );
		}
	}
}

// This is a hack to allow areaportals to work in water
// It was done this way for ease of implementation.
// This searches a brush list to find intersecting areaportals and water
// If an areaportal is found inside water, then the water contents and 
// texture information is copied over to the areaportal so that the 
// resulting space has the same properties as the water (normal areaportals assume "empty" surroundings)
void FixupAreaportalWaterBrushes( bspbrush_t *pList )
{
	for ( bspbrush_t *pAreaportal = pList; pAreaportal; pAreaportal = pAreaportal->next )
	{
		if ( !(pAreaportal->original->contents & CONTENTS_AREAPORTAL) )
			continue;

		for ( bspbrush_t *pWater = pList; pWater; pWater = pWater->next )
		{
			if ( !(pWater->original->contents & CONTENTS_WATER) )
				continue;
			if ( BrushesDisjoint( pAreaportal, pWater ) )
				continue;

			bspbrush_t *pIntersect = IntersectBrush( pAreaportal, pWater );
			if ( !pIntersect )
				continue;
			FreeBrush( pIntersect );
			pAreaportal->original->contents |= pWater->original->contents;
			CopyMatchingTexinfos( pAreaportal, pWater );
		}
	}
}

//-----------------------------------------------------------------------------
// MakeBspBrushList 
//-----------------------------------------------------------------------------
// UNDONE: Put detail brushes in a separate brush array and pass that instead of "onlyDetail" ?
bspbrush_t *MakeBspBrushList (int startbrush, int endbrush, const Vector& clipmins, const Vector& clipmaxs, int detailScreen)
{
	ComputeBoundingPlanes( clipmins, clipmaxs );

	bspbrush_t	*pBrushList = NULL;

	int i;
	for (i=startbrush ; i<endbrush ; i++)
	{
		mapbrush_t *mb = &mapbrushes[i];

		if ( detailScreen != FULL_DETAIL )
		{
			bool onlyDetail = (detailScreen == ONLY_DETAIL);
			bool detail = (mb->contents & CONTENTS_DETAIL) != 0;
			if ( onlyDetail ^ detail )
			{
				// both of these must have the same value or we're not interested in this brush
				continue;
			}
		}

		bspbrush_t *pNewBrush = CreateClippedBrush( mb, clipmins, clipmaxs );
		if ( pNewBrush )
		{
			pNewBrush->next = pBrushList;
			pBrushList = pNewBrush;
		}
	}

	return pBrushList;
}


//-----------------------------------------------------------------------------
// A version which uses a passed-in list of brushes 
//-----------------------------------------------------------------------------
bspbrush_t *MakeBspBrushList (mapbrush_t **pBrushes, int nBrushCount, const Vector& clipmins, const Vector& clipmaxs)
{
	ComputeBoundingPlanes( clipmins, clipmaxs );

	bspbrush_t	*pBrushList = NULL;
	for ( int i=0; i < nBrushCount; ++i )
	{
		bspbrush_t *pNewBrush = CreateClippedBrush( pBrushes[i], clipmins, clipmaxs );
		if ( pNewBrush )
		{
			pNewBrush->next = pBrushList;
			pBrushList = pNewBrush;
		}
	}

	return pBrushList;
}


/*
===============
AddBspBrushListToTail
===============
*/
bspbrush_t *AddBrushListToTail (bspbrush_t *list, bspbrush_t *tail)
{
	bspbrush_t	*walk, *next;

	for (walk=list ; walk ; walk=next)
	{	// add to end of list
		next = walk->next;
		walk->next = NULL;
		tail->next = walk;
		tail = walk;
	}

	return tail;
}

/*
===========
CullList

Builds a new list that doesn't hold the given brush
===========
*/
bspbrush_t *CullList (bspbrush_t *list, bspbrush_t *skip1)
{
	bspbrush_t	*newlist;
	bspbrush_t	*next;

	newlist = NULL;

	for ( ; list ; list = next)
	{
		next = list->next;
		if (list == skip1)
		{
			FreeBrush (list);
			continue;
		}
		list->next = newlist;
		newlist = list;
	}
	return newlist;
}


/*
==================
WriteBrushMap
==================
*/
void WriteBrushMap (char *name, bspbrush_t *list)
{
	FILE	*f;
	side_t	*s;
	int		i;
	winding_t	*w;

	Msg("writing %s\n", name);
	f = fopen (name, "w");
	if (!f)
		Error ("Can't write %s\b", name);

	fprintf (f, "{\n\"classname\" \"worldspawn\"\n");

	for ( ; list ; list=list->next )
	{
		fprintf (f, "{\n");
		for (i=0,s=list->sides ; i<list->numsides ; i++,s++)
		{
			w = BaseWindingForPlane (mapplanes[s->planenum].normal, mapplanes[s->planenum].dist);

			fprintf (f,"( %i %i %i ) ", (int)w->p[0][0], (int)w->p[0][1], (int)w->p[0][2]);
			fprintf (f,"( %i %i %i ) ", (int)w->p[1][0], (int)w->p[1][1], (int)w->p[1][2]);
			fprintf (f,"( %i %i %i ) ", (int)w->p[2][0], (int)w->p[2][1], (int)w->p[2][2]);

			fprintf (f, "%s 0 0 0 1 1\n", TexDataStringTable_GetString( GetTexData( texinfo[s->texinfo].texdata )->nameStringTableID ) );
			FreeWinding (w);
		}
		fprintf (f, "}\n");
	}
	fprintf (f, "}\n");

	fclose (f);

}

// UNDONE: This isn't quite working yet
#if 0
void WriteBrushVMF(char *name, bspbrush_t *list)
{
	FILE	*f;
	side_t	*s;
	int		i;
	winding_t	*w;
	Vector	u, v;

	Msg("writing %s\n", name);
	f = fopen (name, "w");
	if (!f)
		Error ("Can't write %s\b", name);

	fprintf (f, "world\n{\n\"classname\" \"worldspawn\"\n");

	for ( ; list ; list=list->next )
	{
		fprintf (f, "\tsolid\n\t{\n");
		for (i=0,s=list->sides ; i<list->numsides ; i++,s++)
		{
			fprintf( f, "\t\tside\n\t\t{\n" );
			fprintf( f, "\t\t\t\"plane\" \"" );
			w = BaseWindingForPlane (mapplanes[s->planenum].normal, mapplanes[s->planenum].dist);

			fprintf (f,"(%i %i %i) ", (int)w->p[0][0], (int)w->p[0][1], (int)w->p[0][2]);
			fprintf (f,"(%i %i %i) ", (int)w->p[1][0], (int)w->p[1][1], (int)w->p[1][2]);
			fprintf (f,"(%i %i %i)", (int)w->p[2][0], (int)w->p[2][1], (int)w->p[2][2]);
			fprintf( f, "\"\n" );
			fprintf( f, "\t\t\t\"material\" \"%s\"\n", GetTexData( texinfo[s->texinfo].texdata )->name );
			// UNDONE: recreate correct texture axes
			BasisForPlane( mapplanes[s->planenum].normal, u, v );
			fprintf( f, "\t\t\t\"uaxis\" \"[%.3f %.3f %.3f 0] 1.0\"\n", u[0], u[1], u[2] );
			fprintf( f, "\t\t\t\"vaxis\" \"[%.3f %.3f %.3f 0] 1.0\"\n", v[0], v[1], v[2] );
			
			fprintf( f, "\t\t\t\"rotation\" \"0.0\"\n" );
			fprintf( f, "\t\t\t\"lightmapscale\" \"16.0\"\n" );

			FreeWinding (w);
			fprintf (f, "\t\t}\n");
		}
		fprintf (f, "\t}\n");
	}
	fprintf (f, "}\n");

	fclose (f);

}
#endif

void PrintBrushContents( int contents )
{
	if( contents & CONTENTS_SOLID )
	{
		printf( "CONTENTS_SOLID " );
	}
	if( contents & CONTENTS_WINDOW )
	{
		printf( "CONTENTS_WINDOW " );
	}
	if( contents & CONTENTS_AUX )
	{
		printf( "CONTENTS_AUX " );
	}
	if( contents & CONTENTS_GRATE )
	{
		printf( "CONTENTS_GRATE " );
	}
	if( contents & CONTENTS_SLIME )
	{
		printf( "CONTENTS_SLIME " );
	}
	if( contents & CONTENTS_WATER )
	{
		printf( "CONTENTS_WATER " );
	}
	if( contents & CONTENTS_MIST )
	{
		printf( "CONTENTS_MIST " );
	}
	if( contents & CONTENTS_OPAQUE )
	{
		printf( "CONTENTS_OPAQUE " );
	}
	if( contents & CONTENTS_AREAPORTAL )
	{
		printf( "CONTENTS_AREAPORTAL " );
	}
}

/*
==================
BrushGE

Returns true if b1 is allowed to bite b2
==================
*/
qboolean BrushGE (bspbrush_t *b1, bspbrush_t *b2)
{
	// Areaportals are allowed to bite water
	// NOTE: This brush combo should have been fixed up
	// in a first pass (FixupAreaportalWaterBrushes)
	if( (b2->original->contents & CONTENTS_WATER) && 
		(b1->original->contents & CONTENTS_AREAPORTAL) )
	{
		return true;
	}
	
	// detail brushes never bite structural brushes
	if ( (b1->original->contents & CONTENTS_DETAIL) 
		&& !(b2->original->contents & CONTENTS_DETAIL) )
		return false;
	if (b1->original->contents & CONTENTS_SOLID)
		return true;
	// Transparent brushes are not marked as detail anymore, so let them cut each other.
	if ( (b1->original->contents & TRANSPARENT_CONTENTS) && (b2->original->contents & TRANSPARENT_CONTENTS) )
		return true;

	return false;
}

/*
=================
ChopBrushes

Carves any intersecting solid brushes into the minimum number
of non-intersecting brushes. 
=================
*/
bspbrush_t *ChopBrushes (bspbrush_t *head)
{
	bspbrush_t	*b1, *b2, *next;
	bspbrush_t	*tail;
	bspbrush_t	*keep;
	bspbrush_t	*sub, *sub2;
	int			c1, c2;

	qprintf ("---- ChopBrushes ----\n");
	qprintf ("original brushes: %i\n", CountBrushList (head));

#if 0
	if (startbrush == 0)
		WriteBrushList ("before.gl", head, false);
#endif
	keep = NULL;

newlist:
	// find tail
	if (!head)
		return NULL;
	for (tail=head ; tail->next ; tail=tail->next)
	;

	for (b1=head ; b1 ; b1=next)
	{
		next = b1->next;
		for (b2=b1->next ; b2 ; b2 = b2->next)
		{
			if (BrushesDisjoint (b1, b2))
				continue;

			sub = NULL;
			sub2 = NULL;
			c1 = 999999;
			c2 = 999999;

			if ( BrushGE (b2, b1) )
			{
//				printf( "b2 bites b1\n" );
				sub = SubtractBrush (b1, b2);
				if (sub == b1)
					continue;		// didn't really intersect
				if (!sub)
				{	// b1 is swallowed by b2
					head = CullList (b1, b1);
					goto newlist;
				}
				c1 = CountBrushList (sub);
			}

			if ( BrushGE (b1, b2) )
			{
//				printf( "b1 bites b2\n" );
				sub2 = SubtractBrush (b2, b1);
				if (sub2 == b2)
					continue;		// didn't really intersect
				if (!sub2)
				{	// b2 is swallowed by b1
					FreeBrushList (sub);
					head = CullList (b1, b2);
					goto newlist;
				}
				c2 = CountBrushList (sub2);
			}

			if (!sub && !sub2)
				continue;		// neither one can bite

			// only accept if it didn't fragment
			// (commening this out allows full fragmentation)
			if (c1 > 1 && c2 > 1)
			{
				const int contents1 = b1->original->contents;
				const int contents2 = b2->original->contents;
				// if both detail, allow fragmentation
				if ( !((contents1&contents2) & CONTENTS_DETAIL) && !((contents1|contents2) & CONTENTS_AREAPORTAL) )
				{
					if (sub2)
						FreeBrushList (sub2);
					if (sub)
						FreeBrushList (sub);
					continue;
				}
			}

			if (c1 < c2)
			{
				if (sub2)
					FreeBrushList (sub2);
				tail = AddBrushListToTail (sub, tail);
				head = CullList (b1, b1);
				goto newlist;
			}
			else
			{
				if (sub)
					FreeBrushList (sub);
				tail = AddBrushListToTail (sub2, tail);
				head = CullList (b1, b2);
				goto newlist;
			}
		}

		if (!b2)
		{	// b1 is no longer intersecting anything, so keep it
			b1->next = keep;
			keep = b1;
		}
	}

	qprintf ("output brushes: %i\n", CountBrushList (keep));
#if 0
	if ( head->original->entitynum == 0 )
	{
		WriteBrushList ("after.gl", keep, false);
		WriteBrushMap ("after.map", keep);
	}
#endif
	return keep;
}


/*
=================
InitialBrushList
=================
*/
bspbrush_t *InitialBrushList (bspbrush_t *list)
{
	bspbrush_t *b;
	bspbrush_t	*out, *newb;
	int			i;

	// only return brushes that have visible faces
	out = NULL;
	for (b=list ; b ; b=b->next)
	{
#if 0
		for (i=0 ; i<b->numsides ; i++)
			if (b->sides[i].visible)
				break;
		if (i == b->numsides)
			continue;
#endif
		newb = CopyBrush (b);
		newb->next = out;
		out = newb;

		// clear visible, so it must be set by MarkVisibleFaces_r
		// to be used in the optimized list
		for (i=0 ; i<b->numsides ; i++)
		{
			newb->sides[i].original = &b->sides[i];
//			newb->sides[i].visible = true;
			b->sides[i].visible = false;
		}
	}

	return out;
}

/*
=================
OptimizedBrushList
=================
*/
bspbrush_t *OptimizedBrushList (bspbrush_t *list)
{
	bspbrush_t *b;
	bspbrush_t	*out, *newb;
	int			i;

	// only return brushes that have visible faces
	out = NULL;
	for (b=list ; b ; b=b->next)
	{
		for (i=0 ; i<b->numsides ; i++)
			if (b->sides[i].visible)
				break;
		if (i == b->numsides)
			continue;
		newb = CopyBrush (b);
		newb->next = out;
		out = newb;
	}

//	WriteBrushList ("vis.gl", out, true);

	return out;
}
