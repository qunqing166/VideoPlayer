#pragma once

#include <QObject>
#include "MP4Decoder.h"
#include <thread>
#include <QAudioSink>
#include <QIODevice>
#include <QElapsedTimer>
#include <memory>

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
	void start();
	//void start(const QString& url);
	void restart();

	//void pause();
	//void stop();
	void seek(double position);
	void setState(PlayController::State state);

	MediaDecoder* getDecode() { return _decode; }

private:

	void setAudioFormat(const QAudioFormat& format);

	void threadAudio();
	void threadVideo();

	MediaDecoder* _decode;
	State _state = none;

	QString _sourceUrl;
	
	QAudioFormat _audioFormat;
	std::unique_ptr<QAudioSink> _audioSink;
	QIODevice* _audioIO = nullptr;

	
	std::unique_ptr<std::thread> _threadAudio;
	std::unique_ptr<std::thread> _threadVideo;

	bool isAudioReady = false;
	int _timeStamp = 0;

signals:
	void started();
	void nextVideoFrame(const QImage& image);
	void audioWrite(const char* data, int len);
	void ptsChanged(double pts);
	//void sourceChanged(int duration);
};

