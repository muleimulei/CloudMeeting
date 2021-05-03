#ifndef NETHEADER_H
#define NETHEADER_H

#include <QMutex>
#include <QQueue>

#include <QImage>
enum MSG_TYPE
{
    IMG_SEND,
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


struct MSGSend //消息结构体
{
    MSG_TYPE msg_type;
    uchar* data;
    long len;
    IMG_FORMAT format;
};

struct QUEUE_SEND //发送队列
{
    QMutex send_queueLock;
    QQueue<MSGSend *> send_queue;
};

struct QUEUE_RECV //接收队列
{
    QMutex recv_queueLock;
    QQueue<MSGSend *> send_queue;
};

#endif // NETHEADER_H
