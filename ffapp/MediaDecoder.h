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

	enum State {
		idle,
		wait,
		wait_next,
		ready,
		finished
	};

	MediaDecoder();
	~MediaDecoder();

	void start();
	void setSource(const std::string& file);
	void fillFrameBuffer(AVFrame* frame);

	void setOpenStream(IOpenStream* stream);

	AVFrame* getNextVideoFrame();
	AVFrame* getNextAudioFrame();
	uint64_t getTimeStamp(uint64_t pts);
	
	const AVFormatContext* getFormatContext() const { return _formatContext; }
	const AVCodecParameters* getVideoFormat() { return _formatContext->streams[_videoStreamIndex]->codecpar; }
	const AVCodecParameters* getAudioFormat() { return _formatContext->streams[_audioStreamIndex]->codecpar; }
	

	void seek(uint64_t ms);
	void printfMediaInfo();

	void clearFrames();

	void setOnImageBufferChanged(std::function<void(char*, int, int)> func) { _onImageBufferChanged = func; }
	State getState() const { return _state.load(); }

protected:
	std::atomic<State> _state;

	void setState(State state);
	

	void initSource();
	void releaseSource();
	void threadDecode();
	
	IOpenStream* _openStream = nullptr;

	std::string url;

	AVCodecContext* _codecVideo = nullptr;
	AVCodecContext* _codecAudio = nullptr;

	AVFormatContext* _formatContext = nullptr;

	std::function<void(char*, int, int)> _onImageBufferChanged;

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
	//std::mutex _mtx;
	Semaphore _sem;
	bool isVideoThreadWaitting = false;
};

