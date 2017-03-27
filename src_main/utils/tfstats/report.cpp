//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Implematation of CReport
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "report.h"
#include "util.h"

//------------------------------------------------------------------------------------------------------
// Function:	CReport::makeHTMLPage
// Purpose:	makes a whole page out of the element that it is called on
// Input:	pageName - the name of the html file
//				pageTitle - the title of the document
//------------------------------------------------------------------------------------------------------
void CReport::makeHTMLPage(char* pageName,char* pageTitle)
{
	CHTMLFile Page(pageName,pageTitle);
	report(Page);
}
//------------------------------------------------------------------------------------------------------
// Function:	CReport::report
// Purpose:	generates the report's output and adds it to anHTML file
// Input:	html - the HTML file to add this report element to
//------------------------------------------------------------------------------------------------------
void CReport::report(CHTMLFile& html)
{
	generate();
	writeHTML(html);
}
