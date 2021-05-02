#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QVideoFrame>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class QCamera;
class QCameraImageCapture;
class MyVideoSurface;

class Widget : public QWidget
{
    Q_OBJECT
private:
    static QRect pos;
    QCamera *_camera; //摄像头
    QCameraImageCapture *_imagecapture; //截屏
    bool  _createmeet; //是否创建会议
    MyVideoSurface *_myvideosurface;

    QVideoFrame mainshow;


    void paintEvent(QPaintEvent *event);
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_createmeetBtn_clicked();
    void cameraImageCapture(QVideoFrame);

    void on_exitmeetBtn_clicked();

private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
