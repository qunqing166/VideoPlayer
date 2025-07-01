#pragma once

#include <QThread>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>

class AudioThread: public QThread
{
public:
	AudioThread()
	{
		QAudioFormat format;
		format.setSampleRate(44100);
		format.setChannelCount(1);
		format.setSampleFormat(QAudioFormat::Float);

		//format.setChannelConfig(QAudioFormat::ChannelConfig2Dot1)
		QAudioSink* sink = new QAudioSink(format);
		QIODevice* dev = sink->start();
	}
	~AudioThread()
	{

	}

	void run() override
	{
		while (1)
		{

		}
	}
};

