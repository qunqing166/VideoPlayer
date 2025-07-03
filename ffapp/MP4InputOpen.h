#pragma once

#include "IOpenStream.h"

class MP4InputOpen: public IOpenStream
{
public:
	// Í¨¹ý IOpenStream ¼Ì³Ð
	AVFormatContext* openStream(const std::string& url) override;
};

