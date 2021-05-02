#ifndef MYVIDEOSURFACE_H
#define MYVIDEOSURFACE_H

#include <QAbstractVideoSurface>


class MyVideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    MyVideoSurface(QObject *parent = 0);

    //支持的像素格式
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const override;

    //检测视频流的格式是否合法，返回bool
    bool isFormatSupported(const QVideoSurfaceFormat &format) const override;
    bool start(const QVideoSurfaceFormat &format) override;
    bool present(const QVideoFrame &frame) override;
    QRect videoRect() const;
    void updateVideoRect();
    void paint(QPainter *painter);

//private:
//    QWidget      * widget_;
//    QImage::Format imageFormat_;
//    QRect          targetRect_;
//    QSize          imageSize_;
//    QVideoFrame    currentFrame_;

signals:
    void frameAvailable(QVideoFrame);

};

#endif // MYVIDEOSURFACE_H
