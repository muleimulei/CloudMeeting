#include "recvsolve.h"
#include <QMetaType>
#include <QDebug>
#include <QMutexLocker>
extern QUEUE_DATA<MESG> queue_recv;

void RecvSolve::stopImmediately()
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;
}

RecvSolve::RecvSolve(QObject *par):QThread(par)
{
    qRegisterMetaType<MESG *>();
    m_isCanRun = true;
}

void RecvSolve::run()
{
    for(;;)
    {
        {
            QMutexLocker locker(&m_lock);
            if(m_isCanRun == false) return;
        }
        MESG * msg = queue_recv.pop_msg();
        if(msg == NULL) continue;
		/*else free(msg);
		qDebug() << "取出队列:" << msg->msg_type;*/
        emit datarecv(msg);
    }
}
