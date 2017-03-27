//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "client.h"
#include "networkstringtabledefs.h"


// ---------------------------------------------------------------------------------------- //
// C_ServerClassInfo implementation.
// ---------------------------------------------------------------------------------------- //

C_ServerClassInfo::C_ServerClassInfo()
{
	m_ClassName = NULL;
	m_DatatableName = NULL;
	m_InstanceBaselineIndex = INVALID_STRING_INDEX;
}

C_ServerClassInfo::~C_ServerClassInfo()
{
	delete [] m_ClassName;
	delete [] m_DatatableName;
}



