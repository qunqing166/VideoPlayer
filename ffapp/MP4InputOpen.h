#pragma once

#include "IOpenStream.h"

class MP4InputOpen: public IOpenStream
{
public:
	AVFormatContext* openStream(const std::string& url) override;
};

