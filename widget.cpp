#include "widget.h"
#include "ui_widget.h"
#include "screen.h"
#include "hintdialog.h"
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QDebug>
#include <QPainter>
#include "myvideosurface.h"

QRect  Widget::pos = QRect(-1, -1, -1, -1);

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    _createmeet = false;
    Widget::pos = QRect(0.1 * Screen::width, 0.1 * Screen::height, 0.8 * Screen::width, 0.8 * Screen::height);

    ui->setupUi(this);
    this->setGeometry(Widget::pos);

    this->setMinimumSize(QSize(Widget::pos.width() * 0.7, Widget::pos.height() * 0.7));
    this->setMaximumSize(QSize(Widget::pos.width(), Widget::pos.height()));


    ui->exitmeetBtn->setDisabled(true);


    //配置摄像头
    _camera = new QCamera(this);
    _imagecapture = new QCameraImageCapture(_camera);
    _myvideosurface = new MyVideoSurface(this);


    //处理每一帧数据
    connect(_myvideosurface, SIGNAL(frameAvailable(QVideoFrame)), this, SLOT(cameraImageCapture(QVideoFrame)));
    //预览窗口重定向在MyVideoSurface
    _camera->setViewfinder(_myvideosurface);
    _camera->setCaptureMode(QCamera::CaptureStillImage);
}

Widget::~Widget()
{
    delete _camera;
    delete _imagecapture;
    delete _myvideosurface;
    delete ui;
}

void Widget::cameraImageCapture(QVideoFrame frame)
{
    mainshow = frame;
    if(mainshow.isValid() && mainshow.isReadable())
    {
        QImage videoImg = QImage(mainshow.bits(), mainshow.width(), mainshow.height(), QVideoFrame::imageFormatFromPixelFormat(mainshow.pixelFormat()));

        QMatrix matrix;
        matrix.rotate(180.0);

        ui->mainshow_label->setPixmap(QPixmap::fromImage(videoImg.transformed(matrix)).scaled(ui->mainshow_label->size()));

        qDebug()<< "format: " <<  videoImg.format() << "size: " << videoImg.size() << "byteSIze: "<< videoImg.sizeInBytes();
//        QLabel *m = new QLabel();
//        m->setPixmap(QPixmap::fromImage(videoImg.transformed(matrix)).scaled(QSize(240, 240)));
//        ui->verticalLayout_3->addWidget(m);
    }
}

void Widget::on_createmeetBtn_clicked()
{
    if(false == _createmeet)
    {
        _camera->start();
        _createmeet = true;

        ui->createmeetBtn->setDisabled(true);
        ui->exitmeetBtn->setDisabled(false);
    }
}

void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    /*
     * 触发事件(3条， 一般使用第二条进行触发)
     * 1. 窗口部件第一次显示时，系统会自动产生一个绘图事件。从而强制绘制这个窗口部件，主窗口起来会绘制一次
     * 2. 当重新调整窗口部件的大小时，系统也会产生一个绘制事件--QWidget::update()或者QWidget::repaint()
     * 3. 当窗口部件被其它窗口部件遮挡，然后又再次显示出来时，就会对那些隐藏的区域产生一个绘制事件
    */
}

void Widget::on_exitmeetBtn_clicked()
{
    if(_createmeet)
    {
        _camera->stop();
        ui->createmeetBtn->setDisabled(false);
        ui->exitmeetBtn->setDisabled(true);
        _createmeet = false;
    }
}
