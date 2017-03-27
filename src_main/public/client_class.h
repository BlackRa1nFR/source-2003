//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( CLIENT_CLASS_H )
#define CLIENT_CLASS_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "dt_recv.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class Vector;
class CMouthInfo;


//-----------------------------------------------------------------------------
// represents a handle used only by the client DLL
//-----------------------------------------------------------------------------

#include "iclientrenderable.h"
#include "iclientnetworkable.h"


class ClientClass;
// Linked list of all known client classes
extern ClientClass *g_pClientClassHead;

// The serial number that gets passed in is used for ehandles.
typedef IClientNetworkable*	(*CreateClientClassFn)( int entnum, int serialNum );
typedef IClientNetworkable*	(*CreateEventFn)();

//-----------------------------------------------------------------------------
// Purpose: Client side class definition
//-----------------------------------------------------------------------------
class ClientClass
{
public:
	ClientClass( char *pNetworkName, CreateClientClassFn createFn, CreateEventFn createEventFn, RecvTable *pRecvTable )
	{
		m_pNetworkName	= pNetworkName;
		m_pCreateFn		= createFn;
		m_pCreateEventFn= createEventFn;
		m_pRecvTable	= pRecvTable;
		
		// Link it in
		m_pNext				= g_pClientClassHead;
		g_pClientClassHead	= this;
	}

public:
	CreateClientClassFn		m_pCreateFn;
	CreateEventFn			m_pCreateEventFn;	// Only called for event objects.
	char					*m_pNetworkName;
	RecvTable				*m_pRecvTable;
	ClientClass				*m_pNext;
};

#define DECLARE_CLIENTCLASS() \
	virtual int YouForgotToImplementOrDeclareClientClass();\
	virtual char const* GetClassName();\
	virtual ClientClass* GetClientClass();\
	static RecvTable *m_pClassRecvTable; \
	DECLARE_CLIENTCLASS_NOBASE()

#define DECLARE_CLIENTCLASS_NOBASE() \
	template <typename T> friend int ClientClassInit(T *);

// This macro adds a ClientClass to the linked list in g_pClientClassHead (so
// the list can be given to the engine).
// Use this macro to expose your client class to the engine.
// networkName must match the network name of a class registered on the server.
#define IMPLEMENT_CLIENTCLASS(clientClassName, dataTable, serverClassName) \
	INTERNAL_IMPLEMENT_CLIENTCLASS_PROLOGUE(clientClassName, dataTable, serverClassName) \
	static IClientNetworkable* _##clientClassName##_CreateObject( int entnum, int serialNum ) \
	{ \
		clientClassName *pRet = new clientClassName; \
		if ( !pRet ) \
			return 0; \
		pRet->Init( entnum, serialNum ); \
		return pRet; \
	} \
	ClientClass __g_##clientClassName##ClientClass(#serverClassName, \
													_##clientClassName##_CreateObject, \
													NULL,\
													&dataTable::g_RecvTable);

// The IMPLEMENT_CLIENTCLASS_DT macros do IMPLEMENT_CLIENT_CLASS and also do BEGIN_RECV_TABLE.
#define IMPLEMENT_CLIENTCLASS_DT(clientClassName, dataTable, serverClassName)\
	IMPLEMENT_CLIENTCLASS(clientClassName, dataTable, serverClassName)\
	BEGIN_RECV_TABLE(clientClassName, dataTable)

#define IMPLEMENT_CLIENTCLASS_DT_NOBASE(clientClassName, dataTable, serverClassName)\
	IMPLEMENT_CLIENTCLASS(clientClassName, dataTable, serverClassName)\
	BEGIN_RECV_TABLE_NOBASE(clientClassName, dataTable)
	

// Using IMPLEMENT_CLIENTCLASS_EVENT means the engine thinks the entity is an event so the entity
// is responsible for freeing itself.
#define IMPLEMENT_CLIENTCLASS_EVENT(clientClassName, dataTable, serverClassName)\
	INTERNAL_IMPLEMENT_CLIENTCLASS_PROLOGUE(clientClassName, dataTable, serverClassName)\
	static clientClassName __g_##clientClassName##; \
	static IClientNetworkable* _##clientClassName##_CreateObject() {return &__g_##clientClassName##;}\
	ClientClass __g_##clientClassName##ClientClass(#serverClassName, \
													NULL,\
													_##clientClassName##_CreateObject, \
													&dataTable::g_RecvTable);

#define IMPLEMENT_CLIENTCLASS_EVENT_DT(clientClassName, dataTable, serverClassName)\
	namespace dataTable {extern RecvTable g_RecvTable;}\
	IMPLEMENT_CLIENTCLASS_EVENT(clientClassName, dataTable, serverClassName)\
	BEGIN_RECV_TABLE(clientClassName, dataTable)


// Register a client event singleton but specify a pointer to give to the engine rather than
// have a global instance. This is useful if you're using Initializers and your object's constructor
// uses some other global object (so you must use Initializers so you're constructed afterwards).
#define IMPLEMENT_CLIENTCLASS_EVENT_POINTER(clientClassName, dataTable, serverClassName, ptr)\
	INTERNAL_IMPLEMENT_CLIENTCLASS_PROLOGUE(clientClassName, dataTable, serverClassName)\
	static IClientNetworkable* _##clientClassName##_CreateObject() {return ptr;}\
	ClientClass __g_##clientClassName##ClientClass(#serverClassName, \
													NULL,\
													_##clientClassName##_CreateObject, \
													&dataTable::g_RecvTable);

#define IMPLEMENT_CLIENTCLASS_EVENT_NONSINGLETON(clientClassName, dataTable, serverClassName)\
	static IClientNetworkable* _##clientClassName##_CreateObject() \
	{ \
		clientClassName *p = new clientClassName; \
		if ( p ) \
			p->Init( -1, 0 ); \
		return p; \
	} \
	ClientClass __g_##clientClassName##ClientClass(#serverClassName, \
													NULL,\
													_##clientClassName##_CreateObject, \
													&dataTable::g_RecvTable);


// Used internally..
#define INTERNAL_IMPLEMENT_CLIENTCLASS_PROLOGUE(clientClassName, dataTable, serverClassName) \
	namespace dataTable {extern RecvTable g_RecvTable;}\
	extern ClientClass __g_##clientClassName##ClientClass;\
	RecvTable*		clientClassName::m_pClassRecvTable = &dataTable::g_RecvTable;\
	char const*		clientClassName::GetClassName() {return #clientClassName;}\
	int				clientClassName::YouForgotToImplementOrDeclareClientClass() {return 0;}\
	ClientClass*	clientClassName::GetClientClass() {return &__g_##clientClassName##ClientClass;}

#endif // CLIENT_CLASS_H
