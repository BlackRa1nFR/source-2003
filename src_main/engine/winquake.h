// winquake.h: Win32-specific Quake header file
#ifndef WINQUAKE_H
#define WINQUAKE_H
#pragma once

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "quakedef.h"

typedef int qboolean;

#define CINTERFACE

extern qboolean	Win32AtLeastV4;

#endif // _WIN32
#endif // WINQUAKE_H