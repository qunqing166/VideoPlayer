#pragma once

#include <string>

class BiliUtils
{

public:


	static std::string getStreamUrl(const std::string& webUrl);

private:

	static std::string getResponse(const std::string& url);
	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

	inline static const std::string videoInfoUrl = "https://api.bilibili.com/x/web-interface/view?bvid=";
	inline static const std::string playUrl = "https://api.bilibili.com/x/player/playurl";

	inline static const std::string referer = "https://www.bilibili.com";
	inline static const std::string userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.85 Safari/537.36 Edg/90.0.818.49";

};

