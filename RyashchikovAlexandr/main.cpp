#include "HeapManager.h"
#include "Objects.h"

#include <iostream>
#include <chrono>
#include <random>


const int numCells = 10;
const int numExperiments = 200;

std::vector<int> types;
std::vector<std::pair<int, int>> actions;

myHeapClass* myHeapSample[numCells];
simpleClass* defHeapSample[numCells];

template<class CBase, class CDerived>
void performAction(CBase* data[], int action, int index)
{
	if (action >= 1 && data[index] == nullptr) {
		data[index] = new CDerived;
	}
	else if (action == 0 && data[index] != nullptr) {
		delete data[index];
		data[index] = nullptr;
	}
}

template<class CBase, class CDerived0, class CDerived1, class CDerived2>
void test(CBase* data[])
{
	for (int i = 0; i < numExperiments; ++i) {
		int index = actions[i].first;
		int action = actions[i].second;
		if (types[index] == 0) {
			performAction<CBase, CDerived0>(data, action, index);
		}
		else if (types[index] == 1) {
			performAction<CBase, CDerived1>(data, action, index);
		}
		else {
			performAction<CBase, CDerived2>(data, action, index);
		}
	}
}

int main()
{	
	for (int i = 0; i < numCells; ++i) {
		myHeapSample[i] = nullptr;
		defHeapSample[i] = nullptr;
	}

	std::uniform_int_distribution<int> chooseType(0, 2);
	std::uniform_int_distribution<int> chooseOperation(0, 2);
	std::uniform_int_distribution<int> chooseIndex(0, numCells - 1);
	std::default_random_engine randomize;

	for (int i = 0; i < numCells; ++i) {
		types.push_back(chooseType(randomize));
	}
	for (int i = 0; i < numExperiments; ++i) {
		actions.push_back(std::make_pair<int, int>(chooseIndex(randomize), chooseOperation(randomize)));
	}
	auto start = std::chrono::steady_clock::now();
	test<myHeapClass, small1, average1, big1>(myHeapSample);
	auto finish = std::chrono::steady_clock::now();
	double result1 = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();
	std::cout << "HeapManager time: " << result1 << std::endl;
	
	start = std::chrono::steady_clock::now();
	test<simpleClass, small2, average2, big2>(defHeapSample);
	finish = std::chrono::high_resolution_clock::now();
	double result2 = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();
	std::cout << "StandardHeap time: " << result2 << std::endl;
	std::cout << "ratio: " << result1 / result2 << std::endl;

	char a;
	std::cin >> a;
	return 0;
}
