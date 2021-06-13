#include "sendimg.h"
#include "netheader.h"
#include <QDebug>
#include <cstring>

extern QUEUE_SEND queue_send;

SendImg::SendImg()
{

}

//消费线程
void SendImg::run()
{
    for(;;)
    {
        queue_lock.lock(); //加锁

        while(imgqueue.size() == 0)
        {
            //qDebug() << this << QThread::currentThreadId();
            queue_waitCond.wait(&queue_lock);
        }

        QImage img = imgqueue.front();
//        qDebug() << "取出队列:" << QThread::currentThreadId();
        imgqueue.pop_front();
        queue_lock.unlock();//解锁
        queue_waitCond.wakeAll(); //唤醒添加线程


        //构造消息体
        MESG * imgsend = new MESG();
        imgsend->msg_type = IMG_SEND;
        imgsend->len = img.sizeInBytes();
        imgsend->data = new uchar[imgsend->len];
        imgsend->format = img.format();
        imgsend->width = img.width();
        imgsend->height = img.height();
//        memcpy_s(imgsend->data, imgsend->len, img.bits(), imgsend->len);
        memcpy(imgsend->data, img.bits(), imgsend->len);

        //加入发送队列
        queue_send.push_msg(imgsend);
    }
}

//添加线程
void SendImg::pushToQueue(QImage img)
{
    qDebug() << "加入队列:" << QThread::currentThreadId();
    queue_lock.lock();
    while(imgqueue.size() > QUEUE_MAXSIZE)
    {
        queue_waitCond.wait(&queue_lock);
    }
    imgqueue.push_back(img);
//    qDebug() << "jiaru";
    queue_lock.unlock();
    queue_waitCond.wakeAll();
}


void SendImg::cameraImageCapture(QVideoFrame frame)
{
//    qDebug() << QThread::currentThreadId() << this;

    if(frame.isValid() && frame.isReadable())
    {
        QImage videoImg = QImage(frame.bits(), frame.width(), frame.height(), QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));

        QMatrix matrix;
        matrix.rotate(180.0);

        videoImg.transformed(matrix);

        pushToQueue(videoImg.transformed(matrix));
        //ui->mainshow_label->setPixmap(QPixmap::fromImage();

        //qDebug()<< "format: " <<  videoImg.format() << "size: " << videoImg.size() << "byteSIze: "<< videoImg.sizeInBytes();
    }
    frame.unmap();
}

void SendImg::clearImgQueue()
{
    qDebug() << "清空视频队列";
    imgqueue.clear();
}
