/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	Mouse.cpp
Owner:	leea@gipsysoft.com
Purpose:	Put anything to do with Mr Mouse in here
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <WinHelper.h>

void GetMousePoint( WinHelper::CPoint &pt )
{
	GetCursorPos( pt );
}
