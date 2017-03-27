//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements the Undo/Redo system.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "History.h"
#include "Worldcraft.h"
#include "Options.h"
#include "MainFrm.h"
#include "MapDoc.h"
#include "ObjectProperties.h"
#include "GlobalFunctions.h"
#include "UndoWarningDlg.h"


static CHistory *pCurHistory;	// The Undo/Redo history associated with the active doc.
static CHistory FakeHistory;	// Used when there is no active doc. Always paused.


//-----------------------------------------------------------------------------
// Purpose: Returns the current active Undo/Redo history.
//-----------------------------------------------------------------------------
CHistory *GetHistory(void)
{
	if (!pCurHistory)
	{
		return(&FakeHistory);
	}

	return(pCurHistory);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CHistory::CHistory(void)
{
	static BOOL bFirst = TRUE;	// fake history is always first
	Opposite = NULL;
	CurTrack = NULL;
	bPaused = bFirst ? 2 : FALSE;	// if 2, never unpaused
	bFirst = FALSE;
	m_bActive = TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CHistory::~CHistory()
{
	POSITION p = Tracks.GetHeadPosition();

	while(p)
	{
		CHistoryTrack *pTrack = Tracks.GetNext(p);
		delete pTrack;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bUndo - 
//			pOpposite - 
//-----------------------------------------------------------------------------
void CHistory::SetOpposite(BOOL bUndo, CHistory *pOpposite)
{
	this->bUndo = bUndo;
	Opposite = pOpposite;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CHistory::IsUndoable()
{
	// return status flag depending on the current track
	return (CurTrack && m_bActive) ? TRUE : FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bActive - 
//-----------------------------------------------------------------------------
void CHistory::SetActive(BOOL bActive)
{
	m_bActive = bActive;
	if (!m_bActive)
	{
		// kill all tracks right now
		POSITION p = Tracks.GetHeadPosition();

		while(p)
		{
			CHistoryTrack *pTrack = Tracks.GetNext(p);
			delete pTrack;
		}

		Tracks.RemoveAll();
		MarkUndoPosition();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Actually, this implements both Undo and Redo, because a Redo is just
//			an Undo in the opposite history track. 
// Input  : pNewSelection - List to populate with the new selection set after the Undo.
//-----------------------------------------------------------------------------
void CHistory::Undo(CMapObjectList *pNewSelection)
{
	Opposite->MarkUndoPosition(&CurTrack->Selected, GetCurTrackName(), TRUE);

	//
	// Track entries are consumed LIFO.
	//
	POSITION p = Tracks.GetTailPosition();
	Tracks.RemoveAt(p);

	//
	// Perform the undo.
	//
	Pause();
	CurTrack->Undo();
	Resume();

	//
	// Get the objects that should be selected from the track entry.
	// 
	pNewSelection->RemoveAll();
	pNewSelection->AddTail(&CurTrack->Selected);

	//
	// Done with this track entry. This track entry will be recreated by the
	// opposite history track if necessary.
	//
	uDataSize -= CurTrack->uDataSize;
	delete CurTrack;

	//
	// Move to the previous track entry.
	//
	p = Tracks.GetTailPosition();
	if (p)
	{
		CurTrack = Tracks.GetAt(p);
	}
	else
	{
		CurTrack = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSelection - 
//			pszName - 
//			bFromOpposite - 
//-----------------------------------------------------------------------------
void CHistory::MarkUndoPosition(CMapObjectList *pSelection, LPCTSTR pszName, BOOL bFromOpposite)
{
	if(Opposite && bUndo && !bFromOpposite)
	{
		// this is the undo tracker and the call is NOT from the redo
		// tracker. kill the redo tracker's history.
		POSITION p = Opposite->Tracks.GetHeadPosition();
		while(p)
		{
			CHistoryTrack *pTrack = Opposite->Tracks.GetNext(p);
			pTrack->m_bAutoDestruct = true;
			delete pTrack;

		}

		Opposite->Tracks.RemoveAll();
		Opposite->CurTrack = NULL;
	}

	if(CurTrack)
	{
		MEMORYSTATUS ms;
		GlobalMemoryStatus(&ms);
		uDataSize += CurTrack->uDataSize;
		BOOL bWarnMemory = AfxGetApp()->GetProfileInt("General", 
			"Undo Memory Warning", TRUE);
		if(ms.dwMemoryLoad > 80 && bWarnMemory)
		{
			// inform user that undo is taking up lots of
			// memory..
			CUndoWarningDlg dlg;
			dlg.DoModal();
			if(dlg.m_bNoShow)
			{
				AfxGetApp()->WriteProfileInt("General", 
					"Undo Memory Warning", FALSE);
			}
		}
	}

	// create a new track
	CurTrack = new CHistoryTrack(this, pSelection);
	Tracks.AddTail(CurTrack);
	CurTrack->SetName(pszName);

	// check # of undo levels ..
	if(Tracks.GetCount() > Options.general.iUndoLevels)
	{
		// remove some.
		int i, i2;
		i = i2 = Tracks.GetCount() - Options.general.iUndoLevels;
		POSITION p = Tracks.GetHeadPosition();
		while(i--)
		{
			CHistoryTrack *pTrack = Tracks.GetNext(p);
			if(pTrack == CurTrack)
			{
				i2 -= (i2 - i);
				break;	// safeguard
			}
			delete pTrack;

		}
		// delete them from the list now
		while(i2--)
		{
			p = Tracks.GetHeadPosition();
			Tracks.RemoveAt(p);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Keeps an object, so changes to it can be undone.
// Input  : pObject - Object to keep.
//-----------------------------------------------------------------------------
void CHistory::Keep(CMapClass *pObject)
{
	if (CurTrack == NULL)
	{
		MarkUndoPosition();
	}

	CurTrack->Keep(pObject);
	
	//
	// Keep this object's children.
	//
	EnumChildrenPos_t pos;
	CMapClass *pChild = pObject->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		CurTrack->Keep(pChild);
		pChild = pObject->GetNextDescendent(pos);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Keeps a list of objects, so changes to them can be undone.
// Input  : pList - List of objects to keep.
//-----------------------------------------------------------------------------
void CHistory::Keep(CMapObjectList *pList)
{
	POSITION pos = pList->GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = pList->GetNext(pos);
		Keep(pObject);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//-----------------------------------------------------------------------------
void CHistory::KeepForDestruction(CMapClass *pObject)
{
	if (CurTrack == NULL)
	{
		MarkUndoPosition();
	}

	CurTrack->KeepForDestruction(pObject);
}


//-----------------------------------------------------------------------------
// Purpose: Keeps a new object, so it can be deleted on an undo.
// Input  : pObject - Object to keep.
//-----------------------------------------------------------------------------
void CHistory::KeepNew(CMapClass *pObject, bool bKeepChildren)
{
	if (CurTrack == NULL)
	{
		MarkUndoPosition();
	}

	//
	// Keep this object's children.
	//
	if (bKeepChildren)
	{
		EnumChildrenPos_t pos;
		CMapClass *pChild = pObject->GetFirstDescendent(pos);
		while (pChild != NULL)
		{
			CurTrack->KeepNew(pChild);
			pChild = pObject->GetNextDescendent(pos);
		}
	}

	CurTrack->KeepNew(pObject);
}


//-----------------------------------------------------------------------------
// Purpose: Keeps a list of new objects, so changes to them can be undone.
// Input  : pList - List of objects to keep.
//-----------------------------------------------------------------------------
void CHistory::KeepNew(CMapObjectList *pList, bool bKeepChildren)
{
	POSITION pos = pList->GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = pList->GetNext(pos);
		KeepNew(pObject, bKeepChildren);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the given history object as the one to use for all Undo operations.
//-----------------------------------------------------------------------------
void CHistory::SetHistory(class CHistory *pHistory)
{
	pCurHistory = pHistory;
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTrackEntry::CTrackEntry()
{
	m_bAutoDestruct = true;
	m_nDataSize = 0;
	m_eType = ttNone;
	m_bUndone = false;
}


//-----------------------------------------------------------------------------
// Purpose: Constructs a track entry from a list of parameters.
// Input  : t - 
//-----------------------------------------------------------------------------
CTrackEntry::CTrackEntry(TrackType_t eType, ...)
{
	m_bAutoDestruct = false;
	m_eType = eType;
	m_bUndone = false;

	va_list vl;
	va_start(vl, eType);

	switch (m_eType)
	{
		//
		// Keep track of an object that was modified by the user. An Undo will cause this
		// object to revert to its original state.
		//
		case ttCopy:
		{
			m_Copy.pCurrent = va_arg(vl, CMapClass *);
			m_Copy.pKeptObject = m_Copy.pCurrent->Copy(false);
			m_nDataSize = sizeof(*this) + m_Copy.pKeptObject->GetSize();
			break;
		}
		
		//
		// Keep track of an object that was created by the user. An Undo will cause this
		// object to be removed from the world.
		//
		case ttCreate:
		{
			m_Create.pCreated = va_arg(vl, CMapClass *);
			ASSERT(m_Create.pCreated != NULL);
			ASSERT(m_Create.pCreated->Parent != NULL);
			m_nDataSize = sizeof(*this);
			break;
		}

		//
		// Keep track of an object that was deleted by the user. An Undo will cause this
		// object to be added back into the world.
		//
		case ttDelete:
		{
			m_Delete.pDeleted = va_arg(vl, CMapClass *);
			m_Delete.pKeptParent = m_Delete.pDeleted->GetParent();
			m_nDataSize = sizeof(*this);
			break;
		}
	}

	va_end(vl);
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Called when history events are removed from the Undo
//			history. The goal here is to clean up any copies of objects that
//			were kept in the history.
//
//			Once a track entry object is destroyed, the user event that it
//			tracks can no longer be undone or redone.
//-----------------------------------------------------------------------------
CTrackEntry::~CTrackEntry()
{
	if (!m_bAutoDestruct || m_eType == ttNone)
	{
		return;
	}

	switch (m_eType)
	{
		//
		// We kept a copy of an object. Delete our copy of the object.
		//
		case ttCopy:
		{
			if (!m_bUndone)
			{
				delete m_Copy.pKeptObject;
			}

			break;
		}

		//
		// We kept track of an object's creation. Nothing to delete here. The object is in the world.
		//
		case ttCreate:
		{
			break;
		}

		//
		// We kept a pointer to an object that was deleted from the world. We need to delete the object,
		// because the object's deletion can no longer be undone.
		//
		case ttDelete:
		{
			//
			// If this entry was undone, the object has been added back into the world, so we
			// should not delete the object.
			//
			if (!m_bUndone)
			{
				delete m_Delete.pDeleted;
			}
			break;
		}

		default:
		{
			ASSERT(0);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Performs the undo by restoring the kept object to its original state.
// Input  : Opposite - Pointer to the opposite history track. If we are in the
//				undo history, it points to the redo history, and vice-versa.
//-----------------------------------------------------------------------------
void CTrackEntry::Undo(CHistory *Opposite)
{
	switch (m_eType)
	{
		//
		// We are undoing a change to an object. Restore it to its original state.
		//
		case ttCopy:
		{
			Opposite->Keep(m_Copy.pCurrent);

			CMapClass *pCurParent = m_Copy.pCurrent->GetParent();

			//
			// Copying back into the world, so update object dependencies.
			//
			m_Copy.pCurrent->CopyFrom(m_Copy.pKeptObject, true);

			//
			// Delete the copy of the kept object.
			//
			delete m_Copy.pKeptObject;
			m_Copy.pKeptObject = NULL;
			break;
		}

		//
		// We are undoing the deletion of an object. Add it to the world.
		//
		case ttDelete:
		{
			//
			// First restore the deleted object's parent so that it is properly kept in the
			// opposite history track. The opposite history track sees this as a new object
			// being created.
			//
			m_Delete.pDeleted->Parent = m_Delete.pKeptParent;
			Opposite->KeepNew(m_Delete.pDeleted);

			//
			// Put the object back in the world.
			//
			Opposite->GetDocument()->AddObjectToWorld(m_Delete.pDeleted, m_Delete.pKeptParent);
			break;
		}

		//
		// We are undoing the creation of an object. Remove it from the world.
		//
		case ttCreate:
		{
			//
			// Create a symmetrical track event in the other history track.
			//
			Opposite->KeepForDestruction(m_Create.pCreated);

			//
			// Remove the object from the world, but not its children. If its children
			// were new to the world they were kept seperately.
			//
			Opposite->GetDocument()->RemoveObjectFromWorld(m_Create.pCreated, false);
			m_Create.pCreated = NULL; // dvs: why do we do this?
			break;
		}
	}

	m_bUndone = true;
}


//-----------------------------------------------------------------------------
// Purpose: Notifies the object that it has been undone/redone. Called after all
//			undo entries have been handled so that objects are dealing with the
//			correct data set when they calculate bounds, etc.
//-----------------------------------------------------------------------------
void CTrackEntry::DispatchUndoNotify(void)
{
	switch (m_eType)
	{
		//
		// We are undoing a change to an object. Restore it to its original state.
		//
		case ttCopy:
		{
			m_Copy.pCurrent->OnUndoRedo();
			m_Copy.pCurrent->NotifyDependents(Notify_Changed);
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParent - 
//			*pSelected - 
// Output : 
//-----------------------------------------------------------------------------
CHistoryTrack::CHistoryTrack(CHistory *pParent, CMapObjectList *pSelected)
{
	Parent = pParent;

	Data.SetSize(0, 16);
	nData = 0;
	uDataSize = 0;

	static dwTrackerID = 1;	// objects start at 0, so we don't want to
	dwID = dwTrackerID ++;
	
	// add to local list of selected objects at time of creation
	if (pSelected)
	{
		Selected.AddTail(pSelected);
	}

	m_bAutoDestruct = true;
	szName[0] = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Called when this track's document is being deleted.
//			Marks all entries in this track for autodestruction, so that when
//			their destructor gets called, they free any object pointers that they
//			hold.
//-----------------------------------------------------------------------------
CHistoryTrack::~CHistoryTrack()
{
	for (int i = 0; i < nData; i++)
	{
		Data[i].m_bAutoDestruct = m_bAutoDestruct;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//			iFlag - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CHistoryTrack::CheckObjectFlag(CMapClass *pObject, int iFlag)
{
	// check for saved copy already..
	if(pObject->Kept.ID != dwID)
	{
		// no id.. make sure types is flag only
		pObject->Kept.ID = dwID;
		pObject->Kept.Types = iFlag;
	}
	else if(!(pObject->Kept.Types & iFlag))
	{
		// if we've already stored that this is a new object in this
		//  track, there is no point in storing a copy since UNDOing
		//  this track will delete the object.
		if(iFlag == CTrackEntry::ttCopy && 
			(pObject->Kept.Types & CTrackEntry::ttCreate))
		{
			return TRUE;
		}

		// id, but no copy flag.. make sure types has flag set
		pObject->Kept.Types |= iFlag;
	}
	else
	{
		// both here.. we have a copy
		return TRUE;
	}

	return FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//-----------------------------------------------------------------------------
void CHistoryTrack::Keep(CMapClass *pObject)
{
	if(Parent->IsPaused() || pObject->IsTemporary())
		return;

	// make a copy of this object so we can undo changes to it

	if(CheckObjectFlag(pObject, CTrackEntry::ttCopy))
		return;

	Parent->Pause();
	CTrackEntry te(CTrackEntry::ttCopy, pObject);
	Data.Add(te);
	te.m_bAutoDestruct = false;
	++nData;
	uDataSize += te.GetSize();
	Parent->Resume();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//-----------------------------------------------------------------------------
void CHistoryTrack::KeepForDestruction(CMapClass *pObject)
{
	if(Parent->IsPaused() || pObject->IsTemporary())
		return;

	// check for saved destruction already..
	if(CheckObjectFlag(pObject, CTrackEntry::ttDelete))
		return;

	GetMainWnd()->pObjectProperties->DontEdit(pObject);

	Parent->Pause();
	CTrackEntry te(CTrackEntry::ttDelete, pObject);
	Data.Add(te);
	++nData;
	te.m_bAutoDestruct = false;
	uDataSize += te.GetSize();
	Parent->Resume();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//-----------------------------------------------------------------------------
void CHistoryTrack::KeepNew(CMapClass *pObject)
{
	if(Parent->IsPaused() || pObject->IsTemporary())
		return;

	// check for saved creation already..
	VERIFY(!CheckObjectFlag(pObject, CTrackEntry::ttCreate));

	Parent->Pause();
	CTrackEntry te(CTrackEntry::ttCreate, pObject);
	Data.Add(te);
	++nData;
	te.m_bAutoDestruct = false;
	uDataSize += te.GetSize();
	Parent->Resume();
}


//-----------------------------------------------------------------------------
// Purpose: Undoes all the track entries in this track.
//-----------------------------------------------------------------------------
void CHistoryTrack::Undo()
{
	for (int i = nData - 1; i >= 0; i--)
	{
		Data[i].Undo(Parent->Opposite);
	}

	//
	// Do notification separately so that objects are dealing with the
	// correct data set when they calculate bounds, etc.
	//
	for (i = nData - 1; i >= 0; i--)
	{
		Data[i].DispatchUndoNotify();
	}
}
