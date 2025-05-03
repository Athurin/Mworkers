
#include "locks.h"
#include "worker.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

int main()
{
	//const int N = 100000; //数组长度是1e5
	const int N = 100000;
	const int M = 15; //假设最开始有15个工人
	const int times = 10000; //每个工人重复的次数

	Locks locks(N);
	locks.initS();

	std::vector<std::thread> threads; //每个线程里面都是一个工人在工作

	// 打开文件以写入输出
	output_file.open("output.txt");
	if (!output_file.is_open()) 
	{
		std::cerr << "Failed to open output file." << std::endl;
		return 1;
	}

	auto start_time = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < M; i++)
	{
		threads.emplace_back(worker, i, std::ref(locks), times, N);
	}

	std::cout << "\nWaiting for a second... ...\n";

	for (auto& thread : threads)
	{
		if (thread.joinable()) 
		{
			thread.join();
		}
		std::cout << "\n...\n";
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
	std::cout << "Program execution time: " << duration.count() << " milliseconds\n";

	std::cout << "\nThe result is saved in the output.txt .\n";
	std::cout << "\nAll completed.\n";

	return 0;
}