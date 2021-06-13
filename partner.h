#ifndef PARTNER_H
#define PARTNER_H

#include <QLabel>

class Partner : public QLabel
{
    Q_OBJECT
private:
    quint32 ip;

    void mousePressEvent(QMouseEvent *ev) override;
    int w;
public:
    Partner(QWidget * parent = nullptr, quint32 = 0);
    void setpic(QImage img);
signals:
    void sendip(quint32); //发送ip
};

#endif // PARTNER_H
