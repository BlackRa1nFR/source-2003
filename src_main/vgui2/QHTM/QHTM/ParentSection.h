/*----------------------------------------------------------------------
Copyright (c) 1998,1999 Gipsysoft. All Rights Reserved.
Please see the file "licence.txt" for licencing details.
File:	ParentSection.h
Owner:	russf@gipsysoft.com
Purpose:	<Description of module>.
----------------------------------------------------------------------*/
#ifndef PARENTSECTION_H
#define PARENTSECTION_H

#ifndef SECTIONABC_H
	#include "SectionABC.h"
#endif	//	SECTIONABC_H

class CParentSection: public CSectionABC
{
public:
	CParentSection( CParentSection *pParent );
	virtual ~CParentSection();

	//	handle a draw event.
	virtual void OnDraw( CDrawContext &dc );

	//	Remove a section from the list of sections
	void RemoveAllSections();


	//	Add a section to the list of sections this section contains.
	void AddSection( CSectionABC *pSect );

	//	Get the number of children the section has
	inline UINT GetSectionCount() const { return m_arrSections.GetSize(); }

	//	Get at a section by index
	inline CSectionABC *GetSectionAt( UINT nIndex ) const { return m_arrSections[ nIndex ]; }

	//	Tell our children about our transparent state
	virtual void Transparent( bool bTransparent );
	virtual CSectionABC * FindSectionFromPoint( const CPoint &pt ) const;

protected:
	typedef Container::CArray< CSectionABC *> CSectionList; 

	CSectionList m_arrSections;

private:


private:
	CParentSection( const CParentSection & );
	CParentSection& operator =( const CParentSection & );
};

#endif //PARENTSECTION_H