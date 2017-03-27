//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CL_ENTS_PARSE_H
#define CL_ENTS_PARSE_H
#ifdef _WIN32
#pragma once
#endif

void CL_Parse_PacketEntities( void );
void CL_Parse_DeltaPacketEntities( void );
void CL_DeleteDLLEntity( int iEnt, char *reason );


#endif // CL_ENTS_PARSE_H
