//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VPHYSICS_SOUND_H
#define VPHYSICS_SOUND_H
#ifdef _WIN32
#pragma once
#endif

#include "SoundEmitterSystemBase.h"

namespace physicssound
{
	struct impactsound_t
	{
		void			*pGameData;
		int				entityIndex;
		int				soundChannel;
		IPhysicsObject *pObject;
		float			volume;
		int				surfaceProps;
	};

	// UNDONE: Use a sorted container and sort by volume/distance?
	struct soundlist_t
	{
		CUtlVector<impactsound_t>	elements;
		impactsound_t	&GetElement(int index) { return elements[index]; }
		impactsound_t	&AddElement() { return elements[elements.AddToTail()]; }
		int Count() { return elements.Count(); }
		void RemoveAll() { elements.RemoveAll(); }
	};

	void PlayImpactSounds( soundlist_t &list )
	{
		for ( int i = list.Count()-1; i >= 0; --i )
		{
			impactsound_t &sound = list.GetElement(i);
			surfacedata_t *psurf = physprops->GetSurfaceData( sound.surfaceProps );
			int offset = 0;
			if ( psurf->impactCount )
			{
				offset = random->RandomInt(0, psurf->impactCount-1);
			}
			const char *pSound = physprops->GetString( psurf->impact, offset );

			CSoundParameters params;
			if ( !CBaseEntity::GetParametersForSound( pSound, params ) )
				break;

			Vector origin;
			sound.pObject->GetPosition( &origin, NULL );
			if ( sound.volume > 1 )
				sound.volume = 1;
			CPASAttenuationFilter filter( origin, params.soundlevel );
			// JAY: If this entity gets deleted, the sound comes out at the world origin
			// this sounds bad!  Play on ent 0 for now.
			CBaseEntity::EmitSound( filter, 0 /*sound.entityIndex*/, sound.soundChannel, params.soundname, params.volume * sound.volume, params.soundlevel, 
				0, params.pitch, &origin );
		}
		list.RemoveAll();
	}
	void AddImpactSound( soundlist_t &list, void *pGameData, int entityIndex, int soundChannel, IPhysicsObject *pObject, int surfaceProps, float volume )
	{
		for ( int i = list.Count()-1; i >= 0; --i )
		{
			impactsound_t &sound = list.GetElement(i);
			// UNDONE: Compare entity or channel somehow?
			// UNDONE: Doing one slot per entity is too noisy.  So now we use one slot per material
			if ( surfaceProps == sound.surfaceProps )
			{
				// UNDONE: Store instance volume separate from aggregate volume and compare that?
				if ( volume > sound.volume )
				{
					sound.pObject = pObject;
					sound.pGameData = pGameData;
					sound.entityIndex = entityIndex;
					sound.soundChannel = soundChannel;
				}
				sound.volume += volume;
				return;
			}
		}

		impactsound_t &sound = list.AddElement();
		sound.pGameData = pGameData;
		sound.entityIndex = entityIndex;
		sound.soundChannel = soundChannel;
		sound.pObject = pObject;
		sound.surfaceProps = surfaceProps;
		sound.volume = volume;
	}
};


#endif // VPHYSICS_SOUND_H
