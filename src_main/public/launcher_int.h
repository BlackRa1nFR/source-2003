#if !defined( LAUNCHER_INT_H )
#define LAUNCHER_INT_H
#ifdef _WIN32
#pragma once
#endif

class CBaseLauncher : public IBaseLauncher
{
public:
	// Get viddef structure
	virtual void		VID_GetVID( struct viddef_s *pvid );
	// Print/error callbacks
	virtual void        ErrorMessage( int nLevel, const char *pszErrorMessage );
	// Retrieves the user's raw cd key
	virtual void        GetCDKey( char *pszCDKey, int *nLength, int *bDedicated );
	// User has valid cd in the drive
	virtual int			IsValidCD( void );
	// Game is switching directories
	virtual void        ChangeGameDirectory( const char *pszNewDirectory );
	// Ask for localized version of string by id.
	virtual char		*GetLocalizedString( unsigned int resId );
};

#define VLAUNCHER_ENGINE_API_VERSION "VLAUNCHER_ENGINE_API_VERSION002"

extern CBaseLauncher g_LauncherInterface;

int			Launcher_CPUMhz( void );
void		VID_GetVID( struct viddef_s *pvid);
void        ErrorMessage(int nLevel, const char *pszErrorMessage);
void		UTIL_GetCDKey( char *pszCDKey, int *nLen, int *bDedicated );
int			Launcher_IsValidCD(void);
void        Launcher_ChangeGameDirectory( const char *pszNewDirectory );
char		*Launcher_GetLocalizedString( unsigned int resId );

#endif // LAUNCHER_INT_H