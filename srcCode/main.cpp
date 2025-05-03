
#include "locks.h"
#include "worker.h"
#include <iostream>
#include <thread>
#include <vector>

int main()
{
	//const int N = 100000; //数组长度是1e5
	const int N = 100;
	const int M = 10; //假设最开始有10个工人
	const int times = 100; //每个工人重复的次数

	Locks locks(N);
	locks.initS();

	std::vector<std::thread> threads; //每个线程里面都是一个工人在工作

	for (int i = 0; i < M; i++)
	{
		threads.emplace_back(worker, i, std::ref(locks), times, N);
	}

	for (auto& thread : threads)
	{
		if (thread.joinable()) 
		{
			thread.join();
		}
	}

	std::cout << "\nAll completed.\n";

	return 0;
}