/*----------------------------------------------------------------------
Copyright (c) 1998 Lee Alexander
Please see the file "licence.txt" for licencing details.
File:		Map.inl
Owner:	leea
Purpose:Implementation of CMap
----------------------------------------------------------------------*/

template<class Key, class Value>
CMap<Key, Value>::CMap( UINT uBucketArrayLength )
	:	m_uBucketArrayLength( uBucketArrayLength )
	,	m_uCount( 0 )
{
	m_pEntrys = new BucketEntry*[m_uBucketArrayLength];

	for( UINT i = 0; i < m_uBucketArrayLength; i++ )
	{
		m_pEntrys[i] = NULL;
	} 
}

template<class Key, class Value>
CMap<Key, Value>::~CMap()
{
	RemoveAll();
	delete [] m_pEntrys;
	m_pEntrys = NULL;
}


template<class Key, class Value>
void CMap<Key, Value>::RemoveAll()
{
	if( m_uCount )
	{
		for( UINT i = 0; i < m_uBucketArrayLength; i++ )
		{
			BucketEntry *pEntry = m_pEntrys[i];
			while( pEntry )
			{
				BucketEntry *pEntryNext = pEntry -> m_pNext;
				delete pEntry;
				pEntry = pEntryNext;
			}

			m_pEntrys[i] = NULL;
		}

		m_uCount = 0;
	}
}


template<class Key, class Value>
Value *CMap<Key, Value>::Lookup( const Key &key )
{
	UINT uHash = HashIt( key ) % m_uBucketArrayLength;
	BucketEntry	*pEntry = Find( uHash, key );
	return pEntry != NULL ? &pEntry -> m_Value : NULL;
}

template<class Key, class Value>
const Value * CMap<Key, Value>::Lookup( const Key &key ) const
{
	UINT uHash = HashIt( key ) % m_uBucketArrayLength;
	BucketEntry	*pEntry = Find( uHash, key );
	return pEntry != NULL ? &pEntry -> m_Value : NULL;
}


template<class Key, class Value>
CMap<Key, Value>::BucketEntry *CMap<Key, Value>::Find( UINT uHash, const Key &key ) const
{
	ASSERT( m_pEntrys );

	for( BucketEntry *pEntry = m_pEntrys[uHash]; pEntry != NULL; pEntry = pEntry -> m_pNext )
	{
		if( ElementsTheSame( pEntry -> m_Key, key ) )
		{
			return pEntry;
		}
	}

	return NULL;
}

template<class Key, class Value>
Value &CMap<Key, Value>::CreateAt( const Key &key )
{
	UINT uHash = HashIt( key ) % m_uBucketArrayLength;
	BucketEntry	*pEntry = Find( uHash , key);

	if( pEntry )
	{
		RemoveAt( key ); // get rid of old entry
	}

	pEntry = new BucketEntry;
	m_uCount++;
	pEntry -> m_Key = key;

	BucketEntry	*pRootEntry = m_pEntrys[uHash];
	m_pEntrys[uHash] = pEntry;
	if( pRootEntry )
	{
		pEntry -> m_pNext = pRootEntry;
	}
	else
	{
		pEntry -> m_pNext = NULL;
	}
	return pEntry -> m_Value;
}

template<class Key, class Value>
void CMap<Key, Value>::SetAt( const Key &key, Value value )
{
	UINT uHash = HashIt( key ) % m_uBucketArrayLength;
	BucketEntry	*pEntry = Find( uHash , key);

	if( pEntry )
	{
		pEntry -> m_Value = value;
	}
	else
	{
		BucketEntry	*pEntry = new BucketEntry;
		m_uCount++;
		pEntry -> m_Value = value;
		pEntry -> m_Key = key;

		BucketEntry	*pRootEntry = m_pEntrys[uHash];
		m_pEntrys[uHash] = pEntry;
		if( pRootEntry )
		{
			pEntry -> m_pNext = pRootEntry;
		}
		else
		{
			pEntry -> m_pNext = NULL;
		}
	}
}

template<class Key, class Value>
void CMap<Key, Value>::RemoveAt( const Key &key )
{
	UINT uHash = HashIt( key ) % m_uBucketArrayLength;
	BucketEntry *pLastItem = NULL;
	for( BucketEntry *pEntry = m_pEntrys[uHash]; pEntry != NULL; pLastItem = pEntry, pEntry = pEntry -> m_pNext )
	{
		if( ElementsTheSame( pEntry -> m_Key, key ) )
		{
			if( pLastItem )
			{
				pLastItem -> m_pNext = pEntry -> m_pNext;
			}
			else
			{
				m_pEntrys[uHash] = pEntry -> m_pNext;
			}
			delete pEntry;
			m_uCount--;
			break;
		}
	}
}
