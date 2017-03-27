// exefuncs.h
#ifndef EXEFUNCS_H
#define EXEFUNCS_H

#include "interface.h"

#define LAUNCHER_INTERFACE_VERSION		"VLauncher002"

class IBaseLauncher
{
public:
	// Get viddef structure
	virtual void		VID_GetVID( struct viddef_s *pvid ) = 0;
	// Print/error callbacks
	virtual void        ErrorMessage( int nLevel, const char *pszErrorMessage ) = 0;
	// Retrieves the user's raw cd key
	virtual void        GetCDKey( char *pszCDKey, int *nLength, int *bDedicated ) = 0;
	// User has valid cd in the drive
	virtual int			IsValidCD( void ) = 0;
	// Game is switching directories
	virtual void        ChangeGameDirectory( const char *pszNewDirectory ) = 0;
	// Ask for localized version of string by id.
	virtual char		*GetLocalizedString( unsigned int resId ) = 0;
};

#endif