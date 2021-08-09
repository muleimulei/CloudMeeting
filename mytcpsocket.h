#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QThread>
#include <QTcpSocket>
#include <QMutex>
#include "netheader.h"
#ifndef MB
#define MB (1024 * 1024)
#endif

typedef unsigned char uchar;


class MyTcpSocket: public QThread
{
    Q_OBJECT
public:
    ~MyTcpSocket();
    MyTcpSocket(QObject *par=NULL);
    bool connectToServer(QString, QString, QIODevice::OpenModeFlag);
    QString errorString();
    void disconnectFromHost();
    quint32 getlocalip();
private:
    void run() override;
    qint64 readn(char *, quint64, int);
    QTcpSocket *_socktcp;
    QThread *_sockThread;
    uchar *sendbuf;
    uchar* recvbuf;
    quint64 hasrecvive;

    QMutex m_lock;
    volatile bool m_isCanRun;
private slots:
    bool connectServer(QString, QString, QIODevice::OpenModeFlag);
    void sendData(MESG *);
    void closeSocket();

public slots:
    void recvFromSocket();
    void stopImmediately();
    void errorDetect(QAbstractSocket::SocketError error);
signals:
    void socketerror(QAbstractSocket::SocketError);
    void sendTextOver();
};

#endif // MYTCPSOCKET_H
