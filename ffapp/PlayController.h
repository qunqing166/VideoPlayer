#pragma once

#include <QObject>
#include "MP4Decoder.h"
#include <thread>
#include <QAudioSink>
#include <QIODevice>
#include <QElapsedTimer>
#include <memory>
#include "SDL2/SDL.h"

class PlayController: public QObject
{
	Q_OBJECT

public:

	enum State
	{
		none,
		running,
		pause,
		stop
	};

	PlayController(QObject* parent = nullptr);
	~PlayController();

	bool setSource(const QString& url);
	void seek(double position);
	void setState(PlayController::State state);
	MediaDecoder* getDecode() { return _decode.get(); }

private:

	static void SDLAudioCallback(void* arg, Uint8* stream, int len);

	void threadVideo();

	//MediaDecoder* _decode;
	std::unique_ptr<MediaDecoder> _decode;
	State _state = none;

	QString _sourceUrl;

	std::unique_ptr<std::thread> _threadVideo;

	SDL_AudioSpec _wantedSpec;
	SDL_AudioSpec _obtainedSpec;

	int _timeStamp = 0;

signals:
	void started();
	void nextVideoFrame(const QImage& image);
	void ptsChanged(double pts);
};

