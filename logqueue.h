#ifndef LOGQUEUE_H
#define LOGQUEUE_H

#include <QThread>
#include <QMutex>
#include <queue>
#include "netheader.h"

class LogQueue : public QThread
{
private:
    void run();
    QMutex m_lock;
    bool m_isCanRun;

    QUEUE_DATA<Log> log_queue;
    FILE *logfile;
public:
    explicit LogQueue(QObject *parent = nullptr);
    void stopImmediately();
    void pushLog(Log*);
};

#endif // LOGQUEUE_H
