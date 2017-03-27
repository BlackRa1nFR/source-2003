
#include <malloc.h>
#include "vallocator.h"
#include "basetypes.h"


VStdAllocator g_StdAllocator;


void* VStdAllocator::Alloc(unsigned long size)
{
	if(size)
	{
		void *ret = malloc(size);
		return ret;
	}
	else
		return 0;
}


void VStdAllocator::Free(void *ptr)
{
	free(ptr);
}

