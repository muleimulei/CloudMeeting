#include "sendimg.h"
#include "netheader.h"

#include <QDebug>
extern QUEUE_SEND queue_send;

SendImg::SendImg()
{

}


//消费线程
void SendImg::run()
{
    while(1)
    {
        queue_lock.lock(); //加锁

        while(imgqueue.size() == 0)
        {
            //qDebug() << this << QThread::currentThreadId();
            queue_waitCond.wait(&queue_lock);
        }

        QImage img = imgqueue.front();
        qDebug() << "quchu";
        imgqueue.pop_front();
        queue_lock.unlock();//解锁
        queue_waitCond.wakeAll(); //唤醒添加线程


        //构造消息体
        MSGSend * imgsend = new MSGSend();
        imgsend->msg_type = IMG_SEND;
        imgsend->len = img.sizeInBytes();
        imgsend->data = new uchar[imgsend->len+1];
        memcpy_s(imgsend->data, imgsend->len, img.bits(), imgsend->len);

        //加入发送队列
        queue_send.send_queueLock.lock();
        queue_send.send_queue.push_back(imgsend);
        queue_send.send_queueLock.unlock();
    }
}

//添加线程
void SendImg::pushToQueue(QImage img)
{
    //qDebug() << "加入队列:" << QThread::currentThreadId();
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
    qDebug() << QThread::currentThreadId() << this;

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
