//管理每一个数组元素对应的锁
#ifndef __LOCKS_H__
#define __LOCKS_H__

#include <mutex>
#include <vector>
#include <random>
#include <shared_mutex>

class Locks
{
private:
	int m_N; //数组的长度
	std::vector<int> m_S; //数组的内容，内容是后续生成的随机数
	//std::vector<std::shared_mutex> m_mutexes; //m_S对应的读写锁
	std::vector<std::shared_ptr<std::shared_mutex>> m_mutexes;
	//shared_mutex的独占模式和共享模式的切换要注意

public:
	Locks():m_N(0) ,m_S(0), m_mutexes(0) {}
	Locks(int size) :
		m_N(size),
		m_S(size),
		m_mutexes(size)
	{
		for (auto& mutex_ptr : m_mutexes) {
			mutex_ptr = std::make_shared<std::shared_mutex>();
		}
	}

	void initS() //为数组生成随机的内容，初始化数组
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dist(0, 1000); // 生成 [0, 1000] 范围的随机整数

		for (int i = 0; i < m_N; ++i) {
			m_S[i] = dist(gen);
		}
	}

	//根据数组索引返回锁的引用
	std::shared_mutex& getSharedMutex(int index)
	{
		return *(m_mutexes[index]);
	}

	//根据下标返回S的值
	int getS(int index)
	{
		return m_S[index];
	}

	//根据下标修改S的值
	void setS(int index, int value)
	{
		m_S[index] = value;
	}

};

#endif