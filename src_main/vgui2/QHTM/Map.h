/*----------------------------------------------------------------------
Copyright (c) 1998 Lee Alexander
Please see the file "licence.txt" for licencing details.
File:		Map.h
Owner:	leea@pobox.com
Purpose:Interface for hash map
----------------------------------------------------------------------*/

#ifndef MAP_CONTAINER_H
#define MAP_CONTAINER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _WINDOWS_
	#include <windows.h>
#endif //#ifndef _WINDOWS_

#ifndef _INC_TCHAR
	#include <tchar.h>
#endif //#ifndef _INC_TCHAR

namespace Container
{
	template< class Key, class Value> class CMapIter;

	template<class Key, class Value> 
	class CMap
	{
		public:
		CMap( UINT uBucketArrayLength = 99 );
		virtual ~CMap();

		//
		// Operations
		Value *Lookup( const Key &key );
		const Value *Lookup( const Key &key ) const;
		
		void SetAt(const Key &key, Value value );
		Value &CreateAt( const Key &key );
		void RemoveAll();
		void RemoveAt( const Key &key );

		UINT GetSize() const { return m_uCount; };

		//
		//	**Implementation**, do use anything under here. the only reason why some of this is public is that there seems
		//	to be a bug in the MSVC 6.0 compiler that even though the CMapIter is a friend it still cannot access CMaps
		//	privates.
		public:
		struct BucketEntry
		{
			Value m_Value;
			Key		m_Key;

			BucketEntry *m_pNext;
			BucketEntry(): m_pNext( NULL ){}
			operator = ( const BucketEntry& ){ m_Value = rhs.m_Value; m_Key = rhs.m_Key; m_pNext = rhs.m_pNext; }
			BucketEntry( const BucketEntry &rhs ){ operator =( rhs ) }
		};
		const UINT	m_uBucketArrayLength;
		BucketEntry	**m_pEntrys;

		private:
		// richg - 19990224 - VC5 chokes if the declarator 'class' is specified here.
		// it produces a 'redefinition error'. Everything works fine if the
		// 'class' declarator is omitted, though.
		// friend class CMapIter< class Key, class Value>;
		friend class CMapIter< Key, Value>;
	
		BucketEntry *Find( UINT uHash, const Key &key ) const;


		UINT				m_uCount;

		private:
			CMap( const CMap & );
			CMap& operator =( const CMap & );
		};

	#include "Map.inl"


	////////////////////////////////////
	// Helper functions


	//
	//	Hash value for simple types
	template<class Key>
	inline UINT HashIt( const Key &key )
	{
		//
		// Default implementation. If you get a compile error here then you need to create a 'HashIt' function
		return ( (UINT)(void*)(DWORD)key) >> 4;
	}


	//
	//	Hash value for a LPCTSTR
	inline UINT HashIt( LPCTSTR pcszText )
	{
		UINT uHash = 0;
		while( *pcszText )
		{
			uHash = uHash << 1 ^ toupper( *pcszText++ );
		}

		return uHash;
	}

	//
	// Default implementation for comparison
	template<class Key>
	inline bool ElementsTheSame( const Key &lhs, const Key &rhs )
	{
		return lhs == rhs;
	}

	//
	//	Overriden comparison for LPCTSTR 
	inline bool ElementsTheSame( LPCTSTR pcszLHS, LPCTSTR pcszRHS )
	{
		return _tcsicmp( pcszLHS, pcszRHS ) == 0;
	}
	
}
#endif //#ifdef MAP_CONTAINER_H