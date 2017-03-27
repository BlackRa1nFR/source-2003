/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLSectionLink.cpp
Owner:	russf@gipsysoft.com
Author: rich@woodbridgeinternalmed.com
Purpose:	Hyperlink 'link' object, links all of the hyperlink sections
					together. Ensure they all highlight at the same time if the
					hyperlink is made up of multiple sections etc.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLSectionLink.h"

void CHTMLSectionLink::OnMouseEnter()
{
	for (UINT i = 0; i < m_arrSections.GetSize(); ++i)
	{
		m_arrSections[i]->SetMouseInSection(true);
		m_arrSections[i]->ForceRedraw();
	}
}

void CHTMLSectionLink::OnMouseLeave()
{
	for (UINT i = 0; i < m_arrSections.GetSize(); ++i)
	{
		m_arrSections[i]->SetMouseInSection(false);
		m_arrSections[i]->ForceRedraw();
	}
}

