#include "HeapManager.h"
#include "Objects.h"

int main()
{	
	average1* ptr = new average1;
	*(static_cast<int*>(static_cast<void*>(ptr))- sizeof(int)) = 2222222;
	delete ptr;

	char a;
	std::cin >> a;
	return 0;
}
