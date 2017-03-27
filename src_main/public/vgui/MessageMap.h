//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MESSAGEMAP_H
#define MESSAGEMAP_H

#ifdef _WIN32
#pragma once
#endif

// more flexible than default pointers to members code required for casting member function pointers
#pragma pointers_to_members( full_generality, virtual_inheritance )

namespace vgui
{

////////////// MESSAGEMAP DEFINITIONS //////////////

#ifndef offsetof
#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(p)	(sizeof(p)/sizeof(p[0]))
#endif


//-----------------------------------------------------------------------------
// Purpose: parameter data type enumeration
//			used internal but the shortcut macros require this to be exposed
//-----------------------------------------------------------------------------
enum DataType_t
{
	DATATYPE_VOID,
	DATATYPE_CONSTCHARPTR,
	DATATYPE_INT,
	DATATYPE_FLOAT,
	DATATYPE_PTR,
	DATATYPE_BOOL,
	DATATYPE_KEYVALUES,
	DATATYPE_CONSTWCHARPTR,
};

class Panel;

typedef void (Panel::*MessageFunc_t)(void);

//-----------------------------------------------------------------------------
// Purpose: Single item in a message map
//			Contains the information to map a string message name with parameters
//			to a function call
//-----------------------------------------------------------------------------
struct MessageMapItem_t
{
	const char *name;
	int id;
	MessageFunc_t func;

	int numParams;

	DataType_t firstParamType;
	const char *firstParamName;

	DataType_t secondParamType;
	const char *secondParamName;

	int nameSymbol;
	int firstParamSymbol;
	int secondParamSymbol;
};

// Message mapping macros
// examples of use in vgui_controls project
// only macros defined work, other variable combinations not supported yet

// no parameters
#define MAP_MESSAGE( type, name, func )						{ name, 0, (vgui::MessageFunc_t)(type::func), 0 }
#define MAP_MSGID( type, name, func )						{ "", name, (vgui::MessageFunc_t)(type::func), 0 }

// implicit single parameter (params is the data store)
#define MAP_MESSAGE_PARAMS( type, name, func )				{ name, 0, (vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_KEYVALUES, NULL }
#define MAP_MSGID_PARAMS( type, name, func )				{ "", name, (vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_KEYVALUES, NULL }

// single parameter
#define MAP_MESSAGE_PTR( type, name, func, param1 )			{ name, 0, (vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_PTR, param1 }
#define MAP_MESSAGE_INT( type, name, func, param1 )			{ name, 0, (vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_INT, param1 }
#define MAP_MSGID_INT( type, name, func, param1 )			{ "", name,(vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_INT, param1 }
#define MAP_MESSAGE_BOOL( type, name, func, param1 )		{ name, 0, (vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_BOOL, param1 }
#define MAP_MESSAGE_FLOAT( type, name, func, param1 )		{ name, 0, (vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_FLOAT, param1 }
#define MAP_MESSAGE_PTR( type, name, func, param1 )			{ name, 0, (vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_PTR, param1 }
#define MAP_MESSAGE_CONSTCHARPTR( type, name, func, param1) { name, 0, (vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_CONSTCHARPTR, param1 }
#define MAP_MESSAGE_CONSTWCHARPTR( type, name, func, param1) { name, 0, (vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_CONSTWCHARPTR, param1 }
#define MAP_MSGID_CONSTCHARPTR( type, name, func, param1 )	{ "", name,(vgui::MessageFunc_t)(type::func), 1, vgui::DATATYPE_CONSTCHARPTR, param1 }

// two parameters
#define MAP_MESSAGE_INT_INT( type, name, func, param1, param2 ) { name, 0, (vgui::MessageFunc_t)type::func, 2, vgui::DATATYPE_INT, param1, vgui::DATATYPE_INT, param2 }
#define MAP_MESSAGE_PTR_INT( type, name, func, param1, param2 ) { name, 0, (vgui::MessageFunc_t)type::func, 2, vgui::DATATYPE_PTR, param1, vgui::DATATYPE_INT, param2 }
#define MAP_MESSAGE_INT_CONSTCHARPTR( type, name, func, param1, param2 ) { name, 0, (vgui::MessageFunc_t)type::func, 2, vgui::DATATYPE_INT, param1, vgui::DATATYPE_CONSTCHARPTR, param2 }
#define MAP_MESSAGE_PTR_CONSTCHARPTR( type, name, func, param1, param2 ) { name, 0, (vgui::MessageFunc_t)type::func, 2, vgui::DATATYPE_PTR, param1, vgui::DATATYPE_CONSTCHARPTR, param2 }
#define MAP_MESSAGE_PTR_CONSTWCHARPTR( type, name, func, param1, param2 ) { name, 0, (vgui::MessageFunc_t)type::func, 2, vgui::DATATYPE_PTR, param1, vgui::DATATYPE_CONSTWCHARPTR, param2 }
#define MAP_MESSAGE_CONSTCHARPTR_CONSTCHARPTR( type, name, func, param1, param2 ) { name, 0, (vgui::MessageFunc_t)type::func, 2, vgui::DATATYPE_CONSTCHARPTR, param1, vgui::DATATYPE_CONSTCHARPTR, param2 }

// if more parameters are needed, just use MAP_MESSAGE_PARAMS() and pass the keyvalue set into the function


//-----------------------------------------------------------------------------
// Purpose: stores the list of objects in the hierarchy
//			used to iterate through an object's message maps
//-----------------------------------------------------------------------------
struct PanelMap_t
{
	MessageMapItem_t *dataDesc;
	int dataNumFields;
	const char *dataClassName;
	PanelMap_t *baseMap;
	int processed;
};

// for use in class declarations
// declares the static variables and functions needed for the data description iteration
#define DECLARE_PANELMAP() \
	static vgui::PanelMap_t m_PanelMap; \
	static vgui::MessageMapItem_t m_MessageMap[]; \
	virtual vgui::PanelMap_t *GetPanelMap( void );

// could embed typeid() into here as well?
#define IMPLEMENT_PANELMAP( derivedClass, baseClass ) \
	vgui::PanelMap_t derivedClass::m_PanelMap = { derivedClass::m_MessageMap, ARRAYSIZE(derivedClass::m_MessageMap), #derivedClass, &baseClass::m_PanelMap }; \
	vgui::PanelMap_t *derivedClass::GetPanelMap( void ) { return &m_PanelMap; }

} // namespace vgui


#endif // MESSAGEMAP_H
