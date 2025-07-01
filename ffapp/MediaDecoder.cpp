#include "MediaDecoder.h"
#include <thread>
#include "spdlog/spdlog.h"

MediaDecoder::MediaDecoder()
{
    //_threadDecode = std::make_unique<std::thread*>(new std::thread(&MediaDecoder::threadDecode, this));
    spdlog::info("threadDecode created");
}

MediaDecoder::~MediaDecoder() 
{

}

void MediaDecoder::start()
{
    _threadDecode = std::make_unique<std::thread*>(new std::thread(&MediaDecoder::threadDecode, this));
}

void MediaDecoder::setSource(const std::string& file)
{
    if (url != file)
    {
        url = file;
        initSource();
        //_threadDecode = std::make_unique<std::thread*>(new std::thread(&MediaDecoder::threadDecode, this));
        spdlog::info("MediaDecoder::setSource(const std::string& file): new url");
    }
    else
    {
        spdlog::info("MediaDecoder::setSource(const std::string& file): url == file");
    }
}

QImage MediaDecoder::frameToImage(AVFrame* frame)
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
    return QImage(_buffer, _codecVideo->width, _codecVideo->height, QImage::Format_RGB888);
}
