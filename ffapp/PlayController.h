#pragma once

#include <QObject>
#include <thread>
#include <QAudioSink>
#include <QIODevice>
#include <QElapsedTimer>
#include <memory>
#include "SDL2/SDL.h"
#include "MediaDecoder.h"
#include <functional>
#include "AVFrameQueue.h"

class PlayController: public QObject
{
	Q_OBJECT

public:

	enum State
	{
		idle,
		running,
		pause,
		stop,
		finished
	};

	enum StreamType
	{
		MP4,
		NetStream
	};

	PlayController(QObject* parent = nullptr);
	~PlayController();

	bool setSource(const QString& url);
	bool setSource(const std::string& url, StreamType type);
	

	/**
	 * @brief 将进度移动到指定位置
	 * @para position 范围0~1
	 */
	void seek(double position);
	void setState(PlayController::State state);
	const State getState() { return _state; }
	const MediaDecoder* getDecode() { return _decode.get(); }

	void setOnImageBufferChanged(std::function<void(char*, int, int)> func) { _decode->setOnImageBufferChanged(func); }
	void setOnMediaPlayFinished(std::function<void(void)> func) { _onMediaPlayFinished = func; }

private:

	static void SDLAudioCallback(void* arg, Uint8* stream, int len);
	void threadVideo();

	std::function<void(void)> _onMediaPlayFinished;

	
	State _state = idle;
	QString _sourceUrl;
	std::unique_ptr<std::thread> _threadVideo;

	SDL_AudioSpec _wantedSpec;
	int _timeStamp = 0;

	Semaphore _sem;
	AVFrameQueue _videoQeueu;
	AVFrameQueue _audioQueue;
	std::unique_ptr<MediaDecoder> _decode;

signals:
	void started();
	void nextVideoFrame();
	void ptsChanged(double pts);
};

