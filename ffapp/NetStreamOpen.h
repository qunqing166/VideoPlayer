#pragma once

#include "IOpenStream.h"

class NetStreamOpen: public IOpenStream
{
public:
	AVFormatContext* openStream(const std::string& url) override;

};

