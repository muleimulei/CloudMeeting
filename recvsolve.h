#ifndef RECVSOLVE_H
#define RECVSOLVE_H

#include <QThread>

class RecvSolve : public QThread
{
    Q_OBJECT
public:
    RecvSolve();
};

#endif // RECVSOLVE_H
