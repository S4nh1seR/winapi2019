#pragma once

#include "HeapManager.h"

class myHeapClass {

public:
	void* operator new(std::size_t size);
	void operator delete(void* mem);
};

class small1 : public myHeapClass {
private:
	int array[10];
};

class average1 : public myHeapClass {
private:
	int array[100];
};

class big1 : public myHeapClass {
private:
	int array[1000];
};

class simpleClass {

};

class small2 : public simpleClass {
private:
	int array[10];
};

class average2 : public simpleClass {
private:
	int array[100];
};

class big2 : public simpleClass {
private:
	int array[1000];
};