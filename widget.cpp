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
#include "sendimg.h"
#include <QRegExp>
#include <QRegExpValidator>
#include <QMessageBox>

QRect  Widget::pos = QRect(-1, -1, -1, -1);

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{

    qDebug() << QThread::currentThreadId();
    //ui界面
    _createmeet = false;
    _openCamera = false;
    Widget::pos = QRect(0.1 * Screen::width, 0.1 * Screen::height, 0.8 * Screen::width, 0.8 * Screen::height);



    ui->setupUi(this);
    this->setGeometry(Widget::pos);
    this->setMinimumSize(QSize(Widget::pos.width() * 0.7, Widget::pos.height() * 0.7));
    this->setMaximumSize(QSize(Widget::pos.width(), Widget::pos.height()));
    ui->exitmeetBtn->setDisabled(true);
    ui->joinmeetBtn->setDisabled(true);
    ui->createmeetBtn->setDisabled(true);
    ui->openAudio->setDisabled(true);
    ui->openVedio->setDisabled(true);

    //创建传输视频帧线程
    _sendImg = new SendImg();
    _imgThread = new QThread();
    _sendImg->moveToThread(_imgThread); //新起线程接受视频帧
    _sendImg->start();
    //_imgThread->start();





    //配置摄像头
    _camera = new QCamera(this);
    //摄像头出错处理
    connect(_camera, SIGNAL(error), this, SLOT(cameraError));
    _imagecapture = new QCameraImageCapture(_camera);
    _myvideosurface = new MyVideoSurface(this);


    //处理每一帧数据
    connect(_myvideosurface, SIGNAL(frameAvailable(QVideoFrame)), _sendImg, SLOT(cameraImageCapture(QVideoFrame)));

    //监听_imgThread退出信号
    connect(_imgThread, SIGNAL(finished()), _sendImg, SLOT(clearImgQueue()));

    //创建TCPsocket
    _mytcpsocket = new MyTcpSocket(this);
    _recvThread = new QThread();
    _recvThread->moveToThread(_mytcpsocket); // _recvThread 接收 _mytcpsocket 从套接字 获取的 数据





    //预览窗口重定向在MyVideoSurface
    _camera->setViewfinder(_myvideosurface);
    _camera->setCaptureMode(QCamera::CaptureStillImage);
}

Widget::~Widget()
{
    delete _recvThread;
    delete _mytcpsocket;
    delete _camera;
    delete _imagecapture;
    delete _myvideosurface;
    delete _imgThread;
    delete _sendImg;
    delete ui;
}

void Widget::on_createmeetBtn_clicked()
{
    if(false == _createmeet)
    {
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


//退出会议（1，加入的会议， 2，自己创建的会议）
void Widget::on_exitmeetBtn_clicked()
{

    ui->createmeetBtn->setDisabled(true);
    ui->exitmeetBtn->setDisabled(false);
    _createmeet = false;
    //-----------------------------------------
    // 关闭套接字
    _mytcpsocket->disconnectFromHost();


    //关闭各个启动线程
    _imgThread->quit();
    _sendImg->quit();

    //关闭TCP发送与传输
    _mytcpsocket->quit();
    _recvThread->quit();
    //-----------------------------------------
}

void Widget::on_openVedio_clicked()
{
    if(_camera->status() == QCamera::ActiveStatus)
    {
        _camera->stop();
        if(_camera->error() == QCamera::NoError)
        {
            _imgThread->quit();
            ui->openVedio->setText("开启摄像头");
        }
    }else
    {
        _camera->start(); //开启摄像头
        if(_camera->error() == QCamera::NoError)
        {
            _imgThread->start();
            ui->openVedio->setText("关闭摄像头");
        }
    }
}

void Widget::on_connServer_clicked()
{
    QString ip = ui->ip->text(), port = ui->port->text();
    ui->outlog->setText("正在连接到" + ip + ":" + port);
    repaint();

    QRegExp ipreg("((2{2}[0-3]|2[01][0-9]|1[0-9]{2}|0?[1-9][0-9]|0{0,2}[1-9])\\.)((25[0-5]|2[0-4][0-9]|[01]?[0-9]{0,2})\\.){2}(25[0-5]|2[0-4][0-9]|[01]?[0-9]{1,2})");

    QRegExp portreg("^([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])$");
    QRegExpValidator ipvalidate(ipreg), portvalidate(portreg);
    int pos = 0;
    if(ipvalidate.validate(ip, pos) != QValidator::Acceptable)
    {
        QMessageBox::warning(this, "Input Error", "Ip Error", QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    if(portvalidate.validate(port, pos) != QValidator::Acceptable)
    {
        QMessageBox::warning(this, "Input Error", "Port Error", QMessageBox::Yes, QMessageBox::Yes);
        return;
    }

    _mytcpsocket->connectToHost(ip, port.toUInt(), QIODevice::ReadWrite);

    if(_mytcpsocket->waitForConnected(5000))
    {
        ui->outlog->setText("成功连接到" + ip + ":" + port);
        ui->openAudio->setDisabled(false);
        ui->openVedio->setDisabled(false);
        ui->createmeetBtn->setDisabled(false);
        ui->exitmeetBtn->setDisabled(false);

        QMessageBox::warning(this, "Connection success", "成功连接服务器" , QMessageBox::Yes, QMessageBox::Yes);

        _mytcpsocket->start(); //开启底层发送线程
        _recvThread->start();//开启底层接受线程
    }
    else
    {
        ui->outlog->setText("连接失败,请重新连接...");
        QMessageBox::warning(this, "Connection error", _mytcpsocket->errorString() , QMessageBox::Yes, QMessageBox::Yes);
    }
}


void Widget::cameraError()
{
    QMessageBox::warning(this, "Camera error", _camera->errorString() , QMessageBox::Yes, QMessageBox::Yes);
}
