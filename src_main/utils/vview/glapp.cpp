/*

  glapp.c - Simple OpenGL shell
  
	There are several options allowed on the command line.  They are:
	-height : what window/screen height do you want to use?
	-width  : what window/screen width do you want to use?
	-bpp    : what color depth do you want to use?
	-window : create a rendering window rather than full-screen
	-fov    : use a field of view other than 90 degrees
*/

#pragma warning(disable:4305)
#pragma warning(disable:4244)
#include "stdafx.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmsystem.h>
#include "glapp.h"
#include "mathlib.h"

windata_t   WinData;
devinfo_t   DevInfo;
#define OSR2_BUILD_NUMBER 1111
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h

float       fAngle = 0.0f;

char       *szFSDesc[] = { "Windowed", "Full Screen" };

// The app needs to define these symbols
// g_szAppName[]			-- char array applicaton name
// void AppInit( void )		-- Called at init time
// void AppRender( void )	-- Called each frame (as often as possible)
// void AppExit( void )		-- Called to shut down
// void AppKey( int key, int down ); -- called on each key up/down
// void AppChar( int key ); -- key was pressed & released
extern "C" char g_szAppName[];
extern "C" void AppInit( void );
extern "C" void AppRender( float frametime, float mouseDeltaX, float mouseDeltaY );
extern "C" void AppExit( void );
extern "C" void AppKey( int key, int down );
extern "C" void AppChar( int key );

extern "C" unsigned int g_Time;
unsigned int g_Time = 0;

BOOL             CreateMainWindow( int width, int height, int bpp, BOOL fullscreen);
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

void MouseCapture( void )
{
	SetCapture( WinData.hWnd );
    ShowCursor(FALSE);
	SetCursorPos( WinData.centerx, WinData.centery );
}

void MouseRelease( void )
{
    ShowCursor(TRUE);
	ReleaseCapture();
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    MSG         msg;
    int         i;
    HDC         hdc;
	
    // Not changable by user
    WinData.hInstance    = hInstance;
    WinData.iCmdShow     = iCmdShow;
    WinData.wndproc      = (WNDPROC)WndProc;
    WinData.pResolutions = 0;
    WinData.NearClip     = 8.0f;
    WinData.FarClip      = 8192.0f;
	
    // User definable
    WinData.fov          = 90.0f;
    WinData.bAllowSoft   = FALSE;
    WinData.bFullScreen  = TRUE;
	
    // Get the current display device info
    hdc = GetDC( NULL );
    DevInfo.bpp    = GetDeviceCaps(hdc, BITSPIXEL);
    DevInfo.width  = GetSystemMetrics(SM_CXSCREEN);
    DevInfo.height = GetSystemMetrics(SM_CYSCREEN);
    ReleaseDC(NULL, hdc);
	
    // Parse the command line if there is one
    WinData.argc = 0;
    if (strlen(szCmdLine) > 0)
	{
        WinData.szCmdLine = szCmdLine;
        GetParameters();
	}
	
    // Default to 640 pixels wide
    if ((i = FindNumParameter("-width")) != -1)
	{
        WinData.width = i;
	}
    else
	{
        WinData.width = 640;
	}
	
    // Default to 480 pixels high
    if ((i = FindNumParameter("-height")) != -1)
	{
        WinData.height = i;
	}
    else
	{
        WinData.height = 480;
	}
	
    // Default to 16 bits of color per pixel
    if ((i = FindNumParameter("-bpp")) != -1)
	{
        WinData.bpp = i;
	}
    else
	{
        WinData.bpp = 16;
	}
	
    // Default to a 65 degree field of view
    if ((i = FindNumParameter("-fov")) != -1)
	{
        WinData.fov = (float)i;
	}
    else
	{
        WinData.fov = 90.0f;
	}
	
    // Check for windowed rendering
    WinData.bFullScreen = FALSE;
    if (FindParameter("-fullscreen"))
	{
        WinData.bFullScreen = TRUE;
	}
	
    // Build up the video mode list
    BuildModeList();
	
    // Set the desired video mode and/or color depth
    if (!SetVideoMode())
	{
        Cleanup();
        return 0;
	}
	
    // Create the main program window, start up OpenGL and create our viewport
    if (CreateMainWindow( WinData.width, WinData.height, WinData.bpp, WinData.bFullScreen) != TRUE)
	{
        ChangeDisplaySettings(0, 0);
        MessageBox(NULL, "Unable to create main window.\nProgram will now end.", "FATAL ERROR", MB_OK);
        Cleanup();
        return 0;
	}
	
    // Turn the cursor off for full-screen mode
    if (WinData.bFullScreen == TRUE)
	{ // Probably want to do this all the time anyway
        ShowCursor(FALSE);
	}

	RECT rect;
	GetWindowRect( WinData.hWnd, &rect );
	
	if ( rect.top < 0)
		rect.top = 0;
	if ( rect.left < 0)
		rect.left = 0;
	if ( rect.bottom >= WinData.height - 1 )
		rect.bottom = WinData.height - 1;
	if ( rect.right >= WinData.width )
		rect.right = WinData.width - 1;

	WinData.centerx = ( rect.left + rect.right ) / 2;
	WinData.centery = ( rect.top + rect.bottom ) / 2;

	// Capture the mouse
	MouseCapture();
	
    // We're live now
    WinData.bActive = TRUE;
	
	// Define this funciton to init your app
	MathLib_Init( true, true, true, 2.2, 2.2, 0, 2 );

	AppInit();

    // Begin the main program loop
    while (WinData.bActive == TRUE)
	{
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
            TranslateMessage (&msg);
            DispatchMessage (&msg);
		}
        if (WinData.hGLRC)
		{
            RenderScene();
		}
	}
    if (WinData.bFullScreen == TRUE)
	{
        ShowCursor(TRUE);
	}
    // Release the parameter and video resolution lists
    Cleanup();
	AppExit();
    return msg.wParam;
}
   
void GetParameters()
{
   int   count;
   char *s, *tstring;
   
   // Make a copy of the command line to count the parameters - strtok is destructive
   tstring = (char *)malloc(sizeof(char)*(strlen(WinData.szCmdLine)+1));
   strcpy(tstring, WinData.szCmdLine);
   
   // Count the parameters
   s = strtok(tstring, " ");
   count = 1;
   while (strtok(NULL, " ") != NULL)
   {
	   count++;
   }
   free(tstring);
   
   // Allocate "pockets" for the parameters
   WinData.argv = (char **)malloc(sizeof(char*)*(count+1));
   
   // Copy first parameter into the "pockets"
   WinData.argc = 0;
   s = strtok(WinData.szCmdLine, " ");
   WinData.argv[WinData.argc] = (char *)malloc(sizeof(char)*(strlen(s)+1));
   strcpy(WinData.argv[WinData.argc], s);
   WinData.argc++;
   
   // Copy the rest of the parameters
   do
   {
	   // get the next token
	   s = strtok(NULL, " ");
	   if (s != NULL)
       { 
		   // add it to the list
		   WinData.argv[WinData.argc] = (char *)malloc(sizeof(char)*(strlen(s)+1));
		   strcpy(WinData.argv[WinData.argc], s);
		   WinData.argc++;
       }
   }
   while (s != NULL);
}

void Cleanup()
{
   int i;
   
   // Free the command line holder memory
   if (WinData.argc > 0)
   {
	   // Free in reverse order of allocation
	   for (i = (WinData.argc-1); i >= 0; i--)
       {
		   free(WinData.argv[i]);
       }
	   // Free the parameter "pockets"
	   free(WinData.argv);
   }
   // Free the memory that holds the video resolution list
   if (WinData.pResolutions)
	   free(WinData.pResolutions);
}

BOOL isdigits( char *s )
{
   int i;
   
   for (i = 0; s[i]; i++)
   {
	   if ((s[i] > '9') || (s[i] < '0'))
       {
		   return FALSE;
       }
   }
   return TRUE;
}

int FindNumParameter(const char *s)
{
   int i;
   
   for (i = 0; i < (WinData.argc-1); i++)
   {
	   if (stricmp(WinData.argv[i], s) == 0)
       {
		   if (isdigits(WinData.argv[i+1]) == TRUE)
           {
			   return(atoi(WinData.argv[i+1]));
           }
		   else
           {
			   return -1;
           }
       }
   }
   return -1;
}


const char *LastParameter( void )
{
	if ( WinData.argc < 1 )
		return NULL;

	return WinData.argv[WinData.argc-1];
}

bool FindParameter(const char *s)
{
   int i;
   
   for (i = 0; i < WinData.argc; i++)
   {
	   if (stricmp(WinData.argv[i], s) == 0)
       {
		   return true;
       }
   }
   return false;
}

const char *FindParameterArg( const char *s )
{
   int i;
   
   for (i = 0; i < WinData.argc-1; i++)
   {
	   if (stricmp(WinData.argv[i], s) == 0)
       {
		   return WinData.argv[i+1];
       }
   }
   return NULL;
}


BOOL CreateMainWindow(int width, int height, int bpp, BOOL fullscreen)
{
   HWND        hwnd;
   WNDCLASSEX  wndclass;
   DWORD       dwStyle, dwExStyle;
   int         x, y, sx, sy, ex, ey, ty;
   
   if ((hwnd = FindWindow(g_szAppName, g_szAppName)) != NULL)
   {
	   SetForegroundWindow(hwnd);
	   return 0;
   }
   
   wndclass.cbSize        = sizeof (wndclass);
   wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wndclass.lpfnWndProc   = (WNDPROC)WinData.wndproc;
   wndclass.cbClsExtra    = 0;
   wndclass.cbWndExtra    = 0;
   wndclass.hInstance     = WinData.hInstance;
   wndclass.hIcon         = 0;
   wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH)COLOR_GRAYTEXT;
   wndclass.lpszMenuName  = NULL;
   wndclass.lpszClassName = g_szAppName;
   wndclass.hIconSm       = 0;
   
   
   if (!RegisterClassEx (&wndclass))
   {
	   MessageBox(NULL, "Window class registration failed.", "FATAL ERROR", MB_OK);
	   return FALSE;
   }
   
   if (fullscreen)
   {
	   dwExStyle = WS_EX_TOPMOST;
	   dwStyle = WS_POPUP | WS_VISIBLE;
	   x = y = 0;
	   sx = WinData.width;
	   sy = WinData.height;
   }
   else
   {
	   dwExStyle = 0;
	   //dwStyle = WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;  // Use this if you want a "normal" window
	   dwStyle = WS_CAPTION;
	   ex = GetSystemMetrics(SM_CXEDGE);
	   ey = GetSystemMetrics(SM_CYEDGE);
	   ty = GetSystemMetrics(SM_CYSIZE);
	   // Center the window on the screen
	   x = (DevInfo.width / 2) - ((WinData.width+(2*ex)) / 2);
	   y = (DevInfo.height / 2) - ((WinData.height+(2*ey)+ty) / 2);
	   sx = WinData.width+(2*ex);
	   sy = WinData.height+(2*ey)+ty;
	   /*
       Check to be sure the requested window size fits on the screen and
       adjust each dimension to fit if the requested size does not fit.
	   */
	   if (sx >= DevInfo.width)
       {
		   x = 0;
		   sx = DevInfo.width-(2*ex);
       }
	   if (sy >= DevInfo.height)
       {
		   y = 0;
		   sy = DevInfo.height-((2*ey)+ty);
       }
   }
   
   if ((hwnd = CreateWindowEx (dwExStyle,
	   g_szAppName,               // window class name
	   g_szAppName,                // window caption
	   dwStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, // window style
	   x,           // initial x position
	   y,           // initial y position
	   sx,           // initial x size
	   sy,           // initial y size
	   NULL,                    // parent window handle
	   NULL,                    // window menu handle
	   WinData.hInstance,       // program instance handle
	   NULL))                   // creation parameters
	   == NULL)
   {
	   ChangeDisplaySettings(0, 0);
	   MessageBox(NULL, "Window creation failed.", "FATAL ERROR", MB_OK);
	   return FALSE;
   }
   
   WinData.hWnd = hwnd;
   
   if (!InitOpenGL())
   {
	   WinData.hWnd = NULL;
	   return FALSE;
   }
   
   ShowWindow(WinData.hWnd, WinData.iCmdShow);
   UpdateWindow(WinData.hWnd);
   
   SetForegroundWindow(WinData.hWnd);
   SetFocus(WinData.hWnd);
   
   return TRUE;
}

void GL_AppShutdown( void )
{
	SendMessage( WinData.hWnd, WM_CLOSE, 0, 0 );
}

void Error( const char *error, ... )
{
	va_list args;
	char output[1024];

	va_start( args, error );

	vprintf( error, args );
	vsprintf( output, error, args );
    MessageBox(NULL, output, "ERROR", MB_OK);
    WinData.bActive = FALSE;
}


LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
		case WM_CHAR:
			switch(wParam)
			{
			case VK_ESCAPE:
				SendMessage(hwnd, WM_CLOSE, 0, 0);
				break;
			}
			AppChar( wParam );
		break;

		case WM_KEYDOWN:
			AppKey( wParam, true );
			break;
		case WM_KEYUP:
			AppKey( wParam, false );
			break;

		
		case WM_ACTIVATE:
			if ((LOWORD(wParam) != WA_INACTIVE) && ((HWND)lParam == NULL))
			{
				ShowWindow(WinData.hWnd, SW_RESTORE);
				SetForegroundWindow(WinData.hWnd);
			}
			else
			{
				if (WinData.bFullScreen)
				{
					ShowWindow(WinData.hWnd, SW_MINIMIZE);
				}
			}
			return 0;
			
		case WM_SIZE:
			if (WinData.hGLRC)
			{
				// Redefine the viewing volume and viewport when the window size changes.
				WinData.glnWidth = (GLsizei)LOWORD(lParam);
				WinData.glnHeight = (GLsizei)HIWORD(lParam);
				WinData.gldAspect = (float)((GLdouble)WinData.glnWidth/(GLdouble)WinData.glnHeight);
				glViewport( 0, 0, WinData.glnWidth, WinData.glnHeight );
			}
			return 0;
			
		case WM_SETFOCUS:
			WinData.bPaused = FALSE;
			MouseCapture();
	//			if ( WinData.bFullScreen )
	//				SetVideoMode();
			break;

		case WM_KILLFOCUS:
			WinData.bPaused = TRUE;
			MouseRelease();
//			if ( WinData.bFullScreen )
//				ReleaseVideoMode();
			break;

		case WM_CLOSE:
			ShutdownOpenGL();
			WinData.bActive = FALSE;
			break;
			
		case WM_DESTROY:
			PostQuitMessage (0);
			return 0;
	}
   
   return DefWindowProc (hwnd, iMsg, wParam, lParam);
}

// This function builds a list the screen resolutions supported by the display driver
void BuildModeList()
{
   DEVMODE  dm;
   int      mode;
   
   mode = 0;
   while(EnumDisplaySettings(NULL, mode, &dm))
   {
	   mode++;
   }
   
   WinData.pResolutions = (screen_res_t *)malloc(sizeof(screen_res_t)*mode);
   mode = 0;
   while(EnumDisplaySettings(NULL, mode, &dm))
   {
	   WinData.pResolutions[mode].width = dm.dmPelsWidth;
	   WinData.pResolutions[mode].height = dm.dmPelsHeight;
	   WinData.pResolutions[mode].bpp = dm.dmBitsPerPel;
	   WinData.pResolutions[mode].flags = dm.dmDisplayFlags;
	   WinData.pResolutions[mode].frequency = dm.dmDisplayFrequency;
	   mode++;
   }
   WinData.iResCount = mode;
}

bool SetVideoMode()
{
   OSVERSIONINFO   vinfo;
   int             mode;
   DEVMODE         dm;
   
   vinfo.dwOSVersionInfoSize = sizeof(vinfo);
   
   WinData.bChangeBPP = FALSE;
   
   if ( GetVersionEx( &vinfo) )
   {
	   if ( vinfo.dwMajorVersion > 4 )
       {
		   WinData.bChangeBPP = TRUE;
       }
	   else
		   if ( vinfo.dwMajorVersion == 4 )
		   {
			   if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
			   {
				   WinData.bChangeBPP = TRUE;
			   }
			   else
				   if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
				   {
					   if ( LOWORD( vinfo.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
					   {
						   WinData.bChangeBPP = TRUE;
					   }
				   }
		   }
   }
   else
   {
	   MessageBox(NULL, "SetVideoMode - GetVersionEx failed\n", "FATAL ERROR", MB_OK);
	   return false;
   }
   
   if (WinData.bFullScreen)
   {
	   if ((WinData.bpp    == DevInfo.bpp) &&
		   (WinData.width  == DevInfo.width) &&
		   (WinData.height == DevInfo.height))
       {
		   WinData.iVidMode = 0;
		   return true;
       }
	   if ((WinData.bChangeBPP == FALSE) && (DevInfo.bpp != WinData.bpp))
       {
		   MessageBox(NULL, "This version of Windows cannot change color depth.\n"
			   "Please request different video mode settings or adjust\n"
			   "your desktop color depth.", "FATAL ERROR", MB_OK);
		   return false;
       }
	   for (mode = 0; mode < WinData.iResCount; mode++)
       {
		   if ((WinData.pResolutions[mode].width == WinData.width) &&
			   (WinData.pResolutions[mode].height == WinData.height) &&
			   (WinData.pResolutions[mode].bpp == WinData.bpp))
           {
			   WinData.iVidMode = mode;
			   
			   memset(&dm, 0, sizeof(dm));
			   dm.dmSize = sizeof(dm);
			   
			   dm.dmPelsWidth  = WinData.width;
			   dm.dmPelsHeight = WinData.height;
			   dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;
			   
			   if (WinData.bpp != DevInfo.bpp)
               {
				   dm.dmBitsPerPel = WinData.bpp;
				   dm.dmFields |= DM_BITSPERPEL;
               }
			   
			   if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL )
               {
				   MessageBox(NULL, "SetVideoMode - ChangeDisplaySettings failed.\n"
					   "Switching to windowed mode.", "WARNING", MB_OK);
				   WinData.bFullScreen = FALSE;
				   return true;
               }
			   return true;
           }
       }
	   MessageBox(NULL, "Your requested video mode is unavailable.\n"
		   "Please request different video mode settings.", "FATAL ERROR", MB_OK);
	   return false;
   }
   else
   {
	   if (DevInfo.bpp != WinData.bpp)
       {
//		   MessageBox(NULL, "Your requested color depth and desktop do not match.\n"
//			   "Using your current desktop color depth.", "WARNING", MB_OK);
       }
   }
   return true;
}

void ReleaseVideoMode( void )
{
	ChangeDisplaySettings(0, 0);
}


bool InitOpenGL()
{
	int     pfm;   // pixel format
	RECT    rect;
	
	PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),  // size of this pfd
			1,                       // version number
			PFD_DRAW_TO_WINDOW |     // support window
			PFD_SUPPORT_OPENGL |     // support OpenGL
			PFD_DOUBLEBUFFER,        // double buffered
			PFD_TYPE_RGBA,           // RGBA type
			24,                      // 24-bit color depth
			0, 0, 0, 0, 0, 0,        // color bits ignored
			0,                       // no alpha buffer
			0,                       // shift bit ignored
			0,                       // no accumulation buffer
			0, 0, 0, 0,              // accum bits ignored
			16,                      // 16-bit z-buffer      
			0,                       // no stencil buffer
			0,                       // no auxiliary buffer
			PFD_MAIN_PLANE,          // main layer
			0,                       // reserved
			0, 0, 0                  // layer masks ignored
	};
	if ((WinData.hDC = GetDC(WinData.hWnd)) == NULL)
	{
		ChangeDisplaySettings(0, 0);
		MessageBox(NULL, "GetDC on main window failed", "FATAL ERROR", MB_OK);
		return false;
	}
	
	if ((pfm = ChoosePixelFormat(WinData.hDC, &pfd)) == 0)
	{
		ChangeDisplaySettings(0, 0);
		MessageBox(NULL, "ChoosePixelFormat failed\n", "FATAL ERROR", MB_OK);
		return false;
	}
	if (SetPixelFormat(WinData.hDC, pfm, &pfd) == FALSE)
	{
		ChangeDisplaySettings(0, 0);
		MessageBox(NULL, "SetPixelFormat failed\n", "FATAL ERROR", MB_OK);
		return false;
	}
	DescribePixelFormat(WinData.hDC, pfm, sizeof(pfd), &pfd);
	
	// Accelerated generic or MCD?
	if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED)) // Not a generic accelerated driver
	{
		// Check for generic software driver
		if ( pfd.dwFlags & PFD_GENERIC_FORMAT )
		{
			// Software
			if (WinData.bAllowSoft == FALSE)
			{
				ChangeDisplaySettings(0, 0);
				MessageBox(NULL, "OpenGL Driver is not accelerated\n", "FATAL ERROR", MB_OK);
				return false;
			}
		}
		// Must be an accelerated ICD
	}
	
	if ((WinData.hGLRC = wglCreateContext(WinData.hDC)) == 0)
	{
		ChangeDisplaySettings(0, 0);
		MessageBox(NULL, "wglCreateContext failed\n", "FATAL ERROR", MB_OK);
		goto fail;
	}
	
	if (!wglMakeCurrent(WinData.hDC, WinData.hGLRC))
	{
		ChangeDisplaySettings(0, 0);
		MessageBox(NULL, "wglMakeCurrent failed\n", "FATAL ERROR", MB_OK);
		goto fail;
	}
	
	GetClientRect(WinData.hWnd, &rect);
	WinData.glnWidth= rect.right;
	WinData.glnHeight = rect.bottom;
	WinData.gldAspect = (float)((GLdouble)WinData.glnWidth/(GLdouble)WinData.glnHeight);
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	
	return true;
	
fail:
	if ( WinData.hGLRC )
	{
		wglDeleteContext( WinData.hGLRC);
		WinData.hGLRC = NULL;
	}
	
	if ( WinData.hDC )
	{
		ReleaseDC(WinData.hWnd, WinData.hDC);
		WinData.hDC = NULL;
	}
	return false;
}


void ShutdownOpenGL(void)
{
   if (WinData.hGLRC)
   {
	   if ((!wglMakeCurrent(NULL, NULL)) && (!WinData.bFullScreen))
       {
		   MessageBox(NULL, "ShutdownOpenGL - wglMakeCurrent failed\n", "ERROR", MB_OK);
       }
	   if (!wglDeleteContext(WinData.hGLRC))
       {
		   MessageBox(NULL, "ShutdownOpenGL - wglDeleteContext failed\n", "ERROR", MB_OK);
       }
	   WinData.hGLRC = NULL;
   }
   if (WinData.hDC)
   {
	   if (!ReleaseDC(WinData.hWnd, WinData.hDC))
       {
		   MessageBox(NULL, "ShutdownOpenGL - ReleaseDC failed\n", "ERROR", MB_OK);
       }
	   WinData.hDC   = NULL;
   }
   if (WinData.bFullScreen)
   {
	   ChangeDisplaySettings( 0, 0 );
   }
}

void MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		fov_x = 90;	// error, set to 90

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}


unsigned int Sys_Milliseconds( void )
{
    __int64 Frequency, Timer;
    QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);
    QueryPerformanceCounter((LARGE_INTEGER*)&Timer);
    double out = double(Timer)/double(Frequency);

	return (unsigned int) ( out * 1000.0 );
}


// Render your scenes through this function
void RenderScene( void )
{
	static DWORD lastTime = 0;
	POINT cursorPoint;
	float deltax = 0, deltay = 0, frametime;

	DWORD newTime = Sys_Milliseconds();
	DWORD deltaTime = newTime - lastTime;

	if ( deltaTime > 1000 )
		deltaTime = 0;
	lastTime = newTime;
	frametime = (float) ((double)deltaTime * 0.001);
	g_Time = newTime;

	if ( WinData.bPaused )
	{
		deltax = deltay = 0;
	}
	else
	{
		GetCursorPos( &cursorPoint );
		SetCursorPos( WinData.centerx, WinData.centery );

		deltax = (cursorPoint.x - WinData.centerx) * 0.1f;
		deltay = (cursorPoint.y - WinData.centery) * -0.1f;
	}

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	
    MYgluPerspective( CalcFov( WinData.fov, WinData.width, WinData.height ), WinData.gldAspect, WinData.NearClip, WinData.FarClip );

    // set up a projection matrix to fill the client window
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	AppRender( frametime, deltax, deltay );

    if (!SwapBuffers(WinData.hDC))
	{
        ChangeDisplaySettings(0, 0);
        MessageBox(NULL, "RenderScene - SwapBuffers failed!\n", "FATAL ERROR", MB_OK);
        SendMessage(WinData.hWnd, WM_CLOSE, 0, 0);
	}
}

void GetScreen( int &width, int &height )
{
	width = WinData.width;
	height = WinData.height;
}
