#ifndef NETHEADER_H
#define NETHEADER_H
#include <QMetaType>
#include <QMutex>
#include <QQueue>
#include <QImage>
#include <QWaitCondition>
#define QUEUE_MAXSIZE 1500
#ifndef MB
#define MB 1024*1024
#endif

#ifndef WAITSECONDS
#define WAITSECONDS 2
#endif
enum MSG_TYPE
{
    IMG_SEND = 0,
    IMG_RECV,
    AUDIO_SEND,
    AUDIO_RECV,
    TEXT_SEND,
    TEXT_RECV,
    CREATE_MEETING,
    EXIT_MEETING,
    JOIN_MEETING,

    CREATE_MEETING_RESPONSE = 20,
    PARTNER_EXIT = 21,
    PARTNER_JOIN = 22,
    JOIN_MEETING_RESPONSE = 23,
    PARTNER_JOIN2 = 24,
    RemoteHostClosedError = 40,
    OtherNetError = 41
};
Q_DECLARE_METATYPE(MSG_TYPE);

struct MESG //消息结构体
{
    MSG_TYPE msg_type;
    uchar* data;
    long len;
    QImage::Format format;
    quint32 ip;
    int width;
    int height;
};
Q_DECLARE_METATYPE(MESG *);

//-------------------------------
struct QUEUE_SEND //发送队列
{
private:
    QMutex send_queueLock;
    QWaitCondition send_queueCond;
    QQueue<MESG *> send_queue;
public:
    void push_msg(MESG * msg)
    {
        send_queueLock.lock();
        while(send_queue.size() > QUEUE_MAXSIZE)
        {
            send_queueCond.wait(&send_queueLock);
        }
        send_queue.push_back(msg);
        send_queueLock.unlock();
        send_queueCond.wakeOne();
    }

    MESG* pop_msg()
    {
        send_queueLock.lock();
        while(send_queue.size() == 0)
        {
            bool f = send_queueCond.wait(&send_queueLock, WAITSECONDS * 1000);
            if(f == false)
            {
                send_queueLock.unlock();
                return NULL;
            }
        }
        MESG * send = send_queue.front();
        send_queue.pop_front();
        send_queueLock.unlock();
        send_queueCond.wakeOne();
        return send;
    }

    void clear()
    {
        send_queueLock.lock();
        send_queue.clear();
        send_queueLock.unlock();
    }
};

struct QUEUE_RECV //接收队列
{
private:
    QMutex recv_queueLock;
    QWaitCondition recv_queueCond;
    QQueue<MESG *> recv_queue;
public:
    void push_msg(MESG * msg)
    {
        recv_queueLock.lock();
        while(recv_queue.size() > QUEUE_MAXSIZE)
        {
            recv_queueCond.wait(&recv_queueLock);
        }
        recv_queue.push_back(msg);
        recv_queueLock.unlock();
        recv_queueCond.wakeOne();
    }

    MESG* pop_msg()
    {
        recv_queueLock.lock();
        while(recv_queue.size() == 0)
        {
            bool f = recv_queueCond.wait(&recv_queueLock, WAITSECONDS * 1000);
            if(f == false)
            {
                recv_queueLock.unlock();
                return NULL;
            }
        }
        MESG * send = recv_queue.front();
        recv_queue.pop_front();
        recv_queueLock.unlock();
        recv_queueCond.wakeOne();
        return send;
    }

    void clear()
    {
        recv_queueLock.lock();
        recv_queue.clear();
        recv_queueLock.unlock();
    }
};

#endif // NETHEADER_H
