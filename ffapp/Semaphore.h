#pragma once

#include <mutex>
#include <condition_variable>

class Semaphore
{
public:
	Semaphore(int num):_num(num) {}
	void wait()
	{
		std::unique_lock<std::mutex> lock(_mtx);
		_cv.wait(lock, [=]() {return _num > 0; });
		--_num;
	}

	void signal()
	{
		std::unique_lock<std::mutex> lock(_mtx);
		++_num;
		_cv.notify_one();
	}
private:
	int _num;
	std::mutex _mtx;
	std::condition_variable _cv;
};

