/*----------------------------------------------------------------------
Copyright (c) 1998 Lee Alexander
Please see the file "licence.txt" for licencing details.
File:		Array.h
Owner:	leea@pobox.com
Purpose:useful little array class
----------------------------------------------------------------------*/

#ifndef ARRAY_CONTAINER_H
#define ARRAY_CONTAINER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifndef _INC_NEW
	#include <new.h>
#endif // #ifndef _INC_NEW

#ifndef _WINDOWS_
	#include <windows.h>
#endif //#ifndef _WINDOWS_

namespace Container
{
	template< class T>
	class CArray
	{
		public:
			CArray();
			explicit CArray( UINT uSize );
			CArray( const CArray<T> &arrToCopy );
			virtual ~CArray();
			void Add( const T &newItem );
			void Add( const T *p, UINT uCount );
			T &Add();

			void InsertAt( UINT iPos, const T &newItem, UINT uCount = 1 );
			void InsertAt( UINT iPos, const CArray<T> &arrToCopy );
			void InsertAt( UINT iPos, const T *p, UINT uCount );

			T	&GetAt( UINT iPos) { return operator [] ( iPos ); }
			const T	&GetAt( UINT iPos) const { return operator [] ( iPos ); }

			T	&operator[](UINT uValue);
			const T	&operator[](UINT iPos) const;
			CArray<T>	&operator = ( const CArray<T> &rhs );

			void RemoveAll();
			void RemoveAt( UINT iPos, UINT uItems = 1 );

			UINT GetSize() const;
			bool SetSize( UINT uSize ); // returns true if a reallocation was needed on the array


			const T *GetData() const;
			T *GetData();
		
		private:
			T			*m_pItems;
			UINT	m_uGrowBy
						, m_uItemsAllocated
						,	m_uItemCount;

	};

	#include "Array.inl"

	///////////////////////////////////////////////////////
	///////////////////////////////////////////////////////
	// CArray overides

	template<class T> void MoveItemsMemOverlap( T *pSource, T *pDestination, UINT uItems )
	//
	//	Moves items from pSource to pDestination. Both memory areas might overlap
	{
		MoveMemory( pDestination, pSource, uItems * sizeof( T ) );
	}


	template<class T> void MoveItemsNoMemOverlap( T *pSource, T *pDestination, UINT uItems )
	//
	//	Moves items from pSource to pDestination. Both memory areas are guaranteed not
	//	to overlap
	{
		CopyMemory( pDestination, pSource, uItems * sizeof( T ) );
	}

	template<class T> void CopyItems( const T *pSource, T *pDestination, UINT uItems )
	//
	//	Copies items from pSource to pDestination. Both memory areas might overlap
	{
		while( uItems-- )
		{
			*pDestination++ = *pSource++;
		}
	}


	template<class T> void ConstructItems( T *pItems, UINT uItems )
	{
		ZeroMemory( pItems, uItems * sizeof( T ) );

		//
		// Call the constructor(s)
		for( ; uItems--; pItems++ )
		{
			::new((void*)pItems) T;
		}
	}

	template<class T> void DestructItems( T *pItems, UINT uItems )
	{
		for( ;uItems--; pItems++ )
		{
			pItems -> ~T();
		}
	}

}

#endif // ARRAY_CONTAINER_H
