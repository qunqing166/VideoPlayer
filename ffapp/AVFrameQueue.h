#pragma once

extern "C" 
{
#include <libavformat/avformat.h>
}

#include <queue>
#include <mutex>
#include "Semaphore.h"

class AVFrameQueue
{
public:
	AVFrameQueue(Semaphore& sem):
		_sem(sem)
	{
		
	}

	~AVFrameQueue()
	{
		realseAll();
	}

	void push(AVFrame* frame)
	{
		_sem.wait();
		std::lock_guard<std::mutex> lock(_mtx);
		_queue.push(frame);
	}

	AVFrame* front()
	{
		std::lock_guard<std::mutex> lock(_mtx);
		if (_queue.empty())
		{
			return nullptr;
		}
		AVFrame* tmp = _queue.top();
		_queue.pop();
		_sem.signal();
		return tmp;
	}

	void realseAll()
	{
		std::lock_guard<std::mutex> lock(_mtx);
		while (!_queue.empty())
		{
			auto f = _queue.top();
			av_frame_free(&f);
			_queue.pop();
			_sem.signal();
		}
	}

private:
	class FrameComparator
	{
	public:
		bool operator()(const AVFrame* a, const AVFrame* b) {
			return a->pts > b->pts;
		}
	};

	std::priority_queue<AVFrame*, std::vector<AVFrame*>, FrameComparator> _queue;

	std::mutex _mtx;
	Semaphore& _sem;
};

