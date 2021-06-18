#ifndef SENDTEXT_H
#define SENDTEXT_H

#include <QThread>
#include <QMutex>
#include <QQueue>
#include "netheader.h"

struct M
{
    QString str;
    MSG_TYPE type;

    M(QString s, MSG_TYPE e)
    {
        str = s;
        type = e;
    }
};

//发送文本数据
class SendText : public QThread
{
    Q_OBJECT
private:
    QQueue<M> textqueue;
    QMutex textqueue_lock; //队列锁
    QWaitCondition queue_waitCond;
    void run() override;
public:
    SendText();
    ~SendText();
private slots:
    void push_Text(MSG_TYPE, QString str = "");
};

#endif // SENDTEXT_H
