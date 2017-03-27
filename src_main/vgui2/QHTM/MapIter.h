/*----------------------------------------------------------------------
Copyright (c) 1998 Lee Alexander
Please see the file "licence.txt" for licencing details.
File:		Map.h
Owner:	leea@pobox.com
Purpose:Iterator interface for map
----------------------------------------------------------------------*/

#ifndef MAP_ITER_H
#define MAP_ITER_H

#pragma warning( disable:4786 ) //identifier was truncated to '255' characters in the debug information

#ifndef MAP_CONTAINER_H
#include <map.h>
#endif //#ifndef MAP_CONTAINER_H

namespace Container
{
	template<class Key, class Value> 
	class CMapIter
	{
		public:
			CMapIter( const CMap<Key, Value> &map );

			//
			//	Operations
			void Reset();

			//
			//	Navigation
			void Next();

			//
			//	State
			bool EOL() const;

			//
			// Content
			Value &GetValue() const;
			const Key &GetKey() const;	 


		private:
			const CMap<Key, Value>	&m_Map;
			CMap<Key, Value>::BucketEntry *m_pBucket;
			CMap<Key, Value>::BucketEntry *m_pNext;
			UINT				m_iEntry;

		private:
			CMapIter();
			CMapIter( const CMapIter &);
			CMapIter& operator =( const CMapIter &);
	};

	#include <MapIter.Inl>
}

#pragma warning( default:4786 )


#endif //MAP_ITER_H