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

#ifndef COLLISIONMODEL_H
#define COLLISIONMODEL_H
#pragma once

extern int Cmd_CollisionText( void );
extern int Cmd_CollisionModel( bool separateJoints );
extern int Cmd_CollisionText( void );

// execute after simplification, before writing
extern void BuildCollisionModel( void );
// execute during writing
extern void WriteCollisionModel( long checkSum );

#endif // COLLISIONMODEL_H
