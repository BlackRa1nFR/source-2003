//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Namespace for functions dealing with Debug Overlays
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "mathlib.h"
#include "player.h"
#include "ndebugoverlay.h"
#include "wcedit.h"

#ifdef _LINUX
#include "ai_basenpc.h"
#include "ai_network.h"
#include "ai_networkmanager.h"
#endif

#define		NUM_DEBUG_OVERLAY_LINES		20
#define		MAX_OVERLAY_DIST_SQR		90000000

int				m_nDebugOverlayIndex	= -1;

OverlayLine_t*	m_debugOverlayLine[NUM_DEBUG_OVERLAY_LINES];


void NDebugOverlay::Box(const Vector &origin, const Vector &mins, const Vector &maxs, int r, int g, int b, int a, float flDuration)
{
	BoxAngles( origin, mins, maxs, vec3_angle, r, g, b, a, flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: Draw bounding box on the client
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NDebugOverlay::BoxDirection(const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &orientation, int r, int g, int b, int a, float duration)
{
	// convert forward vector to angles
	QAngle f_angles = vec3_angle;
	f_angles.y = UTIL_VecToYaw( orientation );

	BoxAngles( origin, mins, maxs, f_angles, r, g, b, a, duration );
}

void NDebugOverlay::BoxAngles(const Vector &origin, const Vector &mins, const Vector &maxs, const QAngle &angles, int r, int g, int b, int a, float duration)
{
	// --------------------------------------------------------------
	// Have to do clip the boxes before sending so we 
	// don't overflow the client message buffer 
	// --------------------------------------------------------------
	CBasePlayer *player = UTIL_PlayerByIndex(1);
	if ( !player )
		return;

	// ------------------------------------
	// Clip boxes that are far away
	// ------------------------------------
	if ((player->GetAbsOrigin() - origin).LengthSqr() > MAX_OVERLAY_DIST_SQR) 
		return;

	// ------------------------------------
	// Clip boxes that are behind the client 
	// ------------------------------------
	Vector clientForward;
	player->EyeVectors( &clientForward );

	// Build a rotation matrix from orientation
	matrix3x4_t fRotateMatrix;
	AngleMatrix(angles, fRotateMatrix);
	
	// Transform the mins and maxs
	Vector tmins, tmaxs;
	VectorRotate( mins, fRotateMatrix, tmins);
	VectorAdd(tmins,origin,tmins);
	VectorRotate( maxs, fRotateMatrix, tmaxs);
	VectorAdd(tmaxs,origin,tmaxs);

	Vector toMins  = tmins - player->GetAbsOrigin();
	Vector toMaxs  = tmaxs - player->GetAbsOrigin();
 	float  dotMins	 = DotProduct(clientForward,toMins);
 	float  dotMaxs	 = DotProduct(clientForward,toMaxs);
	if (dotMins < 0 && dotMaxs < 0)
		return;

	CSingleUserRecipientFilter user( player );

	MessageBegin( user, SVC_DEBUG_BOX_OVERLAY );
		WRITE_VEC3COORD(origin);
		WRITE_VEC3COORD(mins);
		WRITE_VEC3COORD(maxs);
		WRITE_ANGLES(angles); 
		WRITE_SHORT(r);
		WRITE_SHORT(g);
		WRITE_SHORT(b);
		WRITE_SHORT(a);
		WRITE_FLOAT(duration);
	MessageEnd();

}


//-----------------------------------------------------------------------------
// Draws a box around an entity
//-----------------------------------------------------------------------------
void NDebugOverlay::EntityBounds( CBaseEntity *pEntity, int r, int g, int b, int a, float flDuration )
{
	// FIXME: Used OBB or AABB depending on the solid type
	if ( pEntity->IsBoundsDefinedInEntitySpace() )
	{
		BoxAngles( pEntity->GetAbsOrigin(), pEntity->EntitySpaceMins(), pEntity->EntitySpaceMaxs(), pEntity->GetAbsAngles(), r, g, b, a, flDuration );
	}
	else
	{
		BoxAngles( pEntity->GetAbsOrigin(), pEntity->WorldAlignMins(), pEntity->WorldAlignMaxs(), vec3_angle, r, g, b, a, flDuration );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Send debug overlay line to the client
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NDebugOverlay::Line( const Vector &origin, const Vector &target, int r, int g, int b, bool noDepthTest, float duration )
{
	// --------------------------------------------------------------
	// Clip the line before sending so we 
	// don't overflow the client message buffer
	// --------------------------------------------------------------
	CBasePlayer *player = UTIL_PlayerByIndex(1);
	if ( !player )
		return;

	// ------------------------------------
	// Clip line that is far away
	// ------------------------------------
	if (((player->GetAbsOrigin() - origin).LengthSqr() > MAX_OVERLAY_DIST_SQR) &&
		((player->GetAbsOrigin() - target).LengthSqr() > MAX_OVERLAY_DIST_SQR) ) 
		return;

	// ------------------------------------
	// Clip line that is behind the client 
	// ------------------------------------
	Vector clientForward;
	player->EyeVectors( &clientForward );

	Vector toOrigin		= origin - player->GetAbsOrigin();
	Vector toTarget		= target - player->GetAbsOrigin();
 	float  dotOrigin	= DotProduct(clientForward,toOrigin);
 	float  dotTarget	= DotProduct(clientForward,toTarget);
	if (dotOrigin < 0 && dotTarget < 0) 
	{
		return;
	}

	CSingleUserRecipientFilter user( player );

	MessageBegin( user, SVC_DEBUG_LINE_OVERLAY );
		WRITE_VEC3COORD(origin);
		WRITE_VEC3COORD(target);
		WRITE_SHORT(r);
		WRITE_SHORT(g);
		WRITE_SHORT(b);
		WRITE_SHORT(noDepthTest);
		WRITE_FLOAT(duration);
	MessageEnd();

}


//-----------------------------------------------------------------------------
// Purpose: Send debug overlay line to the client
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NDebugOverlay::Triangle( const Vector &p1, const Vector &p2, const Vector &p3, 
				int r, int g, int b, int a, bool noDepthTest, float duration )
{
	// --------------------------------------------------------------
	// Have to do clip the boxes before sending so we 
	// don't overflow the client message buffer 
	// --------------------------------------------------------------
	CBasePlayer *player = UTIL_PlayerByIndex(1);
	if ( !player )
		return;

	Vector to1 = p1 - player->GetAbsOrigin();
	Vector to2 = p2 - player->GetAbsOrigin();
	Vector to3 = p3 - player->GetAbsOrigin();

	// ------------------------------------
	// Clip triangles that are far away
	// ------------------------------------
	if ((to1.LengthSqr() > MAX_OVERLAY_DIST_SQR) && 
		(to2.LengthSqr() > MAX_OVERLAY_DIST_SQR) && 
		(to3.LengthSqr() > MAX_OVERLAY_DIST_SQR))
	{
		return;
	}

	// ------------------------------------
	// Clip triangles that are behind the client 
	// ------------------------------------
	Vector clientForward;
	player->EyeVectors( &clientForward );
	
 	float  dot1 = DotProduct(clientForward, to1);
 	float  dot2 = DotProduct(clientForward, to2);
 	float  dot3 = DotProduct(clientForward, to3);
	if (dot1 < 0 && dot2 < 0 && dot3 < 0) 
	{
		return;
	}

	CSingleUserRecipientFilter user( player );

	MessageBegin( user, SVC_DEBUG_TRIANGLE_OVERLAY );
		WRITE_VEC3COORD(p1);
		WRITE_VEC3COORD(p2);
		WRITE_VEC3COORD(p3);
		WRITE_SHORT(r);
		WRITE_SHORT(g);
		WRITE_SHORT(b);
		WRITE_SHORT(a);
		WRITE_SHORT(noDepthTest);
		WRITE_FLOAT(duration);
	MessageEnd();
}


//-----------------------------------------------------------------------------
// Purpose: Send debug overlay text to the client
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NDebugOverlay::EntityText( int entityID, int text_offset, const char *text, float duration, int r, int g, int b, int a )
{
	CBroadcastRecipientFilter filter;
	MessageBegin( filter, SVC_DEBUG_ENTITYTEXT_OVERLAY);
		WRITE_SHORT(entityID);
		WRITE_SHORT(text_offset);
		WRITE_FLOAT(duration);
		WRITE_SHORT(r);
		WRITE_SHORT(g);
		WRITE_SHORT(b);
		WRITE_SHORT(a);
		WRITE_STRING(text);
	MessageEnd();

}

//-----------------------------------------------------------------------------
// Purpose: Send grid overlay text to the client
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NDebugOverlay::Grid( const Vector &vPosition )
{
	CBroadcastRecipientFilter filter;
	MessageBegin( filter, SVC_DEBUG_GRID_OVERLAY);
		WRITE_VEC3COORD(vPosition);
	MessageEnd();

}

//-----------------------------------------------------------------------------
// Purpose: Send debug overlay text to the client
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NDebugOverlay::Text( const Vector &origin, const char *text, bool bViewCheck, float duration )
{
	CBasePlayer *player = UTIL_PlayerByIndex(1);
	if ( !player )
		return;

	// ------------------------------------
	// Clip text that is far away
	// ------------------------------------
	if ((player->GetAbsOrigin() - origin).LengthSqr() > 4000000) 
		return;

	// ------------------------------------
	// Clip text that is behind the client 
	// ------------------------------------
	Vector clientForward;
	player->EyeVectors( &clientForward );

	Vector toText	= origin - player->GetAbsOrigin();
 	float  dotPr	= DotProduct(clientForward,toText);
	if (dotPr < 0) 
	{
		return;
	}

	if (bViewCheck)
	{
		trace_t tr;
		UTIL_TraceLine(player->GetAbsOrigin(), origin, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr);
		if ((tr.endpos - origin).Length() > 10)
		{
			return;
		}
	}

	CSingleUserRecipientFilter user( player );

	MessageBegin( user, SVC_DEBUG_TEXT_OVERLAY );
		WRITE_VEC3COORD(origin);
		WRITE_FLOAT(duration);
		WRITE_STRING(text);
	MessageEnd();

}

//-----------------------------------------------------------------------------
// Purpose: Send debug overlay text with screen position to the client
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NDebugOverlay::ScreenText( float flXpos, float flYpos, const char *text, int r, int g, int b, int a, float duration)
{
	// --------------------------------------------------------------
	// Clip the text before sending so we 
	// don't overflow the client message buffer
	// --------------------------------------------------------------
	CBasePlayer *pClient = UTIL_PlayerByIndex(1);
	if ( !pClient )
		return;

	CSingleUserRecipientFilter user( pClient );

	MessageBegin( user, SVC_DEBUG_SCREENTEXT_OVERLAY );
		WRITE_FLOAT(flXpos);
		WRITE_FLOAT(flYpos);
		WRITE_FLOAT(duration);
		WRITE_SHORT(r);
		WRITE_SHORT(g);
		WRITE_SHORT(b);
		WRITE_SHORT(a);
		WRITE_STRING(text);
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: Draw a colored 3D cross of the given hull size at the given position
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NDebugOverlay::Cross3D(const Vector &position, const Vector &mins, const Vector &maxs, int r, int g, int b, bool noDepthTest, float fDuration )
{
	// Offset since min and max z in not about center
	Vector new_pos = position;
	new_pos.z -= (maxs.z * 0.5);

	Vector start = mins + new_pos;
	Vector end   = maxs + new_pos;
	NDebugOverlay::Line(start,end, r, g, b, noDepthTest,fDuration);

	start.x += (maxs.x - mins.x);
	end.x	-= (maxs.x - mins.x);
	NDebugOverlay::Line(start,end, r, g, b, noDepthTest,fDuration);

	start.y += (maxs.y - mins.y);
	end.y	-= (maxs.y - mins.y);
	NDebugOverlay::Line(start,end, r, g, b, noDepthTest,fDuration);

	start.x -= (maxs.x - mins.x);
	end.x	+= (maxs.x - mins.x);
	NDebugOverlay::Line(start,end, r, g, b, noDepthTest,fDuration);
}

//-----------------------------------------------------------------------------
// Purpose: Return the old overlay line from the overlay pool
// Input  :
// Output :
//-----------------------------------------------------------------------------
OverlayLine_t*	GetDebugOverlayLine(void)
{
	// Make casing pool if not initialized
	if (m_nDebugOverlayIndex == -1)
	{
		for (int i=0;i<NUM_DEBUG_OVERLAY_LINES;i++)
		{
			m_debugOverlayLine[i]				= new OverlayLine_t;
			m_debugOverlayLine[i]->noDepthTest  = true;
			m_debugOverlayLine[i]->draw			= false;
		}
		m_nDebugOverlayIndex = 0;
	}

	int id;

	m_nDebugOverlayIndex++;
	if (m_nDebugOverlayIndex == NUM_DEBUG_OVERLAY_LINES)
	{
		m_nDebugOverlayIndex = 0;
		id			  = NUM_DEBUG_OVERLAY_LINES-1;

	}
	else
	{
		id = m_nDebugOverlayIndex-1;
	}
	return m_debugOverlayLine[id];
}

//-----------------------------------------------------------------------------
// Purpose: Adds a debug line to be drawn on the screen
// Input  : If testLOS is true, color is based on line of sight test
// Output : 
//-----------------------------------------------------------------------------
void NDebugOverlay::AddDebugLine(const Vector &startPos, const Vector &endPos, bool noDepthTest, bool testLOS) 
{
	OverlayLine_t* debugLine = GetDebugOverlayLine();

	debugLine->origin		= startPos;
	debugLine->dest			= endPos;
	debugLine->noDepthTest	= noDepthTest;
	debugLine->draw = true;

	if (testLOS)
	{
		trace_t tr;
		UTIL_TraceLine ( debugLine->origin, debugLine->dest, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );
		if (tr.startsolid || tr.fraction < 1.0)
		{
			debugLine->r = 255;
			debugLine->g = 0;
			debugLine->b = 0;
			return;
		}
	}

	debugLine->r = 255;
	debugLine->g = 255;
	debugLine->b = 255;
}


//--------------------------------------------------------------------------------
// Purpose : Draw tick marks between start and end position of the given distance
//			 with text every tickTextDist steps apart. 
// Input   :
// Output  :
//--------------------------------------------------------------------------------
void NDebugOverlay::DrawTickMarkedLine(const Vector &startPos, const Vector &endPos, float tickDist, int tickTextDist, int r, int g, int b, bool noDepthTest, float duration )
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex(CBaseEntity::m_nDebugPlayer);

	if (!pPlayer) 
	{
		return;
	}
	

	Vector	lineDir		= (endPos - startPos);
	float	lineDist	= VectorNormalize( lineDir );
	int		numTicks	= lineDist/tickDist;
	Vector	vBodyDir	= pPlayer->BodyDirection2D( );
	Vector  upVec		= 4*vBodyDir;
	Vector	sideDir;
	Vector	tickPos		= startPos;
	int		tickTextCnt = 0;

	CrossProduct(lineDir, upVec, sideDir);

	// First draw the line
	NDebugOverlay::Line(startPos, endPos, r,g,b,noDepthTest,duration);

	// Now draw the ticks
	for (int i=0;i<numTicks+1;i++)
	{
		// Draw tick mark
		Vector tickLeft	 = tickPos - sideDir;
		Vector tickRight = tickPos + sideDir;
		
		// Draw tick mark text
		if (tickTextCnt == tickTextDist)
		{
			char text[25];
			Q_snprintf(text,sizeof(text),"%i",i);
			Vector textPos = tickLeft + Vector(0,0,8);
			NDebugOverlay::Line(tickLeft, tickRight, 255,255,255,noDepthTest,duration);
			NDebugOverlay::Text( textPos, text, true, 0 );
			tickTextCnt = 0;
		}
		else
		{
			NDebugOverlay::Line(tickLeft, tickRight, r,g,b,noDepthTest,duration);
		}
		tickTextCnt++;

		tickPos = tickPos + (tickDist * lineDir);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns z value of floor below given point (up to 384 inches below)
// Input  :
// Output :
//-----------------------------------------------------------------------------
float GetLongFloorZ(const Vector &origin) 
{
	// trace to the ground, then pop up 8 units and place node there to make it
	// easier for them to connect (think stairs, chairs, and bumps in the floor).
	// After the routing is done, push them back down.
	//
	trace_t	tr;
	UTIL_TraceLine ( origin,
					 origin - Vector ( 0, 0, 2048 ),
					 MASK_NPCSOLID_BRUSHONLY,
					 NULL,
					 COLLISION_GROUP_NONE,
					 &tr );

	// This trace is ONLY used if we hit an entity flagged with FL_WORLDBRUSH
	trace_t	trEnt;
	UTIL_TraceLine ( origin,
					 origin - Vector ( 0, 0, 2048 ),
					 MASK_NPCSOLID,
					 NULL,
					 COLLISION_GROUP_NONE,
					 &trEnt );

	
	// Did we hit something closer than the floor?
	if ( trEnt.fraction < tr.fraction )
	{
		// If it was a world brush entity, copy the node location
		if ( trEnt.m_pEnt )
		{
			CBaseEntity *e = trEnt.m_pEnt;
			if ( e && (e->GetFlags() & FL_WORLDBRUSH) )
			{
				tr.endpos = trEnt.endpos;
			}
		}
	}

	return tr.endpos.z;
}

//------------------------------------------------------------------------------
// Purpose : Draws a large crosshair at flCrossDistance from the debug player
//			 with tick marks
// Input   :
// Output  :
//------------------------------------------------------------------------------
void NDebugOverlay::DrawPositioningOverlay( float flCrossDistance )
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex(CBaseEntity::m_nDebugPlayer);

	if (!pPlayer) 
	{
		return;
	}

	Vector pRight;
	pPlayer->EyeVectors( NULL, &pRight, NULL );

#ifdef _WIN32
	Vector topPos		= NWCEdit::AirNodePlacementPosition();
#else
        Vector pForward;
        pPlayer->EyeVectors( &pForward );

        Vector  floorVec = pForward;
        floorVec.z = 0;
        VectorNormalize( floorVec );
        VectorNormalize( pForward );

        float cosAngle = DotProduct(floorVec,pForward);

        float lookDist = g_pAINetworkManager->GetEditOps()->m_flAirEditDistance/cosAngle;
        Vector topPos = pPlayer->EyePosition()+pForward * lookDist;
#endif

	Vector bottomPos	= topPos;
	bottomPos.z			= GetLongFloorZ(bottomPos);

	// Make sure we can see the target pos
	trace_t tr;
	Vector	endPos;
	UTIL_TraceLine(pPlayer->EyePosition(), topPos, MASK_NPCSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction == 1.0)
	{
		Vector rightTrace = topPos + pRight*400;
		float  traceLen	  = (topPos - rightTrace).Length();
		UTIL_TraceLine(topPos, rightTrace, MASK_NPCSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr);
		endPos = topPos+(pRight*traceLen*tr.fraction);
		NDebugOverlay::DrawTickMarkedLine(topPos, endPos, 24.0, 5, 255,0,0,false,0);

		Vector leftTrace	= topPos - pRight*400;
		traceLen			= (topPos - leftTrace).Length();
		UTIL_TraceLine(topPos, leftTrace, MASK_NPCSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr);
		endPos				= topPos-(pRight*traceLen*tr.fraction);
		NDebugOverlay::DrawTickMarkedLine(topPos, endPos, 24.0, 5, 255,0,0,false,0);

		Vector upTrace		= topPos + Vector(0,0,1)*400;
		traceLen			= (topPos - upTrace).Length();
		UTIL_TraceLine(topPos, upTrace, MASK_NPCSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr);
		endPos				= topPos+(Vector(0,0,1)*traceLen*tr.fraction);
		NDebugOverlay::DrawTickMarkedLine(bottomPos, endPos, 24.0, 5, 255,0,0,false,0);

		// Draw white cross at center
		Cross3D(topPos, Vector(-2,-2,-2), Vector(2,2,2), 255,255,255, true, 0);
	}
	else
	{
		// Draw warning  cross at center
		Cross3D(topPos, Vector(-2,-2,-2), Vector(2,2,2), 255,100,100, true, 0 );
	}
	/*  Don't show distance for now.
	char text[25];
	Q_snprintf(text,sizeof(text),"%3.0f",flCrossDistance/12.0);
	Vector textPos = topPos - pRight*16 + Vector(0,0,10);
	NDebugOverlay::Text( textPos, text, true, 0 );
	*/
}

//------------------------------------------------------------------------------
// Purpose : Draw crosshair on ground where player is looking
// Input   :
// Output  :
//------------------------------------------------------------------------------
void NDebugOverlay::DrawGroundCrossHairOverlay(void)
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex(CBaseEntity::m_nDebugPlayer);

	if (!pPlayer) 
	{
		return;
	}

	// Trace a line to where player is looking
	Vector vForward;
	Vector vSource = pPlayer->EyePosition();
	pPlayer->EyeVectors( &vForward );

	trace_t tr;
	UTIL_TraceLine ( vSource, vSource + vForward * 2048, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
	float dotPr = DotProduct(Vector(0,0,1),tr.plane.normal);
	if (tr.fraction != 1.0 &&  dotPr > 0.5)
	{
		tr.endpos.z += 1;
		float	scale	 = 6;
		Vector	startPos = tr.endpos + Vector (-scale,0,0);
		Vector	endPos	 = tr.endpos + Vector ( scale,0,0);
		Line(startPos, endPos, 255, 0, 0,false,0);

		startPos = tr.endpos + Vector (0,-scale,0);
		endPos	 = tr.endpos + Vector (0, scale,0);
		Line(startPos, endPos, 255, 0, 0,false,0);
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void NDebugOverlay::DrawOverlayLines(void) 
{
	if (m_nDebugOverlayIndex != -1)
	{
		for (int i=0;i<NUM_DEBUG_OVERLAY_LINES;i++)
		{
			if (m_debugOverlayLine[i]->draw)
			{
				NDebugOverlay::Line(m_debugOverlayLine[i]->origin,
									 m_debugOverlayLine[i]->dest,
									 m_debugOverlayLine[i]->r,
									 m_debugOverlayLine[i]->g,
									 m_debugOverlayLine[i]->b,
									 m_debugOverlayLine[i]->noDepthTest,0);
			}
		}
	}
}

