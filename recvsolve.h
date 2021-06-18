#ifndef RECVSOLVE_H
#define RECVSOLVE_H
#include "netheader.h"
#include <QThread>

/*
 * 接收线程
 *
 */
class RecvSolve : public QThread
{
    Q_OBJECT
public:
    RecvSolve();
    ~RecvSolve();
    void run() override;
signals:
    void datarecv(MESG *);
};

#endif // RECVSOLVE_H
