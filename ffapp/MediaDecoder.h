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
#include "AVFrameQueue.h"

class MediaDecoder
{
public:

	enum State {
		idle,
		wait,
		wait_seek,
		wait_next,
		ready,
		finished
	};

	MediaDecoder(AVFrameQueue& audio, AVFrameQueue& video);
	~MediaDecoder();
	void setSource(const std::string& file);
	void fillFrameBuffer(AVFrame* frame);

	void setOpenStream(IOpenStream* stream);

	uint64_t getTimeStamp(uint64_t pts);

	//int64_t getVideoTimeBase();
	//int64_t getAudioTimeBase();
	
	const AVFormatContext* getFormatContext() const { return _formatContext; }
	const AVCodecParameters* getVideoFormat() { return _formatContext->streams[_videoStreamIndex]->codecpar; }
	const AVCodecParameters* getAudioFormat() { return _formatContext->streams[_audioStreamIndex]->codecpar; }
	

	void seek(uint64_t ms);
	void printfMediaInfo();

	void clearFrames();

	void setOnImageBufferChanged(std::function<void(char*, int, int)> func) { _onImageBufferChanged = func; }
	State getState() const { return _state.load(); }

protected:

	void setState(State state);

	void initSource();
	void releaseSource();
	void threadDecode();

	std::string url;
	std::atomic<State> _state;

	IOpenStream* _openStream = nullptr;

	AVCodecContext* _codecVideo = nullptr;
	AVCodecContext* _codecAudio = nullptr;
	AVFormatContext* _formatContext = nullptr;

	std::function<void(char*, int, int)> _onImageBufferChanged;

	uchar* _buffer = nullptr;

	int _videoStreamIndex = -1;
	int _audioStreamIndex = -1;

	AVFrameQueue& _videoQueue;
	AVFrameQueue& _audioQueue;

	std::unique_ptr<std::thread> _threadDecode;

	//double _videoTimeBase;
	//double _audioTimeBase;
};

