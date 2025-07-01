#pragma once

#include <chrono>
#include "spdlog/spdlog.h"

class RunningTime
{
public:
	RunningTime()
	{
		_start = std::chrono::high_resolution_clock::now();
	}
	RunningTime(const std::string& msg)
	{
		_msg = msg;
		_start = std::chrono::high_resolution_clock::now();
	}
	~RunningTime()
	{
		_end = std::chrono::high_resolution_clock::now();
		auto micro = std::chrono::duration_cast<std::chrono::microseconds>(_end - _start).count();
		spdlog::info("{} : use time: {} ms", _msg, micro);
	}

private:
	std::chrono::steady_clock::time_point _start;
	std::chrono::steady_clock::time_point _end;
	std::string _msg;
};

