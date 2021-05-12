#include "mytcpsocket.h"

MyTcpSocket::MyTcpSocket(QObject *parent): QThread(parent), QTcpSocket(parent)
{
    QObject::connect((QIODevice *)this, SIGNAL(readyRead), (QIODevice *)this, SLOT(recvFromSocket));
}


void MyTcpSocket::run()
{
    uchar * sendbuf =(uchar *) malloc(2 * MB);
    // 底层写数据线程
    for(;;)
    {

    }
}

void MyTcpSocket::recvFromSocket()
{

}
