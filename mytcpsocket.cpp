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
    sendbuf[bytestowrite++] = '$';
    /*
    *$_MSGType_IPV4_MSGSize_data_# //
    * 1 2 4 4 len(MSGSize) 1
    *底层写数据线程
    */
    for(;;)
    {

        //构造消息体
        MESG * send = queue_send.pop_msg();

        if(send->msg_type == CREATE_MEETING)
        {
            qToBigEndian<quint16>(CREATE_MEETING, sendbuf + bytestowrite);
            bytestowrite += 2;

            quint32 ip = _socktcp->localAddress().toIPv4Address();
            qToBigEndian<quint32>(ip,  sendbuf + bytestowrite);
            bytestowrite += 4;


            qToBigEndian<quint32>(0, sendbuf + bytestowrite);
            bytestowrite += 4;
        }

        //将数据拷入sendbuf
        memcpy(sendbuf + bytestowrite, send->data, send->len);
        bytestowrite += send->len;
        sendbuf[bytestowrite++] = '#';

        //----------------write to server-------------------------
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

        //free
        if(send->msg_type == CREATE_MEETING)
        {
            delete send;
        }
    }
}



qint64 MyTcpSocket::readn(char * buf, quint64 maxsize, int n)
{
    quint64 hastoread = n;
    quint64 hasread = 0;
    do
    {
        qint64 ret  = _socktcp->read(buf + hasread, hastoread);
        if(ret < 0)
        {
            return -1;
        }
        if(ret == 0)
        {
            return hasread;
        }
        hasread += ret;
        hastoread -= ret;
    }while(hastoread > 0 && hasread < maxsize);
    return hasread;
}


void MyTcpSocket::recvFromSocket()
{
    /*
    *$_msgtype_ip_size_data_#
    *
    */
    char *buf = (char *)malloc(4 * MB);
    quint64 rlen = this->readn(buf, 4 * MB, 11); // 11 消息头长度
    if(rlen < 0)
    {
        return;
    }
    if(rlen < 11)
    {
        qDebug() << "data size < 11";
        free(buf);
        buf = NULL;
    }
//    qDebug() << "datalen = " << rlen;
    if(buf[0] == '$')
    {
        MSG_TYPE msgtype;
        qFromBigEndian<quint16>(buf + 1, 2, &msgtype);
        if(msgtype == CREATE_MEETING_RESPONSE)
        {
            quint32 datalen;
            qFromBigEndian<quint32>(buf + 7, 4, &datalen);

            //read data
            rlen = this->readn(buf, 4 * MB, datalen + 1); // read data+#
            if(rlen < datalen + 1)
            {
                qDebug() << "data size < datalen + 1";
                free(buf);
                return;
            }
            else if(buf[rlen - 1] == '#')
            {
                qint32 roomNo;
                qFromBigEndian<qint32>(buf, 4, &roomNo);

                //将消息加入到接收队列
                qDebug() << roomNo;

                MESG *msg = new MESG();
                msg->msg_type = CREATE_MEETING_RESPONSE;
                msg->data = (uchar *)malloc(datalen);
                memcpy(msg->data, &roomNo, datalen);
                msg->len = datalen;
                queue_recv.push_msg(msg);
            }
        }
    }
    else
    {
        qDebug() << "data format error";
        free(buf);
        buf = NULL;
    }

    //free

    if(buf)
    {
        free(buf);
        buf = NULL;
    }


}

MyTcpSocket::~MyTcpSocket()
{
    delete _socktcp;
    delete _sockThread;
    delete sendbuf;
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
    queue_send.clear();
    queue_recv.clear();

}
