#include <QApplication>
#include "widget.h"
#include "screen.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Screen::init();

    Widget w;
    w.show();
    return app.exec();
}
