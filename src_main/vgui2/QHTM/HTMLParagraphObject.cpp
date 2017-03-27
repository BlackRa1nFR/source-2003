/*----------------------------------------------------------------------
Copyright (c) 1998 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.

File:	HTMLParagraphObject.cpp
Owner:	russf@gipsysoft.com
Purpose:	Simple paragraph container object.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "HTMLParse.h"


CHTMLParagraphObject::CHTMLParagraphObject( Type type )
	: m_pPara( NULL )
	, m_type( type )
	, m_pAnchor( NULL )
{
}

CHTMLParagraphObject::~CHTMLParagraphObject()
{
}

void CHTMLParagraphObject::SetParagraph( CHTMLParagraph *pPara )
{
	m_pPara = pPara;
}


CHTMLParagraphObject::Type CHTMLParagraphObject::GetType() const
{
	return m_type;
}