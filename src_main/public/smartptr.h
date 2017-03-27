//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SMARTPTR_H
#define SMARTPTR_H
#ifdef _WIN32
#pragma once
#endif


class CRefCountAccessor
{
public:
	template< class T >
	static void AddRef( T *pObj )
	{
		pObj->AddRef();
	}

	template< class T >
	static void Release( T *pObj )
	{
		pObj->Release();
	}
};


// Smart pointers can be used to automatically free an object when nobody points
// at it anymore. Things contained in smart pointers must implement AddRef and Release
// functions. If those functions are private, then the class must make
// CRefCountAccessor a friend.
template<class T>
class CSmartPtr
{
public:
					CSmartPtr();
					CSmartPtr( T *pObj );
					CSmartPtr( const CSmartPtr<T> &other );
					~CSmartPtr();

	void			operator=( T *pObj );
	void			operator=( const CSmartPtr<T> &other );
	const T*		operator->() const;
	T*				operator->();
	bool			operator!() const;
	bool			IsValid() const;		// Tells if the pointer is valid.


private:
	T				*m_pObj;
};


template< class T >
inline CSmartPtr<T>::CSmartPtr()
{
	m_pObj = NULL;
}

template< class T >
inline CSmartPtr<T>::CSmartPtr( T *pObj )
{
	m_pObj = NULL;
	*this = pObj;
}

template< class T >
inline CSmartPtr<T>::CSmartPtr( const CSmartPtr<T> &other )
{
	m_pObj = NULL;
	*this = other;
}

template< class T >
inline CSmartPtr<T>::~CSmartPtr()
{
	if ( m_pObj )
		CRefCountAccessor::Release( m_pObj );
}

template< class T >
inline void CSmartPtr<T>::operator=( T *pObj )
{
	if ( pObj )
		CRefCountAccessor::AddRef( pObj );

	if ( m_pObj )
		CRefCountAccessor::Release( m_pObj );

	m_pObj = pObj;
}

template< class T >
inline void CSmartPtr<T>::operator=( const CSmartPtr<T> &other )
{
	*this = other.m_pObj;
}

template< class T >
inline const T* CSmartPtr<T>::operator->() const
{
	return m_pObj;
}

template< class T >
inline T* CSmartPtr<T>::operator->()
{
	return m_pObj;
}

template< class T >
inline bool CSmartPtr<T>::operator!() const
{
	return !m_pObj;
}

template< class T >
inline bool CSmartPtr<T>::IsValid() const
{
	return m_pObj != NULL;
}


#endif // SMARTPTR_H
