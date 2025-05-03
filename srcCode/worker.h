//每一个工人的工作都是一个线程
#include "locks.h"
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <set>

// 定义一个全局的互斥锁，用于保护std::cout的访问
std::mutex cout_mutex;

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

			// 尝试获取写锁（unique_lock）和读锁（shared_lock）
			// 写锁 j 延迟上锁, 只是先封装对象
			std::unique_lock<std::shared_mutex> lock_j(locks.getSharedMutex(j), std::defer_lock);
			std::vector<std::shared_lock<std::shared_mutex>> lock_is;//读锁数组

			bool all_locked = false; //最开始都没有上锁

			while (all_locked == false) //只要不是全部都成功上锁
			{
				all_locked = true;

				if (lock_j.try_lock()) //写锁 j 上锁成功
				{
					for (int idx : indicates) //遍历候选项，将不为 j 的索引上读锁
					{
						if (idx != j)
						{
							lock_is.emplace_back(locks.getSharedMutex(idx));
							if (!lock_is.back().try_lock()) //只要任何一个读锁申请失败
							{
								all_locked = false;

								//释放已经获取的读锁
								for (auto& lock : lock_is)
								{
									if (lock.owns_lock())
									{
										lock.unlock();
									}
								}
								lock_is.clear();
								//释放写锁
								lock_j.unlock();
								break;
							}
						}
					}
					if (all_locked == false) //如果没有全部上锁成功，就重试
					{
						std::this_thread::yield(); // 让出CPU，避免忙等待
					}
				}
			}

			//跳出循环证明获得所有锁
			int sum = locks.getS(i) + locks.getS((i + 1) % N) + locks.getS((i + 2) % N);
			locks.setS(j, sum);

			// 使用互斥锁保护std::cout的访问
			std::lock_guard<std::mutex> lock(cout_mutex);
			std::cout << "Worker " << workerId << ": i=" << i << ", j=" << j << ", Sj=" << sum << std::endl;

			// 所有锁在超出作用域时自动释放
			// 下一次循环会重新申请锁
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception in worker " << workerId << ": " << e.what() << std::endl;
		}
	}
}
