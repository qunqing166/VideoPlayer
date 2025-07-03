#include "BiliUtils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <thread>
#include "spdlog/spdlog.h"
#include "curl/curl.h"

std::string BiliUtils::getStreamUrl(const std::string& webUrl)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::string res1 = getResponse(videoInfoUrl + webUrl);
    
    spdlog::info("{}", res1);

    QJsonObject obj = QJsonDocument::fromJson(res1.c_str()).object();
    
    int code = obj["code"].toInt();
    QString fileName = obj["data"].toObject()["title"].toString();
    if (code != 0)
    {
        QString&& errorInfo = obj["message"].toString();
        spdlog::info("bvid error: {}", errorInfo.toStdString());
        return std::string();
    }

    QJsonObject&& obj_data = obj["data"].toObject();

    int64_t aid = obj_data["aid"].toInteger();
    int64_t cid = obj_data["cid"].toInteger();

    qDebug() << "aid: " << aid;
    qDebug() << "cid: " << cid;

    QUrl url = QString("%1?avid=%2&cid=%3&qn=%4&fourk=1").arg(QString(playUrl.c_str())).arg(aid).arg(cid).arg(64);
    std::string res2 = getResponse(url.toString().toStdString());

    obj = QJsonDocument::fromJson(res2.c_str()).object();

    code = obj["code"].toInt();

    if (code != 0)
    {
        QString errorInfo = obj["message"].toString();
        qDebug() << errorInfo;
    }

    QJsonObject&& obj_durl = obj["data"].toObject()["durl"].toArray()[0].toObject();
    url = obj_durl["url"].toString();
    long long size = obj_durl["size"].toInteger();

    curl_global_cleanup();

    spdlog::info("stream url: {}", url.toString().toStdString());
    return url.toString().toStdString();
}

std::string BiliUtils::getResponse(const std::string& url)
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        spdlog::error("curl_easy_perform() failed: {}", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);

    return readBuffer;
}

size_t BiliUtils::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
