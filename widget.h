#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QVideoFrame>
#include <QTcpSocket>
#include "mytcpsocket.h"
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class QCamera;
class QCameraImageCapture;
class MyVideoSurface;
class SendImg;

class Widget : public QWidget
{
    Q_OBJECT
private:
    static QRect pos;
    QCamera *_camera; //摄像头
    QCameraImageCapture *_imagecapture; //截屏
    bool  _createmeet; //是否创建会议
    bool _openCamera; //是否开启摄像头

    MyVideoSurface *_myvideosurface;

    QVideoFrame mainshow;

    SendImg *_sendImg;
    QThread *_imgThread;

//    QThre
    MyTcpSocket *_mytcpsocket;
    QThread *_recvThread;
    void paintEvent(QPaintEvent *event);
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_createmeetBtn_clicked();
    void on_exitmeetBtn_clicked();

    void on_openVedio_clicked();

    void on_connServer_clicked();
    void cameraError();
private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
