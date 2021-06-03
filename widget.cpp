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
    //创建TCPsocket

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
    //ui->openVedio->setDisabled(true);

    //创建传输视频帧线程
    _sendImg = new SendImg();
    _imgThread = new QThread();
    _sendImg->moveToThread(_imgThread); //新起线程接受视频帧
    //_imgThread->start();


    //数据处理
    _mytcpSocket = new MyTcpSocket(); // 专管发送
    connect(_mytcpSocket, SIGNAL(socketerror(QAbstractSocket::SocketError)), this, SLOT(mytcperror(QAbstractSocket::SocketError)));

    //文本传输
    _sendText = new SendText();

    //配置摄像头
    _camera = new QCamera(this);
    //摄像头出错处理
    connect(_camera, SIGNAL(error(QCamera::Error)), this, SLOT(cameraError(QCamera::Error)));
    _imagecapture = new QCameraImageCapture(_camera);
    _myvideosurface = new MyVideoSurface(this);


    //处理每一帧数据
    connect(_myvideosurface, SIGNAL(frameAvailable(QVideoFrame)), _sendImg, SLOT(cameraImageCapture(QVideoFrame)));

    //监听_imgThread退出信号
    connect(_imgThread, SIGNAL(finished()), _sendImg, SLOT(clearImgQueue()));


    //预览窗口重定向在MyVideoSurface
    _camera->setViewfinder(_myvideosurface);
    _camera->setCaptureMode(QCamera::CaptureStillImage);
}

Widget::~Widget()
{
    delete _mytcpSocket;
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
        ui->createmeetBtn->setDisabled(true);
        ui->openAudio->setDisabled(true);
        ui->openVedio->setDisabled(true);
        ui->exitmeetBtn->setDisabled(true);
        _sendText->push_Text(CREATE_MEETING); //将 “创建会议"加入到发送队列


//        _createmeet = true;
//        ui->createmeetBtn->setDisabled(true);
//        ui->exitmeetBtn->setDisabled(false);
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

    if(_camera->status() == QCamera::ActiveStatus)
    {
        _camera->stop();
    }

    ui->createmeetBtn->setDisabled(true);
    ui->exitmeetBtn->setDisabled(false);
    _createmeet = false;
    //-----------------------------------------
    // 关闭套接字

    //关闭各个启动线程
    _imgThread->quit();
    _sendImg->quit();


    //关闭socket
    _mytcpSocket->disconnectFromHost();



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
            _sendImg->quit();
            ui->openVedio->setText("开启摄像头");
        }
    }
    else
    {
        _camera->start(); //开启摄像头
        if(_camera->error() == QCamera::NoError)
        {
            _imgThread->start();
            _sendImg->start();
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

    if(_mytcpSocket ->connectToServer(ip, port, QIODevice::ReadWrite))
    {
        ui->outlog->setText("成功连接到" + ip + ":" + port);
        ui->openAudio->setDisabled(false);
        ui->openVedio->setDisabled(false);
        ui->createmeetBtn->setDisabled(false);
        ui->exitmeetBtn->setDisabled(false);
        QMessageBox::warning(this, "Connection success", "成功连接服务器" , QMessageBox::Yes, QMessageBox::Yes);

        //开启文本传输线程
        _sendText->start();
        ui->connServer->setDisabled(true);
    }
    else
    {
        ui->outlog->setText("连接失败,请重新连接...");
        QMessageBox::warning(this, "Connection error", _mytcpSocket->errorString() , QMessageBox::Yes, QMessageBox::Yes);
    }
}


void Widget::cameraError(QCamera::Error)
{
    QMessageBox::warning(this, "Camera error", _camera->errorString() , QMessageBox::Yes, QMessageBox::Yes);
}

void Widget::mytcperror(QAbstractSocket::SocketError err)
{
    if(err == QAbstractSocket::RemoteHostClosedError)
    {
        QMessageBox::warning(this, "Tcp error", _mytcpSocket->errorString() , QMessageBox::Yes, QMessageBox::Yes);
        ui->createmeetBtn->setDisabled(true);
        ui->exitmeetBtn->setDisabled(true);
        ui->connServer->setDisabled(false);
        ui->outlog->setText(QString("远程服务器关闭"));
    }
}
