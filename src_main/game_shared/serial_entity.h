//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef SERIAL_ENTITY_H
#define SERIAL_ENTITY_H
#ifdef _WIN32
#pragma once
#endif


// -------------------------------------------------------------------------------------------------- //
// CSerialEntity. ehandles point at these.
// -------------------------------------------------------------------------------------------------- //

class CSerialEntity
{
public:
			CSerialEntity();

	IHandleEntity *m_pEntity;
	int	m_SerialNumber;
};


inline CSerialEntity::CSerialEntity()
{
	m_pEntity = 0;
	m_SerialNumber = 0;
}


#endif // SERIAL_ENTITY_H
