#include "PlayerWidget.h"
#include <QPainter>
#include <QThread>
#include <QPushButton>
#include "spdlog/spdlog.h"
#include <QPainterPath>

PlayerWidget::PlayerWidget(PlayController* controller, QWidget* parent):
    QWidget(parent)
{
    this->setController(controller);

    connect(controller, &PlayController::nextVideoFrame, this, [&]() {
        this->update();
        }, Qt::QueuedConnection);
}

PlayerWidget::~PlayerWidget()
{
    
}

void PlayerWidget::setController(PlayController* controller)
{
    _controller = controller;
    _controller->setParent(this);
    _controller->setOnImageBufferChanged([&](char* buffer, int width, int height) {
        if (buffer == nullptr)this->m_currentFrame = QImage();
        else m_currentFrame = QImage((uchar*)buffer, width, height, QImage::Format_RGB888);
        });
}

void PlayerWidget::setTimeBase(double base)
{
    qDebug() << "time_base: " << base * 1000;
    timeBase = base * 1000;
    timeBase = 1.0 / 36;
}

void PlayerWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing, true);
    if (!m_currentFrame.isNull()) {
        QImage&& tmp = m_currentFrame.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawImage(rect().topLeft() + QPoint((this->width() - tmp.width()) / 2, (this->height() - tmp.height()) / 2), tmp);
    }
    if (_controller->getState() == PlayController::pause)
    {
        QImage image = QImage(":/resource/icon/stopping.png").scaled(QSize(50, 50), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawImage(rect().topLeft() + QPoint((this->width() - image.width()) / 2, (this->height() - image.height()) / 2), image);
    }
}

void PlayerWidget::resizeEvent(QResizeEvent* event)
{

}
