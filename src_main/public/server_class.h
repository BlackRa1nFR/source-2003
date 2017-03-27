//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// This helps build the server-side class list (which gets exposed to the engine).
//
//=============================================================================

#ifndef SERVER_CLASS_H
#define SERVER_CLASS_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "dt_send.h"
#include "networkstringtabledefs.h"


class ServerClass;
class SendTable;

extern ServerClass *g_pServerClassHead;


class ServerClass
{
public:
				ServerClass( char *pNetworkName, SendTable *pTable )
				{
					m_pNetworkName = pNetworkName;
					m_pTable = pTable;
					m_pNext = g_pServerClassHead;
					g_pServerClassHead = this;
					m_InstanceBaselineIndex = INVALID_STRING_INDEX;
				}

	const char*	GetName()		{ return m_pNetworkName; }


public:
	char						*m_pNetworkName;
	SendTable					*m_pTable;
	ServerClass					*m_pNext;
	int							m_ClassID;	// Managed by the engine.

	// This is an index into the network string table (sv.GetInstanceBaselineTable()).
	int							m_InstanceBaselineIndex; // INVALID_STRING_INDEX if not initialized yet.
};


class CBaseNetworkable;

// If you do a DECLARE_SERVERCLASS, you need to do this inside the class definition.
#define DECLARE_SERVERCLASS()									\
	public:														\
		virtual ServerClass* GetServerClass();					\
		static SendTable *m_pClassSendTable;					\
		template <typename T> friend int ServerClassInit(T *);	\
		virtual int YouForgotToImplementOrDeclareServerClass();	\

#define DECLARE_SERVERCLASS_NOBASE()							\
	public:														\
		template <typename T> friend int ServerClassInit(T *);	\

// Use this macro to expose your class's data across the network.
#define IMPLEMENT_SERVERCLASS( DLLClassName, sendTable ) \
	IMPLEMENT_SERVERCLASS_INTERNAL( DLLClassName, sendTable )

// You can use this instead of BEGIN_SEND_TABLE and it will do a DECLARE_SERVERCLASS automatically.
#define IMPLEMENT_SERVERCLASS_ST(DLLClassName, sendTable) \
	IMPLEMENT_SERVERCLASS_INTERNAL( DLLClassName, sendTable )\
	BEGIN_SEND_TABLE(DLLClassName, sendTable)

#define IMPLEMENT_SERVERCLASS_ST_NOBASE(DLLClassName, sendTable) \
	IMPLEMENT_SERVERCLASS_INTERNAL( DLLClassName, sendTable )\
	BEGIN_SEND_TABLE_NOBASE( DLLClassName, sendTable )


#ifdef _DEBUG
	#define CHECK_DECLARE_CLASS( DLLClassName, sendTable ) \
		template <typename T> int CheckDeclareClass_Access(T *); \
		template <> int CheckDeclareClass_Access<sendTable::ignored>(sendTable::ignored *) \
		{ \
			return DLLClassName::CheckDeclareClass( #DLLClassName ); \
		} \
		namespace sendTable \
		{ \
			int verifyDeclareClass = CheckDeclareClass_Access( (sendTable::ignored*)0 ); \
		}
#else
	#define CHECK_DECLARE_CLASS( DLLClassName, sendTable )
#endif


#define IMPLEMENT_SERVERCLASS_INTERNAL( DLLClassName, sendTable ) \
	namespace sendTable \
	{ \
		struct ignored; \
		extern SendTable g_SendTable; \
	} \
	CHECK_DECLARE_CLASS( DLLClassName, sendTable ) \
	static ServerClass g_##DLLClassName##_ClassReg(\
		#DLLClassName, \
		&sendTable::g_SendTable\
	); \
	\
	ServerClass* DLLClassName::GetServerClass() {return &g_##DLLClassName##_ClassReg;} \
	SendTable *DLLClassName::m_pClassSendTable = &sendTable::g_SendTable;\
	int DLLClassName::YouForgotToImplementOrDeclareServerClass() {return 0;}


#endif



