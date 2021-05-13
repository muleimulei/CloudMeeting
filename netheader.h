#ifndef NETHEADER_H
#define NETHEADER_H

#include <QMutex>
#include <QQueue>
#include <QImage>
#include <QWaitCondition>
#define QUEUE_MAXSIZE 1500
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
    JOIN_MEETING
};

enum IMG_FORMAT
{
    Format_Invalid = 0,
    Format_RGB32 = 1,
    Format_ARGB32 = 2
};

struct MESG //消息结构体
{
    MSG_TYPE msg_type;
    uchar* data;
    long len;
    IMG_FORMAT format;
};


//-------------------------------
struct QUEUE_SEND //发送队列
{
    QMutex send_queueLock;
    QWaitCondition send_queueCond;
    QQueue<MESG *> send_queue;
};

struct QUEUE_RECV //接收队列
{
    QMutex recv_queueLock;
    QWaitCondition recv_queueCond;
    QQueue<MESG *> recv_queue;
};

#endif // NETHEADER_H
