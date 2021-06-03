#include "sendtext.h"
#include <QDebug>

extern QUEUE_SEND queue_send;

SendText::SendText()
{

}

void SendText::push_Text(MSG_TYPE msgType, QString str)
{
    qDebug() << "加入队列:" << QThread::currentThreadId();
    textqueue_lock.lock();
    while(textqueue.size() > QUEUE_MAXSIZE)
    {
        queue_waitCond.wait(&textqueue_lock);
    }
    textqueue.push_back(M(str, msgType));
    qDebug() << "jiaru";
    textqueue_lock.unlock();
    queue_waitCond.wakeAll();
}


void SendText::run()
{
    for(;;)
    {
        textqueue_lock.lock(); //加锁

        while(textqueue.size() == 0)
        {
            //qDebug() << this << QThread::currentThreadId();
            queue_waitCond.wait(&textqueue_lock);
        }

        M text = textqueue.front();

        qDebug() << "取出队列:" << QThread::currentThreadId();

        textqueue.pop_front();
        textqueue_lock.unlock();//解锁
        queue_waitCond.wakeAll(); //唤醒添加线程

        //构造消息体
        MESG * send = new MESG();

        if(text.type == CREATE_MEETING)
        {
            send->msg_type = CREATE_MEETING;
        }


        //加入发送队列
        queue_send.send_queueLock.lock();
        while(queue_send.send_queue.size() > QUEUE_MAXSIZE)
        {
            queue_send.send_queueCond.wait(&queue_send.send_queueLock);
        }
        queue_send.send_queue.push_back(send);
        queue_send.send_queueLock.unlock();
        queue_send.send_queueCond.wakeAll();
    }
}
