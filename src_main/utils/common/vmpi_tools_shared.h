//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef VMPI_TOOLS_SHARED_H
#define VMPI_TOOLS_SHARED_H
#ifdef _WIN32
#pragma once
#endif


// Packet IDs.
#define VMPI_SHARED_PACKET_ID		10

	#define VMPI_SUBPACKETID_DIRECTORIES	0	// qdir directories.
	#define VMPI_SUBPACKETID_DBINFO			1	// MySQL database info.
	#define VMPI_SUBPACKETID_MACHINE_NAME	2	// machine name
	#define VMPI_SUBPACKETID_CRASH			3	// A worker saying it crashed.
	#define VMPI_SUBPACKETID_MULTICAST_ADDR	4	// Filesystem multicast address.


class CDBInfo;
class CIPAddr;


// Send/receive the qdir info.
void SendQDirInfo();
void RecvQDirInfo();

void SendDBInfo( const CDBInfo *pInfo, unsigned long jobPrimaryID );
void RecvDBInfo( CDBInfo *pInfo, unsigned long *pJobPrimaryID );

void SendMachineName();

void SendMulticastIP( const CIPAddr *pAddr );
void RecvMulticastIP( CIPAddr *pAddr );

void VMPI_HandleCrash( const char *pMessage, bool bAssert );


#endif // VMPI_TOOLS_SHARED_H
