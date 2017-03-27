//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef SND_ASM_I386_H
#define SND_ASM_I386_H

#ifdef ELF
#define C(label) label
#else
#define C(label) _##label
#endif

// portable_samplepair_t structure
// !!! if this is changed, it much be changed in sound.h too !!!
#define psp_left		0
#define psp_right		4
#define psp_size		8

#endif // SND_ASM_I386_H
