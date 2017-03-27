
/************ (C) Copyright 2002 Valve, L.L.C. All rights reserved. ***********
**
** The copyright to the contents herein is the property of Valve, L.L.C.
** The contents may be used and/or copied only with the written permission of
** Valve, L.L.C., or in accordance with the terms and conditions stipulated in
** the agreement/contract under which the contents have been supplied.
**
*******************************************************************************
**
** Contents:
**
**		This file provides the public interface to the Steam service.  This
**		interface is described in the SDK documentation.
**
******************************************************************************/


#ifndef INCLUDED_STEAM_H
#define INCLUDED_STEAM_H


#if defined(_MSC_VER) && (_MSC_VER > 1000)
#pragma once
#endif

#ifndef INCLUDED_STEAM_COMMON_STEAMCOMMON_H
	#include "SteamCommon.h"
#endif


#ifdef __cplusplus
extern "C"
{
#endif


/******************************************************************************
**
** Exported function prototypes
**
******************************************************************************/


/*
** Initialization and misc
*/

STEAM_API int			STEAM_CALL	SteamStartup( unsigned int uUsingMask, TSteamError *pError );
STEAM_API int			STEAM_CALL	SteamCleanup( TSteamError *pError );
STEAM_API void			STEAM_CALL	SteamClearError( TSteamError *pError );
STEAM_API int			STEAM_CALL	SteamGetVersion( char *szVersion, unsigned int uVersionBufSize );

/*
** Asynchrounous call handling
*/

STEAM_API int				STEAM_CALL	SteamProcessCall( SteamCallHandle_t handle, TSteamProgress *pProgress, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamAbortCall( SteamCallHandle_t handle, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamBlockingCall( SteamCallHandle_t handle, unsigned int uiProcessTickMS, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamSetMaxStallCount( unsigned int uNumStalls, TSteamError *pError );
							
/*
** Filesystem
*/

STEAM_API int				STEAM_CALL	SteamMountAppFilesystem( TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamUnmountAppFilesystem( TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamMountFilesystem( unsigned int uAppId, const char *szMountPath, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamUnmountFilesystem( const char *szMountPath, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamStat( const char *cszName, TSteamElemInfo *pInfo, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamSetvBuf( SteamHandle_t hFile, void* pBuf, ESteamBufferMethod eMethod, unsigned int uBytes, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamFlushFile( SteamHandle_t hFile, TSteamError *pError );
STEAM_API SteamHandle_t		STEAM_CALL	SteamOpenFile( const char *cszName, const char *cszMode, TSteamError *pError );
STEAM_API SteamHandle_t		STEAM_CALL	SteamOpenTmpFile( TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamCloseFile( SteamHandle_t hFile, TSteamError *pError );
STEAM_API unsigned int		STEAM_CALL	SteamReadFile( void *pBuf, unsigned int uSize, unsigned int uCount, SteamHandle_t hFile, TSteamError *pError );
STEAM_API unsigned int		STEAM_CALL	SteamWriteFile( const void *pBuf, unsigned int uSize, unsigned int uCount, SteamHandle_t hFile, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetc( SteamHandle_t hFile, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamPutc( int cChar, SteamHandle_t hFile, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamPrintFile( SteamHandle_t hFile, TSteamError *pError, const char *cszFormat, ... );
STEAM_API int				STEAM_CALL	SteamSeekFile( SteamHandle_t hFile, long lOffset, ESteamSeekMethod, TSteamError *pError );
STEAM_API long				STEAM_CALL	SteamTellFile( SteamHandle_t hFile, TSteamError *pError );
STEAM_API long				STEAM_CALL	SteamSizeFile( SteamHandle_t hFile, TSteamError *pError );
STEAM_API SteamHandle_t		STEAM_CALL	SteamFindFirst( const char *cszPattern, ESteamFindFilter eFilter, TSteamElemInfo *pFindInfo, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamFindNext( SteamHandle_t hDirectory, TSteamElemInfo *pFindInfo, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamFindClose( SteamHandle_t hDirectory, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetLocalFileCopy( const char *cszName, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamIsFileImmediatelyAvailable( const char *cszName, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamHintResourceNeed( const char *cszMountPath, const char *cszMasterList, int bForgetEverything, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamForgetAllHints( const char *cszMountPath, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamPauseCachePreloading( const char *cszMountPath, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamResumeCachePreloading( const char *cszMountPath, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamWaitForResources( const char *cszMountPath, const char *cszMasterList, TSteamError *pError );

/*
** Logging
*/

STEAM_API SteamHandle_t		STEAM_CALL	SteamCreateLogContext( const char *cszName );
STEAM_API int				STEAM_CALL	SteamLog( SteamHandle_t hContext, const char *cszMsg );
STEAM_API void				STEAM_CALL	SteamLogResourceLoadStarted( const char *cszMsg );
STEAM_API void				STEAM_CALL	SteamLogResourceLoadFinished( const char *cszMsg );

/*
** Account
*/

STEAM_API SteamCallHandle_t	STEAM_CALL	SteamCreateAccount( const char *cszUser, const char *cszPassphrase, const char *cszCreationKey, const char *cszPersonalQuestion, const char *cszAnswerToQuestion, int *pbCreated, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamDeleteAccount( TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamIsLoggedIn( int *pbIsLoggedIn, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamSetUser( const char *cszUser, int *pbUserSet, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetUser( char *szUser, unsigned int uBufSize, unsigned int *puUserChars, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamLogin( const char *cszUser, const char *cszPassphrase, int bIsSecureComputer, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamLogout( TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamIsSecureComputer(  int *pbIsSecure, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamRefreshLogin( const char *cszPassphrase, int bIsSecureComputer, TSteamError * pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamSubscribe( unsigned int uSubscriptionId, const TSteamSubscriptionBillingInfo *pSubscriptionBillingInfo, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamUnsubscribe( unsigned int uSubscriptionId, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamIsSubscribed( unsigned int uSubscriptionId, int *pbIsSubscribed, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamIsAppSubscribed( unsigned int uAppId, int *pbIsAppSubscribed, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetSubscriptionStats( TSteamSubscriptionStats *pSubscriptionStats, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetSubscriptionIds( unsigned int *puIds, unsigned int uMaxIds, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamEnumerateSubscription( unsigned int uId, TSteamSubscription *pSubscription, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetAppStats( TSteamAppStats *pAppStats, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetAppIds( unsigned int *puIds, unsigned int uMaxIds, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamEnumerateApp( unsigned int uId, TSteamApp *pApp, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamEnumerateAppLaunchOption( unsigned int uAppId, unsigned int uLaunchOptionIndex, TSteamAppLaunchOption *pLaunchOption, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamEnumerateAppIcon( unsigned int uAppId, unsigned int uIconIndex, unsigned char *pIconData, unsigned int uIconDataBufSize, unsigned int *puSizeOfIconData, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamEnumerateAppVersion( unsigned int uAppId, unsigned int uVersionIndex, TSteamAppVersion *pAppVersion, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamWaitForAppReadyToLaunch( unsigned int uAppId, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamLaunchApp( unsigned int uAppId, unsigned int uLaunchOption, const char *cszArgs, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamIsCacheLoadingEnabled( unsigned int uAppId, int *pbIsLoading, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamStartLoadingCache( unsigned int uAppId, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamStopLoadingCache( unsigned int uAppId, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamFlushCache( unsigned int uAppId, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamLoadCacheFromDir( unsigned int uAppId, const char *szPath, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetCacheDefaultDirectory( char *szPath, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamSetCacheDefaultDirectory( const char *szPath, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetAppCacheDir( unsigned int uAppId, char *szPath, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamMoveApp( unsigned int uAppId, const char *szPath, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamGetAppCacheSize( unsigned int uAppId, unsigned int *pCacheSizeInMb, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamSetAppCacheSize( unsigned int uAppId, unsigned int nCacheSizeInMb, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamSetAppVersion( unsigned int uAppId, unsigned int uAppVersionId, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamUninstall( TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamSetNotificationCallback( SteamNotificationCallback_t pCallbackFunction, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamSetNewPassword( const char *cszUser, const char *cszAnswerToQuestion, const SteamSalt_t *cpSaltForAnswer, const char *cszNewPassphrase, int *pbChanged, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamGetPersonalQuestion( const char *cszUser, SteamPersonalQuestion_t PersonalQuestion, SteamSalt_t *pSaltForAnswer, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamChangePassword( const char *cszCurrentPassphrase, const char *cszNewPassphrase, int *pbChanged, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamChangePersonalQA( const char *cszCurrentPassphrase, const char *cszNewPersonalQuestion, const char *cszNewAnswerToQuestion, int *pbChanged, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamChangeEmailAddress( const char *cszNewEmailAddress, int *pbChanged, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamEmailVerified( const char *cszEmailVerificationKey, int *pbVerified, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamSendVerificationEmail( int *pbSent, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamUpdateAccountBillingInfo( const TSteamPaymentCardInfo *pPaymentCardInfo, int *pbChanged, TSteamError *pError );
STEAM_API SteamCallHandle_t	STEAM_CALL	SteamUpdateSubscriptionBillingInfo( unsigned int uSubscriptionId, const TSteamSubscriptionBillingInfo *pSubscriptionBillingInfo, int *pbChanged, TSteamError *pError );
STEAM_API int				STEAM_CALL  SteamGetSponsorUrl( unsigned int uAppId, char *szUrl, unsigned int uBufSize, unsigned int *pUrlChars, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetAppUpdateStats( unsigned int uAppId, TSteamUpdateStats *pUpdateStats, TSteamError *pError );
STEAM_API int				STEAM_CALL	SteamGetTotalUpdateStats( TSteamUpdateStats *pUpdateStats, TSteamError *pError );

/*
** User ID exported functions are in SteamUserIdValidation.h
*/


#ifdef __cplusplus
}
#endif

#endif /* #ifndef INCLUDED_STEAM_H */
