#include "NetStreamOpen.h"
#include "spdlog/spdlog.h"
#include "BiliUtils.h"
#include <regex>

AVFormatContext* NetStreamOpen::openStream(const std::string& url)
{
    std::regex pattern("BV[0-9a-zA-Z]+");
    std::smatch match;

    if (std::regex_search(url, match, pattern)) {
        spdlog::info("find bvid: {}", match[0].str());
    }
    else 
    {
        spdlog::error("none BV");
    }

    std::string res = BiliUtils::getStreamUrl(match[0]);
    auto formatContext = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options, "headers",
        "Referer: https://www.bilibili.com\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.85 Safari/537.36 Edg/90.0.818.49\r\n", 0);
   
    if (avformat_open_input(&formatContext, res.c_str(), NULL, &options) != 0)
    {
        spdlog::error("avformat_open_input(&_formatContext, url.c_str(), NULL, NULL) != 0");
    }
    else
    {
        spdlog::info("bili stream open ok");
    }
    return formatContext;
}
