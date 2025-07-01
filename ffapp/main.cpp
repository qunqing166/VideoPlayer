#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include "PlayerWidget.h"
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include "PlayController.h"
#include "spdlog/spdlog.h"



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    app.exec();
}
