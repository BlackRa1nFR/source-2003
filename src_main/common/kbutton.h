#if !defined( KBUTTON_H )
#define KBUTTON_H
#ifdef _WIN32
#pragma once
#endif

typedef struct kbutton_s
{
	// key nums holding it down
	int		down[2];		
	// low bit is down state
	int		state;			
} kbutton_t;

#endif // KBUTTON_H