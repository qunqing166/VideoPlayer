#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QAudioSink>
#include "MP4Decoder.h"
#include <QElapsedTimer>
#include <QTime>
#include "PlayController.h"

class PlayerWidget : public QWidget
{
    Q_OBJECT

public:
    PlayerWidget(PlayController *controller, QWidget* parent = nullptr);
    ~PlayerWidget();

    void setController(PlayController* controller);

    void setTimeBase(double base);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;


private:

    QImage m_currentFrame;
    QTimer m_timer;
    int pst = 0;
    double timeBase;

    QTime m_base;
    PlayController* _controller = nullptr;

    QThread* _threadVideo;
};

#endif // PLAYERWIDGET_H