//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Defines a special dockable dialog bar that activates itself when
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

#ifndef WORLDCRAFTBAR_H
#define WORLDCRAFTBAR_H
#pragma once


class CWorldcraftBar : public CDialogBar
{
	public:

		CWorldcraftBar(void)
		{
		}

	protected:

		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

		DECLARE_MESSAGE_MAP()
};


#endif // WORLDCRAFTBAR_H
