#pragma once

#include <QObject>
#include <thread>
#include <QAudioSink>
#include <QIODevice>
#include <QElapsedTimer>
#include <memory>
#include "SDL2/SDL.h"
#include "MediaDecoder.h"

class PlayController: public QObject
{
	Q_OBJECT

public:

	enum State
	{
		idle,
		running,
		pause,
		stop
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
	void seek(double position);
	void setState(PlayController::State state);
	MediaDecoder* getDecode() { return _decode.get(); }

private:

	static void SDLAudioCallback(void* arg, Uint8* stream, int len);

	void threadVideo();
	std::unique_ptr<MediaDecoder> _decode;
	State _state = idle;
	QString _sourceUrl;
	std::unique_ptr<std::thread> _threadVideo;

	SDL_AudioSpec _wantedSpec;
	SDL_AudioSpec _obtainedSpec;

	int _timeStamp = 0;

signals:
	void started();
	void nextVideoFrame(const QImage& image);
	void ptsChanged(double pts);
	void sourceChanged();
};

