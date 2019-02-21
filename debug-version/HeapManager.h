#pragma once

#include <map>
#include <set>
#include <unordered_map>
#include <vector>
#include <Windows.h>

class CHeapManager {
public:
	CHeapManager(int minSize, int maxSize);
	~CHeapManager();

	void* Alloc(int size);
	void Free(void* ptr);

	void showAllocated();

private:
	void* reservedMemPtr;
	
	int reservedMemSize;
	int commitedMemSize;

	std::set<std::pair<int, void*>> reservedBlocksBySize;
	std::set<std::pair<int, void*>> commitedBlocksBySize;

	std::map<void*, int> reservedBlocksByPtr;
	std::map<void*, int> commitedBlocksByPtr;

	std::unordered_map<void*, std::pair<int, int>> allocatedBlocks;

	std::vector<int> pages;

	static const int pageSize = 4096;
	static const int minBlockSize = 4;
	static const int maxBlocksOnPage = pageSize / minBlockSize;
	static const int specialValue = -1;

	void initReserve();
	void initCommit();

	void* movePtr(void* ptr, int shift) const;
	int roundToMultiple(int value, int divider) const;

	int getStartPageIndex(void* ptr);
	int getEndPageIndex(void* ptr, int size);
	
	void insertCommitedBlocks(int size, void* ptr);
	void insertReservedBlocks(int size, void* ptr);

	void pushRemainBlock(int realSize, std::pair<int, void*> foundBlock);
	void mergeBlockPtr(void*& blockPtr, int blockSize);
	void decommitMemory(void* blockPtr, int blockSize);
	void checkBorders(void* blockPtr, int blockSize);
};