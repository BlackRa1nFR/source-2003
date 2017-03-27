// vcrtrace.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "tier0/vcrmode.h"



IVCRTrace *g_pTrace = 0;


class CVCRHelpers : public IVCRHelpers
{
public:
	virtual void	ErrorMessage( const char *pMsg )
	{
		printf("ERROR: %s\n", pMsg);
	}

	virtual void*	GetMainWindow()
	{
		return 0;
	}
};

static CVCRHelpers g_VCRHelpers;


void ErrFunction( char const *pMsg )
{
	printf( "%s", pMsg );
}


typedef struct
{
	int			m_Message;
	char const	*m_pName;
} WMessage;
#define DEFINE_WMESSAGE(x) x, #x


// Message table for windows messages.
WMessage g_WMessages[] =
{
	DEFINE_WMESSAGE(WM_NOTIFY),
	DEFINE_WMESSAGE(WM_INPUTLANGCHANGEREQUEST),
	DEFINE_WMESSAGE(WM_INPUTLANGCHANGE),
	DEFINE_WMESSAGE(WM_TCARD),
	DEFINE_WMESSAGE(WM_HELP),
	DEFINE_WMESSAGE(WM_USERCHANGED),
	DEFINE_WMESSAGE(WM_NOTIFYFORMAT),
	DEFINE_WMESSAGE(NFR_ANSI),
	DEFINE_WMESSAGE(NFR_UNICODE),
	DEFINE_WMESSAGE(NF_QUERY),
	DEFINE_WMESSAGE(NF_REQUERY),
	DEFINE_WMESSAGE(WM_CONTEXTMENU),
	DEFINE_WMESSAGE(WM_STYLECHANGING),
	DEFINE_WMESSAGE(WM_STYLECHANGED),
	DEFINE_WMESSAGE(WM_DISPLAYCHANGE),
	DEFINE_WMESSAGE(WM_GETICON),
	DEFINE_WMESSAGE(WM_SETICON),
	DEFINE_WMESSAGE(WM_NCCREATE),
	DEFINE_WMESSAGE(WM_NCDESTROY),
	DEFINE_WMESSAGE(WM_NCCALCSIZE),
	DEFINE_WMESSAGE(WM_NCHITTEST),
	DEFINE_WMESSAGE(WM_NCPAINT),
	DEFINE_WMESSAGE(WM_NCACTIVATE),
	DEFINE_WMESSAGE(WM_GETDLGCODE),
	DEFINE_WMESSAGE(WM_SYNCPAINT),
	DEFINE_WMESSAGE(WM_NCMOUSEMOVE),
	DEFINE_WMESSAGE(WM_NCLBUTTONDOWN),
	DEFINE_WMESSAGE(WM_NCLBUTTONUP),
	DEFINE_WMESSAGE(WM_NCLBUTTONDBLCLK),
	DEFINE_WMESSAGE(WM_NCRBUTTONDOWN),
	DEFINE_WMESSAGE(WM_NCRBUTTONUP),
	DEFINE_WMESSAGE(WM_NCRBUTTONDBLCLK),
	DEFINE_WMESSAGE(WM_NCMBUTTONDOWN),
	DEFINE_WMESSAGE(WM_NCMBUTTONUP),
	DEFINE_WMESSAGE(WM_NCMBUTTONDBLCLK),
	DEFINE_WMESSAGE(WM_KEYFIRST),
	DEFINE_WMESSAGE(WM_KEYDOWN),
	DEFINE_WMESSAGE(WM_KEYUP),
	DEFINE_WMESSAGE(WM_CHAR),
	DEFINE_WMESSAGE(WM_DEADCHAR),
	DEFINE_WMESSAGE(WM_SYSKEYDOWN),
	DEFINE_WMESSAGE(WM_SYSKEYUP),
	DEFINE_WMESSAGE(WM_SYSCHAR),
	DEFINE_WMESSAGE(WM_SYSDEADCHAR),
	DEFINE_WMESSAGE(WM_KEYLAST),
	DEFINE_WMESSAGE(WM_IME_STARTCOMPOSITION),
	DEFINE_WMESSAGE(WM_IME_ENDCOMPOSITION),
	DEFINE_WMESSAGE(WM_IME_COMPOSITION),
	DEFINE_WMESSAGE(WM_IME_KEYLAST),
	DEFINE_WMESSAGE(WM_INITDIALOG),
	DEFINE_WMESSAGE(WM_COMMAND),
	DEFINE_WMESSAGE(WM_SYSCOMMAND),
	DEFINE_WMESSAGE(WM_TIMER),
	DEFINE_WMESSAGE(WM_HSCROLL),
	DEFINE_WMESSAGE(WM_VSCROLL),
	DEFINE_WMESSAGE(WM_INITMENU),
	DEFINE_WMESSAGE(WM_INITMENUPOPUP),
	DEFINE_WMESSAGE(WM_MENUSELECT),
	DEFINE_WMESSAGE(WM_MENUCHAR),
	DEFINE_WMESSAGE(WM_ENTERIDLE),
	DEFINE_WMESSAGE(WM_CTLCOLORMSGBOX),
	DEFINE_WMESSAGE(WM_CTLCOLOREDIT),
	DEFINE_WMESSAGE(WM_CTLCOLORLISTBOX),
	DEFINE_WMESSAGE(WM_CTLCOLORBTN),
	DEFINE_WMESSAGE(WM_CTLCOLORDLG),
	DEFINE_WMESSAGE(WM_CTLCOLORSCROLLBAR),
	DEFINE_WMESSAGE(WM_CTLCOLORSTATIC),
	DEFINE_WMESSAGE(WM_MOUSEMOVE),
	DEFINE_WMESSAGE(WM_LBUTTONDOWN),
	DEFINE_WMESSAGE(WM_LBUTTONUP),
	DEFINE_WMESSAGE(WM_LBUTTONDBLCLK),
	DEFINE_WMESSAGE(WM_RBUTTONDOWN),
	DEFINE_WMESSAGE(WM_RBUTTONUP),
	DEFINE_WMESSAGE(WM_RBUTTONDBLCLK),
	DEFINE_WMESSAGE(WM_MBUTTONDOWN),
	DEFINE_WMESSAGE(WM_MBUTTONUP),
	DEFINE_WMESSAGE(WM_MBUTTONDBLCLK),
	DEFINE_WMESSAGE(WM_MOUSELAST),
	DEFINE_WMESSAGE(WM_PARENTNOTIFY),
	DEFINE_WMESSAGE(WM_ENTERMENULOOP),
	DEFINE_WMESSAGE(WM_EXITMENULOOP                 )
};

// Message table for winsock messages.
WMessage g_WinsockMessages[] = 
{
	DEFINE_WMESSAGE(WSAEWOULDBLOCK),
	DEFINE_WMESSAGE(WSAEINPROGRESS),
	DEFINE_WMESSAGE(WSAEALREADY),
	DEFINE_WMESSAGE(WSAENOTSOCK),
	DEFINE_WMESSAGE(WSAEDESTADDRREQ),
	DEFINE_WMESSAGE(WSAEMSGSIZE),
	DEFINE_WMESSAGE(WSAEPROTOTYPE),
	DEFINE_WMESSAGE(WSAENOPROTOOPT),
	DEFINE_WMESSAGE(WSAEPROTONOSUPPORT),
	DEFINE_WMESSAGE(WSAESOCKTNOSUPPORT),
	DEFINE_WMESSAGE(WSAEOPNOTSUPP),
	DEFINE_WMESSAGE(WSAEPFNOSUPPORT),
	DEFINE_WMESSAGE(WSAEAFNOSUPPORT),
	DEFINE_WMESSAGE(WSAEADDRINUSE),
	DEFINE_WMESSAGE(WSAEADDRNOTAVAIL),
	DEFINE_WMESSAGE(WSAENETDOWN),
	DEFINE_WMESSAGE(WSAENETUNREACH),
	DEFINE_WMESSAGE(WSAENETRESET),
	DEFINE_WMESSAGE(WSAECONNABORTED),
	DEFINE_WMESSAGE(WSAECONNRESET),
	DEFINE_WMESSAGE(WSAENOBUFS),
	DEFINE_WMESSAGE(WSAEISCONN),
	DEFINE_WMESSAGE(WSAENOTCONN),
	DEFINE_WMESSAGE(WSAESHUTDOWN),
	DEFINE_WMESSAGE(WSAETOOMANYREFS),
	DEFINE_WMESSAGE(WSAETIMEDOUT),
	DEFINE_WMESSAGE(WSAECONNREFUSED),
	DEFINE_WMESSAGE(WSAELOOP),
	DEFINE_WMESSAGE(WSAENAMETOOLONG),
	DEFINE_WMESSAGE(WSAEHOSTDOWN),
	DEFINE_WMESSAGE(WSAEHOSTUNREACH),
	DEFINE_WMESSAGE(WSAENOTEMPTY),
	DEFINE_WMESSAGE(WSAEPROCLIM),
	DEFINE_WMESSAGE(WSAEUSERS),
	DEFINE_WMESSAGE(WSAEDQUOT),
	DEFINE_WMESSAGE(WSAESTALE),
	DEFINE_WMESSAGE(WSAEREMOTE),
	DEFINE_WMESSAGE(WSAEDISCON),
	DEFINE_WMESSAGE(WSASYSNOTREADY),
	DEFINE_WMESSAGE(WSAVERNOTSUPPORTED),
	DEFINE_WMESSAGE(WSANOTINITIALISED)
};


static char const* GetWMessageName(WMessage *pMessages, int nMessages, int msg)
{
	static char str[128];
	int i;

	for(i=0; i < nMessages; i++)
	{
		if(pMessages[i].m_Message == msg)
			return pMessages[i].m_pName;
	}

	sprintf(str, "%d", msg);
	return str;
}

template<class T>
static void ReadVal(T &val)
{
	g_pTrace->Read( &val, sizeof(val) );
}

static void VCR_TraceEvents()
{
	int iEvent = 0;
	VCREvent event;

	g_pTrace = g_pVCR->GetVCRTraceInterface();
	
	while( g_pVCR->GetMode() == VCR_Playback )
	{
		++iEvent;

		printf("%5d - ", iEvent);

		event = g_pTrace->ReadEvent();
		switch(event)
		{
			case VCREvent_Sys_FloatTime:
			{
				double ret;
				g_pTrace->Read(&ret, sizeof(ret));
				printf("Sys_FloatTime: %f\n", ret);
			}
			break;
			
			case VCREvent_recvfrom:
			{
				int ret;
				char buf[8192];
				
				g_pTrace->Read(&ret, sizeof(ret));

				assert(ret < (int)sizeof(buf));
				if(ret == SOCKET_ERROR)
				{
					int err;
					ReadVal(err);

					printf("recvfrom: %d (error %s)\n", ret, GetWMessageName(g_WinsockMessages, sizeof(g_WinsockMessages)/sizeof(g_WinsockMessages[0]), err));
				}
				else
				{
					printf("recvfrom: %d\n", ret);
					g_pTrace->Read(buf, ret);
				}
			}
			break;

			case VCREvent_SyncToken:
			{
				char len;
				char buf[256];

				g_pTrace->Read(&len, 1);
				g_pTrace->Read(buf, len);
				buf[len] = 0;

				printf("SyncToken: %s\n", buf);
			}
			break;

			case VCREvent_GetCursorPos:
			{
				POINT pt;
				ReadVal(pt);
				printf("GetCursorPos: (%d, %d)\n", pt.x, pt.y);
			}
			break;

			case VCREvent_SetCursorPos:
			{
				int x, y;
				g_pTrace->Read(&x, sizeof(x));
				g_pTrace->Read(&y, sizeof(y));
				printf("SetCursorPos: (%d, %d)\n", x, y);
			}
			break;

			case VCREvent_ScreenToClient:
			{
				POINT pt;
				ReadVal(pt);
				printf("ScreenToClient: (%d, %d)\n", pt.x, pt.y);
			}
			break;

			case VCREvent_Cmd_Exec:
			{
				int len;
				char *f;

				g_pTrace->Read(&len, sizeof(len));
				
				if(len != -1)
				{
					f = (char*)malloc(len);
					g_pTrace->Read(f, len);
				}
				
				printf("Cmd_Exec: %d\n", len);
			}
			break;

			case VCREvent_CmdLine:
			{
				int len;
				char str[8192];

				g_pTrace->Read(&len, sizeof(len));
				assert(len < sizeof(str));
				g_pTrace->Read(str, len);

				printf("CmdLine: %s\n", str);
			}
			break;

			case VCREvent_RegOpenKeyEx:
			{
				long ret;
				ReadVal(ret);
				printf("RegOpenKeyEx: %d\n", ret);
			}
			break;

			case VCREvent_RegSetValueEx:
			{
				long ret;
				ReadVal(ret);
				printf("RegSetValueEx: %d\n", ret);
			}
			break;

			case VCREvent_RegQueryValueEx:
			{
				long ret;
				unsigned long type, cbData;
				char dummy;

				ReadVal(ret);
				ReadVal(type);
				ReadVal(cbData);
				while(cbData)
				{
					ReadVal(dummy);
					cbData--;
				}

				printf("RegQueryValueEx\n");
			}
			break;

			case VCREvent_RegCreateKeyEx:
			{
				long ret;
				ReadVal(ret);
				printf("RegCreateKeyEx: %d\n", ret);
			}
			break;

			case VCREvent_RegCloseKey:
			{
				printf("VCREvent_RegCloseKey");
			}
			break;
			
			case VCREvent_PeekMessage:
			{
				MSG msg;
				int valid;

				g_pTrace->Read(&valid, sizeof(valid));
				if(valid)
				{
					g_pTrace->Read(&msg, sizeof(MSG));
				}

				printf("PeekMessage - msg: %s, wParam: %x, lParam: %x\n", GetWMessageName(g_WMessages, sizeof(g_WMessages)/sizeof(g_WMessages[0]), msg.message), msg.wParam, msg.lParam);
			}
			break;

			case VCREvent_GameMsg:
			{
				char valid;
				g_pTrace->Read( &valid, sizeof(valid) );

				if ( valid )
				{
					UINT uMsg;
					WPARAM wParam;
					LPARAM lParam;
					g_pTrace->Read( &uMsg, sizeof(uMsg) );
					g_pTrace->Read( &wParam, sizeof(wParam) );
					g_pTrace->Read( &lParam, sizeof(lParam) );
					printf( "GameMsg - msg: %s, wParam: %x, lParam: %x\n", GetWMessageName(g_WMessages, sizeof(g_WMessages)/sizeof(g_WMessages[0]), uMsg), wParam, lParam );
				}
				else
				{
					printf("GameMsg - <end>\n" );
				}
			}
			break;

			case VCREvent_GetNumberOfConsoleInputEvents:
			{
				char val;
				unsigned long nEvents;
				
				ReadVal( val );
				ReadVal( nEvents );

				printf( "GetNumberOfConsoleInputEvents (returned %d, nEvents = %d)\n", val, nEvents );
			}
			break;

			case VCREvent_ReadConsoleInput:
			{
				char val;
				unsigned long nRead;
				INPUT_RECORD recs[1024];

				ReadVal( val );
				if ( val )
				{
					ReadVal( nRead );
					g_pTrace->Read( recs, nRead * sizeof( INPUT_RECORD ) );
				}
				else
				{
					nRead = 0;
				}

				printf( "ReadConsoleInput (returned %d, nRead = %d)\n", val, nRead );
			}
			break;

			default:
			{
				printf( "***ERROR*** VCR_TraceEvent: invalid event" );
				return;
			}
		}
	}
}


int main(int argc, char* argv[])
{
	if(argc <= 1)
	{
		printf("vcrtrace <vcr filename>\n");
		return 1;
	}

	if( !g_pVCR->Start( argv[1], 0, &g_VCRHelpers ) )
	{
		printf("Invalid or mission VCR file '%s'\n", argv[1]);
		return 1;
	}

	VCR_TraceEvents();
	return 0;
}

