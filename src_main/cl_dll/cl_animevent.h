//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Hold definitions for all client animation events
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#if !defined( CL_ANIMEVENT_H )
#define CL_ANIMEVENT_H
#ifdef _WIN32
#pragma once
#endif

//Animation event codes
#define CL_EVENT_MUZZLEFLASH0		5001	// Muzzleflash on attachment 0
#define CL_EVENT_MUZZLEFLASH1		5011	// Muzzleflash on attachment 1
#define CL_EVENT_MUZZLEFLASH2		5021	// Muzzleflash on attachment 2
#define CL_EVENT_MUZZLEFLASH3		5031	// Muzzleflash on attachment 3
#define CL_EVENT_SPARK0				5002	// Spark on attachment 0
#define CL_EVENT_NPC_MUZZLEFLASH0	5003	// Muzzleflash on attachment 0 for third person views
#define CL_EVENT_NPC_MUZZLEFLASH1	5013	// Muzzleflash on attachment 1 for third person views
#define CL_EVENT_NPC_MUZZLEFLASH2	5023	// Muzzleflash on attachment 2 for third person views
#define CL_EVENT_NPC_MUZZLEFLASH3	5033	// Muzzleflash on attachment 3 for third person views
#define CL_EVENT_SOUND				5004	// Emit a sound 
#define CL_EVENT_EJECTBRASS1		6001	// Eject a brass shell from attachment 1

#define CL_EVENT_DISPATCHEFFECT0	9001	// Hook into a DispatchEffect on attachment 0
#define CL_EVENT_DISPATCHEFFECT1	9011	// Hook into a DispatchEffect on attachment 1
#define CL_EVENT_DISPATCHEFFECT2	9021	// Hook into a DispatchEffect on attachment 2
#define CL_EVENT_DISPATCHEFFECT3	9031	// Hook into a DispatchEffect on attachment 3
#define CL_EVENT_DISPATCHEFFECT4	9041	// Hook into a DispatchEffect on attachment 4
#define CL_EVENT_DISPATCHEFFECT5	9051	// Hook into a DispatchEffect on attachment 5
#define CL_EVENT_DISPATCHEFFECT6	9061	// Hook into a DispatchEffect on attachment 6
#define CL_EVENT_DISPATCHEFFECT7	9071	// Hook into a DispatchEffect on attachment 7
#define CL_EVENT_DISPATCHEFFECT8	9081	// Hook into a DispatchEffect on attachment 8
#define CL_EVENT_DISPATCHEFFECT9	9091	// Hook into a DispatchEffect on attachment 9

 
//Muzzle flash definitions
#define	MUZZLEFLASH_AR2				0
#define	MUZZLEFLASH_SHOTGUN			1
#define	MUZZLEFLASH_SMG1			2
#define	MUZZLEFLASH_SMG2			3
#define	MUZZLEFLASH_PISTOL			4

#endif // CL_ANIMEVENT_H