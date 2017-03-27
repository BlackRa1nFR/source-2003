//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Id: $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

// Initializers are a way to register your object to be initialized at startup time.
// They're a good way to have global variables without worrying about dependent
// constructors being called. They also make it so init code doesn't depend on the
// global objects it's initializing.

// To use initializers, just use REGISTER_INITIALIZER to register your global variable like this:
// class SomeClass {....}
// SomeClass *g_pSomeClassSingleton = NULL;
// REGISTER_INITIALIZER(SomeClass, &g_pSomeClassSingleton);

#ifndef INITIALIZER_H
#define INITIALIZER_H


typedef void* (*CreateInitializerObjectFn)();
typedef void (*DeleteInitializerObjectFn)(void *ptr);

class Initializer
{
public:
					Initializer(void **pVar, CreateInitializerObjectFn createFn, DeleteInitializerObjectFn deleteFn);

	// Allocates all the global objects.
	static bool		InitializeAllObjects();
	
	// Free all the global objects.
	static void		FreeAllObjects();


private:
	static Initializer			*s_pInitializers;
	
	void						**m_pVar;
	CreateInitializerObjectFn	m_CreateFn;
	DeleteInitializerObjectFn	m_DeleteFn;
	Initializer					*m_pNext;
};


#define REGISTER_INITIALIZER(className, varPointer)												\
	static void* __Initializer__Create##className##Fn()				{return new className;}		\
	static void* __Initializer__Delete##className##Fn(void *ptr)	{delete (className*)ptr;}	\
	static Initializer g_Initializer_##className##(varPointer, __Initializer__Create##className##Fn, __Initializer__Delete##className##Fn);

#define REGISTER_FUNCTION_INITIALIZER(functionName)	\
	static void* __Initializer__Create##functionName##Fn()			{ functionName(); return 0; } \
	static Initializer g_Initializer_##functionName##(0, __Initializer__Create##functionName##Fn, 0);

#endif

