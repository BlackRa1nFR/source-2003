//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Implements a special dockable dialog bar that activates itself when
//			the mouse cursor moves over it. This enables stacking of the
//			bars with only a small portion of each visible.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Worldcraft.h"
#include "WorldcraftBar.h"


BEGIN_MESSAGE_MAP(CWorldcraftBar, CDialogBar)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Automagically bring this bar to the top when the mouse cursor passes
//			over it.
// Input  : pWnd - 
//			nHitTest - 
//			message - 
// Output : Always returns FALSE.
//-----------------------------------------------------------------------------
BOOL CWorldcraftBar::OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message)
{
	if (APP()->IsActiveApp())
	{
		// The control bar window is actually our grandparent.
		CWnd *pwnd = GetParent();
		if (pwnd != NULL)
		{
			pwnd = pwnd->GetParent();
			if (pwnd != NULL)
			{
				pwnd->BringWindowToTop();
			}
		}
	}

	return(FALSE);
}
