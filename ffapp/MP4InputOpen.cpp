#include "MP4InputOpen.h"
#include "spdlog/spdlog.h"

AVFormatContext* MP4InputOpen::openStream(const std::string& url)
{
    auto formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, url.c_str(), NULL, NULL) != 0)
    {
        spdlog::error("avformat_open_input(&_formatContext, url.c_str(), NULL, NULL) != 0");
    }
    else
    {
        spdlog::info("open stream: {}", url);
    }
    return formatContext;
}
