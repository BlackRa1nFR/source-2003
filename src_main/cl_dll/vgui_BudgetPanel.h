//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Does anyone ever read this?
//
//=============================================================================

#ifndef VGUI_BUDGETPANEL_H
#define VGUI_BUDGETPANEL_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include "vgui_BudgetHistoryPanel.h"
#include "vgui_BudgetBarGraphPanel.h"
#include "tier0/vprof.h"
#include "hudelement.h"

class CBudgetPanel : public CHudElement, public vgui::Frame
{
	typedef vgui::Frame BaseClass;

public:
	CBudgetPanel( const char *pElementName );
	~CBudgetPanel();

	DECLARE_MULTIPLY_INHERITED();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Paint( void );
	virtual void PerformLayout();

	void Destroy();
	void Rebuild();
	void Initialize( void );

	// CHudBase overrides
	virtual void Init( void );
	virtual bool ShouldDraw();

	virtual void SnapshotVProfHistory( void );


	const double *GetBudgetGroupData( int &nGroups, int &nSamplesPerGroup, int &nSampleOffset ) const
	{
		nGroups = g_VProfCurrentProfile.GetNumBudgetGroups();
		nSamplesPerGroup = VPROF_HISTORY_COUNT;
		nSampleOffset = m_BudgetHistoryOffset;
		if( m_BudgetGroupTimes.Count() == 0 )
		{
			return NULL;
		}
		else
		{
			return &m_BudgetGroupTimes[0].m_Time[0];
		}
	}

	// Command handlers
	void UserCmd_ShowBudgetPanel( void );
	void UserCmd_HideBudgetPanel( void );
	void OnNumBudgetGroupsChanged( void );
	// fixme: make an internal class for this shit.
private:
	void UpdateWindowGeometry();
	void ClearTimesForAllGroupsForThisFrame( void );
	void ClearAllTimesForGroup( int groupID );
	void CalculateBudgetGroupTimes_Recursive( CVProfNode *pNode );
	CUtlVector<vgui::Label *> m_GraphLabels;
	CBudgetHistoryPanel *m_pBudgetHistoryPanel;
	CBudgetBarGraphPanel *m_pBudgetBarGraphPanel;

	struct BudgetGroupTimeData_t
	{
		double m_Time[VPROF_HISTORY_COUNT];
	};
	CUtlVector<BudgetGroupTimeData_t> m_BudgetGroupTimes; // [m_CachedNumBudgetGroups][VPROF_HISTORY_COUNT]
	int m_CachedNumBudgetGroups;
	int m_BudgetHistoryOffset;
	bool m_bShowBudgetPanelHeld;
	vgui::HFont		m_hFont;
};

CBudgetPanel *GetBudgetPanel( void );

#endif // VGUI_BUDGETPANEL_H
