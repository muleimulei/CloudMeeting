#include "sendimg.h"
#include "netheader.h"
#include <QDebug>
#include <cstring>

extern QUEUE_SEND queue_send;

SendImg::SendImg(QObject *par):QThread(par)
{

}

//消费线程
void SendImg::run()
{
    m_isCanRun = true;
    for(;;)
    {
        {
            QMutexLocker locker(&m_lock);
            if(!m_isCanRun)//在每次循环判断是否可以运行，如果不行就退出循环
            {
                return;
            }
        }

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
//    qDebug() << "加入队列:" << QThread::currentThreadId();
    queue_lock.lock();
    while(imgqueue.size() > QUEUE_MAXSIZE)
    {
        queue_waitCond.wait(&queue_lock);
    }
    imgqueue.push_back(img);
    queue_lock.unlock();
    queue_waitCond.wakeAll();
}

void SendImg::ImageCapture(QImage img)
{
    pushToQueue(img);
}

void SendImg::clearImgQueue()
{
    qDebug() << "清空视频队列";
    imgqueue.clear();
}


void SendImg::stopImmediately()
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;
}
