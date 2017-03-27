//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
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
#if !defined( PREDICTION_PRIVATE_H )
#define PREDICTION_PRIVATE_H
#ifdef _WIN32
#pragma once
#endif

// point on plane side epsilon
#define	ON_EPSILON		0.1			

#define	MAX_FORWARD	6

// Use smoothing if error is > this
#define	MIN_CORRECTION_DISTANCE		0.25f

// Complain if error is > this and we have cl_showerror set
#define MIN_PREDICTION_EPSILON		0.5f

// Above this is assumed to be a teleport, don't smooth, etc.
#define MAX_PREDICTION_ERROR		64.0f

#endif // PREDICTION_PRIVATE_H