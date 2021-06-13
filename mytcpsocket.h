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
    quint32 getlocalip();
private:
    void run() override;
    qint64 readn(char *, quint64, int);
    QTcpSocket *_socktcp;
    QThread *_sockThread;
    uchar *sendbuf;
private slots:
    void errorDetect(QAbstractSocket::SocketError);
public slots:
    void recvFromSocket();

signals:
    void socketerror(QAbstractSocket::SocketError);
};

#endif // MYTCPSOCKET_H
