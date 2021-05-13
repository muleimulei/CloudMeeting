#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QThread>
#include <QTcpSocket>


#ifndef MB
#define MB (1024 * 1024)
#endif

typedef unsigned char uchar;
class MyTcpSocket: public QThread
{
    Q_OBJECT
public:
    ~MyTcpSocket();
    MyTcpSocket();
    bool connectToServer(QString, QString, QIODevice::OpenModeFlag);
    QString errorString();
    void disconnectFromHost();
private:
    void run() override;
    QTcpSocket *_socktcp;
    QThread *_sockThread;
    uchar *sendbuf;
public slots:
    void recvFromSocket();
};

#endif // MYTCPSOCKET_H
