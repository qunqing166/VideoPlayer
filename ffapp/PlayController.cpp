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
#include "SDL2/SDL.h"
#include "MP4InputOpen.h"
#include "NetStreamOpen.h"

PlayController::PlayController(QObject* parent):
	QObject(parent)
{
    SDL_Init(SDL_INIT_AUDIO);
    _decode.reset(new MediaDecoder());
    _decode->start();

    _wantedSpec.format = AUDIO_F32SYS;
    _wantedSpec.samples = 1024;
    _wantedSpec.userdata = this;
    _wantedSpec.callback = PlayController::SDLAudioCallback;

    _threadVideo.reset(new std::thread(&PlayController::threadVideo, this));
}

PlayController::~PlayController()
{
    _state = finished;
    _threadVideo->join();
    SDL_CloseAudio();
}

bool PlayController::setSource(const QString& url)
{
    setState(stop);
    if (_sourceUrl != url)
    {
        _state = pause;
        this->_sourceUrl = url;
        _decode->setSource(_sourceUrl.toStdString());
        auto audioInfo = _decode->getAudioFormat();
        _wantedSpec.channels = audioInfo->ch_layout.nb_channels;
        _wantedSpec.freq = audioInfo->sample_rate / audioInfo->ch_layout.nb_channels;

        if (SDL_OpenAudio(&_wantedSpec, &_obtainedSpec) < 0)
        {
            spdlog::error("SDL_OpenAudio(&_wantedSpec, &_obtainedSpec)");
        }
        else
        {
            spdlog::info("SDL_OpenAudio OK");
        }
        setState(running);
        return true;
    }
    return false;
}

bool PlayController::setSource(const std::string& url, StreamType type)
{
    setState(stop);
    switch (type)
    {
    case MP4:_decode->setOpenStream(new MP4InputOpen()); break;
    case NetStream:_decode->setOpenStream(new NetStreamOpen()); break;
    default:_decode->setOpenStream(new MP4InputOpen()); break;
    }
    return setSource(QString(url.c_str()));
}

void PlayController::seek(double position)
{
    setState(pause);
    uint64_t sc = _decode->getFormatContext()->duration * position / 1000;
    _decode->seek(sc);
    setState(running);
}

void PlayController::setState(PlayController::State state)
{
    _state = state;
    switch (_state)
    {
    case PlayController::idle:
        break;
    case PlayController::running:
        SDL_PauseAudio(0);
        break;
    case PlayController::pause:
        SDL_PauseAudio(1);
        break;
    case PlayController::stop:
        SDL_CloseAudio();
        break;
    default:
        break;
    }
    spdlog::info("play state changed: {}", state == running ? "running" : "pause");
}

void PlayController::SDLAudioCallback(void* arg, Uint8* stream, int len)
{
    PlayController* obj = (PlayController*)arg;
    SDL_memset(stream, 0, len);
    static int remaining = 0;
    static char* buffer = nullptr;
    while (len > 0)
    {
        if (remaining <= len)
        {
            if (buffer != nullptr && remaining > 0)
            {
                memcpy(stream, buffer, remaining);
                stream += remaining;
                len -= remaining;
                remaining = 0;
            }
            AVFrame* frame = obj->_decode->getNextAudioFrame();
            if (frame == nullptr)
            {
                if (obj->getDecode()->getState() == idle)
                {
                    static std::future<void> fu;
                    fu = std::async(std::launch::async, [=]() {
                        obj->_onMediaPlayFinished();
                        spdlog::debug("async finished");
                        });
                    return;
                    //obj->_onMediaPlayFinished();
                }
                continue;
            }
            remaining = av_samples_get_buffer_size(
                nullptr,
                1,
                frame->nb_samples,
                AV_SAMPLE_FMT_FLTP,
                1
            );
            obj->_timeStamp = obj->_decode->getTimeStamp(frame->pts);
            emit obj->ptsChanged(obj->_timeStamp * 1000.0 / obj->_decode->getFormatContext()->duration);
            buffer = (char*)frame->data[0];
        }
        else
        {
            if (buffer != nullptr)
            {
                memcpy(stream, buffer, len);
                buffer += len;
                remaining -= len;
                len = 0;
            }
            else
            {
                spdlog::warn("PlayController::SDLAudioCallback: buffer is nullptr");
            }
        }
    }
}

void PlayController::threadVideo()
{
    while(_state != finished)
    {
        switch (_state)
        {
        case PlayController::idle:
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

        AVFrame* frame = _decode->getNextVideoFrame();
        if (frame == nullptr)continue;

        int64_t waitTime;
        if (frame->pts == INT64_MIN)waitTime = 1;
        else waitTime = frame->pts * 1000 / 16000 - _timeStamp;

        if (waitTime > 500)
        {
            spdlog::warn("wait time is to long: {} ms", waitTime);
            waitTime = 1;
            //av_frame_free(&frame);
            //continue;
        }
        if (waitTime > 0) {
            QThread::usleep(waitTime * 1000);
            _decode->fillFrameBuffer(frame);
            emit nextVideoFrame();
        }

        av_frame_free(&frame);
    }
    spdlog::warn("thread video quit");
}
