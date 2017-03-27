//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:  Interface of CHTMLFile.
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef HTML_H
#define HTML_H
#ifdef WIN32
#pragma once
#endif
#include <stdio.h>


//------------------------------------------------------------------------------------------------------
// Purpose:  CHTMLFile represents an HTML text file that is being created. It has
// some misc helper stuff, like writing the body tag for you, and linking to the style
// sheet.  Also some little helper functions to do <br>s and <p>s
//------------------------------------------------------------------------------------------------------
class CHTMLFile
{
public:
	static const bool printBody;
	static const bool dontPrintBody;
	static const bool linkStyle;
	static const bool dontLinkStyle;

private:
	FILE* out;
	char filename[100];
	bool fBody;
public:
	CHTMLFile():out(NULL),fBody(false){}

	CHTMLFile(const char*filenm ,const char* title,bool fPrintBody=true,const char* bgimage=NULL,int leftmarg=0,int topmarg=20);
	void open(const char*);

	void write(const char*,...);

	void hr(int len=0,bool alignleft=false);
	void br(){write("<br>\n");}
	void p(){write("<p>\n");};
	void img(const char* i){write("<img src=%s>\n",i);}

	void div(const char* cls){write("<div class=%s>\n",cls);}
	void div(){write("<div>\n");}
	void enddiv(){write("</div>");}
	
	void close();
	~CHTMLFile();
};
#endif // HTML_H
