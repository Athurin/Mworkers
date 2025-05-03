//每一个工人的工作都是一个线程
#include "locks.h"
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <fstream>

// 定义一个全局的互斥锁，用于保护std::cout的访问
std::mutex cout_mutex;

// 定义一个全局的文件输出流
std::ofstream output_file;


void worker(int workerId, Locks& locks, int times, int N)
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


			std::vector<std::unique_lock<std::shared_mutex>> lock_j; //写锁
			std::vector<std::shared_lock<std::shared_mutex>> lock_is;//读锁数组

			bool all_locked = false; //最开始都没有上锁

			while (all_locked == false) //只要不是全部都成功上锁
			{
				all_locked = true;
				lock_j.clear();
				lock_is.clear();

				for (int idx : sortedIndicates)
				{
					if (idx == j)
					{
						lock_j.emplace_back(locks.getSharedMutex(idx), std::defer_lock);
						if (!lock_j.back().try_lock()) //写锁没有成功
						{
							all_locked = false;
							break;
						}
					}
					else
					{
						lock_is.emplace_back(locks.getSharedMutex(idx), std::defer_lock);
						if (!lock_is.back().try_lock()) //读锁有一个没有成功
						{
							all_locked = false;
							break;
						}
					}
				}

				if (!all_locked)
				{
					// 释放已获取的锁
					for (auto& lock : lock_j)
					{
						if (lock.owns_lock())
						{
							lock.unlock();
						}
					}
					for (auto& lock : lock_is)
					{
						if (lock.owns_lock())
						{
							lock.unlock();
						}
					}
					std::this_thread::yield();
				}
			}

			//跳出循环证明获得所有锁
			int sum = locks.getS(i) + locks.getS((i + 1) % N) + locks.getS((i + 2) % N);
			locks.setS(j, sum);

			// 使用互斥锁保护std::cout的访问
			std::lock_guard<std::mutex> lock(cout_mutex);
			//std::cout << "Worker " << workerId << ": i=" << i << ", j=" << j << ", Sj=" << sum << std::endl;
			// 同时将输出写入文件
            if (output_file.is_open()) 
			{
                output_file << "Worker " << workerId << ": i=" << i << ", j=" << j << ", Sj=" << sum << std::endl;
                output_file.flush(); // 确保立即写入文件
            }

			// 释放所有锁
			for (auto& lock : lock_j)
			{
				if (lock.owns_lock())
				{
					lock.unlock();
				}
			}
			for (auto& lock : lock_is)
			{
				if (lock.owns_lock())
				{
					lock.unlock();
				}
			}
		}
		catch (const std::exception& e)
		{
			std::lock_guard<std::mutex> lock(cout_mutex);
			output_file << "Exception in worker " << workerId << ": " << e.what() << std::endl;
			output_file.flush(); // 确保立即写入文件
		}
	}
}
