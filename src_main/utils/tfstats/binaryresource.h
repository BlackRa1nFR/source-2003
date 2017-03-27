//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef BINARYRESOURCE_H
#define BINARYRESOURCE_H
#ifdef WIN32
#pragma once
#endif
#include <string>
#include <stdio.h>
#include "util.h"

class CBinaryResource
{
private:
	std::string filename;
	size_t numBytes;
	unsigned char* pData;
public:
	CBinaryResource(char* name, size_t bytes,unsigned char* data)
	:filename(name),numBytes(bytes),pData(data)
	{}
	
	bool writeOut()
	{
		FILE* f=fopen(filename.c_str(),"wb");
		if (!f)
			return false;
		fwrite(pData,1,numBytes,f);
		fclose(f);
#ifndef WIN32
		chmod(filename.c_str(),PERMIT);
#endif
		return true;
	}
};

#endif // BINARYRESOURCE_H

