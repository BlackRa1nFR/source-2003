#if !defined( KBUTTONH )
#define KBUTTONH
#ifdef _WIN32
#pragma once
#endif

typedef struct kbutton_s
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

#endif // !KBUTTONH