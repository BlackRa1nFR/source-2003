//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "../common/DebugConsole_Interface.h"
#include "../common/winlite.h"

#include <stdio.h>
#include <stdarg.h>

//-----------------------------------------------------------------------------
// Purpose: Simple console implementation
//-----------------------------------------------------------------------------
class CWinConsoleDebugConsole : public IDebugConsole
{
private:
	HANDLE m_hOutput;
	HANDLE m_hInput;

	enum { BUFFER_SIZE = 256 };

	char m_szInputBuffer[BUFFER_SIZE+1];
	int m_iBufPos;

public:
	void Initialize( const char *consoleName )
	{
		::AllocConsole();
		m_hOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
		m_hInput = ::GetStdHandle(STD_INPUT_HANDLE);

		if ( consoleName )
		{
			::SetConsoleTitle( consoleName );
		}
		m_iBufPos = 0;
	}

	void Print( int severity, const char *msgDescriptor, ... )
	{
		if (severity < 3)
			return;

		static char szBuf[2048];
		va_list marker;
		va_start( marker, msgDescriptor );
		vsprintf( szBuf, msgDescriptor, marker );
		va_end( marker );

		DWORD charsWritten;
		::WriteConsole( m_hOutput, szBuf, strlen(szBuf), &charsWritten, NULL );

		// output to file
		FILE *f = fopen("log.txt", "at");
		if (f)
		{
			fputs(szBuf, f);
			fclose(f);
		}
	}

	bool GetInput(char *buffer, int bufferSize)
	{
		buffer[0] = 0;
		unsigned long numRead = 0, numEvents = 0;
		INPUT_RECORD recordBuffer;

		while (1)
		{
			// check for messages
			::GetNumberOfConsoleInputEvents(m_hInput, &numEvents);
			if (!numEvents)
				break;

			// read the console input
			::ReadConsoleInput(m_hInput, &recordBuffer, 1, &numRead);
			if (!numRead)
				break;

			if (recordBuffer.EventType == KEY_EVENT && recordBuffer.Event.KeyEvent.bKeyDown)
			{
				char ch = recordBuffer.Event.KeyEvent.uChar.AsciiChar;

				if (ch == '\n' || ch == '\r')
				{
					// enter has been pressed, so package up the input and send it home
					m_szInputBuffer[m_iBufPos] = 0;

					strncpy(buffer, m_szInputBuffer, min(m_iBufPos, bufferSize));
					buffer[min(m_iBufPos, bufferSize)] = 0;

					// reset the buffer
					m_iBufPos = 0;

					// echo
					Print(10, "\n");

					return true;					
				}
				else if (isalnum(ch))
				{
					m_szInputBuffer[m_iBufPos] = ch;
					m_iBufPos++;

					// echo
					Print(10, "%c", ch);
				}
			}
		}

		return false;
	}

	~CWinConsoleDebugConsole()
	{
		::FreeConsole();
	}
};

EXPOSE_SINGLE_INTERFACE(CWinConsoleDebugConsole, IDebugConsole, DEBUGCONSOLE_INTERFACE_VERSION);
