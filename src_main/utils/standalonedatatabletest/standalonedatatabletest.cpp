// DATATABLETEST_ERASEME.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "quakedef.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "vector.h"
#include "datatable_test.h"


float vec3_origin[3] = {0,0,0};

void Con_Printf(char *msg, ...)
{
	va_list marker;

	va_start(marker, msg);
	vprintf(msg, marker);
	va_end(marker);
}

void Con_DPrintf(char *msg, ...)
{
	va_list marker;

	va_start(marker, msg);
	vprintf(msg, marker);
	va_end(marker);
}

int main(int argc, char* argv[])
{
	// Seed the random number generator?
	unsigned srandNum = 0xFFFF;
	if(argc > 1)
	{
		if(stricmp(argv[1], "-seed") == 0)
		{
			srandNum = time(0);
			srand(srandNum);
		}
	}

	RunDataTableTest();
	return 0;
}
