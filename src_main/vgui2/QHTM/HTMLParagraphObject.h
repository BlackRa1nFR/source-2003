/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLParagraphObject.h
Owner:	russf@gipsysoft.com
Purpose:	Paragraph object!
----------------------------------------------------------------------*/
#ifndef HTMLPARAGRAPHOBJECT_H
#define HTMLPARAGRAPHOBJECT_H

class CHTMLAnchor;
class CHTMLParagraph;

class CHTMLParagraphObject				//	obj
{
public:
	virtual ~CHTMLParagraphObject();
	enum Type { knText, knImage, knHorizontalRule, knAnchor, knTable, knList, knBlockQuote };
	Type GetType() const;

	CHTMLAnchor*	m_pAnchor;

#ifdef _DEBUG
	virtual void Dump() const = 0;
#endif	//	_DEBUG

protected:

	CHTMLParagraphObject( Type type );

	void SetParagraph( CHTMLParagraph *pPara );

private:
	CHTMLParagraph *m_pPara;
	Type m_type;

private:
	CHTMLParagraphObject( const CHTMLParagraphObject &);
	CHTMLParagraphObject& operator =( const CHTMLParagraphObject &);
	friend class CHTMLParagraph;
};


#endif //HTMLPARAGRAPHOBJECT_H