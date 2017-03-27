
#ifndef _BLOCKARRAY_H
#define _BLOCKARRAY_H

template <class T, int nBlockSize, int nMaxBlocks>
class BlockArray
{
public:
	BlockArray()
	{
		nCount = nBlocks = 0;
	}
	~BlockArray()
	{
		GetBlocks(0);
	}

	T& operator[] (int iIndex);
	T& GetAt(int iIndex);
	
	void SetCount(int nObjects);
	int GetCount() { return nCount; }

private:
	T * Blocks[nMaxBlocks+1];
	short nCount;
	short nBlocks;
	void GetBlocks(int nNewBlocks);
};

template <class T, int nBlockSize, int nMaxBlocks>
void BlockArray<T,nBlockSize,nMaxBlocks>::
	GetBlocks(int nNewBlocks)
{
	for(int i = nBlocks; i < nNewBlocks; i++)
	{
		Blocks[i] = new T[nBlockSize];
	}
	for(i = nNewBlocks; i < nBlocks; i++)
	{
		delete[] Blocks[i];
	}

	nBlocks = nNewBlocks;
}

template <class T, int nBlockSize, int nMaxBlocks>
void BlockArray<T,nBlockSize,nMaxBlocks>::
	SetCount(int nObjects)
{
	if(nObjects == nCount)
		return;

	// find the number of blocks required by nObjects, checking for
	// integer rounding error
	int nNewBlocks = (nObjects / nBlockSize);
	if ((nNewBlocks * nBlockSize) < nObjects)
	{
		nNewBlocks++;
	}

	if(nNewBlocks != nBlocks)
		GetBlocks(nNewBlocks);
	nCount = nObjects;
}

template <class T, int nBlockSize, int nMaxBlocks>
T& BlockArray<T,nBlockSize,nMaxBlocks>::operator[] (int iIndex)
{
	if(iIndex >= nCount)
		SetCount(iIndex+1);
	return Blocks[iIndex / nBlockSize][iIndex % nBlockSize];
}

template <class T, int nBlockSize, int nMaxBlocks>
T& BlockArray<T,nBlockSize,nMaxBlocks>::GetAt(int iIndex)
{
	if(iIndex >= nCount)
		SetCount(iIndex+1);
	return Blocks[iIndex / nBlockSize][iIndex % nBlockSize];
}
#endif // _BLOCKARRAY_H