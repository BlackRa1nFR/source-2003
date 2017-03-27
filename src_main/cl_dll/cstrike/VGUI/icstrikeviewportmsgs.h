//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: interface class between viewport msg funcs and "c" based client dll
//
// $NoKeywords: $
//=============================================================================
#if !defined( ICSVIEWPORTMSGS_H )
#define ICSVIEWPORTMSGS_H
#ifdef _WIN32
#pragma once
#endif

//#include "vgui/IViewPortMsgs.h"

class ICSViewPortMsgs /*: public IViewPortMsgs*/
{
public:
	virtual int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf ) = 0;

};


extern ICSViewPortMsgs *gCSViewPortMsgs;

#endif // ICSVIEWPORTMSGS_H