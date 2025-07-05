#include "MediaDecoder.h"
#include <thread>
#include "spdlog/spdlog.h"
#include <RunningTime.h>
#include <qthread.h>

MediaDecoder::MediaDecoder():
    _sem(200)
{
}

MediaDecoder::~MediaDecoder() 
{
    _state = finished;
    _threadDecode->join();
    this->clearFrames();
    avformat_close_input(&_formatContext);
}

void MediaDecoder::start()
{
    _threadDecode.reset(new std::thread(&MediaDecoder::threadDecode, this));
    spdlog::info("threadDecode created");
}

void MediaDecoder::setSource(const std::string& file)
{
    if (url != file)
    {
        url = file;
        initSource();
        spdlog::info("MediaDecoder::setSource(const std::string& file): new url");
    }
    else
    {
        spdlog::info("MediaDecoder::setSource(const std::string& file): url == file");
    }
}

//QImage MediaDecoder::frameToImage(AVFrame* frame)
//{
//    auto swsContext = sws_getContext(
//        _codecVideo->width, _codecVideo->height,
//        _codecVideo->pix_fmt,
//        _codecVideo->width, _codecVideo->height,
//        AV_PIX_FMT_RGB24,
//        SWS_BILINEAR, nullptr, nullptr, nullptr);
//    int tmp = _codecVideo->width * 3;
//
//    sws_scale(
//        swsContext,
//        (const uint8_t* const*)frame->data,
//        frame->linesize,
//        0,
//        _codecVideo->height,
//        &_buffer,
//        &tmp
//    );
//
//    // 这里用复制, 如果直接使用_buffer创建, 可能会崩溃
//    // 直接使用_buffer创建的QImage是直接使用该缓冲区, 需要保证使用时该缓冲区没有被释放
//    // 当切换视频时, 缓冲区会重新创建, 此时使用会导致程序崩溃
//    //QImage image(_codecVideo->width, _codecVideo->height, QImage::Format_RGB888);
//    //memcpy(image.bits(), _buffer, _codecVideo->width * _codecVideo->height * 3);
//    //return image;
//
//    //return QImage(_buffer, _codecVideo->width, _codecVideo->height, QImage::Format_RGB888);
//    return QImage();
//}

void MediaDecoder::fillFrameBuffer(AVFrame* frame)
{
    auto swsContext = sws_getContext(
        _codecVideo->width, _codecVideo->height,
        _codecVideo->pix_fmt,
        _codecVideo->width, _codecVideo->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    int tmp = _codecVideo->width * 3;

    sws_scale(
        swsContext,
        (const uint8_t* const*)frame->data,
        frame->linesize,
        0,
        _codecVideo->height,
        &_buffer,
        &tmp
    );
}

void MediaDecoder::setOpenStream(IOpenStream* stream) 
{ 
    if (_openStream != nullptr)delete _openStream;
    _openStream = stream; 
}

AVFrame* MediaDecoder::getNextVideoFrame()
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

AVFrame* MediaDecoder::getNextAudioFrame()
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

uint64_t MediaDecoder::getTimeStamp(uint64_t pts)
{
    return pts * 1000 / _formatContext->streams[_audioStreamIndex]->time_base.den;
}

void MediaDecoder::seek(uint64_t ms)
{
    _state = wait;
    uint64_t ts = ms * _formatContext->streams[_audioStreamIndex]->time_base.den / 1000;
    spdlog::info("ts: {}", ts);
    av_seek_frame(_formatContext, _audioStreamIndex, ts, AVSEEK_FLAG_BACKWARD);
    this->clearFrames();
    _state = ready;
}

void MediaDecoder::printfMediaInfo()
{
    AVCodecParameters* codecpar;
    const AVCodec* codec;

    if (_videoStreamIndex != -1)
    {
        codecpar = _formatContext->streams[_videoStreamIndex]->codecpar;
        codec = avcodec_find_decoder(codecpar->codec_id);

        spdlog::info("video timebase: {0}/{1}",
            _formatContext->streams[_videoStreamIndex]->time_base.num,
            _formatContext->streams[_videoStreamIndex]->time_base.den);
        spdlog::info("视频文件格式: {}", _formatContext->iformat->name);
        spdlog::info("视频时间长: {}", _formatContext->duration);
        spdlog::info("视频宽高: {}, {}", _codecVideo->width, _codecVideo->height);
        spdlog::info("解码器名称: {}", codec->name);
    }

    if (_audioStreamIndex != -1)
    {
        codecpar = _formatContext->streams[_audioStreamIndex]->codecpar;
        codec = avcodec_find_decoder(codecpar->codec_id);

        spdlog::info("audio timebase: {0}/{1}",
            _formatContext->streams[_audioStreamIndex]->time_base.num,
            _formatContext->streams[_audioStreamIndex]->time_base.den);

        spdlog::info("编码格式: {}", codec->name);
        spdlog::info("采样率: {} Hz", codecpar->sample_rate);
        spdlog::info("channels: {}", codecpar->ch_layout.nb_channels);
        //spdlog::info("Bit Depth: {} bits", av_get_bits_per_sample(codecpar->codec_id));
        spdlog::info("采样格式: {}", av_get_sample_fmt_name((AVSampleFormat)codecpar->format));
        const char* format_name = av_get_sample_fmt_name((AVSampleFormat)codecpar->format);
        if (format_name) spdlog::info("Sample Format: {}\n", format_name);
        //if (codecpar->block_align > 0) spdlog::info("frame size: {} byte", codecpar->block_align);
    }
}

void MediaDecoder::clearFrames()
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
    spdlog::debug("MediaDecoder::clearFrames()");
}

void MediaDecoder::setState(State state)
{
    _state.store(state);
    switch (_state)
    {
    case MediaDecoder::idle:
        break;
    case MediaDecoder::wait:
        break;
    case MediaDecoder::ready:
        isVideoThreadWaitting = false;
        break;
    case MediaDecoder::finished:
        break;
    default:
        break;
    }
}

void MediaDecoder::initSource()
{
    RunningTime("Decoder init source");
    
    setState(wait);

    while (_state.load() != idle);

    this->clearFrames();
    avformat_close_input(&_formatContext);


    _videoStreamIndex = -1;
    _audioStreamIndex = -1;

    _formatContext = _openStream->openStream(url);


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

    AVCodecParameters* codecpar;
    const AVCodec* codec;

    if (_videoStreamIndex != -1)
    {
        codecpar = _formatContext->streams[_videoStreamIndex]->codecpar;
        codec = avcodec_find_decoder(codecpar->codec_id);
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
    }
    else
    {
        spdlog::warn("can not find video stream");
    }

    if (_audioStreamIndex != -1)
    {
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
    }
    else
    {
        spdlog::warn("can not find audio stream");
    }

    if (_videoStreamIndex != -1)
    {
        SwsContext* swsContext = sws_getContext(
            _codecVideo->width, _codecVideo->height, _codecVideo->pix_fmt,
            _codecVideo->width, _codecVideo->height, AV_PIX_FMT_YUV420P,
            SWS_BICUBIC, NULL, NULL, NULL
        );

        if (_buffer != nullptr)delete[] _buffer;
        _buffer = new uchar[_codecVideo->width * _codecVideo->height * 3];
        _onImageBufferChanged((char*)_buffer, _codecVideo->width, _codecVideo->height);
    }
    else
    {
        if (_buffer != nullptr)delete[] _buffer;
        _buffer = nullptr;
        _onImageBufferChanged(nullptr, 0, 0);
    }

    this->printfMediaInfo();


    setState(ready);
}

void MediaDecoder::releaseSource()
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

void MediaDecoder::threadDecode()
{
    spdlog::info("threadDecode running");

    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    while (_state != finished)
    {
        switch (_state.load())
        {
        case MediaDecoder::ready:
            break;
        case MediaDecoder::idle:
            continue;
        case MediaDecoder::wait:
            setState(idle);
            continue;
        case MediaDecoder::wait_next:
            continue;
        default:
            continue;
        }
        packet = av_packet_alloc();
        if (av_read_frame(_formatContext, packet) < 0)
        {
            av_packet_free(&packet);
            spdlog::info("av_read_frame(_formatContext, packet) over");
            setState(idle);
            //std::async(std::launch::async, [&] {
            //    _onVideoPlayFinished();
            //    });
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
    spdlog::warn("decode thread finished");
}
