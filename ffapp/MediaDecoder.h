#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
}

#include <string>
#include <QImage>
#include <queue>
#include <mutex>
#include <memory>
#include <Semaphore.h>
#include "IOpenStream.h"

class MediaDecoder
{
public:

	MediaDecoder();
	virtual ~MediaDecoder();

	void start();
	void setSource(const std::string& file);
	QImage frameToImage(AVFrame* frame);

	void setOpenStream(IOpenStream* stream);

	virtual AVFrame* getNextVideoFrame();
	virtual AVFrame* getNextAudioFrame();
	virtual uint64_t getTimeStamp(uint64_t pts);
	
	const AVFormatContext* getFormatContext() const { return _formatContext; }
	virtual const AVCodecParameters* getVideoFormat() { return _formatContext->streams[_videoStreamIndex]->codecpar; }
	virtual const AVCodecParameters* getAudioFormat() { return _formatContext->streams[_audioStreamIndex]->codecpar; }
	

	virtual void seek(uint64_t ms);
	virtual void printfMediaInfo();

	void clearFrames();

protected:

	enum State {
		idle,
		wait,
		ready,
		finished
	} 
	_state = idle;

	virtual void initSource();
	virtual void releaseSource();
	virtual void threadDecode();
	
	IOpenStream* _openStream = nullptr;

	std::string url;

	AVCodecContext* _codecVideo = nullptr;
	AVCodecContext* _codecAudio = nullptr;

	AVFormatContext* _formatContext = nullptr;

	uchar* _buffer = nullptr;

	int _videoStreamIndex = -1;
	int _audioStreamIndex = -1;
	std::mutex _mtxAudio;
	std::mutex _mtxVideo;

	class FrameComparator
	{
	public:
		bool operator()(const AVFrame* a, const AVFrame* b) {
			return a->pts > b->pts;
		}
	};

	std::priority_queue<AVFrame*, std::vector<AVFrame*>, FrameComparator> _pqVideoFrames;
	std::priority_queue<AVFrame*, std::vector<AVFrame*>, FrameComparator> _pqAudioFrames;

	std::unique_ptr<std::thread> _threadDecode;

	Semaphore _sem;
	std::mutex _mtx;
};

