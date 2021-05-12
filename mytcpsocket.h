#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QThread>
#include <QTcpSocket>


#ifndef MB
#define MB (1024 * 1024)
#endif

typedef unsigned char uchar;
class MyTcpSocket: public QThread, public QTcpSocket
{
public:
    MyTcpSocket(QObject *);
private:
    void run() override;
private slots:
    void recvFromSocket();
};

#endif // MYTCPSOCKET_H
