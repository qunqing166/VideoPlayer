#include "PlayController.h"
#include <QThread>
#include <thread>
#include <QWaitCondition>
#include <QDebug>
#include <QImage>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
}

#include "RunningTime.h"
#include "spdlog/spdlog.h"

PlayController::PlayController(QObject* parent):
	QObject(parent)
{
    _decode = new MP4Decoder();
    _audioFormat.setSampleRate(44100);
    _audioFormat.setChannelCount(1);
    _audioFormat.setSampleFormat(QAudioFormat::Float);
    _audioSink = new QAudioSink(_audioFormat, this);

    _threadAudio = std::make_unique<std::thread*>(new std::thread(&PlayController::threadAudio, this));
    _threadVideo = std::make_unique<std::thread*>(new std::thread(&PlayController::threadVideo, this));

    spdlog::info("PlayController creaed ok");
}

PlayController::~PlayController()
{
    delete _decode;
}

bool PlayController::setSource(const QString& url)
{
    if (_sourceUrl != url)
    {
        this->_sourceUrl = url;
        _decode->setSource(_sourceUrl.toStdString());
        return true;
    }
    return false;
}

void PlayController::start()
{
    _audioIO = _audioSink->start();

    _decode->start();
    this->setState(running);
}

void PlayController::restart()
{
    RunningTime t("restart");
    this->setState(pause);
    _decode->seek(0);
    _timeStamp = 0;
    Sleep(100);
    this->setState(running);
}

void PlayController::seek(double position)
{
    int msc = _decode->getFormatContext()->duration * position / 1000 ;
    _decode->seek(msc);
}

void PlayController::setState(PlayController::State state)
{
    _state = state;
    switch (_state)
    {
    case PlayController::none:
        break;
    case PlayController::running:
        _audioSink->start();
        break;
    case PlayController::pause:
        _audioSink->stop();
        break;
    case PlayController::stop:
        break;
    default:
        break;
    }
    spdlog::info("play state changed: {}", state == running ? "running" : "pause");
}

void PlayController::threadAudio()
{
	for (;;)
	{
        switch (_state)
        {
        case PlayController::none:
            continue;
        case PlayController::running:
            break;
        case PlayController::pause:
            continue;
        case PlayController::stop:
            continue;
        default:
            continue;
        }

        AVFrame* frame = _decode->getNextAudioFrame();
        if (frame == nullptr)continue;

        int size = av_samples_get_buffer_size(
                        nullptr,
                        1,
                        frame->nb_samples,
                        AV_SAMPLE_FMT_FLTP,
                        1
                   );

        static int pre = -1;
        _timeStamp = frame->pts * 1000 / 44100;
        if (pre != _timeStamp)
        {
            pre = _timeStamp;
            emit ptsChanged(_timeStamp * 1000.0 / _decode->getFormatContext()->duration);
        }

        char* data = (char*)frame->data[0];
        int index = 0;
        while (1)
        {
            int free = _audioSink->bytesFree();
            if (free > 0)
            {
                int flag = 0;
                int len = size - index;
                if (len > free)
                {
                    len = free;
                }
                else
                {
                    flag = 1;
                }

                bool isReady = false;   // 比条件变量好用[bushi]

                QMetaObject::invokeMethod(this, [&]() {
                    _audioIO->write(data, len);
                    isReady = true;
                    }, Qt::QueuedConnection);
                emit audioWrite(data, len);

                while (isReady == false);

                data += len;
                index += len;
                if (flag == 1)break;
            }
        }

        av_frame_free(&frame);
    }
}

void PlayController::threadVideo()
{
    for (;;)
    {
        if (_state == State::pause)continue;
        AVFrame* frame = _decode->getNextVideoFrame();
        if (frame == nullptr)continue;

        long long waitTime = frame->pts * 1000 / 16000 - _timeStamp;

        if (waitTime > 1000)
        {
            spdlog::warn("wait time is to long: {} ms", waitTime);
            waitTime = 100;
        }
        if (waitTime > 0) {
            QThread::usleep(waitTime * 1000);
            emit nextVideoFrame(_decode->frameToImage(frame));
        }

        av_frame_free(&frame);
    }
    spdlog::warn("thread video quit");
}
