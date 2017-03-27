//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef GDVAR_H
#define GDVAR_H
#pragma once

#include <afxtempl.h>
#include <TokenReader.h> // dvs: for MAX_STRING. Fix.


class MDkeyvalue;


typedef enum
{
	ivBadType = -1,
	ivAngle,
	ivTargetDest,
	ivTargetSrc,
	ivInteger,
	ivString,
	ivChoices,
	ivFlags,
	ivDecal,
	ivColor255,		// components are 0-255
	ivColor1,		// components are 0-1
	ivStudioModel,
	ivSprite,
	ivSound,
	ivVector,
	ivNPCClass,
	ivFilterClass,
	ivFloat,
	ivMaterial,
	ivScene,
	ivSide,			// One brush face ID.
	ivSideList,		// One or more brush face IDs, space delimited.
	ivOrigin,		// The origin of an entity, in the form "x y z".
	ivVecLine,		// An origin that draws a line back to the parent entity.
	ivAxis,			// The axis of rotation for a rotating entity, in the form "x0 y0 z0, x1 y1 z1".
} GDIV_TYPE;


//-----------------------------------------------------------------------------
// Defines an element in a choices/flags list. Choices values are strings;
// flags values are integers, hence the iValue and szValue members.
//-----------------------------------------------------------------------------
typedef struct
{
	int iValue;					// Bitflag value for ivFlags
	char szValue[MAX_STRING];	// String value for ivChoices
	char szCaption[MAX_STRING];	// Name of this choice
	BOOL bDefault;				// Flag set by default?
} GDIVITEM;


class GDinputvariable
{
	public:

		~GDinputvariable();
		GDinputvariable();

		BOOL InitFromTokens(TokenReader& tr);
		
		// functions:
		inline const char *GetName() { return m_szName; }
		inline const char *GetLongName(void) { return m_szLongName; }
		inline const char *GetDescription(void);

		inline int GetFlagCount() { return m_Items.GetSize(); }
		inline int GetChoiceCount() { return m_Items.GetSize(); }

		inline GDIV_TYPE GetType() { return m_eType; }
		const char *GetTypeText(void);
		
		inline void GetDefault(int *pnStore)
		{ 
			pnStore[0] = m_nDefault; 
		}

		inline void GetDefault(char *pszStore)
		{ 
			strcpy(pszStore, m_szDefault); 
		}
		
		GDIV_TYPE GetTypeFromToken(LPCSTR pszToken);
		trtoken_t GetStoreAsFromType(GDIV_TYPE eType);

		const char *ItemStringForValue(const char *szValue);
		const char *ItemValueForString(const char *szString);

		BOOL IsFlagSet(UINT);
		void SetFlag(UINT, BOOL bSet);

		void ResetDefaults();

		void ToKeyValue(MDkeyvalue* pkv);
		void FromKeyValue(MDkeyvalue* pkv);

		inline bool IsReportable(void);
		inline bool IsReadOnly(void);

		GDinputvariable &operator =(GDinputvariable &Other);
		void Merge(GDinputvariable &Other);

		// for choices/flags:
		CArray<GDIVITEM,GDIVITEM&> m_Items;
		int m_nItems;

	private:

		static char *m_pszEmpty;

		char m_szName[MAX_IDENT];
		char m_szLongName[MAX_STRING];
		char *m_pszDescription;

		GDIV_TYPE m_eType;

		int m_nDefault;
		char m_szDefault[MAX_STRING];

		int m_nValue;
		char m_szValue[MAX_STRING];

		bool m_bReportable;
		bool m_bReadOnly;

		friend class GDclass;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GDinputvariable::GetDescription(void)
{
	if (m_pszDescription != NULL)
	{	
		return(m_pszDescription);
	}

	return(m_pszEmpty);
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not this variable is read only. Read only variables
//			cannot be edited in the Entity Properties dialog.
//-----------------------------------------------------------------------------
bool GDinputvariable::IsReadOnly(void)
{
	return(m_bReadOnly);
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not this variable should be displayed in the Entity
//			Report dialog.
//-----------------------------------------------------------------------------
bool GDinputvariable::IsReportable(void)
{
	return(m_bReportable);
}

#endif // GDVAR_H
