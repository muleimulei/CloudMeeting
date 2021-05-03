#ifndef NETSEND_H
#define NETSEND_H

#include <QThread>

class NetSend: public QThread
{
public:
    NetSend();
    void run() override;
};

#endif // NETSEND_H
