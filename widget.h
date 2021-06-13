#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QVideoFrame>
#include <QTcpSocket>
#include "mytcpsocket.h"
#include <QCamera>
#include "sendtext.h"
#include "recvsolve.h"
#include "partner.h"
#include <QMap>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class QCameraImageCapture;
class MyVideoSurface;
class SendImg;

class Widget : public QWidget
{
    Q_OBJECT
private:
    static QRect pos;
    QSize mainlabel_size; //主屏幕大小
    quint32 mainip; //主屏幕显示的IP图像
    QCamera *_camera; //摄像头
    QCameraImageCapture *_imagecapture; //截屏
    bool  _createmeet; //是否创建会议
    bool _openCamera; //是否开启摄像头

    MyVideoSurface *_myvideosurface;

    QVideoFrame mainshow;

    SendImg *_sendImg;
    QThread *_imgThread;

    RecvSolve *_recvThread;
    SendText * _sendText;
    //socket
    MyTcpSocket * _mytcpSocket;
    void paintEvent(QPaintEvent *event);

    QMap<quint32, Partner *> partner;
    Partner* addPartner(quint32);
    void removePartner(quint32);

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_createmeetBtn_clicked();
    void on_exitmeetBtn_clicked();

    void on_openVedio_clicked();
    void on_connServer_clicked();
    void cameraError(QCamera::Error);
    void mytcperror(QAbstractSocket::SocketError);
    void datasolve(MESG *);
    void recvip(quint32);
private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
