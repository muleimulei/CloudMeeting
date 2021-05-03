#ifndef SENDIMG_H
#define SENDIMG_H

#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QImage>

class SendImg : public QThread
{
//    Q_OBJECT
private:
    QQueue<QImage> imgqueue;
    QMutex queue_lock;
    void run() override;
    bool start;
public:
    void startSend();
    void stopSend();
    SendImg();

    void pushToQueue(QImage);
};

#endif // SENDIMG_H
