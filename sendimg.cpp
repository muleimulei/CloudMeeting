#include "sendimg.h"
#include "netheader.h"
extern QUEUE_SEND queue_send;

SendImg::SendImg()
{
    start = false; //初始不发送视频
}

void SendImg::run()
{
    while(1)
    {
        while(start); //开启开关

        queue_lock.lock(); //加锁
        QImage img = imgqueue.front();
        imgqueue.pop_front();
        queue_lock.unlock();//解锁

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

void SendImg::pushToQueue(QImage img)
{
    queue_lock.lock();
    imgqueue.push_back(img);
    queue_lock.unlock();
}

void SendImg::startSend()
{
    start = true;
}
void SendImg::stopSend()
{
    start = false;
}
