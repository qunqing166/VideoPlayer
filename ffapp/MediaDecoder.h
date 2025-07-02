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

class MediaDecoder
{
public:
	MediaDecoder();
	virtual ~MediaDecoder();

	void start();
	//void stop();

	void setSource(const std::string& file);
	virtual AVFrame* getNextVideoFrame() = 0;
	virtual AVFrame* getNextAudioFrame() = 0;
	QImage frameToImage(AVFrame* frame);
	const AVFormatContext* getFormatContext() const { return _formatContext; }
	virtual const AVCodecParameters* getVideoFormat() { return nullptr; }
	virtual const AVCodecParameters* getAudioFormat() { return nullptr; }
	virtual uint64_t getTimeStamp(uint64_t pts) { return -1; }

	virtual void seek(uint64_t ms) = 0;


protected:

	enum State {
		idle,
		wait,
		ready,
		finished
	} 
	_state = idle;

	virtual void initSource() = 0;
	virtual void releaseSource() = 0;
	virtual void threadDecode() = 0;
	

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

	std::unique_ptr<std::thread*> _threadDecode;

};

