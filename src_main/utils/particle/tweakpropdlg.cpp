
#include <windows.h>
#include "tweakpropdlg.h"
#include "datatable_recv.h"

#define INSIDE_BORDER		5			// Border around the inside of the client area.

#define CONTROL_WIDTH		150			// Horizontal size of controls (and text).
#define CONTROL_HEIGHT		20			// Max vertical size of each control.
#define CONTROL_SPACING		4			// Vertical spacing of controls.


#define TWEAKPROPDLG_CLASSNAME	"TweakPropDlg"


class CTweakPropDlg;


#define MAX_TWEAK_PROPS		256
class TweakProp
{
public:
	CTweakPropDlg	*m_pDlg;		// This points back at the parent dialog.
	RecvProp		*m_pRecvProp;
	int				m_Type;			// Indexes g_TweakPropFunctions.
	HWND			m_hWnd;			// Control window.
	HWND			m_hTitleWnd;	// Title text control (static control).
};



// ------------------------------------------------------------------------ //
// This abstracts out the editable property types.
// ------------------------------------------------------------------------ //
class TweakPropFunctions
{
public:
	// Returns false if it can't handle this property type (in pProp->m_pRecvProp).
	typedef bool				(*CreateEditControlFn)(TweakProp *pProp, RECT *pRect, int controlID);
	typedef LRESULT				(*HandleMessageFn)(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	
	// Set the value in the control into the object.
	typedef void				(*SetValueFn)(TweakProp *pProp);

	CreateEditControlFn			m_CreateEditControl;
	HandleMessageFn				m_HandleMessage;
	SetValueFn					m_SetValue;
};

extern TweakPropFunctions* GetTweakPropFunctions();
extern int NumTweakPropFunctions();




class CTweakPropDlg : public ITweakPropDlg
{
public:
					CTweakPropDlg();
	virtual			~CTweakPropDlg();

	bool			Init(RecvTable *pTable, void *pObj, char *pTitle, void *hParentWindow, void *hInstance);
	void			Term();

	// The window proc.
	static LRESULT CALLBACK StaticWndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	LRESULT			WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);


public:
	virtual void	Release();
	virtual void	SetObj(void *pObj);


public:
	HWND			m_hDlg;
	HINSTANCE		m_hInstance;
	HFONT			m_hFont;

	void			*m_pObj;	// Base address of the object we're editing.

	// Editable properties.
	TweakProp		m_Props[MAX_TWEAK_PROPS];
	int				m_nProps;
};



CTweakPropDlg::CTweakPropDlg()
{
	m_hDlg = 0;
}


CTweakPropDlg::~CTweakPropDlg()
{
	Term();
}


bool CTweakPropDlg::Init(RecvTable *pTable, void *pObj, char *pTitle, void *hParentWindow, void *hInstance)
{
	Term();


	m_hInstance = (HINSTANCE)hInstance;
	m_pObj = pObj;

	
	HDC hDC = GetDC((HWND)hParentWindow);
	m_hFont = CreateFont(
		-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72),
		0,
		0,		// escapement
		0,		// orientation
		FW_REGULAR,	// font weight
		false, false, false,	// italic, underline, strikeout
		ANSI_CHARSET,
		OUT_DEFAULT_PRECIS,		// output precision
		CLIP_DEFAULT_PRECIS,	// clipping precision
		DEFAULT_QUALITY,		// quality
		DEFAULT_PITCH,			// pitch
		"Courier New"
		);
	ReleaseDC((HWND)hParentWindow, hDC);


	// Create the dialog.
	WNDCLASSEX wndclass;

	wndclass.cbSize        = sizeof (wndclass);
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = (WNDPROC)&CTweakPropDlg::StaticWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = m_hInstance;
	wndclass.hIcon         = 0;
	wndclass.hCursor       = LoadCursor (m_hInstance, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = TWEAKPROPDLG_CLASSNAME;
	wndclass.hIconSm       = 0;

	if (!RegisterClassEx (&wndclass))
	{
		return false;
	}


	m_hDlg = CreateWindowEx (
		WS_EX_TOOLWINDOW,
	   TWEAKPROPDLG_CLASSNAME,               // window class name
	   pTitle,                // window caption
	   WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, // window style
	   0,           // initial x position
	   0,           // initial y position
	   50,           // initial x size
	   50,           // initial y size
	   NULL,                    // parent window handle
	   NULL,                    // window menu handle
	   m_hInstance,       // program instance handle
	   NULL);
	if(!m_hDlg)
	{
	   Term();
	   return false;
	}

	SetWindowLong(m_hDlg, GWL_USERDATA, (LONG)this);
	ShowWindow(m_hDlg, SW_SHOW);
	UpdateWindow(m_hDlg);


	// Create a control for each property.
	m_nProps = 0;
	for(int iProp=0; iProp < pTable->m_nProps; iProp++)
	{
		TweakProp *pOutProp = &m_Props[m_nProps];

		pOutProp->m_pRecvProp = &pTable->m_pProps[iProp];
		pOutProp->m_pDlg = this;

		RECT rcControl;
		rcControl.left = INSIDE_BORDER + CONTROL_WIDTH;
		rcControl.right = rcControl.left + CONTROL_WIDTH;
		rcControl.top = INSIDE_BORDER + (CONTROL_HEIGHT + CONTROL_SPACING) * m_nProps;
		rcControl.bottom = rcControl.top + CONTROL_HEIGHT;
	
		// Find a handler that can handle it.
		for(int iFn=0; iFn < NumTweakPropFunctions(); iFn++)
		{
			pOutProp->m_Type = iFn;

			if(GetTweakPropFunctions()[iFn].m_CreateEditControl(pOutProp, &rcControl, m_nProps))
			{
				// Create its title text.
				RECT rcText;
				rcText.left = INSIDE_BORDER;
				rcText.right = rcText.left + CONTROL_WIDTH;
				rcText.top = INSIDE_BORDER + (CONTROL_HEIGHT + CONTROL_SPACING) * m_nProps;
				rcText.bottom = rcControl.top + CONTROL_HEIGHT;

				pOutProp->m_hTitleWnd = CreateWindow (
								"static",		// Class name
								pOutProp->m_pRecvProp->m_pVarName,           // Window text
								SS_CENTER | WS_CHILD | WS_VISIBLE | WS_BORDER,       // Window style
								rcText.left,              // x coordinate of the upper-left corner
								rcText.top,              // y coordinate of the upper-left corner
								rcText.right - rcText.left,  // Width of the edit control window
								rcText.bottom - rcText.top,  // Height of the edit control window
								m_hDlg,           // Window handle to parent window
								(HMENU)m_nProps+MAX_TWEAK_PROPS, // Control identifier
								m_hInstance,        // Instance handle
								NULL);          // Specify NULL for this parameter when 
												// creating a control

				SendMessage(pOutProp->m_hTitleWnd, WM_SETFONT, (WPARAM)m_hFont, true);

				++m_nProps;
				if(m_nProps >= MAX_TWEAK_PROPS)
					break;
			}

			if(m_nProps >= MAX_TWEAK_PROPS)
				break;
		}
	}

	// Resize our window to fit all the properties in.
	int newWidth = INSIDE_BORDER*3 + CONTROL_WIDTH*2;
	int newHeight = INSIDE_BORDER*2 + (CONTROL_HEIGHT+CONTROL_SPACING) * (m_nProps+1);
	SetWindowPos(m_hDlg, NULL, 0, 0, newWidth, newHeight, SWP_NOMOVE|SWP_NOZORDER);

	return true;
}


void CTweakPropDlg::Term()
{
	if(m_hDlg)
	{
		SetWindowLong(m_hDlg, GWL_USERDATA, NULL);
		DestroyWindow(m_hDlg);
		m_hDlg = NULL;
	}

	DeleteObject(m_hFont);
	m_hFont = NULL;
}


LRESULT CTweakPropDlg::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if(iMsg == WM_COMMAND)
	{
		int controlID = LOWORD(wParam);
		if(controlID >= 0 && controlID < MAX_TWEAK_PROPS)
		{
			return GetTweakPropFunctions()[m_Props[controlID].m_Type].m_HandleMessage(hwnd, iMsg, wParam, lParam);
		}
	}
	
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}


LRESULT CTweakPropDlg::StaticWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	CTweakPropDlg *pDlg = (CTweakPropDlg*)GetWindowLong(hwnd, GWL_USERDATA);
	if(pDlg)
		return pDlg->WndProc(hwnd, iMsg, wParam, lParam);
	else	
		return DefWindowProc(hwnd, iMsg, wParam, lParam);
}


void CTweakPropDlg::Release()
{
	delete this;
}


void CTweakPropDlg::SetObj(void *pObj)
{
	m_pObj = pObj;

	for(int i=0; i < m_nProps; i++)
		GetTweakPropFunctions()[m_Props[i].m_Type].m_SetValue(&m_Props[i]);
}


ITweakPropDlg* CreateTweakPropDlg(
	RecvTable *pTable, 
	void *pObj, 
	char *pTitle,
	void *hParentWindow,
	void *hInstance)
{
	CTweakPropDlg *pDlg = new CTweakPropDlg;
	if(!pDlg || !pDlg->Init(pTable, pObj, pTitle, hParentWindow, hInstance))
	{
		pDlg->Release();
		return false;
	}

	return pDlg;
}


// ------------------------------------------------------------------------ //
// Functions for editable property types.
// ------------------------------------------------------------------------ //

void WriteValueToControl_Number(TweakProp *pProp)
{
	char valStr[256];

	valStr[0] = 0;

	RecvProp *pRecvProp = pProp->m_pRecvProp;

	void *pData = ((unsigned char*)pProp->m_pDlg->m_pObj) + pProp->m_pRecvProp->Offset();
	if(pRecvProp->Type() == DPT_Int)
	{
		if( pRecvProp->ProxyFn() == RecvProxy_Int32ToInt32 )
			sprintf(valStr, "%d", *((int*)pData));
		else if( pRecvProp->ProxyFn() == RecvProxy_Int32ToInt16 )
			sprintf(valStr, "%d", *((unsigned short*)pData));
		else
			sprintf(valStr, "%d", *((unsigned char*)pData));
	}
	else if(pProp->m_pRecvProp->Type() == DPT_Float)
	{
		sprintf(valStr, "%.3f", *((float*)pData));
	}
	else if(pProp->m_pRecvProp->Type() == DPT_Vector)
	{
		sprintf(valStr, "%.3f %.3f %.3f", ((float*)pData)[0], ((float*)pData)[1], ((float*)pData)[2]);
	}

	SetWindowText(pProp->m_hWnd, valStr);
}


bool CreateEditControl_Number(TweakProp *pProp, RECT *pRect, int controlID)
{
	if(pProp->m_pRecvProp->Type() != DPT_Int && 
		pProp->m_pRecvProp->Type() != DPT_Float && 
		pProp->m_pRecvProp->Type() != DPT_Vector)
	{
		return false;
	}

	pProp->m_hWnd = CreateWindow (
					"edit",		// Class name
					NULL,           // Window text
					ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,        // Window style
					pRect->left,              // x coordinate of the upper-left corner
					pRect->top,              // y coordinate of the upper-left corner
					pRect->right-pRect->left,  // Width of the edit control window
					pRect->bottom-pRect->top,  // Height of the edit control window
					pProp->m_pDlg->m_hDlg,           // Window handle to parent window
					(HMENU)controlID, // Control identifier
					(HINSTANCE)pProp->m_pDlg->m_hInstance,        // Instance handle
					NULL);          // Specify NULL for this parameter when 
									// creating a control
	
	SendMessage(pProp->m_hWnd, WM_SETFONT, (WPARAM)pProp->m_pDlg->m_hFont, true);
	SetWindowLong(pProp->m_hWnd, GWL_USERDATA, (LONG)pProp);
	WriteValueToControl_Number(pProp);

	return !!pProp->m_hWnd;
}	

void SetValue_Number(TweakProp *pProp)
{
	char valueText[256];
	DVariant variant;

	
	GetWindowText(pProp->m_hWnd, valueText, sizeof(valueText));

	if(pProp->m_pRecvProp->Type() == DPT_Int)
	{
		sscanf(valueText, "%d", &variant.m_Int);
	}			
	else if(pProp->m_pRecvProp->Type() == DPT_Float)
	{
		sscanf(valueText, "%f", &variant.m_Float);
	}
	else if(pProp->m_pRecvProp->Type() == DPT_Vector)
	{
		sscanf(valueText, "%f %f %f", &variant.m_Vector[0], &variant.m_Vector[1], &variant.m_Vector[2]);
	}

	void *pData = ((unsigned char*)pProp->m_pDlg->m_pObj) + pProp->m_pRecvProp->Offset();
	pProp->m_pRecvProp->ProxyFn()(
		pProp->m_pRecvProp,
		pProp->m_pDlg->m_pObj,
		pData,
		&variant,
		0);
}

LRESULT HandleMessage_Number(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	TweakProp *pProp = (TweakProp*)GetWindowLong((HWND)lParam, GWL_USERDATA);
	if(pProp)
	{
		if(HIWORD(wParam) == EN_UPDATE)
		{
			SetValue_Number(pProp);
		}
	}

	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}




TweakPropFunctions g_TweakPropFunctions[] =
{
	{CreateEditControl_Number, HandleMessage_Number, SetValue_Number}
};

TweakPropFunctions* GetTweakPropFunctions()
{
	return g_TweakPropFunctions;
}

int NumTweakPropFunctions()
{
	return sizeof(g_TweakPropFunctions) / sizeof(g_TweakPropFunctions[0]);
}


