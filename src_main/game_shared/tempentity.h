//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef TEMPENTITY_H
#define TEMPENTITY_H
#ifdef _WIN32
#pragma once
#endif

#define TE_EXPLFLAG_NONE		0	// all flags clear makes default Half-Life explosion
#define TE_EXPLFLAG_NOADDITIVE	1	// sprite will be drawn opaque (ensure that the sprite you send is a non-additive sprite)
#define TE_EXPLFLAG_NODLIGHTS	2	// do not render dynamic lights
#define TE_EXPLFLAG_NOSOUND		4	// do not play client explosion sound
#define TE_EXPLFLAG_NOPARTICLES	8	// do not draw particles
#define TE_EXPLFLAG_DRAWALPHA	16	// sprite will be drawn alpha
#define TE_EXPLFLAG_ROTATE		32	// rotate the sprite randomly
#define TE_EXPLFLAG_NOFIREBALL	64	// do not draw a fireball

#define	TE_BEAMPOINTS		0		// beam effect between two points
#define TE_SPRITE			1	// additive sprite, plays 1 cycle
#define TE_BEAMDISK			2	// disk that expands to max radius over lifetime
#define TE_BEAMCYLINDER		3		// cylinder that expands to max radius over lifetime
#define TE_BEAMFOLLOW		4		// create a line of decaying beam segments until entity stops moving
#define TE_BEAMRING			5		// connect a beam ring to two entities
#define TE_BEAMSPLINE		6		
#define TE_BEAMRINGPOINT	7
#define	TE_BEAMLASER		8		// Fades according to viewpoint
#define TE_BEAMTESLA		9


#endif // TEMPENTITY_H
