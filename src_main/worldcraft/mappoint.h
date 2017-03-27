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

#ifndef MAPPOINT_H
#define MAPPOINT_H
#pragma once


#include "MapAtom.h"


class CMapPoint : public CMapAtom
{
	public:

		CMapPoint(void);

		virtual void GetOrigin(Vector& pfOrigin);
		virtual void SetOrigin(Vector& pfOrigin);
		
	protected:

		void DoTransform(Box3D *t);
		void DoTransMove(const Vector &Delta);
		void DoTransRotate(const Vector *pRefPoint, const QAngle &Angles);
		void DoTransScale(const Vector &RefPoint, const Vector &Scale);
		void DoTransFlip(const Vector &RefPoint);

		Vector m_Origin;
};


#endif // MAPPOINT_H
