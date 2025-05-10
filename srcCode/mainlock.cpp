#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <fstream>
#include <vector>
#include <random>
#include <fstream>

//不使用面向对象，精简代码

//文件和控制台读写锁
std::mutex foutMutex;

//文件输出流对象
std::ofstream outputfile;


//读写锁和数组的初始化工作
void initSmutexes(const int& N, std::vector<int>& S, std::vector<std::shared_ptr<std::shared_mutex>>& mutexes)
{
	//数组S里面分配随机数
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(0, 1000); // 生成 [0, 1000] 范围的随机整数

	for (int i = 0; i < N; ++i) 
	{
		S[i] = dist(gen);
	}

	//为每个元素添加读写锁，但是还没有上锁
	for (auto& mutex_ptr : mutexes)
	{
		mutex_ptr = std::make_shared<std::shared_mutex>();
	}
}


//根据数组索引返回锁
std::shared_mutex& getMutex(int idx, std::vector<std::shared_ptr<std::shared_mutex>>& mutexes)
{
	return *(mutexes[idx]);
}


//工人线程
void worker(int workerId, std::vector<int>& S, std::vector<std::shared_ptr<std::shared_mutex>>& mutexes, int times, int N)
{
	//随机数
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(0, N - 1);

	for (int t = 0; t < times; t++)
	{
		try
		{
			int i = dist(gen);
			int j = dist(gen);

			//收集所有需要锁定的索引
			std::set<int> indicates = { j, i, (i + 1) % N, (i + 2) % N };
			std::vector<int> sortedIndicates(indicates.begin(), indicates.end());
			std::sort(sortedIndicates.begin(), sortedIndicates.end()); //按照索引的大小来进行上锁

			std::unique_lock<std::shared_mutex> lock_j(getMutex(j, mutexes), std::defer_lock); //写锁
			std::vector<std::shared_lock<std::shared_mutex>> lock_is;//读锁数组

			bool allLocked = false; 

			//等待全部上锁，否则释放锁并回滚
			while (allLocked == false)
			{
				allLocked = true;
				//if(lock_j.owns_lock()) lock_j.unlock();
				//lock_is.clear();

				for (int idx : sortedIndicates)
				{
					if (idx == j) //单独上写锁
					{
						if (lock_j.try_lock() == false)
						{
							allLocked = false;
							break;
						}
					}
					else
					{
						lock_is.emplace_back(getMutex(idx, mutexes), std::defer_lock);

						if (lock_is.back().try_lock() == false)
						{
							allLocked = false;
							break;
						}
					}
				}

				if (allLocked == false) //没有一次性都申请完毕，就释放回滚
				{
					for (auto& lock : lock_is)
					{
						lock.unlock();
					}
					if (lock_j.owns_lock()) lock_j.unlock();
				}
			}

			//操作临界资源
			if (allLocked)
			{
				S[j] = S[i] + S[(i + 1)%N] + S[(i + 2)%N];
			}

			// 使用互斥锁保护std::cout的访问
			std::lock_guard<std::mutex> lock(foutMutex);
			if (outputfile.is_open())
			{
				outputfile << "Worker " << workerId << ": i=" << i << ", j=" << j << ", Sj=" << S[j] << std::endl;
				outputfile.flush(); // 确保立即写入文件
			}

			//释放锁
			for (auto& lock : lock_is)
			{
				lock.unlock();
			}
			if (lock_j.owns_lock()) lock_j.unlock();

		}
		catch (const std::exception& e)
		{
			std::lock_guard<std::mutex> lock(foutMutex);
			outputfile << "Exception in worker " << workerId << ": " << e.what() << std::endl;
			outputfile.flush(); // 确保立即写入文件
		}
	}


}



int main()
{
	const int N = 1e5;
	const int M = 15;
	const int times = 1e4; //循环次数

	std::vector<int> S(N);
	std::vector<std::shared_ptr<std::shared_mutex>> mutexes(N); //数组S对应的读写锁

	//读写锁和数组的初始化工作
	initSmutexes(N, S, mutexes);

	// 打开文件以写入输出
	outputfile.open("outputfile.txt");
	if (!outputfile.is_open())
	{
		std::cerr << "Failed to open output file." << std::endl;
		return 1;
	}

	//创建线程数组
	std::vector<std::thread> threads;

	
	auto start_time = std::chrono::high_resolution_clock::now();

	//启动线程
	for (int w = 0; w < M; w++)
	{
		threads.emplace_back(worker, w, S, mutexes, times, N);
	}

	//等待线程
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
	std::cout << "Program execution time: " << duration.count() << " ms\n";

	std::cout << "\nThe result is saved in the outputfile.txt .\n";
	std::cout << "\nAll completed.\n";


	return 0;
}
