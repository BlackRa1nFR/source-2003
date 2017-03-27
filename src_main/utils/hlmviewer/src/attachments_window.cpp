//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "attachments_window.h"
#include "ControlPanel.h"
#include "ViewerSettings.h"
#include "StudioModel.h"
#include "MatSysWin.h"


#define IDC_ATTACHMENT_LIST			(IDC_ATTACHMENT_WINDOW_FIRST+0)
#define IDC_ATTACHMENT_LIST_BONES	(IDC_ATTACHMENT_WINDOW_FIRST+1)
#define IDC_ATTACHMENT_TRANSLATION	(IDC_ATTACHMENT_WINDOW_FIRST+2)
#define IDC_ATTACHMENT_ROTATION		(IDC_ATTACHMENT_WINDOW_FIRST+3)
#define IDC_ATTACHMENT_QC_STRING	(IDC_ATTACHMENT_WINDOW_FIRST+4)


CAttachmentsWindow::CAttachmentsWindow( ControlPanel* pParent )	: mxWindow( pParent, 0, 0, 0, 0 )
{
	m_pControlPanel = pParent;
	g_viewerSettings.m_iEditAttachment = -1;
}


void CAttachmentsWindow::Init( )
{
	int left, top;
	left = 5;
	top = 0;

	// Attachment selection list
	new mxLabel( this, left + 3, top + 4, 60, 18, "Attachment" );
	m_cAttachmentList = new mxListBox( this, left, top + 20, 260, 100, IDC_ATTACHMENT_LIST );
	m_cAttachmentList->add ("None");
	m_cAttachmentList->select (0);
	mxToolTip::add (m_cAttachmentList, "Select an attachment to modify");

	left = 280;
	new mxLabel( this, left + 3, top + 4, 60, 18, "Attach To Bone" );
	m_cBoneList = new mxListBox( this, left, top + 20, 260, 100, IDC_ATTACHMENT_LIST_BONES );
	m_cBoneList->add ("None");
	m_cBoneList->select( 0 );
	mxToolTip::add( m_cBoneList, "Select a bone to attach to" );

	
	left = 5;
	top = 120;
	new mxLabel( this, left + 3, top + 4, 60, 18, "Translation" );
	m_cTranslation = new mxLineEdit2( this, left + 70, top, 90, 25, "10 20 30", IDC_ATTACHMENT_TRANSLATION );

	
	left = 170;
	top = 120;
	new mxLabel( this, left + 3, top + 4, 60, 18, "Rotation" );
	m_cRotation = new mxLineEdit2( this, left + 70, top, 90, 25, "0 90 180", IDC_ATTACHMENT_ROTATION );

	
	top = 145;
	left = 5;
	new mxLabel( this, left, top, 60, 18, "QC String" );
	m_cQCString = new mxLineEdit2( this, left + 70, top, 400, 25, "$attachment \"controlpanel0_ur\" \"Vgui\" -22 -15 4 rotate 0 0 0", IDC_ATTACHMENT_QC_STRING );
}


void CAttachmentsWindow::OnLoadModel()
{
	g_viewerSettings.m_iEditAttachment = -1;
	PopulateBoneList();
	PopulateAttachmentsList();

	g_viewerSettings.m_iEditAttachment = 0;
	UpdateStrings();
}


void CAttachmentsWindow::OnTabSelected()
{
	g_viewerSettings.m_iEditAttachment = m_cAttachmentList->getSelectedIndex();
}


void CAttachmentsWindow::OnTabUnselected()
{
	g_viewerSettings.m_iEditAttachment = -1;
}


void CAttachmentsWindow::PopulateAttachmentsList()
{
	m_cAttachmentList->removeAll();

	if ( g_pStudioModel )
	{
		studiohdr_t* pHdr = g_pStudioModel->getStudioHeader();
		if (pHdr->numattachments)
		{
			for ( int i = 0; i < pHdr->numattachments; i++ )
			{
				m_cAttachmentList->add ( pHdr->pAttachment(i)->pszName() );
			}

			m_cAttachmentList->select (0);
			OnSelChangeAttachmentList();
			return;
		}
	}

	m_cAttachmentList->add( "None" );
	m_cAttachmentList->select (0);
}


void CAttachmentsWindow::PopulateBoneList()
{
	m_cBoneList->removeAll();

	if ( g_pStudioModel )
	{
		studiohdr_t* pHdr = g_pStudioModel->getStudioHeader();
		if (pHdr->numbones)
		{
			for ( int i = 0; i < pHdr->numbones; i++ )
			{
				m_cBoneList->add ( pHdr->pBone(i)->pszName() );
			}

			m_cBoneList->select (0);
			return;
		}
	}

	m_cBoneList->add( "None" );
	m_cBoneList->select (0);
}


int CAttachmentsWindow::handleEvent (mxEvent *event)
{
	if ( !g_pStudioModel )
		return 0;

	studiohdr_t* pHdr = g_pStudioModel->getStudioHeader();
	switch( event->action )
	{
		case IDC_ATTACHMENT_LIST:
		{
			OnSelChangeAttachmentList();
		}
		break;

		case IDC_ATTACHMENT_LIST_BONES:
		{
			int iAttachment = g_viewerSettings.m_iEditAttachment;
			int iBone = m_cBoneList->getSelectedIndex();

			if ( iAttachment >= 0 && 
				iAttachment < pHdr->numattachments && 
				iBone >= 0 && 
				iBone < pHdr->numbones )
			{
				pHdr->pAttachment( iAttachment )->bone = iBone;
				UpdateStrings();
			} 
		}
		break;

		case IDC_ATTACHMENT_TRANSLATION:
		{
			int iAttachment = g_viewerSettings.m_iEditAttachment;

			if ( iAttachment >= 0 && 
				iAttachment < pHdr->numattachments )
			{
				mstudioattachment_t *pAttachment = pHdr->pAttachment( iAttachment );
				
				Vector vTrans( 0, 0, 0 );
				char curText[512];
				m_cTranslation->getText( curText, sizeof( curText ) );
				sscanf( curText, "%f %f %f", &vTrans.x, &vTrans.y, &vTrans.z );
				
				pAttachment->local[0][3] = vTrans.x;
				pAttachment->local[1][3] = vTrans.y;
				pAttachment->local[2][3] = vTrans.z;

				UpdateStrings( true, false, false );
			}
		}
		break;
	
		case IDC_ATTACHMENT_ROTATION:
		{
			int iAttachment = g_viewerSettings.m_iEditAttachment;

			if ( iAttachment >= 0 && 
				iAttachment < pHdr->numattachments )
			{
				mstudioattachment_t *pAttachment = pHdr->pAttachment( iAttachment );
				
				QAngle vRotation( 0, 0, 0 );
				char curText[512];
				m_cRotation->getText( curText, sizeof( curText ) );
				sscanf( curText, "%f %f %f", &vRotation.x, &vRotation.y, &vRotation.z );
				
				Vector vTrans = GetCurrentTranslation();
				AngleMatrix( vRotation, vTrans, pAttachment->local );
				
				UpdateStrings( true, false, false );
			}
		}
		break;
	
		default:
			return 0;
	}

	return 1;
}


void CAttachmentsWindow::OnSelChangeAttachmentList()
{
	int iAttachment = m_cAttachmentList->getSelectedIndex();
	if ( g_pStudioModel && iAttachment >= 0 && iAttachment < g_pStudioModel->getStudioHeader()->numattachments )
	{
		g_viewerSettings.m_iEditAttachment = iAttachment;

		// Init the bone list index.
		m_cBoneList->select( g_pStudioModel->getStudioHeader()->pAttachment( iAttachment )->bone );
	}
	else
	{
		g_viewerSettings.m_iEditAttachment = -1;
	}

	UpdateStrings();
}


Vector CAttachmentsWindow::GetCurrentTranslation()
{
	int iAttachment = m_cAttachmentList->getSelectedIndex();
	if ( g_pStudioModel && iAttachment >= 0 && iAttachment <= g_pStudioModel->getStudioHeader()->numattachments )
	{
		studiohdr_t *pHdr = g_pStudioModel->getStudioHeader();
		mstudioattachment_t *pAttachment = pHdr->pAttachment( iAttachment );

		return Vector( pAttachment->local[0][3],
			pAttachment->local[1][3],
			pAttachment->local[2][3] );
	}
	else
	{
		return Vector( 0, 0, 0 );
	}
}


Vector CAttachmentsWindow::GetCurrentRotation()
{
	int iAttachment = m_cAttachmentList->getSelectedIndex();
	if ( g_pStudioModel && iAttachment >= 0 && iAttachment <= g_pStudioModel->getStudioHeader()->numattachments )
	{
		studiohdr_t *pHdr = g_pStudioModel->getStudioHeader();
		mstudioattachment_t *pAttachment = pHdr->pAttachment( iAttachment );

		float angles[3];
		MatrixAngles( pAttachment->local, angles );
		return Vector( angles[0], angles[1], angles[2] );
	}
	else
	{
		return Vector( 0, 0, 0 );
	}
}


void CAttachmentsWindow::UpdateStrings( bool bUpdateQC, bool bUpdateTranslation, bool bUpdateRotation )
{
	char str[1024];

	int iAttachment = -1;
	studiohdr_t* pHdr = NULL;
	if ( g_pStudioModel )
	{
		pHdr = g_pStudioModel->getStudioHeader();
		iAttachment = m_cAttachmentList->getSelectedIndex();
		if ( iAttachment < 0 || iAttachment >= pHdr->numattachments )
			iAttachment = -1;
	}

	if ( iAttachment == -1 )
	{
		m_cTranslation->setText( "(none)" );
		m_cRotation->setText( "(none)" );
		m_cQCString->setText( "(none)" );
	}
	else
	{
		mstudioattachment_t *pAttachment = pHdr->pAttachment( iAttachment );

		Vector vTranslation = GetCurrentTranslation();
		Vector vRotation = GetCurrentRotation();

		if ( bUpdateQC )
		{
			sprintf( str, "$attachment \"%s\" \"%s\" %.2f %.2f %.2f rotate %.0f %.0f %.0f", 
				pAttachment->pszName(),
				pHdr->pBone( pAttachment->bone )->pszName(),
				VectorExpand( vTranslation ),
				VectorExpand( vRotation ) );

			m_cQCString->setText( str );
		}

		if ( bUpdateTranslation )
		{
			sprintf( str, "%.2f %.2f %.2f", VectorExpand( vTranslation ) );
			m_cTranslation->setText( str );
		}

		if ( bUpdateRotation )
		{
			sprintf( str, "%.0f %.0f %.0f", VectorExpand( vRotation ) );
			m_cRotation->setText( str );
		}
	}
}


