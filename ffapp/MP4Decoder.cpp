#include "MP4Decoder.h"
#include <iostream>
#include <QImage>
#include <QDebug>
#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioBuffer>
#include <QThread>
#include "spdlog/spdlog.h"
#include "RunningTime.h"

MP4Decoder::MP4Decoder() :
    MediaDecoder(), _sem(200)
{
    //m_buffer = new uchar[codecVideo->width * codecVideo->height * 3];
}

MP4Decoder::~MP4Decoder()
{
}

void MP4Decoder::setSource(const QString& file)
{

}

void MP4Decoder::initSource()
{
    RunningTime("Decoder init source");
    _formatContext = avformat_alloc_context();


    avformat_close_input(&_formatContext);

    if (avformat_open_input(&_formatContext, url.c_str(), NULL, NULL) != 0)
    {
        spdlog::error("avformat_open_input(&_formatContext, url.c_str(), NULL, NULL) != 0");
        system("pause");
    }

    if (avformat_find_stream_info(_formatContext, NULL) < 0)
    {
        spdlog::error("avformat_find_stream_info(_formatContext, NULL) < 0");
        system("pause");
    }

    for (int i = 0; i < _formatContext->nb_streams; i++)
    {
        if (_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            _videoStreamIndex = i;
            break;
        }
    }

    for (int i = 0; i < _formatContext->nb_streams; i++)
    {
        if (_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            _audioStreamIndex = i;
            break;
        }
    }

    if (_videoStreamIndex == -1)
    {
        spdlog::error("can not find video stream");
        system("pause");
    }

    AVCodecParameters* codecpar = _formatContext->streams[_videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    _codecVideo = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(_codecVideo, codecpar);

    if (codec == nullptr)
    {
        spdlog::error("find video decoder error");
        system("pause");
    }

    if (avcodec_open2(_codecVideo, codec, NULL) < 0)
    {
        spdlog::error("video avcodec_open2 error");
        system("pause");
    }

    codecpar = _formatContext->streams[_audioStreamIndex]->codecpar;
    codec = avcodec_find_decoder(codecpar->codec_id);
    _codecAudio = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(_codecAudio, codecpar);

    if (codec == nullptr)
    {
        spdlog::error("find audio decoder error");
        system("pause");
    }

    if (avcodec_open2(_codecAudio, codec, NULL) < 0)
    {
        spdlog::error("audio avcodec_open2 error");
        system("pause");
    }

    //qDebug() << _formatContext->streams[_videoStreamIndex]->time_base.den << " " << _formatContext->streams[_videoStreamIndex]->time_base.num;
    
    SwsContext* swsContext = sws_getContext(
        _codecVideo->width, _codecVideo->height, _codecVideo->pix_fmt,
        _codecVideo->width, _codecVideo->height, AV_PIX_FMT_YUV420P,
        SWS_BICUBIC, NULL, NULL, NULL
    );


    spdlog::info("video timebase: {0}/{1}",
        _formatContext->streams[_videoStreamIndex]->time_base.num,
        _formatContext->streams[_videoStreamIndex]->time_base.den);
    spdlog::info("audio timebase: {0}/{1}",
        _formatContext->streams[_audioStreamIndex]->time_base.num,
        _formatContext->streams[_audioStreamIndex]->time_base.den);
    spdlog::info("�����ʽ: {}", codec->name);
    spdlog::info("������: {} Hz", codecpar->sample_rate);
    spdlog::info("layout: {}", codecpar->ch_layout.nb_channels);
    spdlog::info("Bit Depth: {} bits", av_get_bits_per_sample(codecpar->codec_id));
    spdlog::info("������ʽ: {}", av_get_sample_fmt_name((AVSampleFormat)codecpar->format));
    const char* format_name = av_get_sample_fmt_name((AVSampleFormat)codecpar->format);
    if (format_name) {
        spdlog::info("Sample Format: {}\n", format_name);
    }
    else {
        spdlog::info("Sample Format: Unknown\n");
    }
    if (codecpar->block_align > 0) {
        spdlog::info("frame size: {} byte", codecpar->block_align);
    }
    spdlog::info("��Ƶ�ļ���ʽ: {}", _formatContext->iformat->name);
    spdlog::info("��Ƶʱ�䳤: {}", _formatContext->duration);
    spdlog::info("��Ƶ���: {}, {}", _codecVideo->width, _codecVideo->height);
    spdlog::info("����������: {}", codec->name);

    if (_buffer != nullptr)delete[] _buffer;
    _buffer = new uchar[_codecVideo->width * _codecVideo->height * 3];
    _state = State::ready;
}

void MP4Decoder::releaseSource()
{
    if (_codecAudio != nullptr)
    {
        avcodec_free_context(&_codecAudio);
        _codecAudio = nullptr;
    }
    if (_codecVideo != nullptr)
    {
        avcodec_free_context(&_codecVideo);
        _codecVideo = nullptr;
    }
    if (_formatContext != nullptr)
    {
        avformat_free_context(_formatContext);
        _formatContext = nullptr;
    }
}
AVFrame* MP4Decoder::getNextVideoFrame()
{
    std::lock_guard<std::mutex> lock(_mtxVideo);
    if (_pqVideoFrames.empty())
    {
        return nullptr;
    }
    auto tmp = _pqVideoFrames.top();
    _pqVideoFrames.pop();
    _sem.signal();
    return tmp;
}

AVFrame* MP4Decoder::getNextAudioFrame()
{
    std::lock_guard<std::mutex> lock(_mtxAudio);
    if (_pqAudioFrames.empty())
    {
        return nullptr;
    }
    auto tmp = _pqAudioFrames.top();
    _pqAudioFrames.pop();
    _sem.signal();
    return tmp;
}


void MP4Decoder::threadDecode()
{   
    spdlog::info("threadDecode running");

    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    while (1)
    {
        switch (_state)
        {
        case MediaDecoder::ready:
            break;
        case MediaDecoder::none:;
        case MediaDecoder::finished:;
        default:
            continue;
        }
        packet = av_packet_alloc();
        if (av_read_frame(_formatContext, packet) < 0)
        {
            av_packet_free(&packet);
            spdlog::info("av_read_frame(_formatContext, packet) over");
            _state = finished;
            continue;
        }
        const int maxSize = 50;
        if (_pqVideoFrames.size() > maxSize || _pqAudioFrames.size() > maxSize)
        {
            QThread::msleep(100);
        }
        
        if (packet->stream_index == _videoStreamIndex)
        {
            if (avcodec_send_packet(_codecVideo, packet) == 0)
            {
                while (1) 
                {
                    frame = av_frame_alloc();
                    if (avcodec_receive_frame(_codecVideo, frame) != 0)break;
                    _sem.wait();
                    std::lock_guard<std::mutex> lock(_mtxVideo);
                    _pqVideoFrames.push(frame);
                }
                av_frame_free(&frame);
            }
        }
        else if (packet->stream_index == _audioStreamIndex)
        {
            if (avcodec_send_packet(_codecAudio, packet) == 0)
            {
                while (1)
                {
                    frame = av_frame_alloc();
                    if (avcodec_receive_frame(_codecAudio, frame) != 0)break;
                    _sem.wait();
                    std::lock_guard<std::mutex> lock(_mtxAudio);
                    _pqAudioFrames.push(frame);
                }
                av_frame_free(&frame);
            }
        }
        av_packet_free(&packet);
    }
}

double MP4Decoder::getTimeBase()
{
    return _formatContext->streams[_videoStreamIndex]->time_base.num * 1.0 / _formatContext->streams[_videoStreamIndex]->time_base.den;
}

void MP4Decoder::seek(uint64_t ms)
{
    _state = wait;
    //int ts = s * 16;
    uint64_t ts = ms * _formatContext->streams[_audioStreamIndex]->time_base.den / 1000;
    spdlog::info("ts: {}", ts);
    av_seek_frame(_formatContext, _audioStreamIndex, ts, AVSEEK_FLAG_BACKWARD);
    this->clearFrames();
    _state = ready;
}

const AVCodecParameters* MP4Decoder::getVideoFormat()
{
    return _formatContext->streams[_videoStreamIndex]->codecpar;
}

const AVCodecParameters* MP4Decoder::getAudioFormat()
{
    return _formatContext->streams[_audioStreamIndex]->codecpar;
}

uint64_t MP4Decoder::getTimeStamp(uint64_t pts)
{
    // (pts * 1 / den) s
    return pts * 1000 / _formatContext->streams[_audioStreamIndex]->time_base.den;
    // return s * 1000 (ms)
}

void MP4Decoder::clearFrames()
{
    {
        std::lock_guard<std::mutex> lock(_mtxAudio);
        while (!_pqAudioFrames.empty())
        {
            auto f = _pqAudioFrames.top();
            av_frame_free(&f);
            _pqAudioFrames.pop();
            _sem.signal();
        }
    }
    {
        std::lock_guard<std::mutex> lock(_mtxVideo);
        while (!_pqVideoFrames.empty())
        {
            auto f = _pqVideoFrames.top();
            av_frame_free(&f);
            _pqVideoFrames.pop();
            _sem.signal();
        }
    }
}
