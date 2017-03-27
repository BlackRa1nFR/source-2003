//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef AI_RESPONSESYSTEM_H
#define AI_RESPONSESYSTEM_H
#ifdef _WIN32
#pragma once
#endif

class AI_CriteriaSet;
class AI_Response;

class IResponseSystem
{
public:
	virtual bool FindBestResponse( const AI_CriteriaSet& set, AI_Response& response ) = 0;
};

IResponseSystem *InstancedResponseSystemCreate( const char *scriptfile );

#endif // AI_RESPONSESYSTEM_H
