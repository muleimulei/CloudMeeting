#include <QApplication>
#include "widget.h"
#include "screen.h"
#include <QTextCodec>
int main(int argc, char* argv[])
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF8"));
    QApplication app(argc, argv);
    Screen::init();

    Widget w;
    w.show();
    return app.exec();
}
