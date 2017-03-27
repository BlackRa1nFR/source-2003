//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:	Debugging overlay functions
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include "edict.h"
#include "debugoverlay.h"
#include "cdll_int.h"
#include "vid.h"
#include "vgui_int.h"
#include "draw.h"
#include "materialsystem/IMesh.h"
#include "gl_matsysiface.h"
#include "glquake.h"
#include "server.h"
#include "client_class.h"
#include "icliententitylist.h"
#include "VMatrix.h"
#include "icliententity.h"
#include "overlaytext.h"
#include "engine/IVDebugOverlay.h"
#include "cmodel_engine.h"


extern vrect_t	scr_vrect;
extern edict_t *EDICT_NUM(int n);

bool OverlayText_t::IsDead()
{
	if ( m_nCreationMessage != -1 )
	{
		if ( m_nServerCount != cl.servercount )
			return true;
		if ( cls.netchan.incoming_sequence > m_nCreationMessage )
			return true;
		return false;
	}
	else if ( m_nCreationFrame != -1 )
	{
		if ( m_nServerCount != cl.servercount )
			return true;
		if ( host_framecount > m_nCreationFrame )
			return true;
		return false;
	}

	return (cl.gettime() >= m_flEndTime) ? true : false;
}

void OverlayText_t::SetEndTime( float duration )
{
	if ( duration == 0.0f )
	{
		m_flEndTime = 0.0f;
		m_nCreationMessage = cls.netchan.incoming_sequence;
		m_nServerCount = cl.servercount;
		return;
	}
	else if ( duration == -1.0f )
	{
		m_flEndTime = 0.0f;
		m_nCreationFrame = host_framecount;
		m_nServerCount = cl.servercount;
		return;
	}
	
	m_flEndTime = cl.gettime() + duration;
}

namespace CDebugOverlay
{

enum OverlayType_t
{
	OVERLAY_BOX = 0,
	OVERLAY_LINE,
	OVERLAY_TRIANGLE,
	OVERLAY_SWEPT_BOX,
};

struct OverlayBase_t
{
	OverlayBase_t()
	{
		m_Type = OVERLAY_BOX;
		m_nCreationMessage = -1;
		m_nServerCount = -1;
		m_nCreationFrame = -1;
		m_flEndTime = 0.0f;
		m_pNextOverlay = NULL;
	}

	bool			IsDead()
	{
		if ( m_nCreationMessage != -1 )
		{
			if ( m_nServerCount != cl.servercount )
				return true;
			if ( cls.netchan.incoming_sequence > m_nCreationMessage )
				return true;
			return false;
		}
		else if ( m_nCreationFrame != -1 )
		{
			if ( m_nServerCount != cl.servercount )
				return true;
			if ( host_framecount > m_nCreationFrame )
				return true;
			return false;
		}

		return (cl.gettime() >= m_flEndTime) ? true : false;
	}

	void			SetEndTime( float duration )
	{
		if ( duration == 0.0f )
		{
			m_flEndTime = 0.0f;
			m_nCreationMessage = cls.netchan.incoming_sequence;
			m_nServerCount = cl.servercount;
			return;
		}
		else if ( duration == -1.0f )
		{
			m_flEndTime = 0.0f;
			m_nCreationFrame = host_framecount;
			m_nServerCount = cl.servercount;
			return;
		}
		
		m_flEndTime = cl.gettime() + duration;
	}

	OverlayType_t	m_Type;				// What type of overlay is it?
	int				m_nCreationMessage; // If an overlay has a duration of zero, then
										//  this number is the cls.netchan.incoming_sequence when the
										//  overlay was created
	int				m_nCreationFrame;	// Duration -1 means go away after this frame #
	int				m_nServerCount;		// Latch server count, too
	float			m_flEndTime;		// When does this box go away
	OverlayBase_t	*m_pNextOverlay;
};

struct OverlayBox_t : public OverlayBase_t
{
	OverlayBox_t() { m_Type = OVERLAY_BOX; }

	Vector			origin;
	Vector			mins;
	Vector			maxs;
	QAngle			angles;
	int				r;
	int				g;
	int				b;
	int				a;
};

struct OverlayLine_t : public OverlayBase_t 
{
	OverlayLine_t() { m_Type = OVERLAY_LINE; }

	Vector			origin;
	Vector			dest;
	int				r;
	int				g;
	int				b;
	bool			noDepthTest;
};

struct OverlayTriangle_t : public OverlayBase_t 
{
	OverlayTriangle_t() { m_Type = OVERLAY_TRIANGLE; }

	Vector			p1;
	Vector			p2;
	Vector			p3;
	int				r;
	int				g;
	int				b;
	int				a;
	bool			noDepthTest;
};

struct OverlaySweptBox_t : public OverlayBase_t 
{
	OverlaySweptBox_t() { m_Type = OVERLAY_SWEPT_BOX; }

	Vector			start;
	Vector			end;
	Vector			mins;
	Vector			maxs;
	QAngle			angles;
	int				r;
	int				g;
	int				b;
	int				a;
};


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
void DrawOverlays();
void DrawGridOverlay();
void ClearAllOverlays();
void ClearDeadOverlays();


//-----------------------------------------------------------------------------
// Init static member variables
//-----------------------------------------------------------------------------
OverlayText_t*	s_pOverlayText = NULL;	// text is handled differently; for backward compatibility reasons
OverlayBase_t*	s_pOverlays = NULL; 
Vector			s_vGridPosition(0,0,0);
bool			s_bDrawGrid = false;


//-----------------------------------------------------------------------------
// Purpose: Hack to allow this code to run on a client that's not connected to a server
//  (i.e., demo playback, or multiplayer game )
// Input  : ent_num - 
//			origin - 
//			mins - 
//			maxs - 
// Output : static void
//-----------------------------------------------------------------------------
static bool GetEntityOriginClientOrServer( int ent_num, Vector& origin )
{
	// Assume failure
	origin.Init();

	if ( sv.active )
	{
		edict_t *e = EDICT_NUM( ent_num );
		if ( e )
		{
			IServerEntity *serverEntity = e->GetIServerEntity();
			if ( serverEntity )
			{
				CM_WorldSpaceCenter( serverEntity->GetCollideable(), &origin );
			}

			return true;
		}
	}
	else
	{
		IClientEntity *clent = entitylist->GetClientEntity( ent_num );
		if ( clent )
		{
			CM_WorldSpaceCenter( clent->GetClientCollideable(), &origin );
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Given a point, return the screen position
// Input  :
// Output :
//-----------------------------------------------------------------------------
int ScreenPosition(const Vector& point, Vector& screen)
{
	int retval = g_EngineRenderer->ClipTransform(point,&screen);

	screen[0] =  0.5 * screen[0] * scr_vrect.width;
	screen[1] = -0.5 * screen[1] * scr_vrect.height;
	screen[0] += 0.5 *  scr_vrect.width;
	screen[1] += 0.5 *  scr_vrect.height;
	return retval;
}

//-----------------------------------------------------------------------------
// Purpose: Given an xy screen pos (0-1), return the screen position 
// Input  :
// Output :
//-----------------------------------------------------------------------------
int ScreenPosition(float flXPos, float flYPos, Vector& screen)
{
	if (flXPos > 1.0 ||
		flYPos > 1.0 ||
		flXPos < 0.0 ||
		flYPos < 0.0 )
	{
		return 1; // Fail
	}
	screen[0] =  flXPos * scr_vrect.width;
	screen[1] =  flYPos * scr_vrect.height;
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Add new entity positioned overlay text
// Input  : Entity to attach text to
//			How many lines to offset text from entity origin
//			The text to print
// Output :
//-----------------------------------------------------------------------------
void AddEntityTextOverlay(int ent_index, int line_offset, float duration, int r, int g, int b, int a, const char *text)
{
	OverlayText_t *new_overlay = new OverlayText_t;

	Vector myPos, myMins, myMaxs;

	GetEntityOriginClientOrServer( ent_index, myPos );

	VectorCopy(myPos,new_overlay->origin);
	strcpy(new_overlay->text,text);
	new_overlay->bUseOrigin = true;
	new_overlay->lineOffset	= line_offset;
	new_overlay->SetEndTime( duration );
	new_overlay->r			= r;
	new_overlay->g			= g;
	new_overlay->b			= b;
	new_overlay->a			= a;

	new_overlay->nextOverlayText = s_pOverlayText;
	s_pOverlayText = new_overlay;
}

//-----------------------------------------------------------------------------
// Purpose: Add new overlay text
// Input  : Position of text & text
// Output :
//-----------------------------------------------------------------------------
void AddGridOverlay(const Vector& vPos) 
{
	s_vGridPosition[0]		= vPos[0];
	s_vGridPosition[1]		= vPos[1];
	s_vGridPosition[2]		= vPos[2];
	s_bDrawGrid				= true;
}

//-----------------------------------------------------------------------------
// Purpose: Add new overlay text
// Input  : Position of text & text
// Output :
//-----------------------------------------------------------------------------
void AddTextOverlay(const Vector& textPos, float duration, const char *text) 
{
	OverlayText_t *new_overlay = new OverlayText_t;

	VectorCopy(textPos,new_overlay->origin);
	strcpy(new_overlay->text,text);
	new_overlay->bUseOrigin = true;
	new_overlay->lineOffset	= 0;
	new_overlay->SetEndTime( duration );
	new_overlay->r			= 255;
	new_overlay->g			= 255;
	new_overlay->b			= 255;
	new_overlay->a			= 255;

	new_overlay->nextOverlayText = s_pOverlayText;
	s_pOverlayText = new_overlay;
}

//-----------------------------------------------------------------------------
// Purpose: Add new overlay text
// Input  : Position of text & text
// Output :
//-----------------------------------------------------------------------------
void AddTextOverlay(const Vector& textPos, float duration, float alpha, const char *text) 
{
	OverlayText_t *new_overlay = new OverlayText_t;

	VectorCopy(textPos,new_overlay->origin);
	strcpy(new_overlay->text,text);
	new_overlay->bUseOrigin = true;
	new_overlay->lineOffset	= 0;
	new_overlay->SetEndTime( duration );
	new_overlay->r			= 255;
	new_overlay->g			= 255;
	new_overlay->b			= 255;
	new_overlay->a			= (int)clamp(alpha * 255.f,0.f,255.f);

	new_overlay->nextOverlayText = s_pOverlayText;
	s_pOverlayText = new_overlay;
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void AddScreenTextOverlay(float flXPos, float flYPos, float duration, int r, int g, int b, int a, const char *text)
{
	OverlayText_t *new_overlay = new OverlayText_t;

	strcpy(new_overlay->text,text);
	new_overlay->flXPos		= flXPos;
	new_overlay->flYPos		= flYPos;
	new_overlay->bUseOrigin = false;
	new_overlay->lineOffset	= 0;
	new_overlay->SetEndTime( duration );
	new_overlay->r			= r;
	new_overlay->g			= g;
	new_overlay->b			= b;
	new_overlay->a			= a;

	new_overlay->nextOverlayText = s_pOverlayText;
	s_pOverlayText = new_overlay;
}


//-----------------------------------------------------------------------------
// Purpose: Add new overlay text
// Input  : Position of text 
//			How many lines to offset text from position
//			ext
// Output :
//-----------------------------------------------------------------------------
void AddTextOverlay(const Vector& textPos, int line_offset, float duration, const char *text) 
{
	OverlayText_t *new_overlay = new OverlayText_t;

	VectorCopy(textPos,new_overlay->origin);
	strcpy(new_overlay->text,text);
	new_overlay->bUseOrigin = true;
	new_overlay->lineOffset	= line_offset;
	new_overlay->SetEndTime( duration );
	new_overlay->r			= 255;
	new_overlay->g			= 255;
	new_overlay->b			= 255;
	new_overlay->a			= 255;
	new_overlay->bUseOrigin = true;

	new_overlay->nextOverlayText = s_pOverlayText;
	s_pOverlayText = new_overlay;
}

void AddTextOverlay(const Vector& textPos, int line_offset, float duration, float alpha, const char *text)
{
	OverlayText_t *new_overlay = new OverlayText_t;

	VectorCopy(textPos,new_overlay->origin);
	strcpy(new_overlay->text,text);
	new_overlay->bUseOrigin = true;
	new_overlay->lineOffset	= line_offset;
	new_overlay->SetEndTime( duration );
	new_overlay->r			= 255;
	new_overlay->g			= 255;
	new_overlay->b			= 255;
	new_overlay->a			= (int)clamp(alpha * 255.f,0.f,255.f);
	new_overlay->bUseOrigin = true;

	new_overlay->nextOverlayText = s_pOverlayText;
	s_pOverlayText = new_overlay;
}

//-----------------------------------------------------------------------------
// Purpose: Add new overlay box
// Input  : Position of box
//			size of box
//			angles of box
//			color & alpha
// Output :
//-----------------------------------------------------------------------------
void AddBoxOverlay(const Vector& origin, const Vector& mins, const Vector& maxs, QAngle const& angles, int r, int g, int b, int a, float flDuration) 
{
	OverlayBox_t *new_overlay = new OverlayBox_t;

	new_overlay->origin = origin;

	new_overlay->mins[0] = mins[0];
	new_overlay->mins[1] = mins[1];
	new_overlay->mins[2] = mins[2];

	new_overlay->maxs[0] = maxs[0];
	new_overlay->maxs[1] = maxs[1];
	new_overlay->maxs[2] = maxs[2];

	new_overlay->angles = angles;

	new_overlay->r = r;
	new_overlay->g = g;
	new_overlay->b = b;
	new_overlay->a = a;

	new_overlay->SetEndTime( flDuration );

	new_overlay->m_pNextOverlay = s_pOverlays;
	s_pOverlays = new_overlay;
}

void AddSweptBoxOverlay(const Vector& start, const Vector& end, 
	const Vector& mins, const Vector& maxs, QAngle const& angles, int r, int g, int b, int a, float flDuration)
{
	OverlaySweptBox_t *new_overlay = new OverlaySweptBox_t;

	new_overlay->start = start;
	new_overlay->end = end;

	new_overlay->mins[0] = mins[0];
	new_overlay->mins[1] = mins[1];
	new_overlay->mins[2] = mins[2];

	new_overlay->maxs[0] = maxs[0];
	new_overlay->maxs[1] = maxs[1];
	new_overlay->maxs[2] = maxs[2];

	new_overlay->angles = angles;

	new_overlay->r = r;
	new_overlay->g = g;
	new_overlay->b = b;
	new_overlay->a = a;

	new_overlay->SetEndTime( flDuration );

	new_overlay->m_pNextOverlay = s_pOverlays;
	s_pOverlays = new_overlay;
}


//-----------------------------------------------------------------------------
// Purpose: Add new overlay text
// Input  : Entity to attach text to
//			How many lines to offset text from entity origin
//			The text to print
// Output :
//-----------------------------------------------------------------------------
void AddLineOverlay(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float flDuration) 
{
	OverlayLine_t *new_loverlay = new OverlayLine_t;

	new_loverlay->origin[0] = origin[0];
	new_loverlay->origin[1] = origin[1];
	new_loverlay->origin[2] = origin[2];

	new_loverlay->dest[0] = dest[0];
	new_loverlay->dest[1] = dest[1];
	new_loverlay->dest[2] = dest[2];

	new_loverlay->r = r;
	new_loverlay->g = g;
	new_loverlay->b = b;

	new_loverlay->noDepthTest = noDepthTest;

	new_loverlay->SetEndTime( flDuration );

	new_loverlay->m_pNextOverlay = s_pOverlays;
	s_pOverlays = new_loverlay;
}


//-----------------------------------------------------------------------------
// Purpose: Add new triangle overlay
//-----------------------------------------------------------------------------
void AddTriangleOverlay(const Vector& p1, const Vector& p2, const Vector &p3, 
				int r, int g, int b, int a, bool noDepthTest, float flDuration)
{
	OverlayTriangle_t *pTriangle = new OverlayTriangle_t;
	pTriangle->p1 = p1;
	pTriangle->p2 = p2;
	pTriangle->p3 = p3;

	pTriangle->r = r;
	pTriangle->g = g;
	pTriangle->b = b;
	pTriangle->a = a;

	pTriangle->noDepthTest = noDepthTest;
	pTriangle->SetEndTime( flDuration );

	pTriangle->m_pNextOverlay = s_pOverlays;
	s_pOverlays = pTriangle;
}


//------------------------------------------------------------------------------
// Purpose :	Draw a grid around the s_vGridPosition
// Input   :
// Output  :
//------------------------------------------------------------------------------
void DrawGridOverlay(void)
{
	static int gridSpacing		= 100;
	static int numHorzSpaces	= 16;
	static int numVertSpaces	= 3;

	Vector startGrid;
	startGrid[0] = gridSpacing*((int)s_vGridPosition[0]/gridSpacing);
	startGrid[1] = gridSpacing*((int)s_vGridPosition[1]/gridSpacing);
	startGrid[2] = s_vGridPosition[2];

	// Shift to the left
	startGrid[0] -= (numHorzSpaces/2)*gridSpacing;
	startGrid[1] -= (numHorzSpaces/2)*gridSpacing;

	Vector color(20,180,190);
	for (int i=1;i<numVertSpaces+1;i++)
	{
		// Draw x axis lines
		Vector startLine;
		VectorCopy(startGrid,startLine);
		for (int j=0;j<numHorzSpaces+1;j++)
		{
			Vector endLine;
			VectorCopy(startLine,endLine);
			endLine[0] += gridSpacing*numHorzSpaces;
			Draw_Line( startLine, endLine, color[0], color[1], color[2], false);

			Vector bottomStartLine;
			VectorCopy(startLine,bottomStartLine);
			for (int k=0;k<numHorzSpaces+1;k++)
			{
				Vector bottomEndLine;
				VectorCopy(bottomStartLine,bottomEndLine);
				bottomEndLine[2] -= gridSpacing;
				Draw_Line( bottomStartLine, bottomEndLine, color[0], color[1], color[2], false);
				bottomStartLine[0] += gridSpacing;
			}
			startLine[1] += gridSpacing;
		}

		// Draw y axis lines
		VectorCopy(startGrid,startLine);
		for (j=0;j<numHorzSpaces+1;j++)
		{
			Vector endLine;
			VectorCopy(startLine,endLine);

			endLine[1] += gridSpacing*numHorzSpaces;
			Draw_Line( startLine, endLine, color[0], color[1], color[2], false);
			startLine[0] += gridSpacing;
		}
		VectorScale( color, 0.7, color );
		startGrid[2]-= gridSpacing;
	}
	s_bDrawGrid				= false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			mins - 
//			maxs - 
//			angles- 
//			r - 
//			g - 
//			b - 
//-----------------------------------------------------------------------------

static void DrawAxes( const Vector& origin, Vector* pts, int idx, CMeshBuilder& meshBuilder )
{
	Vector start, temp;
	VectorAdd( pts[idx], origin, start );
	meshBuilder.Position3fv( start.Base() );
	meshBuilder.AdvanceVertex();

	int endidx = (idx & 0x1) ? idx - 1 : idx + 1;
	VectorAdd( pts[endidx], origin, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( start.Base() );
	meshBuilder.AdvanceVertex();

	endidx = (idx & 0x2) ? idx - 2 : idx + 2;
	VectorAdd( pts[endidx], origin, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( start.Base() );
	meshBuilder.AdvanceVertex();

	endidx = (idx & 0x4) ? idx - 4 : idx + 4;
	VectorAdd( pts[endidx], origin, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();
}

static void DrawExtrusionFace( const Vector& start, const Vector& end,
		Vector* pts, int idx1, int idx2, CMeshBuilder& meshBuilder )
{
	Vector temp;
	VectorAdd( pts[idx1], start, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();

	VectorAdd( pts[idx2], start, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();

	VectorAdd( pts[idx2], end, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();

	VectorAdd( pts[idx1], end, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();

	VectorAdd( pts[idx1], start, temp );
	meshBuilder.Position3fv( temp.Base() );
	meshBuilder.AdvanceVertex();
}

static void Draw_WireframeSweptBox( OverlaySweptBox_t* pBox )
{
	// Build a rotation matrix from angles
	matrix3x4_t fRotateMatrix;
	AngleMatrix(pBox->angles, fRotateMatrix);
		
	materialSystemInterface->Bind( g_materialWireframe );
	g_materialWireframe->ColorModulate( pBox->r*(1.0f/255.0f), pBox->g*(1.0f/255.0f), pBox->b*(1.0f/255.0f) );
	if (pBox->a > 0)
		g_materialWireframe->AlphaModulate( pBox->a*(1.0f/255.0f) );
	g_materialWireframe->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, 0 );

	IMesh *pMesh = materialSystemInterface->GetDynamicMesh( );
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 30 );
	
	Vector delta;
	VectorSubtract( pBox->end, pBox->start, delta );

	// Compute the box points, rotated but without the origin added
	Vector temp;
	Vector pts[8];
	float dot[8];
	int minidx = 0;
	for ( int i = 0; i < 8; ++i )
	{
		temp.x = (i & 0x1) ? pBox->maxs[0] : pBox->mins[0];
		temp.y = (i & 0x2) ? pBox->maxs[1] : pBox->mins[1];
		temp.z = (i & 0x4) ? pBox->maxs[2] : pBox->mins[2];

		// Rotate the corner point
		VectorRotate( temp, fRotateMatrix, pts[i]);

		// Find the dot product with dir
		dot[i] = DotProduct( pts[i], delta );
		if (dot[i] < dot[minidx])
			minidx = i;
	}

	// Choose opposite corner
	int maxidx = minidx ^ 0x7;

	// Draw the start + end axes...
	DrawAxes( pBox->start, pts, minidx, meshBuilder );
	DrawAxes( pBox->end, pts, maxidx, meshBuilder );

	// Draw the extrusion faces
	for (int j = 0; j < 3; ++j )
	{
		int dirflag1 = ( 1 << ((j+1)%3) );
		int dirflag2 = ( 1 << ((j+2)%3) );

		int idx1, idx2, idx3;
		idx1 = (minidx & dirflag1) ? minidx - dirflag1 : minidx + dirflag1;
		idx2 = (minidx & dirflag2) ? minidx - dirflag2 : minidx + dirflag2;
		idx3 = (minidx & dirflag2) ? idx1 - dirflag2 : idx1 + dirflag2;

		DrawExtrusionFace( pBox->start, pBox->end, pts, idx1, idx3, meshBuilder );
		DrawExtrusionFace( pBox->start, pBox->end, pts, idx2, idx3, meshBuilder );
	}
	meshBuilder.End();
	pMesh->Draw();
}


//------------------------------------------------------------------------------
// Draws a generic overlay
//------------------------------------------------------------------------------
void DrawOverlay( OverlayBase_t *pOverlay )
{
	switch( pOverlay->m_Type)
	{
	case OVERLAY_LINE:
		{
			// Draw the line
			OverlayLine_t *pLine = static_cast<OverlayLine_t*>(pOverlay);
			Draw_Line( pLine->origin, pLine->dest, pLine->r, pLine->g, pLine->b, pLine->noDepthTest);
		}
		break;

	case OVERLAY_BOX:
		{
			// Draw the box
			OverlayBox_t *pCurrBox = static_cast<OverlayBox_t*>(pOverlay);
			if (pCurrBox->a > 0.0) 
			{
				Draw_AlphaBox( pCurrBox->origin, pCurrBox->mins, pCurrBox->maxs, pCurrBox->angles, pCurrBox->r, pCurrBox->g, pCurrBox->b, pCurrBox->a);
			}
			Draw_WireframeBox( pCurrBox->origin, pCurrBox->mins, pCurrBox->maxs, pCurrBox->angles, pCurrBox->r, pCurrBox->g, pCurrBox->b);
		}
		break;

	case OVERLAY_SWEPT_BOX:
		{
			OverlaySweptBox_t *pBox = static_cast<OverlaySweptBox_t*>(pOverlay);
			Draw_WireframeSweptBox( pBox );
		}
		break;

	case OVERLAY_TRIANGLE:
		{
			OverlayTriangle_t *pTriangle = static_cast<OverlayTriangle_t*>(pOverlay);
			Draw_Triangle( pTriangle->p1, pTriangle->p2, pTriangle->p3, pTriangle->r,
				pTriangle->g, pTriangle->b, pTriangle->a, pTriangle->noDepthTest );
		}
		break;

	default:
		Assert(0);
	}
}

void DestroyOverlay( OverlayBase_t *pOverlay )
{
	switch( pOverlay->m_Type)
	{
	case OVERLAY_LINE:
		{
			// Draw the line
			OverlayLine_t *pCurrLine = static_cast<OverlayLine_t*>(pOverlay);
			delete pCurrLine;
		}
		break;

	case OVERLAY_BOX:
		{
			// Draw the box
			OverlayBox_t *pCurrBox = static_cast<OverlayBox_t*>(pOverlay);
			delete pCurrBox;
		}
		break;

	case OVERLAY_SWEPT_BOX:
		{
			OverlaySweptBox_t *pCurrBox = static_cast<OverlaySweptBox_t*>(pOverlay);
			delete pCurrBox;
		}
		break;

	case OVERLAY_TRIANGLE:
		{
			OverlayTriangle_t *pTriangle = static_cast<OverlayTriangle_t*>(pOverlay);
			delete pTriangle;
		}
		break;

	default:
		Assert(0);
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void DrawAllOverlays(void)
{
	OverlayBase_t* pCurrOverlay = s_pOverlays;
	OverlayBase_t* pPrevOverlay = NULL;
	OverlayBase_t* pNextOverlay;

	while (pCurrOverlay) 
	{
		// Is it time to kill this overlay?
		if ( pCurrOverlay->IsDead() )
		{
			if (pPrevOverlay)
			{
				// If I had a last overlay reset it's next pointer
				pPrevOverlay->m_pNextOverlay = pCurrOverlay->m_pNextOverlay;
			}
			else
			{
				// If the first line, reset the s_pOverlays pointer
				s_pOverlays = pCurrOverlay->m_pNextOverlay;
			}

			pNextOverlay = pCurrOverlay->m_pNextOverlay;
			DestroyOverlay( pCurrOverlay );
			pCurrOverlay = pNextOverlay;
		}
		else
		{
			DrawOverlay( pCurrOverlay );

			pPrevOverlay = pCurrOverlay;
			pCurrOverlay = pCurrOverlay->m_pNextOverlay;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void Draw3DOverlays(void)
{
	// If I'm just starting the map clear out all overlays
	static int previous_servercount = -1;
	if ( previous_servercount != cl.servercount )
	{
		ClearAllOverlays();
		previous_servercount = cl.servercount;
		return;
	}

	DrawAllOverlays();

	if (s_bDrawGrid)
	{
		DrawGridOverlay();
	}
}


//------------------------------------------------------------------------------
// Purpose : Deletes all overlays
// Input   :
// Output  :
//------------------------------------------------------------------------------
void	ClearAllOverlays(void)
{
	while (s_pOverlays) 
	{
		OverlayBase_t *pOldOverlay = s_pOverlays;
		s_pOverlays = s_pOverlays->m_pNextOverlay;
		DestroyOverlay( pOldOverlay );
	}

	while (s_pOverlayText) 
	{
		OverlayText_t *cur_ol = s_pOverlayText;
		s_pOverlayText = s_pOverlayText->nextOverlayText;
		delete cur_ol;
	}

	s_bDrawGrid = false;
}


void ClearDeadOverlays( void )
{
	OverlayText_t* pCurrText = s_pOverlayText;
	OverlayText_t* pLastText = NULL;
	OverlayText_t* pNextText = NULL;
	while (pCurrText) 
	{
		// Is it time to kill this Text?
		if ( pCurrText->IsDead() )
		{
			// If I had a last Text reset it's next pointer
			if (pLastText)
			{
				pLastText->nextOverlayText = pCurrText->nextOverlayText;
			}
			// If the first Text, reset the s_pOverlayText pointer
			else
			{
				s_pOverlayText = pCurrText->nextOverlayText;
			}
			pNextText = pCurrText->nextOverlayText;
			delete pCurrText;
			pCurrText = pNextText;
		}
		else
		{
			pLastText = pCurrText;
			pCurrText = pCurrText->nextOverlayText;
		}
	}
}

}	// end namespace CDebugOverlay


//-----------------------------------------------------------------------------
// Purpose:	export debug overlay to client DLL
// Input  :
// Output :
//-----------------------------------------------------------------------------
class CIVDebugOverlay : public IVDebugOverlay
{
private:
	char m_text[1024];
	va_list	m_argptr;

public:
	void AddEntityTextOverlay(int ent_index, int line_offset, float duration, int r, int g, int b, int a, const char *format, ...)
	{
		va_start( m_argptr, format );
		vsprintf( m_text, format, m_argptr );
		va_end( m_argptr );

		CDebugOverlay::AddEntityTextOverlay(ent_index, line_offset, duration, r, g, b, a, m_text);
	}

	void AddBoxOverlay(const Vector& origin, const Vector& mins, const Vector& max, QAngle const& angles, int r, int g, int b, int a, float duration)
	{
		CDebugOverlay::AddBoxOverlay(origin, mins, max, angles, r, g, b, a, duration);
	}

	void AddLineOverlay(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float duration)
	{
		CDebugOverlay::AddLineOverlay(origin, dest, r, g, b, noDepthTest, duration);
	}

	void AddTriangleOverlay(const Vector& p1, const Vector& p2, const Vector &p3, int r, int g, int b, int a, bool noDepthTest, float duration)
	{
		CDebugOverlay::AddTriangleOverlay(p1, p2, p3, r, g, b, a, noDepthTest, duration);
	}

	void AddTextOverlay(const Vector& origin, float duration, const char *format, ...)
	{
		va_start( m_argptr, format );
		vsprintf( m_text, format, m_argptr );
		va_end( m_argptr );

		CDebugOverlay::AddTextOverlay(origin, duration, m_text);
	}

	void AddTextOverlay(const Vector& origin, int line_offset, float duration, const char *format, ...)
	{
		va_start( m_argptr, format );
		vsprintf( m_text, format, m_argptr );
		va_end( m_argptr );

		CDebugOverlay::AddTextOverlay(origin, line_offset, duration, m_text);
	}

	int ScreenPosition(const Vector& point, Vector& screen)
	{
		return CDebugOverlay::ScreenPosition( point, screen );
	}

	int ScreenPosition(float flXPos, float flYPos, Vector& screen)
	{
		return CDebugOverlay::ScreenPosition( flXPos, flYPos, screen );
	}

	virtual OverlayText_t *GetFirst( void )
	{
		return CDebugOverlay::s_pOverlayText;
	}

	virtual OverlayText_t *GetNext( OverlayText_t *current )
	{
		return current->nextOverlayText;
	}

	virtual void ClearDeadOverlays( void )
	{
		CDebugOverlay::ClearDeadOverlays();
	}
};

EXPOSE_SINGLE_INTERFACE( CIVDebugOverlay, IVDebugOverlay, VDEBUG_OVERLAY_INTERFACE_VERSION );
