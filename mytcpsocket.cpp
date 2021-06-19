#include "mytcpsocket.h"
#include "netheader.h"
#include <QHostAddress>
#include <QtEndian>
#include <QMetaObject>

extern QUEUE_SEND queue_send;
extern QUEUE_RECV queue_recv;

MyTcpSocket::MyTcpSocket()
{
   // qDebug() << "hello " << QThread::currentThread();
    _socktcp = new QTcpSocket();
    _sockThread = new  QThread();
    this->moveToThread(_sockThread);
    sendbuf =(uchar *) malloc(2 * MB);
    connect(_socktcp, SIGNAL(readyRead()), this, SLOT(recvFromSocket()), Qt::QueuedConnection); //接受数据
    connect(_socktcp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorDetect(QAbstractSocket::SocketError)), Qt::DirectConnection);
    qRegisterMetaType<QAbstractSocket::SocketError>();
}

void MyTcpSocket::errorDetect(QAbstractSocket::SocketError error)
{
    emit socketerror(error);
}

/*
 * 发送线程
 */
void MyTcpSocket::run()
{
    quint64 bytestowrite = 0;
    //构造消息头
    sendbuf[bytestowrite++] = '$';

    /*
    *$_MSGType_IPV4_MSGSize_data_# //
    * 1 2 4 4 MSGSize 1
    *底层写数据线程
    */
    for(;;)
    {
        bytestowrite = 1;
        //构造消息体
        MESG * send = queue_send.pop_msg();
        //消息类型
        qToBigEndian<quint16>(send->msg_type, sendbuf + bytestowrite);
        bytestowrite += 2;

        //发送者ip
        quint32 ip = _socktcp->localAddress().toIPv4Address();
        qToBigEndian<quint32>(ip,  sendbuf + bytestowrite);
        bytestowrite += 4;

        if(send->msg_type == CREATE_MEETING)
        {
            //发送数据大小
            qToBigEndian<quint32>(0, sendbuf + bytestowrite);
            bytestowrite += 4;
        }
        else if(send->msg_type == IMG_SEND)
        {
            //发送数据大小(多出来的2字节是图片类型, 宽度，高度)
            qToBigEndian<quint32>(send->len + 2 + 2 + 2, sendbuf + bytestowrite);
            bytestowrite += 4;
            qToBigEndian<quint16>(send->format, sendbuf + bytestowrite);
            bytestowrite += 2;
            qToBigEndian<quint32>(send->width, sendbuf + bytestowrite);
            bytestowrite += 4;
            qToBigEndian<quint32>(send->height, sendbuf + bytestowrite);
            bytestowrite += 4;
        }
        else if(send->msg_type == JOIN_MEETING)
        {
            qToBigEndian<quint32>(send->len, sendbuf + bytestowrite);
            bytestowrite += 4;
            qToBigEndian<quint32>(send->data, 4,  send->data);
        }

        //将数据拷入sendbuf
        memcpy(sendbuf + bytestowrite, send->data, send->len);
        bytestowrite += send->len;
        sendbuf[bytestowrite++] = '#'; //结尾字符

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
        if(send)
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
    char *buf = (char *)malloc(2 * MB);
    qint64 rlen = this->readn(buf, 2 * MB, 11); // 11 消息头长度
    if(rlen < 0)
    {
        free(buf);
        buf = NULL;
        return;
    }
    if(rlen < 11)
    {
        qDebug() << "data size < 11";
        free(buf);
        buf = NULL;
        return;
    }
//    qDebug() << "data_len = " << rlen;
    if(buf[0] == '$')
    {
        MSG_TYPE msgtype, msgtype_back; //不知道为什么下面调用qFromBigEndian使局部变量会改变，所以多准备一个back变量
        qFromBigEndian<quint16>(buf + 1, 2, &msgtype);
        msgtype_back = msgtype;
        qDebug() << "type "<<msgtype_back;
        if(msgtype == CREATE_MEETING_RESPONSE || msgtype == JOIN_MEETING_RESPONSE)
        {
            quint32 data_len=4, datalen_back;

            qFromBigEndian<quint32>(buf + 7, 4, &data_len);
            datalen_back = data_len;


            //read data
            rlen = this->readn(buf, 2 * MB, datalen_back + 1); // read data+#
            if(rlen < datalen_back + 1)
            {
                qDebug() << "data size < data_len + 1";
                free(buf);
                return;
            }
            else if(buf[rlen - 1] == '#')
            {
                if(msgtype_back == CREATE_MEETING_RESPONSE)
                {
                    qint32 roomNo;
                    qFromBigEndian<qint32>(buf, 4, &roomNo);

                    //将消息加入到接收队列
    //                qDebug() << roomNo;

                    MESG *msg = ( MESG *)malloc(sizeof(MESG));
                    memset(msg, 0, sizeof(sizeof (MESG)));
                    msg->msg_type = msgtype_back;
                    msg->data = (uchar *)malloc(datalen_back);
                    memcpy(msg->data, &roomNo, datalen_back);
                    msg->len = datalen_back;
                    queue_recv.push_msg(msg);
                }
                else if(msgtype_back == JOIN_MEETING_RESPONSE)
                {
                    char c;
                    memcpy(&c, buf, datalen_back);

                    MESG *msg = ( MESG *)malloc(sizeof(MESG));
                    memset(msg, 0, sizeof(sizeof (MESG)));
                    msg->msg_type = msgtype_back;
                    msg->data = (uchar *)malloc(datalen_back);
                    memcpy(msg->data, &c, datalen_back);
                    msg->len = datalen_back;
                    queue_recv.push_msg(msg);

                }
            }
        }
        else if(msgtype == IMG_RECV || msgtype == PARTNER_JOIN || msgtype == PARTNER_EXIT)
        {
            //read ipv4
            quint32 ip, ip_back;
            qFromBigEndian<quint32>(buf + 3, 4, &ip);
            ip_back = ip;

            //read datalen
            quint32 data_len, datalen_back;
            qFromBigEndian<quint32>(buf + 7, 4, &data_len);
            datalen_back = data_len;

            //read data
            rlen = this->readn(buf, 2 * MB, data_len + 1); // read data+#
            if(rlen < data_len + 1)
            {
                qDebug() << "data size < data_len + 1";
                free(buf);
                return;
            }
            else if(buf[rlen - 1] == '#')
            {
                if(msgtype_back == IMG_RECV )
                {
                    QImage::Format imageformat, imageformat_back;
                    qFromBigEndian<qint32>(buf, 2, &imageformat);
                    imageformat_back = imageformat;

                    quint32 width, width_back, height, height_back;
                    qFromBigEndian<quint32>(buf + 2, 4, &width);
                    width_back = width;

                    qFromBigEndian<quint32>(buf + 6, 4, &height);
                    height_back = height;

                    //将消息加入到接收队列
    //                qDebug() << roomNo;

                    MESG *msg = ( MESG *)malloc(sizeof(MESG));
                    memset(msg, 0, sizeof(sizeof (MESG)));
                    msg->msg_type = msgtype_back;
                    msg->format = imageformat_back;
                    msg->width = width_back;
                    msg->height = height_back;
                    msg->data = (uchar *)malloc(datalen_back - 10); // 10 = format + width + width
                    memcpy(msg->data, buf + 10, datalen_back - 10);
                    msg->len = datalen_back - 10;
                    queue_recv.push_msg(msg);
                }
                else if(msgtype_back == PARTNER_JOIN || msgtype_back == PARTNER_EXIT)
                {
                    MESG *msg = ( MESG *)malloc(sizeof(MESG));
                    memset(msg, 0, sizeof(sizeof (MESG)));

                    msg->msg_type = msgtype_back;
                    msg->ip = ip_back;
                    queue_recv.push_msg(msg);
                }
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
    disconnect(_socktcp, SIGNAL(readyRead()), this, SLOT(recvFromSocket()));
    disconnectFromHost();

    delete sendbuf;
    delete _sockThread;
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
    if(_socktcp->isOpen()) _socktcp->close();
    //清空 发送 队列，清空接受队列
    queue_send.clear();
    queue_recv.clear();
}


quint32 MyTcpSocket::getlocalip()
{
    if(_socktcp->isOpen())
    {
        return _socktcp->localAddress().toIPv4Address();
    }
    else
    {
        return -1;
    }
}
