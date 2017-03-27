
#ifndef __TOUCHLIST_H__
#define __TOUCHLIST_H__

#include "cmodel.h"  // trace_t

#define MAX_TOUCHLIST_ENTS	600

class TouchList
{
public:
	// results, tallied on client and server, but only used by server to run SV_Impact.
	// we store off our velocity in the trace_t structure so that we can determine results
	//  of shoving boxes etc. around.
	int		   numtouch;
	trace_t  touchindex[MAX_TOUCHLIST_ENTS];
};

#endif


