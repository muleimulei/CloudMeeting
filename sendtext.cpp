#include "sendtext.h"
#include <QDebug>

extern QUEUE_SEND queue_send;

SendText::SendText()
{

}

SendText::~SendText()
{
    if(this->isRunning())
    {
        this->quit();
    }
}

void SendText::push_Text(MSG_TYPE msgType, QString str)
{
    textqueue_lock.lock();
    while(textqueue.size() > QUEUE_MAXSIZE)
    {
        queue_waitCond.wait(&textqueue_lock);
    }
    textqueue.push_back(M(str, msgType));

    textqueue_lock.unlock();
    queue_waitCond.wakeAll();
//    qDebug() << "加入队列:" << QThread::currentThreadId();
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

//        qDebug() << "取出队列:" << QThread::currentThreadId();

        textqueue.pop_front();
        textqueue_lock.unlock();//解锁
        queue_waitCond.wakeAll(); //唤醒添加线程

        //构造消息体
        MESG * send = new MESG();

        if(text.type == CREATE_MEETING)
        {
            send->len = 0;
            send->data = NULL;
            send->msg_type = CREATE_MEETING;
        }
        else if(text.type == JOIN_MEETING)
        {
            send->msg_type = JOIN_MEETING;
            send->len = 4; //房间号占4个字节
            send->data = (uchar *)malloc(send->len);
            quint32 roomno = text.str.toUInt();
            memcpy(send->data, &roomno, sizeof (roomno));
        }

        //加入发送队列
        queue_send.push_msg(send);
    }
}
