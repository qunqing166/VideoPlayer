#pragma once

#include "IOpenStream.h"

class NetStreamOpen: public IOpenStream
{
public:


	// ͨ�� IOpenStream �̳�
	AVFormatContext* openStream(const std::string& url) override;

};

