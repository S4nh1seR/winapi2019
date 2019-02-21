#include "Objects.h"

int minSize = 1024 * 32;
int maxSize = 500 * 1024 * 1024;

CHeapManager heapManager(minSize, maxSize);

void* myHeapClass::operator new(size_t size) {
	void* result = heapManager.Alloc(size);
	//heapManager.showAllocated();
	return result;
}
void myHeapClass::operator delete(void* ptr) {
	heapManager.Free(ptr);
}
