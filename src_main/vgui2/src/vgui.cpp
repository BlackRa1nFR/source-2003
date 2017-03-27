//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Core implementation of vgui
//
// $NoKeywords: $
//=============================================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define PROTECTED_THINGS_DISABLE

#include <vgui/VGUI.h>
#include <vgui/Dar.h>
#include <vgui/IInputInternal.h>
#include <vgui/IPanel.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IClientPanel.h>
#include <vgui/IScheme.h>
#include <KeyValues.h>

#include <string.h>
#include <Assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <tier0/dbg.h>

#include "vgui_internal.h"
#include "VPanel.h"
#include "IMessageListener.h"

#include "UtlLinkedList.h"
#include "UtlPriorityQueue.h"
#include "UtlVector.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


using namespace vgui;
static const int WARN_PANEL_NUMBER = 4096; // in DEBUG if more panels than this are created then throw an Assert, helps catch panel leaks
static const int INITIAL_PANEL_NUMBER = 1024; // seed the linked list with this number of entries to start with
static const int GROW_PANEL_NUMBER = 1024; // tell the linked list to grow this number of entries each time

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Used to reference panels in a safe manner
//-----------------------------------------------------------------------------
struct SerialPanel_t
{
	unsigned short m_iSerialNumber;
	VPANEL m_pPanel;
};
}

//-----------------------------------------------------------------------------
// Purpose: Single item in the message queue
//-----------------------------------------------------------------------------
struct MessageItem_t
{
	KeyValues *_params; // message data
						// _params->GetName() is the message name

	HPanel _messageTo;	// the panel this message is to be sent to
	HPanel _from;		// the panel this message is from (if any)
	float _arrivalTime;	// time at which the message should be passed on to the recipient

	int _messageID;		// incrementing message index
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool PriorityQueueComp(const MessageItem_t& x, const MessageItem_t& y) 
{
	if (x._arrivalTime > y._arrivalTime)
	{
		return true;
	}
	else if (x._arrivalTime < y._arrivalTime)
	{
		return false;
	}

	// compare messageID's to ensure we have the messages in the correct order
	return (x._messageID > y._messageID);
}



//-----------------------------------------------------------------------------
// Purpose: Implementation of core vgui functionality
//-----------------------------------------------------------------------------
class CVGui : public IVGui
{
public:
	CVGui();
	~CVGui();

	virtual bool Init( CreateInterfaceFn *factoryList, int numFactories );

//-----------------------------------------------------------------------------
	// SRC specific stuff
	// Here's where the app systems get to learn about each other 
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void *QueryInterface( const char *pInterfaceName );

	// Init, shutdown
	virtual InitReturnVal_t Init();
	virtual void Shutdown();
	// End of specific interface
//-----------------------------------------------------------------------------


	virtual void RunFrame();

	virtual void Start()
	{
		m_bRunning = true;
	}

	// signals vgui to Stop running
	virtual void Stop()
	{
		m_bRunning = false;
	}

	// returns true if vgui is current active
	virtual bool IsRunning()
	{
		return m_bRunning;
	}

	virtual void ShutdownMessage(unsigned int shutdownID);

	// safe-pointer handle methods
	virtual VPANEL AllocPanel();
	virtual void FreePanel(VPANEL ipanel);
	virtual HPanel PanelToHandle(VPANEL panel);
	virtual VPANEL HandleToPanel(HPanel index);
	virtual void MarkPanelForDeletion(VPANEL panel);

	virtual void AddTickSignal(VPANEL panel, int intervalMilliseconds = 0);
	virtual void RemoveTickSignal(VPANEL panel );


	// message pump method
	virtual void PostMessage(VPANEL target, KeyValues *params, VPANEL from, float delaySeconds = 0.0f);

	virtual void SetSleep( bool state ) { m_bDoSleep = state; };

	virtual void DPrintf(const char *format, ...);
	virtual void DPrintf2(const char *format, ...);


	// Creates/ destroys vgui contexts, which contains information
	// about which controls have mouse + key focus, for example.
	virtual HContext CreateContext();
	virtual void DestroyContext( HContext context ); 

	// Associates a particular panel with a vgui context
	// Associating NULL is valid; it disconnects the panel from the context
	virtual void AssociatePanelWithContext( HContext context, VPANEL pRoot );

	// Activates a particular input context, use DEFAULT_VGUI_CONTEXT
	// to get the one normally used by VGUI
	virtual void ActivateContext( HContext context );

private:
	// VGUI contexts
	struct Context_t
	{
		HInputContext m_hInputContext;
	};

	struct Tick_t
	{
		VPANEL	panel;
		int		interval;
		int		nexttick;

		// Debugging
		char	panelname[ 64 ];
	};

	// Returns the current context
	Context_t *GetContext( HContext context );

	void PanelCreated(VPanel *panel);
	void PanelDeleted(VPanel *panel);
	bool DispatchMessages();
	void DestroyAllContexts( );


	bool m_bRunning;

	// safe panel handle stuff
	CUtlLinkedList<SerialPanel_t, unsigned short> m_PanelList;
	unsigned short m_iNextSerialNumber;
	int m_iPanelsAlloced;
	bool m_bInitialized;
	int m_iCurrentMessageID;
	bool m_bDoSleep;

	CUtlVector< Tick_t * > m_TickSignalVec;
	CUtlLinkedList< Context_t >	m_Contexts;

	bool m_InDispatcher;
	HContext m_hContext;
	Context_t m_DefaultContext;

#ifdef DEBUG
	int m_iDeleteCount, m_iDeletePanelCount;
#endif

	// message queue. holds all vgui messages generated by windows events
	CUtlLinkedList<MessageItem_t, short> m_MessageQueue;

	// secondary message queue, holds all vgui messages generated by vgui
	CUtlLinkedList<MessageItem_t, short> m_SecondaryQueue;

	// timing queue, holds all the messages that have to arrive at a specified time
	CUtlPriorityQueue<MessageItem_t> m_DelayedMessageQueue;
};

CVGui g_VGui;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CVGui, IVGui, VGUI_IVGUI_INTERFACE_VERSION, g_VGui);

namespace vgui
{

vgui::IVGui *ivgui()
{
	return &g_VGui;
}

}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVGui::CVGui() : m_DelayedMessageQueue(0, 4, PriorityQueueComp), m_PanelList(GROW_PANEL_NUMBER, INITIAL_PANEL_NUMBER) 
{
	m_iPanelsAlloced = 0;
	m_bInitialized = false;
	m_bDoSleep = true;
	m_hContext = DEFAULT_VGUI_CONTEXT;
	m_iNextSerialNumber = 0;
	m_DefaultContext.m_hInputContext = DEFAULT_INPUT_CONTEXT;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CVGui::~CVGui()
{
	// print out debug results
	if (m_iPanelsAlloced > 0)
	{
		ivgui()->DPrintf2("Memory leak: panels left in memory: %d\n", m_iPanelsAlloced);
	}

	if ( m_PanelList.Count() > 0 )
	{
		ivgui()->DPrintf2("Memory leak: panels left in Panel List: %d\n", m_PanelList.Count());
	}
/*
	// even though we would like to iterate the panels and display their names here, we can't
	// since the Client() pointer will be invalid since the clients dll has already been unloaded
	for (int i = 0; i < MAX_SAFE_PANELS; i++)
	{
		if (m_PanelList[i].m_pPanel)
		{
			ivgui()->DPrintf2("%s - %s\n", m_PanelList[i].m_pPanel->Client()->GetName(), m_PanelList[i].m_pPanel->Client()->GetClassName());
		}
	}
*/
}

//-----------------------------------------------------------------------------
// Purpose: initializes internal vgui interfaces
//-----------------------------------------------------------------------------
bool CVGui::Init(CreateInterfaceFn *factoryList, int numFactories)
{
	if (!m_bInitialized)
	{
		m_bInitialized = true;
		return VGui_InternalLoadInterfaces( factoryList, numFactories );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Creates/ destroys "input" contexts, which contains information
// about which controls have mouse + key focus, for example.
//-----------------------------------------------------------------------------
HContext CVGui::CreateContext()
{
	HContext i = m_Contexts.AddToTail();
	m_Contexts[i].m_hInputContext = input()->CreateInputContext();
	return i;
}

void CVGui::DestroyContext( HContext context )
{
	Assert( context != DEFAULT_VGUI_CONTEXT );

	if ( m_hContext == context )
	{
		ActivateContext( DEFAULT_VGUI_CONTEXT );
	}

	input()->DestroyInputContext( GetContext(context)->m_hInputContext );
	m_Contexts.Remove(context);
}

void CVGui::DestroyAllContexts( )
{
	HContext next;
	HContext i = m_Contexts.Head();
	while (i != m_Contexts.InvalidIndex())
	{
		next = m_Contexts.Next(i);
		DestroyContext( i );
		i = next;
	}
}


//-----------------------------------------------------------------------------
// Returns the current context
//-----------------------------------------------------------------------------
CVGui::Context_t *CVGui::GetContext( HContext context )
{
	if (context == DEFAULT_VGUI_CONTEXT)
		return &m_DefaultContext;
	return &m_Contexts[context];
}


//-----------------------------------------------------------------------------
// Associates a particular panel with a context
// Associating NULL is valid; it disconnects the panel from the context
//-----------------------------------------------------------------------------
void CVGui::AssociatePanelWithContext( HContext context, VPANEL pRoot )
{
	Assert( context != DEFAULT_VGUI_CONTEXT );
	input()->AssociatePanelWithInputContext( GetContext(context)->m_hInputContext, pRoot );
}


//-----------------------------------------------------------------------------
// Activates a particular context, use DEFAULT_VGUI_CONTEXT
// to get the one normally used by VGUI
//-----------------------------------------------------------------------------
void CVGui::ActivateContext( HContext context )
{
	Assert( (context == DEFAULT_VGUI_CONTEXT) || m_Contexts.IsValidIndex(context) );

	if (m_hContext != context)
	{
		// Clear out any messages queues that may be full...
		DispatchMessages();

		m_hContext = context;
		input()->ActivateInputContext( GetContext(m_hContext)->m_hInputContext ); 

		if (context != DEFAULT_VGUI_CONTEXT)
		{
			input()->RunFrame( );
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: Runs a single vgui frame, pumping all message to panels
//-----------------------------------------------------------------------------
void CVGui::RunFrame() 
{
	Assert(_heapchk() == _HEAPOK);
	bool shouldSleep = false;

#ifdef DEBUG
//  memory allocation debug helper
//	DPrintf( "Delete Count:%i,%i\n", m_iDeleteCount, m_iDeletePanelCount );
//	m_iDeleteCount = 	m_iDeletePanelCount = 0;
#endif

	// this will generate all key and mouse events as well as make a real repaint
	{
		VPROF( "surface()->RunFrame()" );
		surface()->RunFrame();
	}

	// give the system a chance to process
	{
		VPROF( "system()->RunFrame()" );
		system()->RunFrame();
	}

	// update cursor positions
	int cursorX, cursorY;
	{
		VPROF( "update cursor positions" );
		input()->GetCursorPos(cursorX, cursorY);

		// this does the actual work given a x,y and a surface
		input()->UpdateMouseFocus(cursorX, cursorY);

		// if the cursor is not over any of the surfaces the mouse focus is set to null
		if (!surface()->IsWithin(cursorX, cursorY))
		{
			input()->SetMouseFocus(NULL);
			if (input()->GetFocus() == NULL)
			{
				// nothing has focus, sleep
				shouldSleep = true;
			}
		}
	}

	// while we are at it, this looks like a good place to set the emulated cursor position to its current position
//	_private->_surfaceBaseDar[i]->setEmulatedCursorPos(cursorX, cursorY);

	{
		VPROF( "input()->RunFrame()" );
		input()->RunFrame();
	}

	// messenging
	{
		VPROF( "messenging" );
				// send all the messages waiting in the queue
		// send all the messages waiting in the queue
		if (!DispatchMessages())
		{
			shouldSleep = true;
		}

		int time = system()->GetTimeMillis();

		// directly invoke tick all who asked to be ticked
		int count = m_TickSignalVec.Count();
		for (int i = count - 1; i >= 0; i-- )
		{
			Tick_t *t = m_TickSignalVec[i];
			
			VPANEL tickTarget = t->panel;
			if ( !tickTarget )
			{
				m_TickSignalVec.Remove( i );
				delete t;
				continue;
			}

			if ( t->interval != 0 )
			{
				if ( time < t->nexttick )
					continue;

				t->nexttick = time + t->interval;
			}

			PostMessage(tickTarget, new KeyValues("Tick"), NULL);
		}
	}

	{
		VPROF( "SolveTraverse" );
		// make sure the hierarchy is up to date
		surface()->SolveTraverse(surface()->GetEmbeddedPanel());
		surface()->ApplyChanges();

		Assert(_heapchk() == _HEAPOK);
	}

	if (m_bDoSleep && shouldSleep)
	{
		::Sleep(5);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
VPANEL CVGui::AllocPanel()
{
	m_iPanelsAlloced++;
#ifdef DEBUG
	m_iDeleteCount++;
#endif

	VPanel *panel = new VPanel;
	PanelCreated(panel);
	return (VPANEL)panel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVGui::FreePanel(VPANEL ipanel)
{
	PanelDeleted((VPanel *)ipanel);
	delete (VPanel *)ipanel;
	m_iPanelsAlloced--;
#ifdef DEBUG
	m_iDeleteCount--;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Returns the safe index of the panel
//-----------------------------------------------------------------------------
HPanel CVGui::PanelToHandle(VPANEL panel)
{
	if (!panel)
		return INVALID_PANEL;

	HPanelList baseIndex =  ((VPanel *)panel)->GetListEntry();
	return (baseIndex << 16) + m_PanelList[baseIndex].m_iSerialNumber;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the panel at the specified index
//-----------------------------------------------------------------------------
VPANEL CVGui::HandleToPanel(HPanel index)
{
	HPanelList baseIndex = static_cast<HPanelList>( (index >> 16) );
	int serial = index - (baseIndex << 16);

	if ( index == INVALID_PANEL ||  !m_PanelList.IsValidIndex(baseIndex) )
	{
		return NULL;
	}

	if (m_PanelList[baseIndex].m_iSerialNumber != serial)
		return NULL;

	return (VPANEL)(m_PanelList[baseIndex].m_pPanel);
}

//-----------------------------------------------------------------------------
// Purpose: Called whenever a panel is constructed
//-----------------------------------------------------------------------------
void CVGui::PanelCreated(VPanel *panel)
{
	SerialPanel_t slot;
	slot.m_pPanel = (VPANEL)panel;
	slot.m_iSerialNumber = m_iNextSerialNumber++;
	HPanelList index = m_PanelList.AddToTail();
	m_PanelList[ index ] = slot;

	Assert( m_PanelList.Count() < WARN_PANEL_NUMBER );

	((VPanel *)panel)->SetListEntry( index );

	surface()->AddPanel((VPANEL)panel);
}

//-----------------------------------------------------------------------------
// Purpose: instantly stops the app from pointing to the focus'd object
//			used when an object is being deleted
//-----------------------------------------------------------------------------
void CVGui::PanelDeleted(VPanel *focus)
{
	Assert( focus );
	surface()->ReleasePanel((VPANEL)focus);
	input()->PanelDeleted((VPANEL)focus);

	// remove from safe handle list

	Assert( m_PanelList.IsValidIndex(((VPanel *)focus)->GetListEntry()) );
	if ( m_PanelList.IsValidIndex(((VPanel *)focus)->GetListEntry()) )
	{
		m_PanelList.Remove( ((VPanel *)focus)->GetListEntry() );
	}

	((VPanel *)focus)->SetListEntry( INVALID_PANELLIST );

	// remove from tick signal dar
	RemoveTickSignal( (VPANEL)focus );
}


//-----------------------------------------------------------------------------
// Purpose: Adds the panel to a tick signal list, so the panel receives a message every frame
// Input  : *panel - 
//-----------------------------------------------------------------------------
void CVGui::AddTickSignal(VPANEL panel, int intervalMilliseconds /*=0*/ )
{
	Tick_t *t;
	// See if it's already in list
	int count = m_TickSignalVec.Count();
	for (int i = 0; i < count; i++ )
	{
		Tick_t *t = m_TickSignalVec[i];
		if ( t->panel == panel )
		{
			// Go ahead and update intervals
			t->interval = intervalMilliseconds;
			t->nexttick = system()->GetTimeMillis() + t->interval;
			return;
		}
	}

	// Add to list
	t = new Tick_t;

	t->panel = panel;
	t->interval = intervalMilliseconds;
	t->nexttick = system()->GetTimeMillis() + t->interval;

	if ( strlen( ((VPanel *)panel)->Client()->GetName() ) > 0 )
	{
		strncpy( t->panelname, ((VPanel *)panel)->Client()->GetName(), sizeof( t->panelname ) );
	}
	else
	{
		strncpy( t->panelname, ((VPanel *)panel)->Client()->GetClassName(), sizeof( t->panelname ) );
	}

	// simply add the element to the list 
	m_TickSignalVec.AddToTail( t );
	// panel is removed from list when deleted
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : panel - 
//-----------------------------------------------------------------------------
void CVGui::RemoveTickSignal( VPANEL panel )
{
	// remove from tick signal dar
	int count = m_TickSignalVec.Count();

	for (int i = 0; i < count; i++ )
	{
		Tick_t *tick = m_TickSignalVec[i];
		if ( tick->panel == panel )
		{
			m_TickSignalVec.Remove( i );
			delete tick;
			return;
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: message pump
//			loops through and sends all active messages
//			note that more messages may be posted during the process
//-----------------------------------------------------------------------------
bool CVGui::DispatchMessages()
{
	int time = system()->GetTimeMillis();

	m_InDispatcher = true;
	bool doneWork = (m_MessageQueue.Count() > 12);

	bool bUsingDelayedQueue = (m_DelayedMessageQueue.Count() > 0);

	while (m_MessageQueue.Count() > 0 || (m_SecondaryQueue.Count() > 0) || bUsingDelayedQueue)
	{
		// get the first message
		MessageItem_t *messageItem = NULL;
		int messageIndex = 0;

		// use the secondary queue until it empties. empty it after each message in the
		// primary queue. this makes primary messages completely resolve 
		bool bUsingSecondaryQueue = (m_SecondaryQueue.Count() > 0);
		if (bUsingSecondaryQueue)
		{
			doneWork = true;
			messageIndex = m_SecondaryQueue.Head();
			messageItem = &m_SecondaryQueue[messageIndex];
		}
		else if (bUsingDelayedQueue)
		{
			if (m_DelayedMessageQueue.Count() >0)
			{
				messageItem = (MessageItem_t*)&m_DelayedMessageQueue.ElementAtHead();
			}
			if (!messageItem || messageItem->_arrivalTime > time)
			{
				// no more items in the delayed message queue, move to the system queue
				bUsingDelayedQueue = false;
				continue;
			}
		}
		else
		{
			messageIndex = m_MessageQueue.Head();
			messageItem = &m_MessageQueue[messageIndex];
		}

		// message debug code 
	/*	
		char qname[20] = "Primary";
		if (bUsingSecondaryQueue)
			strcpy(qname, "Secondary");
		
		if (strcmp(messageItem->_params->GetName(), "Tick")
			&& strcmp(messageItem->_params->GetName(), "MouseFocusTicked") 
			&& strcmp(messageItem->_params->GetName(), "KeyFocusTicked")
			&& strcmp(messageItem->_params->GetName(), "CursorMoved"))
		{
			if (!stricmp(messageItem->_params->GetName(), "command"))
			{
				ivgui()->DPrintf2( "%s Queue dispatching command( %s, %s -- %i )\n", qname, messageItem->_params->GetName(), messageItem->_params->GetString("command"), messageItem->_messageID );
			}
			else
			{
				ivgui()->DPrintf2( "%s Queue dispatching( %s -- %i )\n", qname ,messageItem->_params->GetName(), messageItem->_messageID );
			}
		}
	*/	
		
		// send it
		KeyValues *params = messageItem->_params;
		VPanel *vto = (VPanel *)ivgui()->HandleToPanel(messageItem->_messageTo);
		if (vto)
		{
			vto->SendMessage(params, ivgui()->HandleToPanel(messageItem->_from));
		}

		// free the keyvalues memory
		// we can't reference the messageItem pointer anymore since the queue might have moved in memory
		if (params)
		{
			params->deleteThis();
		}

		// remove it from the queue
		if (bUsingSecondaryQueue)
		{
			m_SecondaryQueue.Remove(messageIndex);
		}
		else if (bUsingDelayedQueue)
		{
			m_DelayedMessageQueue.RemoveAtHead();
		}
		else
		{
			m_MessageQueue.Remove(messageIndex);
		}
	}

	m_InDispatcher = false;
	return doneWork;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVGui::MarkPanelForDeletion(VPANEL panel)
{
	PostMessage(panel, new KeyValues("Delete"), NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Adds a message to the queue to be sent to a user
//-----------------------------------------------------------------------------
void CVGui::PostMessage(VPANEL target, KeyValues *params, VPANEL from, float delay)
{
	if (!target)
	{
		if (params)
		{
			params->deleteThis();
		}
		return;
	}

	MessageItem_t messageItem;
	messageItem._messageTo = ivgui()->PanelToHandle(target);
	messageItem._params = params;
	Assert(params->GetName());
	messageItem._from = ivgui()->PanelToHandle(from);
	messageItem._arrivalTime = 0;
	messageItem._messageID = m_iCurrentMessageID++;

	/* message debug code
	//if ( stricmp(messageItem._params->GetName(),"CursorMoved") && stricmp(messageItem._params->GetName(),"KeyFocusTicked"))
	{
		ivgui()->DPrintf2( "posting( %s -- %i )\n", messageItem._params->GetName(), messageItem._messageID );
	}
	*/
				
	// add the message to the correct message queue
	if (delay > 0.0f)
	{
		messageItem._arrivalTime = system()->GetTimeMillis() + (delay * 1000);
		m_DelayedMessageQueue.Insert(messageItem);
	}
	else if (m_InDispatcher)
	{
		m_SecondaryQueue.AddToTail(messageItem);
	}
	else
	{
		m_MessageQueue.AddToTail(messageItem);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVGui::ShutdownMessage(unsigned int shutdownID)
{
	// broadcast Shutdown to all the top level windows, and see if any take notice
	VPANEL panel = surface()->GetEmbeddedPanel();
	for (int i = 0; i < ((VPanel *)panel)->GetChildCount(); i++)
	{
		ivgui()->PostMessage((VPANEL)((VPanel *)panel)->GetChild(i), new KeyValues("ShutdownRequest", "id", shutdownID), NULL);
	}

	// post to the top level window as well
	ivgui()->PostMessage(panel, new KeyValues("ShutdownRequest", "id", shutdownID), NULL);
}


/*
static void*(*staticMalloc)(size_t size)=malloc;
static void(*staticFree)(void* memblock)=free;

static int g_iMemoryBlocksAllocated = 0;

void *operator new(size_t size)
{
	g_iMemoryBlocksAllocated += 1;
	return staticMalloc(size);
}

void operator delete(void* memblock)
{
	if (!memblock)
		return;

	g_iMemoryBlocksAllocated -= 1;

	if (g_iMemoryBlocksAllocated < 0)
	{
		int x = 3;
	}

	staticFree(memblock);
}

void *operator new [] (size_t size)
{
	return staticMalloc(size);
}

void operator delete [] (void *pMem)
{
	staticFree(pMem);
}
*/

void CVGui::DPrintf(const char* format,...)
{
	char    buf[2048];
	va_list argList;

	va_start(argList,format);
	vsprintf(buf,format,argList);
	va_end(argList);

	::OutputDebugString(buf);
}

void CVGui::DPrintf2(const char* format,...)
{
	char    buf[2048];
	va_list argList;
	static int ctr=0;

	sprintf(buf,"%d:",ctr++);

	va_start(argList,format);
	vsprintf(buf+strlen(buf),format,argList);
	va_end(argList);

	::OutputDebugString(buf);
}

void vgui::vgui_strcpy(char* dst,int dstLen,const char* src)
{
	Assert(dst!=null);
	Assert(dstLen>=0);
	Assert(src!=null);

	int srcLen=strlen(src)+1;
	if(srcLen>dstLen)
	{
		srcLen=dstLen;
	}

	memcpy(dst,src,srcLen-1);
	dst[srcLen-1]=0;
}

//-----------------------------------------------------------------------------
	// HL2/TFC specific stuff
//-----------------------------------------------------------------------------
// Here's where the app systems get to learn about each other 
//-----------------------------------------------------------------------------
bool CVGui::Connect( CreateInterfaceFn factory )
{
	return VGui_InternalLoadInterfaces( &factory, 1 );
}

void CVGui::Disconnect()
{
	// FIXME: Blat out interface pointers
}


//-----------------------------------------------------------------------------
// Init, shutdown
//-----------------------------------------------------------------------------
InitReturnVal_t CVGui::Init()
{
	m_hContext = DEFAULT_VGUI_CONTEXT;
	return INIT_OK;
}

void CVGui::Shutdown()
{
	DestroyAllContexts();
	scheme()->Shutdown();
}

//-----------------------------------------------------------------------------
// Here's where systems can access other interfaces implemented by this object
// Returns NULL if it doesn't implement the requested interface
//-----------------------------------------------------------------------------
void *CVGui::QueryInterface( const char *pInterfaceName )
{
	// FIXME: Should this go here?
	// Access other global interfaces exposed by this system...
	CreateInterfaceFn vguiFactory = Sys_GetFactoryThis();
	return vguiFactory( pInterfaceName, NULL );
}

	// End of specific interface
//-----------------------------------------------------------------------------

