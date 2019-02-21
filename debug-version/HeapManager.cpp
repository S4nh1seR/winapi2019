
#include <assert.h>
#include <iostream>
#include "HeapManager.h"

CHeapManager::CHeapManager(int minSize, int maxSize)
{
	assert(minSize >= 0 && maxSize >= 0);
	assert(minSize <= maxSize);

	reservedMemSize = roundToMultiple(maxSize, pageSize);
	commitedMemSize = roundToMultiple(minSize, pageSize);

	initReserve();
	initCommit();
}

CHeapManager::~CHeapManager()
{
	assert(!VirtualFree(reservedMemPtr, 0, MEM_RESET));

	for (auto allocatedBlock : allocatedBlocks) {
		std::cout << "address: [" << allocatedBlock.first << "] size: " << allocatedBlock.second.second << std::endl;
	}
}

void CHeapManager::initReserve()
{
	reservedMemPtr = VirtualAlloc(nullptr, reservedMemSize, MEM_RESERVE, PAGE_READWRITE);
	assert(reservedMemPtr);
	pages.assign(reservedMemSize / pageSize, 0);
}

void CHeapManager::initCommit()
{
	LPVOID commitedMemPtr = VirtualAlloc(reservedMemPtr, commitedMemSize, MEM_COMMIT, PAGE_READWRITE);
	assert(commitedMemPtr);
	assert(commitedMemPtr == reservedMemPtr);

	int remainderSize = reservedMemSize - commitedMemSize;
	void* remainderPtr = movePtr(commitedMemPtr, commitedMemSize);

	insertCommitedBlocks(commitedMemSize, commitedMemPtr);
	insertReservedBlocks(remainderSize, remainderPtr);
}


void* CHeapManager::Alloc(int size)
{
	int realSize = roundToMultiple(size + 2 * sizeof(int), minBlockSize);
	std::pair<int, void*> foundBlock = std::make_pair(0, nullptr);
	
	auto setIterator = commitedBlocksBySize.lower_bound(std::make_pair(realSize, nullptr));
	bool foundCommitedBlock = (setIterator != commitedBlocksBySize.end());
	
	if (foundCommitedBlock) {
		foundBlock = *setIterator;

		commitedBlocksBySize.erase(setIterator);
		commitedBlocksByPtr.erase(foundBlock.second);
		pushRemainBlock(realSize, foundBlock);
	}
	else {
		setIterator = reservedBlocksBySize.lower_bound(std::make_pair(realSize, nullptr));
		assert(setIterator != reservedBlocksBySize.end());
		foundBlock = *setIterator;

		reservedBlocksBySize.erase(setIterator);
		reservedBlocksByPtr.erase(foundBlock.second);
		pushRemainBlock(realSize, foundBlock);

		LPVOID commitedMemPtr = VirtualAlloc(foundBlock.second, roundToMultiple(realSize, pageSize), MEM_COMMIT, PAGE_READWRITE);
		assert(commitedMemPtr);
	}

	pages[getStartPageIndex(foundBlock.second)]++;
	pages[getEndPageIndex(foundBlock.second, realSize)]++;

	allocatedBlocks[foundBlock.second] = std::make_pair(realSize, size);

	*static_cast<int*>(foundBlock.second) = specialValue;
	*static_cast<int*>(movePtr(foundBlock.second, sizeof(int) + size)) = specialValue;
	return movePtr(foundBlock.second, sizeof(int));
}
void CHeapManager::showAllocated() {
	for (auto it = allocatedBlocks.begin(); it != allocatedBlocks.end(); ++it) {
		std::cout << it->first << " " << it->second.first << " " << it->second.second << std::endl;
	}
}
void CHeapManager::pushRemainBlock(int realSize, std::pair<int, void*> foundBlock)
{
	int remainderSize = foundBlock.first - realSize;
	if (remainderSize > 0) {
		void* remainderPtr = movePtr(foundBlock.second, realSize);
		insertReservedBlocks(remainderSize, remainderPtr);
	}
}

void CHeapManager::insertCommitedBlocks(int size, void* ptr)
{
	commitedBlocksBySize.insert(std::make_pair(size, ptr));
	commitedBlocksByPtr[ptr] = size;
}

void CHeapManager::insertReservedBlocks(int size, void* ptr)
{
	reservedBlocksBySize.insert(std::make_pair(size, ptr));
	reservedBlocksByPtr[ptr] = size;
}

void CHeapManager::Free(void* blockPtr)
{
	blockPtr = movePtr(blockPtr, -static_cast<int>(sizeof(int)));
	auto hashIterator = allocatedBlocks.find(blockPtr);
	assert(hashIterator != allocatedBlocks.end());
	
	int realBlockSize = hashIterator->second.first;
	int blockSize = hashIterator->second.second;

	checkBorders(blockPtr, blockSize);
	pages[getStartPageIndex(blockPtr)]--;
	pages[getEndPageIndex(blockPtr, realBlockSize)]--;
	allocatedBlocks.erase(hashIterator);

	mergeBlockPtr(blockPtr, realBlockSize);
	decommitMemory(blockPtr, realBlockSize);
}

void CHeapManager::checkBorders(void* blockPtr, int blockSize)
{
	if (*static_cast<int*>(blockPtr) != specialValue || *static_cast<int*>(movePtr(blockPtr, sizeof(int) + blockSize)) != specialValue) {

		
		std::cout << "Memory bound error! address: " << blockPtr << " " << __FILE__ << " " << __LINE__ << std::endl;
	}
}

void CHeapManager::mergeBlockPtr(void*& blockPtr, int blockSize)
{
	auto rightCommitedIter = commitedBlocksByPtr.upper_bound(blockPtr);
	if (rightCommitedIter != commitedBlocksByPtr.end() && rightCommitedIter->first == movePtr(blockPtr, blockSize)) {
		void* rightBlockPtr = rightCommitedIter->first;
		int rightBlockSize = rightCommitedIter->second;
		blockSize += rightBlockSize;
		rightCommitedIter = commitedBlocksByPtr.erase(rightCommitedIter);
		commitedBlocksBySize.erase(std::make_pair(rightBlockSize, rightBlockPtr));
	}
	if (rightCommitedIter != commitedBlocksByPtr.begin()) {
		auto leftCommitedIter = --rightCommitedIter;
		void* leftBlockPtr = leftCommitedIter->first;
		int leftBlockSize = leftCommitedIter->second;
		if (blockPtr == movePtr(leftBlockPtr, leftBlockSize)) {
			blockSize += leftBlockSize;
			blockPtr = leftBlockPtr;
			commitedBlocksByPtr.erase(leftBlockPtr);
			commitedBlocksBySize.erase(std::make_pair(leftBlockSize, leftBlockPtr));
		}
	}
}
void CHeapManager::decommitMemory(void* blockPtr, int blockSize)
{
	int startIndex = getStartPageIndex(blockPtr);
	int endIndex = getEndPageIndex(blockPtr, blockSize);
	startIndex += (pages[startIndex] > 0);
	endIndex -= (pages[endIndex] > 0);

	if (endIndex >= startIndex) {
		void* decommitPtr = movePtr(reservedMemPtr, startIndex * pageSize);
		int decommitSize = (endIndex - startIndex + 1) * pageSize;
		VirtualFree(decommitPtr, decommitSize, MEM_DECOMMIT);
		
		reservedBlocksBySize.insert(std::make_pair(decommitSize, decommitPtr));

		int leftBlockSize = 0;
		if (getStartPageIndex(blockPtr) != startIndex) {
			leftBlockSize = static_cast<int*>(decommitPtr) - static_cast<int*>(blockPtr);
			insertCommitedBlocks(leftBlockSize, blockPtr);
		}
		if (getEndPageIndex(blockPtr, blockSize) != endIndex) {
			int rightBlockSize = blockSize - decommitSize - leftBlockSize;
			void* rightBlockPtr = movePtr(decommitPtr, decommitSize);
			insertCommitedBlocks(rightBlockSize, rightBlockPtr);
		}
	}
	else {
		insertCommitedBlocks(blockSize, blockPtr);
	}
}
int CHeapManager::getStartPageIndex(void* ptr)
{
	return (static_cast<int*>(ptr) - static_cast<int*>(reservedMemPtr)) / pageSize;
}

int CHeapManager::getEndPageIndex(void* ptr, int size)
{
	return (static_cast<int*>(ptr) + (size - 1) - static_cast<int*>(reservedMemPtr)) / pageSize;
}

void* CHeapManager::movePtr(void* ptr, int shift) const
{
	return static_cast<void*>(shift + static_cast<int*>(ptr));
}

int CHeapManager::roundToMultiple(int value, int divider) const
{
	int remainder = value % divider;
	int result = value - remainder;
	if (value == 0 || remainder != 0) {
		result += divider;
	}
	return result;
}
