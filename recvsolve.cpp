#include "recvsolve.h"
#include <QMetaType>
#include <QDebug>
extern QUEUE_RECV queue_recv;


RecvSolve::RecvSolve()
{
    qRegisterMetaType<MESG *>();
}


void RecvSolve::run()
{
    for(;;)
    {
        MESG * msg = queue_recv.pop_msg();
        emit datarecv(msg);
        qDebug() << "取出队列:" << QThread::currentThreadId();
    }
}


RecvSolve::~RecvSolve()
{
    if(this->isRunning())
    {
        this->terminate();
        this->quit();
        this->wait();
    }
}
