//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include <stdio.h>
#include "vphysics_interface.h"
#include "physics_environment.h"
#include "physics_material.h"
#include "physics_object.h"
#include "physics_constraint.h"
#include "physics_spring.h"
#include "physics_fluid.h"
#include "physics_shadow.h"
#include "physics_motioncontroller.h"
#include "physics_vehicle.h"
#include "convert.h"
#include "utlvector.h"
#include "vphysics/constraints.h"
#include "vphysics/vehicles.h"
#include "vphysics_saverestore.h"
#include "vphysics_internal.h"

#include "ivp_physics.hxx"
#include "ivu_linear_macros.hxx"
#include "ivp_templates.hxx"
#include "ivp_collision_filter.hxx"
#include "ivp_listener_collision.hxx"
#include "ivp_listener_object.hxx"
#include "ivp_mindist.hxx"
#include "ivp_core.hxx"
#include "ivp_friction.hxx"
#include "ivp_anomaly_manager.hxx"
#include "ivp_time.hxx"
#include "ivp_listener_psi.hxx"
#include "ivp_phantom.hxx"

#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IVP_Synapse_Friction *GetOppositeSynapse( IVP_Synapse_Friction *pfriction )
{
	IVP_Contact_Point *contact = pfriction->get_contact_point();
	IVP_Synapse_Friction *ptest = contact->get_synapse(0);
	if ( ptest == pfriction )
	{
		ptest = contact->get_synapse(1);
	}

	return ptest;
}

IVP_Real_Object *GetOppositeSynapseObject( IVP_Synapse_Friction *pfriction )
{
	IVP_Synapse_Friction *opposite = GetOppositeSynapse( pfriction );
	return opposite->get_object();
}

// simple delete queue
class IDeleteQueueItem
{
public:
	virtual void Delete() = 0;
};

template <typename T>
class CDeleteProxy : public IDeleteQueueItem
{
public:
	CDeleteProxy(T *pItem) : m_pItem(pItem) {}
	virtual void Delete() { delete m_pItem; }
private:
	T *m_pItem;
};

class CDeleteQueue
{
public:
	void Add( IDeleteQueueItem *pItem )
	{
		m_list.AddToTail( pItem );
	}

	template <typename T>
	void QueueForDelete( T *pItem )
	{
		Add( new CDeleteProxy<T>(pItem) );
	}
	void DeleteAll()
	{
		for ( int i = m_list.Count()-1; i >= 0; --i)
		{
			m_list[i]->Delete();
			delete m_list[i];
		}
		m_list.RemoveAll();
	}

private:
	CUtlVector< IDeleteQueueItem * >	m_list;
};

class CPhysicsCollisionData : public IPhysicsCollisionData
{
public:
	CPhysicsCollisionData( IVP_Contact_Situation *contact ) : m_pContact(contact) {}

	virtual void GetSurfaceNormal( Vector &out ) { ConvertDirectionToHL( m_pContact->surf_normal, out ); }
	virtual void GetContactPoint( Vector &out ) { ConvertPositionToHL( m_pContact->contact_point_ws, out ); }
	virtual void GetContactSpeed( Vector &out ) { ConvertPositionToHL( m_pContact->speed, out ); }

	const IVP_Contact_Situation *m_pContact;
};

class CPhysicsFrictionData : public IPhysicsCollisionData
{
public:
	CPhysicsFrictionData( IVP_Synapse_Friction *synapse )
	{
		m_pPoint = synapse->get_contact_point(); 
		m_pContact = NULL;
	}
	CPhysicsFrictionData( IVP_Event_Friction *pEvent )
	{
		m_pPoint = pEvent->friction_handle;
		m_pContact = pEvent->contact_situation;
	}

	virtual void GetSurfaceNormal( Vector &out ) 
	{ 
		if ( m_pContact )
		{
			ConvertDirectionToHL( m_pContact->surf_normal, out ); 
		}
		else
		{
			out.Init();
		}
	}
	virtual void GetContactPoint( Vector &out ) 
	{
		if ( m_pContact )
		{
			ConvertPositionToHL( m_pContact->contact_point_ws, out ); 
		}
		else
		{
			ConvertPositionToHL( *m_pPoint->get_contact_point_ws(), out ); 
		}
	}
	virtual void GetContactSpeed( Vector &out ) 
	{
		if ( m_pContact )
		{
			ConvertPositionToHL( m_pContact->speed, out );
		}
		else
		{
			out.Init();
		}
	}

	const IVP_Contact_Point *m_pPoint;
	const IVP_Contact_Situation *m_pContact;
};


//-----------------------------------------------------------------------------
// Purpose: Routes object event callbacks to game code
//-----------------------------------------------------------------------------
class CSleepObjects : public IVP_Listener_Object
{
public:
	CSleepObjects( void ) : IVP_Listener_Object() 
	{
		m_pCallback = NULL;
	}

	void SetHandler( IPhysicsObjectEvent *pListener )
	{
		m_pCallback = pListener;
	}

	void Remove( int index )
	{
		// fast remove preserves indices except for the last element (moved into the empty spot)
		m_activeObjects.FastRemove(index);
		// If this isn't the last element, shift its index over
		if ( index < m_activeObjects.Count() )
		{
			m_activeObjects[index]->SetActiveIndex( index );
		}
	}

	void DeleteObject( CPhysicsObject *pObject )
	{
		int index = pObject->GetActiveIndex();
		if ( index < m_activeObjects.Count() )
		{
			Assert( m_activeObjects[index] == pObject );
			Remove( index );
			pObject->SetActiveIndex( 0xFFFF );
		}
		else
		{
			Assert(index==0xFFFF);
		}
				
	}

    void event_object_deleted( IVP_Event_Object *pEvent )
	{
		CPhysicsObject *pObject = static_cast<CPhysicsObject *>(pEvent->real_object->client_data);
		if ( !pObject )
			return;

		DeleteObject(pObject);
	}

    void event_object_created( IVP_Event_Object *pEvent )
	{
	}

    void event_object_revived( IVP_Event_Object *pEvent )
	{
		CPhysicsObject *pObject = static_cast<CPhysicsObject *>(pEvent->real_object->client_data);
		if ( !pObject )
			return;

		int sleepState = pObject->GetSleepState();
		
		pObject->NotifyWake();

		// asleep, but already in active list
		if ( sleepState == OBJ_STARTSLEEP )
			return;

		// don't track static objects (like the world).  That way we only track objects that will move
		if ( pObject->GetObject()->get_movement_state() != IVP_MT_STATIC )
		{
			Assert(pObject->GetActiveIndex()==0xFFFF);
			if ( pObject->GetActiveIndex()!=0xFFFF)
				return;

			int index = m_activeObjects.AddToTail( pObject );
			pObject->SetActiveIndex( index );
		}
		if ( m_pCallback )
		{
			m_pCallback->ObjectWake( pObject );
		}
	}

    void event_object_frozen( IVP_Event_Object *pEvent )
	{
		CPhysicsObject *pObject = static_cast<CPhysicsObject *>(pEvent->real_object->client_data);
		if ( !pObject )
			return;

		pObject->NotifySleep();
		if ( m_pCallback )
		{
			m_pCallback->ObjectSleep( pObject );
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: This walks the objects in the environment and generates friction events
	//			for any scraping that is occurring.
	//-----------------------------------------------------------------------------
	void ProcessActiveObjects( IVP_Environment *pEnvironment, IPhysicsCollisionEvent *pEvent )
	{
		// FIXME: Is this correct? Shouldn't it do next PSI - lastScrape?
		float nextTime = pEnvironment->get_old_time_of_last_PSI().get_time();
		float delta = nextTime - m_lastScrapeTime;

		// only process if we have done a PSI
		if ( delta < pEnvironment->get_delta_PSI_time() )
			return;

		float t = 1.0f;
		t /= delta;
		if ( t > 10.0f )
			t = 10.0f;

		m_lastScrapeTime = nextTime;

		// UNDONE: This only calls friciton for one object in each pair.
		// UNDONE: Split energy in half and call for both objects?
		// UNDONE: Don't split/call if one object is static (like the world)?
		for ( int i = 0; i < m_activeObjects.Count(); i++ )
		{
			CPhysicsObject *pObject = m_activeObjects[i];
			IVP_Real_Object *ivpObject = pObject->GetObject();
			
			// no friction callbacks for this object
			if ( ! (pObject->GetCallbackFlags() & CALLBACK_GLOBAL_FRICTION) )
				continue;

			// UNDONE: IVP_Synapse_Friction is supposed to be opaque.  Is there a better way
			// to implement this?  Using the friction listener is much more work for the CPU
			// and considers sleeping objects.
			IVP_Synapse_Friction *pfriction = ivpObject->get_first_friction_synapse();
			while ( pfriction )
			{
				IVP_Contact_Point *contact = pfriction->get_contact_point();
				IVP_Synapse_Friction *pOpposite = GetOppositeSynapse( pfriction );
				IVP_Real_Object *pobj = pOpposite->get_object();
				CPhysicsObject *pScrape = (CPhysicsObject *)pobj->client_data;

				// friction callbacks for this object?
				if ( pScrape->GetCallbackFlags() & CALLBACK_GLOBAL_FRICTION )
				{
					float energy = IVP_Contact_Point_API::get_eliminated_energy( contact );
					if ( energy ) 
					{
						IVP_Contact_Point_API::reset_eliminated_energy( contact );
						// scrape with an estimate for the energy per unit mass
						// This assumes that the game is interested in some measure of vibration
						// for sound effects.  This also assumes that more massive objects require
						// more energy to vibrate.
						energy = energy * t * ivpObject->get_core()->get_inv_mass();

						if ( energy > 0.05f )
						{
							int hitSurface = physprops->RemapIVPMaterialIndex( pOpposite->get_material_index() );
							CPhysicsFrictionData data(pfriction);
							pEvent->Friction( pObject, ConvertEnergyToHL(energy), pObject->GetMaterialIndexInternal(), hitSurface, &data );
						}
					}
				}
				pfriction = pfriction->get_next();
			}
		}
	}
	int	GetActiveObjectCount( void )
	{
		return m_activeObjects.Count();
	}
	void GetActiveObjects( IPhysicsObject **pOutputObjectList )
	{
		for ( int i = 0; i < m_activeObjects.Count(); i++ )
		{
			pOutputObjectList[i] = m_activeObjects[i];
		}
	}
	void UpdateSleepObjects( void )
	{
		int i;

		CUtlVector<CPhysicsObject *> sleepObjects;

		for ( i = 0; i < m_activeObjects.Count(); i++ )
		{
			CPhysicsObject *pObject = m_activeObjects[i];
			
			if ( pObject->GetSleepState() != OBJ_AWAKE )
			{
				sleepObjects.AddToTail( pObject );
			}
		}

		for ( i = sleepObjects.Count()-1; i >= 0; --i )
		{
			// put fully to sleep
			sleepObjects[i]->NotifySleep();

			// remove from the active list
			DeleteObject( sleepObjects[i] );
		}
	}

private:
	CUtlVector<CPhysicsObject *>	m_activeObjects;
	float							m_lastScrapeTime;
	IPhysicsObjectEvent				*m_pCallback;
};

class CEmptyCollisionListener : public IPhysicsCollisionEvent
{
public:
	virtual void PreCollision( vcollisionevent_t *pEvent ) {}
	virtual void PostCollision( vcollisionevent_t *pEvent ) {}

	// This is a scrape event.  The object has scraped across another object consuming the indicated energy
	virtual void Friction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit, IPhysicsCollisionData *pData ) {}

	virtual void StartTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData ) {}
	virtual void EndTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData ) {}

	virtual void FluidStartTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid ) {}
	virtual void FluidEndTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid ) {}

	virtual void ObjectEnterTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject ) {}
	virtual void ObjectLeaveTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject ) {}

	virtual void PostSimulationFrame() {}
};

CEmptyCollisionListener g_EmptyCollisionListener;

#define ALL_COLLISION_FLAGS (IVP_LISTENER_COLLISION_CALLBACK_PRE_COLLISION|IVP_LISTENER_COLLISION_CALLBACK_POST_COLLISION|IVP_LISTENER_COLLISION_CALLBACK_FRICTION)
//-----------------------------------------------------------------------------
// Purpose: Routes collision event callbacks to game code
//-----------------------------------------------------------------------------
class CPhysicsListenerCollision : public IVP_Listener_Collision, public IVP_Listener_Phantom
{
public:
	CPhysicsListenerCollision();

	void SetHandler( IPhysicsCollisionEvent *pCallback )
	{
		m_pCallback = pCallback;
	}
	IPhysicsCollisionEvent *GetHandler() { return m_pCallback; }

    virtual void event_pre_collision( IVP_Event_Collision *pEvent )
	{
		m_event.isCollision = false;
		m_event.isShadowCollision = false;
		IVP_Contact_Situation *contact = pEvent->contact_situation;
		CPhysicsObject *pObject1 = static_cast<CPhysicsObject *>(contact->objects[0]->client_data);
		CPhysicsObject *pObject2 = static_cast<CPhysicsObject *>(contact->objects[1]->client_data);
		if ( !pObject1 || !pObject2 )
			return;

		unsigned int flags1 = pObject1->GetCallbackFlags();
		unsigned int flags2 = pObject2->GetCallbackFlags();

		m_event.isCollision = (flags1 & flags2 & CALLBACK_GLOBAL_COLLISION) ? true : false;
		
		// only call shadow collisions if one is shadow and the other isn't (hence the xor)
		// (if both are shadow, the collisions happen in AI - if neither, then no callback)
		m_event.isShadowCollision = ((flags1^flags2) & CALLBACK_SHADOW_COLLISION) ? true : false;

		m_event.pObjects[0] = pObject1;
		m_event.pObjects[1] = pObject2;
		m_event.deltaCollisionTime = pEvent->d_time_since_last_collision;
		// This timer must have been reset or something (constructor initializes time to -1000)
		// Fake the time to 50ms (resets happen often in rolling collisions for some reason)
		if ( m_event.deltaCollisionTime > 999 )
		{
			m_event.deltaCollisionTime = 1.0;
		}
			

		CPhysicsCollisionData data(contact);
		m_event.pInternalData = &data;

		// clear out any static object collisions unless flagged to keep them
		if ( contact->objects[0]->get_movement_state() == IVP_MT_STATIC )
		{
			// don't call global if disabled
			if ( !(flags2 & CALLBACK_GLOBAL_COLLIDE_STATIC) )
			{
				m_event.isCollision = false;
			}
		}
		if ( contact->objects[1]->get_movement_state() == IVP_MT_STATIC )
		{
			// don't call global if disabled
			if ( !(flags1 & CALLBACK_GLOBAL_COLLIDE_STATIC) )
			{
				m_event.isCollision = false;
			}
		}

		if ( !m_event.isCollision && !m_event.isShadowCollision )
			return;

		// look up surface props
		for ( int i = 0; i < 2; i++ )
		{
			m_event.surfaceProps[i] = physprops->GetIVPMaterialIndex( contact->materials[i] );
			if ( m_event.surfaceProps[i] < 0 )
			{
				m_event.surfaceProps[i] = m_event.pObjects[i]->GetMaterialIndex();
			}
		}

		m_pCallback->PreCollision( &m_event );
	}

    virtual void event_post_collision( IVP_Event_Collision *pEvent )
	{
		// didn't call preCollision, so don't call postCollision
		if ( !m_event.isCollision && !m_event.isShadowCollision )
			return;

		IVP_Contact_Situation *contact = pEvent->contact_situation;

		float collisionSpeed = contact->speed.dot_product(&contact->surf_normal);
		m_event.collisionSpeed = ConvertDistanceToHL( fabs(collisionSpeed) );
		CPhysicsCollisionData data(contact);
		m_event.pInternalData = &data;

		m_pCallback->PostCollision( &m_event );
	}

    virtual void event_collision_object_deleted( class IVP_Real_Object *) 
	{
		// enable this in constructor
	}

    virtual void event_friction_created( IVP_Event_Friction *pEvent )
	{
		IVP_Contact_Situation *contact = pEvent->contact_situation;
		CPhysicsObject *pObject1 = static_cast<CPhysicsObject *>(contact->objects[0]->client_data);
		CPhysicsObject *pObject2 = static_cast<CPhysicsObject *>(contact->objects[1]->client_data);

		if ( !pObject1 || !pObject2 )
			return;

		unsigned int flags1 = pObject1->GetCallbackFlags();
		unsigned int flags2 = pObject2->GetCallbackFlags();
		unsigned int allflags = flags1|flags2;

		bool calltouch = ( allflags & CALLBACK_GLOBAL_TOUCH ) ? true : false;
		if ( !calltouch )
			return;

		if ( pObject1->IsStatic() || pObject2->IsStatic() )
		{
			if ( !( allflags & CALLBACK_GLOBAL_TOUCH_STATIC ) )
				return;
		}

		CPhysicsFrictionData data(pEvent);
		m_pCallback->StartTouch( pObject1, pObject2, &data );
	}


    virtual void event_friction_deleted( IVP_Event_Friction *pEvent )
	{
		IVP_Contact_Situation *contact = pEvent->contact_situation;
		CPhysicsObject *pObject1 = static_cast<CPhysicsObject *>(contact->objects[0]->client_data);
		CPhysicsObject *pObject2 = static_cast<CPhysicsObject *>(contact->objects[1]->client_data);
		if ( !pObject1 || !pObject2 )
			return;

		unsigned int flags1 = pObject1->GetCallbackFlags();
		unsigned int flags2 = pObject2->GetCallbackFlags();

		unsigned int allflags = flags1|flags2;

		bool calltouch = ( allflags & CALLBACK_GLOBAL_TOUCH ) ? true : false;
		if ( !calltouch )
			return;

		if ( pObject1->IsStatic() || pObject2->IsStatic() )
		{
			if ( !( allflags & CALLBACK_GLOBAL_TOUCH_STATIC ) )
				return;
		}

		CPhysicsFrictionData data(pEvent);
		m_pCallback->EndTouch( pObject1, pObject2, &data );
	}

	virtual void event_friction_pair_created( class IVP_Friction_Core_Pair *pair );
	virtual void event_friction_pair_deleted( class IVP_Friction_Core_Pair *pair );
	virtual void mindist_entered_volume( class IVP_Controller_Phantom *controller,class IVP_Mindist_Base *mindist ) {}
	virtual void mindist_left_volume(class IVP_Controller_Phantom *controller, class IVP_Mindist_Base *mindist) {}

	virtual void core_entered_volume( IVP_Controller_Phantom *controller, IVP_Core *pCore )
	{
		CPhysicsFluidController *pFluid = static_cast<CPhysicsFluidController *>( controller->client_data );
		IVP_Real_Object *pivp = pCore->objects.element_at(0);
		CPhysicsObject *pObject = static_cast<CPhysicsObject *>(pivp->client_data);
		if ( !pObject )
			return;

		if ( pFluid )
		{
			if ( pObject && (pObject->GetCallbackFlags() & CALLBACK_FLUID_TOUCH) )
			{
				m_pCallback->FluidStartTouch( pObject, pFluid );
			}
		}
		else
		{
			// must be a trigger
			IVP_Real_Object *pTriggerIVP = controller->get_object();
			CPhysicsObject *pTrigger = static_cast<CPhysicsObject *>(pTriggerIVP->client_data);

			m_pCallback->ObjectEnterTrigger( pTrigger, pObject );
		}
	}

	virtual void core_left_volume( IVP_Controller_Phantom *controller, IVP_Core *pCore )
	{
		CPhysicsFluidController *pFluid = static_cast<CPhysicsFluidController *>( controller->client_data );
		IVP_Real_Object *pivp = pCore->objects.element_at(0);
		CPhysicsObject *pObject = static_cast<CPhysicsObject *>(pivp->client_data);
		if ( !pObject )
			return;

		if ( pFluid )
		{
			if ( pObject && (pObject->GetCallbackFlags() & CALLBACK_FLUID_TOUCH) )
			{
				m_pCallback->FluidEndTouch( pObject, pFluid );
			}
		}
		else
		{
			// must be a trigger
			IVP_Real_Object *pTriggerIVP = controller->get_object();
			CPhysicsObject *pTrigger = static_cast<CPhysicsObject *>(pTriggerIVP->client_data);

			m_pCallback->ObjectLeaveTrigger( pTrigger, pObject );
		}
	}
	void phantom_is_going_to_be_deleted_event(class IVP_Controller_Phantom *controller) {}

	void EventPSI( CPhysicsEnvironment *pEnvironment )
	{
		m_pCallback->PostSimulationFrame();
		UpdatePairListPSI( pEnvironment );
	}
private:
	
	struct corepair_t
	{
		corepair_t() {}
		corepair_t( IVP_Friction_Core_Pair *pair )
		{
			int index = ( pair->objs[0] < pair->objs[1] ) ? 0 : 1;
			core0 = pair->objs[index];
			core1 = pair->objs[!index];
			lastImpactTime= pair->last_impact_time_pair;
		}

		IVP_Core *core0;
		IVP_Core *core1;
		IVP_Time lastImpactTime;
	};

	static bool CorePairLessFunc( const corepair_t &lhs, const corepair_t &rhs ) 
	{ 
		if ( lhs.core0 != rhs.core0 )
			return ( lhs.core0 < rhs.core0 );
		else
			return ( lhs.core1 < rhs.core1 );
	}
	void UpdatePairListPSI( CPhysicsEnvironment *pEnvironment )
	{
		unsigned short index = m_pairList.FirstInorder();
		IVP_Time currentTime = pEnvironment->GetIVPEnvironment()->get_current_time();

		while ( m_pairList.IsValidIndex(index) )
		{
			unsigned short next = m_pairList.NextInorder( index );
			corepair_t &test = m_pairList.Element(index);
			
			// only keep 4 seconds worth of data
			if ( (currentTime - test.lastImpactTime) > 1.0 )
			{
				m_pairList.RemoveAt( index );
			}
			index = next;
		}
	}

	CUtlRBTree<corepair_t>			m_pairList;
	float							m_pairListOldestTime;


	IPhysicsCollisionEvent			*m_pCallback;
	vcollisionevent_t				m_event;

};


CPhysicsListenerCollision::CPhysicsListenerCollision() : IVP_Listener_Collision( ALL_COLLISION_FLAGS ), m_pCallback(&g_EmptyCollisionListener) 
{
	m_pairList.SetLessFunc( CorePairLessFunc );
}


void CPhysicsListenerCollision::event_friction_pair_created( IVP_Friction_Core_Pair *pair )
{
	corepair_t test(pair);
	unsigned short index = m_pairList.Find( test );
	if ( m_pairList.IsValidIndex( index ) )
	{
		corepair_t &save = m_pairList.Element(index);
		// found this one already, update the time
		if ( save.lastImpactTime.get_seconds() > pair->last_impact_time_pair.get_seconds() )
		{
			pair->last_impact_time_pair = save.lastImpactTime;
		}
		else
		{
			save.lastImpactTime = pair->last_impact_time_pair;
		}
	}
	else
	{
		if ( m_pairList.Count() < 16 )
		{
			m_pairList.Insert( test );
		}
	}
}


void CPhysicsListenerCollision::event_friction_pair_deleted( IVP_Friction_Core_Pair *pair )
{
	corepair_t test(pair);
	unsigned short index = m_pairList.Find( test );
	if ( m_pairList.IsValidIndex( index ) )
	{
		corepair_t &save = m_pairList.Element(index);
		// found this one already, update the time
		if ( save.lastImpactTime.get_seconds() < pair->last_impact_time_pair.get_seconds() )
		{
			save.lastImpactTime = pair->last_impact_time_pair;
		}
	}
	else
	{
		if ( m_pairList.Count() < 16 )
		{
			m_pairList.Insert( test );
		}
	}
}


class CCollisionSolver : public IVP_Collision_Filter, public IVP_Anomaly_Manager
{
public:
	CCollisionSolver( void ) : IVP_Anomaly_Manager(IVP_FALSE) { m_pSolver = NULL; }
	void SetHandler( IPhysicsCollisionSolver *pSolver ) { m_pSolver = pSolver; }

	// IVP_Collision_Filter
    IVP_BOOL check_objects_for_collision_detection(IVP_Real_Object *ivp0, IVP_Real_Object *ivp1)
	{
		if ( m_pSolver )
		{
			CPhysicsObject *pObject0 = static_cast<CPhysicsObject *>(ivp0->client_data);
			CPhysicsObject *pObject1 = static_cast<CPhysicsObject *>(ivp1->client_data);
			if ( pObject0 && pObject1 )
			{
				if ( (pObject0->GetCallbackFlags() & CALLBACK_ENABLING_COLLISION) && (pObject1->GetCallbackFlags() & CALLBACK_MARKED_FOR_DELETE) )
					return IVP_FALSE;

				if ( (pObject1->GetCallbackFlags() & CALLBACK_ENABLING_COLLISION) && (pObject0->GetCallbackFlags() & CALLBACK_MARKED_FOR_DELETE) )
					return IVP_FALSE;

				if ( !m_pSolver->ShouldCollide( pObject0, pObject1, pObject0->GetGameData(), pObject1->GetGameData() ) )
					return IVP_FALSE;
			}
		}
		return m_baseFilter.check_objects_for_collision_detection( ivp0, ivp1 );
	}
	void environment_will_be_deleted(IVP_Environment *) {}

	// IVP_Anomaly_Manager
	virtual void inter_penetration( IVP_Mindist *mindist,IVP_Real_Object *ivp0, IVP_Real_Object *ivp1)
	{
		if ( m_pSolver )
		{
			// UNDONE: project current velocity onto rescue velocity instead
			// This will cause escapes to be slow - which is probably a good
			// thing.  That's probably a better heuristic than only rescuing once
			// per PSI!
			CPhysicsObject *pObject0 = static_cast<CPhysicsObject *>(ivp0->client_data);
			CPhysicsObject *pObject1 = static_cast<CPhysicsObject *>(ivp1->client_data);
			if ( pObject0 && pObject1 )
			{
				if ( (pObject0->GetCallbackFlags() & CALLBACK_MARKED_FOR_DELETE) ||
					(pObject1->GetCallbackFlags() & CALLBACK_MARKED_FOR_DELETE) )
					return;

				// moveable object pair?
				if ( pObject0->IsMoveable() && pObject1->IsMoveable() )
				{
					// only push each pair apart once per PSI
					if ( CheckObjPair( ivp0, ivp1 ) )
						return;
				}
				IVP_Environment *env = ivp0->get_environment();
				float deltaTime = env->get_delta_PSI_time();

				if ( !m_pSolver->ShouldSolvePenetration( pObject0, pObject1, pObject0->GetGameData(), pObject1->GetGameData(), deltaTime ) )
					return;
			}
			else
			{
				return;
			}
		}

		IVP_Anomaly_Manager::inter_penetration( mindist, ivp0, ivp1 );
	}

public:
	void DisableCollisions( CPhysicsObject *pObject1, CPhysicsObject *pObject2 )
	{
		IVP_Real_Object *pRO1 = pObject1->GetObject();
		IVP_Real_Object *pRO2 = pObject2->GetObject();

		m_baseFilter.disable_collision_between_objects( pRO1, pRO2 );
	}
	
	void EnableCollisions( CPhysicsObject *pObject1, CPhysicsObject *pObject2 )
	{
		IVP_Real_Object *pRO1 = pObject1->GetObject();
		IVP_Real_Object *pRO2 = pObject2->GetObject();

		m_baseFilter.enable_collision_between_objects( pRO1, pRO2 );
	}

	bool ShouldCollide( CPhysicsObject *pObject1, CPhysicsObject *pObject2 )
	{
		IVP_Real_Object *pRO1 = pObject1->GetObject();
		IVP_Real_Object *pRO2 = pObject2->GetObject();

		return m_baseFilter.check_objects_for_collision_detection( pRO1, pRO2 ) ? true : false;
	}

	void EventPSI( CPhysicsEnvironment * )
	{
		m_rescue.RemoveAll();
	}


private:
	struct realobjectpair_t
	{
		IVP_Real_Object *pObj0;
		IVP_Real_Object *pObj1;
		inline bool operator==( const realobjectpair_t &src ) const
		{
			return (pObj0 == src.pObj0) && (pObj1 == src.pObj1);
		}
	};
	// basically each moveable object pair gets 1 rescue per PSI
	// UNDONE: Add a counter to do more?
	bool CheckObjPair( IVP_Real_Object *pObj0, IVP_Real_Object *pObj1 )
	{
		realobjectpair_t tmp;
		tmp.pObj0 = pObj0 < pObj1 ? pObj0 : pObj1;
		tmp.pObj1 = pObj0 > pObj1 ? pObj0 : pObj1;

		if ( m_rescue.Find( tmp ) != m_rescue.InvalidIndex() )
			return true;
		m_rescue.AddToTail( tmp );
		return false;
	}

private:
	IVP_Collision_Filter_Exclusive_Pair		m_baseFilter;
	IPhysicsCollisionSolver					*m_pSolver;
	// UNDONE: Linear search? should be small, but switch to rb tree if this ever gets large
	CUtlVector<realobjectpair_t>	m_rescue;
};



class CPhysicsListenerConstraint : public IVP_Listener_Constraint
{
public:
	CPhysicsListenerConstraint()
	{
		m_pCallback = NULL;
	}

	void SetHandler( IPhysicsConstraintEvent *pHandler )
	{
		m_pCallback = pHandler;
	}

    void event_constraint_broken( IVP_Constraint *pConstraint )
	{
		// IVP_Constraint is not allowed, something is broken
		Assert(0);
	}

    void event_constraint_broken( hk_Breakable_Constraint *pConstraint )
	{
		if ( m_pCallback )
		{
			IPhysicsConstraint *pObj = GetClientDataForHkConstraint( pConstraint );
			m_pCallback->ConstraintBroken( pObj );
		}
	}
private:
	IPhysicsConstraintEvent *m_pCallback;
};


#define AIR_DENSITY	2

class CDragController : public IVP_Controller_Independent
{
public:

	CDragController( void ) 
	{
		m_airDensity = AIR_DENSITY;
	}
	virtual ~CDragController( void ) {}

    virtual void do_simulation_controller(IVP_Event_Sim *event,IVP_U_Vector<IVP_Core> *core_list)
	{
		int i;
		for( i = core_list->len()-1; i >=0; i--) 
		{
			IVP_Core *pCore = core_list->element_at(i);

			IVP_Real_Object *pivp = pCore->objects.element_at(0);
			CPhysicsObject *pPhys = static_cast<CPhysicsObject *>(pivp->client_data);
			
			float dragForce = -0.5 * pPhys->GetDragInDirection( pCore->speed ) * m_airDensity * event->delta_time;
			if ( dragForce < -1.0f )
				dragForce = -1.0f;
			if ( dragForce < 0 )
			{
				IVP_U_Float_Point dragVelocity;
				dragVelocity.set_multiple( &pCore->speed, dragForce );
				pCore->speed.add( &dragVelocity );
			}
			float angDragForce = -pPhys->GetAngularDragInDirection( pCore->rot_speed ) * m_airDensity * event->delta_time;
			if ( angDragForce < -1.0f )
				angDragForce = -1.0f;
			if ( angDragForce < 0 )
			{
				IVP_U_Float_Point angDragVelocity;
				angDragVelocity.set_multiple( &pCore->rot_speed, angDragForce );
				pCore->rot_speed.add( &angDragVelocity );
			}
		}
	}
    
	virtual IVP_CONTROLLER_PRIORITY get_controller_priority() 
	{ 
		return IVP_CP_MOTION;
	}
	float GetAirDensity() { return m_airDensity; }
	void SetAirDensity( float density ) { m_airDensity = density; }

private:
	float	m_airDensity;
};

class CVPhysicsAnomalyManager : public IVP_Anomaly_Manager
{
public:

};


//-----------------------------------------------------------------------------
// Purpose: An ugly little class to get a callback on PSIs
//-----------------------------------------------------------------------------
class CPSICounter : public IVP_Listener_PSI
{
public:
    virtual void event_PSI( IVP_Event_PSI *pEvent )
	{
		CPhysicsEnvironment *pVEnv = (CPhysicsEnvironment *)pEvent->environment->client_data;
		pVEnv->EventPSI();
	}
    virtual void environment_will_be_deleted(IVP_Environment *) 
	{
		delete this;
	}
};


CPhysicsEnvironment::CPhysicsEnvironment( void )
// assume that these lists will have at least one object
{
	// set this to true to force the 
	m_deleteQuick = false;
	m_queueDeleteObject = false;
	m_inSimulation = false;
    // build a default environment
    IVP_Environment_Manager *env_manager;
    env_manager = IVP_Environment_Manager::get_environment_manager();

    IVP_Application_Environment appl_env;
	m_pCollisionSolver = new CCollisionSolver;
    appl_env.collision_filter = m_pCollisionSolver;
	appl_env.material_manager = physprops->GetIVPManager();
	appl_env.anomaly_manager = m_pCollisionSolver;

	BEGIN_IVP_ALLOCATION();
    m_pPhysEnv = env_manager->create_environment( &appl_env, "JAY", 0xBEEF );
	END_IVP_ALLOCATION();

	// UNDONE: Revisit brush/terrain/object shrinking and tune this number to something larger
	// UNDONE: Expose this to callers, also via physcollision
	m_pPhysEnv->set_global_collision_tolerance( ConvertDistanceToIVP( 0.25 ) );	// 1/4 inch tolerance
	m_pSleepEvents = new CSleepObjects;

	m_pDeleteQueue = new CDeleteQueue;

	BEGIN_IVP_ALLOCATION();
	m_pPhysEnv->add_listener_object_global( m_pSleepEvents );
	END_IVP_ALLOCATION();

	m_pCollisionListener = new CPhysicsListenerCollision;
	
	BEGIN_IVP_ALLOCATION();
	m_pPhysEnv->add_listener_collision_global( m_pCollisionListener );
	END_IVP_ALLOCATION();

	m_pConstraintListener = new CPhysicsListenerConstraint;

	BEGIN_IVP_ALLOCATION();
	m_pPhysEnv->add_listener_constraint_global( m_pConstraintListener );
	END_IVP_ALLOCATION();

	m_pDragController = new CDragController;

	IVP_Anomaly_Limits *limits = m_pPhysEnv->get_anomaly_limits();
	if ( limits )
	{
		// UNDONE: Expose this value for tuning
		limits->max_collisions_per_psi = 6;
	}
	m_pPhysEnv->client_data = (void *)this;
	m_simPSIs = 0;
	m_invPSIscale = 0;

	BEGIN_IVP_ALLOCATION();

	// this is auto-deleted on exit
	m_pPhysEnv->add_listener_PSI( new CPSICounter );

	END_IVP_ALLOCATION();
}

CPhysicsEnvironment::~CPhysicsEnvironment( void )
{
	// no callbacks during shutdown
	SetCollisionSolver( NULL );
	m_pPhysEnv->remove_listener_object_global( m_pSleepEvents );

	// don't bother waking up other objects as we clear them out
	SetQuickDelete( true );

	// delete/remove the listeners
	m_pPhysEnv->remove_listener_collision_global( m_pCollisionListener );
	delete m_pCollisionListener;
	m_pPhysEnv->remove_listener_constraint_global( m_pConstraintListener );
	delete m_pConstraintListener;

	// Clean out the list of physics objects
	for ( int i = m_objects.Count()-1; i >= 0; --i )
	{
		CPhysicsObject *pObject = static_cast<CPhysicsObject *>(m_objects[i]);
		PhantomRemove( pObject );
		delete pObject;
	}
		
	m_objects.RemoveAll();
	ClearDeadObjects();

	// Clean out the list of fluids
	m_fluids.PurgeAndDeleteElements();

	delete m_pSleepEvents;
	delete m_pDragController;
	delete m_pPhysEnv;
	delete m_pDeleteQueue;

	// must be deleted after the environment (calls back in destructor)
	delete m_pCollisionSolver;
}

void CPhysicsEnvironment::SetGravity( const Vector& gravityVector )
{
    IVP_U_Point gravity; 

	ConvertPositionToIVP( gravityVector, gravity );
    m_pPhysEnv->set_gravity( &gravity );
}


void CPhysicsEnvironment::GetGravity( Vector& gravityVector )
{
    const IVP_U_Point *gravity = m_pPhysEnv->get_gravity();

	ConvertPositionToHL( *gravity, gravityVector );
}


IPhysicsObject *CPhysicsEnvironment::CreatePolyObject( const CPhysCollide *pCollisionModel, int materialIndex, const Vector& position, const QAngle& angles, objectparams_t *pParams )
{
	IPhysicsObject *pObject = ::CreatePhysicsObject( this, pCollisionModel, materialIndex, position, angles, pParams, false );
	m_objects.AddToTail( pObject );
	return pObject;
}

IPhysicsObject *CPhysicsEnvironment::CreatePolyObjectStatic( const CPhysCollide *pCollisionModel, int materialIndex, const Vector& position, const QAngle& angles, objectparams_t *pParams )
{
	IPhysicsObject *pObject = ::CreatePhysicsObject( this, pCollisionModel, materialIndex, position, angles, pParams, true );
	m_objects.AddToTail( pObject );
	return pObject;
}


IPhysicsSpring *CPhysicsEnvironment::CreateSpring( IPhysicsObject *pObjectStart, IPhysicsObject *pObjectEnd, springparams_t *pParams )
{
	return ::CreateSpring( m_pPhysEnv, static_cast<CPhysicsObject *>(pObjectStart), static_cast<CPhysicsObject *>(pObjectEnd), pParams );
}

IPhysicsFluidController *CPhysicsEnvironment::CreateFluidController( IPhysicsObject *pFluidObject, fluidparams_t *pParams )
{
	CPhysicsFluidController *pFluid = ::CreateFluidController( m_pPhysEnv, static_cast<CPhysicsObject *>(pFluidObject), pParams );
	m_fluids.AddToTail( pFluid );
	return pFluid;
}

IPhysicsConstraint *CPhysicsEnvironment::CreateRagdollConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_ragdollparams_t &ragdoll )
{
	return ::CreateRagdollConstraint( m_pPhysEnv, (CPhysicsObject *)pReferenceObject, (CPhysicsObject *)pAttachedObject, pGroup, ragdoll );
}

IPhysicsConstraint *CPhysicsEnvironment::CreateHingeConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_hingeparams_t &hinge )
{
	return ::CreateHingeConstraint( m_pPhysEnv, (CPhysicsObject *)pReferenceObject, (CPhysicsObject *)pAttachedObject, pGroup, hinge );
}

IPhysicsConstraint *CPhysicsEnvironment::CreateFixedConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_fixedparams_t &fixed )
{
	return ::CreateFixedConstraint( m_pPhysEnv, (CPhysicsObject *)pReferenceObject, (CPhysicsObject *)pAttachedObject, pGroup, fixed );
}

IPhysicsConstraint *CPhysicsEnvironment::CreateSlidingConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_slidingparams_t &sliding )
{
	return ::CreateSlidingConstraint( m_pPhysEnv, (CPhysicsObject *)pReferenceObject, (CPhysicsObject *)pAttachedObject, pGroup, sliding );
}

IPhysicsConstraint *CPhysicsEnvironment::CreateBallsocketConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_ballsocketparams_t &ballsocket )
{
	return ::CreateBallsocketConstraint( m_pPhysEnv, (CPhysicsObject *)pReferenceObject, (CPhysicsObject *)pAttachedObject, pGroup, ballsocket );
}

IPhysicsConstraint *CPhysicsEnvironment::CreatePulleyConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_pulleyparams_t &pulley )
{
	return ::CreatePulleyConstraint( m_pPhysEnv, (CPhysicsObject *)pReferenceObject, (CPhysicsObject *)pAttachedObject, pGroup, pulley );
}

IPhysicsConstraint *CPhysicsEnvironment::CreateLengthConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_lengthparams_t &length )
{
	return ::CreateLengthConstraint( m_pPhysEnv, (CPhysicsObject *)pReferenceObject, (CPhysicsObject *)pAttachedObject, pGroup, length );
}

IPhysicsConstraintGroup *CPhysicsEnvironment::CreateConstraintGroup( void )
{
	return CreatePhysicsConstraintGroup( m_pPhysEnv );
}

void CPhysicsEnvironment::DisableCollisions( IPhysicsObject *pObject1, IPhysicsObject *pObject2 )
{
	m_pCollisionSolver->DisableCollisions( (CPhysicsObject *)pObject1, (CPhysicsObject *)pObject2 );
}

void CPhysicsEnvironment::EnableCollisions( IPhysicsObject *pObject1, IPhysicsObject *pObject2 )
{
	m_pCollisionSolver->EnableCollisions( (CPhysicsObject *)pObject1, (CPhysicsObject *)pObject2 );
}

bool CPhysicsEnvironment::ShouldCollide( IPhysicsObject *pObject1, IPhysicsObject *pObject2 )
{
	return m_pCollisionSolver->ShouldCollide( (CPhysicsObject *)pObject1, (CPhysicsObject *)pObject2 );

}

void CPhysicsEnvironment::Simulate( float deltaTime )
{
	if ( !m_pPhysEnv )
		return;

	// stop updating objects that went to sleep during the previous frame.
	m_pSleepEvents->UpdateSleepObjects();

	//get_next_PSI_time
	// Trap interrupts and clock changes
	if ( deltaTime > 1.0 || deltaTime < 0.0 )
	{
		deltaTime = 0;
	}
	else if ( deltaTime > 0.1 )
	{
		deltaTime = 0.1f;
	}

	m_simPSIs = 0;
	IVP_DOUBLE nextTime = m_pPhysEnv->get_current_time().get_seconds() + deltaTime;
	IVP_DOUBLE simTime = m_pPhysEnv->get_next_PSI_time().get_seconds();
	while ( nextTime > simTime )
	{
		m_simPSIs++;
		simTime += m_pPhysEnv->get_delta_PSI_time();
	}
	m_simPSIcurrent = m_simPSIs;

	m_inSimulation = true;
	// don't simulate less than .1 ms
	if ( deltaTime > 0.0001 )
	{
		m_pPhysEnv->simulate_dtime( deltaTime );
		// UNDONE: Change over to this?
//		m_pPhysEnv->simulate_variable_time_step( deltaTime );
	}
	m_inSimulation = false;

	// If the queue is disabled, it's only used during simulation.
	// Flush it as soon as possible (which is now)
	if ( !m_queueDeleteObject )
	{
		ClearDeadObjects();
	}

	if ( m_pCollisionListener->GetHandler() )
	{
		m_pSleepEvents->ProcessActiveObjects( m_pPhysEnv, m_pCollisionListener->GetHandler() );
	}
}

void CPhysicsEnvironment::ResetSimulationClock()
{
	// UNDONE: You'd think that all of this would make the system deterministic, but
	// it doesn't.
	extern void SeedRandomGenerators();

	m_pPhysEnv->reset_time();
	m_pPhysEnv->get_time_manager()->env_set_current_time( m_pPhysEnv, IVP_Time(0) );
	m_pPhysEnv->reset_time();
	SeedRandomGenerators();
}

float CPhysicsEnvironment::GetSimulationTimestep( void )
{
	return m_pPhysEnv->get_delta_PSI_time();
}

void CPhysicsEnvironment::SetSimulationTimestep( float timestep )
{
	m_pPhysEnv->set_delta_PSI_time( timestep );
}

float CPhysicsEnvironment::GetSimulationTime( void )
{
	return (float)m_pPhysEnv->get_current_time().get_time();
}

int CPhysicsEnvironment::GetTimestepsSimulatedLast()
{
	return m_simPSIs;
}

// true if currently running the simulator (i.e. in a callback during physenv->Simulate())
bool CPhysicsEnvironment::IsInSimulation( void ) const
{
	return m_inSimulation;
}

void CPhysicsEnvironment::DestroyObject( IPhysicsObject *pObject )
{
	if ( !pObject )
	{
		DevMsg("Deleted NULL vphysics object\n");
		return;
	}
	m_objects.FindAndRemove( pObject );

	if ( m_inSimulation || m_queueDeleteObject )
	{
		// add this flag so we can optimize some cases
		pObject->SetCallbackFlags( pObject->GetCallbackFlags() | CALLBACK_MARKED_FOR_DELETE );

		// don't delete while simulating
		m_deadObjects.AddToTail( pObject );
	}
	else
	{
		m_pSleepEvents->DeleteObject( static_cast<CPhysicsObject *>(pObject) );
		delete pObject;
	}
}

void CPhysicsEnvironment::DestroySpring( IPhysicsSpring *pSpring )
{
	delete pSpring;
}
void CPhysicsEnvironment::DestroyFluidController( IPhysicsFluidController *pFluid )
{
	m_fluids.FindAndRemove( (CPhysicsFluidController *)pFluid );
	delete pFluid;
}


void CPhysicsEnvironment::DestroyConstraint( IPhysicsConstraint *pConstraint )
{
	if ( !m_deleteQuick && pConstraint )
	{
		IPhysicsObject *pObj0 = pConstraint->GetReferenceObject();
		if ( pObj0 )
		{
			pObj0->Wake();
		}

		IPhysicsObject *pObj1 = pConstraint->GetAttachedObject();
		if ( pObj1 )
		{
			pObj1->Wake();
		}
	}
	if ( m_inSimulation )
	{
		pConstraint->Deactivate();
		m_pDeleteQueue->QueueForDelete( pConstraint );
	}
	else
	{
		delete pConstraint;
	}
}

void CPhysicsEnvironment::DestroyConstraintGroup( IPhysicsConstraintGroup *pGroup )
{
	delete pGroup;
}

void CPhysicsEnvironment::TraceBox( trace_t *ptr, const Vector &mins, const Vector &maxs, const Vector &start, const Vector &end )
{
	// UNDONE: Need this?
}

void CPhysicsEnvironment::SetCollisionSolver( IPhysicsCollisionSolver *pSolver )
{
	m_pCollisionSolver->SetHandler( pSolver );
}


void CPhysicsEnvironment::ClearDeadObjects( void )
{
	for ( int i = 0; i < m_deadObjects.Count(); i++ )
	{
		CPhysicsObject *pObject = (CPhysicsObject *)m_deadObjects.Element(i);

		m_pSleepEvents->DeleteObject( pObject );
		delete pObject;
	}
	m_deadObjects.Purge();
	m_pDeleteQueue->DeleteAll();
}


void CPhysicsEnvironment::SetCollisionEventHandler( IPhysicsCollisionEvent *pCollisionEvents )
{
	m_pCollisionListener->SetHandler( pCollisionEvents );
}


void CPhysicsEnvironment::SetObjectEventHandler( IPhysicsObjectEvent *pObjectEvents )
{
	m_pSleepEvents->SetHandler( pObjectEvents );
}

void CPhysicsEnvironment::SetConstraintEventHandler( IPhysicsConstraintEvent *pConstraintEvents )
{
	m_pConstraintListener->SetHandler( pConstraintEvents );
}


IPhysicsShadowController *CPhysicsEnvironment::CreateShadowController( IPhysicsObject *pObject, bool allowTranslation, bool allowRotation )
{
	return ::CreateShadowController( static_cast<CPhysicsObject*>(pObject), allowTranslation, allowRotation );
}

void CPhysicsEnvironment::DestroyShadowController( IPhysicsShadowController *pController )
{
	delete pController;
}

IPhysicsPlayerController *CPhysicsEnvironment::CreatePlayerController( IPhysicsObject *pObject )
{
	return ::CreatePlayerController( static_cast<CPhysicsObject*>(pObject) );
}

void CPhysicsEnvironment::DestroyPlayerController( IPhysicsPlayerController *pController )
{
	delete pController;
}

IPhysicsMotionController *CPhysicsEnvironment::CreateMotionController( IMotionEvent *pHandler )
{
	return ::CreateMotionController( this, pHandler );
}

void CPhysicsEnvironment::DestroyMotionController( IPhysicsMotionController *pController )
{
	delete pController;
}

IPhysicsVehicleController *CPhysicsEnvironment::CreateVehicleController( IPhysicsObject *pVehicleBodyObject, const vehicleparams_t &params, unsigned int nVehicleType, IPhysicsGameTrace *pGameTrace )
{
	return ::CreateVehicleController( this, static_cast<CPhysicsObject*>(pVehicleBodyObject), params, nVehicleType, pGameTrace );
}

void CPhysicsEnvironment::DestroyVehicleController( IPhysicsVehicleController *pController )
{
	delete pController;
}

int	CPhysicsEnvironment::GetActiveObjectCount( void )
{
	return m_pSleepEvents->GetActiveObjectCount();
}


void CPhysicsEnvironment::GetActiveObjects( IPhysicsObject **pOutputObjectList )
{
	m_pSleepEvents->GetActiveObjects( pOutputObjectList );
}

void CPhysicsEnvironment::SetAirDensity( float density )
{
	CDragController *pDrag = ((CDragController *)m_pDragController);
	if ( pDrag )
	{
		pDrag->SetAirDensity( density );
	}
}

float CPhysicsEnvironment::GetAirDensity( void )
{
	CDragController *pDrag = ((CDragController *)m_pDragController);
	if ( pDrag )
	{
		return pDrag->GetAirDensity();
	}
	return 0;
}

void CPhysicsEnvironment::EventPSI( void )
{
	m_inSimulation = false;
	if ( !m_queueDeleteObject )
	{
		ClearDeadObjects();
	}

	m_pCollisionSolver->EventPSI( this );
	m_pCollisionListener->EventPSI( this );
	// do a little hack here to spread the effects of per-game-frame controllers
	// over the entire sim time
	if ( m_simPSIcurrent )
	{
		m_invPSIscale = 1.0f / (float)m_simPSIcurrent;
		m_simPSIcurrent--;
	}
	else
	{
		m_invPSIscale = 0;
	}
	m_inSimulation = true;
}


void CPhysicsEnvironment::CleanupDeleteList()
{
	ClearDeadObjects();
}

bool CPhysicsEnvironment::IsCollisionModelUsed( CPhysCollide *pCollide )
{
	int i;

	for ( i = m_deadObjects.Count()-1; i >= 0; --i )
	{
		if ( m_deadObjects[i]->GetCollide() == pCollide )
			return true;
	}
	
	for ( i = m_objects.Count()-1; i >= 0; --i )
	{
		if ( m_objects[i]->GetCollide() == pCollide )
			return true;
	}

	return false;
}


// manage phantoms
void CPhysicsEnvironment::PhantomAdd( CPhysicsObject *pObject )
{
	IVP_Controller_Phantom *pPhantom = pObject->GetObject()->get_controller_phantom();
	if ( pPhantom )
	{
		pPhantom ->add_listener_phantom( m_pCollisionListener );
	}
}

void CPhysicsEnvironment::PhantomRemove( CPhysicsObject *pObject )
{
	IVP_Controller_Phantom *pPhantom = pObject->GetObject()->get_controller_phantom();
	if ( pPhantom )
	{
		pPhantom ->remove_listener_phantom( m_pCollisionListener );
	}
}


//-------------------------------------

IPhysicsObject *CPhysicsEnvironment::CreateSphereObject( float radius, int materialIndex, const Vector& position, const QAngle& angles, objectparams_t *pParams, bool isStatic )
{
	IPhysicsObject *pObject = ::CreatePhysicsSphere( this, radius, materialIndex, position, angles, pParams, isStatic );
	m_objects.AddToTail( pObject );
	return pObject;
}


IPhysicsEnvironment *CreatePhysicsEnvironment( void )
{
	return new CPhysicsEnvironment;
}

