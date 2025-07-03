#pragma once

extern "C"
{
#include <libavformat/avformat.h>
}
#include <string>

class IOpenStream
{
public:
	virtual AVFormatContext* openStream(const std::string& url) = 0;
};

