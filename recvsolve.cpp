#include "recvsolve.h"
#include <QMetaType>
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
    }
}
