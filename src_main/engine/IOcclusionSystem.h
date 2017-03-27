//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// This file contains code to perform dynamic occlusion in the engine
//=============================================================================

#ifndef IOCCLUSIONSYSTEM_H
#define	IOCCLUSIONSYSTEM_H

#ifdef _WIN32
#pragma once
#endif


class Vector;
class VMatrix;
struct model_t;
class VPlane;
class CUtlBuffer;


//-----------------------------------------------------------------------------
// Occlusion system interface
//-----------------------------------------------------------------------------
class IOcclusionSystem
{
public:
	// Activate/deactivate an occluder brush model
	virtual void ActivateOccluder( int nOccluderIndex, bool bActive ) = 0;

	// Sets the view transform
	virtual void SetView( const Vector &vecCameraPos, float flFOV, const VMatrix &worldToCamera, 
		const VMatrix &cameraToProjection, const VPlane &nearClipPlane ) = 0;

	// Test for occlusion (bounds specified in abs space)
	virtual bool IsOccluded( const Vector &vecAbsMins, const Vector &vecAbsMaxs ) = 0;
};


//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
IOcclusionSystem *OcclusionSystem();


#endif // IOCCLUSIONSYSTEM_H
