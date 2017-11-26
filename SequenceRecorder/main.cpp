#include "mainwindow.h"
#include <QApplication>

Q_DECLARE_METATYPE(CapturedFrame)



int main(int argc, char *argv[])
{
    qRegisterMetaType<CapturedFrame>("CapturedFrame");

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
