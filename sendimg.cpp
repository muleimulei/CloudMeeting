#include "sendimg.h"
#include "netheader.h"
#include <QDebug>
#include <cstring>
#include <QBuffer>

extern QUEUE_DATA<MESG> queue_send;

SendImg::SendImg(QObject *par):QThread(par)
{

}

//消费线程
void SendImg::run()
{
    WRITE_LOG("start sending picture thread: 0x%p", QThread::currentThreadId());
    m_isCanRun = true;
    for(;;)
    {
        queue_lock.lock(); //加锁

        while(imgqueue.size() == 0)
        {
            //qDebug() << this << QThread::currentThreadId();
            bool f = queue_waitCond.wait(&queue_lock, WAITSECONDS * 1000);
			if (f == false) //timeout
			{
				QMutexLocker locker(&m_lock);
				if (m_isCanRun == false)
				{
                    queue_lock.unlock();
					WRITE_LOG("stop sending picture thread: 0x%p", QThread::currentThreadId());
					return;
				}
			}
        }

        QByteArray img = imgqueue.front();
//        qDebug() << "取出队列:" << QThread::currentThreadId();
        imgqueue.pop_front();
        queue_lock.unlock();//解锁
        queue_waitCond.wakeOne(); //唤醒添加线程


        //构造消息体
        MESG* imgsend = (MESG*)malloc(sizeof(MESG));
        if (imgsend == NULL)
        {
            WRITE_LOG("malloc error");
            qDebug() << "malloc imgsend fail";
        }
        else
        {
            memset(imgsend, 0, sizeof(MESG));
			imgsend->msg_type = IMG_SEND;
			imgsend->len = img.size();
            qDebug() << "img size :" << img.size();
            imgsend->data = (uchar*)malloc(imgsend->len);
            if (imgsend->data == nullptr)
            {
                free(imgsend);
				WRITE_LOG("malloc error");
				qDebug() << "send img error";
                continue;
            }
            else
            {
                memset(imgsend->data, 0, imgsend->len);
				memcpy_s(imgsend->data, imgsend->len, img.data(), img.size());
				//加入发送队列
				queue_send.push_msg(imgsend);
            }
        }
    }
}

//添加线程
void SendImg::pushToQueue(QImage img)
{
    //压缩
    QByteArray byte;
    QBuffer buf(&byte);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "JPEG");
    QByteArray ss = qCompress(byte);
    QByteArray vv = ss.toBase64();
//    qDebug() << "加入队列:" << QThread::currentThreadId();
    queue_lock.lock();
    while(imgqueue.size() > QUEUE_MAXSIZE)
    {
        queue_waitCond.wait(&queue_lock);
    }
    imgqueue.push_back(vv);
    queue_lock.unlock();
    queue_waitCond.wakeOne();
}

void SendImg::ImageCapture(QImage img)
{
    pushToQueue(img);
}

void SendImg::clearImgQueue()
{
    qDebug() << "清空视频队列";
    QMutexLocker locker(&queue_lock);
    imgqueue.clear();
}


void SendImg::stopImmediately()
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;
}
