#ifndef SENDIMG_H
#define SENDIMG_H

#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QImage>
#include <QVideoFrame>
#include <QWaitCondition>


class SendImg : public QThread
{
    Q_OBJECT
private:
    QQueue<QImage> imgqueue;
    QMutex queue_lock;
    QWaitCondition queue_waitCond;
    void run() override;
public:
    SendImg();

    void pushToQueue(QImage);
public slots:
    void cameraImageCapture(QVideoFrame); //捕获到视频帧
    void clearImgQueue(); //线程结束时，清空视频帧队列
};

#endif // SENDIMG_H
