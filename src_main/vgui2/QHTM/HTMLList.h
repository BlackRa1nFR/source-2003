/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	HTMLList.h
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#ifndef HTMLLIST_H
#define HTMLLIST_H

#ifndef HTMLPARAGRAPHOBJECT_H
	#include "HTMLParagraphObject.h"
#endif	//	HTMLPARAGRAPHOBJECT_H

class CHTMLListItem;
class CHTMLList : public CHTMLParagraphObject				//	List
//
//	List, ordered and unordered.
//	Can have many items
{
public:
	CHTMLList(	bool bOrdered );

	~CHTMLList();

	//	Add a item to the list
	void AddItem( CHTMLListItem *pItem );

	ArrayClass< CHTMLListItem * > m_arrItems;

	bool m_bOrdered;

#ifdef _DEBUG
	virtual void Dump() const;
#endif	//	_DEBUG

private:
	CHTMLList( const CHTMLList &);
	CHTMLList& operator =( const CHTMLList &);
};



#endif //HTMLLIST_H