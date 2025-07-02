#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
}

#include "MediaDecoder.h"
#include "Semaphore.h"

class MP4Decoder : public MediaDecoder
{
public:
	MP4Decoder();
	~MP4Decoder();

	void setSource(const QString& file);
	double getTimeBase();

	void seek(uint64_t ms) override;

	const AVCodecParameters* getVideoFormat() override;
	const AVCodecParameters* getAudioFormat() override;
	uint64_t getTimeStamp(uint64_t pts) override;
	void printfMediaInfo() override;

protected:
	void initSource() override;
	void releaseSource() override;
	void threadDecode() override;
	AVFrame* getNextVideoFrame() override;
	AVFrame* getNextAudioFrame() override;

private:

	void clearFrames();
	Semaphore _sem;
	std::mutex _mtx;
};

