//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef INTERPOLATEDVAR_H
#define INTERPOLATEDVAR_H
#ifdef _WIN32
#pragma once
#endif

#include "utlfixedlinkedlist.h"

template <class T>
inline T LoopingLerp( float flPercent, T flFrom, T flTo )
{
	T s = flTo * flPercent + flFrom * (1.0f - flPercent);
	return s;
}

template <>
inline float LoopingLerp( float flPercent, float flFrom, float flTo )
{
	if ( fabs( flTo - flFrom ) >= 0.5f )
	{
		if (flFrom < flTo)
			flFrom += 1.0f;
		else
			flTo += 1.0f;
	}

	float s = flTo * flPercent + flFrom * (1.0f - flPercent);

	s = s - (int)(s);
	if (s < 0.0f)
		s = s + 1.0f;

	return s;
}

template <class T>
inline T Lerp_Hermite( float t, const T& p0, const T& p1, const T& p2 )
{
	T d1 = p1 - p0;
	T d2 = p2 - p1;

	T output;
	float tSqr = t*t;
	float tCube = t*tSqr;

	output = p1 * (2*tCube-3*tSqr+1);
	output += p2 * (-2*tCube+3*tSqr);
	output += d1 * (tCube-2*tSqr+t);
	output += d2 * (tCube-tSqr);

	return output;
}

template<> 
inline QAngle Lerp_Hermite<QAngle>( float t, const QAngle& p0, const QAngle& p1, const QAngle& p2 )
{
	// Can't do hermite with QAngles, get discontinuities, just do a regular interpolation
	return Lerp( t, p1, p2 );
}

template <class T>
inline T LoopingLerp_Hermite( float t, T p0, T p1, T p2  )
{
	return Lerp_Hermite( t, p0, p1, p2 );
}

template <>
inline float LoopingLerp_Hermite( float t, float p0, float p1, float p2 )
{
	if  ( fabs( p1 - p0 ) > 0.5f )
	{
		if ( p0 < p1 )
			p0 += 1.0f;
		else
			p1 += 1.0f;
	}

	if ( fabs( p2 - p1 ) > 0.5f )
	{
		if (p1 < p2)
			p1 += 1.0f;
		else
			p2 += 1.0f;
	}
		
	float s = Lerp_Hermite( t, p0, p1, p2 );

	s = s - (int)(s);
	if (s < 0.0f)
	{
		s = s + 1.0f;
	}

	return s;
}


#define LATCH_ANIMATION_VAR  (1<<0)
#define LATCH_SIMULATION_VAR (1<<1)

#define EXCLUDE_AUTO_LATCH			(1<<2)
#define EXCLUDE_AUTO_INTERPOLATE	(1<<3)

class IInterpolatedVar
{
public:
	virtual void Setup( void *pValue, int type ) = 0;
	virtual void NoteChanged( C_BaseEntity *entity, int changeType ) = 0;
	virtual void NoteChanged( C_BaseEntity *entity, int changeType, float changetime ) = 0;
	virtual void Reset() = 0;
	virtual void Interpolate( C_BaseEntity *entity, float currentTime ) = 0;
	virtual int	 GetType() const = 0;
	virtual void RestoreToLastNetworked() = 0;
};

template< class Type > 
class CInterpolatedVar : public IInterpolatedVar
{
public:
	CInterpolatedVar()
	{
		m_pValue = NULL;
		m_fType = LATCH_ANIMATION_VAR;
		m_ListHead = m_VarHistory.InvalidIndex();
		m_bLooping = false;
		memset( &m_LastNetworked, 0, sizeof( m_LastNetworked ) );
	}

	virtual ~CInterpolatedVar()
	{
		ClearHistory();
	}

	virtual void Setup( void *pValue, int type )
	{
		m_pValue = ( Type * )pValue;
		SetType( type );
	}

	void SetType( int type )
	{
		m_fType = type;
	}

	int GetType() const
	{
		return m_fType;
	}

	virtual void NoteChanged( C_BaseEntity *entity, int changeType )
	{
		float changetime = entity->GetLastChangeTime( GetType() );
		NoteChanged( entity, changeType, changetime );
	}

	virtual void NoteChanged( C_BaseEntity *entity, int changeType, float changetime )
	{
		bool shouldLatch = ( GetType() & changeType ) ? true : false;
		if ( !shouldLatch )
			return;

		Assert( m_pValue );

		AddToHead( changetime, *m_pValue );

		// Latch this value
		m_LastNetworked = *m_pValue;

		// Now remove the old ones
		float oldesttime = gpGlobals->curtime - entity->GetInterpolationAmount( GetType() ) - 0.1f;

		int c = 0;
		int next = m_VarHistory.InvalidIndex();
		// Always leave three of entries in the list...
		for ( int i = m_ListHead; i != m_VarHistory.InvalidIndex(); c++, i = next )
		{
			next = m_VarHistory.Next( i );

			// Always leave elements 0 1 and 2 alone...
			if ( c <= 2 )
				continue;

			CInterpolatedVarEntry *h = &m_VarHistory[ i ];
			// Remove everything off the end until we find the first one that's not too old
			if ( h->changetime > oldesttime )
				continue;

			// Unlink rest of chain
			m_VarHistory.Free( i );
		}
	}

	void RestoreToLastNetworked()
	{
		Assert( m_pValue );
		*m_pValue = m_LastNetworked;
	}

	void ClearHistory()
	{
		while ( m_ListHead != m_VarHistory.InvalidIndex() )
		{
			int next = m_VarHistory.Next( m_ListHead );
			m_VarHistory.Free( m_ListHead );
			m_ListHead = next;
		}

		Assert( m_ListHead == m_VarHistory.InvalidIndex() );
	}

	void AddToHead( float changeTime, const Type& val )
	{
		int newslot = m_VarHistory.Alloc( true );
		
		CInterpolatedVarEntry *e = &m_VarHistory[ newslot ];
		e->changetime	= changeTime;
		e->value		= val;
	
		int insertSpot = m_ListHead;
		while ( insertSpot != m_VarHistory.InvalidIndex() )
		{
			CInterpolatedVarEntry *check = &m_VarHistory[ insertSpot ];
			if ( check->changetime <= changeTime )
				break;

			int next = m_VarHistory.Next( insertSpot );
			if ( next == m_VarHistory.InvalidIndex() )
			{
				m_VarHistory.LinkAfter( insertSpot, newslot );
				return;
			}
			insertSpot = next;
		}

		m_VarHistory.LinkBefore( insertSpot, newslot );

		if ( insertSpot == m_ListHead )
		{
			m_ListHead = newslot;
		}
	}

	virtual void Reset()
	{
		Assert( m_pValue );

		ClearHistory();

		AddToHead( gpGlobals->curtime, *m_pValue );
		AddToHead( gpGlobals->curtime, *m_pValue );
		AddToHead( gpGlobals->curtime, *m_pValue );

		m_LastNetworked = *m_pValue;
	}

	virtual void Interpolate( C_BaseEntity *entity, float currentTime )
	{
		float interpolation_amount = entity->GetInterpolationAmount( GetType() );

		Assert( m_pValue );

		float targettime = currentTime - interpolation_amount;
		int i;

		// Walk forward from most recent looking for spanning indices
		CInterpolatedVarEntry *newer = NULL;

		for ( i = m_ListHead; i != m_VarHistory.InvalidIndex(); i = m_VarHistory.Next( i ) )
		{
			CInterpolatedVarEntry *older = &m_VarHistory[ i ];
			
			float older_change_time = older->changetime;

			if ( older_change_time == 0.0f )
				break;

			if ( !newer )
			{
				// There'll be no older data
				if ( targettime >= older_change_time )
				{
					*m_pValue = older->value;
					return;
				}
			}
			else
			{
				if ( targettime >= older_change_time )
				{
					// Found span
					Assert( newer );

					float newer_change_time = newer->changetime;

					float dt = newer_change_time - older_change_time;
					if ( dt > 0.0001f )
					{
						float frac = ( targettime - older_change_time ) / ( newer_change_time - older_change_time );
						frac = min( frac, 2.0f );

						bool hermite = false;
						CInterpolatedVarEntry *oldest = NULL;
						
						int oldestindex = m_VarHistory.Next( i );

						if ( oldestindex != m_VarHistory.InvalidIndex() )
						{
							oldest = &m_VarHistory[ oldestindex ];
							float oldest_change_time = oldest->changetime;
							float dt2 = older_change_time - oldest_change_time;
							if ( dt2 > 0.0001f )
							{
								hermite = true;
							}
						}

						if ( hermite && oldest )
						{
							*m_pValue = _Interpolate_Hermite( frac, oldest, older, newer );
						}
						else
						{
							*m_pValue = _Interpolate( frac, older, newer );
						}
						return;
					}
					else
					{
						*m_pValue = newer->value;
						return;
					}
				}
			}

			newer = older;
		}

		// Didn't find any, return last entry???
		if ( newer )
		{
			*m_pValue = newer->value;
			return;
		}
	}

	const Type&	GetPrev() const
	{
		Assert( m_pValue );

		int ihead = m_ListHead;
		if ( ihead != m_VarHistory.InvalidIndex() )
		{
			ihead = m_VarHistory.Next( ihead );
			if ( ihead != m_VarHistory.InvalidIndex() )
			{
				CInterpolatedVarEntry const *h = &m_VarHistory[ ihead ];
				return h->value;
			}
		}
		return *m_pValue;
	}

	const Type&	GetCurrent() const
	{
		Assert( m_pValue );

		int ihead = m_ListHead;
		if ( ihead != m_VarHistory.InvalidIndex() )
		{
			CInterpolatedVarEntry const *h = &m_VarHistory[ ihead ];
			return h->value;
		}
		return *m_pValue;
	}

	float	GetInterval() const
	{	
		int head = m_ListHead;
		if ( head != m_VarHistory.InvalidIndex() )
		{
			int next = m_VarHistory.Next( head );
			if ( next != m_VarHistory.InvalidIndex() )
			{
				CInterpolatedVarEntry const *h = &m_VarHistory[ head ];
				CInterpolatedVarEntry const *n = &m_VarHistory[ next ];
				
				return ( h->changetime - n->changetime );
			}
		}

		return 0.0f;
	}

	bool	IsValidIndex( int i )
	{
		return m_VarHistory.IsValidIndex( i );
	}

	Type	*GetHistoryValue( int index, float& changetime )
	{
		if ( index == m_VarHistory.InvalidIndex() )
		{
			changetime = 0.0f;
			return NULL;
		}

		CInterpolatedVarEntry *entry = &m_VarHistory[ index ];
		changetime = entry->changetime;
		return &entry->value;
	}

	int		GetHead( void )
	{
		return m_ListHead;
	}

	int		GetNext( int i )
	{
		return m_VarHistory.Next( i );
	}

	void	SetLooping( bool looping )
	{
		m_bLooping = looping;
	}

private:

	struct CInterpolatedVarEntry
	{
		float		changetime;
		Type		value;
	};

	Type _Interpolate( float frac, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end )
	{
		Type retVal;

		Assert( start );
		Assert( end );
		// Allow extrapolation for now...
		//Assert( frac >= 0.0f && frac <= 1.0f );

		// Note that QAngle has a specialization that will do quaternion interpolation here...
		if ( m_bLooping )
		{
			retVal = LoopingLerp( frac, start->value, end->value );
		}
		else
		{
			retVal = Lerp( frac, start->value, end->value );
		}
		return retVal;
	}

	Type _Interpolate_Hermite( float frac, CInterpolatedVarEntry *prev, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end, bool looping = false )
	{
		Type retVal;

		Assert( start );
		Assert( end );
		// Allow extrapolation for now...
		//Assert( frac >= 0.0f && frac <= 1.0f );

		float dt1 = end->changetime - start->changetime;
		float dt2 = start->changetime - prev->changetime;

		CInterpolatedVarEntry fixup;

		// If times are not of the same interval renormalize the earlier sample to allow for uniform hermite spline interpolation
		if ( fabs( dt1 - dt2 ) > 0.0001f &&
			dt2 > 0.0001f )
		{
			// Renormalize
			float frac = dt1 / dt2;

			// Fixed interval into past
			fixup.changetime = start->changetime - dt1;
			fixup.value = Lerp( 1-frac, prev->value, start->value );

			// Point previous sample at fixed version
			prev = &fixup;
		}

		// Note that QAngle has a specialization that will do quaternion interpolation here...
		if ( m_bLooping )
		{
			retVal = LoopingLerp_Hermite( frac, prev->value, start->value, end->value );
		}
		else
		{
			retVal = Lerp_Hermite( frac, prev->value, start->value, end->value );
		}
		return retVal;
	}

	bool ValidOrder()
	{
		float newestchangetime = 0.0f;
		bool first = true;
		for ( int i = GetHead(); IsValidIndex( i ); i = GetNext( i ) )
		{
			CInterpolatedVarEntry *entry = &m_VarHistory[ i ];
			if ( first )
			{
				first = false;
				newestchangetime = entry->changetime;
				continue;
			}

			// They should get older as wel walk backwards
			if ( entry->changetime > newestchangetime )
			{
				Assert( 0 );
				return false;
			}

			newestchangetime = entry->changetime;
		}

		return true;
	}

	// The underlying data element
	Type								*m_pValue;
	Type								m_LastNetworked;
	int									m_fType;
	int									m_ListHead;
	bool								m_bLooping;
	static CUtlFixedLinkedList< CInterpolatedVarEntry >	m_VarHistory;
};

template< class Type, int COUNT > 
class CInterpolatedVarArray : public IInterpolatedVar
{
public:
	CInterpolatedVarArray()
	{
		m_pValue = NULL;
		m_fType = LATCH_ANIMATION_VAR;
		m_ListHead = m_VarHistory.InvalidIndex();
		memset( m_bLooping, 0x00, sizeof( m_bLooping ) );
		m_nMaxCount = COUNT;
		memset( m_LastNetworked, 0, sizeof( m_LastNetworked ) );
	}

	virtual ~CInterpolatedVarArray()
	{
		ClearHistory();
	}

	virtual void Setup( void *pValue, int type )
	{
		m_pValue = ( Type * )pValue;
		SetType( type );
	}

	void SetType( int type )
	{
		m_fType = type;
	}

	int GetType() const
	{
		return m_fType;
	}

	virtual void NoteChanged( C_BaseEntity *entity, int changeType )
	{
		float changetime = entity->GetLastChangeTime( GetType() );
		NoteChanged( entity, changeType, changetime );
	}

	virtual void NoteChanged( C_BaseEntity *entity, int changeType, float changetime )
	{
		bool shouldLatch = ( GetType() & changeType ) ? true : false;
		if ( !shouldLatch )
			return;

		Assert( m_pValue );

		AddToHead( changetime, m_pValue );

		memcpy( m_LastNetworked, m_pValue, COUNT * sizeof( Type ) );

		// Now remove the old ones
		float oldesttime = gpGlobals->curtime - entity->GetInterpolationAmount( GetType() ) - 0.1f;

		int c = 0;
		int next = m_VarHistory.InvalidIndex();
		// Always leave three of entries in the list...
		for ( int i = m_ListHead; i != m_VarHistory.InvalidIndex(); c++, i = next )
		{
			next = m_VarHistory.Next( i );

			// Always leave elements 0 1 and 2 alone...
			if ( c <= 2 )
				continue;

			CInterpolatedVarEntry *h = &m_VarHistory[ i ];
			// Remove everything off the end until we find the first one that's not too old
			if ( h->changetime > oldesttime )
				continue;

			// Unlink rest of chain
			m_VarHistory.Free( i );
		}
	}

	void RestoreToLastNetworked()
	{
		Assert( m_pValue );
		memcpy( m_pValue, m_LastNetworked, COUNT * sizeof( Type ) );
	}

	void ClearHistory()
	{
		while ( m_ListHead != m_VarHistory.InvalidIndex() )
		{
			int next = m_VarHistory.Next( m_ListHead );
			m_VarHistory.Free( m_ListHead );
			m_ListHead = next;
		}

		Assert( m_ListHead == m_VarHistory.InvalidIndex() );
	}

	void AddToHead( float changeTime, const Type* values )
	{
		int newslot = m_VarHistory.Alloc( true );
		
		CInterpolatedVarEntry *e = &m_VarHistory[ newslot ];
		e->changetime	= changeTime;
		memcpy( e->value, values, m_nMaxCount * sizeof( Type ) );
	
		int insertSpot = m_ListHead;
		while ( insertSpot != m_VarHistory.InvalidIndex() )
		{
			CInterpolatedVarEntry *check = &m_VarHistory[ insertSpot ];
			if ( check->changetime <= changeTime )
				break;

			int next = m_VarHistory.Next( insertSpot );
			if ( next == m_VarHistory.InvalidIndex() )
			{
				m_VarHistory.LinkAfter( insertSpot, newslot );
				return;
			}
			insertSpot = next;
		}

		m_VarHistory.LinkBefore( insertSpot, newslot );

		if ( insertSpot == m_ListHead )
		{
			m_ListHead = newslot;
		}
	}

	virtual void Reset()
	{
		Assert( m_pValue );

		ClearHistory();

		AddToHead( gpGlobals->curtime, m_pValue );
		AddToHead( gpGlobals->curtime, m_pValue );
		AddToHead( gpGlobals->curtime, m_pValue );

		memcpy( m_LastNetworked, m_pValue, COUNT * sizeof( Type ) );
	}

	virtual void Interpolate( C_BaseEntity *entity, float currentTime )
	{
		float interpolation_amount = entity->GetInterpolationAmount( GetType() );

		Assert( m_pValue );

		float targettime = currentTime - interpolation_amount;
		int i;

		// Walk forward from most recent looking for spanning indices
		CInterpolatedVarEntry *newer = NULL;

		for ( i = m_ListHead; i != m_VarHistory.InvalidIndex(); i = m_VarHistory.Next( i ) )
		{
			CInterpolatedVarEntry *older = &m_VarHistory[ i ];
			
			float older_change_time = older->changetime;

			if ( older_change_time == 0.0f )
				break;

			if ( !newer )
			{
				// There'll be no older data
				if ( targettime >= older_change_time )
				{
					memcpy( m_pValue, &older->value[0], m_nMaxCount * sizeof( Type ) );
					return;
				}
			}
			else
			{
				if ( targettime >= older_change_time )
				{
					// Found span
					Assert( newer );

					float newer_change_time = newer->changetime;

					float dt = newer_change_time - older_change_time;
					if ( dt > 0.0001f )
					{
						float frac = ( targettime - older_change_time ) / ( newer_change_time - older_change_time );
						frac = min( frac, 2.0f );

						bool hermite = false;
						CInterpolatedVarEntry *oldest = NULL;
						
						int oldestindex = m_VarHistory.Next( i );

						if ( oldestindex != m_VarHistory.InvalidIndex() )
						{
							oldest = &m_VarHistory[ oldestindex ];
							float oldest_change_time = oldest->changetime;
							float dt2 = older_change_time - oldest_change_time;
							if ( dt2 > 0.0001f )
							{
								hermite = true;
							}
						}

						if ( hermite && oldest )
						{
							_Interpolate_Hermite( m_pValue, frac, oldest, older, newer );
						}
						else
						{
							_Interpolate( m_pValue, frac, older, newer );
						}
						return;
					}
					else
					{
						memcpy( m_pValue, &newer->value[0], m_nMaxCount * sizeof( Type ) );
						return;
					}
				}
			}

			newer = older;
		}

		// Didn't find any, return last entry???
		if ( newer )
		{
			memcpy( m_pValue, &newer->value[0], m_nMaxCount * sizeof( Type ) );
			return;
		}
	}

	const Type*	GetPrev( int index ) const
	{
		Assert( m_pValue );
		Assert( index >= 0 && index < m_nMaxCount );

		int ihead = m_ListHead;
		if ( ihead != m_VarHistory.InvalidIndex() )
		{
			ihead = m_VarHistory.Next( ihead );
			if ( ihead != m_VarHistory.InvalidIndex() )
			{
				CInterpolatedVarEntry const *h = &m_VarHistory[ ihead ];
				return &h->value[ index ];
			}
		}
		return &m_pValue[ index ];
	}

	const Type&	GetCurrent( int index ) const
	{
		Assert( m_pValue );
		Assert( index >= 0 && index < m_nMaxCount );

		int ihead = m_ListHead;
		if ( ihead != m_VarHistory.InvalidIndex() )
		{
			CInterpolatedVarEntry const *h = &m_VarHistory[ ihead ];
			return &h->value[ index ];
		}
		return &m_pValue[ index ];
	}

	float	GetInterval() const
	{	
		int head = m_ListHead;
		if ( head != m_VarHistory.InvalidIndex() )
		{
			int next = m_VarHistory.Next( head );
			if ( next != m_VarHistory.InvalidIndex() )
			{
				CInterpolatedVarEntry const *h = &m_VarHistory[ head ];
				CInterpolatedVarEntry const *n = &m_VarHistory[ next ];
				
				return ( h->changetime - n->changetime );
			}
		}

		return 0.0f;
	}

	bool	IsValidIndex( int i )
	{
		return m_VarHistory.IsValidIndex( i );
	}

	Type	*GetHistoryValue( int index, int item, float& changetime )
	{
		Assert( item >= 0 && item < m_nMaxCount );
		if ( index == m_VarHistory.InvalidIndex() )
		{
			changetime = 0.0f;
			return NULL;
		}

		CInterpolatedVarEntry *entry = &m_VarHistory[ index ];
		changetime = entry->changetime;
		return &entry->value[ item ];
	}

	int		GetHead( void )
	{
		return m_ListHead;
	}

	int		GetNext( int i )
	{
		return m_VarHistory.Next( i );
	}

	void SetHistoryValuesForItem( int item, Type& value )
	{
		Assert( item >= 0 && item < m_nMaxCount );

		int i;
		for ( i = m_ListHead; i != m_VarHistory.InvalidIndex(); i = m_VarHistory.Next( i ) )
		{
			CInterpolatedVarEntry *entry = &m_VarHistory[ i ];
			entry->value[ item ] = value;
		}
	}

	void	SetLooping( int slot, bool looping )
	{
		Assert( slot >= 0 && slot < m_nMaxCount );
		m_bLooping[ slot ] = looping;
	}

	void	SetMaxCount( int newmax )
	{
		Assert( newmax <= COUNT );
		bool changed = ( newmax != m_nMaxCount ) ? true : false;
		m_nMaxCount = newmax;
		// Wipe everything any time this changes!!!
		if ( changed )
		{
			Reset();
		}
	}

private:

	struct CInterpolatedVarEntry
	{
		float		changetime;
		Type		value[ COUNT ];
	};

	void _Interpolate( Type *out, float frac, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end )
	{
		Assert( start );
		Assert( end );
		// Allow extrapolation for now...
		//Assert( frac >= 0.0f && frac <= 1.0f );

		// Note that QAngle has a specialization that will do quaternion interpolation here...
		for ( int i = 0; i < m_nMaxCount; i++ )
		{
			if ( m_bLooping[ i ] )
			{
				out[i] = LoopingLerp( frac, start->value[i], end->value[i] );
			}
			else
			{
				out[i] = Lerp( frac, start->value[i], end->value[i] );
			}
		}
	}

	void _Interpolate_Hermite( Type *out, float frac, CInterpolatedVarEntry *prev, CInterpolatedVarEntry *start, CInterpolatedVarEntry *end, bool looping = false )
	{
		Assert( start );
		Assert( end );
		// Allow extrapolation for now...
		//Assert( frac >= 0.0f && frac <= 1.0f );

		float dt1 = end->changetime - start->changetime;
		float dt2 = start->changetime - prev->changetime;

		CInterpolatedVarEntry fixup;

		// If times are not of the same interval renormalize the earlier sample to allow for uniform hermite spline interpolation
		if ( fabs( dt1 - dt2 ) > 0.0001f &&
			dt2 > 0.0001f )
		{
			// Renormalize
			float frac = dt1 / dt2;

			// Fixed interval into past
			fixup.changetime = start->changetime - dt1;

			for ( int i = 0; i < m_nMaxCount; i++ )
			{
				fixup.value[i] = Lerp( 1-frac, prev->value[i], start->value[i] );
			}

			// Point previous sample at fixed version
			prev = &fixup;
		}

		for( int i = 0; i < m_nMaxCount; i++ )
		{
			// Note that QAngle has a specialization that will do quaternion interpolation here...
			if ( m_bLooping[ i ] )
			{
				out[ i ] = LoopingLerp_Hermite( frac, prev->value[i], start->value[i], end->value[i] );
			}
			else
			{
				out[ i ] = Lerp_Hermite( frac, prev->value[i], start->value[i], end->value[i] );
			}
		}
	}

	bool ValidOrder()
	{
		float newestchangetime = 0.0f;
		bool first = true;
		for ( int i = GetHead(); IsValidIndex( i ); i = GetNext( i ) )
		{
			CInterpolatedVarEntry *entry = &m_VarHistory[ i ];
			if ( first )
			{
				first = false;
				newestchangetime = entry->changetime;
				continue;
			}

			// They should get older as wel walk backwards
			if ( entry->changetime > newestchangetime )
			{
				Assert( 0 );
				return false;
			}

			newestchangetime = entry->changetime;
		}

		return true;
	}

	// The underlying data element
	Type								*m_pValue;
	// Store networked values so when we latch we can detect which values were changed via networking
	Type								m_LastNetworked[ COUNT ];
	int									m_fType;
	int									m_ListHead;
	bool								m_bLooping[ COUNT ];
	int									m_nMaxCount;
	static CUtlFixedLinkedList< CInterpolatedVarEntry >	m_VarHistory;
};

#endif // INTERPOLATEDVAR_H
