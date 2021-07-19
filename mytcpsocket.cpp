#include "mytcpsocket.h"
#include "netheader.h"
#include <QHostAddress>
#include <QtEndian>
#include <QMetaObject>
#include <QMutexLocker>

extern QUEUE_SEND queue_send;
extern QUEUE_RECV queue_recv;

void MyTcpSocket::stopImmediately()
{
    {
        QMutexLocker lock(&m_lock);
        if(m_isCanRun == true) m_isCanRun = false;
    }
    //关闭read
    _sockThread->quit();
    _sockThread->wait();
}


MyTcpSocket::MyTcpSocket(QObject *par):QThread(par)
{
    qRegisterMetaType<QAbstractSocket::SocketError>();
    _socktcp = new QTcpSocket(); //tcp

    _sockThread = new QThread(); //发送数据线程
    this->moveToThread(_sockThread);

    sendbuf =(uchar *) malloc(2 * MB);
    recvbuf = (uchar*)malloc(2 * MB);
    connect(_socktcp, SIGNAL(readyRead()), this, SLOT(recvFromSocket())); //接受数据

    //处理套接字错误
    connect(_socktcp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorDetect(QAbstractSocket::SocketError)));

}


void MyTcpSocket::errorDetect(QAbstractSocket::SocketError error)
{
    qDebug() <<"Sock error" <<QThread::currentThreadId();
    MESG * msg = (MESG *) malloc(sizeof (MESG));
    memset(msg, 0, sizeof (MESG));
    if(error == QAbstractSocket::RemoteHostClosedError)
    {
        msg->msg_type = RemoteHostClosedError;
    }
    else
    {
        msg->msg_type = OtherNetError;
    }
    queue_recv.push_msg(msg);
}

/*
 * 发送线程
 */
void MyTcpSocket::run()
{
    qDebug() << "send data" << QThread::currentThreadId();
    m_isCanRun = true; //标记可以运行
    /*
    *$_MSGType_IPV4_MSGSize_data_# //
    * 1 2 4 4 MSGSize 1
    *底层写数据线程
    */
    for(;;)
    {
        {
            QMutexLocker locker(&m_lock);
            if(m_isCanRun == false) return; //在每次循环判断是否可以运行，如果不行就退出循环
        }
        quint64 bytestowrite = 0;
        //构造消息头
        sendbuf[bytestowrite++] = '$';
        bytestowrite = 1;
        //构造消息体
        MESG * send = queue_send.pop_msg();
        if(send == NULL) continue;
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
            qDebug() << "send img";
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

        if(send->data)
        {
            free(send->data);
        }
        //free
        if(send)
        {
            free(send);
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
    qDebug() << "recv data socket" <<QThread::currentThread();
    /*
    *$_msgtype_ip_size_data_#
    *
    */
    
    qint64 rlen = this->readn((char *)recvbuf, 2 * MB, 11); // 11 消息头长度
    if(rlen < 0)
    {
        qDebug() << "rlen < 0";
        return;
    }
    if(rlen < 11)
    {
        qDebug() << "data size < 11";
        return;
    }
//    qDebug() << "data_len = " << rlen;
    if(recvbuf[0] == '$')
    {
        MSG_TYPE msgtype, msgtype_back; //不知道为什么下面调用qFromBigEndian使局部变量会改变，所以多准备一个back变量
        qFromBigEndian<quint16>(recvbuf + 1, 2, &msgtype);
        msgtype_back = msgtype;
        if(msgtype == CREATE_MEETING_RESPONSE || msgtype == JOIN_MEETING_RESPONSE)
        {
            quint32 data_len=4, datalen_back;

            qFromBigEndian<quint32>(recvbuf + 7, 4, &data_len);
            datalen_back = data_len;


            //read data
            rlen = this->readn((char *)recvbuf, 2 * MB, datalen_back + 1); // read data+#
            if(rlen < datalen_back + 1)
            {
                qDebug() << "data size < data_len + 1";
                return;
            }
            else if(recvbuf[rlen - 1] == '#')
            {
                if(msgtype_back == CREATE_MEETING_RESPONSE)
                {
                    qint32 roomNo;
                    qFromBigEndian<qint32>(recvbuf, 4, &roomNo);

                    //将消息加入到接收队列
    //                qDebug() << roomNo;

                    MESG *msg = ( MESG *)malloc(sizeof(MESG));
                    if (msg == NULL)
                    {
                        qDebug() << __LINE__ << " malloc failed";
                    }
                    else
                    {
						memset(msg, 0, sizeof(sizeof(MESG)));
						msg->msg_type = msgtype_back;
						msg->data = (uchar*)malloc(datalen_back);
                        if (msg->data == NULL)
                        {
                            qDebug() << __LINE__ << " malloc failed";
                        }
                        else
                        {
							memcpy(msg->data, &roomNo, datalen_back);
							msg->len = datalen_back;
							queue_recv.push_msg(msg);
                        }
						
                    }
                }
                else if(msgtype_back == JOIN_MEETING_RESPONSE)
                {
                    char c;
                    memcpy(&c, recvbuf, datalen_back);

                    MESG *msg = ( MESG *)malloc(sizeof(MESG));

                    if (msg == NULL)
                    {
                        qDebug() << __LINE__ << " malloc failed";
                    }
                    else
                    {
						memset(msg, 0, sizeof(sizeof(MESG)));
						msg->msg_type = msgtype_back;
						msg->data = (uchar*)malloc(datalen_back);
                        if (msg->data == NULL)
                        {
                            qDebug() << __LINE__ << " malloc failed";
                        }
                        else
                        {
							memcpy(msg->data, &c, datalen_back);
							msg->len = datalen_back;
							queue_recv.push_msg(msg);
                        }
                    }
                }
            }
        }
        else if(msgtype == IMG_RECV || msgtype == PARTNER_JOIN || msgtype == PARTNER_EXIT)
        {
            //read ipv4
            quint32 ip, ip_back;
            qFromBigEndian<quint32>(recvbuf + 3, 4, &ip);
            ip_back = ip;

            //read datalen
            quint32 data_len, datalen_back;
            qFromBigEndian<quint32>(recvbuf + 7, 4, &data_len);
            datalen_back = data_len;

            //read data
            rlen = this->readn((char *)recvbuf, 2 * MB, data_len + 1); // read data+#
            if(rlen < data_len + 1)
            {
                qDebug() << "data size < data_len + 1";
                return;
            }
            else if(recvbuf[rlen - 1] == '#')
            {
                if(msgtype_back == IMG_RECV )
                {
                    QImage::Format imageformat, imageformat_back;
                    qFromBigEndian<qint32>(recvbuf, 2, &imageformat);
                    imageformat_back = imageformat;

                    quint32 width, width_back, height, height_back;
                    qFromBigEndian<quint32>(recvbuf + 2, 4, &width);
                    width_back = width;

                    qFromBigEndian<quint32>(recvbuf + 6, 4, &height);
                    height_back = height;

                    //将消息加入到接收队列
    //                qDebug() << roomNo;

                    MESG *msg = ( MESG *)malloc(sizeof(MESG));
                    if (msg == NULL)
                    {
                        qDebug() << __LINE__ << " malloc failed";
                    }
                    else
                    {
						memset(msg, 0, sizeof(sizeof(MESG)));
						msg->msg_type = msgtype_back;
						msg->format = imageformat_back;
						msg->width = width_back;
						msg->height = height_back;
						msg->data = (uchar*)malloc(datalen_back - 10); // 10 = format + width + width
                        if (msg->data == NULL)
                        {
                            qDebug() << __LINE__ << " malloc failed";
                        }
                        else
                        {
							memcpy(msg->data, recvbuf + 10, datalen_back - 10);
							msg->len = datalen_back - 10;
							queue_recv.push_msg(msg);
                        }
						
                    }
                }
                else if(msgtype_back == PARTNER_JOIN || msgtype_back == PARTNER_EXIT)
                {
                    MESG *msg = ( MESG *)malloc(sizeof(MESG));
                    if (msg == NULL)
                    {
                        qDebug() << __LINE__ <<" malloc failed";
                    }
                    else
                    {
						memset(msg, 0, sizeof(sizeof(MESG)));
						msg->msg_type = msgtype_back;
						msg->ip = ip_back;
						queue_recv.push_msg(msg);
                    }
                }
            }
        }
    }
    else
    {
        qDebug() << "data format error";
    }
}


MyTcpSocket::~MyTcpSocket()
{
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
    //write
    if(this->isRunning())
    {
        QMutexLocker locker(&m_lock);
        m_isCanRun = false;
    }

    if(_sockThread->isRunning()) //read
    {
        _sockThread->quit();
        _sockThread->wait();
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
