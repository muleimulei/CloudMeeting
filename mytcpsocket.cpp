#include "mytcpsocket.h"
#include "netheader.h"
#include <QHostAddress>
#include <QtEndian>
#include <QMetaObject>

extern QUEUE_SEND queue_send;
extern QUEUE_RECV queue_recv;

MyTcpSocket::MyTcpSocket()
{
    qDebug() << "hello " << QThread::currentThread();
    _socktcp = new QTcpSocket();
    _sockThread = new  QThread();
    this->moveToThread(_sockThread);
    sendbuf =(uchar *) malloc(2 * MB);
    connect(_socktcp, SIGNAL(readyRead()), this, SLOT(recvFromSocket())); //接受数据
    connect(_socktcp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorDetect(QAbstractSocket::SocketError)));
    qRegisterMetaType<QAbstractSocket::SocketError>();
}

void MyTcpSocket::errorDetect(QAbstractSocket::SocketError error)
{
    emit socketerror(error);
}

void MyTcpSocket::run()
{


    quint64 bytestowrite = 0;
    //构造消息头
    sendbuf[0] = '$';
    quint32 ip = _socktcp->localAddress().toIPv4Address();
    qToBigEndian<quint32>(ip,  sendbuf + 1);

    // $_IPV4_MSGType_MSGSize_# // 12
    // 1 4 2 4 1
    // 底层写数据线程
    for(;;)
    {
        bytestowrite = 5;
        //构造消息体
        queue_send.send_queueLock.lock();
        while(queue_send.send_queue.size() == 0)
        {
            queue_send.send_queueCond.wait(&queue_send.send_queueLock);
        }
        MESG * send = queue_send.send_queue.front();
        queue_send.send_queue.pop_front();
        queue_send.send_queueLock.unlock();
        queue_send.send_queueCond.wakeAll();

        if(send->msg_type == CREATE_MEETING)
        {
            qToBigEndian<quint16>(CREATE_MEETING, sendbuf + bytestowrite);
            bytestowrite += 2;

            qToBigEndian<quint32>(bytestowrite + 5, sendbuf + bytestowrite);
            sendbuf[bytestowrite+4] = '#';
            bytestowrite += 5;
        }

        qint64 hastowrite = bytestowrite;
        qint64 ret = 0, haswrite = 0;
        while((ret = _socktcp->write((char *)sendbuf + haswrite, hastowrite - haswrite)) < hastowrite )
        {
            if(ret == -1 && _socktcp->error() == QAbstractSocket::TemporaryError)
            {
                ret = 0;
                continue;
            }
            else if(ret == -1)
            {
                qDebug() << "network error";
                break;
            }
            haswrite += ret;
            hastowrite -= ret;
        }

        _socktcp->waitForBytesWritten();

        if(send->msg_type == CREATE_MEETING)
        {
            delete send;
        }
    }
}

void MyTcpSocket::recvFromSocket()
{
    char buf[100];
    _socktcp->read(buf, 100);
    qDebug() << QString(buf);
}

MyTcpSocket::~MyTcpSocket()
{

}

bool MyTcpSocket::connectToServer(QString ip, QString port, QIODevice::OpenModeFlag flag)
{
    _socktcp->connectToHost(ip, port.toUShort(), flag);

    if(_socktcp->waitForConnected(5000))
    {
        this->start() ; //写数据
        _sockThread->start(); //读数据
        return true;
    }
    return false;
}

QString MyTcpSocket::errorString()
{
    return _socktcp->errorString();
}

void MyTcpSocket::disconnectFromHost()
{
    if(this->isRunning()) // read
    {
        this->quit();
    }
    if(_sockThread->isRunning()) //write
    {
        _sockThread->quit();
    }

    //清空 发送 队列，清空接受队列
    queue_send.send_queueLock.lock();
    queue_send.send_queue.clear();
    queue_send.send_queueLock.unlock();


    queue_recv.recv_queueLock.lock();
    queue_recv.recv_queue.clear();
    queue_recv.recv_queueLock.unlock();

}
