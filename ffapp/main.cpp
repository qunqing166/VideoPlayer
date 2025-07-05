#include "MainWindow.h"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    app.exec();
    return 0;
}
