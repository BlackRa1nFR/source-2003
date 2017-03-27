//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: interface class between viewport msg funcs and "c" based client dll
//
// $NoKeywords: $
//=============================================================================
#if !defined( IVIEWPORTMSGS_H )
#define IVIEWPORTMSGS_H
#ifdef _WIN32
#pragma once
#endif



class IViewPortMsgs 
{
public:
	// Message Handlers
	virtual int MsgFunc_ValClass(const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_TeamNames(const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_Feign(const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_Detpack(const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_VGUIMenu(const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_BuildSt( const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_RandomPC( const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_ServerName( const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_Spectator( const char *pszName, int iSize, void *pbuf ) = 0;
//	virtual int MsgFunc_AllowSpec( const char *pszName, int iSize, void *pbuf ) = 0;
	virtual int MsgFunc_SpecFade( const char *pszName, int iSize, void *pbuf ) = 0;	
	virtual int MsgFunc_ResetFade( const char *pszName, int iSize, void *pbuf ) = 0;	
};


extern IViewPortMsgs *gViewPortMsgs;

#endif // IVIEWPORTMSGS_H