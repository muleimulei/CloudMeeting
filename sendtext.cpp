#include "sendtext.h"
#include <QDebug>

extern QUEUE_DATA<MESG> queue_send;
#ifndef WAITSECONDS
#define WAITSECONDS 2
#endif
SendText::SendText(QObject *par):QThread(par)
{

}

SendText::~SendText()
{

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
    queue_waitCond.wakeOne();
}


void SendText::run()
{
    m_isCanRun = true;
    WRITE_LOG("start sending text thread: 0x%p", QThread::currentThreadId());
    for(;;)
    {
        textqueue_lock.lock(); //加锁
        while(textqueue.size() == 0)
        {
            bool f = queue_waitCond.wait(&textqueue_lock, WAITSECONDS * 1000);
            if(f == false) //timeout
            {
                QMutexLocker locker(&m_lock);
                if(m_isCanRun == false)
                {
                    textqueue_lock.unlock();
					WRITE_LOG("stop sending text thread: 0x%p", QThread::currentThreadId());
                    return;
                }
            }
        }

        M text = textqueue.front();

//        qDebug() << "取出队列:" << QThread::currentThreadId();

        textqueue.pop_front();
        textqueue_lock.unlock();//解锁
        queue_waitCond.wakeOne(); //唤醒添加线程

        //构造消息体
        MESG* send = (MESG*)malloc(sizeof(MESG));
        if (send == NULL)
        {
            WRITE_LOG("malloc error");
            qDebug() << __FILE__  <<__LINE__ << "malloc fail";
            continue;
        }
        else
        {
			memset(send, 0, sizeof(MESG));

			if (text.type == CREATE_MEETING || text.type == CLOSE_CAMERA)
			{
				send->len = 0;
				send->data = NULL;
				send->msg_type = text.type;
                queue_send.push_msg(send);
			}
			else if (text.type == JOIN_MEETING)
			{
				send->msg_type = JOIN_MEETING;
				send->len = 4; //房间号占4个字节
                send->data = (uchar*)malloc(send->len + 10);
                
                if (send->data == NULL)
                {
                    WRITE_LOG("malloc error");
                    qDebug() << __FILE__ << __LINE__ << "malloc fail";
                    free(send);
                    continue;
                }
                else
                {
                    memset(send->data, 0, send->len + 10);
					quint32 roomno = text.str.toUInt();
					memcpy(send->data, &roomno, sizeof(roomno));
					//加入发送队列

					queue_send.push_msg(send);
                }
			}
            else if(text.type == TEXT_SEND)
            {
                send->msg_type = TEXT_SEND;
                QByteArray data = qCompress(QByteArray::fromStdString(text.str.toStdString())); //压缩
                send->len = data.size();
                send->data = (uchar *) malloc(send->len);
                if(send->data == NULL)
                {
                    WRITE_LOG("malloc error");
                    qDebug() << __FILE__ << __LINE__ << "malloc error";
                    free(send);
                    continue;
                }
                else
                {
                    memset(send->data, 0, send->len);
                    memcpy_s(send->data, send->len, data.data(), data.size());
                    queue_send.push_msg(send);
                }
            }
        }
    }
}
void SendText::stopImmediately()
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;
}
