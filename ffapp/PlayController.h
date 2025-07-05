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
	 * @brief �������ƶ���ָ��λ��
	 * @para position ��Χ0~1
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

	std::unique_ptr<MediaDecoder> _decode;
	State _state = idle;
	QString _sourceUrl;
	std::unique_ptr<std::thread> _threadVideo;

	SDL_AudioSpec _wantedSpec;
	SDL_AudioSpec _obtainedSpec;
	int _timeStamp = 0;

signals:
	void started();
	void nextVideoFrame();
	void ptsChanged(double pts);
};

