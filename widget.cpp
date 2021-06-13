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
#include <QScrollBar>
#include <QHostAddress>
QRect  Widget::pos = QRect(-1, -1, -1, -1);

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{

    //qDebug() << QThread::currentThreadId();
    //创建TCPsocket

    //ui界面
    _createmeet = false;
    _openCamera = false;
    Widget::pos = QRect(0.1 * Screen::width, 0.1 * Screen::height, 0.8 * Screen::width, 0.8 * Screen::height);

    ui->setupUi(this);
    mainlabel_size = ui->mainshow_label->size();
    mainip = 0;
    this->setGeometry(Widget::pos);
    this->setMinimumSize(QSize(Widget::pos.width() * 0.7, Widget::pos.height() * 0.7));
    this->setMaximumSize(QSize(Widget::pos.width(), Widget::pos.height()));
    ui->exitmeetBtn->setDisabled(true);
    ui->joinmeetBtn->setDisabled(true);
    ui->createmeetBtn->setDisabled(true);
    ui->openAudio->setDisabled(true);
    ui->openVedio->setDisabled(true);
    //ui->openVedio->setDisabled(true);

    //-----------------------------------------------
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


    //---------启动接收数据线程-------------------------
    _recvThread = new RecvSolve();
    connect(_recvThread, SIGNAL(datarecv(MESG *)), this, SLOT(datasolve(MESG *)));
    _recvThread->start();

    //预览窗口重定向在MyVideoSurface
    _camera->setViewfinder(_myvideosurface);
    _camera->setCaptureMode(QCamera::CaptureStillImage);

    //设置滚动条
    ui->scrollArea->verticalScrollBar()->setStyleSheet("QScrollBar:vertical"
                                                       "{"
                                                       "width:8px;"
                                                       "background:rgba(0,0,0,0%);"
                                                       "margin:0px,0px,0px,0px;"
                                                       "padding-top:9px;"
                                                       "padding-bottom:9px;"
                                                       "}"
                                                       "QScrollBar::handle:vertical"
                                                       "{"
                                                       "width:8px;"
                                                       "background:rgba(0,0,0,25%);"
                                                       " border-radius:4px;"
                                                       "min-height:20;"
                                                       "}"
                                                       "QScrollBar::handle:vertical:hover"
                                                       "{"
                                                       "width:8px;"
                                                       "background:rgba(0,0,0,50%);"
                                                       " border-radius:4px;"
                                                       "min-height:20;"
                                                       "}"
                                                       "QScrollBar::add-line:vertical"
                                                       "{"
                                                       "height:9px;width:8px;"
                                                       "border-image:url(:/images/a/3.png);"
                                                       "subcontrol-position:bottom;"
                                                       "}"
                                                       "QScrollBar::sub-line:vertical"
                                                       "{"
                                                       "height:9px;width:8px;"
                                                       "border-image:url(:/images/a/1.png);"
                                                       "subcontrol-position:top;"
                                                       "}"
                                                       "QScrollBar::add-line:vertical:hover"
                                                       "{"
                                                       "height:9px;width:8px;"
                                                       "border-image:url(:/images/a/4.png);"
                                                       "subcontrol-position:bottom;"
                                                       "}"
                                                       "QScrollBar::sub-line:vertical:hover"
                                                       "{"
                                                       "height:9px;width:8px;"
                                                       "border-image:url(:/images/a/2.png);"
                                                       "subcontrol-position:top;"
                                                       "}"
                                                       "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical"
                                                       "{"
                                                       "background:rgba(0,0,0,10%);"
                                                       "border-radius:4px;"
                                                       "}"
                                                       );

}

Widget::~Widget()
{
    delete _mytcpSocket;
    delete _camera;
    delete _imagecapture;
    delete _myvideosurface;
    delete _imgThread;
    delete _sendImg;
    delete _recvThread;
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
    ui->exitmeetBtn->setDisabled(true);
    _createmeet = false;
    //-----------------------------------------
    // 关闭套接字

    //关闭各个启动线程
    if(_imgThread->isRunning())
    {
        _imgThread->quit();
    }

    if(_sendImg->isRunning())
    {
        _sendImg->quit();
    }

    //关闭socket
    _mytcpSocket->disconnectFromHost();
    ui->outlog->setText(QString("已退出会议"));
    ui->connServer->setDisabled(false);
    ui->groupBox->setTitle(QString("副屏幕"));
    QMessageBox::warning(this, "Information", "退出会议" , QMessageBox::Yes, QMessageBox::Yes);
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
        ui->openAudio->setDisabled(true);
        ui->openVedio->setDisabled(true);
        ui->createmeetBtn->setDisabled(false);
        ui->exitmeetBtn->setDisabled(true);
        ui->joinmeetBtn->setDisabled(false);
        QMessageBox::warning(this, "Connection success", "成功连接服务器" , QMessageBox::Yes, QMessageBox::Yes);

        //开启文本传输线程
        if(!_sendText->isRunning()) _sendText->start();
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
    ui->createmeetBtn->setDisabled(true);
    ui->exitmeetBtn->setDisabled(true);
    ui->connServer->setDisabled(false);
    ui->joinmeetBtn->setDisabled(true);

    if(err == QAbstractSocket::RemoteHostClosedError)
    {
        QMessageBox::warning(this, "Meeting Information", "会议结束" , QMessageBox::Yes, QMessageBox::Yes);
        _mytcpSocket->disconnectFromHost();
        ui->outlog->setText(QString("会议结束"));
    }
    else
    {
        QMessageBox::warning(this, "Network Error", "网络异常" , QMessageBox::Yes, QMessageBox::Yes);
        _mytcpSocket->disconnectFromHost();
        ui->outlog->setText(QString("网络异常......"));
    }

}


void Widget::datasolve(MESG *msg)
{
//    qDebug() << msg;
    if(msg->msg_type == CREATE_MEETING_RESPONSE)
    {
        int roomno;
        memcpy(&roomno, msg->data, msg->len);

        QMessageBox::information(this, "Room No", QString("房间号：%1").arg(roomno), QMessageBox::Yes, QMessageBox::Yes);

        ui->groupBox->setTitle(QString("房间号: %1").arg(roomno));

//        qDebug() <<"hello" << roomno;
        ui->exitmeetBtn->setDisabled(false);
        ui->openAudio->setDisabled(false);
        ui->openVedio->setDisabled(false);
        ui->joinmeetBtn->setDisabled(true);

        //添加用户自己
        addPartner(_mytcpSocket->getlocalip());
    }
    else if(msg->msg_type == IMG_RECV)
    {
        QHostAddress a(msg->ip);
        qDebug() << a.toString();
        QImage img(msg->data, msg->width, msg->height, msg->format);
        if(partner.count(msg->ip) == 1)
        {
            Partner* p = partner[msg->ip];
            p->setpic(img);
        }
        else
        {
            Partner* p = addPartner(msg->ip);
            p->setpic(img);
        }

        if(msg->ip == mainip)
        {
            ui->mainshow_label->setPixmap(QPixmap::fromImage(img).scaled(mainlabel_size));
        }
    }

    if(msg->data)
    {
        free(msg->data);
        msg->data = NULL;
    }
    if(msg)
    {
        free(msg);
        msg = NULL;
    }
}

Partner* Widget::addPartner(quint32 ip)
{
    Partner *p = new Partner(ui->scrollAreaWidgetContents ,ip);
    connect(p, SIGNAL(sendip(quint32)), this, SLOT(recvip(quint32)));
    partner[ip] =p;
    ui->verticalLayout_3->addWidget(p);
    return p;
}

void Widget::removePartner(quint32 ip)
{
    if(partner.count(ip) == 1)
    {
        Partner *p = partner[ip];
        disconnect(p, SIGNAL(sendip(quint32)), this, SLOT(recvip(quint32)));
        ui->verticalLayout_3->removeWidget(p);
        partner.remove(ip);
        delete p;
    }
}


void Widget::recvip(quint32 ip)
{
    mainip = ip;
    qDebug() << ip;
}
